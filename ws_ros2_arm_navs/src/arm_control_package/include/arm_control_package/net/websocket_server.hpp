#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "arm_control_package/base/global.hpp"

namespace arm_control_package
{
	class WebSocketServer
	{
	public:
		struct Options
		{
			std::string bind_address{"0.0.0.0"};
			uint16_t port{9002};
			std::string identity{"ROBOT-CTRL-0001"};
			size_t max_message_bytes{262144};
			int64_t duplicate_ttl_ms{60000};
			size_t high_priority_worker_threads{2};
			size_t normal_priority_worker_threads{4};
		};

		enum class ApiPriority
		{
			High,
			Normal,
		};

		struct HandlerOptions
		{
			ApiPriority priority{ApiPriority::Normal};
		};

		using ApiHandler = std::function<std::vector<arm_common_package::MessageEnvelope>(const arm_common_package::MessageEnvelope &)>;
		using LogHandler = std::function<void(const std::string &, const std::string &)>;

		explicit WebSocketServer(Options options, LogHandler log_handler)
			: options_(std::move(options)),
			  log_handler_(std::move(log_handler))
		{
		}

		void register_handler(const std::string &api, ApiHandler handler)
		{
			register_handler(api, std::move(handler), HandlerOptions{});
		}

		void register_handler(const std::string &api, ApiHandler handler, const HandlerOptions &options)
		{
			{
				std::lock_guard<std::mutex> lock(handler_mutex_);
				handlers_[api].push_back(HandlerRegistration{std::move(handler), options});
			}
			ensure_api_pool(api, options.priority);
		}

		void send_message(arm_common_package::MessageEnvelope message)
		{
			normalize_outgoing(message);
			for (const auto &session : get_active_sessions())
			{
				send_message(message, session);
			}
		}

		~WebSocketServer()
		{
			stop();
		}

		bool start()
		{
			if (running_.load())
			{
				return true;
			}

			boost::system::error_code ec;
			const auto address = boost::asio::ip::make_address(options_.bind_address, ec);
			if (ec)
			{
				log("ERROR", "invalid bind address: " + options_.bind_address + ", error=" + ec.message());
				return false;
			}

			acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(ioc_);
			const boost::asio::ip::tcp::endpoint endpoint(address, options_.port);

			acceptor_->open(endpoint.protocol(), ec);
			if (ec)
			{
				log("ERROR", "acceptor open failed: " + ec.message());
				return false;
			}

			acceptor_->set_option(boost::asio::socket_base::reuse_address(true), ec);
			if (ec)
			{
				log("ERROR", "acceptor set_option failed: " + ec.message());
				return false;
			}

			acceptor_->bind(endpoint, ec);
			if (ec)
			{
				log("ERROR", "acceptor bind failed: " + ec.message());
				return false;
			}

			acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
			if (ec)
			{
				log("ERROR", "acceptor listen failed: " + ec.message());
				return false;
			}

			running_.store(true);
			accept_thread_ = std::thread(&WebSocketServer::accept_loop, this);
			log("INFO", "websocket server started at " + options_.bind_address + ":" + std::to_string(options_.port));
			return true;
		}

		void stop()
		{
			if (!running_.exchange(false))
			{
				return;
			}

			if (acceptor_ != nullptr)
			{
				boost::system::error_code ec;
				acceptor_->close(ec);
			}

			if (accept_thread_.joinable())
			{
				accept_thread_.join();
			}

			stop_all_api_pools();

			log("INFO", "websocket server stopped");
		}

	private:
		using tcp = boost::asio::ip::tcp;
		using websocket_stream = boost::beast::websocket::stream<tcp::socket>;

		struct SessionContext
		{
			explicit SessionContext(tcp::socket socket)
				: ws(std::move(socket))
			{
			}

			websocket_stream ws;
			std::mutex write_mutex;
			std::atomic<bool> active{true};
		};

