#! /bin/bash
f=$1
e=`echo $0|sed s/sh$/pl/`

perl $e < $f > /tmp/c
diff -u /tmp/c $f
sdiff -s -w`stty size|awk '{print $2}'` $f  /tmp/c && exit 0
echo "cp -pvf $f $f.orig; cp -pvf /tmp/c $f"
