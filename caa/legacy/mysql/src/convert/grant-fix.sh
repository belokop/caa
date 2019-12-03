#! /bin/bash

. `dirname $0`/functions.sh || exit 1

# GRANT ALL PRIVILEGES ON hostdb_scfab_se.* TO 'rt'@'localhost' WITH GRANT OPTION
# REVOKE priv_type [(column_list)] [, priv_type [(column_list)] ...]
#    ON {tbl_name | * | *.* | db_name.*}
#    FROM user_name [, user_name ...]

TMP=/tmp/delete.me/G && mkdir -p /tmp/delete.me || exit 1
#for user in ihp rt otrs perdition horde;do
for user in root ;do
    echo $user
    host=localhost
    for localdb in `echo "show databases;" | $MYSQL -uroot -p${PWD}|grep -vE "^test|mysql|Database"`; do

	[ "$localdb" = "hostdb_physto_se" ] || continue
	
	echo "   $localdb"
	[ -z "`locateDB $localdb $HOST`" ] && continue
	echo  "SHOW GRANTS FOR '${user}'@'${host}'" | $MYSQL -uroot -pmysql $localdb > $TMP 2>/dev/null || continue
	[ -s $TMP ] || continue

#cat $TMP	
	grep -vE "Grants for|ERROR" $TMP | \
	    grep -v " ON $user" | \
	    sed "s/ WITH .*/ /"     | \
	    sed "s/GRANT/REVOKE/" | \
	    sed "s/ TO / FROM /"  | \
	    while read line; do

	    echo "$line" > $TMP   # escape from the shell
	    grep 'REVOKE  ON' $TMP >/dev/null 2>&1 && \
		echo "$line"|sed "s/REVOKE  ON/REVOKE GRANT OPTION ON/" > $TMP
	    grep 'REVOKE ON' $TMP >/dev/null 2>&1 && \
		echo "$line"|sed "s/REVOKE ON/REVOKE GRANT OPTION ON/" > $TMP
	    
	    # delete ALL PRIVILEGES ON *.* IDENTIFIED BY ...
	    grep PASSWORD $TMP  >/dev/null 2>&1 && {
		grep ' ON ... FROM '  >/dev/null 2>&1 $TMP || continue
	    }

	    cat $TMP
	    cat $TMP | $MYSQL -uroot -pmysql $localdb || exit 1
	done
    done
done
rm -rf /tmp/delete.me