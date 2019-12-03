#! /usr/local/bin/perl
#
# Parce the syslog files and write the adb database
#

BEGIN {(my $dir=`dirname $0`)=~s/(\n|\r)//;
       unshift(@INC,$dir,"$dir/..");}

use strict;
use adb;
use sql;
require "adb.config"; # Here is the filter !

my ($dryrun, $counterF, $counterL);

my @masks = (
#	  ($service,$pid,$login, $logFrom) = ($line =~ /\s([a-z]\w+)[\[\:]([\:\]\d]*) .*$mask/i)
	     'Login (\S+) to (\S+) as (\S+)',               # IMP WRONG serv/pid/login sequence !
	     '(\w+) login on (/dev/tty\S+|:\S+)?',          # DUnix root login
	     'SU (\w+) on /dev',                            # DUnix su
	     'session opened for user (\w+) by ',           # Linux PAM_unix
	     'LOGIN as .(\w+). from ([\w\.\-]+)',           # sshd 
	     'log: Password authentication for (\w+) accepted',  # sshd
	     'Accepted password for (\w+) from (\S+) ',     # sshd
	     'Accepted publickey for (\w+) from (\S+) ',
	     'au\S+ user\=(\w+) host\=([\w\.\-]+) ',        # imap/pop
	     'login user\=(\w+) host\=([\w\.\-]+) ',        # imap/pop
	     '(halt|reboot) by (\w+):',                     # shutdown:
	     'Alpha (\w+): available memory',               # Alpha boot 
	     'inux version ([\w\.\-]+) ',   # Linux boot 
	     );

my %dateMasks = (
		 1 => '^(\d+)[ -](\d+)',
		 2 => '^(\w+\s+\d+)\s+(\d+:\d+:\d+)\s+', # Aug  1 15:34:59 darklx05 
		 );

(my $localDomain = `hostname`) =~ s /(\n|\r)//g;
$localDomain =~ s/^.*\.(\w+)\.se/$1.se/;  # Dont use 'domainname' for that, destroys dUNIX !!!!!

my ($date, $time, $host, $login, $logFrom, $service, $pid);

my (%hosts, %users, %peers, %times);

foreach my $file (@ARGV) {
  next  if $file =~ /Problems|debug/;
  if      ($file =~ /^-/) { $dryrun++; # "-no"
			    print "********************* NO mysql !\n";
			    next;
			}
  my $Host = config::ip2name (`basename $file`);
  print   "... Reading $Host\n";
  if ($file =~ /gz$/) { open F, "gunzip -c $file|"; }
  else                { open F,            $file; }
  ++$counterF;
  while (chomp(my $line=lc <F>)){
#      print "$line\n";
      foreach my $mask (@masks) {
	  ++$counterL; my $p3;
	  ($service,$pid,$login, $logFrom, $p3) = ($line =~ /\s([a-zA-Z]\w+)[\[\:]([\:\]\d]*) .*$mask/i)
	      or next;
	  
	  if ($service eq 'imp') {
	      $logFrom = config::ip2name($login).'(IMP)';
	      $login   = $p3;
	  }

	  # unix/linux root login does not fit the mask...
	  if ($line =~ /(syslog|login|root)[\[\]\d\:]+ root login on (\S+)/i){
	      $service = 'login';
	      $login    = 'root';
	      $logFrom = $2;
	      $logFrom = $logFrom =~ /(:0|console)/ ? "Console" : "";
	  }
	  
	  my $dateMask;
	  foreach $dateMask (values %dateMasks){
	      ($date,$time) =  ($line =~ /$dateMask/);
	      last if $time;
	  }
	  &time::parce ("$date $time");
	  $date = $time::d;
	  $time = $time::t;
	  
	  $logFrom = config::ip2name($logFrom);
	  
	  $host = $Host;
	  $host = $1 if $line =~ /$dateMask ([\w\.\-]+) $service/;
	  $host = config::ip2name ($host);

	  $service =~ s/ipop.*/pop/;	  $service =~ s/imap.*/imap/;

#	  print "\t$date $time $host, $service, $login, $logFrom\n";
	  
	  if ($service =~ /shutdown/){
	      my $t    = $logFrom;
	      $logFrom = uc $login;
	      $login    = $t;
	  }
	  if ($service =~ /kernel|vmunix/ && $login =~ /^(boot|\d)/){
	      $logFrom = $login if $service eq 'kernel';
	      $login    = 'BOOT';
	      $service = 'startup';
	  }
	  
	config::joinIt ($times{'Timing'},   $date, $time, $host, $login, $logFrom, $service);
	config::joinIt ($users{lc $login},  $date, $time, $host, $login, $logFrom, $service);
	config::joinIt ($hosts{lc $host},   $date, $time, $host, $login, $logFrom, $service);
	config::joinIt ($peers{lc $logFrom},$date, $time, $host, $login, $logFrom, $service)
	    if $logFrom && $service ne 'shutdown';
      }
  }
  close F;
}

&config::printIt ("Chronology of events:", \%times);
#&config::printIt ("Logins",      \%users);
#&config::printIt ("Logged to",   \%hosts);
#&config::printIt ("Logged from", \%peers);

&config::mysqlIt ("adb",         \%times)
    unless $dryrun;

print "\n\nSummary:\n";
my $execTime = time - $^T;
my $speed = $counterL / ($execTime || 1);
printf "%15d files processed\n", $counterF if $counterF > 1;
printf "%15d lines processed\n", $counterL;
printf "%15d seconds execution time\n", $execTime;
printf "%15d lines/second processing speed\n\n", $speed;
exit;

package uty;
sub filter { return config::ip2name ($_[0]); }

