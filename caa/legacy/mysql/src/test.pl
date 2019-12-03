#! /usr/local/bin/perl

use strict;
use postfix;

my $k = shift;
my $v = shift;

print "get from perl    $k:", postfix::get ($k), "\n"; 
print "get from postfix $k:", `postalias -q $k /etc/physto.se/maildb`,"\n";

postfix::put ($k, $v);

print "get from perl    $k:", postfix::get ($k), "\n"; 
print "get from postfix $k:", `postalias -q $k /etc/physto.se/maildb`,"\n";

