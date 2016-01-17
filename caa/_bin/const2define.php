<?php

$file = $argv[1];
foreach(explode("\n",file_get_contents($file)) as $line){
  if (strpos($line,'const ') === 0){
    $nl = preg_replace('/const *(\w*)([\s ]*)?[=](\s*)?(.*)(;.*)$/','define(\'\\1\'\\2,\\3\\4)\\5',trim($line));
    print "$nl\n";
  }else{
    print "$line\n";
  }
}
