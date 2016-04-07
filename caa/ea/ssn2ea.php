<?php
if (function_exists("date_default_timezone_set") and
    function_exists("date_default_timezone_get"))  @date_default_timezone_set(@date_default_timezone_get());

define('cnf_dev',True);
define('EA_administrator_here',True);

$_GET['org'] = 'nordita';

ob_start();
require_once  '../myPear/config.inc';
require_once  '../myPear/includes/bForm.inc';
require_once  '../myPear/includes/bForm/Avatar.inc';

myPear_db();

$output = ob_get_contents();
ob_end_clean();
$msg = '';

$cmd = 'maildb -list m_myorg inst pers_number namefam -if cmsep -ORDER namefam| grep -v ,$  | grep -v ,,|grep -vE "uninitialized|000000"|grep ,|grep -v Namefam';
foreach(explode("\n",`$cmd`) as $line){
  if (strpos($line,',') === False) continue;
  list($id,$m_myorg,$inst,$pers_number,$namefam) = explode(',',$line);
  $q = myPear_db()->query("SELECT * FROM zzz_avatars WHERE av_id = $m_myorg");
  $ssn = '';
  if (myPear_db()->num_rows($q) != 1){
    print "\n???Can't match m_myorg=$m_myorg\n   $line\n";
  }else{
    while($r = myPear_db()->next_record($q)) $ssn = $r['av_ssn'];
  }
  if (SSN::valid($ssn) && ($ssn != $pers_number)){
    $msg .= sprintf("<tt>%-20s %6d %-12s %-12s</tt><br/>\n",$namefam,$m_myorg,$ssn,$pers_number);
    myPear_db()->qquery("UPDATE zzz_avatars SET av_ssn='$pers_number' WHERE av_id=$m_myorg",True);
  }
}

print $msg;
print "\n".__FILE__."---------------done<br>\n";
