const CLIENT_ID = "Remote client application 1.0   ";

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

const Keys = {
    'Escape'	: 0xff1b,
    'F1'	: 0xffbe,
    'F2'	: 0xffbf,
    'F3'	: 0xffc0,
    'F4'	: 0xffc1,
    'F5'	: 0xffc2,
    'F6'	: 0xffc3,
    'F7'	: 0xffc4,
    'F8'	: 0xffc5,
    'F9'	: 0xffc6,
    'F10'	: 0xffc7,
    'F11'	: 0xffc8,
    'F12'	: 0xffc9,
    'PrintScreen' : 0xff61,
    'Delete'	: 0xffff,
    'Backquote' : 0x060,
    'Digit1'	: 0x031,
    'Digit2'	: 0x032,
    'Digit3'	: 0x033,
    'Digit4'	: 0x034,
    'Digit5'	: 0x035,
    'Digit6'	: 0x036,
    'Digit7'	: 0x037,
    'Digit8'	: 0x038,
    'Digit9'	: 0x039,
    'Digit0'	: 0x030,
    'Minus'	: 0x02d,
    'Equal'	: 0x03d,
    'Backspace' : 0xff08,
    'Tab'	: 0xff09,
    'KeyQ'	: 0x071,
    'KeyW'	: 0x077,
    'KeyE'	: 0x065,
    'KeyR'	: 0x072,
    'KeyT'	: 0x074,
    'KeyY'	: 0x079,
    'KeyU'	: 0x075,
    'KeyI'	: 0x069,
    'KeyO'	: 0x06f,
    'KeyP'	: 0x070,
    'BracketLeft': 0x05b,
    'BracketRight': 0x05d,
    'CapsLock'	: 0xffe5,
    'KeyA'	: 0x061,
    'KeyS'	: 0x073,
    'KeyD'	: 0x064,
    'KeyF'	: 0x066,
    'KeyG'	: 0x067,
    'KeyH'	: 0x068,
    'KeyJ'	: 0x06a,
    'KeyK'	: 0x06b,
    'KeyL'	: 0x06c,
    'Semicolon'	: 0x03b,
    'Quote'	: 0x027,
    'Backslash'	: 0x05c,
    'Enter'	: 0xff0d,
    'ShiftLeft' : 0xffe1,
    'IntlBackslash': 0x03c,
    'KeyZ'	: 0x07a,
    'KeyX'	: 0x078,
    'KeyC'	: 0x063,
    'KeyV'	: 0x076,
    'KeyB'	: 0x062,
    'KeyN'	: 0x06e,
    'KeyM'	: 0x06d,
    'Comma'	: 0x02c,
    'Period'	: 0x02e,
    'Slash'	: 0x02f,
    'ShiftRight': 0xffe2,
    'ControlLeft': 0xffe3,
    'MetaLeft'	: 0xffe7,
    'AltLeft'	: 0xffe9,
    'Space'	: 0x020,
    'AltRight'	: 0xffea,
    'ControlRight': 0xffe4,
    'ArrowLeft' : 0xff51,
    'ArrowUp'	: 0xff52,
    'ArrowRight': 0xff53,
    'ArrowDown'	: 0xff54,

    'Home'	: 0xff50,
    'End'	: 0xff57,
    'PageUp'	: 0xff55,
    'PageDown'	: 0xff56,

    'ScrollLock': 0xff14,
    'Insert'	: 0xff63
};

