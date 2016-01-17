<?php
function jam_tablesForRenumber(){
  static $reply = array();
  if (empty($reply)){

    $reply = array_diff(array_values(preg_grep('/jam_old/',myPear_db()->getTables())) , array('jam_old_flavors'));
    b_debug::print_r($reply,__FUNCTION__);

    //
    // Populate myPear::$sql_identities_extra with the old tables to get the right master_ID
    myPear::$sql_identities_extra['jam_old_avatars'] = array('t'=>'jam_old_avatars',
							     'i'=>'av_id');
    $q=myPear_db()->qquery("SELECT COUNT(*),av_id FROM jam_old_avatars GROUP BY av_id HAVING COUNT(*)>1 ORDER BY COUNT(*) DESC",True);
    while ($r = myPear_db()->next_record($q))       b_debug::print_r($r,'jam_old_avatars');

    if (!in_array('PRIMARY',myPear_db()->showIndex('jam_old_avatars'))) myPear_db()->qquery("ALTER TABLE `jam_old_avatars` ADD PRIMARY KEY ( `av_id` )",True);
    myPear_db()->qquery("OPTIMIZE TABLE `jam_old_avatars`",True);
    
    foreach($reply as $old_table){
      $table = str_replace('_old','',$old_table);
      if (!myPear_db()->tableExists($table)) continue;
      foreach(myPear::$sql_identities as $class=>$id){ 
	if ($id['t'] != $table) continue;

	$q=myPear_db()->qquery("SELECT COUNT(*),$id[i]  FROM $old_table GROUP BY $id[i]  HAVING COUNT(*)>1 ORDER BY COUNT(*) DESC",True);
	while ($r = myPear_db()->next_record($q))       b_debug::print_r($r,$old_table);

	myPear::$sql_identities_extra[$old_table] = array('t'=>$old_table,
							  'i'=>$id['i']);
	if (!in_array('PRIMARY',myPear_db()->showIndex($old_table))) myPear_db()->qquery("ALTER TABLE `$old_table` ADD PRIMARY KEY ( `$id[i]` )",True);
	myPear_db()->qquery("OPTIMIZE TABLE `$old_table`",True);
      }
    }
    b_debug::print_r(myPear::$sql_identities_extra,'identities_extra');
  }
  return $reply;
}

function fix_jam_email(&$r){
  $r['av_email'] = str_replace('@@','@', $r['av_email']);
  if (stripos($r['av_email'],'bradley') !== False){
    $r['av_email']     = 'michael.bradley@physics.umu.se';
    $r['av_firstname'] = 'Michael';
    $r['av_lastname']  = 'Bradley';
  }

  if (stripos($r['av_email'],'e.winstanley') !== False){
    $r['av_email']     = 'e.winstanley@sheffield.ac.uk';
  }
  
  if (stripos($r['av_email'],'jmsmith@oddjob.uchicago.edu') !== False){
    $r['av_lastname']     = 'Smith';
  }

  if (stripos($r['av_email'],'sturgut@metu') !== False){
    $r['av_email']     = 'sturgut@metu.edu.tr';
  }
  
  if (trim($r['av_email']) == 'j.m'){
    $r['av_email']     = 'jose.figueroa@ipmu.jp';
  }
  
  if (trim(strToLower($r['av_lastname']))=='lipatnikov'){
    $r['av_email']     = 'lipatn@tfd.chalmers.se';
  }

  if (trim(strToLower($r['av_lastname']))=='cimarelli'){
    $r['av_email']     = 'andrea.cimarelli2@unibo.it';
  }

  if (b_fmt::RFC_2822($r['av_email'])){
    if (empty($r['av_lastname'])) $r['av_lastname'] = $r['av_firstname'];
  }else{
    print "???? drop non-valid av_email=\"$r[av_email]\"  <br/>\n";
    unset($r['av_email']);
  }
}

function open_dbg_file($name=Null,$file=Null){
  print "<h1>$name</h1>\n";
  return;
  global $dryRun;  if ($dryRun) return;
  static $cache_file = Null; // keep the name of the current cache file
  
  if (!empty($name)){
    print "<split>$name</split>\n";
    print "<h1>$name</h1>";
  }
}

function create_unitsSC(){
  global $dryRun;  
  /*
   * replace the bForm_SC by the standard 'bUnit' class
   */
  $dryRun = 0;
  $db = myPear_db();
  if ($db->tableExists('jam_old_sc_members')) {
    open_dbg_file(__METHOD__,__FILE__);
    $qq = $db->qquery('SELECT sc_posid FROM jam_old_sc GROUP BY sc_posid ORDER BY sc_posid',1);
    while ($rr = $db->next_record($qq)) {
      if (!($pos = myPear::getInstance('bForm_jam_Position',$rr['sc_posid']))){
	print_r($rr);
	//b_debug::internalError("pos == 0");
	continue;
      }
      print "<h2>$pos->ID --------------------------------------".$pos->name()."</h2>\n";
      $q = $db->qquery("SELECT * FROM jam_old_sc_members WHERE scmember_posid='$pos->ID' ORDER BY scmember_role_RO",1);
      while ($r = $db->next_record($q)) {
	$rank=$r['scmember_role_RO'];
	if ($rank != RANK_jam_observer &&
	    $rank != RANK_jam_SC2 &&
	    $rank != RANK_jam_SC1) b_debug::internalError("unexpected role $rank");
	
	if ($av= myPear::getInstance('bForm_Avatar_jam',$r['scmember_avid'],array('nocache','strict'))){
	  JAM_unit($rank,$pos->ID)->add_unitMember($av->ID);
	}else{
	  myPear::WARNING("Can't hook '$r[scmember_avid]'");
	}
      }
    }
    if (!$dryRun) $db->qquery('DROP TABLE jam_old_sc',1);
    if (!$dryRun) $db->qquery('DROP TABLE jam_old_sc_members',1);
    myPear_db()->reset_cache();
  }
}

function convert_comma_Lists_unit(){
  if (file_exists('/tmp/jam_old_positions')){
    open_dbg_file(__METHOD__,__FILE__);
    
    $convert = array('pos_cm_admins'    => RANK_jam_posadmin,
		     'pos_cm_secretary' => RANK_jam_secr,
		     'pos_cm_observers' => RANK_jam_observer);
    
    $pos_id_o = 0;
    foreach(explode("\n",`cat /tmp/jam_old_positions|grep ','`) as $item){
      if (trim($item)){
	list($pos_id,$f,$cm) = explode(',',trim($item),3);
	$pos = myPear::getInstance('bForm_jam_Position',$pos_id);
	JAM_posID($pos->ID);
	if ($pos_id != $pos_id_o)	print "<h2>$pos->ID --------------------------------------".$pos->name()."</h2>\n";
	$pos_id_o = $pos_id;
	$rank = $convert[$f];
	foreach (explode(',',$cm) as $login){
	  $login = str_replace('belokop','yb',   $login);
	  $login = str_replace('stefan', 'shofm',$login);
	  $av = bForm_Avatar::hook(array('av_identity'=>$login),True);
	  if (@$av->ID) JAM_unit($rank,$pos->ID)->add_unitMember($av);
	  else myPear::WARNING("Can't hook '$login'");
	}
      }
    }
  }
}

