This directory contains:

- collector.php 
  Script which reads the output from flexlm &
  Mathematica monitoring tools (by calling seeLicenseUsage) and writes it to the database.
  Should be installed as a cron job and run every 5-10 minutes,
  depending on the resolution you want

- seeLicenseUsage 
  The bash script which reads teh lmutil & monitorlm output

- lmutil flexlm monitoring tool

- monitorlm Mathematica monitoring tool

- Reference files
  Mathematica monitors:
   /afs/fysik.su.se/common/uadmin/lm/mathematica/pro/Linux/monitorlm
  Flexlm monitors:
   /afs/fysik.su.se/common/uadmin/lm/idl/bin-8.0/idl/idl/bin/lmutil
