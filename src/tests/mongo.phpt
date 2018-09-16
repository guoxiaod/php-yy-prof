--TEST--
Check mongo
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
<?php if (!extension_loaded("mongo")) print "skip"; ?>
--EXTENSIONS--
mongo
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
--FILE--
<?php 

yy_prof_clear_stats();

$c = 'mongodb://10.0.11.226';
$m = new mongo($c);
$db = $m->selectdb('test');
$collection = new mongocollection($db, 'produce');

// 搜索水果
$fruitquery = array('type' => 'fruit');

$cursor = $collection->find($fruitquery);
var_export($cursor);
echo "\n";
foreach ($cursor as $doc) {
    var_dump($doc);
}

// 搜索甜的产品 taste is a child of details. 
$sweetquery = array('details.taste' => 'sweet');
$cursor = $collection->find($sweetquery);
var_export($cursor);
echo "\n";
foreach ($cursor as $doc) {
    var_dump($doc);
    echo "\n";
}

$r = yy_prof_get_all_func_stat();

var_export($r);
?>
--EXPECTF--
MongoCursor::__set_state(array(
))
MongoCursor::__set_state(array(
))
array (
  'MDB:produce' => 
  array (
    'type' => 6,
    'count' => 2,
    'time' => %d,
    'request_bytes' => %d,
    'response_bytes' => %d,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
)
