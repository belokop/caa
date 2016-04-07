<?php
define('myPear_root_files','/Users/yb/Sites/drupal/sites/default/files/');
/*
 * Convert the old preprints database to myPear
 */
$verbose = 0;
$_GET['org'] = 'nordita';
$_GET['q']   = 'prp';
$_GET['warnings']   = 1;
$_GET['showdb']   = 1;
$_GET['smtpdebug']   = 1;

error_reporting(E_ALL);

require_once  '../myPear/config.inc';

bMaster()->reset();
myPear_db()->truncateTable('prp_list');

require_once  '../myPear/includes/myPear.inc';
require_once  '../preprints/config.inc';
require_once  '../myPear/includes/bForm.inc';
require_once  '../myPear/includes/bHolder/Logs.inc';
require_once  '../myPear/includes/b_registry.inc';
b_reg::$current_module = PRP_MODULE;
require_once  '../preprints/includes/prp_updates.inc';
require_once  '../preprints/includes/bForm/prp.inc';
locateAndInclude('bForm_Avatar_prp');

print x('h1',"Start ".__FILE__).'<br/>';

foreach(array('prp_old_accounts','prp_old_publications') as $t){
  if (!myPear_db()->tableExists($t)){
    print "*** table '$t' doesn't not exist, aborting\n";
    exit;
  }
}

function copyDB(){
  $fr="/afs/fysik.su.se/common/www/nordita.preprints/????";
  $fr="root@www.nordita.org:/data/DocumentRoot/www.nordita.org/preprintsDB/????";
  foreach(array(myPear_root_files.PRP_MODULE,
		'/data/DocumentRoot/files/prp',
		'/Users/yb/Sites/data.area/files.drupal/prp') as $t){
    if (!is_dir(dirname($t))) continue;
    b_os::mkdir($t);
    $to = "$t/" .  sprintf("%06d/",myOrg('nordita')->ID);
    b_os::mkdir($to);
    if (is_dir($to))   print "rsync -av --exclude test $fr $to ; chown -R www $to\n";
  }
}

copyDB();

$ok_pwd = False;
$afs = '/setup/maintenance/mail/2leif/passwd';
if (file_exists($afs)) system("cp -pvf $afs passwd");
foreach(array($afs, 'passwd') as $pwd){
  if (!$ok_pwd){
    if ($handle = @fopen($pwd, "r")){
      while (($buffer = fgets($handle, 4096)) !== false) {
	$p = explode(':',$buffer);
	if (!empty($p[2])) $passwd[$p[2]] = $p;
      }
      if (!feof($handle)) {
	echo "Error: unexpected fgets() fail\n";
      }else{
	$ok_pwd = True;
      }
      fclose($handle);
    }
  }
}
if (!$ok_pwd){
  die("no password file");
}

foreach(array('hvzm') as $manager){
  $av = bForm_Avatar::hook(array('av_identity'=>$manager));
  if (is_object($av)) PRP::_MANAGERS_UNIT()->add_unitMember($av);
  else  b_debug::internalError("can't find $manager");
}

foreach(array('brandenb') as $censor){
  $av = bForm_Avatar::hook(array('av_identity'=>$censor));
  if (is_object($av)) PRP_censors()->add_unitMember($av);
  else  b_debug::internalError("can't find $censor");
}

