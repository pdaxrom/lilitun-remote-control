<?php

require_once('config.php');

session_start();

setcookie('PHPSESSID', session_id(), time() + 60 * 60 * 24 * 7, '/');


function guidv4($data = null) {
    // Generate 16 bytes (128 bits) of random data or use the data passed into the function.
    $data = $data ?? random_bytes(16);
    assert(strlen($data) == 16);

    // Set version to 0100
    $data[6] = chr(ord($data[6]) & 0x0f | 0x40);
    // Set bits 6-7 to 10
    $data[8] = chr(ord($data[8]) & 0x3f | 0x80);

    // Output the 36 character UUID.
    return vsprintf('%s%s-%s-%s-%s-%s%s%s', str_split(bin2hex($data), 4));
}

$dbase = new PDO("sqlite:" . DBASEFILE);
try {
    $dbase->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    $dbase->exec("CREATE TABLE IF NOT EXISTS Sessions(".
	"id INTEGER PRIMARY KEY,".
	"sessionId TEXT NOT NULL UNIQUE,".
	"userId TEXT,".
	"appServer TEXT,".
	"controlServer TEXT,".
	"lastTime INTEGER".
	")");

    $dbase->exec("CREATE TABLE IF NOT EXISTS Projectors(".
	"id INTEGER PRIMARY KEY,".
	"sessionId TEXT NOT NULL UNIQUE,".
	"hostname TEXT,".
	"clientPort,".
	"clientIpv6Port,".
	"clientPassword,".
	"startTime INTEGER,".
	"stopTime INTEGER".
	")");

} catch(PDOException $e) {
    die("OOPs something went wrong");
}

if (@$_REQUEST['action']) {
    $action = addslashes($_REQUEST['action']);

    if ($action === 'message') {
	$json = file_get_contents('php://input');
	$data = json_decode($json);

	$requestType = addslashes($data->requestType);
	if ($requestType === 'remoteControlStart' || $requestType === 'remoteControlStop') {
	    $sessionId = addslashes($data->sessionId);
	    if ($requestType === 'remoteControlStart' && @$data->port && @$data->ipv6port && @$data->password) {
		$host = addslashes($data->hostname);
		$port = $data->port;
		$ipv6port = $data->ipv6port;
		$password = addslashes($data->password);
		if (is_numeric($port) && is_numeric($ipv6port) && $password !== '') {
		    $sessionId_check = $dbase->query("SELECT sessionId FROM Sessions WHERE sessionId='$sessionId'");
		    if ($sessionId_check) {
			$time = time();
			$dbase->exec("INSERT INTO Projectors(sessionId, hostname, clientPort, clientIpv6Port, clientPassword, startTime)".
" VALUES('$sessionId', '$host', $port, $ipv6port, '$password', '$time')");
			$dbase->exec("UPDATE Sessions SET lastTime=$time WHERE sessionId = '$sessionId'");
			http_response_code(200);

			$content = $port.': 127.0.0.1:'.$port;
			file_put_contents('/var/cache/token/'.$sessionId.'.conf', $content);

			exit;
		    }
		}
	    }
	    if ($requestType === 'remoteControlStop') {
		$dbase->exec("DELETE FROM Projectors WHERE sessionId='$sessionId'");
		$dbase->exec("UPDATE Sessions SET lastTime=strftime('%s', 'now') WHERE sessionId = '$sessionId'");
		http_response_code(200);

		unlink('/var/cache/token/'.$sessionId.'.conf');

		exit;
	    }
	}
    }

    http_response_code(403);
    exit;
}

$errUserId = "";

if (@$_SESSION['errUserId']) {
    $errUserId = $_SESSION['errUserId'];
    unset($_SESSION['errUserId']);
}

if (@$_REQUEST['userId_submitted']) {
    $newUserId = trim(addslashes($_REQUEST['newUserId']));

    if ($newUserId !== "" && $newUserId !== $_SESSION['userId']) {
	if (preg_match('/^[0-9A-F]{8}-[0-9A-F]{4}-4[0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}$/i', $_SESSION['userId']) !== 1) {
	    if ($_SESSION['userId'] !== "") {
		$_SESSION['errUserId'] = "<span class=\"err\"> (Invalid user ID!)</span>";
	    }
	} else {
	    $_SESSION['userId'] = $newUserId;
	    $_SESSION['sessionId'] = guidv4();
	}
    }

    header("Location: desktop.php", true, 301);
    exit();
} elseif (!@$_SESSION['userId']) {
    $_SESSION['userId'] = guidv4();
}

