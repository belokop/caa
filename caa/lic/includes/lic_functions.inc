<?php

function lic_showDB($arg) {
  locateAndInclude('lic_updates.inc');

  if (!bAuth()->loginPrompt()) return '';
  
  if (empty($arg) || ($arg === LIC_MODULE)) $arg = 'ALL';
  b_debug::xxx("arg=$arg",array(2,cnf_dev));

  $show_all = (strToLower($arg)=='all');
  $header = array('e_time' => 'Date',
		  'soft'   => 'Software',
		  's_vrsn' => 'Version',
		  'u_name' => 'User',
		  'h_name' => 'Host',
		  'srv_name'=> 'Lic.server');

  $h1 = sprintf('"%s" %s',$arg,bText::_('licenses usage'));
  
  $where = array(1);
  //  if (!$show_all)
  foreach(array_keys($header) as $k){
    if (!myPear::is_empty($v=@$_GET[$k]) && myPear_db()->columnExists($k)){
      $where[] = ($k == 'e_time'
		  ? sprintf("$k > %s AND $k < %s",b_time::_($v,0),b_time::_($v,24))
		  : myPear_DB()->quote($v,True,$k));
      $h1 = ($k == 'e_time'
	     ? $header[$k]." ".date('Y-m-d',$v) 
	     : $header[$k]." $v"); 
      b_cnf::_unset($k);
    }  
  }  
  unset($header[$show_all||!empty($h1) ? 's_vrsn' : 'soft']);
  if (!$show_all && ($where===array(1)))    $where[] = myPear_DB()->quote($arg,True,'s_name');
  
  print myPear::H1($h1,array('reset','noTranslate'));

  $considered_now = time() - 15*60; // last 15 minutes
  $requestToSort = !empty($_GET['sBy']);
  if (!myPear_db()->columnExists(@$_GET['sBy'])) $_GET['sBy'] = 'e_time';
  if (  $_GET['sBy'] == 'e_time') $_GET['sByDir'] = 'DESC';
  if (cnf_dev) b_debug::var_dump($where);
  
  // get the logs
  $query = $requestToSort 
    ? $_SESSION[LIC_MODULE.'query']
    : "SELECT *,CONCAT(srv_port,'@',srv_name) AS srv, CONCAT(s_name,'-',s_vrsn) AS soft ".
    " FROM lic_event ".
    " LEFT JOIN lic_server ON licsrv_id = e_srvid ".
    " LEFT JOIN lic_host   ON lich_id   = e_hostid ".
    " LEFT JOIN lic_user   ON licu_id   = e_userid ".
    " LEFT JOIN lic_soft   ON lics_id   = e_sid ".
    " WHERE ".join(' AND ',$where).
    " ORDER BY $_GET[sBy] $_GET[sByDir] ".
    " LIMIT ".b_cnf::variable_get('LIC_maxLines',111);
  $_SESSION[LIC_MODULE.'query'] = $query;

  $q=myPear_db()->qquery($query,cnf_dev);
  
  print "<center>";
  $t = new b_table_zebra($header);
  $t->showLineCounter = False;
  $t->repetValuesCol1only = $_GET['sBy'];
  $t->getHeader();
  while($r = myPear_db()->next_record($q)) {
    if (preg_match("/^i000[-\.0-9]*$/",$r['soft'])) continue;
    if ($r['e_time'] >= $considered_now) $t->extraTD[] = bIcons()->get(array('i'=>'i-sys-monitor','d'=>'Active now'));
    $t->prt(array('u_name' => x("a href=".x("'",b_url::same("?u_name=$r[u_name]")),preg_replace("/administr.*/","administrator",mb_strToLower($r['u_name']))),
		  'soft'   => x("a href=".x("'",b_url::same("?soft=$r[soft]")),    preg_replace('/\.(.)0+$/','.\\1',$r['soft'])),
		  's_vrsn' => x("a href=".x("'",b_url::same("?s_vrsn=$r[s_vrsn]")),preg_replace("/\.(.)0+$/",'.\\1',$r['s_vrsn'])),
		  'h_name' => x("a href=".x("'",b_url::same("?h_name=$r[h_name]")),lic_tidyHost(mb_strToLower($r['h_name']))),
		  'e_time' => x("a href=".x("'",b_url::same("?e_time=".b_time::noon(0,0,0,$r['e_time']))),date("Y-m-d",$r['e_time'])),
		  'srv_name'=> x("a href=".x("'",b_url::same("?srv_name=$r[srv_name]")),$r['srv_name']),
		  ));
    b_time::strftime("%Y-%m-%d",$r['e_time']);
  }
  $t->close();
  bIcons()->explain();
  print "</center>";
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
