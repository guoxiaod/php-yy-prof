<?php


/*
$x = array(
    'funcs' => array(
        'MongoDB::Command',
        'MongoCollection::find',
        'MongoCollection::findOne',
        'MongoCollection::update',
        'MongoCollection::insert',
        'MongoCollection::save',
    )
);

yy_prof_enable($x);
*/


$c = 'mongodb://10.0.11.226';
$m = new mongo($c);
$db = $m->selectdb('test');
$collection = new mongocollection($db, 'produce');

// 搜索水果
$fruitquery = array('type' => 'fruit');

$cursor = $collection->find($fruitquery);
var_export($cursor);
foreach ($cursor as $doc) {
    var_dump($doc);
}

// 搜索甜的产品 taste is a child of details. 
$sweetquery = array('details.taste' => 'sweet');
$cursor = $collection->find($sweetquery);
var_export($cursor);
foreach ($cursor as $doc) {
    var_dump($doc);
}

