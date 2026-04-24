# 高空机械臂视觉自动导航业务消息文档

- 文档版本：V2.0
- 更新日期：2026-04-12
- 适用协议：WebSocket 通讯协议规范 V1.0
- 数据格式：JSON（UTF-8）
- 文档定位：业务消息层定义（接口语义、业务字段、消息示例）

## 1. 文档范围

本文档用于定义自动作业、状态、控制、地图、安全和日志相关的业务消息。所有消息必须遵循 WebSocket 通讯协议规范中的统一外层结构。

## 2. 通用消息结构

### 2.1 外层字段顺序（固定）

1. `v`
2. `type`
3. `api`
4. `msg_id`
5. `identity`
6. `code`
7. `body`
8. `ext`
9. `ts`

### 2.2 外层字段定义

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| v | string | 是 | 协议版本，固定 `1.0` |
| type | string | 是 | `payload`/`ack`/`ping`/`pong` |
| api | string | 是 | 业务接口标识 |
| msg_id | string | 是 | 全局唯一消息 ID |
| identity | string | 是 | 设备 ID 或令牌标识 |
| code | integer | 是 | 通讯层状态码，业务结果不使用该字段 |
| body | object | 是 | 业务消息数据 |
| ext | object | 否 | 扩展字段，默认 `{}` |
| ts | integer | 是 | Unix 毫秒时间戳 |

### 2.3 业务字段命名规范

1. `body` 内业务字段统一使用小写下划线命名。
2. 多单词字段必须使用下划线分隔。
3. 禁止使用驼峰命名。

### 2.4 返回规则

1. 业务消息请求使用 `type = payload`。
2. 通讯层返回使用 `type = ack`。
3. `ack.body` 必须包含 `ack_msg_id`，值为对应请求的 `msg_id`。
4. `ack` 表示已接收，不表示业务处理完成。

## 3. 接口目录

1. 机械臂自动作业请求
2. 机械臂自动作业响应
3. 机械臂状态查询
4. 机械臂状态响应
5. 机械臂控制指令
6. 地图与障碍物查询
7. 地图与障碍物响应
8. 机械臂安全设置
9. 机械臂日志查询
10. 机械臂日志查询响应
11. 机械臂控制授权请求

## 4. 接口明细

## 4.1 机械臂自动作业请求

### 功能描述
由 PAD 或调度侧发起自动作业任务，请求运控进入自动作业流程并绑定目标作业参数。

### API 标识
`ARM_AUTO_JOB_REQ`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| task_id | 是 | string | 作业任务 ID，iPad业务端自定义 |
| task_type | 是 | string | 作业类型：`PICK` / `LAND` / `RESET` |
| job_flag | 是 | string | 作业开关：`START` / `STOP` |
| job_mode | 否 | string | 作业模式，如 `AUTO`/`SEMI_AUTO`，预留字段支持自动抓取和放置动作 |
| target_point | 否 | object | 目标点 `{x,y,z}`, 预留字段支持从平板发起指定作业目标点 |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_AUTO_JOB_REQ",
  "msg_id": "65fa97a8-e282-459e-952a-6cd3bf6f6088",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "task_id": "TASK202604120001",
    "task_type": "PICK",
    "job_flag": "START",
    "job_mode": "SEMI_AUTO",
    "target_point": {"x": 16.2, "y": 8.4, "z": 14.1}
  },
  "ext": {},
  "ts": 1775972400000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_AUTO_JOB_REQ",
  "msg_id": "81e19120-31b2-4dca-835b-536d94e1789a",
  "identity": "ALG-CORE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "65fa97a8-e282-459e-952a-6cd3bf6f6088"
  },
  "ext": {},
  "ts": 1775972400020
}
```

### 特殊说明
- 自动作业请求的业务执行结果由“机械臂自动作业响应”提供。

---

## 4.2 机械臂自动作业响应

### 功能描述
由运控/机械臂侧返回自动作业业务处理结果、执行状态和失败原因。特殊场景如iPad发起急停指令后会主动响应 `INTERRUPTED` 状态，并停止自动作业。

### API 标识
`ARM_AUTO_JOB_RSP`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| task_id | 是 | string | 作业任务 ID |
| task_type | 是 | string | 作业类型：`PICK` / `LAND` / `RESET` |
| job_status | 是 | string | `ACCEPTED` / `RUNNING` / `INTERRUPTED` / `DONE` / `FAILED` |
| description | 否 | string | 业务结果描述 |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_AUTO_JOB_RSP",
  "msg_id": "0fed4c91-019c-46d8-aa3a-2223bf97a8b6",
  "identity": "ROBOT-CTRL-0001",
  "code": 0,
  "body": {
    "task_id": "TASK202604120001",
    "task_type": "PICK",
    "job_status": "RUNNING",
    "description": "auto job is running"
  },
  "ext": {},
  "ts": 1775972405000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_AUTO_JOB_RSP",
  "msg_id": "72397eaf-6d9d-4f51-bf5c-78d90bb27031",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "0fed4c91-019c-46d8-aa3a-2223bf97a8b6"
  },
  "ext": {},
  "ts": 1775972405020
}
```

