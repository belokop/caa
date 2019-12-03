#! /usr/local/bin/perl 
use strict;

my $Host = `hostname`;      chomp $Host;
my $host = `hostname -s`;   chomp $host;
(my $localDomain = $Host) =~ s/$host.//;

my @net = ($localDomain eq "physto.se") 
    ? (
       "192.168.205", 
       "130.242.128", 
       "130.237.208", 
       "130.237.205", 
#       "192.168.208",
       )
    : ("130.242.129");

my @list;
my %e;
foreach my $net (@net) {
    foreach my $n (1..254){
	my ($name, $alias, $addrtype, $lng, @addr) = gethostbyaddr pack("C4",split (/\./, "$net.$n", 4)), ,2;
	next if $name =~ /^ex-|^dummy|^gw|obsolete/i;
	next if $name =~ /-[0123]/i;
	next unless $name =~ /$localDomain/;
#	next unless $name =~ /atom/;

	$name =~ s/.$localDomain//;
#	system ("hostdb $name -if noup");
	push @list, $name;

	$name =~ s/[\.-].*//;
	$name =~ s/[-\d\.]//g;
	$e{$name}++ if $name =~ /\w/;
    }
}

exe ("hostdb @list  -order seen -list status opsys seen");

@list = keys %e;
#exe ("hostdb \"".join('|',sort @list)."\" -e -order seen -list status opsys seen");

sub exe { print "\n\n",$_[0],"\n";
	  system $_[0];
      }











