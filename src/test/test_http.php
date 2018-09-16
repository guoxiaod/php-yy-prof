<?php

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
while ($response = $client->getResponse()) {
    printf("%s returned '%s' (%d)\n",
            $response->getTransferInfo("effective_url"),
            $response->getInfo(),
            $response->getResponseCode()
          );
}

