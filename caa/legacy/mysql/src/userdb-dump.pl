#! /usr/local/bin/perl
# Dump sendmail db to ascii

BEGIN { chomp(my $src=`dirname $0`);
	unshift @INC , $src; }

use sendmail;
use strict;
my $DB = shift;

$main::sendmailAgent = "postfix";
$main::sendmailDB = "/etc/postfix/maildb.db";

$DB = $main::sendmailDB;

$sendmail::debug = 10;

&sendmail::open($DB);

foreach my $key (sort keys %sendmail::a) {
    print "$key ",&sendmail::get ($key), "\n";
}
&sendmail::close();
