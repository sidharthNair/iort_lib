/*

    iort_interface.cpp

*/

#include <mqtt/async_client.h>
#include <mqtt/message.h>
#include <cpr/cpr.h>
#include "iort_lib/iort.hpp"

#include <string>
#include <chrono>
using namespace std::chrono_literals;

#include <config.h>

//#define DEBUG
#ifdef DEBUG
#include <iostream>
#include <deque>
#endif

namespace iort
{

static const std::string FUNCTION_URL = HTTP_ENDPOINT;
static const std::string ENDPOINT_URL = "ssl://"
					  MQTT_ENDPOINT
					  ":8883";

inline bool inline_get(const std::string& uuid, Json::Value& ret,
                       int32_t timeout = 1000)
{
    cpr::Response r =
        cpr::Post(cpr::Url{ FUNCTION_URL },
                  cpr::Parameters{ { "uuid", uuid }, { "op", "get" } },
                  cpr::Timeout(timeout));

    // return false if server error or timeout
    if ((r.status_code != 200) || ((r.elapsed * 1000 + 1 - timeout) > 0))
    {
#ifdef DEBUG
        std::cout << "status: " << r.status_code << "\ntext: " << r.text
                  << "\n";
#endif
        return false;
    }

    Json::Reader reader;
    return reader.parse(r.text, ret);
}

inline bool inline_query(const std::string& query_string, Json::Value& ret,
                         int32_t timeout = 1000)
{
    cpr::Response r =
        cpr::Post(cpr::Url{ FUNCTION_URL },
                  cpr::Parameters{ { "qs", query_string }, { "op", "query" } },
                  cpr::Timeout(timeout));

    // return false if server error or timeout
    if ((r.status_code != 200) || ((r.elapsed * 1000 + 1 - timeout) > 0))
    {
#ifdef DEBUG
        std::cout << "status: " << r.status_code << "\ntext: " << r.text
                  << "\n";
#endif
        return false;
    }

    Json::Reader reader;
    return reader.parse(r.text, ret);
}

int Subscriber::id = 0;

Subscriber::Subscriber(const std::string& uuid_,
                       const std::function<void(Json::Value)>& cb_,
                       CallbackQueue& cb_queue_, const int32_t timeout_,
                       const int32_t failure_count_)
    : cb_queue(cb_queue_)
{
    uuid = uuid_;
    msg_uuid = "null";
    cb = cb_;
    timeout = timeout_;
    topic = "device/" + uuid_ + "/data";
    failure_count = 0;
    max_failure_count = failure_count_;

    running = false;
    running = start();
}

Subscriber::~Subscriber()
{
    stop();
}

bool Subscriber::isRunning(void)
{
    return running;
}

bool Subscriber::start(void)
{
    if (running) return false;
    exitCond = new std::promise<void>();
    subThread =
        std::thread(&Subscriber::run, this, std::move(exitCond->get_future()));
    return true;
}

bool Subscriber::stop(void)
{
    if (!running) return false;
    exitCond->set_value();
    subThread.join();
    running = false;
    return true;
}

void Subscriber::run(std::future<void> exitSig)
{
    mqtt::async_client cli(ENDPOINT_URL, "client_" + std::to_string(id++));
    auto sslopts = mqtt::ssl_options_builder()
               .private_key(KEY_PATH)
		.key_store(CERT_PATH)
		.trust_store(CA_PATH)
		.private_keypassword("")
		.ssl_version(3)
		.error_handler([](const std::string& msg) {
			std::cerr << "SSL Error: " << msg << std::endl;
		})
					   .finalize();
    auto connOpts = mqtt::connect_options_builder()
		.clean_session(true)
		.ssl(std::move(sslopts))
		.finalize();
    cli.start_consuming();
#ifdef DEBUG
    std::cout << "Connecting to the MQTT server " << ENDPOINT_URL << "..." << std::endl;
#endif
    auto tok = cli.connect(connOpts);
    auto rsp = tok->get_connect_response();
    if (!rsp.is_session_present())
    	cli.subscribe(topic, 1)->wait();
#ifdef DEBUG
    std::cout << cli.get_client_id() << " waiting for messages on topic: '" << topic << "'" << std::endl;
#endif
    while (exitSig.wait_for(1ms) == std::future_status::timeout)
    {
        Json::Value payload;
        mqtt::const_message_ptr msg;
        if (!cli.try_consume_message_for(&msg, 10ms))
            continue;
        reader.parse(msg->to_string(), payload);
#ifdef DEBUG
        const auto epoch =
            std::chrono::system_clock::now().time_since_epoch();
        const auto us =
            std::chrono::duration_cast<std::chrono::microseconds>(epoch);
        int64_t then = payload["time"].asInt64();
        std::cout << "latency: " << us.count() - then << "\n";
#endif
        cb_queue.push({ cb, payload["data"] });
    }
    if (cli.is_connected()) {
#ifdef DEBUG
	std::cout << "\nShutting down and disconnecting from the MQTT server..." << std::endl;
#endif
	cli.unsubscribe(topic)->wait();
	cli.stop_consuming();
	cli.disconnect()->wait();
	}
    else {
#ifdef DEBUG
	std::cout << "\nClient was disconnected" << std::endl;
#endif
    } 
}

Core::Core()
{
    exitCond = new std::promise<void>();
    callbackThread =
        std::thread(&Core::run, this, std::move(exitCond->get_future()));
}

Core::~Core()
{
    exitCond->set_value();
    callbackThread.join();
}

void Core::run(std::future<void> extiSig)
{
    while (extiSig.wait_for(1ms) == std::future_status::timeout)
    {
        if (!callbackQueue.empty())
        {
#ifdef DEBUG
            std::cout << "callback called"
                      << "\n";
#endif
            CallbackQueueItem qitem = callbackQueue.front();
            qitem.cb(qitem.data);
            callbackQueue.pop();
        }
    }
}

bool Core::get(const std::string& uuid, Json::Value& ret, int32_t timeout)
{
    return inline_get(uuid, ret, timeout);
}

bool Core::query(const std::string& query_string, Json::Value& ret,
                 int32_t timeout)
{
    return inline_query(query_string, ret, timeout);
}

}    // end namespace iort
