<?php


yy_prof_clear_stats();
$x = array("a", "b", "c");


function a() {
    usleep(10000);
}

function b() {
    usleep(10000);
}

function c() {
    usleep(10000);
}

for($i = 0; $i < 4; $i ++) {
    echo "=======================\n";
    echo memory_get_usage(), "\n";
    yy_prof_enable();
    yy_prof_set_funcs($x);
    echo memory_get_usage(), "\n";
    a();
    echo memory_get_usage(), "\n";
    b();
    echo memory_get_usage(), "\n";
    c();
    echo memory_get_usage(), "\n";
    yy_prof_disable();
    echo memory_get_usage(), "\n";
}
