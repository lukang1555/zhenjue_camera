/*
 * WebSocket SDK for crane auto-navigation business protocol.
 * Features:
 * - Auto heartbeat ping/pong
 * - Auto ack for incoming payload
 * - Auto reconnect with backoff
 * - Ack timeout callback for outgoing messages
 * - Business message send/receive by API and message type
 */

(function (globalFactory) {
  if (typeof module !== "undefined" && module.exports) {
    module.exports = globalFactory();
  } else {
    window.CraneWebSocketSDK = globalFactory();
  }
})(function () {
  "use strict";

  function nowTs() {
    return Date.now();
  }

  function uuidV4() {
    if (typeof crypto !== "undefined" && crypto.randomUUID) {
      return crypto.randomUUID();
    }

    // Fallback UUID v4 generator for environments without crypto.randomUUID.
    return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, function (c) {
      var r = (Math.random() * 16) | 0;
      var v = c === "x" ? r : (r & 0x3) | 0x8;
      return v.toString(16);
    });
  }

  function createMessage(params) {
    // Keep protocol field order stable.
    return {
      v: params.v,
      type: params.type,
      api: params.api,
      msg_id: params.msg_id,
      identity: params.identity,
      code: params.code,
      body: params.body,
      ext: params.ext,
      ts: params.ts,
    };
  }

  function safeJsonParse(text) {
    try {
      return { ok: true, value: JSON.parse(text) };
    } catch (err) {
      return { ok: false, error: err };
    }
  }

  function noop() {}

  function mergeDefaults(options) {
    var defaults = {
      url: "",
      identity: "",
      version: "1.0",
      heartbeatApi: "SYS_HEARTBEAT",
      pingIntervalMs: 5000,
      pongTimeoutMs: 30000,
      ackTimeoutMs: 3000,
      reconnectBackoffMs: [1000, 2000, 4000, 8000, 16000, 30000],
      maxReconnectAttempts: Infinity,
      autoReconnect: true,
      autoAck: true,
      autoPong: true,
      debug: false,
      protocols: undefined,
      WebSocketImpl: typeof WebSocket !== "undefined" ? WebSocket : undefined,
      onOpen: noop,
      onClose: noop,
      onError: noop,
      onReconnectAttempt: noop,
      onAckTimeout: noop,
      onAckCodeError: noop,
      onMessage: noop,
      onPayload: noop,
      onAck: noop,
      onPing: noop,
      onPong: noop,
      onProtocolError: noop,
    };

    var out = {};
    var key;
    for (key in defaults) out[key] = defaults[key];
    if (options) {
      for (key in options) out[key] = options[key];
    }
    return out;
  }

  function CraneWebSocketClient(options) {
    this.options = mergeDefaults(options);
    this.ws = null;
    this.connected = false;
    this.manualClosed = false;
    this.reconnectAttempts = 0;

    this.lastPongAt = 0;
    this.lastPingAt = 0;

    this.pingTimer = null;
    this.pongGuardTimer = null;

    this.pendingAckMap = new Map();
    this.apiHandlers = new Map();
    this.typeHandlers = new Map();

    this._onSocketOpen = this._onSocketOpen.bind(this);
    this._onSocketClose = this._onSocketClose.bind(this);
    this._onSocketError = this._onSocketError.bind(this);
    this._onSocketMessage = this._onSocketMessage.bind(this);
  }

  CraneWebSocketClient.prototype._log = function () {
    if (!this.options.debug) return;
    var args = Array.prototype.slice.call(arguments);
    args.unshift("[CraneWebSocketClient]");
    // eslint-disable-next-line no-console
    console.log.apply(console, args);
  };

  CraneWebSocketClient.prototype.connect = function () {
    if (!this.options.WebSocketImpl) {
      throw new Error("WebSocket implementation not found. Please provide options.WebSocketImpl.");
    }
    if (!this.options.url) {
      throw new Error("options.url is required.");
    }
    if (!this.options.identity) {
      throw new Error("options.identity is required.");
    }

    this.manualClosed = false;

    if (this.ws && (this.ws.readyState === 0 || this.ws.readyState === 1)) {
      this._log("connect skipped: socket already connecting/open");
      return;
    }

    this._log("connecting to", this.options.url);
    this.ws = new this.options.WebSocketImpl(this.options.url, this.options.protocols);

    this.ws.addEventListener("open", this._onSocketOpen);
    this.ws.addEventListener("close", this._onSocketClose);
    this.ws.addEventListener("error", this._onSocketError);
    this.ws.addEventListener("message", this._onSocketMessage);
  };

  CraneWebSocketClient.prototype.disconnect = function (code, reason) {
    this.manualClosed = true;
    this._stopHeartbeat();
    this._clearPendingAck("manual_disconnect");

    if (this.ws) {
      try {
        this.ws.close(code || 1000, reason || "manual_disconnect");
      } catch (err) {
        this._log("socket close error", err);
      }
    }
  };

  CraneWebSocketClient.prototype.isConnected = function () {
    return this.connected;
  };

  CraneWebSocketClient.prototype._onSocketOpen = function () {
    this.connected = true;
    this.reconnectAttempts = 0;
    this.lastPongAt = nowTs();
    this._startHeartbeat();
    this.options.onOpen();
    this._log("connected");
  };

  CraneWebSocketClient.prototype._onSocketClose = function (event) {
    this.connected = false;
    this._stopHeartbeat();
    this.options.onClose(event);
    this._log("closed", event && event.code, event && event.reason);

    if (!this.manualClosed && this.options.autoReconnect) {
      this._scheduleReconnect();
    }
  };

  CraneWebSocketClient.prototype._onSocketError = function (event) {
    this.options.onError(event);
    this._log("socket error", event);
  };

  CraneWebSocketClient.prototype._onSocketMessage = function (event) {
    var parsed = safeJsonParse(event.data);
    if (!parsed.ok) {
      this.options.onProtocolError({
        reason: "invalid_json",
        raw: event.data,
        error: parsed.error,
      });
      return;
    }

    var msg = parsed.value;
    this.options.onMessage(msg);

    if (!msg || typeof msg !== "object" || !msg.type || !msg.msg_id) {
      this.options.onProtocolError({
        reason: "invalid_envelope",
        message: msg,
      });
      return;
    }

    this._dispatchType(msg.type, msg);

    if (msg.type === "payload") {
      this.options.onPayload(msg);
      this._dispatchApi(msg.api, msg);
      if (this.options.autoAck) {
        this._sendAck(msg);
      }
      return;
    }

    if (msg.type === "ack") {
      this.options.onAck(msg);
      this._handleAck(msg);
      return;
    }

    if (msg.type === "ping") {
      this.options.onPing(msg);
      if (this.options.autoPong) {
        this._sendPong(msg);
      }
      return;
    }

    if (msg.type === "pong") {
      this.lastPongAt = nowTs();
      this.options.onPong(msg);
      return;
    }
  };

  CraneWebSocketClient.prototype._dispatchApi = function (api, message) {
    if (!api) return;
    var handlers = this.apiHandlers.get(api);
    if (!handlers) return;
    handlers.forEach(function (handler) {
      try {
        handler(message);
      } catch (err) {
        // eslint-disable-next-line no-console
        console.error("api handler error", api, err);
      }
    });
  };

  CraneWebSocketClient.prototype._dispatchType = function (type, message) {
    if (!type) return;
    var handlers = this.typeHandlers.get(type);
    if (!handlers) return;
    handlers.forEach(function (handler) {
      try {
        handler(message);
      } catch (err) {
        // eslint-disable-next-line no-console
        console.error("type handler error", type, err);
      }
    });
  };

  CraneWebSocketClient.prototype.onApi = function (api, handler) {
    if (!this.apiHandlers.has(api)) {
      this.apiHandlers.set(api, new Set());
    }
    this.apiHandlers.get(api).add(handler);

    var self = this;
    return function unsubscribe() {
      var set = self.apiHandlers.get(api);
      if (!set) return;
      set.delete(handler);
      if (set.size === 0) self.apiHandlers.delete(api);
    };
  };

  CraneWebSocketClient.prototype.onType = function (type, handler) {
    if (!this.typeHandlers.has(type)) {
      this.typeHandlers.set(type, new Set());
    }
    this.typeHandlers.get(type).add(handler);

    var self = this;
    return function unsubscribe() {
      var set = self.typeHandlers.get(type);
      if (!set) return;
      set.delete(handler);
      if (set.size === 0) self.typeHandlers.delete(type);
    };
  };

  CraneWebSocketClient.prototype.sendpayload = function (api, body, options) {
    options = options || {};

    var message = createMessage({
      v: this.options.version,
      type: "payload",
      api: api,
      msg_id: options.msg_id || uuidV4(),
      identity: this.options.identity,
      code: typeof options.code === "number" ? options.code : 0,
      body: body || {},
      ext: options.ext || {},
      ts: typeof options.ts === "number" ? options.ts : nowTs(),
    });

    this._sendRaw(message);

    if (options.expectAck === false) {
      return Promise.resolve({ sent: true, msg_id: message.msg_id });
    }

    return this._waitAck(message, options.ackTimeoutMs);
  };

  CraneWebSocketClient.prototype.sendPing = function () {
    this.lastPingAt = nowTs();
    var msg = createMessage({
      v: this.options.version,
      type: "ping",
      api: this.options.heartbeatApi,
      msg_id: uuidV4(),
      identity: this.options.identity,
      code: 0,
      body: {},
      ext: {},
      ts: nowTs(),
    });
    this._sendRaw(msg);
    return msg.msg_id;
  };

  CraneWebSocketClient.prototype._sendAck = function (incoming) {
    var ack = createMessage({
      v: this.options.version,
      type: "ack",
      api: incoming.api,
      msg_id: uuidV4(),
      identity: this.options.identity,
      code: 0,
      body: {
        ack_msg_id: incoming.msg_id,
      },
      ext: {},
      ts: nowTs(),
    });
    this._sendRaw(ack);
  };

  CraneWebSocketClient.prototype._sendPong = function (incomingPing) {
    var pong = createMessage({
      v: this.options.version,
      type: "pong",
      api: this.options.heartbeatApi,
      msg_id: uuidV4(),
      identity: this.options.identity,
      code: 0,
      body: {},
      ext: {
        echo_msg_id: incomingPing.msg_id,
      },
      ts: nowTs(),
    });
    this._sendRaw(pong);
  };

  CraneWebSocketClient.prototype._sendRaw = function (message) {
    if (!this.ws || this.ws.readyState !== 1) {
      throw new Error("socket_not_connected");
    }
    this.ws.send(JSON.stringify(message));
  };

  CraneWebSocketClient.prototype._waitAck = function (message, ackTimeoutMs) {
    var self = this;
    var timeout = typeof ackTimeoutMs === "number" ? ackTimeoutMs : this.options.ackTimeoutMs;

    return new Promise(function (resolve, reject) {
      var timer = setTimeout(function () {
        self.pendingAckMap.delete(message.msg_id);
        var timeoutError = {
          reason: "ack_timeout",
          msg_id: message.msg_id,
          ack_msg_id: message.msg_id,
          api: message.api,
          timeout_ms: timeout,
          message: message,
        };
        self.options.onAckTimeout(timeoutError);
        reject(timeoutError);
      }, timeout);

      self.pendingAckMap.set(message.msg_id, {
        resolve: resolve,
        reject: reject,
        timer: timer,
        message: message,
      });
    });
  };

  CraneWebSocketClient.prototype._handleAck = function (ackMessage) {
    var ackMsgId = ackMessage && ackMessage.body && ackMessage.body.ack_msg_id;
    if (!ackMsgId) return;

    var pending = this.pendingAckMap.get(ackMsgId);
    if (!pending) return;

    clearTimeout(pending.timer);
    this.pendingAckMap.delete(ackMsgId);

    if (typeof ackMessage.code === "number" && ackMessage.code !== 0) {
      var ackCodeError = {
        reason: "ack_code_non_zero",
        msg_id: ackMsgId,
        api: pending.message.api,
        ack_code: ackMessage.code,
        ack: ackMessage,
        request: pending.message,
      };
      this.options.onAckCodeError(ackCodeError);
      pending.reject(ackCodeError);
      return;
    }

    pending.resolve({
      ack: ackMessage,
      request: pending.message,
    });
  };

  CraneWebSocketClient.prototype._startHeartbeat = function () {
    var self = this;
    this._stopHeartbeat();

    this.pingTimer = setInterval(function () {
      if (!self.connected) return;
      try {
        self.sendPing();
      } catch (err) {
        self._log("send ping failed", err);
      }
    }, this.options.pingIntervalMs);

    this.pongGuardTimer = setInterval(function () {
      if (!self.connected) return;
      var now = nowTs();
      if (now - self.lastPongAt > self.options.pongTimeoutMs) {
        self._log("pong timeout, closing socket");
        try {
          self.ws.close(4000, "pong_timeout");
        } catch (err) {
          self._log("close on pong timeout failed", err);
        }
      }
    }, Math.min(this.options.pingIntervalMs, 1000));
  };

  CraneWebSocketClient.prototype._stopHeartbeat = function () {
    if (this.pingTimer) {
      clearInterval(this.pingTimer);
      this.pingTimer = null;
    }
    if (this.pongGuardTimer) {
      clearInterval(this.pongGuardTimer);
      this.pongGuardTimer = null;
    }
  };

  CraneWebSocketClient.prototype._scheduleReconnect = function () {
    if (this.reconnectAttempts >= this.options.maxReconnectAttempts) {
      this._log("max reconnect attempts reached");
      return;
    }

    var idx = Math.min(this.reconnectAttempts, this.options.reconnectBackoffMs.length - 1);
    var delay = this.options.reconnectBackoffMs[idx];
    this.reconnectAttempts += 1;

    this.options.onReconnectAttempt({
      attempt: this.reconnectAttempts,
      delay_ms: delay,
    });

    var self = this;
    setTimeout(function () {
      if (self.manualClosed) return;
      self.connect();
    }, delay);
  };

  CraneWebSocketClient.prototype._clearPendingAck = function (reason) {
    this.pendingAckMap.forEach(function (pending, msgId) {
      clearTimeout(pending.timer);
      pending.reject({
        reason: reason || "cleared",
        ack_msg_id: msgId,
      });
    });
    this.pendingAckMap.clear();
  };

  return {
    CraneWebSocketClient: CraneWebSocketClient,
  };
});
