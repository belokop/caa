#! /bin/bash

. `dirname $0`/functions.sh || exit 1

#
# Grant access for the superuser for all LOCAL databases
# (needed by web interface + some old code)
#
# WHAT ABOUT USER 'www'?

for db in `echo "show databases;" | $MYSQL -uroot -p${PWD}|grep -vE "^test|mysql|Database"`; do
echo $db
    [ -z "`locateDB $db $HOST`" ] && continue
    for h in \
	localhost \
	syslx10 \
	syslx11 \
	fclx01 \
	mail \
	`hostname -s` \
	webmail \
	www www.it www.cn\
	; do 
      grantToAllAliases root $h ${PWD} $db
    done
done
exit

#
# Users
#
for set in \
    'horde3:inboxsql.physto.se:horde:mbox inbox mail webmail:horde' \
    'ihp::ihp:www.fullerenes.net:eur0p3' \
    'rt::rt:www:rt' \
    'otrs::otrs:www:otrs' \
    'perdition::perdition:www:perdition' \
    'horde2:inboxsql.physto.se:horde:inbox:horde' \
    ; do
echo $set
  db=`echo $set | awk -F: {'print $1}'`
  at=`echo $set | awk -F: {'print $2}'`
  us=`echo $set | awk -F: {'print $3}'`
  sr=`echo $set | awk -F: {'print $4}'`
  pw=`echo $set | awk -F: {'print $5}'`

  if [ -z "$at" ]; then
      n=`locateDB $db|wc -l|sed "s/ //g"`
      mess="is on many servers, please specify:"
      [ "$n" = "0" ] && mess="DOES NOT EXIST"      
      [ "$n" = "1" ] || {
	  echo;echo "*** database $db $mess"
	  locateDB $db
	  echo && continue
      }
      at=`locateDB $db`  
  fi

  set $sr
  while test -n "$1"; do
      grantToAllAliases  $us $1 $pw $db $at
      shift
  done
done

rm -rf /tmp/delete.me/
