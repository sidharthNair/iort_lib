/*

    iort_interface.cpp

*/

#include <cpr/cpr.h>
#include "iort_lib/iort.hpp"

#include <chrono>
using namespace std::chrono_literals;

#define DEBUG
#ifdef DEBUG
#include <iostream>
#include <deque>
#endif

namespace iort
{

static const std::string FUNCTION_URL = "https://sf0nkng705.execute-api.us-east-2.amazonaws.com/prod/get-latest-by-uuid";

Subscriber::Subscriber(const std::string& uuid_, 
            const boost::function<void(Json::Value)> & cb_, 
            const int32_t timeout_,
            const int32_t timeout_count_)
{
    uuid = uuid_;
    msg_uuid = "null";
    cb = cb_;
    timeout = timeout_;
    timeout_count = 0;
    max_timeout_count = timeout_count_;

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
    subThread = std::thread(&Subscriber::run, this, 
                            std::move(exitCond->get_future()));
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

void Subscriber::run (std::future<void> exitSig)
{
#ifdef DEBUG
    std::deque<int64_t> running_avg;
#endif
    while (exitSig.wait_for(1ms) == std::future_status::timeout) {
        cpr::Response r = cpr::Post(
            cpr::Url{FUNCTION_URL},
            cpr::Body{"{\"uuid\" : \"" + uuid + "\",\"points\" : \"1\"}"}
        );

        if (r.status_code != 200) {
            std::cout << r.text << "\n";
            continue;
        }

        if (r.elapsed * 1000 + 1 - timeout > 0) {
            if (++timeout_count >= max_timeout_count) break;
        } else {
            timeout_count = 0;
        }

        Json::Value payload;
        Json::Reader reader;
        if (reader.parse(r.text, payload)) {
#ifdef DEBUG
            const auto epoch = std::chrono::system_clock::now().time_since_epoch();
            const auto us = std::chrono::duration_cast<std::chrono::microseconds>(epoch);
            int64_t then = payload["time"].asInt64();
            running_avg.push_back(us.count() - then);
            if (running_avg.size() > 64) {
                running_avg.pop_front();
            }
            int64_t avg = 0;
            for (int64_t lat : running_avg) {
                avg += lat;
            }
            std::cout << "latency: " << avg / running_avg.size() << "\n";
            // std::cout << payload.toStyledString();
#endif
            // std::string new_msg_uuid = payload["aws_msg_uuid"].asString();
            // if (msg_uuid != new_msg_uuid) {
            //     msg_uuid = new_msg_uuid;
            //     cb(payload["data"]);
            // }
        }
    }
}

Core::Core()
{
    // TODO
}

Core::~Core()
{
    // TODO
}

Json::Value Core::get(const std::string& uuid_, int32_t timeout_)
{
    // TODO
}

    
} // end namespace iort