function cleanLogs(){

  $db = myPear_db();
  if ($db->tableExists('jam_old_logs')) {
    print '<h1>DOING '.__FUNCTION__.'</h1>';
    
    // clean non-interesting events
    $db->debug = 1;
    foreach(array('mail'    =>'',
		  'fix'     =>'',
		  'bug'     =>'',
		  'create'  =>'',
		  'log_form'=> '9999999',
		  'log_text'=> 'Ask to refill the form',
		  'log_user'=> 'yb',
		  ) as $t=>$v){
      if (empty($v)) $db->qquery("DELETE FROM jam_old_logs WHERE log_type = '$t'",1);
      else           $db->qquery("DELETE FROM jam_old_logs WHERE $t = '$v'",1);
    }

    foreach(array('log_form','log_time') as $t){
      $db->qquery("DELETE FROM jam_old_logs WHERE $t='' OR $t=0 OR $t IS NULL OR $t=999999",1);
    }
    $db->debug = 0;
  }
}

function fixLogs($dryRun=False){
  $dryRun = (bool)$dryRun;
  static $mmm = array("a.hebecker.thphys.uni.heidelberg", "alejandro.saenz.physik.hu.berlin", "alessandro.lanzafame.oact.inaf.i",
		      "alessandro.melchiorri.roma1.infn", "alessandro.tomasiello.mib.infn.i", "alexander.chroneos.imperial.ac.u",
		      "andrei.starinets.physics.ox.ac.u", "antoine.vanproeyen.fys.kuleuven",  "dmitri.semikoz.apc.univ.paris7.f",
		      "antti.pekka.jauho.nanotech.dtu.d", "apostolos.pilaftsis.hep.manchest", "apostolos.pilaftsis.manchester.a",
		      "c.wetterich.thphys.uni.heidelber", "cezar.condeescu.cpht.polytechniq", "christine.gottschalkson.aei.mpg",
		      "claude.billionnet.cpht.polytechn", "claudine.lacroix.grenoble.cnrs.f", "danielrayneaukirkhope.hotmail.co",
		      "david.andriot.physik.uni.muenche", "dieter.luest.physik.uni.muenchen", "dimitar.mladenov.phys.uni.sofia",
		      "emilian.dudas.cpht.polytechnique", "evelyne.gil.lasmea.univ.bpclermo", "fendley.rockpile.phys.virginia.e",
		      "fiete.ph.utexas.edu",              "gabriele.ghisellini.brera.inaf.i", "georgios.kavoulakis.matfys.lth.s",
		      "gerard.bonhomme.lpmi.uhp.nancy.f", "giancarlo.ghirlanda.brera.inaf.i", "giesel.theorie3.physik.uni.erlan",
		      "giovanni.amelino.camelia.roma1.i", "giovanni.montani.frascati.enea.i", "giuseppe.dibernardo.physics.gu.s",
		      "guillaume.malpuech.lasmea.univ.b", "haitham_zaraket.hotmail.com",      "herta.wiesbeck.physik.uni.muench",
		      "herve.partouche.cpht.polytechniq", "honecker.theorie.physik.uni.goet", "ilka.brunner.physik.uni.muenchen",
		      "j.march.russell1.physics.ox.ac.u", "j.pawlowski.thphys.uni.heidelber", "jeffrey.forshaw.hep.manchester.a",
		      "kawano.hep.th.phys.s.u-tokyo.ac",  "klaus.richter.physik.uni.regensb", "kobayash.gauge.scphys.kyoto.u.ac",
		      "kurt.wiesenfeld.physics.gatech.e", "l.amendola.thphys.uni.heidelberg", "lasky.tat.physik.uni.tuebingen.d",
		      "levan.chotorlishvili.physik.uni",  "magdalena.larfors.physik.uni.mue", "marios.petropoulos.cpht.polytech",
		      "michael.flohr.itp.uni.hannover.d", "michael.haack.physik.uni.muenche", "monika.kuerzinger.physik.uni.mue",
		      "mtsige.physics.siu.edu",           "natalia.borodatchenkova.physik.u","niemeyer.astro.physik.uni.goetti",
		      "nierste.particle.uni.karlsruhe.d", "pantelis.tziveloglou.cpht.polyte", "parthasarathi.majumdar.saha.ac.i",
		      "predrag.cvitanovic.physics.gatec", "quader.scorpio.kent.edu",          "r.h.dijkgraaf.uva.nl",
		      "reinhard.noack.physik.uni.marbur", "sakagami.grav.mbox.media.kyoto.u", "saueressig.thep.physik.uni.mainz",
		      "stefan.hofmann.physik.uni.muench", "stefan.k.leupold.theo.physik.uni", "suganuma.ruby.scphys.kyoto.u.ac",
		      "tapio.ala.nissila.tkk.fi",         "thiemann.theorie3.physik.uni.erl", "timo.weigand.tphys.uni.heidelber",
		      "tobias.kramer.physik.uni.regensb", "tomas.mail.cern.ch",               "tony.lun.sci.monash.edu.au",
		      "trauzettel.physik.uni.wuerzburg", "viatcheslav.mukhanov.physik.lmu",   "viatcheslav.mukhanov.physik.uni",
		      "wolfgang.weber.ipcms.u.strasbg.f", "yamaguch.het.phys.sci.osaka.u.ac", "yvonne.wong.physik.rwth.aachen.d",
		      );
  $db = myPear_db();
  search_for_ID(0);
  
  if ($db->tableExists('jam_old_logs')) {
    open_dbg_file(__FUNCTION__);
    print '<h1>DOING fixLogs</h1>';
    b_debug::xxx(' ');

    print '<h2>log_user (column size was too small)</h2>';
    $db->qquery("ALTER TABLE `jam_old_logs` CHANGE `log_user` `log_user` VARCHAR( 132 ) NOT NULL",1); 
    foreach ($mmm as $k){
      foreach(array('jam_old_avatars','zzz_avatars') as $table){
	if (!$db->tableExists($table))  continue;
	foreach(array('av_email2','av_email') as $email){
	  if (!$db->columnExists($email,$table))  continue;
	  $q = $db->query("SELECT $email FROM $table WHERE $email REGEXP '$k'");
	  while ($r = $db->next_record($q)) {
	    foreach(preg_split('/[ ,]/',$r[$email]) as $e){
	      if (preg_match("/$k/",$e)) $db->qquery("UPDATE jam_old_logs SET log_user='$e' WHERE log_user REGEXP '$k'",1);
	    }
	  }
	}
      }
    }
    
    print '<h2>counting</h2>';

    // See what do we have
    $q = $db->query("SELECT * FROM jam_old_logs GROUP BY log_type  ORDER BY log_type ASC");
    $total = 0;
    while ($r = $db->next_record($q)) {
      $qq = $db->qquery("SELECT log_id FROM jam_old_logs WHERE log_type='$r[log_type]'",1);
      $n = $db->num_rows($qq);
      $ccc[$r['log_type']] = $n;
      $total += $n;
    }
    b_debug::print_r($ccc,"Total $total");

    // Try to reformat
    print '<h2>processing</h2>';
    $wasBefore = '?';

    $where = 'reminder';
    $where = 'application';
    $where = 'recommendation';
    $where = 'review';
    $where = 'login';

    $where = ($dryRun ? "log_type='$where'" : "1");
    $where = 1;   

    $q = $db->qquery("SELECT * FROM jam_old_logs WHERE $where ORDER BY log_time DESC",1);
    while ($r = $db->next_record($q)) {
      if ($dryRun){
      	// if (strpos($r['log_text'],' (') === False) continue;
	if (@$not_too_long++ > 33333) break;
	print "---------------------------------------------------------------------------------------------------------------------------------------<br>\n";
      }
      @$itog['x_total']['_read']++;
      @$itog[$r['log_type']]['_read']++;
      $log_form_class = $av = Null;
      $comment = '';
      if ($r['timestamp'] == '0000-00-00 00:00:00') $r['timestamp'] = date('Y-m-d H:i:s',$r['log_time']);      
      foreach(array('ap_posid','ap_avid','r_apid','r_avid','rec_apid','rec_avid') as $id){
	unset($_GET[$id]); 
	unset($_REQUEST[$id]);
      }
      
      $log_form = $r['log_form'];
      $log_type = $r['log_type'];
      $log_comment = '';
      
      $record_id = '';
      $drop_this_record = False;
      if ($log_form > 0){
	switch($log_form_name=myPear::getInstanceFromID($log_form,$noFormPrefix=True)){
	case 'jam_Applicant':
	  $qz = $db->qquery("SELECT * FROM jam_applicants WHERE ap_id = $log_form",$dryRun);
	  if ($db->num_rows($qz)){
	    while($rz = $db->next_record($qz)){
	      foreach(array('ap_posid','ap_avid') as $id) $_REQUEST[$id] = $_GET[$id] = $rz[$id];
	    }
	    $log_form_class = myPear::getInstanceFromID($log_form,'class');
	    if (($av = $log_form_class->av) instanceof bForm_Avatar){
	      $log_type = 'application';
	      $record_id = "av=$av->ID log_type=$log_type log_form=$log_form";
	    }
	  }
	  
	  // However, let's keep the "login" type...
	  switch($t=$r['log_type']){
	  case 'login':
	    $log_type = $t;
	    $log_form = 0;
	    $log_form_class = $av;
	    $record_id = "av=$av->ID log_type=$log_type";
	    break;
	  case 'reminder':
	    @$itog['x_total'][$msg="rejected strange $t-->$log_type"]++;
	    @$itog[$t][$msg]++;
	    $drop_this_record = True;
	  }
	  break;
	  
	case 'jam_Review':        // =================================================================
	  $qz = $db->qquery("SELECT * FROM jam_reviews WHERE r_id = $log_form",$dryRun);
	  while($rz = $db->next_record($qz)){
	    foreach(array('r_apid','r_avid') as $id) $_REQUEST[$id] = $_GET[$id] = $rz[$id];
	  }
	  $log_form_class = myPear::getInstanceFromID($log_form,'class');
	  $av = $log_form_class->av;
	  $log_type = 'review';
	  // However...
	  switch($t=$r['log_type']){
	  case 'reminder':
	    @$itog['x_total'][$msg="rejected strange $t-->$log_type"]++;
	    @$itog[$t][$msg]++;
	    $drop_this_record = True;
	      break;
	  case 'login':
	    if (empty($r['log_user'])){}	   
	  }
	  break;
	  
	case 'jam_Recommendation': // =================================================================
	  switch($log_type){
	  case 'reminder': // normal case, the log_form is the recommendation reminder
	    break;
	    
	  case 'login':
	    $log_form = 0;
	    $av = @$log_form_class->av;
	    break;

	  case 'review':
	    $log_form = 0;
	    $av = $log_form_class = bForm_Avatar::hook(array('av_identity'=>$r['log_user']));
	    break;
	    
	  default:                
	    $qz = $db->qquery("SELECT * FROM jam_recommendations WHERE rec_id = $log_form",$dryRun);
	    while($rz = $db->next_record($qz)){
	      foreach(array('rec_apid','rec_avid') as $id) $_REQUEST[$id] = $_GET[$id] = $rz[$id];
	    }
	    $log_form_class = myPear::getInstanceFromID($log_form,'class');
	    //$log_form_class->ap = myPear::getInstance('bForm_jam_Applicant',$_GET['rec_apid'],array('nocache','strict'));
	    $log_form_class->av = myPear::getInstance('bForm_Avatar',$_GET['rec_avid'],array('nocache','strict'));
	    $av = $log_form_class->av;
	    $log_type = 'recommendation';
	  }
	  break;
	  
	case 'Avatar':            // =================================================================
	  $av = $log_form_class = myPear::getInstanceFromID($log_form,'class');
	  $record_id = "av=$av->ID log_type=$log_type";
	  if (!myPear::is_empty($log_comment = $r['log_text'])) 	  $record_id .= " log_comment=$log_comment";
	  switch($log_type){
	  case 'reminder':
	    $log_form = $av;
	    if (stripos($log_comment,'Recommendation') !== False) $log_form = 0;
	    $av = bForm_Avatar::hook($r['log_user']);
 	    break;
	    
	  case 'login':
	  case 'login':
	    $log_comment = '';
	    $log_form = 0;
	    $log_form_class = $av;
	    $record_id = "av=$av->ID log_type=$log_type";
	    break;
	  case 'review':
	  default:
	  }
	  break;
	  
	default: 
	  if (!empty($r['log_text'])){	
	    $user = preg_replace('/ \(.*/','',$r['log_text']);
	    $log_form_class = $av = bForm_Avatar::hook($user);
	  }
	  if (empty($log_form_class)) b_debug::print_r($r,"unexpected myPear::getInstanceFromID($log_form) = $log_form_name");
	}
	
	if (empty($log_form_class)) $log_form_class = myPear::getInstanceFromID($log_form,'class');
      }

      if (empty($log_form_class)){
	print "myPear::getInstanceFromID($log_form,'class') = $log_form_name<br>\n";
	$match = search_for_ID($r['log_form']);
	@$itog['x_total']['rejected-empty log_form']++;
	@$itog[$log_type]['rejected-empty log_form']++;
	$drop_this_record = True;
      }
      if ($drop_this_record)  continue;

      if ($dryRun) print "log_form_class = $log_form_class name=".$log_form_class->name()." <br>\n";
      
      if (empty($r['log_user']) && strpos('@',$r['log_text'] !== False)){
	$r['log_user'] = preg_replace('/ .*/','',$r['log_text']);
      }
      if (empty($av) && ($u = $r['log_user'])){
	if (!($av = bForm_Avatar::hook($u,1))) $av = bForm_Avatar::hook(array('av_email' => $u));
      }
      if (empty($av) && $log_type=='login'       && (int)$r['log_form']>0 ){
	$av = myPear::getInstance('bForm_Avatar',$r['log_form'],array('nocache','strict'));
	if (is_object($av))	$log_form = 0; // avoid duplicated entries
      }
      if (empty($av) && $log_type=='application' && (int)$r['log_form']>0 ){
	if ($ap = myPear::getInstance('bForm_jam_Applicant',$r['log_form'],array('nocache','strict'))){
	  $av = $ap->av;
	  $log_form = $ap->ID;
	}
      }

      if ($av instanceof bForm_Avatar){
	if ($log_type != $r['log_type']){
	  print "!!! log_type is changed: $r[log_type] ---> $log_type <br>\n";
	  @$itog[$r['log_type']]["_changed_to&nbsp;&nbsp; $log_type"]++;
	  @$itog[$log_type]["_changed_from $r[log_type]"]++;
	  @$itog['x_changed'][$r['log_type']][$log_type]++;
	  @$itog['x_total']['changed']++;
	}

	if (!empty($record_id)){
	  if ($record_id == $wasBefore){
	    @$itog['x_total'][($id="rejected-sequential $record_id")]++;
	    @$itog[$log_type][$id]++;
	    if ($dryRun)	if (@$gjzzg[$id]++ < 9) b_debug::print_r($r,$id);
	    continue;
	  }
	}
	$wasBefore = $record_id;

	@$itog['x_total']['_written']++;
	@$itog[$log_type]['_written']++;
	$query = array();

	if ($av->ID == $log_form) $log_form = 0;
	foreach (array(
		       'log_api'    => JAM_MODULE,
		       'log_avid'   => $av->ID,
		       'log_type'   => $log_type,
		       'log_form'   => $log_form,
		       'log_comment'=> $log_comment,
		       'log_ip'     => $r['log_ip'],
		       'log_time'   => $r['log_time'],
		       'timestamp'  => $r['timestamp'],
		       ) as $k=>$v){
	  if (!empty($v)) $query[] = $k.'='.myPear_db()->quote($v);
	}
	if ($dryRun){
	  print "$log_type accepted, av=".$av->fmtName('Lf,s')."<br>\n";
	  unset($r['log_id']);
	  if ($log_type != $r['log_type']) b_debug::print_r($r,"accepted, but $r[log_type] --> $log_type");
	  // 	  else b_debug::print_r($query,"accepted");
	}else{
	  myPear_db()->query("INSERT INTO zzz_logs SET ".join(',',$query));
	}
      }else{
	unset($r['log_id']);
	if ($gjzzg++ < 9) b_debug::print_r($r,'rejected - no avatar');
	@$itog['x_total']['rejected-no avatar']++;
	@$itog[$log_type]['rejected-no avatar']++;
	if (($n=count(search_for_ID($r['log_form']))) != 1) print "??????????? $n answers for log_form=$r[log_form]<br>\n"; 
      }
      if ($log_form_class instanceof bForm) $log_form_class->__clean_cache();
      if (is_object($av))                   $av->__clean_cache();
    }
    if (!$dryRun) $db->query('DROP TABLE jam_old_logs',1);
    myPear_db()->reset_cache();
    print "---------------------------------------------- finish with logs <br/>\n";
    ksort($itog);
    foreach(array_keys($itog) as $k) ksort($itog[$k]);
    b_debug::print_r($itog,'itog');
  }
}

