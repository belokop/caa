#! /bin/bash
cd `dirname $0`
src=`pwd -P`
for d in myPear $2; do
    echo "-$0----------------------------------- $d"
    cd $src/../$d || exit 1
    $src/sql_install.sh $1
done

