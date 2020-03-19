#! /bin/bash

set -e
set +o posix

echo Currently tagged versions:
git tag -l | sort -n | tr -d [^v]

read -r -p "... What is the desired version number? " v_desired
if [[ "$v_desired" =~ ^[1-9]+.[0-9]+ ]]; then
    tag_desired=v$v_desired
else
    echo ... Aborting, wrong version number '$v_desired'
    exit 1
fi
echo; read -r -p "... Are you sure to set version='$v_desired' & git tag='$tag_desired' ? [y/N] " response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]; then echo; else echo ... Aborting; exit 1; fi

echo ========> Desired version $v_desired; echo

# Go to the repositary root
src=$(cd `dirname $0`; pwd -P)

echo Modified areas:
cd $src/..
changes=$(git status | grep -E "(modified|new file):" | awk -F: '{print $2}' | awk -F/ '{print $1"/"$2}' | sort -u)
for area in $changes; do echo $area; done
echo

for area in $changes; do
    [ -f $src/../$area/config.inc ] || continue
    conf=$(cd $src/../$area; pwd -P)/config.inc
    module=$(basename $area)
    echo
    echo ======================================================================================= $area
    echo 

    # $releaseDate = '2019-12-15';
    date=$(date +%Y-%m-%d)
    cur_d=$(grep -E "^.releaseDate" $conf)
    f1=`echo $cur_d | tr [\$\(\)\,\'\"\;\ ] \.`
    t1="\$releaseDate = \'$date\'\;"

    # define('VM_VERSION','5.14');
    cur_v=$(grep -E "^define.*_VERSION" $conf)
    txt=`echo $cur_v | awk -F [\"\'] '{print $2}'`
    f2=`echo $cur_v | tr [\$\(\)\,\'\"\;\ ] \.`
    t2="define\(\'$txt\',\'$v_desired\'\)\;"
    sed -e "s/$f1/$t1/" -e "s/$f2/$t2/" $conf > /tmp/conf_$module
    sdiff -sbB  $conf  /tmp/conf_$module || echo -n
    echo
    echo cp -pvf /tmp/conf_$module  $conf
    echo
done
