#! /bin/bash
f=$1
e=/Users/yb/w/modules/trunk/_bin/fix_members.pl

$e < $f > /tmp/c
diff -u /tmp/c $f
sdiff -s -w`stty size|awk '{print $2}'` $f  /tmp/c && exit 0
echo "mv $f $f.orig; mv /tmp/c $f"
# mv -v $f $f.orig; mv -v /tmp/c $f
