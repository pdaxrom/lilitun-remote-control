## LiliTun remote control system

The system is designed for remote desktop control via a web. Supported operating systems - Linux, MacOS, Windows. The client works in a web browser and can be used on both desktop and mobile devices.

The system consists of three components - an application for sharing the desktop, a control server for managing connections, and an application server for managing users and user sessions.
<p align="center">
<img src="remote-desktop-system.svg" width="70%">
</p>
The source code implements a demo version of the application server, included in the docker image of the control server, which can be immediately launched on the VPS and used without any modifications - you only need to add an SSL certificate (e.g. from Let's Encrypt or self-signed).
Connections are made via websocket 443 and can be hidden behind the CDN (tested with cloudflare cdn).

The remote control application uses its own desktop sharing protocol and web client.
The control server uses [LibVNC/libvncserver](https://github.com/LibVNC/libvncserver) and [noVNC](https://novnc.com/info.html) for remote desktop sharing (soon to be replaced with the native protocol).

## LiliTun remote control application

The application can be launched by a link like ```lilink://``` or from the command line using the link to configure the application or set the settings manually.

### LiliTun remote control configuration

base64 encoded json data:
```
  {
    "requestType"      : "remoteControl",
    "appServerUrl"     : "https://remote.lilitun.link/desktop.php",
    "controlServerUrl" : "wss://remote.lilitun.link/projector-ws",
    "sessionId"        : "ba519a71-b4ae-47ee-b959-49769a009299"
  }
```

appServerUrl points to a webscript that accepts http(s) post requests from the remote control server.

`appServerUrl` is the url of the application server

`controlServerUrl` is the url of the control server

`sessionId` is session id

#### Example of remote control parameter:
```
  lilink://eyJhdXRoSGVhZGVyIjoiYXV0aEhlYWRlciB2YWx1ZSIsInJlcXVlc3RUeXBlIjoicmV
  tb3RlQ29udHJvbCIsImFwcFNlcnZlclVybCI6InJlbW90ZS5saWxpdHVuLmxpbmsvZGVza3RvcC5
  waHAiLCJjb250cm9sU2VydmVyVXJsIjoicmVtb3RlLmxpbGl0dW4ubGluayIsInNlc3Npb25JZCI
  6ImJhNTE5YTcxLWI0YWUtNDdlZS1iOTU5LTQ5NzY5YTAwOTI5OSJ9
```

## Control server

### Control server messages

The Control server sends POST requests to the Application Server with the action type `message`:
```
  room.lilitun.link/desktop.php?action=message
```

RequestType is `remoteControlStart` when remote connection started:

POST data is json data:
```
  {
    "requestType" : "remoteControlStart",
    "sessionId"   : "sessionId",
    "hostname"    : "hostname",
    "port"        : "port",
    "ipv6port"    : "ipv6port",
    "password"    : "password"   
  }
```

RequestType is `remoteConnectionStop` when connection finished:
```
  {
    "requestType" : "remoteControlStop",
    "sessionId"   : "sessionId",
  }
```

The Control server accepts POST requests with json data:
```
  {
    "requestType" : "stopSharing",
    "sessionId"   : "sessionId",
  }
```

RequestType `stopSharing` will force disconnect the remote connection with requested sessionId.

## User connections

To share a remote connection with remote users, the Application server must prepare links for viewing, for example:
```
https://<appServerUrl>/novnc/vnc.html?autoconnect=true&host=<controlServerUrl>&port=443&resize=scale&encrypt=1&password=<password received from control server>&path=/websockify?token=<port received from control server>
```

## Build demo

Build demo with docker:
```
  ./00_docker_build
```

Start demo in docker:
```
  ./01_docker_server
```

Connect to the demo host with link:
```
  https://<demo host>/desktop.php
```

For Linux and Windows, after downloading the application, run it to register it in the system. Once launched, the application will close and will be ready to launch from the link.

## Manual compilation with Ubuntu

Install Ubuntu packages:
```
  sudo apt install git ccache pkg-config m4 build-essential cmake zlib1g-dev libpng-dev libjpeg-dev \
    liblzo2-dev libssl-dev automake autoconf libtool chrpath libx11-dev libxft-dev libgl1-mesa-dev \
    libglu1-mesa-dev libxinerama-dev libxcursor-dev libxcb1-dev libxcb-shm0-dev libxcb-xtest0-dev \
    libxcb-keysyms1-dev imagemagick debhelper mingw-w64 nasm
```

Build the sources
```
  ./build.sh
```

Build the sources for Windows:
```
  ./build.sh Windows
```

Build the sources for macOS (install osx crosstools required):
```
  ./build.sh MacOS
```

Server package installation:
```
  dpkg -i release/ubuntu/controlserver_1.0_amd64.deb
```

Enable the service:
```
  systemctl enable controlserver
```

Start the service:
```
  systemctl start controlserver
```

To reconfigure the service, stop it and:
```
  dpkg-reconfigure controlserver
```

Then start it again.

## Misc

Sources formatting:
```
  indent -linux -l 120 -i 4 *.c *.h
```
