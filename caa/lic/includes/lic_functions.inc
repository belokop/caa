<?php

function lic_showDB($arg) {
  global $Where;
  locateAndInclude('lic_updates.inc');
  if ($arg == LIC_MODULE) $arg = 'ALL';
  if (bAuth()->loginPrompt()){
    print myPear::H1(($arg?"\"$arg\" ":' ').bText::_('licenses usage'),array('reset','noTranslate'));
    
    $show_all = (empty($arg) || mb_strToLower($arg)=='all');
    $where = array();
  
    if (!$show_all)$where[] = "s_name  = '$arg'";
    if ($where) $Where = "WHERE ".join(' AND ',$where);
    
    $considered_now = time() - 15*60; // last 15 minutes
    if (!@$_GET['sBy'])             $_GET['sBy'] = 'e_time';
    if (  $_GET['sBy'] == 'e_time') $_GET['sByDir'] = 'DESC';
    
    // get the software
    $q=myPear_db()->query("SELECT MAX(e_time) AS last_access ".
			  " FROM lic_event ".
			  " LEFT JOIN lic_soft   ON lics_id   = e_sid ".
			  " $Where GROUP BY NULL ");
    while($r = myPear_db()->next_record($q)) $last_access = $r['last_access'] - 1;
    
    // get the logs
    if (!myPear_db()->columnExists($_GET['sBy'])) $_GET['sBy'] = 'e_time';
    $q=myPear_db()->query("SELECT *,CONCAT(srv_port,'@',srv_name) AS srv, CONCAT(s_name,'-',s_vrsn) AS soft ".
			  " FROM lic_event ".
			  " LEFT JOIN lic_server ON licsrv_id = e_srvid ".
			  " LEFT JOIN lic_host   ON lich_id   = e_hostid ".
			  " LEFT JOIN lic_user   ON licu_id   = e_userid ".
			  " LEFT JOIN lic_soft   ON lics_id   = e_sid ".
			  " $Where ORDER BY $_GET[sBy] $_GET[sByDir] ".
			  " LIMIT ".b_cnf::variable_get('LIC_maxLines',111));
    
    $h['e_time']  = 'Date';
    if ($show_all) $h['soft']   = 'Software';
    else           $h['s_vrsn'] = 'Version';
    $h['u_name']  = 'User';
    $h['h_name']  = 'Host';
    $h['srv']     = 'Lic.server';
    print "<center>";
    $t = new b_table_zebra($h);
    $t->showLineCounter = False;
    $t->repetValuesCol1only = $_GET['sBy'];
    $t->getHeader();
    while($r = myPear_db()->next_record($q)) {
      if (preg_match("/^i000[-\.0-9]*$/",$r['soft']))                                 continue;
      if ($r['e_time'] >= $considered_now) $t->extraTD[] = bIcons()->get(array('i'=>'i-sys-monitor','d'=>'Active now'));
      $r['u_name'] = preg_replace("/administr.*/","administrator",mb_strToLower($r['u_name']));
      $r['soft']   = preg_replace('/\.0+/','.0',$r['soft']);
      $r['s_vrsn'] = preg_replace("/00$/",'',$r['s_vrsn']);
      $r['srv']    = preg_replace('/.*@/','',$r['srv']);
      $r['h_name'] = lic_tidyHost(mb_strToLower($r['h_name']));
      $r['e_time'] = b_time::strftime("%Y-%m-%d",$r['e_time']);
      $t->prt($r);
    }
    $t->close();
    bIcons()->explain();
    print "</center>";
  }
}

