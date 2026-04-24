# PLC 协议指令语义化封装清单

- 来源文档：家庆23米夹具AI控制-通讯协议更新V1.0 - 2026.03.28.docx
- 整理版本：V1.0
- 整理日期：2026-04-12
- 适用对象：PLC 控制程序、上位机业务层、SDK 字段映射层

## 1. 命名规范

1. 统一使用 `snake_case`。
2. bit 开关量使用布尔语义字段：`true/false`。
3. 模拟量使用原始值 + 工程值双字段：
- `*_raw`: 原始 bit/字节值
- `*`: 按倍率换算后的工程值
4. 帧方向命名：
- `ai_to_plc_*`: 遥控器/AI 发送给控制器
- `plc_to_ai_*`: 控制器发送给遥控器

## 2. AI -> PLC（0x180 + node_id）开关量封装

说明：以下为 8 字节开关量帧，文档当前明确了 Data1~Data4 与 Data8.Bit7 的语义。

### 2.1 Data1（Byte1）

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data1 | bit0 | 照明灯 | cmd_light_enable | 照明开关 |
| data1 | bit1 | 臂架高速档位 | cmd_boom_speed_high | 臂架速度档位-高 |
| data1 | bit2 | 臂架中速档位 | cmd_boom_speed_mid | 臂架速度档位-中 |
| data1 | bit3 | 臂架低速档位 | cmd_boom_speed_low | 臂架速度档位-低 |
| data1 | bit4 | 平台载重模式 | cmd_platform_load_mode | 平台载重模式开关 |
| data1 | bit5 | 行驶高速档位 | cmd_drive_speed_high | 行驶速度档位-高 |
| data1 | bit6 | 行驶中速档位 | cmd_drive_speed_mid | 行驶速度档位-中 |
| data1 | bit7 | 行驶低速档位 | cmd_drive_speed_low | 行驶速度档位-低 |

### 2.2 Data2（Byte2）

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data2 | bit0 | 启动-使能信号 | cmd_enable_start | 使能启动 |
| data2 | bit1 | 喇叭 | cmd_horn | 喇叭控制 |
| data2 | bit2 | 夹具夹紧 | cmd_clamp_close | 夹具夹紧 |
| data2 | bit3 | 夹具松开 | cmd_clamp_open | 夹具松开 |
| data2 | bit4 | 夹具顺转 | cmd_clamp_rotate_cw | 夹具顺时针旋转 |
| data2 | bit5 | 夹具逆转 | cmd_clamp_rotate_ccw | 夹具逆时针旋转 |
| data2 | bit6 | 夹具左移 | cmd_clamp_move_left | 夹具左移 |
| data2 | bit7 | 夹具右移 | cmd_clamp_move_right | 夹具右移 |

### 2.3 Data3（Byte3）

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data3 | bit0 | 夹具上移 | cmd_clamp_move_up | 夹具上移 |
| data3 | bit1 | 夹具下移 | cmd_clamp_move_down | 夹具下移 |
| data3 | bit2 | 前方区域确认 | cmd_front_area_confirm_1 | 前方区域确认（通道1） |
| data3 | bit3 | 前方区域确认 | cmd_front_area_confirm_2 | 前方区域确认（通道2） |
| data3 | bit4 | 平台调平上 | cmd_platform_level_up | 平台调平上 |
| data3 | bit5 | 平台调平下 | cmd_platform_level_down | 平台调平下 |
| data3 | bit6 | 平台顺转 | cmd_platform_rotate_cw | 平台顺时针旋转 |
| data3 | bit7 | 平台逆转 | cmd_platform_rotate_ccw | 平台逆时针旋转 |

### 2.4 Data4（Byte4）

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data4 | bit0 | 飞臂起 | cmd_fly_boom_up | 飞臂上升 |
| data4 | bit1 | 飞臂落 | cmd_fly_boom_down | 飞臂下降 |
| data4 | bit2 | 飞臂左转 | cmd_fly_boom_turn_left | 飞臂左转 |
| data4 | bit3 | 飞臂右转 | cmd_fly_boom_turn_right | 飞臂右转 |
| data4 | bit4 | 主臂伸 | cmd_main_boom_extend | 主臂伸出 |
| data4 | bit5 | 主臂缩 | cmd_main_boom_retract | 主臂回缩 |
| data4 | bit6 | 折臂起（备用） | cmd_fold_boom_up_reserved | 备用位 |
| data4 | bit7 | 折臂落（备用） | cmd_fold_boom_down_reserved | 备用位 |

