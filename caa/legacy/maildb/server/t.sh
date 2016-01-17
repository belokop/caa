#! /bin/bash

p="-HLO iam=root host=`hostname` cmd=maildb"

echo  -list von  $p | perl maildb-server 
echo  belokop $p | perl maildb-server 
exit
echo  nordita -unset Changeslog                      $p | perl maildb-server 
echo -search jeb  $p | perl maildb-server 
echo -search hertz  $p | perl maildb-server 
exit
echo  mailer -set mailname=yb@albanova.se -unset Changeslog $p | perl maildb-server 
echo  mailer -set mailname="mail.mailerovich"e -unset Changeslog $p | perl maildb-server 
echo  mailer -set maildrop=yb@albanova.se -unset Changeslog $p | perl maildb-server 
echo  mailer -unset maildrop Changeslog inst=FKM#NOR $p | perl maildb-server 

