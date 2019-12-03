#! /bin/bash

MYSQL=/usr/pkg/mysql/pro/bin/mysql
HOST=`hostname`
PWD=mysql
[ -n "`hostname|grep scfab.se`" ] && PWD=scfab
export MYSQL HOST PWD

TMP=/tmp/delete.me/$$ && rm -rf /tmp/delete.me && mkdir /tmp/delete.me || exit 1
export TMP

ipaddr() {
    [ -n "`echo $1|grep -E '^[123456789]'`" ] && echo $1 && return
    /usr/bin/host $1 2>/dev/null | \
	grep 'has address' | \
	tail -1|sed 's/.*address //'
}

dnsname() {
    [ -n "`echo $1|grep -E '^[123456789]'`" ] && ip=$1
    [ -z "$ip" ] && ip=`ipaddr $1` 
    [ -z "$ip" ] && return
    /usr/bin/host $ip 2>/dev/null | \
	grep pointer | \
	sed -e "s/.* pointer //" -e "s/\.se\./.se/"
}

function locateDB(){
    # 1 - database name
    # 2 - (optional) sql server
    
    for server in \
	mysql \
	syssql \
	inboxsql \
	mail3 \
	;do
      for domain in physto.se scfab.se; do
	  if [ -n "$2" ]; then
	      [ "`dnsname $2`" = "`dnsname $server.$domain`" ] || continue
	  fi
	  echo 'show databases'|mysql -uroot -pmysql -h $server.$domain 2>/dev/null|while read db; do
	[ $db = $1 ] && echo ${server}.$domain
      done
    done
    done
}

grantIt() {
    user=$1
    host=`dnsname $2`; [ -z "$host" ]    && return
    password=$3;       [ -z "$password" ]&& return
    db=$4;             [ -z "$db" ]      && return
    sqlh=$5;  [ -n "$sqlh" ] && sqlh="-h$sqlh"

    stamp=${TMP}${1}${2}${3}${4}${5}
    [ -f $stamp ] && return || touch $stamp

    host=`echo $host|sed "s/localhos.*/localhost/"`    
    # May be the privs are granted already
    echo  "SHOW GRANTS FOR '${user}'@'${host}'"|$MYSQL $sqlh -uroot -pmysql $db|grep -i localhost
    echo  "SHOW GRANTS FOR '${user}'@'${host}'"|$MYSQL $sqlh -uroot -pmysql $db 2>/dev/null | \
	grep -v ' ON ... TO ' | \
	grep "WITH GRANT OPTION" | \
	grep "'${user}'@'${host}'" | \
	grep -E "( ON $db\.| ON .$db.\.| ON \*\.\*)" > $stamp

    if [ -s $stamp ]; then
#	echo;echo Existing grants on database $db
	return
#    else
#	echo ... no grants on $db
    fi

    if [ -n "$5" ]; then
	srvip=`ipaddr $5`
	[ -z "`/sbin/ifconfig -a |grep $srvip`" ] && {
	    stamp=${TMP}${srvip}
	    [ -f $stamp ] && return || touch $stamp
	    echo;echo "*** Can't grant privs for db '$db' on behalf of $5... will try anyway"
#	    return
	}
    fi
    
    
    echo "GRANT ALL ON $db.* TO '${user}'@'${host}' IDENTIFIED BY '$password' WITH GRANT OPTION;" > $TMP || exit 1
    $MYSQL                      $db < $TMP >/dev/null 2>&1 
    $MYSQL $sqlh -uroot -pmysql $db < $TMP 2>>$TMP || {
	echo 1>&2 && cat $TMP 1>&2 && echo 1>&2 && return
    }
    echo access granted to $db $sqlh for $user@$host pwd=$password|sed "s/.physto.se//g"
    rm -rf $TMP
}

grantToAllAliases() {
    us=$1
    alias=$2
    pw=$3
    db=$4
    at=$5
    for h in \
	localhost \
	$alias \
	`dnsname ${1}|sed  -e "s/.physto.se//g" -e "s/.scfab.se//g"` \
	`dnsname ${1}1|sed -e "s/.physto.se//g" -e "s/.scfab.se//g"` \
	`dnsname ${1}2|sed -e "s/.physto.se//g" -e "s/.scfab.se//g"` \
	`dnsname ${1}3|sed -e "s/.physto.se//g" -e "s/.scfab.se//g"` \
	; do
      
      grantIt $us ${h}   $pw $db $at
      grantIt $us ${h}1  $pw $db $at
      grantIt $us ${h}2  $pw $db $at
      grantIt $us ${h}3  $pw $db $at
      grantIt $us ${h}-1 $pw $db $at
      grantIt $us ${h}-2 $pw $db $at
      grantIt $us ${h}-2.scfab.se $pw $db $at
      grantIt $us ${h}-3 $pw $db $at
      grantIt $us ${h}-4 $pw $db $at
      grantIt $us ${h}-5 $pw $db $at
      grantIt $us ${h}-6 $pw $db $at
    done
}