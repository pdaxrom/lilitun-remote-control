const CLIENT_ID = "Remote client application 1.0   ";
const PASSWORD = "helloworld";

const Request = {
    REQ_ERROR : 0,
    REQ_SCREEN_INFO : 1,
    REQ_SCREEN_UPDATE : 2,
    REQ_KEYBOARD : 3,
    REQ_POINTER : 4,
    REQ_USER_CONNECTION : 5,
    REQ_USER_PASSWORD : 6,
    REQ_APPSERVER_HOST : 7,
    REQ_SESSION_ID : 8,
    REQ_HOSTNAME : 9,
    REQ_AUTHORIZATION : 10,
    REQ_STOP : 11,
    REQ_SIGNATURE : 128
};

const Pix = {
    PIX_RAW_RGBA : 0,
    PIX_RAW_BGRA : 1,
    PIX_JPEG_RGBA : 2,
    PIX_JPEG_BGRA : 3,
    PIX_LZ4_RGBA : 4,
    PIX_LZ4_BGRA : 5
};

const State = {
    ReadUint32 : 0,
    ReadData : 1,
    ReadAck : 2,
    ReadRegionHeader : 3,
    ReadRegionData : 4
};

let ScreenInfo = {
    width : 0,
    height : 0,
    depth : 0,
    pixelformat : 0
};

let RegionHeader = {
    x : 0,
    y : 0,
    width : 0,
    height : 0,
    depth : 0,
    size : 0
};

let RemoteHost = "Unknown";

function htonl(n)
{
    // Mask off 8 bytes at a time then shift them into place
    return new Uint8Array([
        (n & 0xFF000000) >>> 24,
        (n & 0x00FF0000) >>> 16,
        (n & 0x0000FF00) >>>  8,
        (n & 0x000000FF) >>>  0,
    ]);
}

function ntohl(n)
{
    return (n[0] << 24) | (n[1] << 16) | (n[2] << 8) | n[3];
}

function array_to_string(array)
{
    return String.fromCharCode.apply(String, new Uint8Array(array));
}

function strlen(string)
{
    return string.length;
}

function array_append(buffer1, buffer2) {
  let tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
  tmp.set(new Uint8Array(buffer1), 0);
  tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
  return tmp;
};

function send_string(socket, string)
{
  socket.send(htonl(strlen(string)));
  socket.send(new Blob([string]));
}

function send_password(socket, password)
{
  let data = array_append(htonl(Request.REQ_AUTHORIZATION), htonl(strlen(PASSWORD)));
  state = State.ReadAck;
  socket.send(data);
  socket.send(new Blob([password]));
}

function send_request(socket, request)
{
    socket.send(htonl(request));
}

let state = 0;
let request = 0;
let data_len = 0;
let regions = 0;

let canvas;
let ctx;

let socket = new WebSocket("wss://localhost:4443/projector-ws", ['binary']);

console.log(socket.binaryType);

socket.onopen = function(e) {
  console.log("[open] Connection established");
  console.log("Sending to server");
  state = State.ReadUint32;
  request = Request.REQ_SIGNATURE;
  send_string(socket, CLIENT_ID);
};

