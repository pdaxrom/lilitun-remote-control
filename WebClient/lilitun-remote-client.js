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

function htonl(n) {
    // Mask off 8 bytes at a time then shift them into place
    return new Uint8Array([
        (n & 0xFF000000) >>> 24,
        (n & 0x00FF0000) >>> 16,
        (n & 0x0000FF00) >>>  8,
        (n & 0x000000FF) >>>  0,
    ]);
}

function ntohl(n) {
    return (n[0] << 24) | (n[1] << 16) | (n[2] << 8) | n[3];
}

function array_to_string(array) {
    return String.fromCharCode.apply(String, new Uint8Array(array));
}

function strlen(string) {
    return string.length;
}

function array_append(buffer1, buffer2) {
    let tmp = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
    tmp.set(new Uint8Array(buffer1), 0);
    tmp.set(new Uint8Array(buffer2), buffer1.byteLength);
    return tmp;
};

function send_string(socket, string) {
    socket.send(htonl(strlen(string)));
    socket.send(new Blob([string]));
}

function send_password(socket, password) {
    let data = array_append(htonl(Request.REQ_AUTHORIZATION), htonl(strlen(PASSWORD)));
    state = State.ReadAck;
    socket.send(data);
    socket.send(new Blob([password]));
}

function send_request(socket, request) {
    socket.send(htonl(request));
}

function mouse_event(buttons, x, y, wheel) {
    this.buttons = buttons;
    this.x = x;
    this.y = y;
    this.wheel = wheel;
}

function send_mouse_event(socket, event) {
    let tmp = new Uint8Array(20);
    tmp.set(htonl(Request.REQ_POINTER), 0);
    tmp.set(htonl(event.buttons), 4);
    tmp.set(htonl(event.x), 8);
    tmp.set(htonl(event.y), 12);
    tmp.set(htonl(event.wheel), 16);
    socket.send(tmp);
}

