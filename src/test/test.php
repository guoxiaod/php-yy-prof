<?php

yy_prof_clear_stats();
error_reporting(E_ALL|E_NOTICE);
yy_prof_set_funcs(
    array(
        'hello_world',
        'gogogo',
        'thanks',
    ));

ini_set("yy_prof.slow_run_time", 0);
yy_prof_enable();

$x = yy_prof_get_funcs();
var_export($x);
echo "\n";
$b = microtime(true);

function gogogo() {
    usleep(10000);
}
function hello_world() {
    gogogo();
}

function thanks() {
    hello_world();
}

echo memory_get_usage(), "\n";
for($i = 0; $i < 10; $i++) {
    thanks();
    echo memory_get_usage(), "\n";
}
echo memory_get_usage(), "\n";

echo "\n", microtime(true) - $b, "\n";


$r = yy_prof_get_all_func_stat();

var_export($r);

echo "\n";