function showApplicantsStatistics(){  
  myPear::WARNING('DOING showApplicantsStatistics');
  $db = myPear_db();
  $q  = $db->query('SELECT * FROM jam_applicants GROUP BY ap_avid ORDER BY ap_lastname');
  while ($r = $db->next_record($q)) {
    $av = myPear::getInstance('bForm_Avatar',$r['ap_avid'],array('nocache'));
    $data = array($av->name());
    $qq = $db->query("SELECT * FROM jam_applicants WHERE ap_avid = $r[ap_avid] ORDER BY ap_submitdate");
    while ($rr = $db->next_record($qq)) {
      $ap = myPear::getInstance('bForm_jam_Applicant',$rr['ap_id']);
      $i = $counter[$av->ID.$ap->pos->ID]++ ? "span class='redText'" : 'i';
      $data[] = "<$i>".$ap->pos->name().($i=='i'?'':' ?????????????????????????????????? duplicated application')."</$i>\n";
    }
    myPear::MESSAGE(join('<br/>',$data));
  }
}

function tidy_country($targetVal,$table,$col,$targetCol,$value=''){
  global $stat;

  $translation = array(
		       'durres|albany|alban\b'                                      => 'albania',
		       'algeria|algiers|constantine.*university|usthb'              => 'algeria',
		       'argent|Buenos a|La Plata'                                   => 'argentina',
		       'yerevan|Erevan'                                             => 'armenia',
		       'queensland|melbourne|sydney|new south wales|griffith'       => 'australia',
		       'adelaide|austral|macquarie|\brmit\b'                        => 'australia',
		       'research school of astronomy and astrophysics'              => 'australia',
		       'vienna|johannes kepler university|linz|uni.*graz'           => 'austria',
		       'innsbruck'                                                  => 'austria',
		       '\bbaku\b'                                                   => 'azerbaijan',
		       'dhaka|shahjalal|rajshahi'                                   => 'bangladesh',
		       '\bminsk\b|\bGomel|\bbelarus'                                => 'belarus',
		       'bruxelles|louvain|mons|liege|Gent|Leuven|namur|ghent'       => 'belgium',
		       'hasselt|uni.*\bde\b.*\bli.*ge\b|belgian|marche.en.famen'    => 'belgium',
		       '\bgenk\b'                                                   => 'belgium',
		       'Bosnia|Sarajevo'                                            => 'bosnia',
		       '\bbra.il|S.*o Paulo|avenida.*fernando.*ferrari|sao carlos'  => 'brazil',
		       'paraiba|rio de janeiro|universidade.*da para.*ba|campinas'  => 'brazil',
		       'sofia|bulgar'                                               => 'bulgaria',
		       'saskatchewan|calgary|alberta|perimeter|qu.*bec|toronto'     => 'canada',
		       'Regina|\bMcGill\b|ontario|(dal|saint mary.*) university'    => 'canada',
		       'mcmaster|Concordia|triumf|carleton|canadian|windsor'        => 'canada',
		       'montreal|waterloo|brunswick|\blaval|Victoria|lethbridge'    => 'canada',
		       'simon.*fraser|canad.*institute|sherbrooke'                  => 'canada',
		       'federico santa maria|Bio-Bio|valparaiso'                    => 'chile',
		       'institute of high energy of physics|hunan|shenyang|wuhan'   => 'china',
		       'high energy physics.*cas|shaanxi|henan normal|zhongshan'    => 'china',
		       'university of aeronautics and astronautics|tsing.?hua'      => 'china',
		       'chang.an|chin|Shandong|beijing|shanghai|peking|Sichuan'     => 'china',
		       'Zhejiang|nanjing|central south|fudan|jingzhou|dalian'       => 'china',
		       'huazhong|huazhong|lanzhou|nankai|beihang|jilin|xiamen'      => 'china',
		       'ji lin|tsinghua|northwest university.*xi.*an|yunnan'        => 'china',
		       'Bogot(.*D.*C|a)|sim.*n bol.*var'                            => 'colombia',
		       'universidad (de los andes|sergio arboleda)|olombia'         => 'colombia',  // But might be also Chile or Venezuela or Peru
		       'Croatia|Zagreb'                                             => 'croatia',
		       '\bcuba\b|university of oriente'                             => 'cuba',
		       'prague|praha|Czech|pavol jozef safarik|charles|Masaryk'     => 'czech republic',
		       'esk.*republika|\bbrno\b'                                    => 'czech republic',
		       'helsingoer|copenhagen|aarhus|niels boh|danish'              => 'denmark',
		       'giza|Azhar|alexandria|ain shams|pharos university'          => 'egypt',
		       'agricultur.*engineering.*research.*institute|cairo'         => 'egypt',
		       'tartu'                                                      => 'estonia',
		       'Addis Ababa|ethiopia|jimma|adama university'                => 'ethiopia',
		       'university of gond.r'                                       => 'ethiopia',
		       'finn|helsinki|turku|oulu|Aalto| tkk|kouvola|suomi'          => 'finland',
		       'bo akademi university|jyv.*skyl'                            => 'finland',
		       'Normal.* sup|bl.*se.pascal|marseille|\blyon\b|\becole '     => 'france',
		       'fourier|savoie|palaiseau|strasbourg|corsica|laum/ensim'     => 'france',
		       'Sophia.*Antipolis|\blapth\b|french|paris|toulouse|cnrs'     => 'france',
		       'curie|poincare|cpht|in2p3|ipht|\bcea\b|montpellier'         => 'france',
		       'lptens|des hautes etudes|grenoble|bourgogne|dijon|\bimt\b'  => 'france',
		       'universit.* d. (mans|provence)|bretagne occidentale'        => 'france',
		       'hautes .*tudes scientifiques|lpt ens|\biuem\b|\brouen\b'    => 'france',
		       '\bnice u'                                                   => 'france',
		       'Tbilisi|kazbegi'                                            => 'georgia',
		       'Freiburg|deutsches|heinrich.*heine|germany|dortmund'        => 'germany',
		       'potsdam|Tuebingen|Dresden|bonn|erlangen|wuppertal'          => 'germany',
		       'maximilians|\bjena\b|argelander.institut|mainz|frankfurt'   => 'germany',
		       'karlsruhe|\bkiel\b|humboldt|rostock|bochum|tubingen'        => 'germany',
		       'stuttgart|g.*ttingen|german|konstanz|nuremberg|oldenburg'   => 'germany',
		       'nuernberg|duesseldorf|m.*nchen.*tum|leipzig|paderborn'      => 'germany',
		       'albert.*einstein.*institut|helmholtz.*fias|kaiserslautern'  => 'germany',
		       'augsburg|aachen|darmstadt|munich|wuerzburg|muenchen'        => 'germany',
		       'wittenberg|max.*planck|w.*rzburg|\bdesy|siegen'             => 'germany',
		       'berlin|bremen|heidelberg|wusterhausen|fulda|dusseldorf'     => 'germany',
		       'bielefeld|cologne|bethe center|muenster|drezden'            => 'germany',
		       'hannover|Sch.*sch Hall|Hamburg|uni.* m.*nchen'              => 'germany',
		       'Ludwig.*Maxim|regensburg|bayreuth|\bkit\b'                  => 'germany',
		       'Institute for Computational Physics|johann wolfgang goethe' => 'germany',
		       'athens|greek|ioannina|Patras|crete|Thessaloniki|hellenic'   => 'greece',
		       'demokritos|ioannina'                                        => 'greece',
		       'budapest|\bhungar\b|debrecen|eotvos lorand|kfki'            => 'hungary',
		       'e.*tv.*s lor.*nd|roland.*e.*tv.*univ|hungarian'             => 'hungary',
		       'iceland|slands'                                             => 'iceland',
		       'Dehradun|Patna|Kalna|Madhyasasan|india|Burdwan|Bombay'      => 'india',
		       'centre for fundamental research and creative education'     => 'india',
		       'utkal|homi bh|\bpune\b|har.*chandr|delhi|Aligarh U|kalyani' => 'india',
		       'tata|mumbai|bangalore|ind instit|calcutta|calicut|Jadavpur' => 'india',
		       'pondicherry|jalandhar|moradabad|jawaharlal|^(K\.U\.)$'      => 'india',
		       'ind.*insitute of technology|sastra u|gujarat|kakatiya'      => 'india',
		       'institute for plasma research|saha institute|kurukshetra'   => 'india',
		       'the institute of mathematical sciences|anna university'     => 'india',
		       'new delhi|kolkata|bengal|tamil|punjab|srinagar|siliguri'    => 'india',
		       'vidyasagar|ujjain|hindu|meerut|bharathiar|rajiv gandhi'     => 'india',
		       'inter.*university.*centre.*astronomy.*astrophysics'         => 'india',
		       'hyderabad|Garhwal|tiruchirapalli|osmania|lucknow'           => 'india',
		       'jamia.*millia.*islamia|ngm college|alagappa|kharagpur'      => 'india',
		       'raman research institute|\bk\.u\.\b|madras|chennai'         => 'india',
		       'shivaji university|cultivation of science|bhavnagar'        => 'india',
		       'bhopal|rangasamy|\biucaa\b|maharaj|iisc|rohilkhand|shimla'  => 'india',
		       'S*V.*NATIONAL.*INSTITUTE|surat|panjab|chandigarh|kanpur'    => 'india',
		       'rmd engineering college|homi.bhabha|bhubaneshwar|tezpur'    => 'india',
		       'thiruvananthapuram|guru nanak|csir\b.*\bimmt|\bHBNI\b'	    => 'india',
		       'hebei normal university'                                    => 'indonesia',
		       'dublin|university college cork|irish'                       => 'ireland',
		       'technion|tel.aviv|haifa|I.rael|Jerusalem|Hebrew'            => 'israel',
		       'Ben.*Gurion|Weizmann|Isreali|bar ilan|\bil\.'               => 'israel',
		       '\bictp\b|florence|genov?a|Velletri|Faenza|Brindisi|Pavia'   => 'italy',
		       'Bari|Naples|Parma|napoli|camerino|padua|catania'            => 'italy',
		       '\brome\b|\broma\b|campobasso|milano|venezia|\bpisa\b|milan' => 'italy',
		       'italia|abdus salam|lecce|\binaf\b|salerno|aquila'           => 'italy',
		       'international centre for theoretical physics|palermo'       => 'italy',
		       'sissa|turin|padova|trieste|firenze|torino|infn|bologna'     => 'italy',
		       'ferrara|insubria|trento|Torino|s3.*nanoscienze|sapienza'    => 'italy',
		       'international school for advanced studies|Trieste'          => 'italy',
		       'tor vergat|verona|universit.*degli studi|salento|\bcmcc\b'  => 'italy',
		       'jap|tokyo|Osaka|kyoto|yukawa|nagoya|\bkek\b|kwansei'        => 'japan',
		       'Hokkaido|Kyushu|niigata|tsukuba|tohoku|\bkobe\b|\briken\b'  => 'japan',
		       '\bkinki\b|\bkindai\b|\bkeio\b|sokendai|rikkyo'              => 'japan',
		       'chiba\b|graduate university for advanced studies'           => 'japan',
		       'High.*Energy.*Accelerator.*Research'                        => 'japan', // KEK
		       'yarmouk'                                                    => 'jordan',
		       'riga|latvija'                                               => 'latvia',
		       'middle east|hariri canad|leban'                             => 'lebanon',
		       'vilnius|vinlius'                                            => 'lithuania',
		       'luxembourg'                                                 => 'luxemburg',
		       'antananarivo|malagasy'                                      => 'madagascar',
		       'petronas|malaysia|usm.*university'                          => 'malaysia',
		       'macedonia'                                                  => 'macedonia',
		       'morocc'                                                     => 'morocc',
		       'universidad nacional|mexic|ifm-umsnh|michoacana'            => 'mexic',
		       'metropolitan.*autonomous.*university'                       => 'mexic',
		       'tribhu.an|nepal?ese'                                        => 'nepal',
		       'amsterdam|nikhef|netherlands|esa.estec|unesco-ihe|utrech'   => 'netherlands',
		       'leiden|utecht|utrecht|lorentz institute|groningen'          => 'netherlands',
		       'dutch|alkmaar|vlissingen|delft|nijmegen|Netherlan|antwerp'  => 'netherlands',
		       'centrum wiskunde|eindhoven'                                 => 'netherlands',
		       'massey|auckland'                                            => 'new zealand',
		       'akintola|\bilorin\b|unwana'                                 => 'nigeria',
		       'norw|oslo|university of bergen|ntnu'                        => 'norway',
		       'palestinian'                                                => 'palestine',
		       'islamabad|national university of sciences and technology'   => 'pakistan',
		       'pakist?an|quaid-i-azam'                                     => 'pakistan',
		       'wroclaw|jagiellon|warsaw|Jagellonian|jagellon|silesia'      => 'poland',
		       'nicolaus copernicus|poznan|mickiewicz|polish'               => 'poland',
		       'portug|Lisbon|porto|coimbra|camgsd|beira.*interior'         => 'portugal',
		       'aveiro|Lisboa|CENTRA |instituto superior t.*cnico'          => 'portugal',
		       'asia pacific center|yonsei|seoul|sungkyunkwan|sogang'       => 'republic of korea',
		       '\bkores\b|korea|cheongju'                                   => 'republic of korea',
		       'kaist|center.*quantum spacetime|pohang|hanyang|ewha '       => 'republic of korea',
		       's.*kore|asia-pacific center for theoretical'                => 'republic of korea',
		       'romania|bucharest|hulubei|modena'                           => 'romania',
		       'nanyang'		                                    => 'singapore',
		       'a.i.alikhanov|Dubna|russ|Leningrad|s.*petersburg|rostov'    => 'russia',
		       'kapitza|moscow|Krasnoyarsk|Kirensky|Kaliningrad|bogoliubov' => 'russia',
		       'Institute for Theoretical and Experimental Physics|mipt'    => 'russia',
		       'landau|\bras\b|irkutsk|south.*federal|nrnu.*mephi'          => 'russia',
		       'USSR|moscow|Novosibirsk|\bitep\b|\bioffe\b'                 => 'russia',
		       'joint institute for nuclear research|jinr'                  => 'russia',
		       'Sau.*Arabia'                                                => 'saudi arabia',
		       'saudi|king abdullah'                                        => 'saudi arabia',
		       'belgrade'                                                   => 'serbia',
		       'Slovenia|\bCelje\b|ljubljana|jozef stefan|sloven\b'         => 'slovenia',
		       'african institute for mathematical sciences|wit?watersrand' => 'south africa',
		       'Stellenbosch|nithep|cape town|wits university'              => 'south africa',
		       'johannesburg'                                               => 'south africa',
		       'span|barcelona|madrid|zaragoza|santiago|granada|valencia'   => 'spain',
		       'calabria| de oviedo|\bific\b|ift uam.csic| uam|of Granda'   => 'spain',
		       'universidad de la laguna|modulo c-xi|autonoma|complutense'  => 'spain',
		       'salamanca|uned|catalonia|Instituto de F.*sica Te.*rica'     => 'spain',
		       'university of catalo|catal.n.a'                             => 'spain',
		       'karlstad university| i teknisk fysik|oskar klein'           => 'sweden',
		       '3903|swed|boras|hyltebruk|uppsal|chalmers|sverige'          => 'sweden',
		       'ume.* univer|\blund\b|stockholm|svensk|nordita|country'     => 'sweden',
		       'gothenburg|sp.*nga gymnasium.*grundskola|linnaeus'          => 'sweden',
		       'sweden|Goteborg|Link.*ping|rebro Uni'                       => 'sweden',
		       '\bltu\b'                                                    => 'sweden',
		       '\bkth\b|royal institute|Lule.*|\bG.*teborgs? U'             => 'sweden',
                       'kungliga.*tekniska'                                         => 'sweden',
		       'fribourg|\bcern\b|swiss|geneva|zurich|bern|geneve'          => 'switzerland',
		       'switzerland|Neuchatel|EPFL|eth\b.*z.*rich|basel'            => 'switzerland',
		       'uni.*gen.*ve|univ.*z.*rich|paul.*scherrer'                  => 'switzerland',
		       'nicosia|cypruit|cypriot'                                    => 'cyprus',
		       'nctu|national central university|tsing hua|chung.yuan'      => 'taiwan',
		       'tawan university|chiao.*tung'                               => 'taiwan',
		       'chulalongkorn'                                              => 'thailand',
		       'bilkent|eastern mediterranean|cukurova|yuzuncu|bogazici'    => 'turkey',
		       'izzet baysal university|istanbul|(koc|ege) uni|turkish'     => 'turkey',
		       'zmir institute of technology|turkie|diyarbakir'             => 'turkey',
		       'kiev|kyie?v|donetsk|khark.v|odessa|donbass|uzhgorod'        => 'ukraine',
		       'B.*Verkin| NASU|kharkiv|ukrainian'                          => 'ukraine',
		       'Cameroon|buea'                                              => 'un. rep. of cameroon',
		       'institute for particle physics phenomenology|cardiff'       => 'united kingdom',
		       'queen mary|nottingham|leicester|leeds|sussex|sheffield'     => 'united kingdom',
		       'liverpool|united kingdom|\bu\.k\.|scotland| of york'        => 'united kingdom',
		       'cambridge|oxford|brit|glasgow|belfast|swansea|england'      => 'united kingdom',
		       'heriot-watt|university of cambrige|aberdeen|Surrey'         => 'united kingdom',
		       'strathclyde|\buk\b|\bU.K.\b|lancaster|warwick|imperial'     => 'united kingdom',
		       'canterbury| st.? andrews| exeter|Edinburgh'                 => 'united kingdom',
		       'london|durham|edinburgh|manchester|southampton|portsmouth'  => 'united kingdom',
		       'united.*(kingdom|knigdom)|hertfordshire|Newcastle|Reading'  => 'united kingdom',
		       'saint marys|\bde montfort'                                  => 'united kingdom',
		       'Center for Cosmology and Particle Physics|\bwayne\b'        => 'usa',
		       'southwest university|\bias\b|american|nebraska|hillsdale'   => 'usa',
		       'albert einstein center| irvine|cincinnati|c.n. yang inst'   => 'usa',
		       'institute for advanced study|union college|kenyon college'  => 'usa',
		       ' lawrence high school|national center for atmospheric'      => 'usa',
		       'national institute of standards and technology|santa cruz'  => 'usa',
		       'boston college|duke univers|\bmit\b|stroudsburg|dartmouth'  => 'usa',
		       'case western|evansville|argonne national laboratory'        => 'usa',
		       'pennsylvania|chicago|unit.*stat|california|wisconsin'       => 'usa',
		       'u.s.a.|virginia|stony brook|illinois|texas|washington'      => 'usa',
		       'arizona|buffalo|ucla|uc sa|william and mary|northeastern'   => 'usa',
		       'mississippi|missouri|michigan|kansas|united.*ameri'         => 'usa',
		       'san francisco|new york|st john|massachusetts|florida|iowa'  => 'usa',
		       '\busa\b|waltham|idaho|us citizen|rochester|methodist'       => 'usa',
		       'Cornell|Harvard|stanford|princeton|maryland|delaware|ohio'  => 'usa',
		       'rutgers|tufts|usa|los alamos|kavli|livermore|rice u'        => 'usa',
		       'Arlington|syracuse|connecticut|jefferson|uc(sb|sd)'         => 'usa',
		       'Columbia|Brown U|yale u|Harvard|John.*Hopkins|Purdue|utah\b'=> 'usa',
		       'texas |caltech|colorado|\breno\b|george mason'              => 'usa',
		       'ut austin|pennslyvania|hawaii|pennstate'                    => 'usa',
		       'tennessee|pittsburgh|berkeley|harvey mudd|brandeis|miami'   => 'usa',
		       'vanderbilt|montana|haverford|\bkent state|uc davis'         => 'usa',
		       'northwestern|notre dame|wisocnsin|north carolina|minnesota' => 'usa',
		       'oregon|oklahoma|carnegie m|uiuc|penn state|kentucky'        => 'usa',
		       '^(u.s.a.|usa|us|united states)$|brookhaven'                 => 'usa',
		       'de la republica'                                            => 'uruguay',
		       'arifov institute of electronics|Tashkent'                   => 'uzbekistan',
		       'Vene.uela|caracas'                                          => 'venezuela',
		       'ho chi min|hanoi|viet nam'                                  => 'vietnam',
		       'iasbs\b|\bipm\b|institute.*research in fundamental sciences'=> 'iran',
		       'bu.ali.sina|Sahand|ferdowsi|hellas.forth|iran|sharif'       => 'iran',
		       'amirkabir|shahrood|\brazi\b|shahid|azad un|\bpers\b'        => 'iran',
		       'tehran|tabriz|yazd|zanjan|isfahan|mazandaran|kermanshah'    => 'iran',
		       'tarbyat|Shiraz|shahrekord|tarbiat modares|abbaspour'        => 'iran',
		       'tafresh university|shahid beheshti|payam no.r|alzahra'      => 'iran',
		       'institute.*research.*fundamental.*sciences|esfahan'         => 'iran',
		       '(azad|semnan).*uni|persian'                                 => 'iran',
		       );

  if (empty($targetVal))                 return;
  if (strstr($targetVal,'buquqwlfagns')) return;
  if (strstr($targetVal,'url=http://'))  return;

  $country = strip_tags(trim(mb_strToLower(str_replace('(','',str_replace(')','',str_replace("\n",' ',str_replace("\r",' ',$targetVal)))))));

  if (!b_cc::getCC($country)){
    foreach ($translation as $f=>$t){
      if (preg_match(';\b'.$t.'\b;', $country) || preg_match(";$f;i", $country)){
	$country = $t;
	break;
      }
    }
    
    foreach(b_cc::getList() as $cc=>$name){
      if (!b_cc::getCC($country)) if (stristr($country,$name) !== false) $country = $name;
      if (!b_cc::getCC($country)) if (stristr($name,$country) !== false) $country = $name;
    }
  }
  
  if (empty($value))  $value = $targetVal;
  $whwre = myPear_db()->quote($value,1,$targetCol);

  if ($code=b_cc::getCC($country)){
    myPear_db()->qquery("UPDATE $table SET $col='$code' WHERE $whwre");
    @$stat[' YES match']++;
    @$stat[$code]++;
  }else{
    b_fmt::printt("???? '$country'");
    myPear_db()->qquery("UPDATE $table SET $col='' WHERE $whwre");
    @$stat[' NO match']++;
  }
}	      

