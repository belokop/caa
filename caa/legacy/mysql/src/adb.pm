package adb;

use strict;
use time;

my $deepBug = 0;
my $debug   = 0;
my @DBs     = ('hostdb' , 'maildb');
my ($IDpeer , $ID_dop , $DBpeer);

my $SSH;
foreach my $s ("/usr/local/bin/ssh", "/usr/bin/ssh") { $SSH = $s if -e $s; }
die "*** No ssh found\n" unless $SSH;

&time::fmt(1);

sub init {
    my $hlo   = $_[0];
    $sql::iam = $$hlo{iam} || getlogin || (getpwuid($<))[0] || "Intruder";

    return (
	    undef,                # subCheckValue,
	    undef,                # \&subDaemonUpdate,    # daemon update
	    \&subMaint,
	    undef,
	    undef,
	    );
}

my $LL    = 'last_login';
my $D     = 'duration';
my $LOGAT = 'logon_at';
my $LOGFR = 'logon_from';

sub subMaint {
    my ($maintList , $usersList) = @_;
    
    $debug = $sql::debug + $main::debug; undef $sql::debug;
    if (($DBpeer) = ($maintList =~ /(\w+db)/)){
	# copy from adb to maildb/hostdb the last logons
	&sql::opendb ($DBpeer);
	&sql::setSilentlyUpdated ($LL, $LOGFR, $D, ) unless $deepBug;
		
	($IDpeer) = ($sql::primaries{"database:$DBpeer"}); # principal key of the peer DB    
	($ID_dop) = grep !/$IDpeer/, @DBs;      # hostdb <=> maidb dopolnenie do
	
	my @clients = $usersList
	    ? split /\s/,$usersList
	    : &sql::get (undef,undef,$IDpeer);
	foreach (@clients) { updatePeerLogins($_); }
	
    }else{ 

	# update adb itself 
	#      Go thru all hosts and log the logons

	&sql::opendb ('hostdb');
	&sql::setSilentlyUpdated ($LOGFR, $LL, $D) unless $deepBug;
	&sql::setSilentlyUpdated ($LOGAT)          unless $debug;
	
	my @nodes;
	if ($usersList){
	    @nodes = split /\s/, $usersList;
	}else{
	    
	  debug: if (0) { my %logs = ( &readlogPopImap('pop')  );
			  storeLogRec (\%logs); 
			  return 1;
		      }
	    
	    $sql::WHEREclause = join (' AND ', 
				      join (' OR ',
					    &sql::fqn('opsys').  " LIKE '%linux%'", 
					    &sql::fqn('opsys').  " LIKE '%unix%'",
					    ),
				      join (' AND ',
					    &sql::fqn('alias').  " = ''", 
					    &sql::fqn('service')." = ''", 
					    ),
				      );
				      @nodes = &sql::get (undef, undef, 'host');
	    
	  centralService: { my %logs = (
					&readlogPoolman($adb::poolmanlog) ,
					&readlogPopImap('pop') , 
					&readlogPopImap('pop.students'),
					);
			    storeLogRec (\%logs); }
	    
	  VMS:            { my %logs = &readlogVMS($adb::vmslog); 
			    storeLogRec (\%logs); }
	    
	}	    
	
      UNIX:               { my $x;
			    foreach my $node (sort @nodes ){
				my %hash = &readlogLast($node);
				storeLogRec (\%hash, $node);  
			    }}
    }
    return 1;
}

my @notStore = qw(root toor login);

