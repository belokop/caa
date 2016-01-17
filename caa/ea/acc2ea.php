<?php
if (function_exists("date_default_timezone_set") and
    function_exists("date_default_timezone_get"))  @date_default_timezone_set(@date_default_timezone_get());

define('cnf_dev',True);
define('EA_admin_here',True);

$_GET['org'] = 'nordita';
$_GET['showdb'] = 1;

ob_start();
require_once  '../myPear/config.inc';
require_once  '../myPear/includes/bForm.inc';
require_once  '../myPear/includes/bForm/Avatar/ea.inc';

myPear_db();

$output = ob_get_contents();
ob_end_clean();

// Remove "1-july" from dates
$q = myPear_db()->qquery("SELECT av_id, av_birthdate, av_ms_year, av_phd_year FROM zzz_avatars WHERE NOT ( av_ms_year IS NULL ) OR NOT ( av_phd_year IS NULL ) OR NOT ( av_birthdate IS NULL )",1);
while($r=myPear_db()->next_record($q)){
  $updates = array();
  foreach(array('av_birthdate', 'av_ms_year', 'av_phd_year',) as $i){
    if (!empty($r[$i])){
      list($Y,$x) = explode(' ',date('Y mdHis',$r[$i]));
      if ($x == '0701120000') $updates[$i] = $Y;
    }
  }
  if (!empty($updates)){
    $update = "UPDATE zzz_avatars SET ".b_fmt::joinX(',',$updates)." WHERE av_id = $r[av_id]";
    myPear_db()->qquery($update,1);
  }
}


// Sync maildb
$cmd = 'maildb -list EmployeeTitle Employeetype Office Groupid status namefam m_myorg acc_kth acc_su NorPeriodOfStay inst -if lfsep | grep -vE "Namefam|uninitialized"|grep LF|grep NOR|grep -i active';
foreach(explode("\n",`$cmd`) as $line){
  $line = trim($line);
  if (empty($line)) continue;
  list($id,$EmployeeTitle,$Employeetype,$Office,$Groupid,$status,$namefam,$m_myorg,$acc_kth,$acc_su,$NorPeriodOfStay,$inst) = explode('-LF-',$line);
  if (empty($m_myorg))              continue;
  $q = myPear_db()->query("SELECT * FROM zzz_avatars WHERE av_id = $m_myorg");
  if ($n=myPear_db()->num_rows($q) != 1){
    print "\n???Can't match m_myorg=$m_myorg, nrows=$n\n   $line\n";
    continue;
  }
  
  //  if ($m_myorg != 93146) continue;

  // YB 2014-10-20
  // Skip the SU account, it is not usable now...
  $av_identity = array();
  if (!empty($acc_kth)) $av_identity[] = $acc_kth;
  if (!empty($acc_su))  $av_identity[] = $acc_su;

  $updates = array("av_identity='".join(',',array_unique($av_identity))."'");
  if (!empty($EmployeeTitle))  $updates[] = "av_position='$EmployeeTitle'";
  if (!empty($updates)) myPear_db()->qquery("UPDATE zzz_avatars SET ".join(',',$updates)." WHERE av_id=$m_myorg",True);

  $av = new bForm_Avatar_ea($m_myorg);
  if ($av->isE() || $av->isV()){
    $er = $av->hook_employment();
    $records = $er->get_currentEmploementRecords(array('e_org'=>myOrg_ID));
    if ((count($records) == 1) && !empty($records[0]['lm_id'])){

      list($day1,$day2) = (preg_match('/(\d*-\d*-\d*)-(\d*-\d*-\d*)$/',$NorPeriodOfStay,$m)
			   ? $m
			   : array(0,0));
      $updates = array();
      $er_keys = array_merge(array('lm_value','lm_status','lm_key'),$er->get_packed_items());
      foreach($er_keys as $k){
	$nv = $v = @$records[0][$k];
	switch($k){
	case 'lm_key':   if (!empty($day1))          $nv = b_time::txt2Unit($day1); break; // Start of position
	case 'lm_status':if (!empty($day2))          $nv = b_time::txt2Unit($day2); break; // End of position
	case 'lm_value': if (!empty($EmployeeTitle)) $nv = $EmployeeTitle; break;
	case 'e_gid':    if (!empty($Groupid))       $nv = $Groupid; break;
	case 'e_off':    if (!empty($Office))        $nv = $Office; break;
	case 'e_rate':   if (!empty($Employeetype))  $nv = $Employeetype; break;
	case 'e_org':                                $nv = myOrg_ID; break;
	case 'e_cc':  
	case 'e_inst':
	case 'e_type':
	  break;
	default:
	  b_debug::internalError("Forgotten option '$k'");
	}
	if (!myPear::is_empty($v) || !myPear::is_empty($nv)){
	  if (strToLower($nv) != strToLower($v)) print "-------------------------------- $k: $v => $nv\n";
	  $updates[$k] = $nv;
	}
      }
      //      if (@$gkdgdg++ < 3)	print_r($updates);
      $er->updateMember($updates,$records[0]['lm_id']);
    }elseif (@$gdgdg++ < 3){
      print_r($records);
    }
  }
}
print "\n".__FILE__."---------------done<br>\n";
