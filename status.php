<?php

  define("DATABASE_SERVER", "localhost");
  define("DATABASE_USER", "api");
  define("DATABASE_PASSWORD", "********");

  $outcome = array();

  function GetDDBState($name)
  {
    $connection = mysql_connect(DATABASE_SERVER, DATABASE_USER, DATABASE_PASSWORD);
    mysql_select_db($name, $connection);
    $result = mysql_query("SELECT MAX(`Date`) AS `date` FROM Tables", $connection);
    $row = mysql_fetch_assoc($result);
    mysql_close($connection);

    $date = DateTime::createFromFormat("Y-m-d H:i:s", $row["date"], new DateTimeZone("UTC"));
    return array("date" => $date->getTimestamp());
  }

  function GetRP2CState($file)
  {
    $data = file_get_contents($file);
    $parameters = unpack("A19date/@31/astate", $data);
    $date = DateTime::createFromFormat("Y-m-d H:i:s", $parameters["date"], new DateTimeZone("UTC"));
    $state = ($parameters["state"] == '*');
    return array("date" => $date->getTimestamp(), "state" => $state);
  }

  function GetCCSState($file)
  {
    $data = file_get_contents($file);
    $parameters = unpack("Lnumber/Lzero/a14date", $data);
    $date = DateTime::createFromFormat("YmdHis", $parameters["date"], new DateTimeZone("CET"));
    return array("date" => $date->getTimestamp());
  }

  function GetMasterState($file)
  {
    $data = file_get_contents($file);
    $parameters = unpack("Qdate", $data);
    return array("date" => $parameters["date"]);
  }

  $outcome = array(
    "A" => GetDDBState("Local"),
    "B" => GetDDBState("Global"),
    "C" => GetRP2CState("/opt/BorderGate/Data/ModuleC.dat"),
    "D" => GetCCSState("/opt/BorderGate/Data/ModuleD.dat"),
    "E" => GetMasterState("/opt/BorderGate/Data/ModuleE.dat")
  );

  if (array_key_exists("callback", $_GET))
  {
    header("Conent-Type: application/javascript");
    print($_GET["callback"] . "(" . json_encode($outcome) . ")");
  }
  else
  {
    header("Conent-Type: application/json");
    print(json_encode($outcome));
  }

?>
