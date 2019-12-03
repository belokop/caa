#! /usr/local/bin/perl

BEGIN {(my $dir=`dirname $0`)=~s/(\n|\r)//;
       unshift(@INC,$dir,"$dir/..");}

#use strict;
use Socket;
use time;

my (%ips, %arp, %name, %arpReverse,
    %arp2name,
    $IP, $ARP, $ENDS, $IPold,
    $badcount, $badline, $badline1,
    %hostdb, 
    %lastLease,
    );

&time::fmt(1);
(my $today = time::T()) =~ s/ .*//;

(my $host = `hostname -s`) =~ s/(\r|\n)//g;

$|++;

my %DHCPpool;
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


my %ARPs;
open LEASE, "/var/lib/dhcp/dhcpd.leases" or die $!;
while (<LEASE>){
  $IP   = $1    if /^lease\s(\S+)\s/;    next unless $IP;
  $IPold = $IP unless $IPold;
  $ARP  = $1    if /hardware ethernet (\S+)\;/; 
  $ENDS = $1    if /ends \d+ (\S+) /; $ENDS =~ s/\//-/g;
  $arp2name{$ARP}=lc($1)               if /client-hostname \"(\S+)\"/;
  addit ($name{$ENDS}{$IP},$1)         if /client-hostname \"(\S+)\"/;
  addit ($arp{$ENDS}{$IP},  $1)        if /hardware ethernet (\S+)\;/; 
  addit ($arpReverse{$ENDS}{$1},  $IP) if /hardware ethernet (\S+)\;/; 
  $ips{$ENDS}{$IP}++ if $ENDS;
  $lastLease{$IP} = join '|', $ARP, $arp2name{$ARP}  if $ENDS && $IP && $ARP;
}
close LEASE;

open MESSAGES, "/var/log/messages" or die $!;
while (<MESSAGES>){
  chomp;
  ++$badcount   if /no free leases on subnet/;
  $badline1= $_ if /no free leases on subnet/ && !$badline1;
  $badline = $_ if /no free leases on subnet/;
  if (0) {
    # old code
    next unless /DHCPOFFER on (\S+) to (\S+) /;  my ($ip,$ar) = ($1,$2);
    (my $date = $_) =~ s/ $host.*$//;    $date = (split / /,&time::T($date))[0] or next;
    addit ($arp{$date}{$ip},         $ar);
    addit ($arpReverse{$date}{$ar},  $ip);
  }else{
  # Feb 17 15:45:28 fclx01 dhcpd: DHCPACK on 130.237.205.124 to 00:06:5b:69:ef:d2 via eth0
    next unless /DHCPACK on (\S+) to (\S+) /;  my ($ip,$ar) = ($1,$2) or next;
    (my $date = $_) =~ s/ $host.*$//;    $date = (split / /,&time::T($date))[0] or next;
    addit ($arp{$date}{$ip},         $ar);
    addit ($arpReverse{$date}{$ar},  $ip);
#    print "$date $ip  \t$ar\n";
  }
  $ips{$date}{$ip}++;
}
close MESSAGES;


my %count;
foreach my $date (sort keys %ips) {
    print "\n$date  IP -> ARP table ==========================================================\n";
    foreach my $ip (sort ipsort keys %DHCPpool) { 
#   foreach my $ip (sort ipsort keys %{$ips{$date}}) {
      my $printIP = $ip;
      if ($arp{$date}{$ip} =~ /\w/) {
	foreach my $arpEntry (split /\s+/, $arp{$date}{$ip}){
	  printf
	    " %-17s %s %s\n",
	    $printIP,
	    $arpEntry,
	    ($arp2name{$arpEntry} ? $arp2name{$arpEntry} : "");  $printIP = " ";
	  ++$count{$ip};
	}
      }else{
	printf
	  " %-17s %s %s\n",
	  $printIP,
	  "***",
#	($date eq $today && `ping -c 1 -w 1 $ip|grep " 0% "` ? " ??? REPLIES PING":""),
	  unless $printIP =~ /10\./;
      }
    }
#    next;
#}
#
#foreach my $date (sort keys %arpReverse) {
    print "\n$date ARP -> IP table\n";
    foreach my $arp (sort keys %{$arpReverse{$date}}) {
      next unless $arpReverse{$date}{$arp} =~ /\s/;
      print "   $arp\t", $arp2name{$arp}, "\n";
      print         "\t",$arpReverse{$date}{$arp}, "\n" if $arpReverse{$date}{$arp};
    }
  }

print "****\n**** $badcount occasions of no lease\n**** $badline1**** $badline****\n"
  if $badline;

print "\n----- ",scalar(keys %count)," IP addresses were ever leased out of ",
  scalar (keys %DHCPpool), " in the pool\n\n";

if (0) {
  print "Logging the status ";
  foreach my $ip (sort keys %lastLease) {
    print ".";
    hostdbIt ($ip,
	      split (/\|/, $lastLease{$ip})
	      );
  }
}

print "\n";
`rm -f /tmp/DHCP.inc; touch /tmp/DHCP.inc`;
foreach my $arpEntry (sort keys %arp2name){
    (my $hdb = $arpEntry) =~ s/:/-/g;
    my $mess;
    if (my ($HOST,$aliases,$addrtype,$length,@addrs) = gethostbyname $arp2name{$arpEntry}) {
      next if $HOST =~ /^dhcp/;
      $mess = "/tmp/DHCP.inc";
      open   O,">>$mess" or die $!;
      printf O "
 host %s {
  hardware ethernet %s;
  fixed-address %s;
 }
", $arp2name{$arpEntry}, $arpEntry, inet_ntoa(@addrs);
      close O;
    }
    print "$hdb ", $arp2name{$arpEntry},
    ($mess ? " **** is in DNS, see $mess" : ""),
    "\n";
#    system ("/usr/local/bin/hostdb ".$arp2name{$arpEntry}." -add arp=$hdb&");
}
    exit;

sub addit {
  my ($list, $item) = @_;
  return if $list =~ /\b$item\b/i; 
  $_[0] = join "\t",sort(split(/\s+/,$list),lc $item);
}

sub hostdbIt {
  my ($ip, $arp, $name) = @_;
  $name =~ s/.(scfab|physto).se//;
  $arp = lc $arp;
  my $alive = `ping $ip -c 1`;    return unless $alive =~ / 0% packet loss/;
  (my $instantARP = `arp $ip`) =~ s/(\n|\r)//g; 
  unless ($instantARP =~ /no entry/) {
    my $m = '[a-fA-F0-9]+';
    my $iarp = lc $1 if $instantARP =~ /($m:$m:$m:$m:\S+)/;
    print "??? ARP mismatch $iarp <-> $arp\n"
      unless $arp eq $iarp ;
  }
  
  $arp =~ s/:/-/g;
  $ip =~ /\.(\d+)$/;
  my $host = sprintf "dhcp%03d", $1;
  
  (my $lastARP = `hostdb $host -if noupdate | grep " Arp " | awk "{print $1}"`) =~ s/\s+//g;
  return if lc($lastARP) eq lc($arp);
  
  $name = $name ? "=$name" : "";
  my $cmd = "hostdb $host -reset arp=$arp dhcp$name interface Device Memory Processor opsys -reset bench_w model best_up Firmware Setup pci_bus";
#    print "$cmd\n";
  system ("$cmd&");
}

sub ipsort {
  my @a = split /\./,$a;
  my @b = split /\./,$b;
  for (my $i=0; $i<4; $i++) {
    return   @a[$i] <=> @b[$i]
      unless @a[$i] ==  @b[$i];
  }
}







