#! /usr/local/bin/perl

use strict;
use snmp;

my $debug = 0;
&snmp::Deblev($debug);

my $defNinterfaces = 0;  # interface OIDs
my ($IFDESCR, $IFADDR, $IFTYPE, $IFSPEED, $MEMORY) =
    qw (desc addr type speed memory);
my ($FWDESCR, $FWREV, ) = qw (fwdesc fwrev);
my ($OS, $UPTIME, $MHZ, $MODEL) = qw (os uptime speed model);

my %answer; 
 snmpInitialisation:{
     my %oids = (
		 $OS          => $snmp::sysDescr,
		 $MODEL       => $snmp::svrSystemModel,
		 $MEMORY      => $snmp::svrPhysicalMemorySize,
		 $MHZ         => $snmp::svrCpuSpeed."4",          #  why 4 ???
#		 $USER        => $snmp::sysContact,
#		 $IFNUMBER    => $snmp::ifNumber,
		 $UPTIME      => $snmp::sysUpTime,
		 );
     
     my %interfaces;
     foreach (1..$defNinterfaces) {
	 my $n = $_ > 9 ? "$_" : "0$_";	     $n = "" if ($n eq "01");
	 $interfaces{$IFDESCR."$n"} = $snmp::ifDescr       . "$_"; 
	 $interfaces{$IFADDR. "$n"} = $snmp::ifPhysAddress . "$_";
	 $interfaces{$IFTYPE. "$n"} = $snmp::ifType        . "$_";
	 $interfaces{$IFSPEED."$n"} = $snmp::ifSpeed       . "$_"; 
     }

    my %firmware; # multiply firmware OIDs
    foreach (1..9) {
	my $n = ($_ eq '1') ? "" : "$_";
	$firmware{$FWREV  ."$n"} = $snmp::svrFirmwareRev   . "$_"; 
	$firmware{$FWDESCR."$n"} = $snmp::svrFirmwareDescr . "$_"; 
    }
 
     eval { %answer = snmp::getOK (@ARGV[0] || 'ices01',
			 'public',
			 &snmp::init(%oids, %interfaces, %interfaces, %firmware));
		     };
 }

foreach my $n (0..$#snmp::response){
    print
	@snmp::request[$n], "\t", 
	@snmp::response[$n], "\n";
}

exit;
print "-----------\n";

foreach (sort keys %answer) {
    next unless $answer{$_};
    print "$_\t", $answer{$_}, "\n";
}

exit;
