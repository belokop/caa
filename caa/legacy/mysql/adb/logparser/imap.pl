#! /usr/local/bin/perl

use strict;

my %dejavu;
my (
    $userP,
    %tUser, %tAct, %tActSummary,
    );

while (<STDIN>) {
    
    next if /service init from/i;
    next if /user not known to|client not found in|user unknown/i;
    next if /ssl .*(unknown protocol|wrong version)/i;
    next if /trying to get mailbox|mailbox is open|readonly mailbox/i;
    
    chomp; $_ = lc $_;
    my ($m, $d, $t, $host, $proc, $line ) = split /\s+/, $_, 6;
    $proc =~ s/\[.*//; 
    $line =~ s/\s+/ /g;
  
    my $act;
    my $port;
    my $from = $1    if $line =~ /host=(\S+) /;
    my $user = $1    if $line =~ /user=(\S+) /;
    my $auth = $1    if $line =~ /auth=(\S+) /;
    my $mbx  = $1    if $line =~ /mbx=(\S+):/;
    my $nmsgs= $1.$2 if $line =~ /nmsgs=([1-9])(\S+) /;
    my $dura = $1    if $line =~ /duration=(\d+)\(sec/;
    
    if ($proc eq 'imp') {
	($act, $from, $port, $user) = ($line =~ /(\S+) (\S+) .*:(\d+) as (\S+)/);
	$proc .= ':'.$port;
    }elsif ($proc =~ 'horde'){
	# [imp] Login success for rb@physto.se [217.208.228.152] to {imap.physto.se:993} 
	($proc, $act, $user, $from, $port) = ($line =~ /\[(\w+)\] (.*) for (\S+) (\S+) .*:(\d+)/); 
	# [imp] 217.208.228.152 Message sent to alex from belokop
	($proc, $from, $act, $user) = ($line =~ /\[(\w+)\] (\d\S+) (.*) from (\S+)/)
	    unless $port; 
	# [turba] SQL search by rb@physto.se: table = turba_objects; query = ...
	my $table;
	($proc, $act, $user, $table) = ($line =~ /\[(\w+)\] (.*) by (\S+) table = (\S+)/)
	    unless $from;
	$proc .= ':'.$port;
	$act  .= " table=$table" if $table;
    }elsif ($proc =~ 'xinetd')   { # EXIT: imap pid=30341 duration=0(sec)
	$act   = sprintf "imapd exit: duration %6d secs",$dura
	    if $dura > 600;
	$user  = $userP;
    }elsif ($line =~ /^(\S.* (fail\S+|error|ssl conn\S+))/) {
	$act   = $1;
	$user  = $user || '???';
    }elsif ($line =~ /^(.*)[;, ]while /) {
	$act   = $1;
	$user  = $user || '???';
    }elsif ($line =~ /pam_krb.* for .(\w+). /) {
	$act   = "not in kerberos";
	$user  = $1;
    }elsif ($line =~ /(moved .* new mail).* from (\S+) /) {
	$act   = $1;
	$user  = $2;
	$user  =~ s/.*\/(\w+)$/$1/;
    }elsif ($proc eq 'sendmail') {
	next;
    }else{
	($act) = ($line =~ /^(\S+) /);
  }

#   print "$_\n"    unless $user && $act;
    next unless $act && $user;
  
    $act  .= " nmsgs=$nmsgs" if $nmsgs;
    $act  .= " mbx=$mbx"     if $mbx;
    $act   =~ s/[,;]//g;
    $host  =~ s/-\d$//;
    $proc  =~ s/:$//;
  
    undef $user   if $user eq '???' && $act eq 'logout';
    $user = '???' if $user =~ /\?/;
    $user =~ s/(\@.*|user=|\s)//g;
    $userP = $user;
    
    $from =~ s/[\[\]]//g;
    my $name = lc ((gethostbyaddr (pack('C4',split (/\./, $from, 4)),2))[0])
	if $from =~ /^\d/;
    $from = $name if $name;
    $from =~ s/(-\d)?.physto.se//g;
    $from =~ s/.*\.([-\w]+)\.([a-z]+)$/$1.$2/; # leave on company name
    
  $tUser{$user}{"$d $m $t"} = sprintf "%-14s %7s%-14s %s", $proc, $host, ($from ? "<-$from" : ""), $act
    if toRecordUser ($user, $proc, $host.$from, $act);

  $act = 'authentication failure'
    if  $act =~ /authenticat.*fail|login fail/
    || ($act =~ /fail/ && $proc =~ /imp/);

    if ($act =~ /(fatal mailbox error)/) {
	$act = $1;
	$user= sprintf " %-8s %s", $user, $mbx;
    }
    
    if (toRecordAct ($act)) {
    $tAct{$act}{"$d $m $t"} = sprintf "%-15s %7s <- %-14s %-8s", $proc, $host, $from, $user;
    $tActSummary{sprintf("%-8s %9s @ %-9s",$host, $user,$from)}++;
}

}

if (1) {
  foreach my $user (sort keys %tUser) {
    my $userp = $user;
    foreach my $date (sort keys %{ $tUser{$user} }) {
      printf
	"%-8s %s %s\n",
	$userp,
	$date,
	$tUser{$user}{$date};
      $userp = "";
    }
  }
}


foreach my $act (sort keys %tAct) {
  print "\n------------------ $act\n\n";
  foreach my $date (sort keys %{ $tAct{$act} }) {
    printf
      "   *** %-14s %s\n",
      $date,
      $tAct{$act}{$date};
  }
}

print "\n";

foreach my $key (sort {$tActSummary{$a}<=>$tActSummary{$b}} keys %tActSummary) {
    printf "   *** %4d failures on %s\n", $tActSummary{$key}, $key
	if $tActSummary{$key} > 0;
}

sub toRecordAct {
  my ($act) = @_;
  return 1 if $act =~ /fail|unable to|error/;
  return 1 if $act =~ /command stream end/;
  return 1 if $act =~ /connection reset/;
  return 0;
}


sub toRecordUser {
    my ($user, $proc, $host, $act) = @_;
    return 0 unless $user && $act;

    return 0 if $act =~ /^(logout|autologout)/;

    my $stamp = $user.$host.$proc;
    unless  ($dejavu{$stamp} =~ /fail/i) {
	unless (toRecordAct ($act) || $act =~ /fail/ ) {
	    for my $p (qw(login logout duration)){
		return 0
		    if ($dejavu{$stamp} eq $p && $act eq $p);
#		    if ($dejavu{$stamp} =~ /^$p/i && $act =~ /^$p/i);
	    }
	    
	    my @R =  qw(
			auth
			authenticated
			login
			logout
			autologout
			);
	    for my $p (@R){
		for my $n (@R){
		    return 0
			if ($dejavu{$stamp} eq $p && $act eq $n);
		}
	    }
	}
    }
    $dejavu{$stamp} = $act;
    return 1;
}


