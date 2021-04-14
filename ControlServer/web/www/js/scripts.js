//
//
//

function getOS() {
  var userAgent = window.navigator.userAgent,
      platform = window.navigator.platform,
      macosPlatforms = ['Macintosh', 'MacIntel', 'MacPPC', 'Mac68K'],
      windowsPlatforms = ['Win32', 'Win64', 'Windows', 'WinCE'],
      iosPlatforms = ['iPhone', 'iPad', 'iPod'],
      os = null;

  if (macosPlatforms.indexOf(platform) !== -1) {
    os = 'MacOS';
  } else if (iosPlatforms.indexOf(platform) !== -1) {
    os = 'iOS';
  } else if (windowsPlatforms.indexOf(platform) !== -1) {
    os = 'Windows';
  } else if (/Android/.test(userAgent)) {
    os = 'Android';
  } else if (!os && /Linux/.test(platform)) {
    os = 'Linux';
  }

  return os;
}

//
//
//
function loadFile(url, timeout, callback) {
    var args = Array.prototype.slice.call(arguments, 3);
    var xhr = new XMLHttpRequest();
    xhr.ontimeout = function () {
	console.error("The request for " + url + " timed out.");
    };
    xhr.onload = function() {
	if (xhr.readyState === 4) {
	    if (xhr.status === 200) {
		if (callback != null) {
		    callback.apply(xhr, args);
		}
	    } else {
		console.error(xhr.statusText);
	    }
	}
    };
    xhr.open("GET", url, true);
    xhr.timeout = timeout;
    xhr.send(null);
}

//
//
//
let page_timer;

async function page_fetch(el) {
    let preexec = el.getAttribute("preexec");
    if (preexec != null) {
	if (typeof window[preexec] === "function") {
	    window[preexec](el);
	} else {
	    console.log("Function " + preexec + " not found!");
	}
	el.setAttribute("preexec", "");
    }
    let url = el.getAttribute("src");
    if (url !== null && url.length > 0) {
	fetch(url).then(function(response) {
	    return response.text().then(function(text) {
		while (el.firstChild) {
		    el.removeChild(el.lastChild);
		}

		if (url.substring(url.lastIndexOf('.') + 1) == 'js') {
		    let script = document.createElement("script");
		    script.type="text/javascript";
		    script.innerHTML = text;
		    el.appendChild(script);
		} else {
		    let span = document.createElement("span");
		    span.innerHTML = text;
		    el.appendChild(span);
		    page_updater_onload(el);
		}

		let exec = el.getAttribute("exec");
		if (exec != null) {
		    if (typeof window[exec] === "function") {
			window[exec](el);
		    } else {
			console.log("Function " + exec + " not found!");
		    }
		}
	    });
	});
    } else {
	let exec = el.getAttribute("exec");
	if (exec != null) {
	    if (typeof window[exec] === "function") {
		window[exec](el);
	    } else {
		console.log("Function " + exec + " not found!");
	    }
	}
    }
}

function page_updater_onload(doc) {
    let elements;
    while ((elements = doc.getElementsByClassName("refreshnow")).length > 0) {
	let element = elements[0];
	elements[0].className = elements[0].className.replace(/\brefreshnow\b/g, "");
	page_fetch(element);
    }
}

function page_updater() {
    let elements = document.getElementsByClassName("autorefresh");
    for (let i = 0; i < elements.length; i++) {
	let element = elements[i];
	page_fetch(element);
    }
}

function page_updater_stop() {
    clearInterval(page_timer);
}

window.onload = function() {
    page_updater_onload(document);
    page_timer = setInterval(page_updater, 2000);
}