### 特殊说明

- 收到机械臂控制指令后，自动作业状态切换为 `INTERRUPTED`。

---

## 4.3 机械臂状态查询

### 功能描述
由 PAD 或系统侧查询机械臂实时状态。

### API 标识
`ARM_STATUS_QUERY`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| robot_id | 是 | string | 机械臂设备 ID，控制中心为每台机器配置一个唯一的id |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_STATUS_QUERY",
  "msg_id": "74fc84ad-11db-4625-b0e2-feaf4e2ddd0e",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "robot_id": "RB001"
  },
  "ext": {},
  "ts": 1775972460000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_STATUS_QUERY",
  "msg_id": "c5ccb141-5800-4d9a-b820-5bc807ce72f6",
  "identity": "ROBOT-CTRL-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "74fc84ad-11db-4625-b0e2-feaf4e2ddd0e"
  },
  "ext": {},
  "ts": 1775972460020
}
```

### 特殊说明
- 实际状态数据通过“机械臂状态响应”返回。

---

## 4.4 机械臂状态响应

### 功能描述
机械臂侧返回状态查询结果或主动上报当前状态。

### API 标识
`ARM_STATUS_RSP`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| robot_id | 是 | string | 机械臂设备 ID |
| switch | 是 | array[8] | 开关量 0x180，固定大小 8 bytes |
| status | 是 | array[8] | 状态量 0x200，固定大小 8 bytes |
| analog | 是 | array[8] | 模拟量 0x280，固定大小 8 bytes |
| telemetry_300 | 是 | array[8] | 测量量 0x300，固定大小 8 bytes |
| telemetry_400 | 是 | array[8] | 测量量 0x400，固定大小 8 bytes |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_STATUS_RSP",
  "msg_id": "2366753e-93e7-4a10-b40b-b953bfdf9cde",
  "identity": "ROBOT-CTRL-0001",
  "code": 0,
  "body": {
    "robot_id": "RB001",
    "switch": [0, 0, 0, 0, 0, 0, 0, 0],
    "status": [0, 0, 0, 0, 0, 0, 0, 0],
    "analog": [0, 0, 0, 0, 0, 0, 0, 0],
    "telemetry_300": [0, 0, 0, 0, 0, 0, 0, 0],
    "telemetry_400": [0, 0, 0, 0, 0, 0, 0, 0]
  },
  "ext": {},
  "ts": 1775972462000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_STATUS_RSP",
  "msg_id": "f81d5733-253e-4f03-ae63-116cf98e9643",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "2366753e-93e7-4a10-b40b-b953bfdf9cde"
  },
  "ext": {},
  "ts": 1775972462020
}
```

### 特殊说明
- 为了提高传输效率，机械臂的状态查询直接返回PLC的固定bytes数据，需要解码后使用，案例参考 `js_sdk/protocl` 中的encode和decode实现。


## 4.5 机械臂控制指令

### 功能描述
用于下发机械臂控制动作指令。该指令会打断当前自动作业流程，系统进入受控状态。

