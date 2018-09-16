--TEST--
Check amqp
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--EXTENSIONS--
amqp
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
--FILE--
<?php 

yy_prof_clear_stats();

$opts = array(
    'host' => '127.0.0.1',
    'port' => '5672',
    'login' => 'yy_prof',
    'password' => 'yy_prof',
    'vhost' => '/yy_prof',
);
$c = new AMQPConnection($opts);
$c->connect();

$channel = new AMQPChannel($c);

$exchange = new AMQPExchange($channel);

$exchange->setName("test_user");
$exchange->setType(AMQP_EX_TYPE_DIRECT);
$exchange->setFlags(AMQP_DURABLE);
$exchange->declareExchange();

$q = new AMQPQueue($channel);
$q->setName("test_user");
$q->setFlags(AMQP_DURABLE);
$q->declareQueue();

$q->bind("test_user", "test_user");

$opts = array(
    'delivery_mode' => 2
);
$exchange->publish("test_user", "test_user", AMQP_NOPARAM, $opts);


$r = yy_prof_get_all_func_stat();

var_export($r);

?>
--EXPECTF--
array (
  'AMQPExchange::publish' => 
  array (
    'type' => 3,
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
  'AMQPConnection::connect' => 
  array (
    'type' => 3,
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