### 2.5 Data5（Byte5）

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data5 | bit0 | 预留 | cmd_reserved_data5_bit0 | 预留扩展位 |
| data5 | bit1 | 预留 | cmd_reserved_data5_bit1 | 预留扩展位 |
| data5 | bit2 | 预留 | cmd_reserved_data5_bit2 | 预留扩展位 |
| data5 | bit3 | 预留 | cmd_reserved_data5_bit3 | 预留扩展位 |
| data5 | bit4 | 预留 | cmd_reserved_data5_bit4 | 预留扩展位 |
| data5 | bit5 | 预留 | cmd_reserved_data5_bit5 | 预留扩展位 |
| data5 | bit6 | 预留 | cmd_reserved_data5_bit6 | 预留扩展位 |
| data5 | bit7 | 预留 | cmd_reserved_data5_bit7 | 预留扩展位 |

### 2.6 Data6（Byte6）

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data6 | bit0 | 预留 | cmd_reserved_data6_bit0 | 预留扩展位 |
| data6 | bit1 | 预留 | cmd_reserved_data6_bit1 | 预留扩展位 |
| data6 | bit2 | 预留 | cmd_reserved_data6_bit2 | 预留扩展位 |
| data6 | bit3 | 预留 | cmd_reserved_data6_bit3 | 预留扩展位 |
| data6 | bit4 | 预留 | cmd_reserved_data6_bit4 | 预留扩展位 |
| data6 | bit5 | 预留 | cmd_reserved_data6_bit5 | 预留扩展位 |
| data6 | bit6 | 预留 | cmd_reserved_data6_bit6 | 预留扩展位 |
| data6 | bit7 | 预留 | cmd_reserved_data6_bit7 | 预留扩展位 |

### 2.7 Data7（Byte7）

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data7 | bit0 | 预留 | cmd_reserved_data7_bit0 | 预留扩展位 |
| data7 | bit1 | 预留 | cmd_reserved_data7_bit1 | 预留扩展位 |
| data7 | bit2 | 预留 | cmd_reserved_data7_bit2 | 预留扩展位 |
| data7 | bit3 | 预留 | cmd_reserved_data7_bit3 | 预留扩展位 |
| data7 | bit4 | 预留 | cmd_reserved_data7_bit4 | 预留扩展位 |
| data7 | bit5 | 预留 | cmd_reserved_data7_bit5 | 预留扩展位 |
| data7 | bit6 | 预留 | cmd_reserved_data7_bit6 | 预留扩展位 |
| data7 | bit7 | 预留 | cmd_reserved_data7_bit7 | 预留扩展位 |

### 2.8 Data8（Byte8）状态位

| 字节 | Bit | 原功能 | 封装名称 | 说明 |
|---|---|---|---|---|
| data8 | bit7 | 急停状态/无线通讯状态 | status_emergency_or_comm_lost | `0x00` 表示通讯且急停拉起；`0x80` 表示不通讯或急停按下 |

## 3. AI -> PLC（0x280 + node_id）模拟量封装

| 字节 | 原功能 | 封装名称 | 原始值范围 | 工程语义 |
|---|---|---|---|---|
| data1 | 主臂变幅起落 | cmd_main_boom_luff_raw | 0x00~0xFF | 上: 0x7E~0x00, 下: 0x80~0xFF, 中位: 0x7F |
| data2 | 转台顺逆转 | cmd_turntable_rotate_raw | 0x00~0xFF | 上: 0x7E~0x00, 下: 0x80~0xFF, 中位: 0x7F |
| data3 | 轮胎左转右转 | cmd_wheel_steer_raw | 0x00~0xFF | 左: 0x7E~0x00, 右: 0x80~0xFF, 中位: 0x7F |
| data4 | 行驶前进后退 | cmd_drive_motion_raw | 0x00~0xFF | 上: 0x7E~0x00, 下: 0x80~0xFF, 中位: 0x7F |
| data5~data8 | 备用 | cmd_reserved_5_8_raw | 0x00~0xFF | 预留扩展 |

