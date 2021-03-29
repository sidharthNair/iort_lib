/*

    iort_interface.cpp

*/

#include <cpr/cpr.h>
#include "iort_lib/iort.hpp"

#include <chrono>
using namespace std::chrono_literals;

namespace iort
{

Subscriber::Subscriber(const std::string& uuid_, void (* cb_)(Json::Value), 
                       int32_t timeout_)
{
    // TODO
    uuid = uuid_;
    cb = cb_;
    timeout = timeout_;

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
}

bool Subscriber::stop(void)
{
    if (!running) return false;
    exitCond->set_value();
    subThread.join();
}

void Subscriber::run (std::future<void> exitSig)
{
    while (exitSig.wait_for(1ms) == std::future_status::timeout) {
        cpr::Response r = cpr::Post(
            cpr::Url{"https://todo.org"},
            cpr::Payload{{"key", "value"}},
            cpr::Timeout(timeout)
        );

        if (r.elapsed * 1000 + 1 - timeout > 0) break;

        Json::Value data;
        // TODO process to Json::Value
        cb(data);
    }
    running = false;
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

Subscriber Core::subscribe(const std::string& uuid_, 
                           void (* cb_)(Json::Value))
{
    // TODO
}

    
} // end namespace iort

