#! /bin/bash
# Usage:
#   pnmScale.sh <input image file> <output image file> [mimeType (auto-detected by default)] [height (default 125)]
#

set -e 

PATH=/usr/local/bin:/opt/local/bin:/sw/bin:$PATH
export PATH

which identify > /dev/null 2>&1 || {
    echo "bin/identify does not exists, can't import pictures"
    echo "Please install ImageMagick from http://www.imagemagick.org/"
    exit 1
}

which pamscale > /dev/null 2>&1 || {
    echo "bin/pamscale does not exists, can't import pictures"
    echo "Please install Netpbm from http://netpbm.sourceforge.net/"
    exit 1
}

function getSize() {
    # photos/yb.png[47] PNG 188x250 DirectClass 103kb 0.010u 0:01
    imageH=`identify $1 2>/dev/null|awk '{print $3}'|awk -Fx '{print $2}'`
    imageW=`identify $1 2>/dev/null|awk '{print $3}'|awk -Fx '{print $1}'`
    mimeType=`identify $1 2>/dev/null|awk '{print $2}'|tr A-Z a-z`
}

from=$1
destfile=$2
height=125; [ -n "$4" ] && height=$4

getSize $from

#echo imageH=$imageH
#echo imageW=$imageW
#echo mimeType=$mimeType

[ -z "$destfile" ] && {
    if [ -f "$1" ]; then
	echo $mimeType
	echo HxW ${imageH}x${imageW} >&2
	exit 0
    fi
    if [ -z "$1" ]; then    echo Usage: `basename $0` from to '[mimeType]' '[height]' >&2
    else    echo Wrong usage: `basename $0` \"$1\" \"$2\" \"$3\" >&2
    fi
    exit 0
}

# Make sure that the destination directory exists
destDir=`dirname $destfile`
[ -d "`dirname $destDir`" ] && mkdir -p $destDir

# Just copy the image to the destination if its size is too small,
# otherwise rescale it
if   [ $imageH -le $height -a $imageW -le $height ]; then
    cp -pf $from $destfile
    chmod a+r $destfile
    exit 0
elif [ $imageH -le $imageW ]; then
    height=`expr $imageH \* $height / $imageW`
fi

case $mimeType in
    gif)
	giftopnm  "$from" | pamscale -height=$height | pnmtojpeg > "$destfile"
	;;
    
    png)
	pngtopnm  "$from" | pamscale -height=$height | pnmtojpeg > "$destfile"
	;;
    
    jpg|jpeg)
	jpegtopnm "$from" | pamscale -height=$height | pnmtojpeg > "$destfile"
	;;
    
    *)
	echo "<h3><font color=red>unknown file extension \"$mimeType\"</font></h3>"
	exit 1
	;;
esac
chmod a+r $destfile
exit 0
