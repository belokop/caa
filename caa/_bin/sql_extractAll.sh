#! /bin/bash

export PATH=/usr/pkg/php/pro/bin:$PATH
pwd=$1

cd `dirname $0`
src=`pwd -P`
for d in `ls -1d ../*/sql`; do
    dd=`dirname $d`
    dd=`basename $dd`
    [ $dd = _api ] && continue;
    [ $dd = legacy ] && continue;
    echo "-$dd-------------------------------------------------------------- $d"
    cd $src/$d; cd `pwd -P`;
    php -n $src/sql_extract.php $pwd 2>&1 || exit 1
    svn diff | grep -E '^[-+]' | grep -vE 'Dump completed|revision|working copy|AUTO_INCREMENT|ENGINE'
done

