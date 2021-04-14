<?php

require_once('config.php');

session_start();

if (@$_REQUEST['requestType']) {
    $requestType = $_REQUEST['requestType'];

    if ($requestType == 'stopSharing') {
	if (@$_REQUEST['sessionId']) {
	    $sessionId = addslashes($_REQUEST['sessionId']);

	    $dbase = new PDO("sqlite:" . DBASEFILE);

	    $controlServer = $dbase->query("SELECT controlServer FROM Sessions WHERE sessionId='$sessionId'")->fetchColumn();
	    if ($controlServer != "") {
//		$arrContextOptions = array(
//		    "ssl" => array(
//		    "verify_peer" => false,
//		    "verify_peer_name" => false,
//		    ),
//		);
		// Only for ssl control
		//file_get_contents("https://$controlServer:9996/?requestType=$requestType&sessionId=$sessionId", false, stream_context_create($arrContextOptions));
		//file_get_contents("http://$controlServer:9996/?requestType=$requestType&sessionId=$sessionId", false, stream_context_create($arrContextOptions));

		$url = "http://$controlServer:9996/?action=message";
		$ch = curl_init($url);
		$data = array(
		    'requestType' => $requestType,
		    'sessionId' => $sessionId,
		);
		$payload = json_encode($data);
		curl_setopt($ch, CURLOPT_POSTFIELDS, $payload);
		curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type:application/json'));
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
		$result = curl_exec($ch);
		curl_close($ch);
	    }
	}
    }
}

?>