function createIfMissing($column,$table,$after,$size=4){
  if (!myPear_db()->columnExists($column,$table)){
    myPear_db()->qquery("ALTER TABLE `$table` ADD `$column` VARCHAR($size) NULL AFTER `$after`",True);
    myPear_db()->reset_cache();
  }
  myPear_update_addIndex($table,"$column($size)");
}

function guess_ap_country($from,$what){
  $db = myPear_db();
  $db->debug=1;
  $q = $db->qquery("SELECT * FROM `jam_old_applicants` WHERE ( $what IS NULL OR $what = '' ) AND ap_status != 23");
  $db->debug=0;
  while ($r = $db->next_record($q)){
    tidy_country($r[$from],'jam_old_applicants',$what,'ap_id',$r['ap_id']);
  }
}


function c2c(){
  global $stat;
  print '<h1>'.__FUNCTION__.'</h1>';
  if (myPear_db()->tableExists('jam_old_applicants')){
    open_dbg_file(__FUNCTION__,__FILE__);
     
    if (myPear_db()->columnExists('ap_country','jam_old_applicants')){
      myPear_db()->qquery("ALTER TABLE `jam_old_applicants` DROP `ap_country`",1);
      myPear_db()->reset_cache();
    }
    
    $db = myPear_db();
    
    createIfMissing('ap_residentship','jam_old_applicants','ap_citizenship');
    createIfMissing('ap_ms_country',  'jam_old_applicants','ap_ms_year');
    createIfMissing('ap_phd_country', 'jam_old_applicants','ap_phd_year');
    createIfMissing('av_ms_country',  'zzz_avatars','av_ms_year');
    createIfMissing('av_phd_country', 'zzz_avatars','av_phd_year');
    
    foreach (array('','old_') as $old){
      foreach (array('applicants','avatars') as $tab){
	$table = "jam_$old$tab";
	if (!$db->tableExists($table))       continue;
	foreach(array('av_','ap_') as $av){
	  foreach(array('residentship','citizenship') as $ship){
	    $col = $av.$ship;
	    if (!$db->columnExists($col,$table)) continue;
	    $q = $db->qquery("SELECT $col FROM $table GROUP BY $col");
	    while($r=$db->next_record($q)) tidy_country($r[$col],$table,$col,$col);
	  }
	}
      }
    }    

    guess_ap_country('ap_place_of_birth','ap_citizenship');
    guess_ap_country('ap_institute',     'ap_residentship');
    guess_ap_country('ap_phd_institute', 'ap_phd_country');
    guess_ap_country('ap_ms_institute',  'ap_ms_country');
    print '<h1>country statistics</h1>';
    arsort($stat,SORT_NUMERIC);
    print_r($stat);
  }else{
    print "<h1>myPear_db()->table DOES NOT Exists('jam_old_applicants')</h1>\n";
  }
}

