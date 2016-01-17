#! /bin/bash
#
# Adapt the SQL dump to become utf8_unicode_ci
#

parol=$1
sql=$2

[ -z $sql ] && { echo "??? usage..."; exit 1; }
[ -s $sql ] || { echo "??? file '$sql' does't exist"; exit 1; }
[ -s $sql.latin1 ] || mv -vf $sql $sql.latin1


[ -z "$3" ] && chr=utf8 || chr=$3
[ -z "$4" ] &&  ci=_unicode_ci  ||  ci=$4

echo ...
echo ... charcode  $chr
echo ... collate   $chr$ci
echo ...

pray="CHARACTER SET $chr COLLATE $chr$ci"

cat $sql.latin1  \
    | grep -v hut_street_number \
    | sed \
    -e s/latin1_general_ci/utf8_unicode_ci/g \
    -e "s/^. ENGINE=.*/\) ENGINE=InnoDB DEFAULT CHARSET=$chr COLLATE=$chr$ci;/" \
    > $sql
sdiff -sbB -w200 $sql  $sql.latin1  | grep -v '/\*.*\*/'
