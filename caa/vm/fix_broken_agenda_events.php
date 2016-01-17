<?php

$_GET['org'] = 'nordita';
//$_GET['smtpdebug'] = 1;
error_reporting(E_ALL); 

require_once '../myPear/config.priv.inc';
require_once '../myPear/config.inc';
require_once '../myPear/includes/agenda.inc';
require_once '../myPear/includes/bMailer.inc';
require_once '../myPear/includes/bForm/Avatar.inc';

error_reporting(E_ALL); 

$fmt = "%s %10s %6d %s\n";

$url = 'http://agenda.albanova.se/';
print "\n===================================================================================================== $url\n\n";
$agenda = new agenda($url);

$q = CLI_query("SELECT * FROM abs_events WHERE e_code > 0 ORDER BY e_start",False);
while($r = myPear_db()->next_record($q)){
  // if ($r['e_code'] == 4682) 	b_debug::print_r($r,$r['e_code']);
  $conference = $agenda->getConference($r['e_code']);
  if (empty($conference)){
    printf($fmt,"\n".str_repeat('?',19),($e_start=date('Y-m-d',$r['e_start'])),(int)$r['e_code'],$r['e_name']);
    
    // Show organizers & Registrants
    show_avatars($r['e_id'],'Registrants');
    show_avatars($r['e_id'],'Organizers');

    // Send a reporting email and remove the orfelan records
    if (stripos($url,'agenda.albanova.se') !== False){
      $qq = CLI_query("SELECT * FROM abs_events  LEFT JOIN zzz_list_members ON e_code = lm_value WHERE e_code = ".$r['e_code'],False);
      while($rr = myPear_db()->next_record($qq)){
	if (stripos($url,'agenda.albanova.se') !== False){
	  myPear_mailer()->send('yb@nordita.org',
				"Removing non-accesible agenda event $r[e_id]",
				"
e_name:     $r[e_name]
e_id:       $r[e_id]
e_code:     $r[e_code]
e_start:    $e_start
e_end:      ".date('Y-m-d',$r['e_end'])."
e_reg_start:".date('Y-m-d',$r['e_reg_start'])."
e_reg_end:  ".date('Y-m-d',$r['e_reg_end'])."
",
				$syslog_thisMail=True,
				$no_preview=True);
	}
      }
      $u_id = show_avatars($r['e_id'],'u_id');
      CLI_query("DELETE FROM abs_events       WHERE e_code   = ".$r['e_code'],True);
      CLI_query("DELETE FROM zzz_list_members WHERE lm_value = ".$r['e_code'],True); // bList_vm_agendaEvents
      CLI_query("DELETE FROM zzz_units        WHERE u_parent = ".$r['e_id'],True);   // organizers unit
      CLI_query("DELETE FROM zzz_unit_members WHERE um_uid = $u_id",True);           // organizers unit members
      print "\n";
    }else{
      print str_repeat(' ',19)."SKIP cleanup for text agenda '$url'\n";
    }
  }else{
    foreach ($conference as $id=>$entry){
      if (trim(strToLower($entry['title'])) == trim(strToLower($r['e_name']))){
	$entry['title'] = '';
	//	if ($r['e_code'] != 4682) continue;
	continue;
      }
      printf($fmt,str_repeat('-',19),date('Y-m-d',$r['e_start']),(int)$r['e_code'],$r['e_name']);
      printf($fmt,str_repeat(' ',19),str_repeat(' ',8),          (int)$entry['attenders'],$entry['title']);
      // Show organizers & Registrants
      //      show_avatars($r['e_id'],'Registrants');
      //      show_avatars($r['e_id'],'Organizers');
    }
  }
}

function CLI_query($query,$verbose=True){
  $q = myPear_db()->query($query);
  if ($verbose)  print "$query - ".myPear_db()->num_rows($q)." rows affected\n";
  return $q;
}


function show_avatars($e_id,$who='Organizers'){
  if ($who == 'u_id'){
    $qq = CLI_query("SELECT u_id FROM zzz_units WHERE u_parent = $e_id",False);
    while($rr = myPear_db()->next_record($qq)) return $rr['u_id'];
  }else{
    $n = 0;
    $qq = CLI_query(($who == 'Organizers'
		     ? "SELECT * FROM zzz_units ".
		     " LEFT JOIN zzz_unit_members ON u_id = um_uid ".
		     " LEFT JOIN zzz_avatars ON av_id = um_avid ".
		     " WHERE u_parent = $e_id"
		     : "SELECT * FROM abs_visits ".
		     " LEFT JOIN zzz_avatars ON av_id = v_avid ".
		     " WHERE v_eid = $e_id"),
		    False);
    while($rr = myPear_db()->next_record($qq)){
      if (!$n++) print "   $who:\n";
      printf("       $n - %s\n",b_fmt::unEscape(bForm_Avatar::_fmtName('Lf',$rr)));
    }
  }
}