/*
 * Remove AV records corresponding to the applicants, they are not reliable
 */
function ap2av_cleanup(){

  foreach(array('jam_old_applicants',
		'jam_old_recommendations',
		'jam_old_reviews',
		'jam_old_avatars') as $table)  if (!myPear_db()->tableExists($table)) return;

  print '<h1>'.__FUNCTION__.' remove test records from the JAM (my debug in 2009)</h1>';

  foreach(array('zzz_avatars','jam_old_avatars') as $t){
    foreach(array('av_fax','av_comment') as $f){
      if (myPear_db()->columnExists($f,$t)){
	myPear_db()->qquery("ALTER TABLE $t  DROP `$f`",1);
	myPear_db()->reset_cache();
      }
    }
  }
  
  // old debug
  $q = myPear_db()->qquery("DELETE FROM jam_old_avatars  WHERE av_email REGEXP 'nowhere.com'",1);
  
  //
  //
  print '<h1>'.__FUNCTION__.'_clean empty names</h1>';
  
  foreach(array('zzz_avatars','jam_old_avatars','jam_old_applicants') as $t){
    $pfx = ($t=='jam_old_applicants' ? 'ap' : 'av');
    
    // 't Hooft ...
    $lastname = $pfx.'_lastname';
    $q = myPear_db()->qquery("SELECT * FROM $t WHERE $lastname REGEXP \"'\"",1);
    while($r = myPear_db()->next_record($q)){
      $id = $r["${pfx}_id"];
      myPear_db()->qquery("UPDATE $t SET ".myPear_db()->quote($r[$lastname],1,$lastname)." WHERE  ${pfx}_id = $id",1);
    }
    
    // various misnames
    $l = array();    foreach(array('last','first') as $n) $l[] = "${pfx}_${n}name NOT REGEXP '[a-zA-Z]'";
    $q = myPear_db()->qquery("SELECT * FROM $t WHERE ".join(' OR ',$l),1);
    while ($r = myPear_db()->next_record($q)){
      list($k,$v) = (empty($r['av_id']) 
		     ? array('ap_id',$r['ap_id']) 
		     : array('av_id',$r['av_id']));
      $updates = array();
      foreach(array('last','first') as $n){
	$name = "${pfx}_${n}name";
	if ($r[$name] !== '-') if (!preg_match('/[a-zA-Z]/',$r[$name])) $updates[$name] = "'Unknown'";
      }
      if (!empty($updates)) myPear_db()->qquery("UPDATE $t SET ".b_fmt::joinX(',',$updates)." WHERE $k = $v",True);
    }
  }

  //
  //
  //

  print '<h1>'.__FUNCTION__.'</h1>';
  
  $q = myPear_db()->query("SELECT ap_lastname,ap_firstname,ap_avid,ap_id FROM jam_old_applicants WHERE ap_avid > 0");
  $table1 = $table2 = array();
  while ($r = myPear_db()->next_record($q)){
    myPear_db()->qquery("UPDATE jam_old_applicants SET ap_avid = 0 WHERE ap_id=$r[ap_id]");
    
    // Rare case when applicant is a referee
    $q1 = myPear_db()->qquery("SELECT * FROM jam_old_recommendations WHERE rec_avid=$r[ap_avid]",1);
    $toKeep1 = myPear_db()->num_rows($q1);
    
    // Rare case when applicant is a revier
    $q2 = myPear_db()->qquery("SELECT * FROM jam_old_reviews         WHERE   r_avid=$r[ap_avid]",1);
    $toKeep2 = myPear_db()->num_rows($q2);
    
    if ($toKeep1+$toKeep2 == 0){
      myPear_db()->query("DELETE FROM jam_old_avatars WHERE av_id=$r[ap_avid]");
      @$table1['Remove']++;
    }else{
      if ($toKeep1) @$table2["$r[ap_firstname] $r[ap_lastname]"]['Referee'] = $toKeep1;
      if ($toKeep2) @$table2["$r[ap_firstname] $r[ap_lastname]"]['Reviews'] = $toKeep2;
    }
  }
  ksort($table2);
  b_debug::print_r($table1,'removed avars');
  b_debug::print_r($table2,'kept avatars');

  //
  //
  //

  print '<h1>'.__FUNCTION__.'_preserve</h1>';

  // build the list of tables containing _avid 
  $list_jam_tablesForRenumber = bForm_Avatar::avid_tables(jam_tablesForRenumber());
  b_debug::print_r($list_jam_tablesForRenumber,'list_jam_tablesForRenumber');

  $q = myPear_db()->qquery("SELECT av_id FROM jam_old_avatars",1);
  $list = array();
  while ($r = myPear_db()->next_record($q)) $list[] = 'av_id='.$r['av_id'];

  $qq = myPear_db()->query("SELECT av_id FROM zzz_avatars WHERE ".join(' OR ',$list));

  while ($rr = myPear_db()->next_record($qq)){
    @$av2av_known_preserve++;
    $av_id = $rr['av_id'];
    
    print "<h2>$av2av_known_preserve - Preserve clashing av_id=$av_id</h2>\n";
    $av_new = bMaster()->reserve_masterCounter();

    // replace x_avid/av_id by the new value
    myPear_db()->qquery("UPDATE jam_old_avatars SET av_id='$av_new' WHERE av_id='$av_id'",1);
    foreach ($list_jam_tablesForRenumber as $table=>$cc){
      foreach($cc as $c){
	myPear_db()->qquery("UPDATE $table SET $c='$av_new' WHERE $c='$av_id'",1);
      }
    }
  }
}

