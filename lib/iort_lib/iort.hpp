/*

    iort.hpp - header file for the internet of robotic things compatibility
    layer for C++. This library wraps calls to the cloud API and provides a
    simplified interface to IoT endpoints.

    Dependencies:
        - https://github.com/whoshuu/cpr (thank god no libcurl)
        - https://github.com/open-source-parsers/jsoncpp

*/
#ifndef iort_HPP
#define iort_HPP

#include <json/json.h>
#include <thread>
#include <future>
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace iort
{
class Subscriber
{
private:
    std::string uuid;

    std::string msg_uuid;

    boost::function<void(Json::Value)> cb;

    int32_t timeout;

    int32_t timeout_count;

    int32_t max_timeout_count;

    std::promise<void>* exitCond;

    std::thread subThread;

    bool running;

    void run(std::future<void> exitSig);

public:
    Subscriber(const std::string& uuid_,
               const boost::function<void(Json::Value)>& cb_,
               const int32_t timeout_ = 1000,
               const int32_t timeout_count_ = 10);

    ~Subscriber();

    bool isRunning(void);

    bool start(void);

    bool stop(void);
};

class Core
{
private:
public:
    Core();

    ~Core();

    Json::Value get(const std::string& uuid_, int32_t timeout_ = 1000);

    Subscriber* subscribe(const std::string& uuid_, void (*cb_)(Json::Value),
                          const int32_t timeout_ = 1000,
                          const int32_t timeout_count_ = 10)
    {
        return new Subscriber(uuid_, boost::function<void(Json::Value)>(cb_),
                              timeout_);
    }

    template <class T>
    Subscriber* subscribe(const std::string& uuid_, void (T::*cb_)(Json::Value),
                          T* obj, const int32_t timeout_ = 1000,
                          const int32_t timeout_count_ = 10)
    {
        return new Subscriber(uuid_, boost::bind(cb_, obj, _1), timeout_);
    }
};

}    // end namespace iort

#endif    // end iort_HPP
