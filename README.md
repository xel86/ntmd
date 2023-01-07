# ntmd (WIP)
Network Traffic Monitor Daemon for linux.  
This project is early in development, and although core functionality is working well it still has it's fair share of issues (known & unknown).

## Known Issues
- Can't find process/sockets for qemu virtual machines 
- Can't find process/sockets for mullvad vpn traffic when turned on (and presumably other vpns).

## Installation

ntmd uses a few dependencies that are not bundled with the repository, so you must install them yourself before building.   

These include:
- sqlite3
- nlohmann-json
- libpcap
- cmake
- clang

To install all required dependencies on ubuntu:  
`sudo apt-get install libsqlite3-dev nlohmann-json3-dev libpcap-dev clang cmake`

To install all required dependencies on arch:  
`sudo pacman -S sqlite nlohmann-json libpcap clang cmake`

### Packages
TBD

### Manual Build
After installing all dependencies, clone the repository:  

    git clone https://github.com/xel86/ntmd

Once in the `ntmd/` directory make a build directory and cd into it:  

    mkdir build && cd build

Compile the project with cmake: 

    cmake .. && make

Move ntmd into `/usr/bin`:  

    sudo mv ntmd /usr/bin

If you wish to launch with a custom config edit the default config provided in `docs/ntmd.conf` to your liking and then move it into the default location `/etc/ntmd.conf` or specify its location with the `-c/--config` argument.  

    sudo cp ../docs/ntmd /etc/

If you wish to setup a systemd service to manage ntmd:  

    sudo cp ../systemd/ntmd.service /etc/systemd/system/

If you wish for ntmd to start up with your computer, enable the systemd service:  

    sudo systemctl enable ntmd

To start ntmd immediately:

    sudo systemctl start ntmd

## Usage
Once ntmd is running in the background it will begin monitoring network traffic and matching packets with the processes they belong too. To retrieve that information you should use the socket api that ntmd hosts to allow external applications to view live in-memory network traffic or historical network traffic stored in the database.

Full API documentation of all commands and further information can be found in [docs/api.md](docs/api.md)

Quick examples using netcat to send commands to socket server and jq to pretty print the returned json:  

View current in-memory monitored traffic before database deposit:
`echo 'snapshot' | nc localhost 13889 | jq`  

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

View historical traffic since January 1st 2023:
`echo 'traffic-since 1672549200 | nc localhost 13889 | jq`  

```
{
  "data": {
    "chatterino": {
      "bytesRx": 114048694,
      "bytesTx": 13902477,
      "pktRxCount": 196166,
      "pktTxCount": 193273
    },
    "chromium": {
      "bytesRx": 5156785412,
      "bytesTx": 131679244,
      "pktRxCount": 3590623,
      "pktTxCount": 1697315
    },
    "git-remote-http": {
      "bytesRx": 9096,
      "bytesTx": 4568,
      "pktRxCount": 25,
      "pktTxCount": 32
    },
    "ssh": {
      "bytesRx": 1852839460,
      "bytesTx": 11996499,
      "pktRxCount": 198432,
      "pktTxCount": 138233
    }
  },
  "length": 4,
  "result": "success"
}
```

Successor to previous project https://github.com/xel86/omnis
