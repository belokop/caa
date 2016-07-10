#! /bin/bash
# sed -e s/ea/vm/g -e s/forvmch/foreach/g -e s/myPvmr/myPear/g -e s/lvmding/leading/g vmController.php |grep vm

pwd=`pwd -P`
module=`echo $pwd | sed s,.*/caa/,, | awk -F/ '{print $1}'`
modulU=`echo $module | tr a-z A-Z`
rm -rf /tmp/$module*
echo ----------------------------------------------------------------------------------------------------------------------------------------- $modulU

ea=`echo $pwd | sed s,/$module/,/ea/,`
function editIt(){
    ea_file=$1
    module_file=`echo $ea_file | sed s/^ea/${module}/`
    rm -rf /tmp/$module_file
    # echo ----------------------------------------------------------------------------------------------------------------------------------- $ea_file >&2
    cat $ea/$ea_file | \
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
#   ls -l /tmp/$module_file
}

# editIt eaController.php
# editIt eaCustom_hooks.php
editIt ea.module

php -n `dirname $0`/create.yml.php

d8=`echo $pwd | sed s,/$module.*,/$module/_drupal-8/,`
echo 
ls -l /tmp/$module*
echo
ls -1 /tmp/$module* | awk "{ print \"sdiff -sbB -w200 \"\$1\"   $d8\" }"
echo
# ls -1 /tmp/$module* | awk "{ print \"cp -pfv \"\$1\"   $d8\" }"
