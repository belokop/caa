#! /usr/local/bin/perl

use strict;
use Socket;

chomp (my $YEAR = `date +%Y`);
my ($m,$d,$h,$proc,$login,$fr,
    $x,@rest,
    %cache,);

my $LOG = '/var/log/messages /var/log/maillog';
open L,"cat $LOG| grep -v pam_krb5afs:|grep -E 'per..tion|imap|pop|horde' |" or die $!;

# Apr 3 15:41:42 syslx11 perdition[40]: Auth: 130.242.128.82->130.237.208.17 user="britta" server="imap1.physto.se" port="143" status="ok"
my $perd = '(\S+)\s+(\d+)\s([\d:]+)\s.*tion.* Auth: ([\w\.]+)-.* user="(\w+)" server="([\w\.]+)" port="(\d+)" status="(\w+)"';


# Apr 2 09:17:11 syslx01 horde[07]: [imp] Login success for rdt@physto.se [213.14.18.6] to {fc.physto.se:993}
my $imp = '(\S+)\s+(\d+)\s([\d:]+)\s.*horde.*imp.*(Logout|Login).* for (\S+) \[([\w\.]+)\]';


# Apr 3 16:15:33 syslx13 ipop3d[22]: Logout user=fogle host=atomrs09.physto.se [130.242.128.210] nmsgs=0 ndele=0
my $wu = '(\S+)\s+(\d+)\s([\d:]+)\s.*(ipop3d|imapd).* (Logout|Login) user=(\w+) host=([\w\.]+) ';

# Apr 5 09:06:05 syslx04 pop3d: LOGIN, user=iwe, ip=[::ffff:130.237.208.130]
# Apr 7 07:16:51 syslx14 imapd: LOGOUT, user=kr, ip=[::ffff:130.242.128.50], headers=0, body=0
my $cour = '(\S+) (\d+) ([\d:]+) \S+ (imapd-ssl|imapd|pop3d): (LOGOUT|LOGIN), user=(\w+), ip=\[::\w+:([\w\.]+)\]';


while (<L>) {
  chomp;
  
  if (($m,$d,$h,$x,$login,$fr) = (/$imp/i)) {
    $login =~ s/\@.*//; 
    $proc  = 'imp';
  }elsif (($m,$d,$h,$fr,$login,$proc,$x) = (/$perd/i)) {
    $proc  =~ s/1?.physto.se//;
    $proc  = "pop"  if $x eq "110" || $x eq "995";
    $proc .= ":SSL" if $x eq "993" || $x eq "99p";
  }elsif (($m,$d,$h,$proc,$x,$login,$fr) = (/$wu/i)) {
    $proc  = 'pop'  if $proc =~ /pop/;
    $proc  = 'imap' if $proc =~ /imap/;
  }elsif (($m,$d,$h,$proc,$x,$login,$fr) = (/$cour/i)) {
    my $ssl= ":SSL" if $proc =~ /ssl/i;
    $proc  = 'pop2' if $proc =~ /pop/;
    $proc  = 'imap2'if $proc =~ /imap/;
    $proc .= $ssl;
  }else{
#    print "'$_'\n" if /pop/;
    next;
  }
  updateLL (lc $login, lc $fr, lc $proc, $YEAR, $m, $d, $h);
}
exit;


sub updateLL {
  my ($login, $fr, $proc, $y, $m, $d, $h) = @_;
  return unless $login && $y && $m && $d;

  my $now = getDate($y,$m,$d) or return;
  return if $cache{$login} && $cache{$login} ge $now;

  my $maildb = '/usr/local/bin/maildb';
  chomp (my $currentValue = `$maildb $login -list last|grep -v Name|grep -E "^$login | $login "`);
  $currentValue =~ s/.* (\d\d\d\d-)/$1/;
  $cache{$login} = $currentValue if $currentValue;
  return if $currentValue && $currentValue ge $now;

  system ("$maildb $login -set last='$now"
	  .sprintf(" %-9s <- %s", $proc, getTidyHost($fr)).
	  "~$currentValue'");
  $cache{$login} = $now;
}

sub getTidyHost {
  my $ip = $_[0];
  my ($name,$aliases,$addr,$length,@addrs);
  if ($ip =~ /\d\.\d/) {
    ($name,$aliases,$addr,$length,@addrs) = gethostbyaddr(inet_aton($ip), AF_INET);
  }else{ 
    $name = $ip;
  }
  $name =~ s/.physto.se//;
  $name = "$1.$2" if $name =~ /\.(\w+)\.(\w+)$/ and $name !~ /(kth|su|scfab).se$/;
  $name = $ip unless $name;
  return $name;
}

sub getDate {
  my ($y, $m, $d) = @_;
  my @M = qw (Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
  for my $n (0..11) {
    return sprintf("%4d-%02d-%02d", $y, ++$n, $d)
      if lc @M[$n] eq lc $m;
  }
  return;
}