### API 标识
`ARM_CONTROL_CMD`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| cmd_id | 是 | string | 指令 ID |
| cmd_type | 是 | string | 指令类型，`switch` / `status` / `analog` / `telemetry_300` / `telemetry_400` |
| cmd_params | 否 | array[8] | 指令参数，固定大小 8 bytes |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_CONTROL_CMD",
  "msg_id": "08a4fe80-7f96-4d65-886a-92687ac5cc6a",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "cmd_id": "CMD202604120001",
    "cmd_type": "switch",
    "cmd_params": [0, 0, 0, 0, 0, 0, 0, 0]
  },
  "ext": {},
  "ts": 1775972520000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_CONTROL_CMD",
  "msg_id": "39341bf3-0ed1-4b06-9dd3-d9e91faa46ce",
  "identity": "ROBOT-CTRL-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "08a4fe80-7f96-4d65-886a-92687ac5cc6a"
  },
  "ext": {},
  "ts": 1775972520020
}
```

### 特殊说明
- 为了提高传输效率，机械臂的状态查询直接返回PLC的固定bytes数据，需要解码后使用，案例参考 `js_sdk/protocl` 中的encode和decode实现。

---

## 4.6 地图与障碍物查询

### 功能描述
查询当前地图帧、障碍物信息和目标信息。

### API 标识
`MAP_OBSTACLE_QUERY`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| include_point_cloud | 否 | boolean | 是否包含点云引用 |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "MAP_OBSTACLE_QUERY",
  "msg_id": "db0ad616-44e4-4825-a170-4b27e26fa6cd",
  "identity": "ALG-CORE-0001",
  "code": 0,
  "body": {
    "include_point_cloud": true,
  },
  "ext": {},
  "ts": 1775972580000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "MAP_OBSTACLE_QUERY",
  "msg_id": "75df551d-46c0-4c20-961f-0364157abba6",
  "identity": "VISION-CORE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "db0ad616-44e4-4825-a170-4b27e26fa6cd"
  },
  "ext": {},
  "ts": 1775972580020
}
```

### 特殊说明
- 查询结果由“地图与障碍物响应”返回。

---

## 4.7 地图与障碍物响应

### 功能描述
视觉侧返回地图帧、障碍物和目标信息。

### API 标识
`MAP_OBSTACLE_RSP`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| frame_id | 是 | string | 地图帧编号 |
| map_time | 是 | string(datetime) | 地图时间戳 |
| coordinate_system | 是 | string | 坐标系 |
| targets | 否 | array | 靶标列表 |
| point_cloud_ref | 否 | string | 点云引用地址 |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "MAP_OBSTACLE_RSP",
  "msg_id": "49ae1d23-0406-4934-b67f-f1c881002272",
  "identity": "VISION-CORE-0001",
  "code": 0,
  "body": {
    "frame_id": "MAP_20260412_001",
    "map_time": "2026-04-12 11:03:02.00",
    "coordinate_system": "LOCAL",
    "targets": [
      {
        "target_id": "TAR001",
        "position": {"x": 20.0, "y": 8.5, "z": 15.0}
      }
    ],
    "point_cloud_ref": "pcd://session/20260412/frame001.pcd"
  },
  "ext": {},
  "ts": 1775972582000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "MAP_OBSTACLE_RSP",
  "msg_id": "a45562ea-ed99-4a90-a991-af3f2e5ac632",
  "identity": "ALG-CORE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "49ae1d23-0406-4934-b67f-f1c881002272"
  },
  "ext": {},
  "ts": 1775972582020
}
```

### 特殊说明
- 点云大数据采用引用地址传输，避免超大报文。

---

## 4.8 机械臂安全设置

### 功能描述
配置机械臂安全参数，包括安全距离、速度上限和告警阈值。

### API 标识
`ARM_SAFETY_CONFIG`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| safe_distance | 是 | number | 安全距离，单位 m |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_SAFETY_CONFIG",
  "msg_id": "8070ab0b-e6ee-4ebd-8406-781770fa124f",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "safe_distance": 20
  },
  "ext": {},
  "ts": 1775972640000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_SAFETY_CONFIG",
  "msg_id": "cf07de2f-6da2-43e8-8e6b-37a82eb3641d",
  "identity": "SYS-CORE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "8070ab0b-e6ee-4ebd-8406-781770fa124f"
  },
  "ext": {},
  "ts": 1775972640020
}
```

### 特殊说明
- 安全参数变更应记录审计日志。

---

## 4.9 机械臂日志查询

### 功能描述
查询机械臂作业日志、告警日志和系统事件日志。

