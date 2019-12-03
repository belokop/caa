package hostdb;

use strict;
use snmp;
use Net::DNS;

my @domains = qw(physto.se
		 scfab.se
		 );
my $OS      = 'opsys'; 
my $MODEL   = 'model'; 
my $ALIASES = 'aliases';
my $SERVICE = 'service'; 
my $SERVICES= 'services'; 
my $MEMORY  = 'memory'; 
my $IP      = 'ip';
my $ARP     = 'arp';
my $BEST    = 'best_uptime';
my $SEEN    = 'seen_alive';
my $UPTIME  = 'uptime'; 
my $IFNUMBER= 'ninterfaces'; 
my $FW      = 'fw'; 
my $IF      = 'if'; 
my $TS      = 'timestamp';
my $DHCP    = 'dhcp';
my $DEVICE  = 'device';
my $INTERFACE='interface';

my $STATUS = 'status'; 
my $DESCR  = 'descr'; 
my $ADDR   = 'addr'; 
my $TYPE   = 'type'; 
my $SPEED  = 'speed'; 

my $NOSNMP  = 0;
my $CLEANIP = 0;

@hostdb::oids = ();
$ns::s++; # Supress LOCAL domain name in the host output

sub init {
  my ($hlo, $if) = @_;
  $sql::iam = $$hlo{'iam'} || getlogin || (getpwuid($<))[0] || "Intruder";
  &sql::setSilentlyUpdated (sql::abbrFields('SKIP', 
					    $IFNUMBER,
					    $SERVICE, $SERVICES, $MODEL, $SEEN, 
					    $ALIASES, $BEST, $TS,
					    qw (
						manuf pci device
						setup edt deliver 
						last bench_w ?interface 
						host changeslog
					       )));
  
  @hostdb::oids = &initSNMP();
  ++$snmp::debug if $$if{'debug'};
  ++$NOSNMP      if $$if{'nosnmp'};
  ++$CLEANIP     if $$if{'cleanip'};
  ++$NOSNMP      if `uname -s`=~/Linux/;
  
  return (
	  \&subCheckValue,
	  \&subDaemonUpdate,
	  \&maint,
	  \&subSortFields,
	  \&subPrtField,
	  \&subTreatArgument,
	 );
}

sub subTreatArgument {
  my ($arg, $regexp) = @_;
  my ($sx, %hash, %answer);

  # Check if the argument is a mac address
  if ($arg =~ /^(\w+[:-]\w+[:-]\w+[:-]\w+[:-]\w+[:-]\w+)/) {
    ($main::ARGSmac = lc $1) =~ s/-/:/g; # dirty, but reliable...
    return $main::ARGSmac;
  }
  
  # First try the exact match
  foreach (split/\s+/,$arg) {
      $sx = $sql::DBH->query("SELECT host FROM master WHERE host='$_'");
      ++$answer{$hash{'host'}}   if %hash=$sx->fetchhash();
  }
  return (join ' ',sort keys %answer) if %answer;

  # Then try the (regular) expression
  my ($like,$d) = $regexp
      ? ('RLIKE','')
      : ( 'LIKE','.%');
  foreach (split/\s+/,$arg) {
      $sx = $sql::DBH->query("SELECT host FROM master WHERE host $like '$_$d'");
      while (%hash = $sx->fetchhash){ ++$answer{$hash{'host'}}; }
  }
  return (join ' ',sort keys %answer) if %answer;

    
      # get either the A-record from DNS or just the parameter typed
    foreach my $a (split/\s+/,$arg) {
      my $host = (&ns::gethost($a))[0] || $a;
      $answer{$host}++;
    }
  return (join ' ',sort keys %answer) if %answer;
}