		struct HandlerRegistration
		{
			ApiHandler handler;
			HandlerOptions options;
		};

		struct DispatchTask
		{
			HandlerRegistration registration;
			arm_common_package::MessageEnvelope message;
			std::weak_ptr<SessionContext> session;
		};

		struct WorkerPool
		{
			std::mutex mutex;
			std::condition_variable cv;
			std::deque<DispatchTask> queue;
			std::vector<std::thread> workers;
			bool stopping{false};
			std::string name;
			ApiPriority priority{ApiPriority::Normal};
		};

		void log(const std::string &level, const std::string &text)
		{
			if (log_handler_)
			{
				log_handler_(level, text);
			}
		}

		void add_session(const std::shared_ptr<SessionContext> &session)
		{
			std::lock_guard<std::mutex> lock(session_mutex_);
			sessions_.push_back(session);
		}

		std::vector<std::shared_ptr<SessionContext>> get_active_sessions()
		{
			std::vector<std::shared_ptr<SessionContext>> active_sessions;
			std::lock_guard<std::mutex> lock(session_mutex_);
			for (auto iter = sessions_.begin(); iter != sessions_.end();)
			{
				auto session = iter->lock();
				if (!session || !session->active.load())
				{
					iter = sessions_.erase(iter);
					continue;
				}

				active_sessions.push_back(std::move(session));
				++iter;
			}
			return active_sessions;
		}

		void accept_loop()
		{
			while (running_.load())
			{
				tcp::socket socket(ioc_);
				boost::system::error_code ec;
				acceptor_->accept(socket, ec);

				if (!running_.load())
				{
					break;
				}

				if (ec)
				{
					log("WARN", "accept failed: " + ec.message());
					continue;
				}

				std::thread(&WebSocketServer::session_loop, this, std::move(socket)).detach();
			}
		}

		void session_loop(tcp::socket socket)
		{
			auto session = std::make_shared<SessionContext>(std::move(socket));
			session->ws.read_message_max(options_.max_message_bytes);
			session->ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

			boost::system::error_code ec;
			session->ws.accept(ec);
			if (ec)
			{
				log("WARN", "websocket handshake failed: " + ec.message());
				return;
			}

			add_session(session);

			while (running_.load() && session->active.load())
			{
				boost::beast::flat_buffer buffer;
				session->ws.read(buffer, ec);
				if (ec == boost::beast::websocket::error::closed)
				{
					session->active.store(false);
					return;
				}
				if (ec)
				{
					session->active.store(false);
					log("WARN", "websocket read failed: " + ec.message());
					return;
				}

				const std::string raw = boost::beast::buffers_to_string(buffer.data());
				handle_message(raw, session);
			}

			session->active.store(false);
		}

