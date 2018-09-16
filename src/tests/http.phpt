--TEST--
Check http
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--EXTENSIONS--
iconv
propro
raphf
apfd
http
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
--FILE--
<?php 

yy_prof_clear_stats();

$request = new http\Client\Request("GET",
    "https://www.baidu.com/xxx",
    ["User-Agent"=>"My Client/0.1"]
);
$request->setOptions(["timeout"=>1]);

$request2 = new http\Client\Request("GET",
    "https://www.baidu.com/zzz",
    ["User-Agent"=>"My Client/0.1"]
);
$request->setOptions(["timeout"=>1]);

$request3 = new http\Client\Request("GET",
    "https://www.baidu.com/yyy",
    ["User-Agent"=>"My Client/0.1"]
);
$request->setOptions(["timeout"=>1]);

$client = new http\Client;
$client->enqueue($request);
$client->enqueue($request2);
$client->enqueue($request3);

$client->send();

$client->getResponse($request);

// pop the last retrieved response
$str = array();
while ($response = $client->getResponse()) {
    $str[] = sprintf("%s returned '%s' (%d)\n",
            $response->getTransferInfo("effective_url"),
            $response->getInfo(),
            $response->getResponseCode()
          );
}
sort($str);
foreach($str as $v) {
    echo $v;
}


$r = yy_prof_get_all_func_stat();
ksort($r);
var_export($r);

?>
--EXPECTF--
https://www.baidu.com/xxx returned 'HTTP/1.1 %d Found' (%d)
https://www.baidu.com/yyy returned 'HTTP/1.1 %d Found' (%d)
https://www.baidu.com/zzz returned 'HTTP/1.1 %d Found' (%d)
array (
  'http\\Client::getResponse' => 
  array (
    'type' => 1,
    'count' => 1,
    'time' => 0,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 1,
    'status_300' => 0,
    'status_400' => 0,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'https://www.baidu.com/xxx' => 
  array (
    'type' => 1,
    'count' => 2,
    'time' => %d,
    'request_bytes' => %d,
    'response_bytes' => %d,
    'status_200' => %d,
    'status_300' => %d,
    'status_400' => %d,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'https://www.baidu.com/yyy' => 
  array (
    'type' => 1,
    'count' => 1,
    'time' => %d,
    'request_bytes' => %d,
    'response_bytes' => %d,
    'status_200' => %d,
    'status_300' => %d,
    'status_400' => %d,
    'status_500' => 0,
    'status_501' => 0,
  ),
  'https://www.baidu.com/zzz' => 
  array (
    'type' => 1,
    'count' => 1,
    'time' => %d,
    'request_bytes' => %d,
    'response_bytes' => %d,
    'status_200' => %d,
    'status_300' => %d,
    'status_400' => %d,
    'status_500' => 0,
    'status_501' => 0,
  ),
)
