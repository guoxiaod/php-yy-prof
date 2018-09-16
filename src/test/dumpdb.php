<?php


function dumpdb($file) {
    $db = new LevelDB($file);
    $it = new LevelDBIterator($db);

    // Loop in iterator style
    /*
    while($it->valid()) {
        var_dump($it->key() . " => " . $it->current() . "\n");

        $it->next();
    }
    */

    // Or loop in foreach
    echo "=======$file========\n";
    foreach($it as $key => $value) {
        echo " '{$key}' => '{$value}'\n";
    }
    echo "=======$file========\n\n";

    $db->close();
}

dumpdb("/usr/var/cache/yy_prof/func_stat.7.db/");
dumpdb("/usr/var/cache/yy_prof/page_stat.7.db/");
dumpdb("/usr/var/cache/yy_prof/trace.7.db/");