function start_client() {
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

    let lz4 = new LZ4();

    let mouse_events = [];
    let keyboard_events = [];

    let mouse_buttons = 0;
    let mouse_x = 0;
    let mouse_y = 0;
    let mouse_wheel = 0;

    let state = 0;
    let request = 0;
    let data_len = 0;
    let regions = 0;

    let ctx;
    let pixelRatio = window.devicePixelRatio || 1;
    let canvas = document.getElementById('canvas');
    let win_w = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;
    let win_h = window.innerHeight || document.documentElement.clientHeight || document.body.clientHeight;

    console.log("Pixel ratio " + pixelRatio);
    console.log("Windows width " + win_w * pixelRatio);
    console.log("Windows height " + win_h * pixelRatio);
    canvas.style.width = `${win_w}px`;
    canvas.style.height = `${win_h}px`;

    let socket = new WebSocket("wss://localhost:4443/projector-ws", ['binary']);

    console.log(socket.binaryType);

    socket.onopen = function(e) {
	console.log("[open] Connection established");
	console.log("Sending to server");
	state = State.ReadUint32;
	request = Request.REQ_SIGNATURE;
	send_string(socket, CLIENT_ID);

	canvas.addEventListener('mousemove', e => {
	    mouse_x = e.clientX * (ScreenInfo.width  / win_w);
	    mouse_y = e.clientY * (ScreenInfo.height / win_h);
	    mouse_events.push(new mouse_event(mouse_buttons, mouse_x, mouse_y, mouse_wheel));
	});

	canvas.addEventListener('mousedown', e => {
	    mouse_x = e.clientX * (ScreenInfo.width  / win_w);
	    mouse_y = e.clientY * (ScreenInfo.height / win_h);
	    mouse_buttons |= (1 << event.button);
	    mouse_events.push(new mouse_event(mouse_buttons, mouse_x, mouse_y, mouse_wheel));
	});

	canvas.addEventListener('mouseup', e => {
	    mouse_x = e.clientX * (ScreenInfo.width  / win_w);
	    mouse_y = e.clientY * (ScreenInfo.height / win_h);
	    mouse_buttons &= ~(1 << event.button);
	    mouse_events.push(new mouse_event(mouse_buttons, mouse_x, mouse_y, mouse_wheel));
	});
    };

    socket.onmessage = function(event) {
//  console.log(`[message] Data received from server: ${event.data}`);

	event.data.arrayBuffer().then(buffer => {
//    console.log('Buffer length ' + buffer.byteLength);

	    if (request == Request.REQ_SIGNATURE) {
		if (state == State.ReadUint32) {
		    data_len = ntohl(new Uint8Array(buffer));
		    console.log('Data len ' + data_len);
		    state = State.ReadData;
		} else if (state == State.ReadData) {
		    if (buffer.byteLength != data_len) {
			console.log("Wrong data length!");
		    } else {
			signature = array_to_string(buffer);
			console.log("Signature " + signature);

			request = Request.REQ_AUTHORIZATION;
			state = State.ReadAck;
			send_password(socket, PASSWORD);
		    }
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

		    canvas.width = ScreenInfo.width;
		    canvas.height = ScreenInfo.height;

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
//		    console.log("Reply " + reply);
		    regions = ntohl([tmp[4], tmp[5], tmp[6], tmp[7]]);
//		    console.log("Regions " + regions);
		    state = State.ReadRegionHeader;
		    if (regions == 0) {
			while (mouse_events.length > 0) {
			    send_mouse_event(socket, mouse_events.shift());
			}
			request = Request.REQ_SCREEN_UPDATE;
			state = State.ReadAck;
			setTimeout(() => send_request(socket, request), 50);
		    }
		} else if (state == State.ReadRegionHeader) {
		    regions--;
		    tmp = new Uint8Array(buffer);
		    RegionHeader.x = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
		    RegionHeader.y = ntohl([tmp[4], tmp[5], tmp[6], tmp[7]]);
		    RegionHeader.width = ntohl([tmp[8], tmp[9], tmp[10], tmp[11]]);
		    RegionHeader.height = ntohl([tmp[12], tmp[13], tmp[14], tmp[15]]);
		    RegionHeader.depth = ntohl([tmp[16], tmp[17], tmp[18], tmp[19]]);
		    RegionHeader.size = ntohl([tmp[20], tmp[21], tmp[22], tmp[23]]);
//		    console.log("x = " + RegionHeader.x);
//		    console.log("y = " + RegionHeader.y);
//		    console.log("width = " + RegionHeader.width);
//		    console.log("height = " + RegionHeader.height);
//		    console.log("depth = " + RegionHeader.depth);
//		    console.log("size = " + RegionHeader.size);
		    if (RegionHeader.size > 0) {
			state = State.ReadRegionData;
		    } else if (regions == 0) {
			while (mouse_events.length > 0) {
			    send_mouse_event(socket, mouse_events.shift());
			}
//			console.log("no regions");
			request = Request.REQ_SCREEN_UPDATE;
			state = State.ReadAck;
//			send_request(socket, request);
			setTimeout(() => send_request(socket, request), 50);
		    }
		} else if (state == State.ReadRegionData) {
		    if (RegionHeader.depth == Pix.PIX_JPEG_RGBA || RegionHeader.depth == Pix.PIX_JPEG_BGRA) {
			let tmp = new Uint8Array(buffer);
			const blob = new Blob([tmp], {type: 'image/jpeg'});
			const image = new Image();
			image.src = URL.createObjectURL(blob);
			const x = RegionHeader.x;
			const y = RegionHeader.y;
			const w = RegionHeader.width;
			const h = RegionHeader.height;
			image.onload = function() {
			    ctx.drawImage(this, x, y, w, h);
			}
		    } else if (RegionHeader.depth == Pix.PIX_LZ4_RGBA || RegionHeader.depth == Pix.PIX_LZ4_BGRA) {
			let img_data = lz4.decompress(new Uint8ClampedArray(buffer), RegionHeader.width * RegionHeader.height * 4);
			for (i = 0; i < RegionHeader.width * RegionHeader.height * 4; i += 4) {
			    let swap = img_data[i + 0];
			    img_data[i + 0] = img_data[i + 2];
			    img_data[i + 2] = swap;
			    img_data[i + 3] = 0xff; //tmp[i + 3];
			}
			ctx.putImageData(new ImageData(img_data, RegionHeader.width, RegionHeader.height), RegionHeader.x, RegionHeader.y);
		    } else if (RegionHeader.depth == Pix.PIX_RAW_RGBA || RegionHeader.depth == Pix.PIX_RAW_BGRA) {
			let tmp = new Uint8Array(buffer);
			let img_data = new Uint8ClampedArray(RegionHeader.width * RegionHeader.height * 4);
			for (i = 0; i < RegionHeader.width * RegionHeader.height * 4; i += 4) {
			    img_data[i + 0] = tmp[i + 2];
			    img_data[i + 1] = tmp[i + 1];
			    img_data[i + 2] = tmp[i + 0];
			    img_data[i + 3] = 0xff; //tmp[i + 3];
			}
			ctx.putImageData(new ImageData(img_data, RegionHeader.width, RegionHeader.height), RegionHeader.x, RegionHeader.y);
		    } else {
			console.log("Unsupported pixels yet " + RegionHeader.depth);
		    }
		    if (regions > 0) {
			state = State.ReadRegionHeader;
		    } else {
			while (mouse_events.length > 0) {
			    send_mouse_event(socket, mouse_events.shift());
			}
//			console.log("no regions");
			request = Request.REQ_SCREEN_UPDATE;
			state = State.ReadAck;
//			send_request(socket, request);
			setTimeout(() => send_request(socket, request), 50);
		    }
		}
	    }
	});
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
}

window.onload = function() {
    start_client();
}
