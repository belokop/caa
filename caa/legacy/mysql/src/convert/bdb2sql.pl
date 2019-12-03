#! /usr/local/bin/perl

use maildb;
use strict;

my $count;
$DB::debug = 1;
my %old;

if (0) {
    &DB::open('/root/maildb/user.db-991013');
    
    foreach my $key (sort keys %DB::a) {
	next unless $key =~ /:maildrop/;
	$old{$key} = &DB::get ($key);
    }
    &DB::close();
}else{
    open F,"/root/maildb/user.db.dump" or die $!;
    while (<F>){
	chomp;
	my ($key,$val) = split /\s+/,$_,2;
	next unless $key =~ /:maildrop/;
	$old{$key} = $val;
    }
}


&DB::open('mail.db');
foreach my $key (sort keys %old) {
    &DB::put ($key, $old{$key});
#    print "$key <= $old{$key}\n";
}

&DB::close();
