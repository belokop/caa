#! /bin/bash
#
# Add the newly created photos to the Git repository
#

set -e
tmp=/tmp/delete_me_add_photos.sh
rm -rf $tmp

cd `dirname $0` && cd ../  && pwd -P
git status | grep photos/ | grep '@' | sed 's/.*photos/photos/' | sed "s/ .*//" | awk '{print "git add "$1}' > $tmp

[ -s $tmp ] && {
#    . $tmp
    echo =================================================
    echo =================================================
    cat $tmp
    echo =================================================
    echo =================================================
}
