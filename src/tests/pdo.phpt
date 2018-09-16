--TEST--
Check pdo
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
--EXTENSIONS--
mysqlnd
pdo
pdo_mysql
--FILE--
<?php 

yy_prof_clear_stats();
$begin = microtime(true);
ini_set("yy_prof.slow_run_time", 1);

$pdo = new PDO("mysql:host=127.0.0.1;dbname=yy_prof;charset=utf8", "yy_prof", "YY_prof@123");


$pdo->setAttribute(PDO::ATTR_AUTOCOMMIT, 0);
$pdo->beginTransaction();
$pdo->query("select SQL_NO_CACHE * from test limit 10");
$pdo->exec("select SQL_NO_CACHE * from test limit 10");
$pdo->exec("select SQL_NO_CACHE * from yy_prof.test limit 10");
$pdo->exec("select SQL_NO_CACHE * from `test` limit 10");
$pdo->exec("select SQL_NO_CACHE * from `yy_prof`.`test` limit 10");
$pdo->query("select sleep (1)");
$pdo->commit();

$pdo = new PDO("mysql:host=127.0.0.1;dbname=yy_prof;charset=utf8", "yy_prof", "YY_prof@123");
$pdo->setAttribute(PDO::ATTR_AUTOCOMMIT, 0);
$pdo->beginTransaction();
$pdo->query("select SQL_NO_CACHE * from test limit 10");
$pdo->rollBack();

$query = "select SQL_NO_CACHE * from test limit 10";
$stmt = $pdo->prepare($query);
$stmt->execute();

$query = "update test set id = 1 where id = -100";
$stmt = $pdo->prepare($query);
$stmt->execute();

$end = microtime(true);
echo $end - $begin, "\n";

$r = yy_prof_get_all_func_stat();

ksort($r);
var_export($r);
?>
--EXPECTF--
%f
array (
  'PDO::__construct' => 
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
  'PDO::beginTransaction' => 
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
  'PDO::commit' => 
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
  'PDO::rollBack' => 
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
    'count' => 8,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 8,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'SQL:YY_PROF.TEST' => 
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
)
