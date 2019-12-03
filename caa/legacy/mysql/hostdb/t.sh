#! /bin/bash

p="-HLO iam=root host=`hostname` cmd=hostdb"

echo -getmac 130.237.205.2  $p | perl hostdb-server 
echo -getmac 83.251.64.119  $p | perl hostdb-server 
echo -getmac 130.237.208.42 $p | perl hostdb-server 
exit
echo syslx01  $p | perl hostdb-server 
echo  hertz paolo $p | perl maildb-server 
echo  mailer -set mailname=yb@albanova.se -unset Changeslog $p | perl maildb-server 
echo  mailer -set mailname="mail.mailerovich"e -unset Changeslog $p | perl maildb-server 
echo  belokop $p | perl maildb-server 
echo  mailer -set maildrop=yb@albanova.se -unset Changeslog $p | perl maildb-server 
echo  mailer -unset maildrop Changeslog inst=FKM#NOR $p | perl maildb-server 
echo  nordita -unset Changeslog                      $p | perl maildb-server 

