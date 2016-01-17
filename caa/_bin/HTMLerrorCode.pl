#! /usr/bin/perl

use HTTP::Status;
$code = shift;
printf "%s\n", ($code eq '404'
		? "The page you are looking for is not found on this server"
		: status_message($code));


