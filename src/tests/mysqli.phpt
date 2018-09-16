--TEST--
Check mysqli
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--EXTENSIONS--
mysqlnd
mysqli
pdo
pdo_mysql
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
        'mysqli::prepare',
        'mysqli_stmt::execute',
        'mysqli::query',
        'mysqli::real_query',
    )
);

$begin = microtime(true);
ini_set("yy_prof.slow_run_time", 1);
//yy_prof_enable($x);

$mysqli = new mysqli("127.0.0.1", "yy_prof", "YY_prof@123", "yy_prof");

/* check connection */
if (mysqli_connect_errno()) {
    printf("Connect failed: %s\n", mysqli_connect_error());
    exit();
}

$mysqli->query("select SQL_NO_CACHE * from test limit 10");
$mysqli->query("select SQL_NO_CACHE * from test limit 10");
$mysqli->query("select SQL_NO_CACHE * from yy_prof.test limit 10");
$mysqli->query("select SQL_NO_CACHE * from `test` limit 10");
$mysqli->query("select SQL_NO_CACHE * from `yy_prof`.`test` limit 10");
$mysqli->query("select sleep (1)");

$mysqli->autocommit(false);
$query = "select SQL_NO_CACHE * from test limit 10";
$stmt = $mysqli->prepare($query);
$stmt->execute();
$mysqli->commit();
$mysqli->autocommit(true);
$mysqli->rollback();
$stmt->close();

$query = "update test set id = 1 where id = -100";
$stmt = $mysqli->prepare($query);
$stmt->execute();

$end = microtime(true);

$i = 88;
//$stmt->bind_param("d", $i);
echo $end - $begin, "\n";

$r = yy_prof_get_all_func_stat();

ksort($r);
var_export($r);
?>
--EXPECTF--
%f
array (
  'SQL::SELECT' => 
  array (
    'type' => 2,
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
  'SQL:TEST' => 
  array (
    'type' => 2,
    'count' => 7,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 7,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'SQL:YY_PROF.TEST' => 
  array (
    'type' => 2,
    'count' => 2,
    'time' => %f,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 2,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'mysqli::__construct' => 
  array (
    'type' => 2,
    'count' => 1,
    'time' => %f,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 1,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'mysqli::autocommit' => 
  array (
    'type' => 2,
    'count' => 2,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 2,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'mysqli::commit' => 
  array (
    'type' => 2,
    'count' => 1,
    'time' => %f,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 1,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'mysqli::rollback' => 
  array (
    'type' => 2,
    'count' => 1,
    'time' => %f,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 1,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
)
