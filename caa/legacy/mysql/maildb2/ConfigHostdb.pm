# maildb configuration file
use strict;

# The default domain name.
# It is often tricky to find it, hence better just to define it 
($main::hostname = `hostname`) =~ s/(\r|\n)//;
($main::domain   = `hostname`) =~ s/^.*\.(\w+)\.(\w+)(\r|\n)/$1.$2/;
($main::myip     = `host $main::hostname|grep address|awk '{print \$4}'`) =~ s/(\r|\n)//;
($main::myarp    = `/sbin/ifconfig eth0|tr A-Z a-z|grep hwaddr|awk '{print \$5}'`) =~ s/(\r|\n)//;
($main::HOST     = $main::hostname) =~ s/.(scfab|albanova|physto).se//;

# mysql server && database
($main::sqldatabase,$main::sqlserver) = ('maildb',"syssql.$main::domain");
$main::SQLlogTable   = "master";

if ("@ARGV" =~ /config/){
  print
    "domain=$main::domain\n",
    "mysqldatabase=$main::sqldatabase\n",
    "mysqlserver=$main::sqlserver\n",
    ;
  exit;
}

1;
