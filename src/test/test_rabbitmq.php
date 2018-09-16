<?php



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

$exchange->setName("yy_prof");
$exchange->setType(AMQP_EX_TYPE_DIRECT);
$exchange->setFlags(AMQP_DURABLE);
$exchange->declareExchange();

$q = new AMQPQueue($channel);
$q->setName("yy_prof");
$q->setFlags(AMQP_DURABLE);
$q->declareQueue();

$q->bind("yy_prof", "yy_prof");

$opts = array(
    'delivery_mode' => 2
);
$exchange->publish("yy_prof", "yy_prof", AMQP_NOPARAM, $opts);