sub storeLogRec { # store data got by a daemon to the adb
    my ($hash, $localhost) = @_;
    my ($userP , $cc);
    my $primkey = 'id';
    foreach my $key (sort keys %$hash) {
	my ($user, $Host, $ltime, $Lfrom, $duration, $service) = (split (/\:/,$key,2) ,
							       split (/\|/,$$hash{$key}));
	foreach my $host ($Host, $Lfrom) {
#	foreach my $host ($Host) {
	    my $lfrom = ($host eq $Lfrom) ? "" : $Lfrom;
	    $host    = uty::filter ($host);         $host =~ s/localhost/$localhost/g;
	    $lfrom   = uty::filter ($lfrom);	undef $lfrom if $lfrom =~ /^($host|localhost)$/;
	    my $u    = ($user eq $userP) ? "" : $user;	$userP = $user;	 
	    printf "  %-13s %-8s %s %-10s ", $u, $host, $ltime, $lfrom  if $debug;
	    
	    my %hash = (maildb => $user,
			hostdb => $host,
			);
#	    $hash{$LOGAT} = $ltime	if grep /^$user$/, @storeAll;
	    $hash{$LOGAT} = $ltime;
	    
	    my $f = &sql::fqn($LOGAT);
	    $sql::WHEREclause = "(TO_DAYS($f) - TO_DAYS('$ltime')) <= 0 ORDER BY $f";
	    my $ID = (&sql::get (\%hash, undef, $primkey))[0];
	    
	    $hash{$LOGAT}   = $ltime;
	    $hash{$LOGFR}   = $lfrom;
	    $hash{$D}       = $duration;
	    $hash{'auth'}   = $service;
	    
	    if ($ID) { &sql::set ($ID,  \%hash); print "\n"        if $debug;}
	    else     { &sql::addSimple (\%hash); print " ***new\n" if $debug;}
	}
    }
}


sub updatePeerLogins {
    my ($record) = @_;
    
    print "$record\n"  if $debug;
    my %llHash  = ($IDpeer => $record);
    my ($peerL) = &sql::get (\%llHash, undef, $LL);
    
    my %logons;
    my $fmt = 1;
    foreach my $entry (reverse sort
		       split(/~/,&getLogins($record)), # adb logins
		       split(/~/,$peerL)){
	
	$entry =~ s/\s+\d\d:\d\d(:\d\d)?\s+/ /;  # drop time, leave just date
	my ($date,$obj) = ($entry =~ /(\d\d\d\d-\d\d-\d\S+)\s+(\S+)/)
	    or next;
	
	$logons{lc $entry} = $entry;
	$fmt = uty::max ($fmt, length $obj) if $entry =~ /\<-/;
    }

    my %logsUniq;
    foreach my $k (reverse sort {$logons{$a} cmp $logons{$b} } keys %logons) {
	my ($date, $obj, $sep, $obj2, @rest) = split /\s+/, $logons{$k};
	$obj = sprintf ("%s %-".$fmt."s %s", 
			$date, &uty::filter($obj), 
			$sep." ".& uty::filter($obj2)." @rest");
	next if $logsUniq{$obj};
	$logsUniq{$obj} = $obj;
	print "\t", $obj, "\n" if $debug;
    }   
    
    %llHash = ($LL => join ('~',
			    reverse sort values %logsUniq));
    &sql::set ($record, \%llHash);
}    


sub getLogins { # get 3 fields from adb (time, host/user, log-from) for hostdb/maildb
    my ($record) = @_;

    my %eqH  = ($DBpeer => "$record");

    my @f = ( $LOGAT, $ID_dop, $LOGFR);
    my @adump = &sql::get (\%eqH, undef, @f);
    
    my @logins;  
    for (my $n=0; $n<@adump; $n +=@f ){
	$adump[$n  ] =~ s/\s00:00:00//;
	$adump[$n+1] =  sprintf "%-".$sql::maxlength[1]."s <- %s", @adump[$n+1..$n+2]  
	    if $adump[$n+2] =~ /\w/;
        push @logins , $adump[$n] . ' ' . $adump[$n+1]; 
    }
    return join '~', reverse sort @logins;
}



