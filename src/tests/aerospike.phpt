--TEST--
Check aerospike
--SKIPIF--
<?php if (!extension_loaded("aerospike")) print "skip"; ?>
--EXTENSIONS--
aerospike
--INI--
yy_prof.auto_enable=1
yy_prof.enable_trace=1
yy_prof.enable_trace_cli=0
--FILE--
<?php 
yy_prof_clear_stats();

$config = ["hosts" => [["addr"=>"127.0.0.1", "port"=>3000]], "shm"=>[]];
$client = new Aerospike($config, true);
if (!$client->isConnected()) {
   echo "Aerospike failed to connect[{$client->errorno()}]: {$client->error()}\n";
   exit(1);
}

$key = $client->initKey("test", "yy_prof", 1234);
$bins = ["email" => "hey@example.com", "name" => "Hey There"];
// attempt to 'CREATE' a new record at the specified key
$status = $client->put($key, $bins, 0, [Aerospike::OPT_POLICY_EXISTS => Aerospike::POLICY_EXISTS_CREATE]);
if ($status == Aerospike::OK) {
    echo "Record written.\n";
} elseif ($status == Aerospike::ERR_RECORD_EXISTS) {
    echo "The Aerospike server already has a record with the given key.\n";
} else {
    echo "[{$client->errorno()}] ".$client->error();
}

// check for the existance of the given key in the database, then fetch it
if ($client->exists($key, $foo) == Aerospike::OK) {
    $status = $client->get($key, $record);
    if ($status == Aerospike::OK) {
       var_export($record);
    }
}
echo "\n";

// filtering for specific keys
$status = $client->get($key, $record, ["email"], array(Aerospike::POLICY_RETRY_ONCE => 1));
if ($status == Aerospike::OK) {
    echo "The email for this user is ". $record['bins']['email']. "\n";
} else {
    echo "Cann't find the record\n";
}

$client->remove($key);

$r = yy_prof_get_all_func_stat();

var_export($r);

?>
--EXPECTF--
Record written.
array (
  'key' => 
  array (
    'ns' => 'test',
    'set' => 'yy_prof',
    'key' => NULL,
    'digest' => '%s',
  ),
  'metadata' => 
  array (
    'ttl' => %d,
    'generation' => 1,
  ),
  'bins' => 
  array (
    'email' => 'hey@example.com',
    'name' => 'Hey There',
  ),
)
The email for this user is hey@example.com
array (
  'Aerospike::remove' => 
  array (
    'type' => 5,
    'count' => 1,
    'time' => %d,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
  'Aerospike::exists' => 
  array (
    'type' => 5,
    'count' => 1,
    'time' => %f,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
  'Aerospike::put' => 
  array (
    'type' => 5,
    'count' => 1,
    'time' => %f,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
  'Aerospike::get' => 
  array (
    'type' => 5,
    'count' => 2,
    'time' => %f,
    'request_bytes' => 0,
    'response_bytes' => 0,
    'status_200' => 0,
    'status_404' => 0,
    'status_500' => 0,
    'status_502' => 0,
    'status_503' => 0,
  ),
)