/*
 * Create or hook the Avatar record corresponding to the old Applicant record
 *
 * @param - (array)$r - old AP record
 */
function ap2av_exec(&$r){
  global $av_stat;

  // Fill AV from AP
  $av_data = array();
  foreach(bForm_jam_Applicant::$ap2av as $t=>$f){
    foreach(array($f, str_replace('_ap_','ap_',$f)) as $from){
      $v = trim(@$r[$from]);
      if (!empty($v)) $av_data[$t] = $v;
    }
  }
  
  if     (!empty($av_data['av_email']))  $av = bForm_Avatar::hook(array('av_email'=>$av_data['av_email']));
  elseif (!empty($av_data['ap_email']))  $av = bForm_Avatar::hook(array('av_email'=>$av_data['ap_email']));
  else                                   $av = 0;
  @$av_stat[(is_object($av) 
	     ? 'ap2av - AV is already known to zzz_avatars' 
	     : 'ap2av - AV is created' )]++;
  
  foreach($av_data as $k=>$v){
    switch($k){
    case 'av_pwd2':
      if(is_object($av)) $av_pwd2 = @$av->formDB[$k]; 
      else               $av_pwd2 = '';
      $a = array_unique(array_merge(explode(',',trim($r['ap_pwd'])),
				    explode(',',trim($r['ap_pwd_spurious'])),
				    explode(',',trim($av_pwd2))));
      sort($a);
      $av_data[$k] = join(',',$a);
      $av_data[$k] = preg_replace('/(^,|,$)/','',str_replace(',,',',',$av_data[$k]));
      if(is_object($av)){
	if($av_data[$k] == $av->formDB[$k]) unset($av_data[$k]);
	else                                unset($av->formDB[$k]);
      }
      break;
    case 'av_id2':
      if(is_object($av)) $av_id2 = @$av->formDB[$k]; 
      else               $av_id2 = '';
      $a = array_unique(array_merge(explode(',',trim($r['ap_id'])),
				    explode(',',trim($r['ap_id_spurious'])),
				    explode(',',trim($av_id2))));
      sort($a);
      $av_data[$k] = join(',',$a);
      $av_data[$k] = preg_replace('/(^,|,$)/','',str_replace(',,',',',$av_data[$k]));
      if(is_object($av)){
	if($av_data[$k] == $av->formDB[$k]) unset($av_data[$k]);
	else                                unset($av->formDB[$k]);
      }
      break;
      
    case 'av_birthdate':
    case 'av_ms_year':
    case 'av_phd_year':
      
      $vo = $av_data[$k];
      if((is_object($av) && !empty($av->formDB[$k])) || (is_numeric($vo) && (int)$vo == 0)){	  
	if (is_object($av))	$v = $av->formDB[$k];
	unset($av_data[$k]);
      }elseif(preg_match(';(\d\d)/(\d\d)/(\d\d\d\d);',$vo,$m)){
	$av_data[$k] = b_time::txt2unix("$m[3]-$m[2]-$m[1]");
      }elseif(preg_match(';\b([12]\d\d\d)\b;',$vo,$m)){
	if ($m[1] > 1900 && $m[1] < CONST_eternity_year)	$av_data[$k] = b_time::y2d($m[1]);
      }else{
	$year = (int)$vo;
	if ($year > 1900 && $year < CONST_eternity_year){
	  $av_data[$k] = b_time::y2d($year);
	}else{
	  $av_data[$k] = 0; 	  // "0" means "not set", TBD to the SQL "null"
	  $av_data[$k] = 'NULL'; 
	  myPear::DEBUG("TBD $k: $vo (now is set to '".$av_data[$k]."')");
	}
      }
      break;
      
    default:
      if(is_object($av) && !empty($av->formDB[$k]))	      unset($av_data[$k]);
    }
  }
  
  if (is_object($av)){
    printf("Known  ==============================   %s\n",b_fmt::unEscape($av->name()));
    foreach($av_data as $k=>$v){
      if (($av_1=@$av->getValue($k,1)) != ($av_2=@$av_data[$k])){
	printf("        %+20s in DB = '%s', but here ='%s' \n",$k,$av_1,$av_2);
      }
      $av->updateDB($av_data);
    }
  }else{
    $av = bForm_Avatar::hook($av_data,$doNotCreate=False);
    if (is_object($av)){
      printf("Create ++++++++++++++++++++++++++++++   %s\n",b_fmt::unEscape($av->name()));
    }else{
      b_debug::print_r($r,"Not AV ??????????????????????????????");
      b_debug::print_r($av_data,'av_data');
    }
    // if (@$GLOBALS['sdsdsd']++ < 10) b_debug::print_r($av_data,'unknown');
  }
  return $av;
}
