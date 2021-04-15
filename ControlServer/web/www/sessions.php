<?php

require_once('config.php');

session_start();

if (@$_SESSION['sessionId']) {
    $sessionId = $_SESSION['sessionId'];

    $dbase = new PDO("sqlite:" . DBASEFILE);

    $row = $dbase->query(
"SELECT Sessions.appServer as appServer, Sessions.controlServer as controlServer, Projectors.clientPort as clientPort,".
" Projectors.clientIpv6Port as clientIpv6Port, Projectors.clientPassword as clientPassword, Projectors.startTime as startTime,".
" Projectors.hostname as host".
" FROM Sessions, Projectors WHERE Projectors.sessionId = '$sessionId'".
" AND Sessions.sessionId = '$sessionId'"
)->fetch();

    if ($row) {
/*	$link="https://{$row['appServer']}/novnc/vnc.html?autoconnect=true&".
"host={$row['controlServer']}&port={$row['clientPort']}&resize=scale&encrypt=1&password={$row['clientPassword']}";
 */
	$link="https://{$row['appServer']}/novnc/vnc.html?autoconnect=true&".
"host={$row['controlServer']}&port=443&resize=scale&encrypt=1&password={$row['clientPassword']}&".
"path=/websockify?token={$row['clientPort']}";
?>
<a href="<?php echo $link; ?>" target="_blank">Users can connect to the shared desktop <?php echo $row['host'];?> via this link</a><br>
<a href="" onclick="loadFile('/control.php?requestType=stopSharing&sessionId=<?php echo $sessionId;?>'); return false;">Click this link to stop desktop sharing</a>
<?php
    }
}

?>