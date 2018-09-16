--TEST--
Check curl_multi
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--INI--
yy_prof.auto_enable=1
yy_prof.enable_trace_cli=0
--EXTENSIONS--
curl
--FILE--
<?php 

yy_prof_clear_stats();
$aURLs = array(
    "https://www.baidu.com",
    "https://www.sohu.com"
); // array of URLs
$mh = curl_multi_init(); // init the curl Multi

$aCurlHandles = array(); // create an array for the individual curl handles

foreach ($aURLs as $id=>$url) { //add the handles for each url
    //$ch = curl_setup($url,$socks5_proxy,$usernamepass);
    $ch = curl_init(); // init curl, and then setup your options
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER,1); // returns the result - very important
    curl_setopt($ch, CURLOPT_HEADER, 0); // no headers in the output

    $aCurlHandles[$url] = $ch;
    curl_multi_add_handle($mh,$ch);
}

$active = null;
//execute the handles
do {
    $mrc = curl_multi_exec($mh, $active);
} 
while ($mrc == CURLM_CALL_MULTI_PERFORM);

while ($active && $mrc == CURLM_OK) {
    if (curl_multi_select($mh) != -1) {
        usleep(1);
    }
    do {
        $mrc = curl_multi_exec($mh, $active);
    } while ($mrc == CURLM_CALL_MULTI_PERFORM);
}

/* This is the relevant bit */
// iterate through the handles and get your content
foreach ($aCurlHandles as $url=>$ch) {
    $html = curl_multi_getcontent($ch); // get the content
    // do what you want with the HTML
    curl_multi_remove_handle($mh, $ch); // remove the handle (assuming  you are done with it);
}
/* End of the relevant bit */

curl_multi_close($mh); // close the curl multi handler


$r = yy_prof_get_all_func_stat();

var_export($r);
?>
--EXPECTF--
array (
  'https://www.sohu.com/' => 
  array (
    'type' => 1,
    'count' => 1,
    'time' => %d,
    'request_bytes' => %d,
    'response_bytes' => %d,
    'status_200' => %d,
    'status_300' => %d,
    'status_400' => %d,
    'status_500' => %d,
    'status_501' => %d,
  ),
  'https://www.baidu.com/' => 
  array (
    'type' => 1,
    'count' => 1,
    'time' => %d,
    'request_bytes' => %d,
    'response_bytes' => %d,
    'status_200' => %d,
    'status_300' => %d,
    'status_400' => %d,
    'status_500' => %d,
    'status_501' => %d,
  ),
)
