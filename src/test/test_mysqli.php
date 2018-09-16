<?php

$x = array(
    'funcs' => array(
        'mysqli::prepare',
        'mysqli_stmt::execute',
        'mysqli::query',
        'mysqli::real_query',
    )
);
$str = str_repeat("abcde12345", 100);

$begin = microtime(true);
ini_set("yy_prof.slow_run_time", 1);
//yy_prof_enable($x);

$mysqli = new mysqli("127.0.0.1", "yy_prof", "YY_prof@123", "yy_prof");

/* check connection */
if (mysqli_connect_errno()) {
    printf("Connect failed: %s\n", mysqli_connect_error());
    exit();
}

$mysqli->query("select SQL_NO_CACHE * from test where id = 2 /* $str */");
$mysqli->query("select SQL_NO_CACHE * from test  where id = 2");
$mysqli->query("select sleep (1)");

$query = "select SQL_NO_CACHE * from test  where id = 2 /* $str */";
$stmt = $mysqli->prepare($query);
$stmt->execute();
$stmt->close();

$query = "update test set name = 'abc' where id = -1 /* $str */";
$stmt = $mysqli->prepare($query);
$stmt->execute();

$end = microtime(true);

$i = 88;
//$stmt->bind_param("d", $i);


echo $end - $begin, "\n";
