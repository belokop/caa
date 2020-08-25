#! /bin/bash

set -e
set +o posix

v_desired=v5.16

echo Currently tagged versions:
git tag -l | tr -d [^v]
echo
[ -z "$v_desired" ] && {
    read -r -p "... What is the desired version number? " v_desired
    if [[ "$v_desired" =~ ^[1-9]+.[0-9]+ ]]; then
	v_desired=v$v_desired
    else
	echo ... Aborting, wrong version number '$v_desired'
	exit 1
    fi
}
echo Desired version $v_desired; echo

# Go to the repositary root
src=$(cd `dirname $0`; pwd -P)

echo Modified areas:
cd $src/..
changes=$(git status | grep -E "(modified|new file):" | awk -F: '{print $2}' | awk -F/ '{print $1"/"$2}' | sort -u)
for area in $changes; do echo $area; done
echo

for area in $changes; do
    [ -f $src/../$area/config.inc ] || continue
    conf=$src/../$area/config.inc
    echo $conf

    # $releaseDate = '2019-12-15';
    date=$(date +%Y-%m-%d)
    cur_d=$(grep -E "^.releaseDate" $conf)
    fr=$(echo $cur_d | tr [\$\(\)\,\'\"] \.)
    echo $cur_d
    echo $cur_d|sed "s/$fr/\$releaseDate = '$date';/"

    # define('VM_VERSION','5.14');
    cur_v=$(grep -E "^define.*_VERSION" $conf)
    module=$(echo $cur_v|sed -e s/_VERSION.*/_VERSION/ -e 's/^.*"//' -e "s,^.*',," ])
    echo module=$module
    echo $cur_v 
    fr=$(echo $cur_v | tr [\$\(\)\,\"] \. | sed "s/'/\./g")
    echo $cur_v|sed "s/$fr/define('$module','$v_desired');/"
done
