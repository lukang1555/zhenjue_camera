#pragma once

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace arm_common_package
{

    constexpr uint16_t CanBaseAiToPlcSwitch = 0x180;
    constexpr uint16_t CanBasePlcToAiStatus = 0x200;
    constexpr uint16_t CanBaseAiToPlcAnalog = 0x280;
    constexpr uint16_t CanBasePlcToAiTelemetry300 = 0x300;
    constexpr uint16_t CanBasePlcToAiTelemetry400 = 0x400;
    constexpr uint32_t CanBaseMask = 0x780;
    constexpr uint32_t CanNodeMask = 0x7F;

    struct RosCanFrame
    {
        uint32_t id{0};
        bool is_extended{false};
        bool is_rtr{false};
        bool is_error{false};
        uint8_t dlc{8};
        std::array<uint8_t, 8> data{{0, 0, 0, 0, 0, 0, 0, 0}};
    };

    struct AiToPlcSwitchBits
    {
        bool cmd_light_enable{false};
        bool cmd_boom_speed_high{false};
        bool cmd_boom_speed_mid{false};
        bool cmd_boom_speed_low{false};
        bool cmd_platform_load_mode{false};
        bool cmd_drive_speed_high{false};
        bool cmd_drive_speed_mid{false};
        bool cmd_drive_speed_low{false};

        bool cmd_enable_start{false};
        bool cmd_horn{false};
        bool cmd_clamp_close{false};
        bool cmd_clamp_open{false};
        bool cmd_clamp_rotate_cw{false};
        bool cmd_clamp_rotate_ccw{false};
        bool cmd_clamp_move_left{false};
        bool cmd_clamp_move_right{false};

        bool cmd_clamp_move_up{false};
        bool cmd_clamp_move_down{false};
        bool cmd_front_area_confirm_1{false};
        bool cmd_front_area_confirm_2{false};
        bool cmd_platform_level_up{false};
        bool cmd_platform_level_down{false};
        bool cmd_platform_rotate_cw{false};
        bool cmd_platform_rotate_ccw{false};

        bool cmd_fly_boom_up{false};
        bool cmd_fly_boom_down{false};
        bool cmd_fly_boom_turn_left{false};
        bool cmd_fly_boom_turn_right{false};
        bool cmd_main_boom_extend{false};
        bool cmd_main_boom_retract{false};
        bool cmd_fold_boom_up_reserved{false};
        bool cmd_fold_boom_down_reserved{false};

        bool status_emergency_or_comm_lost{false};
    };

    struct AiToPlcAnalogFields
    {
        uint8_t cmd_main_boom_luff_raw{0};
        uint8_t cmd_turntable_rotate_raw{0};
        uint8_t cmd_wheel_steer_raw{0};
        uint8_t cmd_drive_motion_raw{0};
        uint8_t cmd_reserved_5_raw{0};
        uint8_t cmd_reserved_6_raw{0};
        uint8_t cmd_reserved_7_raw{0};
        uint8_t cmd_reserved_8_raw{0};
    };

    struct PlcToAiStatusBits
    {
        bool fb_enable_state{false};
        bool fb_alarm_active{false};
        bool fb_overload_active{false};
        bool fb_over_range_active{false};
        bool fb_front_area_active{false};
        bool fb_reserved_bit5{false};
        bool fb_reserved_bit6{false};
        bool fb_reserved_bit7{false};
    };

    struct PlcToAiTelemetry300
    {
        int16_t fb_main_boom_angle_raw{0};
        double fb_main_boom_angle_deg{0.0};
        int16_t fb_main_boom_length_raw{0};
        double fb_main_boom_length_m{0.0};
        int16_t fb_chassis_tilt_x_raw{0};
        double fb_chassis_tilt_x_deg{0.0};
        int16_t fb_chassis_tilt_y_raw{0};
        double fb_chassis_tilt_y_deg{0.0};
    };

    struct PlcToAiTelemetry400
    {
        int16_t fb_platform_weight_raw{0};
        double fb_platform_weight_kg{0.0};
        int16_t fb_fly_boom_angle_raw{0};
        double fb_fly_boom_angle_deg{0.0};
        int16_t fb_clamp_rotate_angle_raw{0};
        double fb_clamp_rotate_angle_deg{0.0};
        int16_t fb_reserved_7_8_raw{0};
        double fb_reserved_7_8_value{0.0};
    };

    struct ArmMsgRaw
    {
        std::array<uint8_t, 8> ai_to_plc_switch{{0, 0, 0, 0, 0, 0, 0, 0}};
        std::array<uint8_t, 8> plc_to_ai_status{{0, 0, 0, 0, 0, 0, 0, 0}};
        std::array<uint8_t, 8> ai_to_plc_analog{{0, 0, 0, 0, 0, 0, 0, 0}};
        std::array<uint8_t, 8> plc_to_ai_telemetry_300{{0, 0, 0, 0, 0, 0, 0, 0}};
        std::array<uint8_t, 8> plc_to_ai_telemetry_400{{0, 0, 0, 0, 0, 0, 0, 0}};
    };

    struct ArmMsgReadable
    {
        AiToPlcSwitchBits ai_to_plc_switch;
        AiToPlcAnalogFields ai_to_plc_analog;
        PlcToAiStatusBits plc_to_ai_status;
        PlcToAiTelemetry300 plc_to_ai_telemetry_300;
        PlcToAiTelemetry400 plc_to_ai_telemetry_400;
    };

    struct ArmMsg
    {
        std::string robot_id{"RB001"};
        ArmMsgRaw raw;
        ArmMsgReadable readable;
    };

    inline uint8_t clamp_byte(int value)
    {
        if (value < 0)
        {
            return 0;
        }
        if (value > 255)
        {
            return 255;
        }
        return static_cast<uint8_t>(value);
    }

    inline bool get_bit(uint8_t value, int bit)
    {
        return ((value >> bit) & 0x01) == 1;
    }

    inline void set_bit(uint8_t &value, int bit, bool enabled)
    {
        if (enabled)
        {
            value = static_cast<uint8_t>(value | (1U << bit));
        }
    }

    inline int16_t read_int16_le(uint8_t low, uint8_t high)
    {
        const uint16_t raw = static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
        return static_cast<int16_t>(raw);
    }

    inline std::array<uint8_t, 2> write_int16_le(int16_t value)
    {
        const uint16_t raw = static_cast<uint16_t>(value);
        return {
            static_cast<uint8_t>(raw & 0xFF),
            static_cast<uint8_t>((raw >> 8) & 0xFF),
        };
    }

    inline std::array<uint8_t, 8> to_u8_array(const std::vector<int> &input)
    {
        std::array<uint8_t, 8> out{{0, 0, 0, 0, 0, 0, 0, 0}};
        for (size_t i = 0; i < out.size() && i < input.size(); ++i)
        {
            out[i] = clamp_byte(input[i]);
        }
        return out;
    }

    inline std::array<int, 8> to_int_array(const std::array<uint8_t, 8> &input)
    {
        std::array<int, 8> out{{0, 0, 0, 0, 0, 0, 0, 0}};
        for (size_t i = 0; i < out.size(); ++i)
        {
            out[i] = static_cast<int>(input[i]);
        }
        return out;
    }

    inline std::string bytes_to_hex(const std::array<uint8_t, 8> &bytes)
    {
        std::ostringstream ss;
        ss << std::uppercase << std::hex << std::setfill('0');
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            if (i > 0)
            {
                ss << ' ';
            }
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        return ss.str();
    }

    inline AiToPlcSwitchBits decode_ai_to_plc_switch(const std::array<uint8_t, 8> &bytes)
    {
        AiToPlcSwitchBits out;
        const uint8_t data1 = bytes[0];
        const uint8_t data2 = bytes[1];
        const uint8_t data3 = bytes[2];
        const uint8_t data4 = bytes[3];
        const uint8_t data8 = bytes[7];

        out.cmd_light_enable = get_bit(data1, 0);
        out.cmd_boom_speed_high = get_bit(data1, 1);
        out.cmd_boom_speed_mid = get_bit(data1, 2);
        out.cmd_boom_speed_low = get_bit(data1, 3);
        out.cmd_platform_load_mode = get_bit(data1, 4);
        out.cmd_drive_speed_high = get_bit(data1, 5);
        out.cmd_drive_speed_mid = get_bit(data1, 6);
        out.cmd_drive_speed_low = get_bit(data1, 7);

        out.cmd_enable_start = get_bit(data2, 0);
        out.cmd_horn = get_bit(data2, 1);
        out.cmd_clamp_close = get_bit(data2, 2);
        out.cmd_clamp_open = get_bit(data2, 3);
        out.cmd_clamp_rotate_cw = get_bit(data2, 4);
        out.cmd_clamp_rotate_ccw = get_bit(data2, 5);
        out.cmd_clamp_move_left = get_bit(data2, 6);
        out.cmd_clamp_move_right = get_bit(data2, 7);

        out.cmd_clamp_move_up = get_bit(data3, 0);
        out.cmd_clamp_move_down = get_bit(data3, 1);
        out.cmd_front_area_confirm_1 = get_bit(data3, 2);
        out.cmd_front_area_confirm_2 = get_bit(data3, 3);
        out.cmd_platform_level_up = get_bit(data3, 4);
        out.cmd_platform_level_down = get_bit(data3, 5);
        out.cmd_platform_rotate_cw = get_bit(data3, 6);
        out.cmd_platform_rotate_ccw = get_bit(data3, 7);

        out.cmd_fly_boom_up = get_bit(data4, 0);
        out.cmd_fly_boom_down = get_bit(data4, 1);
        out.cmd_fly_boom_turn_left = get_bit(data4, 2);
        out.cmd_fly_boom_turn_right = get_bit(data4, 3);
        out.cmd_main_boom_extend = get_bit(data4, 4);
        out.cmd_main_boom_retract = get_bit(data4, 5);
        out.cmd_fold_boom_up_reserved = get_bit(data4, 6);
        out.cmd_fold_boom_down_reserved = get_bit(data4, 7);

        out.status_emergency_or_comm_lost = get_bit(data8, 7);
        return out;
    }

    inline std::array<uint8_t, 8> encode_ai_to_plc_switch(const AiToPlcSwitchBits &cmd)
    {
        std::array<uint8_t, 8> bytes{{0, 0, 0, 0, 0, 0, 0, 0}};

        set_bit(bytes[0], 0, cmd.cmd_light_enable);
        set_bit(bytes[0], 1, cmd.cmd_boom_speed_high);
        set_bit(bytes[0], 2, cmd.cmd_boom_speed_mid);
        set_bit(bytes[0], 3, cmd.cmd_boom_speed_low);
        set_bit(bytes[0], 4, cmd.cmd_platform_load_mode);
        set_bit(bytes[0], 5, cmd.cmd_drive_speed_high);
        set_bit(bytes[0], 6, cmd.cmd_drive_speed_mid);
        set_bit(bytes[0], 7, cmd.cmd_drive_speed_low);

        set_bit(bytes[1], 0, cmd.cmd_enable_start);
        set_bit(bytes[1], 1, cmd.cmd_horn);
        set_bit(bytes[1], 2, cmd.cmd_clamp_close);
        set_bit(bytes[1], 3, cmd.cmd_clamp_open);
        set_bit(bytes[1], 4, cmd.cmd_clamp_rotate_cw);
        set_bit(bytes[1], 5, cmd.cmd_clamp_rotate_ccw);
        set_bit(bytes[1], 6, cmd.cmd_clamp_move_left);
        set_bit(bytes[1], 7, cmd.cmd_clamp_move_right);

        set_bit(bytes[2], 0, cmd.cmd_clamp_move_up);
        set_bit(bytes[2], 1, cmd.cmd_clamp_move_down);
        set_bit(bytes[2], 2, cmd.cmd_front_area_confirm_1);
        set_bit(bytes[2], 3, cmd.cmd_front_area_confirm_2);
        set_bit(bytes[2], 4, cmd.cmd_platform_level_up);
        set_bit(bytes[2], 5, cmd.cmd_platform_level_down);
        set_bit(bytes[2], 6, cmd.cmd_platform_rotate_cw);
        set_bit(bytes[2], 7, cmd.cmd_platform_rotate_ccw);

        set_bit(bytes[3], 0, cmd.cmd_fly_boom_up);
        set_bit(bytes[3], 1, cmd.cmd_fly_boom_down);
        set_bit(bytes[3], 2, cmd.cmd_fly_boom_turn_left);
        set_bit(bytes[3], 3, cmd.cmd_fly_boom_turn_right);
        set_bit(bytes[3], 4, cmd.cmd_main_boom_extend);
        set_bit(bytes[3], 5, cmd.cmd_main_boom_retract);
        set_bit(bytes[3], 6, cmd.cmd_fold_boom_up_reserved);
        set_bit(bytes[3], 7, cmd.cmd_fold_boom_down_reserved);

        set_bit(bytes[7], 7, cmd.status_emergency_or_comm_lost);
        return bytes;
    }

    inline AiToPlcAnalogFields decode_ai_to_plc_analog(const std::array<uint8_t, 8> &bytes)
    {
        AiToPlcAnalogFields out;
        out.cmd_main_boom_luff_raw = bytes[0];
        out.cmd_turntable_rotate_raw = bytes[1];
        out.cmd_wheel_steer_raw = bytes[2];
        out.cmd_drive_motion_raw = bytes[3];
        out.cmd_reserved_5_raw = bytes[4];
        out.cmd_reserved_6_raw = bytes[5];
        out.cmd_reserved_7_raw = bytes[6];
        out.cmd_reserved_8_raw = bytes[7];
        return out;
    }

    inline std::array<uint8_t, 8> encode_ai_to_plc_analog(const AiToPlcAnalogFields &analog)
    {
        return {
            analog.cmd_main_boom_luff_raw,
            analog.cmd_turntable_rotate_raw,
            analog.cmd_wheel_steer_raw,
            analog.cmd_drive_motion_raw,
            analog.cmd_reserved_5_raw,
            analog.cmd_reserved_6_raw,
            analog.cmd_reserved_7_raw,
            analog.cmd_reserved_8_raw,
        };
    }

    inline PlcToAiStatusBits decode_plc_to_ai_status(const std::array<uint8_t, 8> &bytes)
    {
        PlcToAiStatusBits out;
        out.fb_enable_state = get_bit(bytes[0], 0);
        out.fb_alarm_active = get_bit(bytes[0], 1);
        out.fb_overload_active = get_bit(bytes[0], 2);
        out.fb_over_range_active = get_bit(bytes[0], 3);
        out.fb_front_area_active = get_bit(bytes[0], 4);
        out.fb_reserved_bit5 = get_bit(bytes[0], 5);
        out.fb_reserved_bit6 = get_bit(bytes[0], 6);
        out.fb_reserved_bit7 = get_bit(bytes[0], 7);
        return out;
    }

    inline std::array<uint8_t, 8> encode_plc_to_ai_status(const PlcToAiStatusBits &status)
    {
        std::array<uint8_t, 8> bytes{{0, 0, 0, 0, 0, 0, 0, 0}};
        set_bit(bytes[0], 0, status.fb_enable_state);
        set_bit(bytes[0], 1, status.fb_alarm_active);
        set_bit(bytes[0], 2, status.fb_overload_active);
        set_bit(bytes[0], 3, status.fb_over_range_active);
        set_bit(bytes[0], 4, status.fb_front_area_active);
        set_bit(bytes[0], 5, status.fb_reserved_bit5);
        set_bit(bytes[0], 6, status.fb_reserved_bit6);
        set_bit(bytes[0], 7, status.fb_reserved_bit7);
        return bytes;
    }

    inline PlcToAiTelemetry300 decode_plc_to_ai_telemetry_300(const std::array<uint8_t, 8> &bytes)
    {
        PlcToAiTelemetry300 out;
        out.fb_main_boom_angle_raw = read_int16_le(bytes[0], bytes[1]);
        out.fb_main_boom_angle_deg = static_cast<double>(out.fb_main_boom_angle_raw) * 0.1;
        out.fb_main_boom_length_raw = read_int16_le(bytes[2], bytes[3]);
        out.fb_main_boom_length_m = static_cast<double>(out.fb_main_boom_length_raw) * 0.1;
        out.fb_chassis_tilt_x_raw = read_int16_le(bytes[4], bytes[5]);
        out.fb_chassis_tilt_x_deg = static_cast<double>(out.fb_chassis_tilt_x_raw) * 0.1;
        out.fb_chassis_tilt_y_raw = read_int16_le(bytes[6], bytes[7]);
        out.fb_chassis_tilt_y_deg = static_cast<double>(out.fb_chassis_tilt_y_raw) * 0.1;
        return out;
    }

    inline PlcToAiTelemetry400 decode_plc_to_ai_telemetry_400(const std::array<uint8_t, 8> &bytes)
    {
        PlcToAiTelemetry400 out;
        out.fb_platform_weight_raw = read_int16_le(bytes[0], bytes[1]);
        out.fb_platform_weight_kg = static_cast<double>(out.fb_platform_weight_raw) * 0.1;
        out.fb_fly_boom_angle_raw = read_int16_le(bytes[2], bytes[3]);
        out.fb_fly_boom_angle_deg = static_cast<double>(out.fb_fly_boom_angle_raw) * 0.1;
        out.fb_clamp_rotate_angle_raw = read_int16_le(bytes[4], bytes[5]);
        out.fb_clamp_rotate_angle_deg = static_cast<double>(out.fb_clamp_rotate_angle_raw) * 0.1;
        out.fb_reserved_7_8_raw = read_int16_le(bytes[6], bytes[7]);
        out.fb_reserved_7_8_value = static_cast<double>(out.fb_reserved_7_8_raw);
        return out;
    }

    inline std::array<uint8_t, 8> encode_plc_to_ai_telemetry_300(const PlcToAiTelemetry300 &telemetry)
    {
        std::array<uint8_t, 8> bytes{{0, 0, 0, 0, 0, 0, 0, 0}};
        const auto pair1 = write_int16_le(telemetry.fb_main_boom_angle_raw);
        const auto pair2 = write_int16_le(telemetry.fb_main_boom_length_raw);
        const auto pair3 = write_int16_le(telemetry.fb_chassis_tilt_x_raw);
        const auto pair4 = write_int16_le(telemetry.fb_chassis_tilt_y_raw);
        bytes[0] = pair1[0];
        bytes[1] = pair1[1];
        bytes[2] = pair2[0];
        bytes[3] = pair2[1];
        bytes[4] = pair3[0];
        bytes[5] = pair3[1];
        bytes[6] = pair4[0];
        bytes[7] = pair4[1];
        return bytes;
    }

    inline std::array<uint8_t, 8> encode_plc_to_ai_telemetry_400(const PlcToAiTelemetry400 &telemetry)
    {
        std::array<uint8_t, 8> bytes{{0, 0, 0, 0, 0, 0, 0, 0}};
        const auto pair1 = write_int16_le(telemetry.fb_platform_weight_raw);
        const auto pair2 = write_int16_le(telemetry.fb_fly_boom_angle_raw);
        const auto pair3 = write_int16_le(telemetry.fb_clamp_rotate_angle_raw);
        const auto pair4 = write_int16_le(telemetry.fb_reserved_7_8_raw);
        bytes[0] = pair1[0];
        bytes[1] = pair1[1];
        bytes[2] = pair2[0];
        bytes[3] = pair2[1];
        bytes[4] = pair3[0];
        bytes[5] = pair3[1];
        bytes[6] = pair4[0];
        bytes[7] = pair4[1];
        return bytes;
    }

    inline RosCanFrame make_can_frame(uint16_t base_id, uint8_t node_id, const std::array<uint8_t, 8> &data)
    {
        RosCanFrame frame;
        frame.id = static_cast<uint32_t>(base_id) + static_cast<uint32_t>(node_id);
        frame.dlc = 8;
        frame.data = data;
        return frame;
    }

    inline void decode_arm_status_from_raw(ArmMsg &status)
    {
        status.readable.ai_to_plc_switch = decode_ai_to_plc_switch(status.raw.ai_to_plc_switch);
        status.readable.ai_to_plc_analog = decode_ai_to_plc_analog(status.raw.ai_to_plc_analog);
        status.readable.plc_to_ai_status = decode_plc_to_ai_status(status.raw.plc_to_ai_status);
        status.readable.plc_to_ai_telemetry_300 = decode_plc_to_ai_telemetry_300(status.raw.plc_to_ai_telemetry_300);
        status.readable.plc_to_ai_telemetry_400 = decode_plc_to_ai_telemetry_400(status.raw.plc_to_ai_telemetry_400);
    }

    inline void encode_arm_status_to_raw(ArmMsg &status)
    {
        status.raw.ai_to_plc_switch = encode_ai_to_plc_switch(status.readable.ai_to_plc_switch);
        status.raw.ai_to_plc_analog = encode_ai_to_plc_analog(status.readable.ai_to_plc_analog);
        status.raw.plc_to_ai_status = encode_plc_to_ai_status(status.readable.plc_to_ai_status);
        status.raw.plc_to_ai_telemetry_300 = encode_plc_to_ai_telemetry_300(status.readable.plc_to_ai_telemetry_300);
        status.raw.plc_to_ai_telemetry_400 = encode_plc_to_ai_telemetry_400(status.readable.plc_to_ai_telemetry_400);
    }

    inline bool apply_ai_command_frame(const RosCanFrame &frame, ArmMsg &status)
    {
        const uint32_t base_id = frame.id & CanBaseMask;
        if (base_id == CanBaseAiToPlcSwitch)
        {
            status.raw.ai_to_plc_switch = frame.data;
            status.readable.ai_to_plc_switch = decode_ai_to_plc_switch(frame.data);
            return true;
        }
        if (base_id == CanBaseAiToPlcAnalog)
        {
            status.raw.ai_to_plc_analog = frame.data;
            status.readable.ai_to_plc_analog = decode_ai_to_plc_analog(frame.data);
            return true;
        }
        return false;
    }

    inline bool apply_plc_feedback_frame(const RosCanFrame &frame, ArmMsg &status)
    {
        const uint32_t base_id = frame.id & CanBaseMask;
        if (base_id == CanBasePlcToAiStatus)
        {
            status.raw.plc_to_ai_status = frame.data;
            status.readable.plc_to_ai_status = decode_plc_to_ai_status(frame.data);
            return true;
        }
        if (base_id == CanBasePlcToAiTelemetry300)
        {
            status.raw.plc_to_ai_telemetry_300 = frame.data;
            status.readable.plc_to_ai_telemetry_300 = decode_plc_to_ai_telemetry_300(frame.data);
            return true;
        }
        if (base_id == CanBasePlcToAiTelemetry400)
        {
            status.raw.plc_to_ai_telemetry_400 = frame.data;
            status.readable.plc_to_ai_telemetry_400 = decode_plc_to_ai_telemetry_400(frame.data);
            return true;
        }
        return false;
    }

    inline nlohmann::json to_json(const AiToPlcSwitchBits &bits)
    {
        return nlohmann::json::object(
            {{"cmd_light_enable", bits.cmd_light_enable},
             {"cmd_boom_speed_high", bits.cmd_boom_speed_high},
             {"cmd_boom_speed_mid", bits.cmd_boom_speed_mid},
             {"cmd_boom_speed_low", bits.cmd_boom_speed_low},
             {"cmd_platform_load_mode", bits.cmd_platform_load_mode},
             {"cmd_drive_speed_high", bits.cmd_drive_speed_high},
             {"cmd_drive_speed_mid", bits.cmd_drive_speed_mid},
             {"cmd_drive_speed_low", bits.cmd_drive_speed_low},
             {"cmd_enable_start", bits.cmd_enable_start},
             {"cmd_horn", bits.cmd_horn},
             {"cmd_clamp_close", bits.cmd_clamp_close},
             {"cmd_clamp_open", bits.cmd_clamp_open},
             {"cmd_clamp_rotate_cw", bits.cmd_clamp_rotate_cw},
             {"cmd_clamp_rotate_ccw", bits.cmd_clamp_rotate_ccw},
             {"cmd_clamp_move_left", bits.cmd_clamp_move_left},
             {"cmd_clamp_move_right", bits.cmd_clamp_move_right},
             {"cmd_clamp_move_up", bits.cmd_clamp_move_up},
             {"cmd_clamp_move_down", bits.cmd_clamp_move_down},
             {"cmd_front_area_confirm_1", bits.cmd_front_area_confirm_1},
             {"cmd_front_area_confirm_2", bits.cmd_front_area_confirm_2},
             {"cmd_platform_level_up", bits.cmd_platform_level_up},
             {"cmd_platform_level_down", bits.cmd_platform_level_down},
             {"cmd_platform_rotate_cw", bits.cmd_platform_rotate_cw},
             {"cmd_platform_rotate_ccw", bits.cmd_platform_rotate_ccw},
             {"cmd_fly_boom_up", bits.cmd_fly_boom_up},
             {"cmd_fly_boom_down", bits.cmd_fly_boom_down},
             {"cmd_fly_boom_turn_left", bits.cmd_fly_boom_turn_left},
             {"cmd_fly_boom_turn_right", bits.cmd_fly_boom_turn_right},
             {"cmd_main_boom_extend", bits.cmd_main_boom_extend},
             {"cmd_main_boom_retract", bits.cmd_main_boom_retract},
             {"cmd_fold_boom_up_reserved", bits.cmd_fold_boom_up_reserved},
             {"cmd_fold_boom_down_reserved", bits.cmd_fold_boom_down_reserved},
             {"status_emergency_or_comm_lost", bits.status_emergency_or_comm_lost}});
    }

    inline nlohmann::json to_json(const AiToPlcAnalogFields &analog)
    {
        return nlohmann::json::object(
            {{"cmd_main_boom_luff_raw", analog.cmd_main_boom_luff_raw},
             {"cmd_turntable_rotate_raw", analog.cmd_turntable_rotate_raw},
             {"cmd_wheel_steer_raw", analog.cmd_wheel_steer_raw},
             {"cmd_drive_motion_raw", analog.cmd_drive_motion_raw},
             {"cmd_reserved_5_raw", analog.cmd_reserved_5_raw},
             {"cmd_reserved_6_raw", analog.cmd_reserved_6_raw},
             {"cmd_reserved_7_raw", analog.cmd_reserved_7_raw},
             {"cmd_reserved_8_raw", analog.cmd_reserved_8_raw}});
    }

    inline nlohmann::json to_json(const PlcToAiStatusBits &status)
    {
        return nlohmann::json::object(
            {{"fb_enable_state", status.fb_enable_state},
             {"fb_alarm_active", status.fb_alarm_active},
             {"fb_overload_active", status.fb_overload_active},
             {"fb_over_range_active", status.fb_over_range_active},
             {"fb_front_area_active", status.fb_front_area_active},
             {"fb_reserved_bit5", status.fb_reserved_bit5},
             {"fb_reserved_bit6", status.fb_reserved_bit6},
             {"fb_reserved_bit7", status.fb_reserved_bit7}});
    }

    inline nlohmann::json to_json(const PlcToAiTelemetry300 &telemetry)
    {
        return nlohmann::json::object(
            {{"fb_main_boom_angle_raw", telemetry.fb_main_boom_angle_raw},
             {"fb_main_boom_angle_deg", telemetry.fb_main_boom_angle_deg},
             {"fb_main_boom_length_raw", telemetry.fb_main_boom_length_raw},
             {"fb_main_boom_length_m", telemetry.fb_main_boom_length_m},
             {"fb_chassis_tilt_x_raw", telemetry.fb_chassis_tilt_x_raw},
             {"fb_chassis_tilt_x_deg", telemetry.fb_chassis_tilt_x_deg},
             {"fb_chassis_tilt_y_raw", telemetry.fb_chassis_tilt_y_raw},
             {"fb_chassis_tilt_y_deg", telemetry.fb_chassis_tilt_y_deg}});
    }

    inline nlohmann::json to_json(const PlcToAiTelemetry400 &telemetry)
    {
        return nlohmann::json::object(
            {{"fb_platform_weight_raw", telemetry.fb_platform_weight_raw},
             {"fb_platform_weight_kg", telemetry.fb_platform_weight_kg},
             {"fb_fly_boom_angle_raw", telemetry.fb_fly_boom_angle_raw},
             {"fb_fly_boom_angle_deg", telemetry.fb_fly_boom_angle_deg},
             {"fb_clamp_rotate_angle_raw", telemetry.fb_clamp_rotate_angle_raw},
             {"fb_clamp_rotate_angle_deg", telemetry.fb_clamp_rotate_angle_deg},
             {"fb_reserved_7_8_raw", telemetry.fb_reserved_7_8_raw},
             {"fb_reserved_7_8_value", telemetry.fb_reserved_7_8_value}});
    }

    inline nlohmann::json to_json(const RosCanFrame &frame)
    {
        return nlohmann::json::object(
            {{"id", frame.id},
             {"is_extended", frame.is_extended},
             {"is_rtr", frame.is_rtr},
             {"is_error", frame.is_error},
             {"dlc", frame.dlc},
             {"data", to_int_array(frame.data)},
             {"data_hex", bytes_to_hex(frame.data)}});
    }

    inline nlohmann::json to_json(const ArmMsgRaw &raw)
    {
        return nlohmann::json::object(
            {{"ai_to_plc_switch", to_int_array(raw.ai_to_plc_switch)},
             {"ai_to_plc_switch_hex", bytes_to_hex(raw.ai_to_plc_switch)},
             {"plc_to_ai_status", to_int_array(raw.plc_to_ai_status)},
             {"plc_to_ai_status_hex", bytes_to_hex(raw.plc_to_ai_status)},
             {"ai_to_plc_analog", to_int_array(raw.ai_to_plc_analog)},
             {"ai_to_plc_analog_hex", bytes_to_hex(raw.ai_to_plc_analog)},
             {"plc_to_ai_telemetry_300", to_int_array(raw.plc_to_ai_telemetry_300)},
             {"plc_to_ai_telemetry_300_hex", bytes_to_hex(raw.plc_to_ai_telemetry_300)},
             {"plc_to_ai_telemetry_400", to_int_array(raw.plc_to_ai_telemetry_400)},
             {"plc_to_ai_telemetry_400_hex", bytes_to_hex(raw.plc_to_ai_telemetry_400)}});
    }

    inline nlohmann::json to_json(const ArmMsgReadable &readable)
    {
        return nlohmann::json::object(
            {{"ai_to_plc_switch", to_json(readable.ai_to_plc_switch)},
             {"ai_to_plc_analog", to_json(readable.ai_to_plc_analog)},
             {"plc_to_ai_status", to_json(readable.plc_to_ai_status)},
             {"plc_to_ai_telemetry_300", to_json(readable.plc_to_ai_telemetry_300)},
             {"plc_to_ai_telemetry_400", to_json(readable.plc_to_ai_telemetry_400)}});
    }

    inline nlohmann::json to_json(const ArmMsg &status)
    {
        return nlohmann::json::object(
            {{"robot_id", status.robot_id},
             {"raw", to_json(status.raw)},
             {"readable", to_json(status.readable)}});
    }

} // namespace arm_control_package
