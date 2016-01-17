#! /bin/bash
set -e 

for sql in sql/*sql; do
    f=`echo $sql | sed -e "s;sql/;;" -e "s;.sql;;"`
    [ -z "`echo 'show tables;' | mysql --defaults-extra-file=~/.my.cnf --protocol=tcp combodb | grep $f`" ] && {
	echo ... $sql
	mysql --defaults-extra-file=~/.my.cnf --protocol=tcp combodb < $sql
    }
done
