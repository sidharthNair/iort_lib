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
#include <queue>
#include <thread>
#include <future>
#include <functional>

namespace iort
{
typedef struct CallbackQueueItem
{
    std::function<void(Json::Value)> cb;
    Json::Value data;
} CallbackQueueItem;

typedef std::queue<CallbackQueueItem> CallbackQueue;

class Subscriber
{
private:
    std::string uuid;

    std::string msg_uuid;

    std::function<void(Json::Value)> cb;

    CallbackQueue& cb_queue;

    int32_t timeout;

    int32_t failure_count;

    int32_t max_failure_count;

    std::promise<void>* exitCond;

    std::thread subThread;

    bool running;

    void run(std::future<void> exitSig);

public:
    Subscriber(const std::string& uuid_,
               const std::function<void(Json::Value)>& cb_,
               CallbackQueue& cb_queue_, const int32_t timeout_ = 1000,
               const int32_t failure_count_ = 10);

    ~Subscriber();

    bool isRunning(void);

    bool start(void);

    bool stop(void);
};

class Core
{
private:
    CallbackQueue callbackQueue;

    std::promise<void>* exitCond;

    std::thread callbackThread;

    void run(std::future<void> exitSig);

public:
    Core();

    ~Core();

    bool get(const std::string& uuid, Json::Value& ret, int32_t timeout = 1000);

    bool query(const std::string& query_string, Json::Value& ret,
               int32_t timeout = 1000);

    Subscriber* subscribe(const std::string& uuid_, void (*cb_)(Json::Value),
                          const int32_t timeout_ = 1000,
                          const int32_t failure_count_ = 10)
    {
        return new Subscriber(uuid_, std::function<void(Json::Value)>(cb_),
                              callbackQueue, timeout_, failure_count_);
    }

    template <class T>
    Subscriber* subscribe(const std::string& uuid_, void (T::*cb_)(Json::Value),
                          T* obj, const int32_t timeout_ = 1000,
                          const int32_t failure_count_ = 10)
    {
        return new Subscriber(uuid_, std::bind(cb_, obj, std::placeholders::_1),
                              callbackQueue, timeout_, failure_count_);
    }
};

}    // end namespace iort

#endif    // end iort_HPP
