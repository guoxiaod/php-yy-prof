--TEST--
Check cli mode
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
yy_prof.enabled_by_detector=0
--EXTENSIONS--
msgpack
json
yar
curl
--FILE--
<?php 

yy_prof_clear_stats();

ini_set("yy_prof.slow_run_time", 1);

$func = "consumer";
$funcs = array("a", "b", "c");


function a() {
    $a = 'hello world';
    usleep(100000);
}

function b() {
    $a = 'hello world';
    usleep(100000);
}

function c() {
    $a = 'hello world';
    usleep(100000);
}


$memoryList = array(1,1,1,1,1,1,1,1,1,1);
for($i = 0; $i < 10; $i ++) {
    yy_prof_set_funcs($funcs);
    yy_prof_enable($func);
    a();
    b();
    c();
    yy_prof_disable();
    $memoryList[$i] = memory_get_usage();
}
$r = yy_prof_get_all_page_stat();
ksort($r);
var_export($r); echo "\n";
$key = '';
foreach($r as $k => $v) {
    if(strpos($k, "#") !== false) {
        $key = $k;
        break;
    } 
}

$r = yy_prof_get_page_func_stat($key);
var_export($r);

array_shift($memoryList);
$sum = array_sum($memoryList);
$avg = $sum / count($memoryList);

if($memoryList[0] == $avg) {
    echo "\nmemory is ok\n";
} else {
    echo "\nmemory is leaks\n";
}

ini_set("yy_prof.protect_load", 0);
ini_set("yy_prof.protect_load_restore", 2);

yy_prof_enable("test");

$x = curl_init();
curl_setopt($x, CURLOPT_URL, "http://www.baidu.com/");
curl_setopt($x, CURLOPT_RETURNTRANSFER, 1);
curl_exec($x);

yy_prof_disable(YY_PROF_FLAG_FORCE_END);

?>
--EXPECTF--
array (
  '%scli.php' => 
  array (
    'request_time' => %d,
    'request_count' => 1,
    'url_count' => 0,
    'url_time' => 0,
    'sql_count' => 0,
    'sql_time' => 0,
    'queue_count' => 0,
    'queue_time' => 0,
    'cache_count' => 0,
    'cache_time' => 0,
    'mongodb_count' => 0,
    'mongodb_time' => 0,
    'default_count' => 0,
    'default_time' => 0,
  ),
  '%s#consumer' => 
  array (
    'request_time' => %d,
    'request_count' => 10,
    'url_count' => 0,
    'url_time' => 0,
    'sql_count' => 0,
    'sql_time' => 0,
    'queue_count' => 0,
    'queue_time' => 0,
    'cache_count' => 0,
    'cache_time' => 0,
    'mongodb_count' => 0,
    'mongodb_time' => 0,
    'default_count' => 0,
    'default_time' => %d,
  ),
)
array (
  'FUNC:a' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
  ),
  'FUNC:b' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
  ),
  'FUNC:c' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
  ),
)
memory is ok
