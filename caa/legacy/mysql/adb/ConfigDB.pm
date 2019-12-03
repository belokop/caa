package config;

(my $src = `dirname $0`) =~ s/(\n|\r)//g;

$main::DB = 'adb';

$main::logfile    = '/var/log/adb';
$sql::server      = 'syssql.physto.se';
$adb::localDomain = 'physto.se';
$adb::poolmanlog  = "$src/acp_logfile";
$adb::vmslog      = "$src/wtmp.VMS"; #   wtmp.VMS -> /vms/bub/chw/www/lastlogin.txt  

$adb::usersAllEntries = join '|',    qw (
					 root
					 ftp
					 );
#					 toor
$adb::servicesAllEntries = join '|', qw (
					 shutdown
					 startup
					 boot
					 );
$adb::hostsAllEntries = join '|',    qw (
					 galactica
					 );

sub main::administrator { # administrator accounts
    return ($_[0] =~ /^(yb|belokop|root|toor|alex)$/);
}


sub ip2name {
  my $ip = lc $_[0];
  $ip =~ s/(\r|\n|(\.\d+\.gz))//g;
  my $name = ($ip =~ /^\d+\.\d+\./)
    ?  gethostbyaddr (pack("C4",split (/\./, $ip, 4)), 2)
      : (gethostbyname $ip)[0];
  if ($name !~ /\w/) {
    $name = "grublx" . sprintf "%02d", $1+10
      if  $ip =~ /^lab(\d\d)/;
    $name = $ip  unless $name;
  }
  
  # physto.se specific
  $name = 'ices01' if $name eq 'projektlinjen.nu';
  $name =~ s/.$adb::localDomain//g;  
  $name =~ s/\-(\d|tu\d)$//;                       # sysman-0 -> sysman
  $name = 'sysman' if $name eq 'mail';
  
  unless ($name =~ /(physto|scfab).se$/){
    my $gov = 'net|com|gov|edu';
    foreach my $c (qw(
		      it.su.se
		      [\-\w]+.se
		      su ru ch
		      dk fi no
		      ft ch be
		      it 
		      nl de pt uk at 
		      sz pl 
		      )) { $gov .= "|$c"; } 
    $name =~ s/^[\-\w]+\.($gov)$/$1/ if $name =~ /\..*\./;
    $name =~ s/^[\-\w]+\.([\-\w]+)\.($gov)$/$1.$2/;
    $name =~ s/^[\-\w]+\.([\-\w]+)\.([\-\w]+)\.($gov)$/$1.$2.$3/;
    $name =~ s/^[\-\w]+\.([\-\w]+)\.([\-\w]+)\.([\-\w]+)\.($gov)$/$1.$2.$3.$4/;
  }
  
#   $name =~ s/(dhcp)\d+/$1/;
    $name =~ s/(ppp).*(it.su|spbnit)\.([\-\w]+)$/$1.$2.$3/;
    $name =~ s/[\-\w\.]+(poolman)$/$1/;
    $name =~ s/[\-\w\.]+(ppp).*(algonet|tninet|worldonline)\.([\-\w]+)$/$1.$2/;
    $name =~ s/[\-\w\.]+\.(kth|swipnet|utfors|studen[ts]+\.\w+u|chello|bredbandsbolaget|bonet).([\-\w]+)$/$1.$2/;
    $name =~ s/[\-\w\.]+(hotmail|mail.yahoo|telia|da.uu|adsl.cybercity|dialup.wplus)\.([\-\w]+)$/$1.$2/;
    $name =~ s/[\-\w\.]+(online.nsk|online.kharkiv|sovintel)\.([\-\w]+)$/$1.$2/;
    $name =~ s/[\-\w\.]+(cern|fnal)\.([\-\w]+)$/$1.$2/;

    $name = 'pop'    if $name =~ /ipop/;

    return $name;
}

sub joinIt {
  my ($cache, $date, $time, $host, $login, $logFrom, $service) = @_;
  undef $logFrom if $logFrom =~ /($host|localhost)/;
  return unless $date =~ /^\d+$/;
  return unless $time =~ /^\d+$/;

  $host = $1 if $service =~ /(pop|imap)/;  #  ?? YB Is that correct??

  my $stamp = join ':', $host, $login, $logFrom, $service;
  my $entry = join ':', $date, $time, $stamp;  return if $cache =~ /$entry/;
  
  # Wipe out the previous entries
  my $entye = join ':', $date, $time, $host, $login, '', $service;
  $cache    =~ s/$entye//g;  # better to record with "from" only
  $cache    =~ s/\d+:\d+:$stamp//
    unless  $login =~ /^($adb::usersAllEntries)$/i
      || $service  =~ /^($adb::servicesAllEntries)$/i
      || $logFrom  =~ /^($adb::hostsAllEntries)$/i;
  
  $cache =~ s/^\s+|\s+$//g;
  $_[0]  = join ' ', sort (split (/\s+/,$cache), $entry);
}

sub printIt {
    my ($title,$hash) = @_;
    print "\n=========================================== $title \n\n";
    foreach my $login (sort keys %$hash) {
	print "$login\n";
	foreach (split / /,$$hash{$login}) {
	    my ($date, $time, $host, $login, $logFrom, @a) = split /:/;
	    $logFrom = " <- $logFrom" if $logFrom;
	    printf "   %s %s %12s %17s %-18s %s", $date, $time, $login, $host, $logFrom, "[@a]";
	    print "\n";
	}
    }
}


sub mysqlIt {
  my ($adb,$hash) = @_;

  my %db = ();
  foreach my $item (reverse sort keys %$hash) {
    foreach (split / /,$$hash{$item}) {
      my ($date, $time, $host, $login, $logFrom, @a) = split /:/;
      $db{"$login:$host"} = join '|',$date .' '. $time, $logFrom, '', @a;
    }
  }

  print "----------------------------------- Write $adb\n"; 

  sql::opendb ($adb);
  adb::init();
  adb::storeLogRec (\%db);
}

1;









