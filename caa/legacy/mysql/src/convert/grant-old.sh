#! /bin/bash

PWD='scfab'
PWD='mysql'

MYSQL=/usr/pkg/mysql/pro/bin/mysql

#shell> mysql -uroot mysql
#mysql> GRANT ALL PRIVILEGES ON *.* TO monty@localhost IDENTIFIED BY 'something' WITH GRANT OPTION;
#mysql> GRANT ALL PRIVILEGES ON *.* TO monty@"%"       IDENTIFIED BY 'something' WITH GRANT OPTION;
#mysql> GRANT RELOAD,PROCESS ON *.* TO admin@localhost;
#mysql> GRANT USAGE ON *.* TO dummy@localhost;

#   echo "create database $db" | $MYSQL -uroot -p${PWD} -h$sqlhost 

ipaddr() {
    /usr/bin/host $1 2>/dev/null | grep 'has address'|tail -1|sed 's/.*address //'
}

hosts=localhost
for h in \
    www www1 www2 www.internal \
    syslx13 syslx13-1 syslx13-2 \
    syslx14 syslx14-1 syslx14-2 \
    syssql mysql \
    fclx01 \
    inbox syslx11 syslx01 \
    mail3 \
    rooms lbs \
    ; do
  for d in scfab.se physto.se; do
      [ -n "`ipaddr $h.$d`" ] && hosts="$hosts $h.$d"
  done
done

users="root www"

grantIt() {
    user=$1
    host=$2; [ -z "$host" ] && { echo "??? host"; exit 1; }
    p=$3
    db=$4
    sqlh=$5; 
    if [ -n "$sqlh" ]; then
	sqlh="-h$sqlh"
    fi
    TMP=/tmp/delete.me/$$
    echo "GRANT ALL ON *.* TO ${user}@'${host}' IDENTIFIED BY '$p' WITH GRANT OPTION;" > $TMP
#    echo;echo -n "$sqlh $db -> ";cat $TMP
    $MYSQL                             < $TMP >/dev/null 2>&1 
    $MYSQL $sqlh -uroot -p${PWD} ${db} < $TMP || { 
	echo -n "ERROR when executing ";cat $TMP
	exit 1
    }
    rm -rf $TMP
}


#    mysql.`domainname -d` \
#    syssql.`domainname -d` \
#    `hostname` \
for sqlhost in \
    localhost \
    ; do
  
  grantIt root $sqlhost ${PWD}

  ip=`ipaddr $sqlhost`
  [ -z "$ip" ] && continue
  [ -z "`/sbin/ifconfig|grep \"addr:$ip \"`" ] && continue
  dbs=`echo "show databases;" | $MYSQL -uroot -p${PWD} -h$sqlhost|grep -vE "^test|mysql|Database"` || exit 1
  [ -z "$dbs" ] && { echo "???? no database found on $sqlhost"; continue; }
  
  for db in $dbs; do
 # [ ${db} = "otrs" ] || continue
      echo ${db} # on ${sqlhost}
      for host in $hosts; do
	  for user in $users; do
	      grantIt $user $host ${PWD} $db $sqlhost
	  done
	  for user in ihp otrs rt horde perdition; do
	      [ -z "`echo $db|grep ^$user`" ] && continue
	      p=$user
	      [ $user = "ihp" ] && p="eur0p3"
	      grantIt $user $host $p $db $sqlhost
	  done
      done
  done
done
