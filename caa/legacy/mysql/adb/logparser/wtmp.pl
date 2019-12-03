#! /usr/local/bin/perl
#
# Parce wtmp (Digital UNIX) files and write adb database
#

BEGIN {(my $dir=`dirname $0`)=~s/(\n|\r)//;
       unshift(@INC,$dir,"$dir/..");}

use strict;
use time;
use adb;
use sql;
require "adb.config"; # Here is the filter !

exit unless (my $system = `uname -s`) =~ /OSF/;

(my $host = `hostname -s`)=~s/(\n|\r)//;
my $debug=0;
my ($count,
    %hosts, %users, %peers, %times);

time::fmt(5);

open F, "/usr/sbin/acct/fwtmp < /var/adm/wtmp |"
    or die "*** Cant open wtmp: $!"; 

my $dateMask  = '[A-Z]\w\w [A-Z]\w\w [\w\:\s]+';
my $loginMask = '[a-z][a-z0-9]+';
my $hostMask  = '[a-z][\w\-\.]+|\d+\.\d+\.\d+\.\d+';
my %db = ();

$| = 1;

while (my $line=<F>) {
    my ($date, $time, $login, $logFrom, $service);
    print  "=" if $debug;

    $line =~ s/\s+/ /g; 
    $line =~ s/ (MET|DST\b)//g;
    ($date = $line) =~ s/.*($dateMask)/$1/;
    next unless $date;

    if ($line =~ /(system boot|shutdown)/) { $service = $1 eq 'shutdown' ? 'SHUTDOWN':'BOOT'; 
					     $login = 'root';
					 }
    
    if ($line =~ /^($loginMask) (tty|ftp|p).* ($hostMask) /){ $service = $2 eq 'ftp'?'ftp':'login';
							      $login   = $1;
							      $logFrom = $3;}
    if ($line =~ /^($loginMask) (:0|cons) (:0|console)/)    { $service = 'login';
							      $login   = $1;
							      $logFrom = 'Console';}
    #alberto fopm2:0 fopm2:0 22985 07 0000 0000 993162915 fopm2:0 Fri Jun 22 00:35:15 2001
    if ($line =~ /^($loginMask) ($hostMask):0 /)            { $service = 'loginx';
							      $login   = $1;
							      $logFrom = $2;}
    
    ($date,$time) = split / /, &time::T($date);
    if ($service){
	if ($logFrom =~ /DEBUG tty/){
	    print "$line\n";
	    printf "-------------- %-10s %s %-8s %s\n", $service, $date, $login, $logFrom;
	}
	print  "." if $debug;;
	$logFrom = config::ip2name($logFrom);
	undef $logFrom if $logFrom =~ /($host|localhost)/;
	next if $logFrom =~ /^tty/;

	&config::joinIt ($times{'Timing'},   $date, $time, $host, $login, $logFrom, $service);
	&config::joinIt ($users{lc $login},  $date, $time, $host, $login, $logFrom, $service);
	&config::joinIt ($hosts{lc $host},   $date, $time, $host, $login, $logFrom, $service);
	&config::joinIt ($peers{lc $logFrom},$date, $time, $host, $login, $logFrom, $service)
	  if $logFrom && $service ne 'shutdown';

    }else{ # See the non-parsed lines
	next if $line =~ /(LOGIN|getty)? (dt|cons|console) console/;
	next if $line =~ /^ (ftp|tty|:0)/;
	next if $line =~ /^ (run-level|lsm|vol|fs|sysconfig|kmk|s2|s3|it|update) /;
	next if $line =~ /\/dev\/conso/;
#	print "$line\n";
    }
}

&config::printIt ("Timing",      \%times);
#&config::printIt ("Logins",      \%users);
#&config::printIt ("Logged to",   \%hosts);
#&config::printIt ("Logged from", \%peers);

my $dryrun;
&config::mysqlIt ("adb",         \%times)
    unless $dryrun;