sub readlogLast {
    my $node = $_[0];
    printf "Read 'wtmp' log from %-10s ... ", $node;  print "\n" if $debug;
    return if emu::warning ("Not in DNS tables",
			    !(uty::nslookup($node))[0]);
    my $open = open F,"$SSH $node /usr/bin/last |";
    return if emu::warning ($!,
			    !$open);
    my (%wtmp , $badLine);
    while (chomp(my $line=<F>)){
	my ($user,$tty,@rest) = split /\s+/,$line;
	next if $user  =~ /\W/ || !$user || length $line < 55;
	my $logtime    = &time::T (substr $line,39,17);
	my ($duration) = ($line =~ /still logged/i
			  ? time::D(time::diff($logtime))
			  : ($line =~ /\(([\d\+\:]+)\)/i));
	$duration =~ s/(\d+:\d+):\d+$/$1/;  $duration =~ s/\+/ /g;
#	my @key = ($user, $node);  push @key, $logtime if grep /^$user$/,@storeAll;
	my @key = ($user, $node, $logtime);
	my $key = lc join ':',@key;
	next if $wtmp{$key};

	$wtmp{$key} = join ('|', 
			    $logtime,
			    fixDomain(substr $line,22,17),
			    $duration,
			    );
	$wtmp{$key} =~ s/\s?\|\s?/\|/g;
    }
    close  F;
    print scalar(keys %wtmp), " entries\n"; 
    return %wtmp;
}

my $logarea = "/var/adm/syslog.dated";
my ($s,$m,$h,$d,$M,$year,$wd) = localtime(); ++$M; 
$year += 1900;  

# read mail.log fules from the remote node
# return the most recent entries for ipop3d and imapd servers
sub readlogPopImap {
    my $node = $_[0];

    my (%files, $area);
    eval { $area = `$SSH $node ls $logarea`; };
    foreach my $file (grep /^\d+-/, split (/\n/, $area)){
	my ($d,$t) = ($file =~ /(\d+-\w+)-(.*)/);
	$files{time::T("$d-$year $t")} = $file;
    }

    my (%pop , %lastRecordedTime);
    foreach my $key (reverse sort keys %files){
	my $rfile = "$logarea/".$files{$key}."/mail.log";
	print "Read pop/imap log on $node - $rfile\n";
	
	open MAIL, "$SSH $node cat $rfile |" or print "*** Shit ... $!";
	while (<MAIL>){
	    next unless /(login|auth(enticated)?) user/i;
	    s/^(\S+)  (\d) (.*)$/$1 0$2 $3/; # ' 6' -> '06'
	    my ($time,$host,$daemon,$user,$fromHost) =
		(/(\S+ \S+ \S+) (\S+) (ipop|imap)3?d.* user=(\S+) host=(\S+) .*$/i);
	    next unless $fromHost;
	    
	    $time   = &time::T($time);
	    $daemon =~ s/ipop/pop/;
	    my $key = "$user:$daemon";
	    next unless $lastRecordedTime{$key} le $time;
	    
	    $pop{$key} = join '|', $time, &fixDomain($fromHost);
	    $lastRecordedTime{$key} = $time;
	}
	close MAIL;
    }
    return %pop;
}


sub readlogPoolman {
    my $poolmanlog = $_[0];
    print "Read poolman log file ... ";
    return if emu::warning ($!,
			    !(open F, $poolmanlog));
    my %poolman; 
    while (chomp(my $line=<F>)){
	my ($date,$time,$node,$ucur,$user) = (split /:/,$line)[3..7]; 
	next if $ucur ne 'login';
	$node =~ s/ppp/poolman/;
	$poolman{"$user:$node"} = time::T ("$date $time")
	    unless $node =~ /hook/;
    }
    print scalar(keys %poolman), " entries\n"; 
    return %poolman;
}


sub readlogVMS {
    my $vmslog = $_[0];
    print "Read VMS log file ... ";
    return if emu::warning ("$vmslog: $!",
			    !(open F, $vmslog));
    my %vms;
    while (chomp(my $line=<F>)){
	my ($user,$date,$time,@rest) = split /\s+/, lc $line; 
	$vms{"$user:VMS"} = time::T ("$date $time") 
	    if $time =~ /\d\d:\d\d/;
    }
    print scalar(keys %vms), " entries\n"; 
    return %vms;
}


my $domain = 'physto.se';
sub fixDomain { # .physt -> .physto.se 
    (my $host = $_[0]) =~ s/\s+//g;
    my $l = length $domain;
    while ($l>=0) {
	my $d = substr $domain, 0, $l--;
	next if !($host =~ /\.$d\s?$/);
	$host =~ s/\.$d\s?$//;
	return $host;
    }
    ($host = $_[0]) =~ s/\s+//g;
    return $host;
}

1;
