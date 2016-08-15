#! /bin/bash
#
# EA is the reference module
#

pwd=`pwd -P`
area=`echo $pwd | sed s,.*/caa/,, | awk -F/ '{print $1}'`
module=`echo $area  | sed s/preprints/prp/`
if [ "$1" = "silent" ]; then silent=yes; else silent=; fi 
if [ -z "$module" ]; then
    echo ... must be run from the module directory tree >&2
    exit 1
fi

modulU=`echo $module | tr a-z A-Z`
rm -rf \
    /tmp/$module* \
    /tmp/*yml \
    /tmp/*module \
    /tmp/src/
echo ----------------------------------------------------------------------------------------------------------------------------------------- $modulU

ea=`echo $pwd | sed s,/$area.*,/ea/_drupal-8,`
echo $ea

function editIt(){
    ea_file=$1
    module_file=`echo $ea_file | sed s/^ea/$module/`
    mkdir -p /tmp/`dirname $module_file`
    if [ -z "$silent" ]; then echo ----------------------------------------------------------------------------------------------------------------------------------- $ea_file >&2; fi
    { cat $ea/$ea_file || exit 1; } | \
	sed \
	-e s/ea/${module}/g \
	-e s/EA/${modulU}/g \
	-e s/for${module}ch/foreach/g \
	-e s/myP${module}r/myPear/g \
	-e s/l${module}ding/leading/g \
	-e s/cl${module}n/clean/g \
	-e s/r${module}dy/ready/g \
	-e s/R${module}dy/Ready/g \
	> /tmp/$module_file 
}

#editIt ea.module
editIt ea.services.yml

php -n `dirname $0`/d8_create.yml.php $1 || { echo; echo ... error exit >&2; exit 1; }

if [ "$1" != "silent" ]; then
    
    d8=`echo $pwd | sed s,/caa/.*,/caa/$area/_drupal-8/,`
    echo 
    ls -l /tmp/$module*yml
    ls -l /tmp/$module*module 2>/dev/null
    ls -l /tmp/src/components/*   2>/dev/null
    echo
    ls -1 /tmp/$module*yml    | awk -F/ "{ print \"[ -s $d8/\"\$3\" ] || cp -pvf /tmp/\"\$3\" $d8/\" }" > /tmp/t; . /tmp/t
    ls -1 /tmp/$module*yml    | awk "{ print \"sdiff -sbB -w200 \"\$1\"   $d8\" }"
    ls -1 /tmp/$module*module 2>/dev/null | awk "{ print \"sdiff -sbB -w200 \"\$1\"   $d8\" }"
    ls -1 /tmp/src/components/* 2>/dev/null | awk "{ print \"sdiff -sbB -w200 \"\$1\"   $d8\" }"
    echo
    # ls -1 /tmp/$module* | awk "{ print \"cp -pfv \"\$1\"   $d8\" }"
fi
