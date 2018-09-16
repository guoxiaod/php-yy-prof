<?php 

function just_make_a_test() {
    $config = ["hosts" => [["addr"=>"10.0.11.135", "port"=>3000]], "shm"=>[]];
    $client = new Aerospike($config, true);
    if (!$client->isConnected()) {
        echo "Aerospike failed to connect[{$client->errorno()}]: {$client->error()}\n";
        exit(1);
    }

    $key = $client->initKey("ya_cache", "ya_cache_users", 1234);
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
            var_export($record['bins']);
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
}

for($i = 0; $i < 10; $i ++) {
    just_make_a_test();
}


