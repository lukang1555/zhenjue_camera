"use strict";

const AI_TO_PLC_FRAME_ID = {
  SWITCH: 0x180,
  ANALOG: 0x280,
};

const PLC_TO_AI_FRAME_ID = {
  STATUS: 0x200,
  TELEMETRY_300: 0x300,
  TELEMETRY_400: 0x400,
};

const AI_TO_PLC_SWITCH_BITS = {
  data1: {
    bit0: "cmd_light_enable",
    bit1: "cmd_boom_speed_high",
    bit2: "cmd_boom_speed_mid",
    bit3: "cmd_boom_speed_low",
    bit4: "cmd_platform_load_mode",
    bit5: "cmd_drive_speed_high",
    bit6: "cmd_drive_speed_mid",
    bit7: "cmd_drive_speed_low",
  },
  data2: {
    bit0: "cmd_enable_start",
    bit1: "cmd_horn",
    bit2: "cmd_clamp_close",
    bit3: "cmd_clamp_open",
    bit4: "cmd_clamp_rotate_cw",
    bit5: "cmd_clamp_rotate_ccw",
    bit6: "cmd_clamp_move_left",
    bit7: "cmd_clamp_move_right",
  },
  data3: {
    bit0: "cmd_clamp_move_up",
    bit1: "cmd_clamp_move_down",
    bit2: "cmd_front_area_confirm_1",
    bit3: "cmd_front_area_confirm_2",
    bit4: "cmd_platform_level_up",
    bit5: "cmd_platform_level_down",
    bit6: "cmd_platform_rotate_cw",
    bit7: "cmd_platform_rotate_ccw",
  },
  data4: {
    bit0: "cmd_fly_boom_up",
    bit1: "cmd_fly_boom_down",
    bit2: "cmd_fly_boom_turn_left",
    bit3: "cmd_fly_boom_turn_right",
    bit4: "cmd_main_boom_extend",
    bit5: "cmd_main_boom_retract",
    bit6: "cmd_fold_boom_up_reserved",
    bit7: "cmd_fold_boom_down_reserved",
  },
  data5: {
    bit0: "cmd_reserved_data5_bit0",
    bit1: "cmd_reserved_data5_bit1",
    bit2: "cmd_reserved_data5_bit2",
    bit3: "cmd_reserved_data5_bit3",
    bit4: "cmd_reserved_data5_bit4",
    bit5: "cmd_reserved_data5_bit5",
    bit6: "cmd_reserved_data5_bit6",
    bit7: "cmd_reserved_data5_bit7",
  },
  data6: {
    bit0: "cmd_reserved_data6_bit0",
    bit1: "cmd_reserved_data6_bit1",
    bit2: "cmd_reserved_data6_bit2",
    bit3: "cmd_reserved_data6_bit3",
    bit4: "cmd_reserved_data6_bit4",
    bit5: "cmd_reserved_data6_bit5",
    bit6: "cmd_reserved_data6_bit6",
    bit7: "cmd_reserved_data6_bit7",
  },
  data7: {
    bit0: "cmd_reserved_data7_bit0",
    bit1: "cmd_reserved_data7_bit1",
    bit2: "cmd_reserved_data7_bit2",
    bit3: "cmd_reserved_data7_bit3",
    bit4: "cmd_reserved_data7_bit4",
    bit5: "cmd_reserved_data7_bit5",
    bit6: "cmd_reserved_data7_bit6",
    bit7: "cmd_reserved_data7_bit7",
  },
  data8: {
    bit7: "status_emergency_or_comm_lost",
  },
};

const PLC_TO_AI_STATUS_BITS = {
  data1: {
    bit0: "fb_enable_state",
    bit1: "fb_alarm_active",
    bit2: "fb_overload_active",
    bit3: "fb_over_range_active",
    bit4: "fb_front_area_active",
    bit5: "fb_reserved_bit5",
    bit6: "fb_reserved_bit6",
    bit7: "fb_reserved_bit7",
  },
};

const AI_TO_PLC_ANALOG_FIELDS = {
  data1: "cmd_main_boom_luff_raw",
  data2: "cmd_turntable_rotate_raw",
  data3: "cmd_wheel_steer_raw",
  data4: "cmd_drive_motion_raw",
  data5: "cmd_reserved_5_raw",
  data6: "cmd_reserved_6_raw",
  data7: "cmd_reserved_7_raw",
  data8: "cmd_reserved_8_raw",
};

const PLC_TO_AI_TELEMETRY_300 = [
  { keyRaw: "fb_main_boom_angle_raw", keyValue: "fb_main_boom_angle_deg", dataLow: 1, dataHigh: 2, scale: 0.1 },
  { keyRaw: "fb_main_boom_length_raw", keyValue: "fb_main_boom_length_m", dataLow: 3, dataHigh: 4, scale: 0.1 },
  { keyRaw: "fb_chassis_tilt_x_raw", keyValue: "fb_chassis_tilt_x_deg", dataLow: 5, dataHigh: 6, scale: 0.1 },
  { keyRaw: "fb_chassis_tilt_y_raw", keyValue: "fb_chassis_tilt_y_deg", dataLow: 7, dataHigh: 8, scale: 0.1 },
];

const PLC_TO_AI_TELEMETRY_400 = [
  { keyRaw: "fb_platform_weight_raw", keyValue: "fb_platform_weight_kg", dataLow: 1, dataHigh: 2, scale: 0.1 },
  { keyRaw: "fb_fly_boom_angle_raw", keyValue: "fb_fly_boom_angle_deg", dataLow: 3, dataHigh: 4, scale: 0.1 },
  { keyRaw: "fb_clamp_rotate_angle_raw", keyValue: "fb_clamp_rotate_angle_deg", dataLow: 5, dataHigh: 6, scale: 0.1 },
  { keyRaw: "fb_reserved_7_8_raw", keyValue: "fb_reserved_7_8_value", dataLow: 7, dataHigh: 8, scale: 1 },
];

module.exports = {
  AI_TO_PLC_FRAME_ID,
  PLC_TO_AI_FRAME_ID,
  AI_TO_PLC_SWITCH_BITS,
  PLC_TO_AI_STATUS_BITS,
  AI_TO_PLC_ANALOG_FIELDS,
  PLC_TO_AI_TELEMETRY_300,
  PLC_TO_AI_TELEMETRY_400,
};
