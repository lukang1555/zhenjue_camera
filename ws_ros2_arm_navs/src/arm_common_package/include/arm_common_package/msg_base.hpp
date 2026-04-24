#pragma once

#include <chrono>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

namespace arm_common_package
{

    enum class MessageType
    {
        Payload,
        Ack,
        Ping,
        Pong,
        Unknown,
    };

    namespace comm_code
    {
        constexpr int Ok = 0;
        constexpr int BadFormat = 1001;
        constexpr int BadType = 1002;
        constexpr int BadApi = 1003;
        constexpr int Unauthorized = 1004;
        constexpr int TooLarge = 1005;
        constexpr int RateLimit = 1006;
        constexpr int DuplicateMsg = 1007;
        constexpr int InternalError = 1010;
    } // namespace comm_code

    struct MessageEnvelope
    {
        std::string v{"1.0"};
        std::string type{"payload"};  // none, payload, ack, ping, pong，其中none啥都不处理
        std::string api;
        std::string msg_id;
        std::string identity;
        int code{0};
        nlohmann::json body = nlohmann::json::object();
        nlohmann::json ext = nlohmann::json::object();
        int64_t ts{0};
    };

    class MessageCodec
    {
    public:
        static int64_t now_ms()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
        }

        static std::string gen_msg_id()
        {
            static thread_local std::mt19937_64 rng(std::random_device{}());
            static constexpr char hex[] = "0123456789abcdef";

            auto rand_hex = [&]()
            {
                return hex[rng() % 16];
            };

            std::string out;
            out.reserve(36);
            const int groups[] = {8, 4, 4, 4, 12};
            for (size_t g = 0; g < 5; ++g)
            {
                for (int i = 0; i < groups[g]; ++i)
                {
                    out.push_back(rand_hex());
                }
                if (g != 4)
                {
                    out.push_back('-');
                }
            }
            return out;
        }

        static MessageType parse_type(const std::string &type)
        {
            if (type == "payload")
            {
                return MessageType::Payload;
            }
            if (type == "ack")
            {
                return MessageType::Ack;
            }
            if (type == "ping")
            {
                return MessageType::Ping;
            }
            if (type == "pong")
            {
                return MessageType::Pong;
            }
            return MessageType::Unknown;
        }

        static bool parse(const std::string &raw, MessageEnvelope &out, int &error_code, std::string &error_text)
        {
            error_code = comm_code::Ok;
            error_text.clear();

            nlohmann::json parsed;
            try
            {
                parsed = nlohmann::json::parse(raw);
            }
            catch (const std::exception &ex)
            {
                error_code = comm_code::BadFormat;
                error_text = ex.what();
                return false;
            }

            if (!parsed.is_object())
            {
                error_code = comm_code::BadFormat;
                error_text = "envelope must be a JSON object";
                return false;
            }

            auto required_string = [&](const char *key, std::string &dst) -> bool
            {
                if (!parsed.contains(key) || !parsed[key].is_string())
                {
                    error_code = comm_code::BadFormat;
                    error_text = std::string("missing or invalid field: ") + key;
                    return false;
                }
                dst = parsed[key].get<std::string>();
                return true;
            };

            if (!required_string("v", out.v) || !required_string("type", out.type) ||
                !required_string("api", out.api) || !required_string("msg_id", out.msg_id) ||
                !required_string("identity", out.identity))
            {
                return false;
            }

            if (!parsed.contains("code") || !parsed["code"].is_number_integer())
            {
                error_code = comm_code::BadFormat;
                error_text = "missing or invalid field: code";
                return false;
            }
            out.code = parsed["code"].get<int>();

            if (!parsed.contains("body") || !parsed["body"].is_object())
            {
                error_code = comm_code::BadFormat;
                error_text = "missing or invalid field: body";
                return false;
            }
            out.body = parsed["body"];

            if (parsed.contains("ext"))
            {
                if (!parsed["ext"].is_object())
                {
                    error_code = comm_code::BadFormat;
                    error_text = "invalid field: ext";
                    return false;
                }
                out.ext = parsed["ext"];
            }
            else
            {
                out.ext = nlohmann::json::object();
            }

            if (!parsed.contains("ts") || !parsed["ts"].is_number_integer())
            {
                error_code = comm_code::BadFormat;
                error_text = "missing or invalid field: ts";
                return false;
            }
            out.ts = parsed["ts"].get<int64_t>();

            if (parse_type(out.type) == MessageType::Unknown)
            {
                error_code = comm_code::BadType;
                error_text = "unsupported message type";
                return false;
            }

            return true;
        }

        static nlohmann::ordered_json to_ordered_json(const MessageEnvelope &msg)
        {
            nlohmann::ordered_json out;
            out["v"] = msg.v;
            out["type"] = msg.type;
            out["api"] = msg.api;
            out["msg_id"] = msg.msg_id;
            out["identity"] = msg.identity;
            out["code"] = msg.code;
            out["body"] = msg.body;
            out["ext"] = msg.ext;
            out["ts"] = msg.ts;
            return out;
        }

        static std::string dump(const MessageEnvelope &msg)
        {
            return to_ordered_json(msg).dump();
        }

        static MessageEnvelope make_ack(const MessageEnvelope &src, const std::string &identity, int code)
        {
            MessageEnvelope ack;
            ack.type = "ack";
            ack.api = src.api;
            ack.msg_id = gen_msg_id();
            ack.identity = identity;
            ack.code = code;
            ack.body = nlohmann::json::object({{"ack_msg_id", src.msg_id}});
            ack.ext = nlohmann::json::object();
            ack.ts = now_ms();
            return ack;
        }

        static MessageEnvelope make_pong(const MessageEnvelope &ping, const std::string &identity)
        {
            MessageEnvelope pong;
            pong.type = "pong";
            pong.api = "SYS_HEARTBEAT";
            pong.msg_id = gen_msg_id();
            pong.identity = identity;
            pong.code = comm_code::Ok;
            pong.body = nlohmann::json::object();
            pong.ext = nlohmann::json::object({{"echo_msg_id", ping.msg_id}});
            pong.ts = now_ms();
            return pong;
        }

        static MessageEnvelope make_error_ack(
            const std::string &api,
            const std::string &ack_msg_id,
            const std::string &identity,
            int code,
            const std::string &reason)
        {
            MessageEnvelope ack;
            ack.type = "ack";
            ack.api = api.empty() ? "SYS_PROTOCOL" : api;
            ack.msg_id = gen_msg_id();
            ack.identity = identity;
            ack.code = code;
            ack.body = nlohmann::json::object();
            if (!ack_msg_id.empty())
            {
                ack.body["ack_msg_id"] = ack_msg_id;
            }
            ack.ext = nlohmann::json::object({{"reason", reason}});
            ack.ts = now_ms();
            return ack;
        }
    };

} // namespace arm_common_package
