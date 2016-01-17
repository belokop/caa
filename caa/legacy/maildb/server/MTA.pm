package MTA;

use strict;
use ConfigDB;
use sendmail;
use postfix;

sub open      {  &sendmail::open ($main::sendmailDB) if $main::MTA eq "sendmail"; }

sub delete    {  &sendmail::delete(@_) if $main::MTA eq "sendmail";
		 &postfix::del(@_)     if $main::MTA eq "postfix"; }


sub getAll    {  &sendmail::getAll(@_) if $main::MTA eq "sendmail";
		 &postfix::getAll(@_)  if $main::MTA eq "postfix"; }

sub get       {  &sendmail::get(@_) if $main::MTA eq "sendmail";
		 &postfix::get(@_)  if $main::MTA eq "postfix"; }

sub put       {  &sendmail::put(@_) if $main::MTA eq "sendmail";
		 &postfix::put(@_)  if $main::MTA eq "postfix"; }

sub close     {  &sendmail::close   if $main::MTA eq "sendmail"; }
1;