sub subDaemonUpdate { # update values from DNS,  SNMP and/or PING
  my ($host) = @_;
  my ($NShost, $NSip, @NSaliases) = &ns::gethost($host);

  if ($NShost eq $host) {
      
      my ( %update, %update1, %update2, %update3);
      
      %update1 = &tryPing($host);
#      foreach my $k (sort keys %update1) { print "update $k\t",$update1{$k},"\n";}
      
      %update2 = &trySNMP($host) if %update1 && !$NOSNMP; &emu::warning("Can\'t contact SNMP daemon $host",
									!$NOSNMP && !%update2) if %update1;
      %update3 = &tryDNS ($host);
    
      %update = ((
		  aliases => join (' ', @NSaliases),
		  services=> join (',',sql::getAnyField ($SERVICE,$host, 'host')),
		  ), %update1, %update2, %update3 );
      
      if ( $update{$SERVICE} ) { # || &sql::getField($host,$SERVICE) ) 
	  ;
#       foreach my $c ($MEMORY, $IFNUMBER, $OS, $MODEL, $SEEN, 'firmware', 'interface') {
#	foreach my $c (keys %sql::tables){
#	    next if $c =~ /^(host|$SERVICE|.*\..*)$/;
#	    next if $c =~ /^($IP|arp)$/ &&  $update{$SERVICE};
#	    delete $update{$c};
#	    $update{$c} = '' if &sql::getField($host,$c);
#	}
      }else{
	  $update{$SERVICE} = '';
      }
      &sql::set ($host,
		 \%update,
		 \&subCheckValue,);
  } else {
      if ( $CLEANIP ) {
	  &sql::delete ($host, 
			\&subCheckValue);
      }else{
	  &sql::set    ($host,
			{ 
#			    $IP      => '' ,
			    $ALIASES => '' ,
			    $SERVICE => '' ,
			    $SERVICES=> '' , 
			},
			\&subCheckValue,
			'unset');
      }
  }
}

sub subSortFields {
    return sort asMail @_;
    sub asMail {
	my $first = "$SERVICE|$MODEL|$OS|status";       # The first is FIRST printed
	my $last  = "$TS|edt|changeslog|seen|best_uptime|type"; # The first is LAST printed
	return -1 if $a=~/^($first)$/i; return +1 if $b=~/^($first)$/i; 
	return +1 if $a=~/^($last)$/i;  return -1 if $b=~/^($last)$/i; 
	return $a cmp $b; } 
}


sub subPrtField {
    my ($ID, $field, $value, $hash) = @_;
    return ('','') if $field =~ /^(host)$/i;
    return ('','') unless $value;
#    my $host = $$hash{$ID};    
    $field =~ s/_/ /g;
    return ($field, $value);
}

sub subCheckValue {
    my ($record, $column, $newValue, $hash, $flag) = @_;     return undef unless $record;
    print "subCheckValue - $record:$column\t'",sql::shortMess($newValue),"'\t$flag\n"  if $sql::debug;

    return undef if &emu::warning("Can\'t delete active host '$record'",
				  $flag=~/delete/                 &&
				  $column eq &sql::getID($column) &&
				  &ns::gethost($record));
#				  &uty::nslookup($record) );

    return $newValue;
}

sub maint {
  foreach (split /\s+/, $_[1]){ &subDaemonUpdate($_); }
}

my $nFW = 6;
my $nIF = 5;
sub initSNMP {
  my $manyIF = $_[0];
  my %sysname = (
		 sysName   => $snmp::sysName,
		 sysDescr  => $snmp::sysDescr,
		 );
  my %system = (
		$OS          => $snmp::sysDescr,
#	        $MEMORY      => $snmp::svrPhysicalMemorySize,
		$MEMORY      => $snmp::hrMemorysize,
#	        $MHZ         => $snmp::svrCpuSpeed."4",          #  why 4 ???
		$IFNUMBER    => $snmp::ifNumber,
		$UPTIME      => $snmp::sysUpTime,
		);
  
  my ( %interfaces , %firmware);
  foreach (1..$nFW) {
    $firmware{"$FW-$TYPE-$_"}  = $snmp::svrFirmwareRev   . "$_"; 
    $firmware{"$FW-$DESCR-$_"} = $snmp::svrFirmwareDescr . "$_"; 
  }	
  
  my $n = $manyIF || $nIF;
  foreach (1..$n) {
    $interfaces{"$IF-$DESCR-$_"} = $snmp::ifDescr       . "$_"; 
    $interfaces{"$IF-$ADDR-$_"}  = $snmp::ifPhysAddress . "$_";
    $interfaces{"$IF-$TYPE-$_"}  = $snmp::ifType        . "$_";
    $interfaces{"$IF-$SPEED-$_"} = $snmp::ifSpeed       . "$_"; 
    $interfaces{"$IF-$STATUS-$_"}= $snmp::ifOperStatus  . "$_"; 
  }
  
  my %iptable;
  
  $iptable{1} = $snmp::ipAdEntAddr.        "130.237.205.2"; 
  $iptable{2} = $snmp::ipAdEntAddr.        "130.237.205.2"; 
  $iptable{3} = $snmp::ipAdEntIfIndex.     "130.237.205.2"; 
  $iptable{4} = $snmp::ipAdEntNetMask.     "130.237.205.2"; 
  $iptable{5} = $snmp::ipAdEntBcastAddr.   "130.237.205.2"; 
  $iptable{6} = $snmp::ipAdEntReasmMaxSize."130.237.205.2"; 
  return &snmp::init(%system,
		     %sysname,
		     %interfaces,
		     %firmware,
		     %iptable,
		     );
}

