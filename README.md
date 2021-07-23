
# IoRT Lib

`iort_lib` is the ROS component of the IoT-Robotics Integration project. Other major components of the project are available at the links below:

1. [`iort_endpoint`](https://github.com/PaperFanz/iort_endpoint) - open source firmware for an ESP32 microcontroller to serve as an AWS IoT Core thing
2. [`insitu`](https://github.com/PaperFanz/insitu) - open source situational awareness package with extension support
3. [`iort_lib`](https://github.com/PaperFanz/iort_lib) - this repository
4. [`iort_filters`](https://github.com/PaperFanz/iort_filters) - example `insitu` extensions using `iort_lib` to construct real time data overlays

You should go through these four links in order and follow the setup isntructions in `README.md`.

## Configuring AWS

The IoT-Robotics Integration project uses AWS to ingest, store, and serve IoT data to robotic clients. The following sections will guide you through the process of creating a database, adding an ingest rule, and setting up an web API gateway. Before doing anything on the AWS web console, **MAKE SURE YOU ARE ON THE SAME REGION AS WHEN YOU SET UP THE ENDPOINT**. You can easily switch regions using the drop-down menu in the top right corner of the page.

### Setting up a Timestream Database

Search for `Timestream` on the AWS console and navigate to `Databases` in the sidebar and hit `Create database`. Select `Standard database`, pick a name (e.g. `iort`), and confirm. Then navigate to `Tables` in the sidebar and hit `Create table`. Select the database you just created, pick a table name (e.g. `data`), choose 1 day for the memory store retention and 30 days for the magnetic store retention, then confirm. Don't worry about the table schema as the columns will be automatically populated by the IoT Core Rule that we are about to set up.

### Setting up IoT Core to Ingest IoT Data

Search for `IoT Core` on the AWS console and navigate to `Act>Rules` in the sidebar and hit `Create`. Give your new rule a name and a description, and paste the following into the `Rule query statement`:

```sql
SELECT
  data
FROM
  'device/+/data'
```

Make sure that it is using SQL version 2016-03-23. Then, under `Set one or more actions`, hit `Add action` and select `Write message into a Timestream table`. On the `Configure action` screen, choose the database and table you just created in the previous section, and copy the following dimensions:

| Dimension Name | Dimension Value |
| -------------- | --------------- |
| dev_uuid | ${topic(2)} |
| dev_time | ${time} |
| msg_uuid | ${traceid()} |
| latency | ${timestamp() * 1000 - time} |

For the `Timestamp` section, copy the following:

| Value | Unit |
| ----- | ---- |
| ${timestamp()} | MILLISECONDS |

Finally, create a new role and give it a meaningful name like `timestream-writer`, then confirm the action. Finally, hit `Create rule`. 

### Testing

If you have correctly set up everything up to this point, you should be able to navigate to `Timestream>Query Editor` and use the following SQL query to look at the ingested data:

```sql
SELECT * FROM "iort"."data" WHERE time between ago(15m) and now() ORDER BY time DESC LIMIT 10 
```

#### Troubleshooting

```c
// TODO
```

### Setting up AWS Lambda and Gateway

First, we must create a role to give our Lambda the permission to read from our Timestream database. Go to the [IAM Console](https://console.aws.amazon.com/iam/home?#/roles$new?step=type), pick the `Lambda` use case, and hit `Next`. Under `Attach permissions policies`, search for `timestream` and check `AmazonTimestreamFullAccess`, then hit `Next`. Add tags (optional), then give your new role a meaningful name and description (e.g. `timestream-reader`, `role for timestream access permissions`).

Then navigate to `Lmabda>Functions` and hit `Create function`. Choose `Author from scratch` and give your function a meaningful name (e.g. `iort_lib_query`), and pick the `Node.js 14.x` runtime. Then click `Change default execution role`, select `Use an existing role`, and pick the role you just created from the drop-down menu. If it is not there, wait a few minutes and try again; AWS takes some time to update every now and then. Confirm by clicking `Create function` and wait for AWS to generate the function stub. 

#### Lambda Code

Copy the following into `index.js` under `Code Source`:

```js
const AWS = require('aws-sdk');
const timestreamquery = new AWS.TimestreamQuery({ region: 'us-west-2' });

function parseRes(res, idx) {
    var qmsg = res.Rows[idx].Data;
    var ret = {};
    
    // indexes into qmsg are left-to-right from timestream queries
    ret['time'] = parseInt(qmsg[0].ScalarValue);
    ret['msgid'] = qmsg[1].ScalarValue;
    ret['data'] = JSON.parse(qmsg[2].ScalarValue);
    
    return ret;
}

exports.handler = async (event) => {
    try {
        const body = event.queryStringParameters;
        const roboParams = event.queryStringParameters;
        
        var params = {};
        
        switch (roboParams.op) {
            case 'query':
                params.QueryString = roboParams.qs;
                break;
            
            case 'get':
            default:
                params.QueryString = 
                    "SELECT dev_time, msg_uuid, measure_value::varchar as data " +
                    "FROM \"iort\".\"data\" " + 
                    "WHERE dev\_uuid = \'"+roboParams.uuid+"\'" + 
                    "ORDER BY dev_time DESC LIMIT 1";
        }
        
        let response = {
            statusCode: 200,
            body: 'null'
        };
        
        try {
            const res = await timestreamquery.query(params).promise();
            if(res.Rows.length == 1 ){
                response = {
                    statusCode: 200,
                    body: JSON.stringify(parseRes(res, 0))
                };
            }
            else if (res.Rows.length > 1)
            {
                let ret = [];
                for (let i = 0; i < res.Rows.length; ++i) {
                    ret[i] = parseRes(res, i);
                }
                response = {
                    statusCode: 200,
                    body: JSON.stringify(ret)
                };
            }
            else
            {
                response = {
                    statusCode: 404,
                    body: 'ERROR: Data for queried sensors not found'
                };
            }
            
            
        } catch (e) {
            response = {
                statusCode: 500,
                body: 'ERROR: ' + e
            };
        }
        
        return response;
    } catch (e) {
        let response = {
            statusCode: 500,
            body: 'ERROR: ' + e
        };
        return response;
    }
};
```

**MAKE SURE THAT THE REGION, DATABASE NAME, AND TABLE NAME MATCH THE ONES YOU JUST CREATED** in the following code sections:

```js
const timestreamquery = new AWS.TimestreamQuery({ region: 'us-west-2' });
```

```js
                params.QueryString = 
                    "SELECT dev_time, msg_uuid, measure_value::varchar as data " +
                    "FROM \"iort\".\"data\" " + 
                    "WHERE dev\_uuid = \'"+roboParams.uuid+"\'" + 
                    "ORDER BY dev_time DESC LIMIT 1";
```

#### Gateway Setup

Click `Function Overview>Add Trigger` and select `API Gateway`. Choose `Create an API` from the dropdown menu, then select `HTTP API` for `API type` and `Open` for `Security`. Confirm by hitting `Add`. Now go back to your lambda function and navigate to `Configuration>Triggers`. Expand the API Gateway details and copy the `API endpoint` URL. Copy this URL into `inc/config.h` on the following declaration:

```cpp
#define HTTP_ENDPOINT "copy url here"
```

## Configuring the project for MQTT subscription

For this section you will need your Thing's Rest API Endpoint URL ([AWS IoT Core page](https://us-west-2.console.aws.amazon.com/iot/home?region=us-west-2#/dashboard) and navigate to `Manage>Things>myEndpoint>Interact`) and certificates that you created when setting up [`iort_endpoint`](https://github.com/PaperFanz/iort_endpoint). 

#### Endpoint URL

In `inc/config.h` copy your Rest API Endpoint URL on the following declaration:

```cpp
#define MQTT_ENDPOINT "copy url here"
```

#### AWS Certificates

Copy the certificate into `iort_lib/certs/certificate.pem.crt`.

Copy the private key into `iort_lib/certs/private.pem.key`.

Copy the root CA into `iort_lib/certs/aws-root-ca.pem`.

## Cloning and Building

This project depends on [`Eclipse Paho MQTT C++ Client Library`](https://github.com/eclipse/paho.mqtt.cpp) which is used for MQTT subscription. Follow the instructions to [build and install the Paho C Library](https://github.com/eclipse/paho.mqtt.cpp#building-the-paho-c-library) and [build and install the Paho C++ library](https://github.com/eclipse/paho.mqtt.cpp#building-the-paho-c-library-1)

The project also depends on [`Curl for People`](https://github.com/whoshuu/cpr) which allows us to programmatically query the Timestream database, but this is included as a submodule in this repository.

Clone the repository into your catkin workspace and run `catkin build`:

```sh
cd ~/catkin_ws/src
git clone --recursive https://github.com/PaperFanz/iort_lib
catkin build iort_lib
```

You may also choose to install the library to your system folders by running

```sh
sudo catkin install iort_lib
```
