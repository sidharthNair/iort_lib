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

namespace iort {

class Subscriber
{
private:
    std::string uuid;

    void (* cb)(Json::Value);

    int32_t timeout;

    std::promise<void> * exitCond;

    std::thread subThread;

    bool running;

    void run(std::future<void> exitSig);

public:
    Subscriber(const std::string& uuid_, void (* cb_)(Json::Value), 
               int32_t timeout_ = 1000);

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

    Subscriber subscribe(const std::string& uuid_, void (* cb_)(Json::Value));

};

} // end namespace iort

#endif  // end iort_HPP