if (!@$_SESSION['sessionId']) {
    $_SESSION['sessionId'] = guidv4();
}

$userId = $_SESSION['userId'];

$sessionId = $_SESSION['sessionId'];

$appServer = gethostname();

$controlServer = gethostname();

if (!$appServer) {
    $appServer = APPSERVER;
}

if (!$controlServer) {
    $controlServer = CONTROLSERVER;
}

$appServer = 'https://'.$appServer."/desktop.php";
$controlServer = 'wss://'.$controlServer."/projector-ws";

$dbase->exec("INSERT OR IGNORE INTO Sessions(sessionId, userId, appServer, controlServer) VALUES('$sessionId', '$userId', '$appServer', '$controlServer')");
$dbase->exec("UPDATE Sessions SET lastTime=strftime('%s', 'now') WHERE userId = '$userId' AND sessionId = '$sessionId'");

?>
<html>
<head>
<title>LiliTun remote control</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" type="text/css" href="css/style.css" />
<script>
<?php
include_once('js/scripts.js');
?>

function startApp() {
    let dl = "";

    let os = getOS();

    if (os == "Windows") {
	dl = "downloads/windows/lilitun-remote-control.exe";
    } else if (os == "MacOS") {
	dl = "downloads/macos/LiliTun%20remote%20control-1.0.pkg";
    } else if (os == "Linux") {
	dl = "downloads/linux/lilitun-remote-control-1.0-x86_64.AppImage";
    }

    if (dl != "") {
	setTimeout(function () { window.location = dl; }, 25);
	window.location = "lilink://<?php echo base64_encode('{"authHeader":"authHeader value",'.
'"requestType":"remoteControl",'.
'"appServerUrl":"'.$appServer.'",'.
'"controlServerUrl":"'.$controlServer.'",'.
'"sessionId":"'.$sessionId.'"'.
"}");?>";
    } else {
	alert("Unsipported platform!");
    }
}

function CopyFunction(idName, idToolTip) {
    let inputs = document.getElementById(idName).elements;
    let el = document.createElement('textarea');
    el.value = inputs['userId'].value;
    el.setAttribute('readonly', '');
    el.style = {position: 'absolute', left: '-9999px'};
    document.body.appendChild(el);
    el.select();
    document.execCommand('copy');
    document.body.removeChild(el);

    let tooltip = document.getElementById(idToolTip);
    tooltip.innerHTML = "Copied: " + inputs['userId'].value;

    return false;
}

function OutCopyFunction(idToolTip) {
  let tooltip = document.getElementById(idToolTip);
  tooltip.innerHTML = "";

  return false;
}

</script>
</head>
<body>
<h2>LiliTun remote control</h2>
  <div class="support_form">
    <form id="userId_form" action="desktop.php">
      <input type="hidden" name="userId_submitted" value="1">
      <div>
        <label for="userId">User Id<?php echo $errUserId; ?></label>
        <input type="text" id="userId" name="userId" value="<?php echo $userId; ?>" disabled>
        <input type="submit" value="Copy User Id" onclick="return CopyFunction('userId_form', 'tooltip');" onmouseout="return OutCopyFunction('tooltip');">
        <span class="tooltiptext" id="tooltip"></span>
      </div>
      <div>
        <label for="newUserId">Existing User Id</label>
        <input type="text" id="newUserId" name="newUserId" value="">
        <input type="submit" value="Update User Id"><br>
      </div>
  </form>
</div>

Launch the remote conrol app with this link <a id="start_link" href="lilink://<?php echo base64_encode('{"authHeader":"authHeader value",'.
'"requestType":"remoteControl",'.
'"appServerUrl":"'.$appServer.'",'.
'"controlServerUrl":"'.$controlServer.'",'.
'"sessionId":"'.$sessionId.'"'.
"}");?>">Launch remote control app</a><br><br>

<h2>Remote user sessions</h2>

<span class="autorefresh refreshnow" src="sessions.php"></span>

<h2>Downloads</h2>
Don't have LiliTun remote control installed? Download it now:<br>
<a href="downloads/linux/lilitun-remote-control-1.0-x86_64.AppImage">LiliTun remote control for Linux x86_64, Ubuntu 18.04+ (AppImage)</a><br>
<a href="downloads/macos/LiliTun%20remote%20control-1.0.pkg">LiliTun remote control for MacOS 10.9+</a><br>
<a href="downloads/windows/lilitun-remote-control.exe">LiliTun remote control for Windows</a><br>
<br>
For Linux and Windows, after downloading, run the application to register it in the system. Once launched, the application will close and will be ready to launch from the link.
</body>
</html>
<?php

?>
