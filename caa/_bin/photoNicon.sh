#! /bin/bash

PATH=`pwd`:`pwd`/bin:`dirname $0`:$PATH
export PATH

from=$1
to=$2
sizePhoto=$3
sizeIcon=$4

iconf=`dirname $to`/icons/`basename $to`
pnmScale.sh "$from" "$to"                                "" $sizePhoto
pnmScale.sh "$from" "`dirname $to`/icons/`basename $to`" "" $sizeIcon