$min_day0 = time() + 9999999;
$max_day0 = 0;
$xxx = $yyy = array();
$q = myPear_db()->qquery("SELECT * from prp_old_publications LEFT JOIN prp_old_accounts ON prp_old_publications.Id = prp_old_accounts.id",1);
while ($r = myPear_db()->next_record($q)){
  if ($verbose)   print "------------------------- $r[Year]-$r[Report]\n";
  foreach($r as $k=>$v) if (empty($v)) unset($r[$k]);
  $av = Null;
  if (empty($r['id'])){
    if ($r['Id'] == 1003)   $r['Id'] = 2197811;
    if (empty($passwd[$r['Id']])){
      if ($verbose) print "???????????? $r[Id]\n";
      $r['username'] = nobody_at_nowhere;
    }else{
      $av = bForm_Avatar::hook($passwd[$r['Id']][0]);
      if ($av){ 
	$av->defineVariables();
	if ($verbose) print '!!!!!!!!!!!!!!!!! '.b_fmt::unescape($av->name())."\n"; 
      }else{
	if($r['Id'] ==   782)  $r['username'] = 'fabio@nordita.org';
	if($r['Id'] ==   854)  $r['username'] = 'stefan.hofmann@physik.lmu.de';
	if($r['Id'] ==   884)  $r['username'] = 'zingg@nordita.org';
	if($r['Id'] ==   894)  $r['username'] = 'es547@nordita.org';
	if($r['Id'] ==   996)  $r['username'] = 'marotta@chalmers.se';
	if($r['Id'] ==  2007)  $r['username'] = 'maciej.trzetrzelewski@gmail.com';
	if($r['Id'] ==  9409)  $r['username'] = 'ktwikfeldt@gmail.com';
	if($r['Id'] ==145708)  $r['username'] = 'fep@fma.if.usp.br';
	if($r['Id'] ==2319904) $r['username'] = 'dhruba@nordita.org';
	if($r['Id'] ==2190860) $r['username'] = 'lars.mattsson@nordita.org';
	if($r['Id'] ==2250981) $r['username'] = 'anders.kvellestad@nordita.org';
	if($r['Id'] ==2251791) $r['username'] = 'harsha.raichur.astro@gmail.com';
	if($r['Id'] ==2266022) $r['username'] = 'erik.widen@nordita.org';
	if($r['Id'] ==2343032) $r['username'] = 'jennifer.schober@nordita.org';
	if($r['Id'] ==2252621) $r['username'] = 'yaron.kedem@nordita.org';
	if($r['Id'] ==268166)  $r['username'] = 'marit.sandstad@astro.uio.no';
	if($r['Id'] ==2346410) $r['username'] = 'marit.sandstad@astro.uio.no';
	if($r['Id'] ==268161)  $r['username'] = 'miguel.zumalacarregui@nordita.org';
	if($r['Id'] ==2264808) $r['username'] = 'miguel.zumalacarregui@nordita.org';
	if (empty($r['username'] ))print "???????????? unmathed $r[Id] ".join(':',$passwd[$r['Id']])."\n";
      }
    }
  }
  
  if (@$r['username'] === 'testien@nordita.org') continue;
  foreach(array('bijnens' =>'bijnens@thep.lu.se',
		'weigert' =>'heribert.weigert@physik.uni-regensburg.de',
		'Audun.Bakk'=>'Audun.Bakk@sintef.no',
		'merlatti'=>'merlatti@to.infn.it',
		'baym'    =>'gbaym@illinois.edu',
		'grimstrp'=>'jesper.grimstrup@gmail.com',
		'micho'   => 'michael.hornquist@liu.se',
		'kasper'  =>'kasper@nordita.dk',
		'hoefner' =>'hoefner@astro.uu.se',
		'melwyn'  =>'melwyn@nbi.dk',
		'ingves'  =>'ingves@phys.ntnu.no',
		'dahle'   =>'hdahle@astro.uio.no',
		'tambjorn'=>'tobias.ambjornsson@thep.lu.se',
		'freyhult'=>'lisa.freyhult@physics.uu.se',
		'kirstine'=>'orensen@fysik.dtu.dk',
		'jonsell' =>'jonsell@fysik.su.se',
		'tuomas'  =>'tuomul@utu.fi',
		'teresia' =>'teresiam@kth.se',
		'mlomholt'=>'mlomholt@memphys.sdu.dk',
		'gentaro' =>'gentaro_watanabe@apctp.org',
		'molsson' =>'molsson@nordita.dk',
		'kristjansen'=>'kristjan@nbi.dk',
		'kristk'  =>'kristk@nordita.dk',
		'tuominen'=>'kimmo.tuominen@phys.jyu.fi',
		'rapp'    =>'rapp@comp.tamu.edu',
		'aristov' =>'dmitry.aristov@phys.spbu.ru',
		'mkorpi'  =>'maarit.korpi@helsinki.fi',
		'tauris'  =>'ttauris@astro.ku.dk',
		'ouyed'   =>'ouyed@phas.ucalgary.ca',
		'phoyer'  => 'paul.hoyer@helsinki.fi',
		'sneppen' =>'sneppen@nbi.dk',
		'diakonov'=>'diakonov@thd.pnpi.spb.ru',
		'alava'   =>'mikko.alava@tkk.fi',
		'gornyi'  =>'igor.gornyi@kit.de',
		'oelgaroy'=>'oystein.elgaroy@astro.uio.no',
		'sylju'   =>'o.f.syljuasen@fys.uio.no',
		'minnhag' =>'minnhagen@tp.umu.se',
		'metz'    =>'metz@ph.tum.de',
		'anja@'   =>'anja@astro.ku.dk',
		'sannino' => 'sannino@ifk.sdu.dk',
		'obers'   =>'obers@nbi.dk',
		'andersw' =>'anders.westerberg@cern.ch',
		'teobert' =>'bertmat@sissa.it',
		'andreas.isacsson'=>'andreas.isacsson@chalmers.se',
		'ajokinen'=>'Asko.Jokinen@helsinki.fi',
		'qhp2009' =>'ardonne@nordita.org',
		'TeV2008' =>'ardonne@nordita.org',
		'Chirality2008' =>'ardonne@nordita.org',
		) as $o=>$n)
    if (preg_match("/^$o/i",@$r['username']))       $r['username'] = $n;
  
  if (preg_match(',nordita.*preprintsDB,i',@$r['Location'])){
    $local = basename($r['Location']);
    if (basename(dirname($r['Location'])) != $r['Year']) print_r($r);
    //print $r['Location']." -> $local\n";
    $r['Location'] = '';
  }else{
    $local = '';
  }

  //if (strpos($r['Title'],'\\')) print "title: $r[Title]\n";
  if ((int)@$r['Year'] == 1997 && (int)$r['Report'] == 52) $r['username'] = nobody_at_nowhere;
  if (trim(@$r['full_name']) == 'Unknown')                 $r['username'] = nobody_at_nowhere;
  if (trim(@$r['username'])  == 'urkedal@nbi.dk')          $r['username'] = nobody_at_nowhere;
  if (empty($r['username'])) $r['username'] = 'username';
  if (empty($av)) $av = bForm_Avatar::hook(str_replace('nordita.dk','nordita.org',$r['username']));
  if (empty($av)) $av = bForm_Avatar::hook($r['username']);
  
  // DOI
  //  <a href=http://dx.doi.org/10.1051/0004-6361/201117023> Astron. Astrophys. 534, A11 (2011)</a>
  if (preg_match('|dx.doi.org/(.*)>.*<|i',($sv=@$r['PublIn']),$m)){
    $r['DOI'] = trim(preg_replace('/>.*/','',preg_replace('/<.*/','',str_replace('"','',$m[1]))));
    $r['PublIn'] = trim(strip_tags(@$r['PublIn']));
    print "DOI: $sv  \n$r[DOI]   \n$r[PublIn] \n\n"; 
  }else{
    $r['DOI'] = '';
  }

  if ($av){
    $av->defineVariables();
    print "\n------------------------------------------------------------".b_fmt::unescape($av->name()).' '.$av->getEmail()."\n";

    if ((int)$r['Year'] > 1955){
      $date = $r['Tm'];
      if (date('Y',$date) != $r['Year']){
	$d = b_time::txt2unix(join('-',array($r['Year'],'01',$r['Report']+1)));
	$date = $d;
      }
      $max_day0 = max($max_day0, $date);
      $min_day0 = min($min_day0, $date);
    }else{
      $date = 0;
    }

    $prp = new bForm_prp('new');
    $prp->debug = 0;
    $prp->updateDB(array('prp_orgid'    => myOrg('nordita')->ID,
			 'prp_avid'     => $av->ID,
			 'prp_local'    => @$local,
			 'prp_report'   => @$r['Report'],
			 'prp_day0'     => @$date,
			 'prp_doi'      => @$r['DOI'],
			 'prp_status'   => @$r['Status'],
			 'prp_field'    => @$r['Field'],
			 'prp_archive'  => @$r['Location'],
			 'prp_authors'  => @$r['Authors'],
			 'prp_title'    => @$r['Title'],
			 'prp_publisher'=> @$r['PublIn'],
			 'prp_timestamp'=> b_time::sql_timestamp(@$r['Tm']),
			 ));
  }else{
    //    print_r($r);
    if (empty($r['full_name'])) $r['full_name'] = 'full_name';
    if (empty($r['username']))  $r['username']  = 'username';
    $xxx[trim($r['username'])][$r['full_name']][] = $r['Title']."; ".$r['Authors'];
  }
  if (empty($yyy[@$r['Status']])) $yyy[@$r['Status']] = 0;
  $yyy[@$r['Status']]++;
}

foreach(preg_grep('/prp_old/',myPear_db()->getTables()) as $t){
  //  myPear_db()->qquery("DROP TABLE `$t`",True);
}

b_debug::print_r($yyy,'summary '.date('Y-m-d',$min_day0).' => '.date('Y-m-d',$max_day0));
b_debug::print_r($xxx,'non-mathed avatars');

copyDB();
