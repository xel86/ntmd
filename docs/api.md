# **ntmd API Documentation (WIP)**

To interact with ntmd as it's running on your computer, you should use the api documented here. 

The api commands not only have access to the local sqlite database, but also in-memory information that has yet to be deposited to disk. This will eventually also include process control flow management.

Although the monitored traffic is stored on a local sqlite database, it is recommended to use the API to retrieve historical traffic data from the database using the commands listed here. If there is functionality that is missing that you believe would be useful, suggest its creation! 

## Usage

The API is hosted via a socket server on the default port 13889, but this can be changed in the config if necessary. All commands return JSON, with one exception being `live-text` which simply returns formatted text.

To send a request to the socket server, simply open a socket and send a string with the name of a command. If said command requires parameters, send them after the command name separated by a space.

Returned JSON payload from api requests contain a `data` field with the contextual data returned by the specific command,  a `length` field which lets you know how many objects are in the `data` field, a `result` field which will let you know if the command was successful or failed, and an `errmsg` field which is only present when an error occurred and contains contextual information as to why the error occurred. 

## Commands (WIP)

### In-Memory Traffic 

**`live-text`** -> A continuous stream of pre-formatted strings on a set interval that gives pretty information about in-memory monitored traffic.

Example payload:
```
Application Traffic:
  chatterino { rx: 6.3 KB/s, tx: 867 B/s, rxc: 126, txc: 129 }
  Discord { rx: 223 B/s, tx: 38 B/s, rxc: 4, txc: 5 }
  chromium { rx: 449 B/s, tx: 289 B/s, rxc: 30, txc: 20 }
```

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
    "interval": 10
    "result": "success"
}
```

**`snapshot`** -> Provides a snapshot of the current buffered in-memory monitored traffic, unlike `live` this is not in sync with the database deposit interval, so a poorly timed call can result in an empty result if called right after a deposit.

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

### Historical database traffic

**`traffic-daily`** -> Provides all traffic accumulated since 12:00AM (0:00) on the current day.

Example payload:
```
{
    "data": {
        "Discord": {
            "bytesRx": 12309123
            "bytesTx": 3219213
            "pktRxCount": 12930,
            "pktTxCount": 92135
        },
        "chromium": {
            "bytesRx": 7777777
            "bytesTx": 7777777
            "pktRxCount": 111111
            "pktTxCount": 111111
        },
    },
    "length": 2
    "result": "success"
}
```

**`traffic-since <timestamp>`** -> Provides all traffic accumulated since the given timestamp, inclusive. The argument timestamp must be present with the request and must fit into a `time_t` variable on your system (Either 32 bit or 64 bit integer). 
A value of 0 can be given to retrieve all traffic. 
Example request sent to socket: `traffic-since 1672549200`

Example payload:
```
{
    "data": {
        "Discord": {
            "bytesRx": 12309123
            "bytesTx": 3219213
            "pktRxCount": 12930,
            "pktTxCount": 92135
        },
        "chromium": {
            "bytesRx": 7777777
            "bytesTx": 7777777
            "pktRxCount": 111111
            "pktTxCount": 111111
        },
    },
    "length": 2
    "result": "success"
}
```

**`traffic-between <start-timestamp> <end-timestamp>`** -> Provides all traffic accumulated between the two timestamps. The start timestamp value must be less than or equal to the end timestamp (i.e. before and after). Both timestamp arguments must be present with the request and must fit into a `time_t` variable on your system (Either 32 bit or 64 bit integer).
Example request sent to socket: `traffic-between 1672549200 1672635600`

Example payload:
```
{
    "data": {
        "Discord": {
            "bytesRx": 12309123
            "bytesTx": 3219213
            "pktRxCount": 12930,
            "pktTxCount": 92135
        },
        "chromium": {
            "bytesRx": 7777777
            "bytesTx": 7777777
            "pktRxCount": 111111
            "pktTxCount": 111111
        },
    },
    "length": 2
    "result": "success"
}
```