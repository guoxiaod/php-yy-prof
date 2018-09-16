--TEST--
Check swoole_http_server
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
<?php if (!extension_loaded("swoole")) print "skip"; ?>
--EXTENSIONS--
sockets
swoole
--INI--
yy_prof.auto_enable=1
yy_prof.enable_cli=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
yy_prof.enable_request_detector=1
--FILE--
<?php 

yy_prof_clear_stats();

if (($pid = pcntl_fork()) == 0) {
    $serv = new Swoole\Http\Server("127.0.0.1", 9502);
    function swoole_http_server_on_request($request, $response) {
        $response->cookie("User", "Swoole");
        $response->header("X-Server", "Swoole");
        $response->end("<h1>Hello Swoole!</h1>");
    }
    $serv->on('Request', "swoole_http_server_on_request");

    $serv->start();
    return;
}

$res = false;
while(!$res) {
    $errno = 0;
    $errstr = "";
    $res = @fsockopen("127.0.0.1", 9502, $errno, $errstr, 0.01);
    if($res) {
        fclose($res);
        break;
    }
}
usleep(500 * 1000);

$cli = new Swoole\Http\Client('127.0.0.1', 9502);
$cli->setHeaders(array('User-Agent' => 'swoole-http-client'));
$cli->setCookies(array('test' => 'value'));

$cli->post('/dump.php', array("test" => 'abc'), function ($cli) {
    var_dump($cli->body);
    $cli->get('/index.php', function ($cli) {
        var_export($cli->cookies);echo "\n";
        var_export($cli->headers);echo "\n";
        $cli->close();
    });
});

$cli->on("close", function() use($pid) {
    exec("kill $pid");
    $status = 0;
    while (pcntl_waitpid($pid, $status, WNOHANG) == 0) { }

    $r = yy_prof_get_all_func_stat();
    var_export($r); echo "\n";

    $r = yy_prof_get_all_page_stat();
    var_export($r); echo "\n";
});

?>
--EXPECTF--
string(22) "<h1>Hello Swoole!</h1>"
array (
  'test' => 'value',
  'User' => 'Swoole',
)
array (
  'x-server' => 'Swoole',
  'server' => 'swoole-http-server',
  'connection' => 'keep-alive',
  'content-length' => '22',
  'content-type' => 'text/html',
  'date' => '%s',
  'set-cookie' => 'User=Swoole',
)
[%s]	NOTICE	Server is shutdown now.
array (
)
array (
  'http://127.0.0.1/index.php' => 
  array (
    'request_time' => %d,
    'request_count' => 1,
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
    'default_time' => 0,
  ),
  '%s/tests/swoole_http_server.php' => 
  array (
    'request_time' => %d,
    'request_count' => 2,
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
    'default_time' => 0,
  ),
  'http://127.0.0.1/dump.php' => 
  array (
    'request_time' => %d,
    'request_count' => 1,
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
    'default_time' => 0,
  ),
)