function htonl(n) {
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

function strlen(string) {
    return string.length;
}

function string_to_bytes(str) {
  let bytes = new Uint8Array(str.length);
  for (let i = 0, strLen = str.length; i < strLen; i++) {
    bytes[i] = str.charCodeAt(i);
  }
  return bytes;
}

function bytes_to_string(buf) {
  return String.fromCharCode.apply(null, new Uint8Array(buf));
}

function send_string(socket, string) {
    socket.send(htonl(strlen(string)));
    socket.send(string_to_bytes(string));
}

function send_password(socket, password) {
    let tmp = new Uint8Array(8);
    tmp.set(htonl(Request.REQ_AUTHORIZATION), 0);
    tmp.set(htonl(strlen(password)), 4);
    socket.send(tmp);
    socket.send(string_to_bytes(password));
}

function send_request(socket, request) {
    socket.send(htonl(request));
}

function mouse_event(buttons, x, y, wheel) {
    this.type = Request.REQ_POINTER;
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

function keyboard_event(down, key) {
    this.type = Request.REQ_KEYBOARD;
    this.down = down;
    this.key = key;
}

function send_keyboard_event(socket, event) {
    let tmp = new Uint8Array(12);
    tmp.set(htonl(Request.REQ_KEYBOARD), 0);
    tmp.set(htonl(event.down), 4);
    tmp.set(htonl(event.key), 8);
    socket.send(tmp);
}

function send_input_event(socket, event) {
    if (event.type == Request.REQ_POINTER) {
	send_mouse_event(socket, event);
    } else {
	send_keyboard_event(socket, event);
    }
}

function start_client(remote_url, password, onerror) {
    function mouse_move(e) {
	mouse_x = e.clientX * (ScreenInfo.width  / win_w);
	mouse_y = e.clientY * (ScreenInfo.height / win_h);
	input_events.push(new mouse_event(mouse_buttons, mouse_x, mouse_y, mouse_wheel));
    }

    function mouse_down(e) {
	mouse_x = e.clientX * (ScreenInfo.width  / win_w);
	mouse_y = e.clientY * (ScreenInfo.height / win_h);
	mouse_buttons |= (1 << event.button);
	input_events.push(new mouse_event(mouse_buttons, mouse_x, mouse_y, mouse_wheel));
    }

    function mouse_up(e) {
	mouse_x = e.clientX * (ScreenInfo.width  / win_w);
	mouse_y = e.clientY * (ScreenInfo.height / win_h);
	mouse_buttons &= ~(1 << event.button);
	input_events.push(new mouse_event(mouse_buttons, mouse_x, mouse_y, mouse_wheel));
    }

    function mouse_onwheel(e) {
	input_events.push(new mouse_event(mouse_buttons | ((e.deltaY < 0) ? (1 << 3) : (e.deltaY > 0) ? (1 << 4) : 0), mouse_x, mouse_y, mouse_wheel));
	input_events.push(new mouse_event(mouse_buttons, mouse_x, mouse_y, mouse_wheel));
    }

    function keyboard_down(e) {
	e.preventDefault();
	let key = Keys[e.code];
	if (key) {
	    input_events.push(new keyboard_event(1, key));
	}
    }

    function keyboard_up(e) {
	e.preventDefault();
	let key = Keys[e.code];
	if (key) {
	    input_events.push(new keyboard_event(0, key));
	}
    }

    function start_input_devices() {
	canvas.addEventListener('mousemove', mouse_move);
	canvas.addEventListener('mousedown', mouse_down);
	canvas.addEventListener('mouseup', mouse_up);
	canvas.addEventListener('wheel', mouse_onwheel);
	window.addEventListener('keydown', keyboard_down);
	window.addEventListener('keyup', keyboard_up);
	input_devices_started = true;
    }

    function stop_input_devices() {
	canvas.removeEventListener('mousemove', mouse_move);
	canvas.removeEventListener('mousedown', mouse_down);
	canvas.removeEventListener('mouseup', mouse_up);
	canvas.removeEventListener('wheel', mouse_onwheel);
	window.removeEventListener('keydown', keyboard_down);
	window.removeEventListener('keyup', keyboard_up);
    }

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

    let input_events = [];

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

    let socket = new WebSocket(remote_url, ['binary']);

    socket.binaryType = 'arraybuffer';

    console.log(socket.binaryType);

    socket.onopen = function(e) {
	console.log("[open] Connection established");
	console.log("Sending to server");
	state = State.ReadUint32;
	request = Request.REQ_SIGNATURE;
	send_string(socket, CLIENT_ID);
    };

    socket.onmessage = function(event) {
//	console.log(`[message] Data received from server: ${event.data}`);
//	console.log('Buffer length ' + event.data.byteLength);
	if (request == Request.REQ_SIGNATURE) {
	    if (state == State.ReadUint32) {
		data_len = ntohl(new Uint8Array(event.data));
		console.log('Data len ' + data_len);
		state = State.ReadData;
	    } else if (state == State.ReadData) {
		if (event.data.byteLength != data_len) {
		    console.log("Wrong data length!");
		} else {
		    signature = bytes_to_string(event.data);
		    console.log("Signature " + signature);

		    request = Request.REQ_AUTHORIZATION;
		    state = State.ReadAck;
		    send_password(socket, password);
		}
	    }
	} else if (request == Request.REQ_AUTHORIZATION) {
	    if (state == State.ReadAck) {
		status = ntohl(new Uint8Array(event.data));
		if (status != 0) {
		    console.log("Wrong password!");
		    onerror(1, "Bad password");
		}

		request = Request.REQ_SCREEN_INFO;
		state = State.ReadAck;
		send_request(socket, request);
	    }
	} else if (request == Request.REQ_SCREEN_INFO) {
	    if (state == State.ReadAck) {
		tmp = new Uint8Array(event.data);
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
		tmp = new Uint8Array(event.data);
		reply = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
		console.log("Reply " + reply);
		data_len = ntohl(tmp[4], tmp[5], tmp[6], tmp[7]);
		state = State.ReadData;
	    } else if (state == State.ReadData) {
		RemoteHost = bytes_to_string(event.data);
		console.log("Remote host " + RemoteHost);
		document.title = RemoteHost;
		start_input_devices();
		request = Request.REQ_SCREEN_UPDATE;
		state = State.ReadAck;
		send_request(socket, request);
	    }
	} else if (request == Request.REQ_SCREEN_UPDATE) {
	    if (state == State.ReadAck) {
		tmp = new Uint8Array(event.data);
		reply = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
//		  console.log("Reply " + reply);
		regions = ntohl([tmp[4], tmp[5], tmp[6], tmp[7]]);
//		  console.log("Regions " + regions);
		state = State.ReadRegionHeader;

		if (regions == 0) {
		    while (input_events.length > 0) {
			send_input_event(socket, input_events.shift());
		    }
		    request = Request.REQ_SCREEN_UPDATE;
		    state = State.ReadAck;
		    setTimeout(() => send_request(socket, request), 50);
		}
	    } else if (state == State.ReadRegionHeader) {
		regions--;
		tmp = new Uint8Array(event.data);
		RegionHeader.x = ntohl([tmp[0], tmp[1], tmp[2], tmp[3]]);
		RegionHeader.y = ntohl([tmp[4], tmp[5], tmp[6], tmp[7]]);
		RegionHeader.width = ntohl([tmp[8], tmp[9], tmp[10], tmp[11]]);
		RegionHeader.height = ntohl([tmp[12], tmp[13], tmp[14], tmp[15]]);
		RegionHeader.depth = ntohl([tmp[16], tmp[17], tmp[18], tmp[19]]);
		RegionHeader.size = ntohl([tmp[20], tmp[21], tmp[22], tmp[23]]);
//		  console.log("x = " + RegionHeader.x);
//		  console.log("y = " + RegionHeader.y);
//		  console.log("width = " + RegionHeader.width);
//		  console.log("height = " + RegionHeader.height);
//		  console.log("depth = " + RegionHeader.depth);
//		  console.log("size = " + RegionHeader.size);

		if (RegionHeader.size > 0) {
		    state = State.ReadRegionData;
		} else if (regions == 0) {
		    while (input_events.length > 0) {
			send_input_event(socket, input_events.shift());
		    }
//		      console.log("no regions");
		    request = Request.REQ_SCREEN_UPDATE;
		    state = State.ReadAck;
//		      send_request(socket, request);
		    setTimeout(() => send_request(socket, request), 50);
		}
	    } else if (state == State.ReadRegionData) {
		if (RegionHeader.depth == Pix.PIX_JPEG_RGBA || RegionHeader.depth == Pix.PIX_JPEG_BGRA) {
		    let tmp = new Uint8Array(event.data);
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
		    let img_data = lz4.decompress(new Uint8ClampedArray(event.data), RegionHeader.width * RegionHeader.height * 4);
		    for (let i = 0; i < RegionHeader.width * RegionHeader.height * 4; i += 4) {
			let swap = img_data[i + 0];
			img_data[i + 0] = img_data[i + 2];
			img_data[i + 2] = swap;
			img_data[i + 3] = 0xff; //tmp[i + 3];
		    }
		    ctx.putImageData(new ImageData(img_data, RegionHeader.width, RegionHeader.height), RegionHeader.x, RegionHeader.y);
		} else if (RegionHeader.depth == Pix.PIX_RAW_RGBA || RegionHeader.depth == Pix.PIX_RAW_BGRA) {
		    let tmp = new Uint8Array(event.data);
		    let img_data = new Uint8ClampedArray(RegionHeader.width * RegionHeader.height * 4);
		    for (let i = 0; i < RegionHeader.width * RegionHeader.height * 4; i += 4) {
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
		    while (input_events.length > 0) {
			send_input_event(socket, input_events.shift());
		    }
//		      console.log("no regions");
		    request = Request.REQ_SCREEN_UPDATE;
		    state = State.ReadAck;
//		      send_request(socket, request);
		    setTimeout(() => send_request(socket, request), 50);
		}
	    }
	}
    };

    socket.onclose = function(event) {
	stop_input_devices();
	if (event.wasClean) {
	    console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
	    onerror(0, "Disconnected");
	} else {
	    // e.g. server process killed or network down
	    // event.code is usually 1006 in this case
	    console.log('[close] Connection died');
	    onerror(2, "Connection error");
	}
    };

    socket.onerror = function(error) {
	console.log(`[error] ${error.message}`);
    };
}
