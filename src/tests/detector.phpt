--TEST--
Check request detector
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
<?php if (1) print "skip"; ?>
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
yy_prof.enable_request_detector=1
--FILE--
<?php 

yy_prof_clear_stats();

function test_1() {
    test_2();
}

function test_2() {
    test_3();
}

function test_3() {
    test_4();
}

function test_4() {
    usleep(1000);
}

$callback = null;

function deal_message($uri, $data) {
    test_1();
    test_4();
}

yy_prof_set_funcs(array("test_1", "test_2", "test_3", "test_4"));

for($i = 0; $i < 10; $i ++) {
    $callback("a", "b");
}

yy_prof_disable();

$r = yy_prof_get_all_func_stat();

var_export($r);
?>
--EXPECTF--
array (
  'FUNC:test_1' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
  'FUNC:test_2' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
  'FUNC:test_3' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
  'FUNC:test_4' => 
  array (
    'type' => 0,
    'count' => 20,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
)
