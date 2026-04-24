/*
 * Minimal Node.js WebSocket demo server for js_sdk/demo.html.
 * Features:
 * - Accept WebSocket clients
 * - Reply pong on ping
 * - Reply ack on payload
 * - Push ARM_STATUS_RSP when receiving ARM_STATUS_QUERY
 */

const { WebSocketServer } = require("ws");
const protocol = require("./protocol");

const PORT = Number(process.env.PORT || 8080);
const PATH = process.env.WS_PATH || "/ws";

function nowTs() {
  return Date.now();
}

function uuidV4() {
  if (typeof crypto !== "undefined" && crypto.randomUUID) {
    return crypto.randomUUID();
  }
  return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, function (c) {
    const r = (Math.random() * 16) | 0;
    const v = c === "x" ? r : (r & 0x3) | 0x8;
    return v.toString(16);
  });
}

function createMessage({
  v = "1.0",
  type,
  api,
  msg_id,
  identity = "DEMO-SERVER-0001",
  code = 0,
  body = {},
  ext = {},
  ts = nowTs(),
}) {
  return { v, type, api, msg_id, identity, code, body, ext, ts };
}

function safeParse(text) {
  try {
    return { ok: true, value: JSON.parse(text) };
  } catch (error) {
    return { ok: false, error };
  }
}

function bytesToHex(bytes) {
  return bytes
    .map((v) => (Number(v) & 0xff).toString(16).padStart(2, "0"))
    .join(" ")
    .toUpperCase();
}

function parseHexToBytes(hexText) {
  if (typeof hexText !== "string") {
    throw new Error("data_hex must be string");
  }
  const clean = hexText.replace(/[^0-9a-fA-F]/g, " ").trim();
  if (!clean) throw new Error("data_hex empty");
  const parts = clean.split(/\s+/).filter(Boolean);
  if (parts.length !== 8) {
    throw new Error("data_hex must contain exactly 8 bytes");
  }
  return parts.map((p) => {
    const n = Number.parseInt(p, 16);
    if (Number.isNaN(n) || n < 0 || n > 255) throw new Error("invalid hex byte");
    return n;
  });
}

function decodeByFrameType(frameType, bytes) {
  switch (frameType) {
    case "switch":
      return protocol.decodeAiToPlcSwitch(bytes);
    case "status":
      return protocol.decodePlcToAiStatus(bytes);
    case "analog":
      return protocol.decodeAiToPlcAnalog(bytes);
    case "telemetry_300":
      return protocol.decodePlcToAiTelemetry300(bytes);
    case "telemetry_400":
      return protocol.decodePlcToAiTelemetry400(bytes);
    default:
      throw new Error("unsupported frame_type");
  }
}

function encodeByFrameType(frameType, fields) {
  switch (frameType) {
    case "switch":
      return protocol.encodeAiToPlcSwitch(fields || {});
    case "analog":
      return protocol.encodeAiToPlcAnalog(fields || {});
    case "telemetry_300":
      return protocol.encodePlcToAiTelemetry300(fields || {});
    case "telemetry_400":
      return protocol.encodePlcToAiTelemetry400(fields || {});
    default:
      throw new Error("unsupported frame_type");
  }
}

const wss = new WebSocketServer({
  port: PORT,
  path: PATH,
});

console.log(`[demo_server] ws://127.0.0.1:${PORT}${PATH}`);

