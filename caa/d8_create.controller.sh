#! /bin/bash
#
# EA is the reference module
#

pwd=`pwd -P`
area=`echo $pwd | sed s,.*/caa/,, | awk -F/ '{print $1}'`
module=`echo $area  | sed s/preprints/prp/`
if [ -z "$module" ]; then
    echo ... must be run from the module directory tree >&2
    exit 1
fi

modulU=`echo $module | tr a-z A-Z`
rm -rf /tmp/$module*
echo ----------------------------------------------------------------------------------------------------------------------------------------- $modulU

ea=`echo $pwd | sed s,/$area/,/ea/,`
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
	> /tmp/$module_file || exit 1
#   ls -l /tmp/$module_file
}

# editIt ea.custom_hooks.inc
editIt ea.module

php -n `dirname $0`/D8_create.yml.php || { echo; echo ... error exit >&2; exit 1; }

d8=`echo $pwd | sed s,/caa/.*,/caa/$area/_drupal-8/,`
echo 
ls -l /tmp/$module*
echo
ls -1 /tmp/$module* | awk "{ print \"sdiff -sbB -w200 \"\$1\"   $d8\" }"
echo
# ls -1 /tmp/$module* | awk "{ print \"cp -pfv \"\$1\"   $d8\" }"
