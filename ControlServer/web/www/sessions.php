<?php

require_once('config.php');

session_start();

if (@$_SESSION['userId']) {
    $userId = $_SESSION['userId'];

    $dbase = new PDO("sqlite:" . DBASEFILE);

    $rows = $dbase->query(
"SELECT Sessions.appServer as appServer, Sessions.controlServer as controlServer, Projectors.clientPort as clientPort,".
" Projectors.clientIpv6Port as clientIpv6Port, Projectors.clientPassword as clientPassword, Projectors.startTime as startTime,".
" Projectors.hostname as host, Projectors.sessionId as sessionId".
" FROM Sessions, Projectors WHERE Projectors.sessionId = Sessions.sessionId AND Sessions.userId = '$userId'"
);

    foreach ($rows as $row) {
	$controlServerHost = parse_url($row['controlServer'], PHP_URL_HOST);
	$appServerHost = parse_url($row['appServer'], PHP_URL_HOST);
	$link="https://$appServerHost/novnc/vnc.html?autoconnect=true&".
"host=$controlServerHost&port=443&resize=scale&encrypt=1&password={$row['clientPassword']}&".
"path=/websockify?token={$row['clientPort']}";
?>
<a href="<?php echo $link; ?>" target="_blank">Users can connect to the shared desktop '<?php echo "{$row['host']}";?>' via this link</a><br>
<a href="" onclick="loadFile('/control.php?requestType=stopSharing&sessionId=<?php echo "{$row['sessionId']}";?>'); return false;">Click this link to stop desktop sharing '<?php echo "{$row['host']}";?>'</a>
<br><br>
<?php
    }
}

?>
