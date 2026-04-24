# js_sdk WebSocket 封装说明

## 功能覆盖

1. WebSocket 协议统一信封消息发送
2. 自动定时心跳（ping）
3. 自动响应 ping（pong）
4. 自动回复 ack（收到 payload 即回）
5. 断线自动重连（退避重连）
6. 发送 payload 后等待 ack，超时触发异常回调
7. 业务消息按 `api` 与 `type` 进行接收分发

## 文件

- `index.js`: SDK 实现文件
- `demo_server.js`: Node.js 最小联调服务端
- `demo.html`: 浏览器一键联调页面
- `protocol/constants.js`: PLC 指令语义化常量映射
- `protocol/codec.js`: PLC 报文编解码工具
- `protocol/index.js`: protocol 模块统一导出

## 快速使用

```javascript
const { CraneWebSocketClient } = require("./index");

const sdk = new CraneWebSocketClient({
  url: "wss://your-server/ws",
  identity: "PAD-DEVICE-0001",
  pingIntervalMs: 5000,
  pongTimeoutMs: 30000,
  ackTimeoutMs: 3000,
  onAckTimeout: (err) => {
    console.error("ACK timeout", err);
    // err.msg_id 可用于定位超时消息
  },
  onAckCodeError: (err) => {
    console.error("ACK code non-zero", err);
  },
  onReconnectAttempt: (info) => {
    console.log("reconnect", info);
  },
});

sdk.connect();
```

## 发送业务消息

```javascript
sdk
  .sendpayload("ARM_AUTO_JOB_REQ", {
    task_id: "TASK202604120001",
    job_flag: "START",
  })
  .then(({ ack }) => {
    // ack.code 为通讯层状态码，0 表示成功
    console.log("ack received", ack.code, ack);
  })
  .catch((err) => {
    console.error("send failed / ack timeout / ack code non-zero", err);
  });
```

## 接收业务消息

按 `api` 接收：

```javascript
const offStatusRsp = sdk.onApi("ARM_STATUS_RSP", (msg) => {
  console.log("status response", msg.body);
});

// 不再监听时调用
// offStatusRsp();
```

按 `type` 接收：

```javascript
const offPayload = sdk.onType("payload", (msg) => {
  console.log("payload message", msg.api, msg.body);
});
```

## 默认行为

1. 收到 `payload` 自动发送 `ack`
2. 收到 `ping` 自动发送 `pong`
3. 发送 `payload` 默认等待 `ack`
4. 若在 `ackTimeoutMs` 内未收到对应 `ack_msg_id`，触发 `onAckTimeout`，并携带 `msg_id`
5. 若收到 `ack` 但 `ack.code != 0`，触发 `onAckCodeError`，同时 `sendpayload` Promise reject
6. 连接断开后按 `reconnectBackoffMs` 自动重连

## 对外发送 API

- 仅保留：`sendpayload(api, body, options)`

## 可配置项

- `url`: WebSocket 地址
- `identity`: 发送方标识
- `version`: 协议版本，默认 `1.0`
- `heartbeatApi`: 心跳 API，默认 `SYS_HEARTBEAT`
- `pingIntervalMs`: 心跳间隔，默认 5000
- `pongTimeoutMs`: pong 超时，默认 30000
- `ackTimeoutMs`: ack 超时，默认 3000
- `reconnectBackoffMs`: 重连退避数组，默认 `[1000,2000,4000,8000,16000,30000]`
- `maxReconnectAttempts`: 最大重连次数，默认 `Infinity`
- `autoReconnect`: 是否自动重连
- `autoAck`: 是否自动 ack
- `autoPong`: 是否自动 pong
- `debug`: 是否输出调试日志
- `WebSocketImpl`: 自定义 WebSocket 实现（Node 环境可注入）
- `onAckCodeError`: ACK 通讯 code 非 0 回调

## 说明

SDK 中 `code` 字段用于通讯层状态，不承载业务状态码。

## PLC 协议封装模块

```javascript
const protocol = require("./protocol");

// 1) 将业务开关指令编码为 8 字节（0x180+node_id）
const switchBytes = protocol.encodeAiToPlcSwitch({
  cmd_enable_start: true,
  cmd_horn: true,
  cmd_clamp_close: true,
});

// 2) 解析 PLC 状态位（0x200+node_id）
const status = protocol.decodePlcToAiStatus([0x1b, 0, 0, 0, 0, 0, 0, 0]);

// 3) 解析 0x300 遥测量
const telemetry300 = protocol.decodePlcToAiTelemetry300([0x2c, 0x01, 0xf4, 0x01, 0x10, 0x00, 0xf0, 0xff]);
```

主要能力：

1. `encodeAiToPlcSwitch` / `decodeAiToPlcSwitch`
2. `encodeAiToPlcAnalog` / `decodeAiToPlcAnalog`
3. `decodePlcToAiStatus`
4. `decodePlcToAiTelemetry300` / `decodePlcToAiTelemetry400`
5. `encodePlcToAiTelemetry300` / `encodePlcToAiTelemetry400`

### #sym:encodeAiToPlcAnalog 使用案例

