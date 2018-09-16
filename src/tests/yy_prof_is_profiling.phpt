--TEST--
Check yy_prof_is_profiling
--SKIPIF--
<?php if (!extension_loaded("yy_prof")) print "skip"; ?>
--INI--
yy_prof.auto_enable=0
--FILE--
<?php 

var_export(yy_prof_is_profiling()); echo "\n";

yy_prof_enable();
var_export(yy_prof_is_profiling()); echo "\n";

yy_prof_enable();
var_export(yy_prof_is_profiling()); echo "\n";

yy_prof_enable();
var_export(yy_prof_is_profiling()); echo "\n";
yy_prof_disable();
var_export(yy_prof_is_profiling()); echo "\n";

?>
--EXPECT--
false
true
true
true
false
