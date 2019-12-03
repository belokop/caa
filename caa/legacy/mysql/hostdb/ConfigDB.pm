package ConfigDB;

$hostdb::localDomain= 'physto.se';
$hostdb::nameServer = "ns." . $hostdb::localDomain;
$main::logfile      = '/var/log/messages;/log/hostdb/messages'; # 2 logfiles!

$main::DB    = 'hostdb';
$sql::server = 'syssqlh';

if ($ARGV[0] =~ /^\s?-config/) {
  print $main::DB, " ", $sql::server,"\n";
  exit 1;
}

sub main::administrator { # administrator accounts
    return 1 if &main::root;
    return ($_[0] =~ /^(www|moa)$/);
}
sub main::root { # root accounts
    return ($_[0] =~ /^(yb|belokop|root)$/);
}


#########################################################
#              Do not edit below this line              #
#########################################################
$snmp::localDomain = $hostdb::localDomain;
 $sql::localDomain = $hostdb::localDomain;
  $ns::localDomain = $hostdb::localDomain;

1;