function lic_fillDB($debug=False) {

  locateAndInclude('lBasic');
  global $regs;
  print b_time::strftime("%Y-%m-%d %H:%M\n",time());
  
  $cmd = b_cnf::get('monitorExec');
  /*
  License server status: 27000@meklab02.fysik.su.se
  License file(s) on meklab02.fysik.su.se: C:\National Instruments\Shared\License Manager\Licenses\LV_RemotePanelConnection.lic:
  meklab02.fysik.su.se: license server UP v7.2
  Vendor daemon status (on MEKLAB02):
    nilm: UP v7.2
  Feature usage info:
  Users of LabVIEW_RemotePanelConnection:  (Total of 1 license issued;  Total of 0 licenses in use)
  */
  if ($f=popen("$cmd 2>&1","r")) {
    while (!feof($f)) {
      myPear_db()->debug = $debug;
      $l = rtrim (fgets($f,1000));
      b_debug::_('lic_fillDB','',$l);
      if (preg_match('/^Service: (\S+) /',$l,$regs)){
	$service = $regs[1]; 
	unset ($serverClass);
	unset ($hostClas);
	unset ($softClass);
      }
      //License server status: 1700@syslx13.fysik.su.se,1700@syslx14.fysik.su.se,1700@syslx08.fysik.su.se
      if (preg_match('/License server status: (\d+)@(.*)/',$l,$regs)){
	$srv_port = $regs[1];
	if ($debug) print "$l\n       ------> $regs[1]\n";
	$serverClass = new lServer();
	$serverClass->debug = $debug;
	$serverClass->updateDB(array('srv_name'=>lic_tidySrv($regs[2],$srv_port),
				     'srv_port'=>$srv_port));
      }

      // License file(s) on el-09.fysik.su.se: C:\National Instruments\Shared\License Manager\Licenses\LV_RemotePanelConnection.lic:
      if (preg_match('/License file.s. on (\S+):/',$l,$regs)){
	if ($debug) print "$l\n       ------> $regs[1]\n";
	$serverClass = new lServer();
	$serverClass->debug = $debug;
	$serverClass->updateDB(array('srv_name'=>lic_tidySrv($regs[1],$srv_port),
				     'srv_port'=>$srv_port));
      }

      // mathematica start syslx52.fysik.su.se 7.0 
      // mathematica rt 128.111.247.29 ronny-thomales-macbook-pro.local 7.0 
      // mathematica end
      if (preg_match('/^mathematica (\S+)\b/', $l, $regs)){
	switch ($regs[1]){
	case 'start':
	  list($id,$code,$host,$vrsnLM) = explode(' ',mb_strToLower($l));
	  $serverClass = new lServer();
	  $serverClass->debug = $debug;
	  $serverClass->updateDB(array('srv_name'=>lic_tidySrv($host,b_cnf::variable_get('LIC_mathlmPort',16286)),
				       'srv_port'=>b_cnf::get('LIC_mathlmPort')));
	case 'end':
	  break;
	  
	default:
	  list($id,$user,$ip,$host,$vrsn) = explode(' ',mb_strToLower($l));
	  $softClass = new lSoft();
	  $softClass->debug = $debug;
	  $softClass->updateDB(array('s_name'=>'mathematica', 's_vrsn'=>$vrsn));
	  
	  $hostClas = new lHost();
	  $hostClas->debug = $debug;
	  $hostClas->updateDB(array('h_name'=>lic_tidyHost($ip)));
	  
	  $userClass = new lUser();
	  $userClass->debug = $debug;
	  $userClass->updateDB(array('u_name'=>mb_strToLower($user)));
	  
	  $eClass = new lEvent();
	  $eClass->debug = $debug;
	  $eClass->updateDB(array('e_userid'=>$userClass->ID,
				  'e_hostid'=>$hostClas->ID,
				  'e_srvid' =>$serverClass->ID,
				  'e_sid'   =>$softClass->ID));
	  $eClass->updateDB(array('e_time'  =>time()));
	}
      }
      
      // "Maple11" v2008.0331, vendor: maplelmg
      // "I00000000000000000080200001000" v2010.0228, vendor: INTEL
      // "FCompL" v2010.0228, vendor: INTEL
      if (preg_match('/^\s+"(\S+)"\s+(\S+),.*vendor: (\S+)/',$l,$regs)){
	  lic_tidySoft($regs);
	  if (!empty($regs)) {
	    $softClass = new lSoft();
	    $softClass->debug = $debug;
	    $softClass->updateDB(array('s_name'=>strToLower($regs[1]),
				       's_vrsn'=>preg_replace('/^v/','',$regs[2])));
	  }
      }
      //yarevsky mollx52.fysik.su.se /dev/pts/0 (v2008.0213) (syslx17.fysik.su.se/28518 206), start Thu 1/15 6:36
      //belokop norlxu01.albanova.se norlxu01.albanova.se/localhost:10 (v7.0) (syslx13.fysik.su.se/1700 772), start Fri 1/2 21:28, 6 licenses
      //yb cnlx03.fysik.su.se /dev/pts/5 (v2007.0216) (syslx08.fysik.su.se/27000 337), start Sat 1/3 18:06
      if (preg_match('/^\s+(\S+) (\S+) .*start/',$l,$regs)){
	if ($debug) print "$l\n       ------> $regs[1]\n";
	preg_match('/ (\d+) licenses$/',$l,$lic);
	$hostClas = new lHost();
	$hostClas->debug = $debug;
	$hostClas->updateDB(array('h_name'=>lic_tidyHost($regs[2])));
	
	$userClass = new lUser();
	$userClass->debug = $debug;
	$userClass->updateDB(array('u_name'=>mb_strToLower($regs[1])));
	
	$eClass = new lEvent();
	$eClass->debug = $debug;
	$eClass->updateDB(array('e_userid'=>$userClass->ID,
				'e_hostid'=>$hostClas->ID,
				'e_srvid' =>$serverClass->ID,
				'e_sid'   =>$softClass->ID));
	$eClass->updateDB(array('e_time'  =>time()));
      }
      // Users of Maple10WithExcel:  (Total of 2 licenses available)      $soft = preg_replace("/\"/","",$regs[1]);
      // NAG: UP v9.2
      // lmgrd is not running: Cannot read data from license server (-16,287)
    }
  }
}
