--TEST--
Check curl
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
yy_prof.slow_run_time=1
--EXTENSIONS--
curl
--FILE--
<?php 

$funcs = array("curl_exec");
yy_prof_set_funcs($funcs);
yy_prof_enable();
// 创建一个cURL资源

function test_curl() {
    $ch = curl_init();

    // 设置URL和相应的选项
    curl_setopt($ch, CURLOPT_URL, "https://www.baidu.com/");
    curl_setopt($ch, CURLOPT_HEADER, 0);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

    // 抓取URL并把它传递给浏览器
    curl_exec($ch);

    // 关闭cURL资源，并且释放系统资源
    curl_close($ch);
}

yy_prof_clear_stats();

test_curl();
test_curl();

yy_prof_disable();

$r = yy_prof_get_all_func_stat();

var_export($r);

?>
--EXPECTF--
array (
  'https://www.baidu.com/' => 
  array (
    'type' => 1,
    'count' => 2,
    'time' => %d,
    'request_bytes' => %d,
    'response_bytes' => %d,
    'status_200' => 2,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
)
