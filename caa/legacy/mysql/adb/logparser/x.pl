#! /usr/local/bin/perl
#
# Usage $o -d sorts by date, otherwise by IP number
#
BEGIN {(my $dir=`dirname $0`)=~s/(\n|\r)//;
       unshift(@INC,$dir,"$dir/..");}

use strict;
use Socket;
use time;

my $bydate = shift;

chomp (my $host = `hostname -s`);
$host = "fclx01|syslx10|dhcp[123]";

my (%lease, %DHCPpool, %dejavu, %lastseen, %names, %bydt, %lastip,
    $date, $time, $count, $ip, $arp, 
    );


 ReadDHCPpool: {
   open P, "/etc/dhcpd.conf";
   while (<P>) {
     s/\#.*$//;
     next unless /range\s+(\S+)\s+(\S+);/;
     my ($p1, $p2) = ($1, $2);
     my ($a,$b,$c,$d) = split /\./, $p1;
     my ($e,$f,$g,$D) = split /\./, $p2;
     while ($d <= $D) { $DHCPpool{"$a.$b.$c.".$d++} = ''; }
   }
   close P;
#  foreach (sort keys %DHCPpool) { print "$_\n"; }
 }


open F, "cat /var/log/messages*|grep dhcpd:|" or die $!;
&time::fmt(1);

while (<F>) {
  (my $d  = $_) =~ s/ ($host).*$//;
  my $datetime  = time::T($d);
  ($date,$time) = (split / /, $datetime) or next;
  
  if (/DHCP(REQUEST|ACK|OFFER) .* ([\da-f]+:\S+) \((.*)\) via/) {
    my ($arp,$name) = ($2,$3);
    $names{$arp} = $name;
  }
  
  if (/DHCPACK on (\S+) to (\S+) /) {
    # Feb 17 16:40:28 fclx01 dhcpd: DHCPACK on 130.237.205.124 to 00:06:5b:69:ef:d2 via eth0
    ($ip,$arp) = ($1,$2);
  }elsif (/DHCPREQUEST for (\S+) .*rom (\S+) .* (ignored|unknown)/) {
    #  DHCPREQUEST for 192.168.2.25 (130.242.128.21) from 00:08:74:9a:b1:e7 via eth0: ignored (not auth
    ($ip,$arp) = ($1,$2);
    $ip .= " . ?";
  }else{
    next;
  }
  $lease{$date}{$arp}    = $ip;
  $lastseen{$date}{$arp} = $time;
  $dejavu{$date}{$ip}++;
  if (!$bydt{$arp} ||
      ($bydt{$arp} && $bydt{$arp} lt $datetime)) {
    $bydt{$arp}            = $datetime;
    $lastip{$arp}          = $ip;
  }
  # last if ++$count > 333;
}


if ($bydate) {
  foreach my $arp (sort {$bydt{$a} cmp $bydt{$b}} keys %bydt) {
    my $ip = $lastip{$arp};
    (my $dns = (gethostbyaddr(inet_aton($ip), AF_INET))[0]) =~ s/.physto.se//;
    undef $dns if $dns =~ /dhcp/;
    printf
      "%s   %s %-18s %-16s %s %s\n",
      $bydt{$arp},
      $arp,
      lc $names{$arp},
      $lastip{$arp},
      $dns;
  }
  exit;
}
  

foreach $date (sort keys %lease) {
  printf "\n%s  -------------------------------------------------------------\n\n", $date;
  foreach my $arp (sort  ipsort  keys %{$lease{$date}}) {
    sub ipsort () { my @a = split /\./, $lease{$date}{$a};
		    my @b = split /\./, $lease{$date}{$b};
#		    print "@a <=> @b  '$a' '$b'\n";
		    for my $n (0..3) {
#		      print "  $n - @a[$n] ne @b[$n]\n";
		      if (@a[$n] ne @b[$n]) { return int @a[$n] <=> int @b[$n]; }
		    }
		  }
    my $ip   =  $lease{$date}{$arp};
    
    (my $dns = (gethostbyaddr(inet_aton($ip), AF_INET))[0]) =~ s/.physto.se//;
    undef $dns if $dns =~ /dhcp/;
      $dns = "<--" if lc($dns) eq lc($names{$arp}) && $names{$arp};
    
    if (!$dns) {
      $dns = "*in*DNS*" if gethostbyname lc($names{$arp});
    }
    
    printf
      "%-20s %s %-12s %-16s %s\n",
      $ip, $arp,
      lc $names{$arp},
      $dns,
      $lastseen{$date}{$arp};
  }
}