```javascript
const protocol = require("./protocol");

const analogBytes = protocol.encodeAiToPlcAnalog({
  cmd_main_boom_luff_raw: 0x60,
  cmd_turntable_rotate_raw: 0x7f,
  cmd_wheel_steer_raw: 0x90,
  cmd_drive_motion_raw: 0x80,
  cmd_reserved_5_raw: 0x00,
  cmd_reserved_6_raw: 0x00,
  cmd_reserved_7_raw: 0x00,
  cmd_reserved_8_raw: 0x00,
});

console.log(analogBytes);
// [96, 127, 144, 128, 0, 0, 0, 0]

const decoded = protocol.decodeAiToPlcAnalog(analogBytes);
console.log(decoded);
// {
//   cmd_main_boom_luff_raw: 96,
//   cmd_turntable_rotate_raw: 127,
//   cmd_wheel_steer_raw: 144,
//   cmd_drive_motion_raw: 128,
//   cmd_reserved_5_raw: 0,
//   cmd_reserved_6_raw: 0,
//   cmd_reserved_7_raw: 0,
//   cmd_reserved_8_raw: 0
// }
```

### #sym:encodePlcToAiTelemetry300 使用案例

```javascript
const protocol = require("./protocol");

const bytes300 = protocol.encodePlcToAiTelemetry300({
  fb_main_boom_angle_deg: 12.3,
  fb_main_boom_length_m: 45.6,
  fb_chassis_tilt_x_deg: -1.5,
  fb_chassis_tilt_y_deg: 0.8,
});

console.log(bytes300);
// 示例输出（按 int16 小端 + scale=0.1）：
// [123, 0, 200, 1, 241, 255, 8, 0]

const parsed300 = protocol.decodePlcToAiTelemetry300(bytes300);
console.log(parsed300);
// {
//   fb_main_boom_angle_raw: 123,
//   fb_main_boom_angle_deg: 12.3,
//   fb_main_boom_length_raw: 456,
//   fb_main_boom_length_m: 45.6,
//   fb_chassis_tilt_x_raw: -15,
//   fb_chassis_tilt_x_deg: -1.5,
//   fb_chassis_tilt_y_raw: 8,
//   fb_chassis_tilt_y_deg: 0.8
// }
```

### #sym:decodePlcToAiTelemetry400 使用案例

```javascript
const protocol = require("./protocol");

// 平台重量=123.4kg，飞臂角=25.0°，夹具角=-8.5°，预留=100
const telemetry400Bytes = [210, 4, 250, 0, 171, 255, 100, 0];

const parsed400 = protocol.decodePlcToAiTelemetry400(telemetry400Bytes);
console.log(parsed400);
// {
//   fb_platform_weight_raw: 1234,
//   fb_platform_weight_kg: 123.4,
//   fb_fly_boom_angle_raw: 250,
//   fb_fly_boom_angle_deg: 25,
//   fb_clamp_rotate_angle_raw: -85,
//   fb_clamp_rotate_angle_deg: -8.5,
//   fb_reserved_7_8_raw: 100,
//   fb_reserved_7_8_value: 100
// }
```

### #sym:encodePlcToAiTelemetry400 使用案例

```javascript
const protocol = require("./protocol");

const bytes400 = protocol.encodePlcToAiTelemetry400({
  fb_platform_weight_kg: 88.8,
  fb_fly_boom_angle_deg: 31.2,
  fb_clamp_rotate_angle_deg: 14.6,
  fb_reserved_7_8_value: 20,
});

console.log(bytes400);
// [120, 3, 56, 1, 146, 0, 20, 0]

const verify400 = protocol.decodePlcToAiTelemetry400(bytes400);
console.log(verify400);
// 结果应与输入工程值保持一致（受四舍五入影响）
```

## Node.js 联调服务（最小可运行）

在 `js_sdk` 目录执行：

```bash
npm install
npm run demo:server
```

默认服务地址：`ws://127.0.0.1:8080/ws`

联调步骤：

1. 启动 Node.js 服务端
2. 浏览器打开 `demo.html`
3. URL 保持 `ws://127.0.0.1:8080/ws`
4. 点击“一键联调”

默认行为：

1. 服务端对 `payload` 返回 `ack`
2. 服务端对 `ping` 返回 `pong`
3. 服务端收到 `ARM_STATUS_QUERY` 后，会额外推送一条 `ARM_STATUS_RSP`

## CAN 十六进制联调 API

服务端内置两个调试 API，可直接通过 `sendpayload` 调用。

### 1) CAN_DEBUG_DECODE

请求：

```javascript
sdk.sendpayload("CAN_DEBUG_DECODE", {
  frame_type: "switch", // switch | status | analog | telemetry_300 | telemetry_400
  data_hex: "01 02 00 00 00 00 00 80"
});
```

响应 `api = CAN_DEBUG_DECODE_RSP`：

```json
{
  "frame_type": "switch",
  "bytes": [1,2,0,0,0,0,0,128],
  "data_hex": "01 02 00 00 00 00 00 80",
  "decoded": {
    "cmd_light_enable": true,
    "cmd_horn": true,
    "status_emergency_or_comm_lost": true
  }
}
```

### 2) CAN_DEBUG_ENCODE

请求：

```javascript
sdk.sendpayload("CAN_DEBUG_ENCODE", {
  frame_type: "switch", // switch | status | analog | telemetry_300 | telemetry_400
  fields: {
    cmd_enable_start: true,
    cmd_clamp_close: true
  }
});
```

响应 `api = CAN_DEBUG_ENCODE_RSP`：

```json
{
  "frame_type": "switch",
  "fields": {
    "cmd_enable_start": true,
    "cmd_clamp_close": true
  },
  "bytes": [0,5,0,0,0,0,0,0],
  "data_hex": "00 05 00 00 00 00 00 00"
}
```
