/*

    iort_interface.cpp

*/

#include <cpr/cpr.h>
#include "iort_lib/iort.hpp"

#include <chrono>
using namespace std::chrono_literals;

// #define DEBUG
#ifdef DEBUG
#include <iostream>
#include <deque>
#endif

namespace iort
{
static const std::string FUNCTION_URL =
    "https://5p1y6wnp3k.execute-api.us-west-2.amazonaws.com/default/"
    "iort_lib_query";

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
    const int32_t rateus = 100000;
    auto cur_time = std::chrono::system_clock::now();
    while (exitSig.wait_until(cur_time + std::chrono::microseconds(rateus)) ==
           std::future_status::timeout)
    {
        cur_time = std::chrono::system_clock::now();
        Json::Value payload;
        if (inline_get(uuid, payload, 2000))
        {
#ifdef DEBUG
            const auto epoch =
                std::chrono::system_clock::now().time_since_epoch();
            const auto us =
                std::chrono::duration_cast<std::chrono::microseconds>(epoch);
            int64_t then = payload["time"].asInt64();
            std::cout << "latency: " << us.count() - then << "\n";
#endif
            std::string new_msg_uuid = payload["msgid"].asString();
            if (msg_uuid != new_msg_uuid)
            {
                msg_uuid = new_msg_uuid;
                cb_queue.push({ cb, payload["data"] });
            }
        }
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
