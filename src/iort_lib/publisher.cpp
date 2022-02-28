#include <config.h>
#include <json/json.h>
#include <mqtt/async_client.h>
#include <mqtt/message.h>

#include <chrono>
#include <regex>
#include <sstream>
using namespace std::chrono_literals;

#include "ros/ros.h"
#include "std_msgs/Bool.h"
#include "std_msgs/Int64.h"
#include "std_msgs/String.h"

static const std::string FUNCTION_URL = HTTP_ENDPOINT;
static const std::string ENDPOINT_URL = "ssl://" MQTT_ENDPOINT
                                        ":8883";
static const std::string TOPIC = "device/" UUID
                                  "/data";

std::string convertToTopic(std::string name) {
    for (int i = 0; i < name.length(); i++) {
        if (name[i] == ' ')
            name[i] = '_';
    }
    return name;
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "publisher");
    ros::NodeHandle n;
    std::map<std::string, ros::Publisher> publishers;
    Json::Reader reader;
    mqtt::async_client cli(ENDPOINT_URL, "client_publisher");
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
    auto tok = cli.connect(connOpts);
    auto rsp = tok->get_connect_response();
    if (!rsp.is_session_present())
        cli.subscribe(TOPIC, 1)->wait();
    while (ros::ok()) {
        Json::Value payload;
        mqtt::const_message_ptr msg;
        if (!cli.try_consume_message_for(&msg, 10ms))
            continue;
        reader.parse(msg->to_string(), payload);
        Json::Value data = payload["data"];
        std::vector<std::string> members = data.getMemberNames();
        int size = 0;
        for (std::string member : members) {
            if (publishers.find(member) == publishers.end()) {
                if (data[member].isInt64()) {
                    publishers[member] = n.advertise<std_msgs::Int64>("mqtt/" + convertToTopic(member), 3);
                } else if (data[member].isString()) {
                    publishers[member] = n.advertise<std_msgs::String>("mqtt/" + convertToTopic(member), 3);
                } else if (data[member].isBool()) {
                    publishers[member] = n.advertise<std_msgs::Bool>("mqtt/" + convertToTopic(member), 3);
                } else {
                    continue;  // unrecognized data type, skip
                }
            }
            if (data[member].isInt64()) {
                std_msgs::Int64 msg;
                msg.data = data[member].asInt64();
                publishers[member].publish(msg);
            } else if (data[member].isString()) {
                std_msgs::String msg;
                std::stringstream ss;
                ss << data[member].asString();
                msg.data = ss.str();
                publishers[member].publish(msg);
            } else if (data[member].isBool()) {
                std_msgs::Bool msg;
                msg.data = data[member].asBool();
                publishers[member].publish(msg);
            } else {
                continue;  // unrecognized data type, skip
            }
            size++;
        }
        // if (size != publishers.size()) {
        //     publishers.clear();
        // }
    }
}