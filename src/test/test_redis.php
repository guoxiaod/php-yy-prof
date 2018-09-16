<?php


/*
$x = array(
    'funcs' => array(
        'Redis::'
    )
);
*/
// yy_prof_enable($x);

$redis = new Redis();

$redis->connect("127.0.0.1");

$begin = microtime(true);
for($i = 0; $i < 1000; $i ++) {
$redis->select(1);
}
$end = microtime(true);

echo $end - $begin, "\n";

$redis->set("user", "123");

$v = $redis->get("user");

echo $v, "\n";
