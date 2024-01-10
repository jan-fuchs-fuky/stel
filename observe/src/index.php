<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="cs">
  <head>
    <meta http-equiv="Content-language" content="cs">
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
    <meta http-equiv="Pragma" content="no-cache">
    <meta http-equiv="Cache-Control" content="no-cache">
    <meta http-equiv="Expires" content="-1">
    <title>Observe status</title>
    <meta name="author" content="Jan Fuchs">
    <meta name="robots" content="index">
  </head>
<body bgcolor="#d0dce0">
<center>

<table bgcolor="#ffffff" border="0" cellpadding="10" cellspacing="0" width="820">
  <tbody><tr bgcolor="#ffffff"><td>  

<h1>Observe status</h1>

<table bgcolor="#000000" border="0" cellpadding="5" cellspacing="1">
<tr bgcolor="#D0DCE0">
<td>Host</td>
<td>Port</td>
<td>Daemon</td>
<td>State</td>
<td>Last availability</td>
</tr>

<?php

// DEBUG
//ini_set("display_startup_errors", 1);
//ini_set("display_errors", 1);
//error_reporting(-1);

$mysqli = new mysqli("tyche", "observe", "heslo", "observe");
if ($mysqli->connect_errno) {
    $this->error("Failed to connect to MySQL");
}

$mysqli->set_charset('utf8');

$result = $mysqli->query("SELECT host, port, daemon, date, UNIX_TIMESTAMP(date) as timestamp_last, UNIX_TIMESTAMP() as timestamp_now FROM daemons");
if ($result->num_rows > 0) {
    while ($line = $result->fetch_assoc()) {
        $timestamp_diff = (int)$line["timestamp_now"] - (int)$line["timestamp_last"];

        if ($timestamp_diff > 300) {
            $color = "#FF8888";
            $state = "not available";
        }
        else {
            $color = "#FFFFFF";
            $state = "available";
        }

        echo "<tr bgcolor=\"#FFFFFF\">\n";
        echo '<td bgcolor="'.$color.'">'.$line["host"]."</td>\n";
        echo '<td bgcolor="'.$color.'">'.$line["port"]."</td>\n";
        echo '<td bgcolor="'.$color.'">'.$line["daemon"]."</td>\n";
        echo '<td bgcolor="'.$color.'">'.$state."</td>\n";
        echo '<td bgcolor="'.$color.'">'.$line["date"]."</td>\n";
        echo '</tr>';
    }
}

$mysqli->close();

?>

</table>

    </td>
  </tr></tbody>
</table>
</center>
</body>