%main::snmpDead = ();
sub trySNMP {
  my $host = $_[0]; return if $main::snmpDead{$host};
  
  my %snmpAnswer = snmp::getOK ($host,
				'public',
				@hostdb::oids);
#  if ($snmp::debug) {
#    foreach my $k (sort keys %snmpAnswer){
#      printf "SNMP answer: %-13s %s\n", $k, $snmpAnswer{$k} if $snmpAnswer{$k};
#    }
#  }

  unless (@snmp::response) {
    $main::snmpDead{$host}++;
    return;	
  }

    (my $sysDescr =    $snmpAnswer{sysDescr})      =~ s/.$hostdb::localDomain//i;
    (my $sysName  = lc $snmpAnswer{sysName}||$host)=~ s/.$hostdb::localDomain//i;
    my $dnsName   = (&ns::gethost($host))[0];
    my $dhcp      = (my $status = &sql::getField($host,'status')) =~ /dhcp/i;
#    print "host     $host\nsysName  $sysName\nsysDescr $sysDescr\ndnsName  $dnsName\n", $snmpAnswer{$OS}, " <-----\n";

    my %datahash = ();    
    
  system: {
      if ($dnsName ne $host && $sysName eq $dnsName) { # Alias
	print "???????? $dnsName alias  DEBUG\n";
	  $datahash{$SERVICE}= '';
	  return %datahash;
      }
      
      if ("$sysName.$hostdb::localDomain" ne $dnsName && $sysName ne $dnsName && !$dhcp){ # this is a service, not a computer
	 print "???????? $dnsName - $sysName service  DEBUG\n";
	  $datahash{$SERVICE} = $sysName;
	  return %datahash;
      }
      
      #	  foreach my $key ( $MEMORY, $IFNUMBER, ){
      foreach my $key ( $MEMORY, ){
	$datahash{$key} = $snmpAnswer{$key} if $snmpAnswer{$key};
      }
      $datahash{$MODEL} = $snmp::hard if $snmp::hard;
      $datahash{$OS}    = $snmp::soft || $snmpAnswer{$OS} || $sysDescr;
      $datahash{$DHCP}  = $sysName if $dhcp;
      if (my $upt = $snmpAnswer{$UPTIME}){
	my ($nd) = ($upt =~ /(\d+) day/);
	  (my $t = &sql::getField($host,$BEST)) =~ s/\D.*//;
	  $datahash{$BEST} = "$nd days~".$sql::time if $nd-$t>0; # $nd > $t does not work...
      }
    }
    
  firmware: {
      for (my $n=1; $n < $nFW; $n++) {
	my %item;
	foreach my $key (grep  /$FW-.*-$n/, keys %snmpAnswer) {
	  my ($f) = ($key =~ /$FW-(\w+)-/);
	  $item{$f} = $snmpAnswer{$key} if $f && $snmpAnswer{$key};
	} next unless %item;
	
	my @if = ($item{$DESCR} . " " . $item{$TYPE});
	unshift @if, $datahash{firmware}  if $datahash{firmware};
	$datahash{firmware} = join '~',@if;
      }  
    }  
    
  interfaces: { 
      # List Ethernet interfaces
      # This piece of code should be at the end, 
      # since SNMP might be contacted again...
      $datahash{$IFNUMBER} = 0;
      
      my $separ = '~-';    
      my $m = '[\da-f]+[-:]'; # filter out non-ethernet interfaces
      $snmpAnswer{$IFNUMBER} = 50 if int $snmpAnswer{$IFNUMBER} >= 9;
      my $emptyIF = 0;
      my %IF; # Not to print many times the same info

      for (my $n=1; $n <= $snmpAnswer{$IFNUMBER}; $n++) {
	my (%item, $addr);
#	print "=$n\n" if $snmp::debug;
	foreach my $key (grep  /$IF-.*-$n$/, keys %snmpAnswer) {
	  my ($f) = ($key =~ /$IF-(\w+)-/);
	  $item{$f} = $snmpAnswer{$key} if $f && $snmpAnswer{$key};
	  ++$addr                       if $item{$f} =~ /^$m$m$m$m$m/;
#	  printf "--%6s: %s\n", $f,$snmpAnswer{$key}  if $f && $snmpAnswer{$key} && $snmp::debug;
	} 
	++$emptyIF unless $addr || %item;
	next       unless $addr;
	
	foreach (keys %item) { # Delete repetetive items
	    delete $item{$_} if    $item{$_} eq $IF{$_}; 
	    delete $item{$_} if lc $item{$_} eq $INTERFACE; 
	    $IF{$_} =              $item{$_} if $item{$_} && !/status/;
	}
	
	my @if;
	foreach (sort sortIF keys %item) { push @if, sprintf ("%-7s %s",$_.':', $item{$_}); } 
        unshift @if, $datahash{$INTERFACE}.$separ  if $datahash{$INTERFACE};
	
	$datahash{$INTERFACE} = join '~',@if;
	$datahash{$ARP}       = $item{$ADDR}  unless $datahash{$ARP};
	$datahash{$IFNUMBER}++; # Count only Ethernet if
      }
      
      $datahash{$IFNUMBER} = 0         if $datahash{$IFNUMBER} < 2;
      $datahash{$IFNUMBER} = $datahash{$IFNUMBER} + $emptyIF if $snmpAnswer{$IFNUMBER} >= 9;
      $datahash{$ARP}      = ''        if $datahash{$IFNUMBER};
      $datahash{$ARP}      = ''; # Do it without snmp 
      
      
    compactAbit:{ my @f = split /$separ~/,$datahash{$INTERFACE};
		  my $Desc;
		  foreach my $n (0..$#f) {
		      my @sa = split /\~/ , $f[$n];
		      my ($desr) = grep /^$DESCR:/, @sa; $desr =~ s/^\S+\s//;
		      $Desc = $desr || $Desc;
		      $desr = $desr || $Desc;
		      @sa = grep !/^$DESCR:/, @sa; unshift @sa, "$separ $desr";
		      $f[$n] = join '~', @sa;
		  }
		  $datahash{$INTERFACE} = join '~', @f if @f;
	      }
  }
    $datahash{$SEEN} = $sql::time;
    return %datahash;
    sub sortIF { return 1 if $b eq 'status'; return -1 if $a eq 'status'; $a cmp $b; }
}

