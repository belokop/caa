#! /usr/local/bin/perl

use strict;
use uty;

my ($DBdump, $iddef) = @ARGV;
#  ($DBdump, $iddef) = ("hosts.db.dump" , 'host') unless $iddef;
die "*** Wrong parameters @ARGV \n" unless $iddef;


my %FMaster      = (
		    id       => "SMALLINT UNSIGNED AUTO_INCREMENT",
		    );

my %FPeople      = (
		    id        => "SMALLINT UNSIGNED AUTO_INCREMENT",
		    von       => '',
		    name      => '',
		    namefam   => '',
		    position  => '',
		    status    => ":ENUM('', 'Active', 'Passive', 'Expired')",
		    address   => '',
		    homephone => '',
		    pers_number=> '',
		    );

my %FOfficies    = (
		    id        => "SMALLINT UNSIGNED AUTO_INCREMENT",
		    office    => '',
		    phone     => '',
		    );

my %AMaster      = (
		    id        => "SMALLINT UNSIGNED AUTO_INCREMENT",
		    hostdb    => '',
		    maildb    => '',
		    duration  => '',
		    "logon_at"=> ':DATETIME',
		    "logon_from"=> '',
		    );

my %dnsdbMaster  = (
		    id        => "SMALLINT UNSIGNED AUTO_INCREMENT",
		    soa       => 'SOA',
		    ns        => 'NS',
		    ip        => 'A',
		    mx        => 'MX',
		    rtp       => 'PTR',
		    cname     => 'CNAME',
		    );


my %maildbMaster = (
		    changeslog=> ':TEXT',
		    edt       => ':DATE',
		    uid       => ':SMALLINT UNSIGNED',
		    gid       => ':SMALLINT UNSIGNED', 
		    uic       => '',
		    poolman   => '',
		    expiration=> ':DATE',
		    dependant => '',  # link to responsable (how ???)
		    cmt       => '',
		    maildrop  => '',
		    mailname  => '', # preferred e-mail address
		    altname   => '',
		    altacct   => '',
		    responsible=> '',  # peroson responsible for the account
		    'last-login'=> ':TEXT',
		    );


my %hostdbMaster = (
		    acco      => 'account',
		    model     => '',
		    sn        => '',
		    type      => '',
		    ankt      => "delivery:DATE",
		    kr        => 'price:MEDIUMINT(7)',
		    status    => ":ENUM('', 'off','RIS client')",
		    mem       => 'memory',
		    mhz       => 'speed_mhz:MEDIUMINT(5)',
		    bench_chw => '',
		    cmt       => '',
		    edt       => ':DATE',     
		    );

my %hostdbNet    = (
		    henet     => 'arp',
		    alias     => '',
		    aliases   => '',
		    service   => '', 
		    services  => '', 
		    penet     => '',
		    ip        => '',
		    eth0      => ':VARCHAR(100)',
		    tu0       => ':VARCHAR(100)',
		    tu1       => ':VARCHAR(100)',
		    tu2       => ':VARCHAR(100)',
		    tu3       => ':VARCHAR(100)',
		    interface => ':TEXT',
		    ninterfaces => ':SMALLINT',
		    );


my %hostdbStatus = (
		    user     => "",
		    room     => "",
		    netgroup => ":ENUM('','auxaxp','bubaxp','fopaxp','grubaxp',".
		                         "'iceaxp','molaxp','qclaxp','qcpaxp',".
		                         "'sysaxp','bublx','syslx','snlx')",
		    opsys    => "",
		    firmware => "",
		    setup    => ":DATE",
		    changeslog      => ":TEXT",
		    'seen-alive'    => ":DATE",
		    'best-uptime'   => "",
		    'last-login'    => ":TEXT",
		    );

my ($crazy, %unused, $recordO, %line, );
my @vons = ('de los', 'a', 'de', 'al', 'von', 'di', 'le', 'Mc', 'e');
my $vons = join '|', @vons;

my @dbs = ($DBdump =~ /host/) 
    ? qw( hostdb dnsdb adb )
    : qw( maildb adb );
print "\nRead from $DBdump\n\n";

for my $db (@dbs){
    system ("touch data/$db.DEF");
    open  DEF, ">>data/$db.DEF"    or die $!;
    print DEF  "use $db;\n";
    
    my @f = 'master';
    @f = qw(master.drift.net)                    if $db eq 'hostdb';
    @f = qw(master.officies.people)              if $db eq 'maildb';
    @f = qw(master officies people)              if $db eq 'fysikum';
    
    foreach my $table (@f) { 
	convert ($db, $table);
    } 
    close DEF;
} 
    
