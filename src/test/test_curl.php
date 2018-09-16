<?php

ini_set("yy_prof.slow_run_time", 1);
yy_prof_enable();
// 创建一个cURL资源

function test_curl($url) {
    $ch = curl_init();

    // 设置URL和相应的选项
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, "a=b&c=d");
    curl_setopt($ch, CURLOPT_HEADER, 0);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

    // 抓取URL并把它传递给浏览器
    curl_exec($ch);

    // 关闭cURL资源，并且释放系统资源
    curl_close($ch);
}


$url = "https://www.baidu.com/";
echo memory_get_usage(), "\n";
test_curl($url);
echo memory_get_usage(), "\n";
test_curl($url);
echo memory_get_usage(), "\n";
test_curl($url);
echo memory_get_usage(), "\n";