		void handle_message(const std::string &raw, const std::shared_ptr<SessionContext> &session)
		{
			using arm_common_package::MessageCodec;
			using arm_common_package::MessageEnvelope;
			using arm_common_package::MessageType;

			if (raw.size() > options_.max_message_bytes)
			{
				const auto ack = MessageCodec::make_error_ack("SYS_PROTOCOL", "", options_.identity, arm_common_package::comm_code::TooLarge, "payload too large");
				send_message(ack, session);
				return;
			}

			MessageEnvelope message;
			int parse_code = arm_common_package::comm_code::Ok;
			std::string parse_err;
			if (!MessageCodec::parse(raw, message, parse_code, parse_err))
			{
				std::string api = "SYS_PROTOCOL";
				std::string ack_msg_id;
				const auto partial = nlohmann::json::parse(raw, nullptr, false);
				if (partial.is_object())
				{
					if (partial.contains("api") && partial["api"].is_string())
					{
						api = partial["api"].get<std::string>();
					}
					if (partial.contains("msg_id") && partial["msg_id"].is_string())
					{
						ack_msg_id = partial["msg_id"].get<std::string>();
					}
				}
				const auto err_ack = MessageCodec::make_error_ack(api, ack_msg_id, options_.identity, parse_code, parse_err);
				send_message(err_ack, session);
				return;
			}

			const auto msg_type = MessageCodec::parse_type(message.type);
			if ((msg_type == MessageType::Payload || msg_type == MessageType::Ping) && !remember_msg_id(message.msg_id))
			{
				const auto dup_ack = MessageCodec::make_error_ack(
					message.api,
					message.msg_id,
					options_.identity,
					arm_common_package::comm_code::DuplicateMsg,
					"duplicate msg_id");
				send_message(dup_ack, session);
				return;
			}

			if (msg_type == MessageType::Ping)
			{
				send_message(MessageCodec::make_pong(message, options_.identity), session);
				return;
			}

			if (msg_type == MessageType::Payload)
			{
				const auto handlers = get_handlers(message.api);
				if (handlers.empty())
				{
					const auto bad_api_ack = MessageCodec::make_error_ack(
						message.api,
						message.msg_id,
						options_.identity,
						arm_common_package::comm_code::BadApi,
						"api not registered");
					send_message(bad_api_ack, session);
					return;
				}

				send_message(MessageCodec::make_ack(message, options_.identity, arm_common_package::comm_code::Ok), session);

				for (const auto &handler : handlers)
				{
					enqueue_task(message.api, DispatchTask{handler, message, session});
				}
				return;
			}
		}

		std::vector<HandlerRegistration> get_handlers(const std::string &api)
		{
			std::lock_guard<std::mutex> lock(handler_mutex_);
			const auto iter = handlers_.find(api);
			if (iter == handlers_.end())
			{
				return {};
			}
			return iter->second;
		}

		void ensure_api_pool(const std::string &api, ApiPriority priority)
		{
			std::lock_guard<std::mutex> lock(pool_mutex_);
			if (api_pools_.count(api) != 0)
			{
				return;
			}

			auto pool = std::make_shared<WorkerPool>();
			pool->name = api;
			pool->priority = priority;
			const size_t thread_count =
				priority == ApiPriority::High ? options_.high_priority_worker_threads : options_.normal_priority_worker_threads;
			start_pool_workers(*pool, thread_count);
			api_pools_[api] = std::move(pool);
		}

		void start_pool_workers(WorkerPool &pool, size_t count)
		{
			const size_t thread_count = std::max<size_t>(1, count);
			for (size_t i = 0; i < thread_count; ++i)
			{
				pool.workers.emplace_back([this, &pool]()
										  {
                while (true)
                {
                    DispatchTask task;
                    {
                        std::unique_lock<std::mutex> lock(pool.mutex);
                        pool.cv.wait(lock, [&pool]()
                        {
                            return pool.stopping || !pool.queue.empty();
                        });
                        if (pool.stopping && pool.queue.empty())
                        {
                            return;
                        }
                        task = std::move(pool.queue.front());
                        pool.queue.pop_front();
                    }

                    if (!task.registration.handler)
                    {
                        continue;
                    }

                    auto responses = task.registration.handler(task.message);
                    auto session = task.session.lock();
                    if (!session || !session->active.load())
                    {
                        continue;
                    }

                    for (auto response : responses)
                    {
                        normalize_outgoing(response);
                        send_message(response, session);
                    }
                } });
			}
		}

		void stop_all_api_pools()
		{
			std::vector<std::shared_ptr<WorkerPool>> pools;
			{
				std::lock_guard<std::mutex> lock(pool_mutex_);
				for (auto &item : api_pools_)
				{
					pools.push_back(item.second);
				}
				api_pools_.clear();
			}

			for (auto &pool : pools)
			{
				stop_pool(*pool);
			}
		}