wss.on("connection", (ws, req) => {
  const remote = req.socket.remoteAddress;
  console.log(`[demo_server] client connected: ${remote}`);

  ws.on("message", (data) => {
    const text = data.toString();
    const parsed = safeParse(text);

    if (!parsed.ok) {
      const errorAck = createMessage({
        type: "ack",
        api: "UNKNOWN",
        msg_id: uuidV4(),
        code: 1001,
        body: {
          ack_msg_id: "",
          error: "invalid_json",
        },
      });
      ws.send(JSON.stringify(errorAck));
      return;
    }

    const msg = parsed.value;
    if (!msg || !msg.type || !msg.msg_id) {
      const errorAck = createMessage({
        type: "ack",
        api: msg && msg.api ? msg.api : "UNKNOWN",
        msg_id: uuidV4(),
        code: 1001,
        body: {
          ack_msg_id: msg && msg.msg_id ? msg.msg_id : "",
          error: "invalid_envelope",
        },
      });
      ws.send(JSON.stringify(errorAck));
      return;
    }

    if (msg.type === "ping") {
      const pong = createMessage({
        type: "pong",
        api: msg.api || "SYS_HEARTBEAT",
        msg_id: uuidV4(),
        code: 0,
        body: {},
        ext: {
          echo_msg_id: msg.msg_id,
        },
      });
      ws.send(JSON.stringify(pong));
      return;
    }

    if (msg.type === "payload") {
      const ack = createMessage({
        type: "ack",
        api: msg.api,
        msg_id: uuidV4(),
        code: 0,
        body: {
          ack_msg_id: msg.msg_id,
        },
      });
      ws.send(JSON.stringify(ack));

      if (msg.api === "ARM_STATUS_QUERY") {
        const rsp = createMessage({
          type: "payload",
          api: "ARM_STATUS_RSP",
          msg_id: uuidV4(),
          code: 0,
          body: {
            robot_id: (msg.body && msg.body.robot_id) || "RB001",
            arm_length: 38.5,
            arm_angle: 54.2,
            slew_angle: 112.0,
            hook_height: 18.3,
            work_mode: "AUTO",
            status_time: new Date().toISOString().replace("T", " ").replace("Z", ""),
          },
        });

        setTimeout(() => {
          if (ws.readyState === ws.OPEN) {
            ws.send(JSON.stringify(rsp));
          }
        }, 300);
      }

      if (msg.api === "CAN_DEBUG_DECODE") {
        try {
          const frameType = msg.body && msg.body.frame_type;
          const bytes = parseHexToBytes(msg.body && msg.body.data_hex);
          const decoded = decodeByFrameType(frameType, bytes);

          const rsp = createMessage({
            type: "payload",
            api: "CAN_DEBUG_DECODE_RSP",
            msg_id: uuidV4(),
            code: 0,
            body: {
              frame_type: frameType,
              bytes,
              data_hex: bytesToHex(bytes),
              decoded,
            },
          });
          ws.send(JSON.stringify(rsp));
        } catch (err) {
          const rsp = createMessage({
            type: "payload",
            api: "CAN_DEBUG_DECODE_RSP",
            msg_id: uuidV4(),
            code: 1001,
            body: {
              error: err.message,
            },
          });
          ws.send(JSON.stringify(rsp));
        }
      }

      if (msg.api === "CAN_DEBUG_ENCODE") {
        try {
          const frameType = msg.body && msg.body.frame_type;
          const fields = (msg.body && msg.body.fields) || {};
          const bytes = encodeByFrameType(frameType, fields);

          const rsp = createMessage({
            type: "payload",
            api: "CAN_DEBUG_ENCODE_RSP",
            msg_id: uuidV4(),
            code: 0,
            body: {
              frame_type: frameType,
              fields,
              bytes,
              data_hex: bytesToHex(bytes),
            },
          });
          ws.send(JSON.stringify(rsp));
        } catch (err) {
          const rsp = createMessage({
            type: "payload",
            api: "CAN_DEBUG_ENCODE_RSP",
            msg_id: uuidV4(),
            code: 1001,
            body: {
              error: err.message,
            },
          });
          ws.send(JSON.stringify(rsp));
        }
      }

      return;
    }

    if (msg.type === "ack") {
      // Server receives client ack; no further action in demo mode.
      return;
    }
  });

  ws.on("close", () => {
    console.log(`[demo_server] client disconnected: ${remote}`);
  });

  ws.on("error", (err) => {
    console.error("[demo_server] socket error", err.message);
  });
});
