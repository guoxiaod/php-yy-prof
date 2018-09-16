--TEST--
Check redis
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--EXTENSIONS--
igbinary
redis
posix
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
--FILE--
<?php 
yy_prof_clear_stats();

$x = array(
    'funcs' => array(
        'Redis::'
    )
);
// yy_prof_enable($x);

system("/usr/bin/redis-server --port 6380 --pidfile /tmp/redis.6380.pid --daemonize yes --appendfilename '' >> /dev/null 2>&1");

$redis = new Redis();
$ret = false;
while (!$ret) {
    usleep(500000);
    try {
        $ret = $redis->connect("127.0.0.1", 6380);
    } catch (Exception $e) {}
}

$redis->select(1);

$redis->set("user", "123");

$v = $redis->get("user");

echo $v, "\n";

$pid = (int) file_get_contents("/tmp/redis.6380.pid");
while (posix_kill($pid, 0)) {
    posix_kill($pid, SIGKILL);
    usleep(10000);
}
unlink("/tmp/redis.6380.pid");

$r = yy_prof_get_all_func_stat();
ksort($r);
var_export($r);
?>
--EXPECTF--
123
array (
  'Redis::connect' => 
  array (
    'type' => 4,
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
  'Redis::get' => 
  array (
    'type' => 4,
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
  'Redis::select' => 
  array (
    'type' => 4,
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
  'Redis::set' => 
  array (
    'type' => 4,
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
