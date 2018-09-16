<?php


$begin = microtime(true);
yy_prof_clear_stats();
$x = array("a", "b", "c", "d", "e", "f", "g");

function a() {
    //usleep(1);
}

function b() {
    //usleep(1);
}

function c() {
    //usleep(1);
}

yy_prof_enable();
yy_prof_set_funcs($x);
$begin = microtime(true);
for($i = 0; $i < 10000; $i ++) {
    a();
    b();
    c();
    a();
    b();
    c();
    a();
    b();
    c();
    a();
    b();
    c();
    a();
    b();
    c();
    a();
    b();
    c();
    a();
    b();
    c();
}
$end = microtime(true);
yy_prof_disable();

echo $end - $begin , "\n";
