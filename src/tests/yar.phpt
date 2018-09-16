--TEST--
Check yar
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
--FILE--
<?php 

yy_prof_clear_stats();

ini_set("yy_prof.slow_run_time", 1);

$x = array(
    'funcs' => array(
        'Yar_Client::get'
    )
);

//yy_prof_enable($x);

function h() {
    $yar = new Yar_Client("https://www.baidu.com/");
    try {
        $t = $yar->get(array("user_id" => 1, "keys" => "user_type"));
    } catch (Exception $e) {}
}


h();

$r = yy_prof_get_all_func_stat();

var_export($r);

?>
--EXPECTF--
array (
  'https://%s/get' => 
  array (
    'type' => 1,
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
