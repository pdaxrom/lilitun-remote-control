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
	"sessionId TEXT PRIMARY KEY,".
	"appServer TEXT,".
	"controlServer TEXT".
	")");

    $dbase->exec("CREATE TABLE IF NOT EXISTS Projectors(".
	"id INTEGER PRIMARY KEY,".
	"sessionId TEXT,".
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
			http_response_code(200);

			$content = $port.': 127.0.0.1:'.$port;
			file_put_contents('/var/cache/token/'.$sessionId.'.conf', $content);

			exit;
		    }
		}
	    }
	    if ($requestType === 'remoteControlStop') {
		$dbase->exec("DELETE FROM Projectors WHERE sessionId='$sessionId'");
		http_response_code(200);

		unlink('/var/cache/token/'.$sessionId.'.conf');

		exit;
	    }
	}
    }

    http_response_code(403);
    exit;
}

if (!@$_SESSION['sessionId']) {
//    $_SESSION['sessionId'] = bin2hex(random_bytes(12));
    $_SESSION['sessionId'] = guidv4();
}

$sessionId = $_SESSION['sessionId'];

$appServer = getenv('HOST_NAME');

$controlServer = getenv('HOST_NAME');

if (!$appServer) {
    $appServer = gethostname();
}

if (!$controlServer) {
    $controlServer = gethostname();
}

if (!$appServer) {
    $appServer = APPSERVER;
}

if (!controlServer) {
    $controlServer = CONTROLSERVER;
}

$dbase->exec("INSERT OR IGNORE INTO Sessions(sessionId, appServer, controlServer) VALUES('$sessionId', '$appServer', '$controlServer')");
?>
<html>
<head>
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
'"appServerUrl":"'.$appServer.'/desktop.php",'.
'"controlServerUrl":"'.$controlServer.'",'.
'"sessionId":"'.$sessionId.'"'.
"}");?>";
    } else {
	alert("Unsipported platform!");
    }
}

</script>
</head>
<body>
<h2>LiliTun remote control session</h2>
<form onsubmit="return updateProjectorLink();">
  <label for="appServer">Application server:</label><br>
  <input type="text" id="appServer" name="appServer" value="<?php echo $appServer; ?>" disabled><br>
  <label for="controlServer">Control server:</label><br>
  <input type="text" id="controlServer" name="controlServer" value="<?php echo $controlServer; ?>" disabled><br>
  <label for="sessionId">Session Id:</label><br>
  <input type="text" id="sessionId" name="sessionId" value="<?php echo $sessionId; ?>" disabled><br>
  <input type="submit" value="Update link" disabled>
</form>

Launch the remote conrol app with this link <a id="start_link" href="lilink://<?php echo base64_encode('{"authHeader":"authHeader value",'.
'"requestType":"remoteControl",'.
'"appServerUrl":"'.$appServer.'/desktop.php",'.
'"controlServerUrl":"'.$controlServer.'",'.
'"sessionId":"'.$sessionId.'"'.
"}");?>">Launch remote control app</a><br><br>

<!--
Start the projector app with this link <a id="start_link" href="" onclick="startApp(); return false;">Share screen</a><br><br>
-->

<h2>Remote session</h2>

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
