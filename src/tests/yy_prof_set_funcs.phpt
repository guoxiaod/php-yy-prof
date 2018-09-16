--TEST--
Check yy_prof_set_funcs
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--FILE--
<?php 

$funcs = array(
    'a',
    'b',
    'c',
    'de',
    'fg_123',
);
yy_prof_set_funcs($funcs);
$y = yy_prof_get_funcs();
var_export($y); echo "\n";

$funcs = array( 'curl_exec',);
yy_prof_set_funcs($funcs);
$y = yy_prof_get_funcs();
var_export($y); echo "\n";

$funcs = array(
        'a',
        'curl_exec',
        'b',
    );
yy_prof_set_funcs($funcs);
$y = yy_prof_get_funcs();
var_export($y); echo "\n";


yy_prof_reset_funcs();
$origin = yy_prof_get_funcs();
yy_prof_set_funcs($funcs, YY_PROF_FLAG_APPEND);
$y = yy_prof_get_funcs();
var_export(count($y) - count($origin)); echo "\n";
var_export(array_diff($origin, $y)); echo "\n";
var_export(array_diff($y, $origin));

?>
--EXPECTF--
array (
  0 => 'a',
  1 => 'b',
  2 => 'c',
  3 => 'de',
  4 => 'fg_123',
)
array (
  0 => 'curl_exec',
)
array (
  0 => 'a',
  1 => 'b',
  2 => 'curl_exec',
)
3
array (
)
array (
  %d => 'a',
  %d => 'b',
)
