# Hikvision PTZ 可调用能力记录

本文件用于沉淀当前设备（海康智能球机）经 ISAPI 实测/查询可用的 PTZ 与补光控制方式，便于后续逐项启用或排查。

## 1. 连续运动（已在代码中接入）

- 接口：`PUT /ISAPI/PTZCtrl/channels/1/continuous`
- Body（XML）：

```xml
<PTZData>
  <pan>-100~100</pan>
  <tilt>-100~100</tilt>
  <zoom>-100~100</zoom>
</PTZData>
```

说明：
- `pan` 正值向右，负值向左；
- `0,0,0` 即停止；
- 当前 `camera_service` 的自动左右摆与 `P+15` 都基于该接口。

## 2. 状态读取（已在代码中接入）

- 接口：`GET /ISAPI/PTZCtrl/channels/1/status`
- 关键字段：
  - `azimuth`：水平角（X，范围 `0~3600`，对应 `0~360.0°`）
  - `elevation`：俯仰角（Y，范围 `0~900`，对应 `0~90.0°`）
  - `absoluteZoom`：绝对变倍（范围 `10~250`）

说明：
- 当前代码会从返回 XML 中提取上述值并发布到 `/camera_ptz_status`；
- 用于闭环动作（例如“转到目标角后停止”）。

## 4. 补光灯（按 capabilities 校正）

- 主接口：`PUT /ISAPI/Image/channels/1/supplementLight`
- 备用接口：`PUT /ISAPI/Image/channels/1`（`ImageChannel` 包裹）
- 关键能力值（来自 capabilities）：
  - `supplementLightMode`：`colorVuWhiteLight` / `irLight` / `close`
  - `mixedLightBrightnessRegulatMode`：`auto` / `schedule` / `on` / `off`

当前默认策略：
- 开灯：`supplementLightMode=colorVuWhiteLight` + `mixedLightBrightnessRegulatMode=on` + `whiteLightBrightness=100`
- 关灯：`supplementLightMode=close` + `mixedLightBrightnessRegulatMode=off`

## 5. 其他可扩展 PTZ 能力（待按需接入）

以下资源已确认存在，建议后续按业务逐项封装：

- 预置位：`/ISAPI/PTZCtrl/channels/1/presets`（设备上限约 300）
- 巡航：`/ISAPI/PTZCtrl/channels/1/patrols`（设备上限约 8）
- 轨迹：`/ISAPI/PTZCtrl/channels/1/patterns`（设备上限约 4）
- 守望位：`/ISAPI/PTZCtrl/channels/1/homeposition`
- 停车动作/空闲动作能力：`parkAction`（`autoscan/patrol/pattern/...`）

---

如果后续需要“动作枚举清单”，可在此基础上再补一份 JSON 模板（method/path/body/timeout），用于逐条压测各动作在该机型上的生效情况。

## 6. 项目内复用方式

`arm_control_package` 已导出 `CameraService`，其他包可直接链接调用，例如：

- `start_relative_pan_positive(15.0)`：向右 15°
- `start_relative_pan_negative(15.0)`：向左 15°
- `start_relative_pan(delta)`：通用相对转动（正右负左）
- `move_to_zero()`：回到 `0,0`

调用前提：
- 运行中存在 `arm_camera_node`（负责消费 `camera_http_control` 并下发 HTTP）。
