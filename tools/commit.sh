#! /bin/bash
#set -e

dryrun=
dryrun=yes

echo Current tags:
git tag -l 
echo

# Go to the repositary root
src=$(cd `dirname $0`; pwd -P)
cd $src/..

echo Modified areas:
changes=$(git status | grep -E "(modified|new file):" | awk -F: '{print $2}' | awk -F/ '{print $1"/"$2}' | sort -u)
for area in $changes; do echo $area; done
echo

for area in $changes; do
    [ -f $src/../$area/config.inc ] || continue
    conf=$src/../$area/config.inc
    echo $conf

    # define('VM_VERSION','5.14');
    cur_v=$(grep -E "^define.*_VERSION" $conf)
    echo $cur_v
    fr=$(echo $cur_v | tr [\$\(\)\,\'\"] \.)
echo $fr
    # $releaseDate = '2019-12-15';
    date=$(date +%Y-%m-%d)
    cur_d=$(grep -E "^.releaseDate" $conf)
    fr=$(echo $cur_d | tr [\$\(\)\,\'\"] \.)
    echo $cur_d
    echo $cur_d|sed "s/$fr/\$releaseDate = '$date';/"
done
