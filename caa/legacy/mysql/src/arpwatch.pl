#! /usr/local/bin/perl
#
#     OBSOLETE
#                       YB 2000

use strict;
use snmp;
use sql;

my $debug = 0; 

my $ARPswap = '*** ARP address chaged';
my ($ARP,$ALIVE) = qw(arp seen_alive);

my (@oids, %hash, %ping, %counters, );

my $m   = '[\da-f]+[-:]'; my $i = '\d+\.';
my $PID = $$;

 main:{
     
     &sql::opendb('hostdb');
     &sql::setSilentlyUpdated ($ALIVE);
     
     @oids = &snmp::init ($ARP, $snmp::ifPhysAddress."1",
			  );
     
     
     foreach my $serie (qw(
			   130.237.205
			   130.237.208 
			   130.242.128 
			   192.168.205
			   )){
	 foreach my $n (1..254){
	     my $ip   = "$serie.$n";
	     my $host = lc nslookup($ip)  or next;	    $host =~ s/.physto.se//;  
	     
	     ++$counters{'Total IP addresses'};
	     
	     spawnIt (\&contactHost, $ip, $host);
	     select (undef, undef, undef, 0.25); # sleep(0.25);
	 }
     }
 }

$debug++;
if ($debug || $counters{$ARPswap}){
    print "\n";
    foreach my $key (sort keys %counters) {
	printf "\t%-20s %3d\n", $key, $counters{$key};
    }
    print "\n";
}


sub contactHost {
    my ($ip, $host) = @_;

    my ($hostDB, $ipDB, $arp, @rest) = dblookup ($host); 
    die "Something wrong in the db - 2 entries: @rest" if @rest;
    
    printf "\n%s...\n\t\t%-15s\t%s \n", $host, $ip, $arp if $debug;
    
    eval { %hash = snmp::getOK ($ip,
				'public',
				@oids); };
    ++$counters{'Replied SNMP'} if $hash{$ARP};
    
    my $arpActual = $hash{$ARP} || &getARPbyPing ($ip)  or return;
    return unless $arpActual =~ /^$m$m$m$m$m/;
    

    my ($s,$x,$h,$d,$M,$y,$wd) = localtime(); ++$M; $y += 1900;   
    my $tstamp = sprintf "%d-%2.2d-%2.2d", $y,$M,$d;
    
    &sql::set ($host, { $ARP   => $arpActual ,
			$ALIVE => $tstamp } );
    
    
    unless ($arp eq $arpActual) {
	++$counters{$ARPswap} if $arp;
	printf
	    "*** %-10s %-15s %18s -> %s\n",
	    $host, $ip, $arp, $arpActual;
    }
}


sub nslookup  { # get back DNS value
    my $ip = @_[0];    my $Name;
    open N, "/usr/bin/nslookup $ip -timeout=5 2>&1 |";
    while (<N>) { my ($id,$value) =  (/(\w+):\s+(.*)(\n|\r)/);	
		  $Name = $value if ($id eq 'Name');  }
    close N;
    return $Name; }


sub dblookup { # match IP ARP HOST in the databse
    my $matchValue = @_[0];
    my $field = 'host';
    if ($matchValue =~ /^$i$i$i/)        { $field = 'ip';  }
    if ($matchValue =~ /^$m$m$m$m$m/)    { $field = 'arp';}
    my %hash = ($field => $matchValue);
    return &sql::get (\%hash, undef, 'host', 'ip', 'arp'); }


sub getARPbyPing{
    my $node = @_[0];    my $mac;
    if (pingEcho($node)){
	open F, "arp  $node |";
	while(my $l=<F>){
	    my ($node,$ip,$d,$ifAddr,@r) = split /\s/,$l;
	    $mac = $ifAddr  unless $ifAddr eq 'no';
	} close F;
	print "getARPbyPing\t\t$node\t$mac\n"  if $debug;
    }
    return $mac;
}


sub pingEcho{
    my  $node = @_[0];  
    unless (exists $ping{$node}){ 
	my $reply = `ping -c 1 $node`;
	$ping{$node} = grep / 0% packet loss/, $reply;
    };
    print "pingEcho\t$node" ,($ping{$node} ? "  OK\n" : "\t.... no reply\n")  if $debug && !$ping{$node}; 
    my $key = ($ping{$node}
	       ? 'Replied ping'
	       : 'No responce');
    ++$counters{$key};
    return     $ping{$node};
}

sub spawnIt {
    die "\n" if $PID ne $$;    #    die "*** fork from fork\n" if $PID ne $$;    
    my $coderef = shift;
    defined (my $pid = fork) or die "*** Cannot fork: $!\n";
    return $pid   if $pid;   # I'm the parent
    exit &$coderef(@_);
}
1;








