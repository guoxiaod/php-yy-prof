--TEST--
Check too long stat key
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--EXTENSIONS--
msgpack
json
yar
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
yy_prof.slow_run_time=1
--FILE--
<?php 

yy_prof_clear_stats();


$func = 'consumer1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890';
$funcs = array("a", "b", "c");

function a() {
    usleep(100000);
}

function b() {
    usleep(100000);
}

function c() {
    usleep(100000);
}


for($i = 0; $i < 10; $i ++) {
    yy_prof_set_funcs($funcs);
    yy_prof_enable($func);
    a();
    b();
    c();
    yy_prof_disable();
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
ksort($r);
var_export($r);

?>
--EXPECTF--
array (
  '%s/tests/too_long_stat_key.php' => 
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
  '%s#consumer%s' => 
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
