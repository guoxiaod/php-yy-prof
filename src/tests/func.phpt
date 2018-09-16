--TEST--
Check normal func
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
yy_prof.slow_run_time=0
--FILE--
<?php 
error_reporting(E_ALL|E_NOTICE);

yy_prof_clear_stats();

$funcs = array('gogogo', 'hello_world', 'thanks',);
yy_prof_set_funcs($funcs);
yy_prof_enable();

function gogogo() {
    usleep(10000);
}
function hello_world() {
    gogogo();
}

function thanks() {
    hello_world();
}

for($i = 0; $i < 10; $i++) {
    thanks();
}

yy_prof_disable();

$r = yy_prof_get_all_func_stat();
var_export($r);
?>
--EXPECTF--
array (
  'FUNC:gogogo' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 10,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'FUNC:hello_world' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 10,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'FUNC:thanks' => 
  array (
    'type' => 0,
    'count' => 10,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 10,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
)
