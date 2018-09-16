--TEST--
Check mongodb
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--EXTENSIONS--
json
mongodb
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
--FILE--
<?php 

yy_prof_clear_stats();

$filter = array(
    "tag" => "mongodb",
    "views" => array('$gt' => 5),
);
$options = array(
    "projection" => array(
        "title" => 1,
        "article" => 1,
    ),
    "sort" => array(
        "views" => -1,
    ),
);
$readPreference = new MongoDB\Driver\ReadPreference(MongoDB\Driver\ReadPreference::RP_PRIMARY);
$query = new MongoDB\Driver\Query($filter, $options);

$manager = new MongoDB\Driver\Manager("mongodb://127.0.0.1:27017");
var_export($manager);
echo "\n";
$cursor = $manager->executeQuery("test.produce", $query, $readPreference);
var_export($cursor);
echo "\n";

foreach($cursor as $document) {
    echo $document["title"], "\n";
}

$r = yy_prof_get_all_func_stat();
var_export($r);
?>
--EXPECTF--
MongoDB\Driver\Manager::__set_state(array(
))
MongoDB\Driver\Cursor::__set_state(array(
))
array (
  'MDB2:test.produce' => 
  array (
    'type' => 5,
    'count' => 1,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 1,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
)