my %ping;
sub tryPing {
  my $host = $_[0];  
  return if &emu::warning("Can\'t ping $host",
			  (my $r=`ping -c 1 -w 1 $host 2>&1`) !~ / 0% /);			
  
  my %datahash = ( $SEEN => $sql::time); 
  open F, "arp  $host |";       
  while (chomp(my $l=<F>)){
    foreach my $item (split /\s/,$l){
      $item =~ s/\:/-/g; 
      $datahash{$ARP} = lc $item
	if $item =~ /[a-fA-F0-9]+\-[a-fA-F0-9]+\-[a-fA-F0-9]+\-[a-fA-F0-9]+\-[a-fA-F0-9]+/i;
    }
  } close F;
  delete $datahash{$ARP}; # Do not set ARP, better do it with a separate script
  return %datahash;
}

sub tryDNS {
  return;
  my $host = $_[0];
#   my ($name, $ip, @aliases) = &uty::nslookup($host);
  my ($name, $ip, @aliases) = &ns::gethost($host);
  
  my %datahash;
  if ($name eq $host) {          $datahash{$IP}    = $ip; }
#    $datahash{$IP} = $ip  
#	if $name eq $host && !sql::getField($host,'ip');
  return %datahash;
}


package ns;

my (%a2ip, %ip2a, %c2a, %a2a, %dejavu, %allAliases);
sub init {
  return if %a2ip;
  foreach my $domain (@domains) {
    my $resolver = new Net::DNS::Resolver;
    $resolver->nameservers($hostdb::nameServer);
    foreach my $rr ($resolver->axfr($domain)) {
      if    ($rr->type eq "A")     { my $ip = $rr->address;
				     $a2ip{lc $rr->name}    = $ip;
				     $ip2a{"$ip"}           = lc $rr->name; 
#				     print $ip2a{$ip}," <- ", $ip, "\n";
				     $a2a {lc $rr->name}    = lc $rr->name; }    
      elsif ($rr->type eq "CNAME") { $c2a{ lc $rr->name}    = lc $rr->cname; 
				     $allAliases{lc $rr->cname} .= lc $rr->name." "; }
    }
  }
}

