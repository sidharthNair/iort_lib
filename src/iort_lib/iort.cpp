/*

    iort_interface.cpp

*/

#include <mosquitto.h>
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
static const std::string ENDPOINT_URL = MQTT_ENDPOINT;

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

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc) {
        exit(-1);
    }
    mosquitto_subscribe(mosq, NULL, ((Subscriber *)obj)->getTopic().c_str(), 0);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    ((Subscriber *)obj)->messageCallback((char *)msg->payload);
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
    int rc, id = 1;
    mosquitto_lib_init();
    mosq = mosquitto_new("subscriber", true, this);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_tls_set(mosq, "build/iort_lib/aws-root-ca.pem", NULL, "build/iort_lib/certificate.pem.crt", "build/iort_lib/private.pem.key", NULL);
    rc = mosquitto_connect(mosq, ENDPOINT_URL.c_str(), 8883, 10);
#ifdef DEBUG
    if (rc) {
    	  printf("Could not connect to broker: %d\n", rc);
    }
#endif
    mosquitto_loop_start(mosq);
    return true;
}

bool Subscriber::stop(void)
{
    if (!running) return false;
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    running = false;
    return true;
}

void Subscriber::messageCallback(char *message) {
    Json::Value payload;
    reader.parse((char *) message, payload);
    cb_queue.push({cb, payload["data"] });
#ifdef DEBUG
    std::cout << "New message with topic " << msg->topic << " " << (char *) msg->payload << std::endl;
    const auto epoch =
	    std::chrono::system_clock::now().time_since_epoch();
    const auto us =
	    std::chrono::duration_cast<std::chrono::microseconds>(epoch);
    int64_t then = payload["time"].asInt64();
    std::cout << "latency: " << us.count() - then << "\n";
#endif
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
    while (true)
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