## 4. PLC -> AI（0x200 + node_id）状态/告警位封装

| 字节 | Bit | 原功能 | 封装名称 | 位语义 |
|---|---|---|---|---|
| data1 | bit0 | START 使能 | fb_enable_state | 0: 关闭使能, 1: 开启使能 |
| data1 | bit1 | 报警状态 | fb_alarm_active | 0: 无报警, 1: 有报警 |
| data1 | bit2 | 超载状态 | fb_overload_active | 0: 未超载, 1: 超载 |
| data1 | bit3 | 超幅状态 | fb_over_range_active | 0: 未超幅, 1: 超幅 |
| data1 | bit4 | 前方区域状态 | fb_front_area_active | 0: 非前方区域, 1: 前方区域 |
| data1 | bit5 | 预留 | fb_reserved_bit5 | 预留 |
| data1 | bit6 | 预留 | fb_reserved_bit6 | 预留 |
| data1 | bit7 | 预留 | fb_reserved_bit7 | 预留 |

## 5. PLC -> AI（0x300 + node_id）测量量封装

> 说明：0x300、0x400 帧中的物理量跨 2 字节，按小端组合为 `int16` 后乘倍率。

| 数据位 | 原功能 | 封装名称 | 数据类型 | 倍率 | 工程字段 |
|---|---|---|---|---|---|
| data1~data2 | 主臂角度 | fb_main_boom_angle_raw | int16 | 0.1°/bit | fb_main_boom_angle_deg |
| data3~data4 | 主臂长度 | fb_main_boom_length_raw | int16 | 0.1m/bit | fb_main_boom_length_m |
| data5~data6 | 底盘倾角X轴 | fb_chassis_tilt_x_raw | int16 | 0.1°/bit | fb_chassis_tilt_x_deg |
| data7~data8 | 底盘倾角Y轴 | fb_chassis_tilt_y_raw | int16 | 0.1°/bit | fb_chassis_tilt_y_deg |

## 6. PLC -> AI（0x400 + node_id）测量量封装

| 数据位 | 原功能 | 封装名称 | 数据类型 | 倍率 | 工程字段 |
|---|---|---|---|---|---|
| data1~data2 | 平台重量 | fb_platform_weight_raw | int16 | 0.1Kg/bit | fb_platform_weight_kg |
| data3~data4 | 飞臂变幅角度 | fb_fly_boom_angle_raw | int16 | 0.1°/bit | fb_fly_boom_angle_deg |
| data5~data6 | 夹具旋转角度 | fb_clamp_rotate_angle_raw | int16 | 0.1°/bit | fb_clamp_rotate_angle_deg |
| data7~data8 | 预留/扩展 | fb_reserved_7_8_raw | int16 | - | 预留扩展 |

## 7. 代码封装结构

```text
protocol/
  ai_to_plc_switch_bits.js
  ai_to_plc_analog_axes.js
  plc_to_ai_status_bits.js
  plc_to_ai_telemetry_300.js
  plc_to_ai_telemetry_400.js
  codec.js
```

## 8. 常量示例

```javascript
export const AI_TO_PLC_SWITCH = {
  DATA1: {
    BIT0: "cmd_light_enable",
    BIT1: "cmd_boom_speed_high",
    BIT2: "cmd_boom_speed_mid",
    BIT3: "cmd_boom_speed_low"
  },
  DATA5: {
    BIT0: "cmd_reserved_data5_bit0",
    BIT1: "cmd_reserved_data5_bit1"
  }
};

export const PLC_TO_AI_STATUS = {
  DATA1: {
    BIT0: "fb_enable_state",
    BIT1: "fb_alarm_active",
    BIT2: "fb_overload_active",
    BIT3: "fb_over_range_active",
    BIT4: "fb_front_area_active"
  }
};
```
