#! /usr/bin/perl
use strict;

my $d;

while (<STDIN>) {
    if (/ereg.*\([\'\"](.*)[\'\"],/){
	my $str = $1;
	if ( $str =~ /\// ) { $d = ';'; }
	else                { $d = '/'; }
	if ( $str =~ /$d/ ) { $d = ','; }
	if ( $str =~ /$d/ ) { 
	    print "cant't find a delimeter '$d':\n $str\n";
	    exit;
	}
	s/eregi\(([\'\"])([^,]*)([\'\"])/preg_match($1\/$2\/i$3/g;
	s/ereg\(([\'\"])([^,]*)([\'\"])/preg_match($1$d$2$d$3/g;
	s/eregi_replace\(([\'\"])([^,]*)([\'\"])/preg_replace($1\/$2\/i$3/g;
	s/ereg_replace\(([\'\"])([^,]*)([\'\"])/preg_replace($1$d$2$d$3/g;
    }
    if (/\<\?=/){
	s/\<\?=/<?php print /g;
    }
    print;
}
