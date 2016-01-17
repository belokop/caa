<?php 
define('myPear_root_files','/Users/yb/Sites/drupal/sites/default/files/');
$_GET['org'] = 'nordita';
$sql = array('myPear'   =>'zzz_',
	     'vm'       =>'abs_',
	     'lic'      =>'lic_',
	     'rbs'      =>'rbs_',
	     'prp'      =>'prp_',
	     'preprints'=>'prp_',
	     'wiw'      =>'wiw_',
	     'jam'      =>'jam_',
	     );

set_include_path(get_include_path() . PATH_SEPARATOR . trim(`pwd -P`).'/../');
$pwd = $argv[1];

require_once  trim(`pwd -P`).'/../../myPear/config.priv.inc';
require_once  trim(`pwd -P`).'/../../myPear/config.inc';
locateAndInclude('myPear');

$src = dirname (trim(`pwd -P`));
$src = basename($src);
$searchKey = $sql[$src];
if (!$searchKey) b_debug::internalError("unknown '$src'");

$db = myPear_db();
$db->debug = 0;
foreach($db->getTables() as $table){
  if (preg_match("/^$searchKey/",$table) && !preg_match('/_old_|_sv_/',$table)){
    extractTable($table,(in_array($table,array('zzz_countries',
					       // 'zzz_semaphore',
					       ))));
  }
}

function extractTable($table,$withData=False){
  global $pwd;
  list($db,$host,$user) = array('combodb','localhost','root');
  $cmd = "mysqldump ".($withData ? '' : '--no-data')." --add-drop-table=FALSE  -u$user -p$pwd -h$host $db $table | ".
    "sed -e 's/CREATE TABLE/CREATE TABLE IF NOT EXISTS/g' "
    ." -e 's/ENGINE=.*/AUTO_INCREMENT=1;/g' "
    ." -e 's/COLLATE latin1_general_ci//' "
    ." -e 's/COLLATE utf8_swedish_ci//' "
    ." -e 's/collate latin1_general_ci//' "
    ." -e 's/ CHARACTER SET latin1 / /g' " 
    ." | "
    ."grep -v \"^/*/\" | grep -v \"^$\" | grep -v \"^.-\""
    . " > $table.sql"
    ;
  print "     $table ".($withData ? '' : '--no-data')."\n";
  system($cmd);
}

