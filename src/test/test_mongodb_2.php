<?php
$filter = array(
    "tag" => "mongodb",
    "views" => array('$gt' => 5),
);
$options = array(
    "projection" => array(
        "title" => 1,
        "article" => 1,
    ),
    "sort" => array(
        "views" => -1,
    ),
);
$readPreference = new MongoDB\Driver\ReadPreference(MongoDB\Driver\ReadPreference::RP_PRIMARY);
$query = new MongoDB\Driver\Query($filter, $options);

$manager = new MongoDB\Driver\Manager("mongodb://10.0.11.226:27017");
var_export($manager);
echo "\n";
$cursor = $manager->executeQuery("test.produce", $query, $readPreference);
var_export($cursor);

foreach($cursor as $document) {
    echo $document["title"], "\n";
}