### API 标识
`ARM_LOG_QUERY`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| log_type | 否 | string | 日志类型，如 `TASK`/`ALARM`/`SYSTEM` |
| start_time | 否 | string(datetime) | 查询开始时间 |
| end_time | 否 | string(datetime) | 查询结束时间 |
| page_no | 否 | integer | 页码 |
| page_size | 否 | integer | 每页条数 |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_LOG_QUERY",
  "msg_id": "0c92183c-7a83-4eb6-ac58-004886f7b769",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "log_type": "ALARM",
    "start_time": "2026-04-12 00:00:00.00",
    "end_time": "2026-04-12 11:05:00.00",
    "page_no": 1,
    "page_size": 20
  },
  "ext": {},
  "ts": 1775972700000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_LOG_QUERY",
  "msg_id": "c359315a-06e7-4afc-8522-41dcd5f410e1",
  "identity": "SYS-CORE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "0c92183c-7a83-4eb6-ac58-004886f7b769"
  },
  "ext": {},
  "ts": 1775972700020
}
```

### 特殊说明
- 日志查询结果通过“机械臂日志查询响应”返回。

---

## 4.10 机械臂日志查询响应

### 功能描述
返回机械臂日志查询结果，包括分页信息和日志明细列表。

### API 标识
`ARM_LOG_QUERY_RSP`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| log_type | 否 | string | 日志类型 |
| page_no | 是 | integer | 当前页码 |
| page_size | 是 | integer | 每页条数 |
| total_count | 是 | integer | 总记录数 |
| log_items | 是 | array | 日志明细列表 |

`log_items` 元素字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| log_id | 是 | string | 日志 ID |
| log_level | 是 | string | 日志等级，如 `INFO`/`WARN`/`ERROR` |
| log_text | 是 | string | 日志内容 |
| event_time | 是 | string(datetime) | 事件时间 |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_LOG_QUERY_RSP",
  "msg_id": "f0b644ba-561a-4406-a410-7d9d9cdb6a9a",
  "identity": "SYS-CORE-0001",
  "code": 0,
  "body": {
    "log_type": "ALARM",
    "page_no": 1,
    "page_size": 20,
    "total_count": 125,
    "log_items": [
      {
        "log_id": "LOG202604120001",
        "log_level": "ERROR",
        "log_text": "obstacle distance too close",
        "event_time": "2026-04-12 10:58:12.00"
      },
      {
        "log_id": "LOG202604120002",
        "log_level": "WARN",
        "log_text": "manual interrupt received",
        "event_time": "2026-04-12 11:02:00.00"
      }
    ]
  },
  "ext": {},
  "ts": 1775972701000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "ARM_LOG_QUERY_RSP",
  "msg_id": "eb69248c-aecd-4db3-b1d9-5c67da238915",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "f0b644ba-561a-4406-a410-7d9d9cdb6a9a"
  },
  "ext": {},
  "ts": 1775972701020
}
```

### 特殊说明
- `log_items` 为空数组表示查询成功但无数据。

---

## 4.11 机械臂控制授权请求

### 功能描述
由 iPad 发起控制授权请求，同一时刻仅允许一台 iPad 被授权，调用当前接口会撤销原授权的iPad，如果存在多个 iPad 需维护自己的变更逻辑。


### API 标识
`ARM_CONTROL_AUTH_REQ`

### 接口参数

`body` 字段：

| 字段 | 必填 | 类型 | 说明 |
|---|---|---|---|
| reason | 否 | string | 请求原因描述 |

### 请求示例

```json
{
  "v": "1.0",
  "type": "payload",
  "api": "ARM_CONTROL_AUTH_REQ",
  "msg_id": "f6f38dd7-5a93-47a3-b495-a1e089f0f8e2",
  "identity": "PAD-DEVICE-0001",
  "code": 0,
  "body": {
    "reason": "device replacement"
  },
  "ext": {},
  "ts": 1775972800000
}
```

### 返回示例（通讯层 ACK）

```json
{
  "v": "1.0",
  "type": "ack",
  "api": "IDENTITY_SET_REQ",
  "msg_id": "4ceb7691-0f62-4924-9a7d-f1a450406c95",
  "identity": "SYS-CORE-0001",
  "code": 0,
  "body": {
    "ack_msg_id": "f6f38dd7-5a93-47a3-b495-a1e089f0f8e2"
  },
  "ext": {},
  "ts": 1775972800020
}
```

### 特殊说明
- 请求通过之后，原授权的 iPad 权限将被取消，同一时刻只允许一台iPad

---

## 5. 心跳与连接异常判定

1. 客户端每 5 秒发送一次 `ping`（`api = SYS_HEARTBEAT`）。
2. 服务端收到 `ping` 后立即返回 `pong`。
3. 客户端连续 30 秒未收到 `pong`，判定连接异常。
4. 服务端连续 30 秒未收到客户端 `ping`，判定连接异常。
5. 连接异常后立即重连，重连退避为 `1s -> 2s -> 4s -> 8s -> 16s -> 30s`。

## 6. 通讯 code 码表

| code | 含义 |
|---|---|
| 0 | OK |
| 1001 | BAD_FORMAT |
| 1002 | BAD_TYPE |
| 1003 | BAD_API |
| 1004 | UNAUTHORIZED |
| 1005 | TOO_LARGE |
| 1006 | RATE_LIMIT |
| 1007 | DUPLICATE_MSG |
| 1010 | INTERNAL_ERROR |

