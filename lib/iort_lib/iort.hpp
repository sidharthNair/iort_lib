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
#include <chrono>

using namespace std::chrono_literals;

namespace iort {

class Subscriber
{
private:
    std::string uuid;
    void (* cb)(Json::Value);

public:
    template<typename TDuration>
    Subscriber(const std::string& uuid_, void (* cb_)(Json::Value &), 
               TDuration timeout);

    Subscriber(const std::string& uuid_, void (* cb_)(Json::Value &));

    ~Subscriber();

    bool isRunning(void);
    void start(void);
    void stop(void);
};

class Core
{
private:

public:
    Core();
    ~Core();

    template<typename TDuration>
    Json::Value & get(const std::string& uuid_, TDuration timeout);

    Json::Value & get(const std::string& uuid_);

    Subscriber subscribe(const std::string& uuid_, void (* cb_)(Json::Value &));

};

} // end namespace iort

#endif  // end iort_HPP