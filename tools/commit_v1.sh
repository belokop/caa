#! /bin/bash

set -e
set +o posix

<<<<<<< HEAD
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
=======
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
>>>>>>> f99c15a59b6f20e6ec3548b20f1b87c1900bd475

# Go to the repositary root
src=$(cd `dirname $0`; pwd -P)

echo Modified areas:
cd $src/..
changes=$(git status | grep -E "(modified|new file):" | awk -F: '{print $2}' | awk -F/ '{print $1"/"$2}' | sort -u)
for area in $changes; do echo $area; done
echo

for area in $changes; do
    [ -f $src/../$area/config.inc ] || continue
<<<<<<< HEAD
    conf=$src/../$area/config.inc
    echo $conf
=======
    conf=$(cd $src/../$area; pwd -P)/config.inc
    module=$(basename $area)
    echo
    echo ======================================================================================= $area
    echo 
>>>>>>> f99c15a59b6f20e6ec3548b20f1b87c1900bd475

    # $releaseDate = '2019-12-15';
    date=$(date +%Y-%m-%d)
    cur_d=$(grep -E "^.releaseDate" $conf)
<<<<<<< HEAD
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
=======
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
>>>>>>> f99c15a59b6f20e6ec3548b20f1b87c1900bd475
done
