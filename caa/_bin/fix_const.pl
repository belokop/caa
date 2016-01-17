#! /usr/bin/perl
use strict;

my $d;

while (<STDIN>) {
  if ($_ =~ /define\(\'/ ){
    s/^\s*//;
    s/,/ ,/;
    s/ /\'/;
    s/;/;              \/\/ const/;
  }
  print;
}

exit;

    if ($_ =~ /define\s?\([\"\']/){
	s/define\s?\([\"\']/const /;
	s/\)//;
	s/[\"\']\s?,(\s*)?/ = /;
    }

    if ($_ =~ /\bconst\b/){
	s/const /define\(\'/;
	s/\s?=/\',/;
	s/;/\);/;
    }