sub gethost {
  init($_[0]);
  my ($name,$domain) = @_;
  my ($DNSname, $ip, $host); 
  
  for my $domain (@domains){
    $DNSname =
      $ip2a{$_[0]} || 
      $a2a{$_[0]} || 
      $ip2a{$name} || 
      $a2a{$name}  || 
      $a2a{$name.".$domain"} || 
      $ip2a{$a2ip{$name}} ||
      $ip2a{$a2ip{$name.".$hostdb::domain"}} || 
      $c2a{$name} ||
      $c2a{$name.".$hostdb::domain"} 
    ;
    $ip   = $a2ip{$DNSname} && last;
  }
  unless ($ip) {
    chomp ($ip = `host $name 2>/dev/null|grep address|sed "s/.*address //"`);
    $DNSname   = $ip2a{$ip};
  }
  $host = $DNSname;
  $host =~ s/.$hostdb::localDomain//g                 if $ns::s;
  $allAliases{$DNSname} =~ s/.$hostdb::localDomain//g if $ns::s;
# print "gethost:  $name ($domain) -> '$host $ip ", &unique($allAliases{$DNSname}), "'\n";
  return ($host, $ip, &unique($allAliases{$DNSname}));
}

my $hostNumber;
sub gethostent {  # get next host
  my ($name, $c);
  foreach my $entry (sort keys %ip2a) { $name = $entry; last if  $c++ == $hostNumber; }
  if (++$hostNumber > scalar keys %ip2a){
      $hostNumber = 0;
      return undef;
  }
  return &gethost ($name);
}

sub gethostlist { # Regular expression on input, list of A records on outpur
    my $regexp = $_[0];
    my %list   = ();
    init ($hostdb::localDomain);
    foreach my $item (keys %c2a, keys %ip2a, keys %a2ip){
	if ($item =~ /$regexp/){ 
	    my ($host, @rest) = gethost ($item);
	    $host =~ s/.$hostdb::localDomain//;
	    $list{$host}++ if $host; 
	}
    }
    return unique (keys %list);
}

sub unique { my %L; foreach my $i (@_){ $L{$i}++ if $i;} return join (' ',sort keys %L); }

sub prt {
    foreach (sort ipsort keys %ip2a) { printf "%-16s %-22s %s %s %s \n", $_, $ip2a{$_}; }
    foreach (sort   sort keys %c2a)  { printf "%-16s %-22s %s\n", $_, $c2a{$_}; }
}

sub ipsort {
    my @a = split /\./, $a;
    my @b = split /\./, $b;
    for my $i (0..3) { return $a[$i] <=> $b[$i] unless $a[$i] == $b[$i]; }
}

1;







