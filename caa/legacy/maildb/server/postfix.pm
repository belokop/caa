package postfix;

use strict;

$postfix::debug = 0;
my $silence = $postfix::debug ? '' : '>/dev/null';
$postfix::af = $main::postfixDB || "/etc/postfix.maps/maildb";
my $exeP  = "/usr/sbin/postfix";
my $exePA = "/usr/sbin/postalias";

sub del       { my ($key) = (@_==1) ? @_ : join ':',@_ ;
		$key =~ s/:.*//;
		print "postfix::del($key)\n" if $postfix::debug;
		system "grep -v '^$key:' $postfix::af > /tmp/$$ && mv /tmp/$$ $postfix::af";
		system "$exePA $postfix::af"; 
		system "$exeP  reload $silence 2>&1"; }


sub getAll    { my %db;
		open F, $postfix::af || die $!;
		while (<F>) {
		    chomp;
		    my ($k, $v) = split /:\s+/, $_, 2;
		    $db{"$k:maildrop"} = $v; }
		return %db; }


sub get       { (my $key = $_[0]) =~ s/:.*//;
		my ($k,$v);
		chomp (my $line = `grep "^$key: " $postfix::af`);
		($k,$v) = split /:\s+/, $line, 2;
		print "postfix::get($key) = $v\n" if $postfix::debug;
		return $v; }

sub getByValue{ my $cmd = "grep \": $_[0]\$\" $postfix::af|sed \"s/:.*//g\"";
		chomp (my $reply = `$cmd`); 
		my @r = split(/[\r\n\s]+/,$reply);
		print "postfix::getByValue($_[0]) = @r\n" if $postfix::debug;
		return @r; }


sub put       { my ($key, $value) = (@_==2) ? @_ : ($_[0].":".$_[1] , $_[2]);
		return unless $value;
		$key =~ s/:.*//;
		return if lc $value eq lc &get($key);
		&del ($key);
		print "postfix::put($key: $value)\n" if $postfix::debug;
		system "echo '$key: $value' >> $postfix::af";
		system "$exePA $postfix::af"; 
		system "$exeP  reload $silence 2>&1"; 
		return $value; }

1;
