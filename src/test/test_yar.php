<?php


ini_set("yy_prof.slow_run_time", 1);

$x = array(
    'funcs' => array(
        'Yar_Client::get'
    )
);

//yy_prof_enable($x);

function h() {
    $yar = new Yar_Client("https://www.baidu.com/");
    $t = $yar->get(array("user_id" => 1, "keys" => "user_type"));
    var_export($t);
}


h();

