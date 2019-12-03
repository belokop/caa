#!/usr/local/bin/perl -w

use strict;
#use sql;

my %h = (key => 'value');

a({key1=>'value1'});
a(\%h);

b(key1=>'value1',
  key2=>['value2'],
  );



sub a {
    my $hash = $_[0];
#    while (@$hash) {	print "$_\t\n";    }

    foreach (keys %$hash) {
	print "$_\t->\t",$$hash{$_},"\n";
    }
}
sub b {
    my %hash = @_;
    foreach (keys %hash) {
	print "$_\t->\t",$hash{$_},"\n";
    }
}
exit;

sub min { return (int $_[0] < int $_[1] ? int $_[0] : int $_[1]); }