foreach my $key (sort keys %unused){
    print "**** Unused $key ",$unused{$key}, " cases\n";
}
print "Crazy lines $crazy\n" if $crazy;

exit;  

sub convert {
    my ($db, $Table) = @_;

    my ($table, %lookFor,
	$count, %format, %lng, );
   
    foreach my $t (reverse split /\./,$Table){    
	my %l;
	%l = %AMaster        if $t eq 'master' && $db=~/adb/;
	%l = %FMaster        if $t eq 'master' && $db=~/fysikum/;
	%l = %FPeople        if $t eq 'people';
	%l = %FOfficies      if $t eq 'officies';
	%l = %maildbMaster   if $t eq 'master' && $db=~/mail/;
	%l = %dnsdbMaster    if $t eq 'master' && $db=~/dns/;
	%l = %hostdbMaster   if $t eq 'master' && $db=~/host/;
	%l = %hostdbNet      if $t eq 'net';
	%l = %hostdbStatus   if $t eq 'drift';
	foreach my $key (keys %l) { $lookFor{$key} = $l{$key}; }
	$table = $t;
	die "*** Unknown table '$table'\n" unless %lookFor;
    }    

    foreach my $key (keys %lookFor){
	my ($name,$f) = split /:/,$lookFor{$key};
	$name = $key   unless $name;
	$name =~ s/-/_/g;
	$format{$name} = $f if $f;
	$lookFor{$key} = $name;
    }

    unless ($db =~ /fysikum|adb/)  { undef $lookFor{id}; undef $format{id}; }
    
    my $lookFor = join '|',keys %lookFor;
    $lookFor = 'last.login' 	if $db =~ /adb/;

    my $lookAll = join 
	'|',
	keys %hostdbMaster,keys %hostdbNet,keys %hostdbStatus,
	keys %dnsdbMaster;
    $lookAll = join
	'|',
	keys %maildbMaster,
	keys %FMaster,     keys %FPeople,   keys %FOfficies
	    if  $db=~/mail|fysikum/;
    $lookAll =~ s/\-/\./g;
    $lookFor =~ s/\-/\./g;
    
    my $id = $lookFor{id} ? 'id' : $iddef;
    $format{$id} = $lookFor{id} if $lookFor{id};

    print
	"Create table '$db.$table'\n",
	"\tKey    '$id'\n",
	"\tFields '$lookFor'\n";
    print
	"\tFormat '$format{$id}'\n" if $format{$id};


    open  F, $DBdump or die "*** Cant read $DBdump: ".$!;

    system ("touch data/$db.$table");
    open  O, ">>data/$db.$table" or die $!;
    print O
	"use $db;\n";
    
    %line = ();

    while (<F>){
	my ($record,$key,$value) = (/(\S+):(\S+) (.*)(\r|\n)$/); 
	next unless $record =~ /^[\w\.\-]+$/;

	unless ($key=~/^($lookFor|fullname)$/) { $unused{$key}++ unless $key=~/^($lookAll)$/; next; };
	next if $key eq 'interface';

	my $tab = '<-';
	my $s = '(<cr>|<br>|##)';
	$value =~ s/(\s+)?$tab(\s+)?/ $tab /ig;
	$value =~ s/(\s+)?$s(\s+)?/~/ig;
	$value =~ s/[\s\']+/ /g;
	$value =~ s/[ ]+//g;

	if (length $value > 10000) { $crazy++; next; }

	my @item;
	foreach my $item (split /\~/, $value){
	    if (my ($date) = ($item =~ /(\d\d\d\d-\S+)\s/)){
		my ($y,$m,$d) = ($date =~ /^(19.[0-9]|2000)-([01]\d)-([0123]\d)/);
		next if (int $y < 1970 || int $m == 0 || int $d == 0);
		next if $item =~ / (show|set|remove) (\w+)=/i;
		if     ($item =~ / add (uid|maildrop|fullname|uic)=/i){
		    $line{$id} = "'$record'" unless %line;    
		    prtLine ($record, \%line);
		    $line{'edt'} = "'$date'";
		    next;
		}
		next if $item =~ / (del|rep|add) (user|gid|uid|group|setup|aliases|altname|altacct|fullname)=/i;
#		print "$record\t$item\n" if $item =~ / show (\w+)=/i;
	    }
	    push @item, $item;
	}
	$value = join '~', @item;
	$value =~ s/\s?;\s?$//;
	next unless $value;
	
#	last if ++$count > 13;

	if ($db =~ /adb/) {
	    next unless $key =~ /^last/;
	    $value =~ s/<-/ /g;
	    $record = uty::filter ($record);
	    foreach my $entry (split /\~/,$value){
		my ($date, $user, $from) = split /\s+/, $entry;
		$from = uty::filter ($from);
		$user = uty::filter ($user);
		%line = ();
		($line{hostdb} , $line{maildb}) = ("'$record'" , "'$user'") if $iddef eq 'host';
		($line{maildb} , $line{hostdb}) = ("'$record'" , "'$user'") if $iddef eq 'user';
		$line{'logon_at'}   = "'$date'";
		$line{'logon_from'} = "'$from'"  if $from;
		prtLine ($entry, \%line);
		%line = ();
	    }
	    next;
	}
	
	$line{$id} = "'$record'" unless %line;    
	prtLine ($record, \%line);
	
	if ($key eq 'fullname'){
	    ($line{name}, 
	     $line{von},
	     $line{namefam}) = fn2nf ($value);
	}else{
	    $line{$lookFor{$key}} = "'$value'";
	}
    }
    
    prtLine('exit', \%line);
    
    
  createTableLine: {
      foreach my $key ($id,keys %lng) { 
	  my $extra = max (5 , ($lng{$key} * 10) / 100); 
	  $format{$key} = "varchar(" . ($lng{$key}+$extra) . ")" unless $format{$key};
      }
      $format{timestamp} = 'TIMESTAMP',
      my (@members, %dejavu);      
      foreach my $name ($id, values %lookFor, 'timestamp'){
	  next if $dejavu{$name}++;
	  next if $name eq $lookFor{$id};
	  next if $name eq 'fullname';
	  if (!$format{$name}) { print "**** Never seen '$name'\n"; 
				 $format{$name} = 'varchar(24)'; }
	  my $str = join 
	      ' ',
	      $name, 
	      $format{$name},
	      " NOT NULL";
	  push @members, $str; 
      }    
      print DEF 
	  "create table if not exists $table (",
	  join (' , ', @members),
	  " , PRIMARY KEY ($id)",
	  ")",
	  ";\n";
  }

    sub prtLine {
	my ($rec,$hash) = @_;  $recordO = $recordO || $rec;
	return if  $recordO eq $rec or !(%$hash);

	my (@colName, @expr); 
	foreach my $k (keys %$hash){
	    push @colName, $k;
	    push @expr,    $$hash{$k};
	    $lng{$k} = length($$hash{$k})   if length($$hash{$k}) > $lng{$k};
	}
	my $str = sprintf 
	    "%s%s%s%s%s",
	    "insert ignore into $table (",
	    join (',',@colName),
	    ") values(",
	    join (',',@expr),
	    ");\n";
	print O $str;
	
	%$hash = ();
	$$hash{$id} = "'$rec'" unless $lookFor{$id};
	$recordO    =   $rec;
    }

}
sub max { return (@_[0]>@_[1] ? int @_[0] : int @_[1]); }

sub fn2nf {
    my $fullname = $_[0];
    my ($prenom,$nom,$von);
    
    $fullname =~ s/(\w)\. (.*)$/$1 $2/;
    $fullname =~ s/[\,\(\.].*$//; # supress everytnig after ","
    my @a = split /\s+/, $fullname;
    if (@a>2) {
	unless (($prenom,$von,$nom) = ($fullname =~ /(.*)\s+($vons)\s+(.*)$/i)){
	    ($prenom,$nom) = (join (' ',@a[0..$#a-1]) ,
			      @a[$#a]);
	}
    }else{
        ($prenom,$nom) = @a;
        ($nom,$prenom) = @a if @a==1;
    }
    swedish2english ($nom);
    swedish2english ($prenom);
#   print "@_ -> $von | $nom | $prenom\n";# if @a>2;
    return (
	    "'$prenom'",
	    "'$von'",
	    "'$nom'",
	    );

    sub swedish2english {
	my $name = @_[0];
	my @a;
	foreach my $n (split /[\s_]+/, $name) { 
	    my @m;