		void stop_pool(WorkerPool &pool)
		{
			{
				std::lock_guard<std::mutex> lock(pool.mutex);
				pool.stopping = true;
			}
			pool.cv.notify_all();
			for (auto &worker : pool.workers)
			{
				if (worker.joinable())
				{
					worker.join();
				}
			}
			pool.workers.clear();
			{
				std::lock_guard<std::mutex> lock(pool.mutex);
				pool.queue.clear();
				pool.stopping = false;
			}
		}

		void enqueue_task(const std::string &api, DispatchTask task)
		{
			std::shared_ptr<WorkerPool> pool;
			{
				std::lock_guard<std::mutex> lock(pool_mutex_);
				const auto iter = api_pools_.find(api);
				if (iter != api_pools_.end())
				{
					pool = iter->second;
				}
			}

			if (!pool)
			{
				auto responses = task.registration.handler(task.message);
				auto session = task.session.lock();
				if (!session || !session->active.load())
				{
					return;
				}
				for (auto response : responses)
				{
					if (response.type == "none")
					{
						continue;
					}
					normalize_outgoing(response);
					send_message(response, session);
				}
				return;
			}

			{
				std::lock_guard<std::mutex> lock(pool->mutex);
				pool->queue.push_back(std::move(task));
			}
			pool->cv.notify_one();
		}

		void normalize_outgoing(arm_common_package::MessageEnvelope &message) const
		{
			if (message.v.empty())
			{
				message.v = "1.0";
			}
			if (message.type.empty())
			{
				message.type = "payload";
			}
			if (message.msg_id.empty())
			{
				message.msg_id = arm_common_package::MessageCodec::gen_msg_id();
			}
			if (message.identity.empty())
			{
				message.identity = options_.identity;
			}
			if (message.body.is_null())
			{
				message.body = nlohmann::json::object();
			}
			if (message.ext.is_null())
			{
				message.ext = nlohmann::json::object();
			}
			if (message.ts == 0)
			{
				message.ts = arm_common_package::MessageCodec::now_ms();
			}
		}

		void send_message(const arm_common_package::MessageEnvelope &message, const std::shared_ptr<SessionContext> &session)
		{
			if (!session || !session->active.load())
			{
				return;
			}

			const std::string payload = arm_common_package::MessageCodec::dump(message);
			boost::system::error_code ec;

			std::lock_guard<std::mutex> lock(session->write_mutex);
			if (!session->active.load())
			{
				return;
			}

			session->ws.text(true);
			session->ws.write(boost::asio::buffer(payload), ec);
			if (ec)
			{
				session->active.store(false);
				log("WARN", "websocket write failed: " + ec.message());
			}
		}

		bool remember_msg_id(const std::string &msg_id)
		{
			const int64_t now = arm_common_package::MessageCodec::now_ms();
			std::lock_guard<std::mutex> lock(cache_mutex_);

			for (auto iter = msg_id_cache_.begin(); iter != msg_id_cache_.end();)
			{
				if (now - iter->second > options_.duplicate_ttl_ms)
				{
					iter = msg_id_cache_.erase(iter);
				}
				else
				{
					++iter;
				}
			}

			if (msg_id_cache_.count(msg_id) != 0)
			{
				return false;
			}

			msg_id_cache_[msg_id] = now;
			return true;
		}

		Options options_;
		LogHandler log_handler_;
		std::mutex handler_mutex_;
		std::unordered_map<std::string, std::vector<HandlerRegistration>> handlers_;

		std::mutex pool_mutex_;
		std::unordered_map<std::string, std::shared_ptr<WorkerPool>> api_pools_;

		std::mutex session_mutex_;
		std::vector<std::weak_ptr<SessionContext>> sessions_;

		std::atomic<bool> running_{false};
		boost::asio::io_context ioc_;
		std::unique_ptr<tcp::acceptor> acceptor_;
		std::thread accept_thread_;

		std::mutex cache_mutex_;
		std::unordered_map<std::string, int64_t> msg_id_cache_;
	};

} // namespace arm_control_package
