# **ntmd API Documentation (WIP)**

To interact with ntmd as it's running on your computer, you should use the api documented here. 

The api commands not only have access to the local sqlite database, but also in-memory information that has yet to be deposited to disk. This will eventually also include process control flow management.

Although the monitored traffic is stored on a local sqlite database, it is recommended to use the API to retrieve historical traffic data from the database using the commands listed here. If there is functionality that is missing that you believe would be useful, suggest its creation! 

## Usage

The API is hosted via a socket server on the default port 13889, but this can be changed in the config if necessary. All commands return JSON, with one exception being `live-text` which simply returns formatted text.

To send a request to the socket server, simply open a socket and send a string with the name of a command. If said command requires parameters, send them after the command name separated by a space.

Returned JSON payload from api requests contain a `data` field with the contextual data returned by the specific command,  a `length` field which lets you know how many objects are in the `data` field, a `result` field which will let you know if the command was successful or failed, and an `errmsg` field which is only present when an error occurred and contains contextual information as to why the error occurred. 

## Commands (WIP)

**`live-text`** -> A continuous stream of pre-formatted strings on a set interval that gives pretty information about in-memory monitored traffic.

**`live`** -> A continuous stream of JSON data on a set 
interval that gives information about in-memory monitored traffic.

Example payload:
```
{
    "data": {
        "Discord": {
            "bytesRx": 123
            "bytesTx": 321
            "pktRxCount": 5,
            "pktTxCount": 5
        },
        "chromium": {
            "bytesRx": 777
            "bytesTx": 777
            "pktRxCount": 11
            "pktTxCount": 11
        },
    },
    "length": 2
    "result": "success"
}
```
