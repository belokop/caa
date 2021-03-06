package ConfigDB;

$main::DB = 'maildb';
sub main::administrator { # administrator accounts, those see the "secret fields"
    return ($main::hlo{iam} =~ /^(xxx_man)$/)
	|| &main::root;
}
sub main::root { # admin accounts, see the error
    return ($main::hlo{iam} =~ /^(yb|hvzm|root|daemon|legacy_man|xxx_man)$/);
}

%main::MX = (
	     FKM     => "physto.se",
	     ALB     => "albanova.se",
	     NOR     => "nordita.org",
	     VH      => "vetenskapenshus.se", 
	     default => "physto.se",
	     );
#$maildb::domain = `hostname`;       chomp $maildb::domain;
#$maildb::domain =~ s/^.*\.(\w+\.\w+)$/$1/;

$maildb::domain = "nordita.org";
$sql::server    = "localhost";
$main::logfile  = "/var/log/maildb";

if ("@ARGV" =~ /-config/) {
  print "$main::DB $sql::server\n";
  exit;
}

# The database is expected to be located 
# on the same server as the maildb daemon (do not mix with SQL server)

$main::MTA = "sendmail";	  
$main::MTA = "postfix";	  

$main::sendmailDB = "/var/adm/sendmail/user.db";
$main::postfixDB  = "/etc/postfix.maps/maildb";

if ($main::MTA eq "sendmail" && ! -f $main::sendmailDB) {
  print "*** Can not access $main::sendmailDB\n";
  die "\n"; }

#if ($main::MTA eq "postfix" && ! -f $main::postfixDB) {
#  print "*** Can not access $main::postfixDB\n";
#  die "\n";  }

1;
