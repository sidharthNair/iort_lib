/*

    iort_interface.cpp

*/

#include <cpr/cpr.h>
#include "iort_lib/iort.hpp"

namespace iort
{

template<typename TDuration>
Subscriber::Subscriber(const std::string& uuid_, void (* cb_)(Json::Value &), 
            TDuration timeout)
{
    // TODO
}

Subscriber::Subscriber(const std::string& uuid_, void (* cb_)(Json::Value &))
{
    // TODO
}

Subscriber::~Subscriber()
{
    // TODO
}

bool Subscriber::isRunning(void)
{
    // TODO
}

void Subscriber::start(void)
{
    // TODO
}

void Subscriber::stop(void)
{
    // TODO
}

Core::Core()
{
    // TODO
}

Core::~Core()
{
    // TODO
}

template<typename TDuration>
Json::Value & Core::get(const std::string& uuid_, TDuration timeout)
{
    // TODO
}

Json::Value & Core::get(const std::string& uuid_)
{
    // TODO
}

Subscriber Core::subscribe(const std::string& uuid_, 
                           void (* cb_)(Json::Value &))
{
    // TODO
}

    
} // end namespace iort

