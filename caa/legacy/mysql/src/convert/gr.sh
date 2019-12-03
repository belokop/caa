#! /bin/csh -fx

#shell> mysql --user=root mysql
#mysql> GRANT ALL PRIVILEGES ON *.* TO monty@localhost IDENTIFIED BY 'something' WITH GRANT OPTION;
#mysql> GRANT ALL PRIVILEGES ON *.* TO monty@"%"       IDENTIFIED BY 'something' WITH GRANT OPTION;
#mysql> GRANT RELOAD,PROCESS ON *.* TO admin@localhost;
#mysql> GRANT USAGE ON *.* TO dummy@localhost;

set dbs = (mysql maildb hostdb adb dnsdb whoiswhere fc_cc hostdb_fc )  
set dbs = ( test mysql )  

foreach db ($dbs)
  echo $db
   echo "GRANT ALL PRIVILEGES ON * TO root@localhost IDENTIFIED BY 'mysql' WITH GRANT OPTION" | /usr/pkg/mysql/pro/bin/mysql  $db
   echo "GRANT ALL PRIVILEGES ON * TO  www@localhost IDENTIFIED BY 'mysql' WITH GRANT OPTION" | /usr/pkg/mysql/pro/bin/mysql  $db
   echo "GRANT ALL PRIVILEGES ON * TO root@'%.physto.se' IDENTIFIED BY 'mysql' WITH GRANT OPTION" | /usr/pkg/mysql/pro/bin/mysql  $db
   echo "GRANT ALL PRIVILEGES ON * TO  www@'%.physto.se' IDENTIFIED BY 'mysql' WITH GRANT OPTION" | /usr/pkg/mysql/pro/bin/mysql  $db
   echo "GRANT ALL PRIVILEGES ON * TO  '%'@'%.physto.se' IDENTIFIED BY 'mysql' WITH GRANT OPTION" | /usr/pkg/mysql/pro/bin/mysql  $db
   echo "GRANT ALL PRIVILEGES ON * TO root@ices02.physto.se IDENTIFIED BY 'mysql' WITH GRANT OPTION" | /usr/pkg/mysql/pro/bin/mysql  $db
end