socket.onmessage = function(event) {
//  console.log(`[message] Data received from server: ${event.data}`);

  event.data.arrayBuffer().then(buffer => {
//    console.log('Buffer length ' + buffer.byteLength);

    if (request == Request.REQ_SIGNATURE) {
	switch (state) {
	case State.ReadUint32:
	    data_len = ntohl(new Uint8Array(buffer));
	    console.log('Data len ' + data_len);
	    state = State.ReadData;
	    break;
	case State.ReadData:
	    if (buffer.byteLength != data_len) {
		console.log("Wrong data length!");
	    } else {
		signature = array_to_string(buffer);
		console.log("Signature " + signature);

		request = Request.REQ_AUTHORIZATION;
		state = State.ReadAck;
		send_password(socket, PASSWORD);
	    }
	    break;
	}
    } else if (request == Request.REQ_AUTHORIZATION) {
	if (state == State.ReadAck) {
	    status = ntohl(new Uint8Array(buffer));
	    if (status != 0) {
		console.log("Wrong password!");
	    }

	    request = Request.REQ_SCREEN_INFO;
	    state = State.ReadAck;
	    send_request(socket, request);
	}
    } else if (request == Request.REQ_SCREEN_INFO) {
	if (state == State.ReadAck) {
	    tmp = new Uint8Array(buffer);
	    reply = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
	    ScreenInfo.width = ntohl([tmp[4], tmp[5], tmp[6], tmp[7]]);
	    ScreenInfo.height = ntohl([tmp[8], tmp[9], tmp[10], tmp[11]]);
	    ScreenInfo.depth = ntohl([tmp[12], tmp[13], tmp[14], tmp[15]]);
	    ScreenInfo.pixelformat = ntohl([tmp[16], tmp[17], tmp[18], tmp[19]]);
	    console.log("Reply " + reply);
	    console.log("width = " + ScreenInfo.width);
	    console.log("height = " + ScreenInfo.height);
	    console.log("depth = " + ScreenInfo.depth);
	    console.log("pixelformat = " + ScreenInfo.pixelformat);

	    const pixelRatio = window.devicePixelRatio || 1;
	    console.log("Pixel ratio " + pixelRatio);
	    let w = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;
	    let h = window.innerHeight || document.documentElement.clientHeight || document.body.clientHeight;
	    canvas = document.getElementById('canvas');
	    canvas.width = ScreenInfo.width;
	    canvas.height = ScreenInfo.height;

	    canvas.style.width = `${w}px`;
	    canvas.style.height = `${h}px`;
	    ctx = canvas.getContext('2d');
	    ctx.mozImageSmoothingEnabled = false;
	    ctx.imageSmoothingEnabled = false;

	    request = Request.REQ_HOSTNAME;
	    state = State.ReadAck;
	    send_request(socket, request);
	}
    } else if (request == Request.REQ_HOSTNAME) {
	if (state == State.ReadAck) {
	    tmp = new Uint8Array(buffer);
	    reply = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
	    console.log("Reply " + reply);
	    data_len = ntohl(tmp[4], tmp[5], tmp[6], tmp[7]);
	    state = State.ReadData;
	} else if (state == State.ReadData) {
	    RemoteHost = array_to_string(buffer);
	    console.log("Remote host " + RemoteHost);
	    document.title = RemoteHost;

	    request = Request.REQ_SCREEN_UPDATE;
	    state = State.ReadAck;
	    send_request(socket, request);
	}
    } else if (request == Request.REQ_SCREEN_UPDATE) {
	if (state == State.ReadAck) {
	    tmp = new Uint8Array(buffer);
	    reply = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
//	    console.log("Reply " + reply);
	    regions = ntohl([tmp[4], tmp[5], tmp[6], tmp[7]]);
	    console.log("Regions " + regions);
	    state = State.ReadRegionHeader;
	} else if (state == State.ReadRegionHeader) {
	    regions--;
	    tmp = new Uint8Array(buffer);
	    RegionHeader.x = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
	    RegionHeader.y = ntohl([tmp[4], tmp[5], tmp[6], tmp[7]]);
	    RegionHeader.width = ntohl([tmp[8], tmp[9], tmp[10], tmp[11]]);
	    RegionHeader.height = ntohl([tmp[12], tmp[13], tmp[14], tmp[15]]);
	    RegionHeader.depth = ntohl([tmp[16], tmp[17], tmp[18], tmp[19]]);
	    RegionHeader.size = ntohl([tmp[20], tmp[21], tmp[22], tmp[23]]);
//	    console.log("x = " + RegionHeader.x);
//	    console.log("y = " + RegionHeader.y);
//	    console.log("width = " + RegionHeader.width);
//	    console.log("height = " + RegionHeader.height);
//	    console.log("depth = " + RegionHeader.depth);
//	    console.log("size = " + RegionHeader.size);
	    if (RegionHeader.size > 0) {
		state = State.ReadRegionData;
	    } else if (regions == 0) {
//		console.log("no regions");
		request = Request.REQ_SCREEN_UPDATE;
		state = State.ReadAck;
//		send_request(socket, request);
		setTimeout(() => send_request(socket, request), 50);
	    }
	} else if (state == State.ReadRegionData) {
	    if (RegionHeader.depth == Pix.PIX_JPEG_RGBA || RegionHeader.depth == Pix.PIX_JPEG_BGRA) {
		tmp = new Uint8Array(buffer);
		const blob = new Blob([tmp], {type: 'image/jpeg'});
		const image = new Image();
		image.src = URL.createObjectURL(blob);
		    const x = RegionHeader.x;
		    const y = RegionHeader.y;
		    const w = RegionHeader.width;
		    const h = RegionHeader.height;
		image.onload = function() {
//		    console.log("draw image on " + x + ", " + y + " - " + w + ", " + h);
		    ctx.drawImage(this, x, y, w, h);
		}
	    } else {
		console.log("Unsupported pixels yet " + RegionHeader.depth);
	    }
	    if (regions > 0) {
		state = State.ReadRegionHeader;
	    } else {
//		console.log("no regions");
		request = Request.REQ_SCREEN_UPDATE;
		state = State.ReadAck;
//		send_request(socket, request);
		setTimeout(() => send_request(socket, request), 50);
	    }
	}
    }
  });
//  var text = event.data.text().then (text => {
//    console.log(`[message] Data received from server: ${text}`);
//  });
};

socket.onclose = function(event) {
  if (event.wasClean) {
    console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
  } else {
    // e.g. server process killed or network down
    // event.code is usually 1006 in this case
    console.log('[close] Connection died');
  }
};

socket.onerror = function(error) {
  console.log(`[error] ${error.message}`);
};
