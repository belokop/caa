#! /bin/bash

js=`echo $1|sed s/.js$//`
jc=`echo $js|sed s/\.min$//`.pack
[ -s $js.js ] && {
    java -jar `dirname $0`/compiler.jar --js $js.js --js_output_file $jc.js || exit 1
    ls -l $jc.js  $js.js
} || {
    echo "??? missin $js.js"
    exit 1
}
