#! /bin/bash
f=$1

e=`cd \`dirname $0\`; pwd -P`/fix_ereg.pl

$e < $f > /tmp/c
# diff -u /tmp/c $f
sdiff -s -w`stty size|awk '{print $2}'` $f  /tmp/c && exit 0
echo "mv $f $f.orig; mv /tmp/c $f"
#mv -v $f $f.orig; mv -v /tmp/c $f
