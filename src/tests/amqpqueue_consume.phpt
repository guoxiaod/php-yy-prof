--TEST--
Check amqpqueue consume
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--EXTENSIONS--
amqp
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
yy_prof.enable_request_detector=1
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
$exchange->publish("test_user", "test_user", AMQP_NOPARAM, $opts);
$exchange->publish("test_user", "test_user", AMQP_NOPARAM, $opts);

function just_make_a_test() {
    // do nothing
    return 1;
}

yy_prof_set_funcs(array("just_make_a_test"), YY_PROF_FLAG_APPEND);

$count = 0;
function test_consume($envelope, $queue) {
    global $count;
    var_export($queue->getName()); echo "\n";
    just_make_a_test();
    if(++$count >= 3) {
        return false;
    }
}

$q->consume("test_consume");

$r = yy_prof_get_all_func_stat();
var_export($r); echo "\n";

$r = yy_prof_get_all_page_stat();
var_export($r); echo "\n";

$r = yy_prof_get_page_func_stat("AMQPQueue::consume::test_user");
var_export($r); echo "\n";

?>
--EXPECTF--
'test_user'
'test_user'
'test_user'
array (
  'AMQPExchange::publish' => 
  array (
    'type' => 3,
    'count' => 3,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 3,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'FUNC:just_make_a_test' => 
  array (
    'type' => 0,
    'count' => 3,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 3,
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
array (
  'AMQPQueue::consume::test_user' => 
  array (
    'request_time' => %d,
    'request_count' => 3,
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
  '%s/tests/amqpqueue_consume.php' => 
  array (
    'request_time' => %d,
    'request_count' => 1,
    'url_count' => 0,
    'url_time' => 0,
    'sql_count' => 0,
    'sql_time' => 0,
    'queue_count' => 4,
    'queue_time' => %d,
    'cache_count' => 0,
    'cache_time' => 0,
    'mongodb_count' => 0,
    'mongodb_time' => 0,
    'default_count' => 0,
    'default_time' => 0,
  ),
)
array (
  'FUNC:just_make_a_test' => 
  array (
    'type' => 0,
    'count' => 3,
    'time' => %d,
  ),
)
