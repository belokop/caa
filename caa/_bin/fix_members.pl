#! /usr/bin/perl
use strict;

# membersList[$av_id][$k]
#my $f = 'membersList';
#my $t = 'get_memberInfo';

while (<STDIN>) {
    if (/membersList/i){
	s/membersList\[(.*)\]\[(.*)\]/get_memberInfo($1,$2)/g;
	s/membersList\[(.*)\]/get_memberInfo($1)/g;
	s/@\$this->get_memberInfo/\$this->get_memberInfo/g;
	s/resetMembersList\(\)/get_memberInfo('reset')/g;
    }
    if (/\<\?=/){
	s/\<\?=/<?php print /g;
    }
    print;
}
