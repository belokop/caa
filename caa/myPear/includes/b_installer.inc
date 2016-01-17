<?php
/*
 * This class is enetended for the initial myPear setup,
 * but is not used.
 */

require_once 'PEAR/Registry.php';

define('DBconfigfile' ,    'config_db.php');             
define('installDoneFlag' , '.install.all.ok');           
define('envChekDoneFlag' , '.install.env.ok');           

function b_install(){
  $i = new b_installer(trim(basename(dirname($_SERVER['PHP_SELF']))));
  $i->checkEnvironment();
  $i->go($_GET['stepname']);
}

class b_installer{
  var $steps = array(
		     'pdftk'    => array('install_exec','pdftk'),
		     'lang'     => 'choose_lang',
		     'lic'      => 'read_license',
		     'db'       => 'db_config',
		     'createdb' => 'create_tables',
		     #		     'funcs'    => 'register_funcs',
		     #		     'group'    => 'add_group',
		     'user'     => 'add_user',
		     #		     'final'    => 'finalize',
		     'end'      => 'end_install',
		     );
  
  var $checkp = array('required'=>array(//'MDB2'       =>'2',
					//'Auth'       =>'1.5',
					array('channel'=> 'base',
					      'pkg'    => 'php',
					      'version'=> '5.0.0'),
					array('channel'=> 'pear',
					      'pkg'    => 'Mail',
					      'version'=> '1.1.14'),
					array('channel'=> 'pear',
					      'pkg'    => 'HTML_Template_Sigma',
					      'version'=> '1.1'),
					array('channel'=> 'pear',
					      'pkg'    => 'Text_Wiki',
					      'version'=> '1.2.0'),
					array('channel'=> 'pear',
					      'pkg'    => 'NET_Imap',
					      'version'=> '1.1'),
					array('channel'=> 'pear',
					      'pkg'    => 'Net_SMTP',
					      'version'=> '1.3.3'),
					),
		      'optional'=>array(//'Log'        =>        '1.11',
					array('channel'=> 'extension',
					      'pkg'    => 'pam_auth',
					      'name'   => 'pam',
					      'version'=> '1.1')));
  
  function __construct($pkg){
    global $installerClass;
    $this->pkg = $pkg;
    $installerClass = $this;
  }
  
  /*
   * Perfrm sequential install of all the steps from $this->steps
   *
   * params
   * $stepName (optional) start form this step
   */ 
  function go($stepName='db'){
    if (file_exists(installDoneFlag) || b_cnf::get('installDoneFlag')) return;
    
    require 'install/lang/'.b_cnf::get('lang','en').'/install.php';
    
    $this->step = 0; 
    $this->stepName = $stepName;
    foreach ($this->steps as $name=>$content) { 
      $this->step++; if ($name==$stepName) break; 
    }
    $status = (int)(100 * $this->step / count($this->steps));
    
    require_once 'HTML/Template/Sigma.php';
    $i = new HTML_Template_Sigma('.');
    $i->loadTemplateFile('install/templates/main.html');
    $i->setVariable("package", $this->pkg);
    $i->setVariable("status",         $status);
    $i->setVariable("statusall",100 - $status);
    $i->setVariable("step",    $this->step);
    $i->setVariable("stepname",$stepName);
    $i->setVariable("allsteps",count($this->steps));
    $func = $this->steps[$stepName];
    if (is_array($func)) $i->setVariable("CONTENT", call_user_func($func[0],$func[1]));
    else                 $i->setVariable("CONTENT", call_user_func($func));
    $i->show();
  }
    
  function checkEnvironment($required=array(),
			    $optional=array()){
    locateAndInclude('PEAR_Registry');
    locateAndInclude('b_table');
    if (file_exists(envChekDoneFlag) || b_cnf::get('envChekDoneFlag')) return;
    
    if ($optional) $this->$checkp['optional'] = $optional;
    if ($required) $this->$checkp['required'] = $required;
    
    $this->printHeader();
    
    $peardir=$_POST['peardir'];
    if (empty($peardir)){
      foreach(preg_split('/:/',get_include_path()) as $path) 
	if (file_exists("$path/PEAR.php")) $peardir = $path;
    }
    print x('h2',"include_path = ".get_include_path());
    if (0)    print 
		"<h1>HOST ".`hostname`."</h1>".
		x("form action='' method='post'","PEAR include path:".
	"<input type='text' name='peardir' value='$peardir' />".
	"<input type='submit' value='check' />");
    $reg = new PEAR_Registry($peardir);
    $allp=array();
  
    b_debug::print_r($reg,"new PEAR_Registry($peardir)");
    foreach ($reg->listAllPackages() as $channel=>$names) {
      b_debug::_(get_class($this),'--------------------',$channel,True);
      foreach ($names as $package){
	locateAndInclude($package);	
	if (class_exists($package,0)) $e = ''; else $e = 'DOES NOT EXIST';
	$vrsn = $reg->_packageInfo($package, 'version ', $channel);
	b_debug::_(get_class($this),$package,"$vrsn $e",1);
	$allp[$package] = array($vrsn,preg_replace('/.php.net/','',$channel));
      }
    }
    
    $t = new b_table_zebra(array('a1'=>'Package',
				 'a2'=>'Installed version',
				 'a3'=>'Needed version',
				 'a4'=>'Check',
				 'a5'=>'Type',
				 'a6'=>'Channel',
				 'a7'=>'Comment'));

    $allrequ = True;
    foreach ($this->checkp as $w=>$packs){
      foreach($packs as $pack){
	$pn      = $pack['pkg'];
	if (!$name=$pack['name']) $name = $pn;
	$vrsn = $pack['version'];
	if ($pn == 'php') $allp['php'] = array(phpversion(), $pack['channel']);
	if ($p = $allp[mb_strToLower($pn)])    list($installedVersion,$channel) = $p;
	else                                list($installedVersion,$channel) = array('&nbsp;', $pack['channel']);
	
	if ($p && $installedVersion>=$vrsn) 
	  list($img,$txt) = array('ok','&nbsp;');
	elseif($p)
	  list($img,$txt) = array('i-notok',
				  "Newer Version needed.<br/>Please upgrade $pn.<br/>Minimal version required: $vrsn<br/>".
				  "<i>$channel upgrade $name</i>");
	elseif(function_exists($pn))
	  list($img,$txt) = array('ok','&nbsp;');
	else
	  list($img,$txt) = array('i-notok',
				  "Please install ".x('b',$pn).",<br />minimal version required: $vrsn<br /><i>$channel install $name</i>");
	
	if ($img == 'i-notok' && $w=='required') $allrequ = False;
	$t->prt(array('a1'=>$pn,
		      'a2'=>$installedVersion,
		      'a3'=>$vrsn,
		      'a4'=>bIcons()->get("i_$img"),
		      'a5'=>$w,
		      'a6'=>$channel,
		      'a7'=>$txt));
      }
    }
    $t->close();
    return;

    if (!$allrequ) 
      print x('h2',"Please check the required packages and install them!");
    else{
      system('touch '.envChekDoneFlag.' 2>&1');
      b_cnf::get('envChekDoneFlag',($_GET['envChekDoneFlag']='yes'));
      core_drawLink('continue',core_sameURL_but());
    }
    $this->printHeader('footer');
  }
  
  function printHeader($what='header'){
    if ($what == 'header'){
      echo "<html>";
      echo "<head>";
      echo "	<title>Pre-check installation</title>";
      echo '	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />';
      echo '	<link rel="stylesheet" type="text/css" href="install/css/phpmyadmin.css?lang=de-utf-8&amp;js_frame=right" />';
      echo '</head>';
      echo '<body>';
    }else{
      print '</body>';
      print '</html>';
      exit;
    }
  }
}

/*
* functions used by the installer
*/

function getVAR(){
  $search=array('|</?\s*SCRIPT.*?>|si','|</?\s*FRAME.*?>|si',
		'|</?\s*OBJECT.*?>|si','|</?\s*META.*?>|si',
		'|</?\s*APPLET.*?>|si','|</?\s*LINK.*?>|si',
		'|</?\s*IFRAME.*?>|si','|STYLE\s*=\s*"[^"]*"|si',
		'/[\'")]* *[oO][rR] *.*(.)(.) *= *\\2(?:--)?\\1?/');
  $replace=array('');
  $result=array();
  foreach (func_get_args() as $var) {
    if (empty($var) || is_array($var))  return;
    if (!isset($_REQUEST[$var])){
      //var doesnt exist maybe there went somthing wrong with the decoding....
      $a=preg_split('/&/',urldecode($_SERVER['QUERY_STRING']));
      if (is_array($a)){
	foreach ($a as $av){
	  $spl=preg_split('/=/',$av);
	  if ($spl[0]==$var)
	    if ($spl[0]!="" && $spl[1]!="")
	      $_REQUEST[$spl[0]]=$spl[1];
	}
      }
    }
    if (isset($_REQUEST[$var])) {
      if (is_array($_REQUEST[$var])){
	foreach ($_REQUEST[$var] as $oldkey=>$oldvar){
	  if (is_array($oldvar)){
	    foreach ($oldvar as $oldkey2=>$oldvar2){
	      if (is_array($oldvar2)){
		foreach ($oldvar2 as $oldkey3=>$oldvar3){
		  if (get_cfg_var("magic_quotes_gpc")=="1")$oldv3=stripslashes($oldvar3);
		  else    		                   $oldv3=$oldvar3;
		  $old[$oldkey][$oldkey2][$oldkey3]=preg_replace($search, $replace, $oldv3);
		}
	      }else{
		if (get_cfg_var("magic_quotes_gpc")=="1") $oldv2=stripslashes($oldvar2);
		else                                      $oldv2=$oldvar2;
		$old[$oldkey][$oldkey2]=preg_replace($search, $replace, $oldv2);
	      }
	    }
	  }else{
	    if (get_cfg_var("magic_quotes_gpc")=="1")$oldv=stripslashes($oldvar);
	    else                                   $oldv=$oldvar;
	    $old[$oldkey]=preg_replace($search, $replace, $oldv);
	  }
	}
      }else{
	if (get_cfg_var("magic_quotes_gpc")=="1")$old=stripslashes($_REQUEST[$var]);
	else				         $old=$_REQUEST[$var];
	$old=preg_replace($search, $replace, $old);
      }
    }elseif (isset($_FILES[$var])){$old=$_FILES[$var];
    }else 	                   $old=null;
    array_push($result, $old);
  }
  if (func_num_args() == 1) return $result[0];
  else                      return $result;
}

function redirectto($url){
  session_write_close();
  header("Location: $url");
  exit;
}

function sameStep(){
  return core_sameURL_but();
}

function nextStep(){
    global $installerClass;
    foreach ($installerClass->steps as $name=>$callBack) {
        if ($name == $installerClass->stepName) { $found = True; continue; }
        if ($found) return  $_SERVER['PHP_SELF'].'?stepname='.rawurldecode($name);
    }
    return $_SERVER['PHP_SELF'];
}
function prevStep(){
  global $installerClass;
$step = '';
  foreach ($installerClass->steps as $name=>$callBack) {
    if ($name == $installerClass->stepName) break;
    $step = $name;
  }
return  $_SERVER['PHP_SELF'].'?stepname='.rawurldecode($step);
}

function install_exec($exec){
  $r = trim(`which $exec`);
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/exec.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  if ($error) {
    $sigma->setVariable("error", $error);
    $link = 'sameStep';
  }else
    $link = 'nextStep';
  $sigma->setCallbackFunction('link',$link);
  return $sigma->get();
}

function choose_lang(){
  if ($lang=getVAR('language')){
    b_cnf::get('lang',$lang);
    redirectto(nextStep());
  }
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/language.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('link', 'sameStep');
  return $sigma->get();
}

function read_license(){
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/license.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('link', 'nextStep');
  return $sigma->get();
}

function db_config(){
  $cf = trim(`pwd`).'/'.DBconfigfile;
  if (file_exists($cf)) require $cf;

  $ok = True;
  foreach(array('DB_name','DB_user','DB_pass','DB_host','DB_type') as $p) 
    if ($v = $_POST[$p]) $$p = $v; else $ok = False;

  if ($ok){
    $handle= fopen($cf,'w');
    $write = fwrite($handle,join("\n",array('<?php',
					    "\$DB_user='$DB_user';",
					    "\$DB_name='$DB_name';",
					    "\$DB_pass='$DB_pass';",
					    "\$DB_host='$DB_host';",
					    "\$DB_type='$DB_type';",
					    '?>')));
    fclose ($handle);  
    if ($write)      redirectto(nextStep());
    $error = "error writing file $cf";
  }
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/dbconfig.html');
  if ($error) $sigma->setVariable("error", $error);
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('link', 'sameStep');
  foreach(array('DB_name','DB_user','DB_pass','DB_host','DB_type') as $p) $sigma->setVariable($p,$$p);
  return $sigma->get();
}

function connectDB(){
  require DBconfigfile;
  if (!myPear_db()->connect(False)){
    if (myPear_db()->error == DB_CONNECT_ERROR){
      $_SESSION['errormessage'] = nbsp("Can't connect to $DB_host:<br/>".myPear_db()->error_text);
      return False;
    }
    // the dabase does not exist, create it
      $_SESSION['errormessage'] = myPear_db()->error_text;
    return False;
  }
  return myPear_db();
}

function SQLparser($file){
  $tables = array();
  if (!file_exists($file)) return False;
  foreach (preg_split("/\n/",file_get_contents($file)) as $line) {
    if (preg_match("/CREATE TABLE (\S+) /",$line,$m)) {
      $tableName = preg_replace('/`/','',$m[1]);
      $tableData = $line;
    }elseif(preg_match("/^\)/",$line)){
#     $tableData .= $line;
      $tableData .= ')';
      $tables[$tableName] = $tableData;
      unset($tableName);
    }elseif ($tableName){
      $tableData .= $line;
    }
  }
  return $tables;
}

function create_tables(){
  require DBconfigfile;
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/tables.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('image','sigma_getimage');
  $sigma->setVariable("database","$DB_name@$DB_host");
  if ($db=connectDB()){
    // parse the sql file
    $tables = SQLparser('src/structure.sql');
    foreach ($tables as $table=>$def){
      $sigma->setCurrentBlock('tables');
      $sigma->setVariable('table',$table);
      if ($db->tableExists($table)){
	$sigma->setVariable('image','i-ok.png');
	$sigma->setVariable('comment','already exists');
      }else{
	$sigma->setVariable('comment',' ');
	$db->query($def);	// create table
	$db->reset_cache();      // otherwise the next query will fail
	if ($db->tableExists($table)){
	  $sigma->setVariable('image','i-ok.png');
	}else{
	  $sigma->setVariable('image','i-notok.png');
	  $sigma->setVariable('error','cant create');
	  $error = "can't create table '$table'";
	}
      }
      $sigma->parseCurrentBlock();
    }
    if ($error){
      $sigma->setCallbackFunction('link', 'sameStep');
      $sigma->setVariable("connecterror",$error);
    }else{
      $sigma->setCallbackFunction('link', 'nextStep');
    }
  }else{
    $sigma->setCallbackFunction('link', 'prevStep');
    $sigma->setVariable("direction",'_back');
    $sigma->setVariable("connecterror",$_SESSION['errormessage']);
  }
  return $sigma->get();
}

function register_funcs(){
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/functions.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('image', 'sigma_getimage');
  $sigma->setCallbackFunction('link', 'sameStep');
  include("registerfunctions.php");
  $functions=reg_funcs();
  return $sigma->get();
}

function add_group(){
  list($groupname,$comment)=getVAR('groupname','comment');
  if ($groupname!=""){
    addGROUP(array('cn'=>$groupname,'description'=>$comment));
    redirectto(nextStep());
  }
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/new_group.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('link', 'nextStep');
  $sigma->setCallbackFunction('image', 'sigma_getimage');
  return $sigma->get();
}

function add_user(){
  list($userid,$username,$gid,$tel,$room,$mail,$password,$searchstring)=getVAR('userid','usersname','maingroup','tel','room','mail','userpassword','searchstring');
  if ($password){
    redirectto(nextStep());
    $insert=addUSER(array('uidnumber'=>'1',
			  'uid'=>$userid,
			  'cn'=>$username,
			  'gidnumber'=>$gid,
			  'telephonenumber'=>$tel,
			  'roomnumber'=>$room,
			  'mail'=>$mail,
			  'userpassword'=>$password));
    $insert=addUSERTOGROUP(1,$gid);
  }
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/new_user.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('link', 'sameStep');
  $sigma->setCallbackFunction('image','sigma_getimage');
  /*
  $groups=getGROUPLIST();
  if ($groups!=false) {
    foreach ($groups['entries'] as $group){
      $sigma->setCurrentBlock('maingroup');
      $sigma->setVariable(array('maingroupid'=>$group['gidnumber'],'maingroup'=>$group['cn']));
      $sigma->parseCurrentBlock();
    }
  }
  */
  return $sigma->get();
}

function finalize(){
  return;
}

function end_install(){
  $sigma = new HTML_Template_Sigma('.');
  $sigma->loadTemplateFile('install/templates/end.html');
  $sigma->setCallbackFunction('lang', 'sigma_setlang');
  $sigma->setCallbackFunction('link', 'nextStep');
  system('touch '.installDoneFlag);
  b_cnf::get('installDoneFlag', ($_GET['installDoneFlag']));
  return $sigma->get();
}

// ----------------------------------------------------------------------
// Purpose of this file: API for PEAR::Sigma
// ----------------------------------------------------------------------

function getconfigVAR($arg){
  return b_cnf::get($arg);
  global $u_cfg;
  return $u_cfg[$arg];
}

//return the defined language
function sigma_setlang($arg){
  if (!defined($arg)){
    $lang=getconfigVAR("language");
#    $langdef=db_selectID("unattended_languages_defines",$arg,"langname");
    if ($langdef[$lang]!=""){
      define ($arg, $langdef[$lang]);
      return $langdef[$lang];
    }else     return $arg;
  }else       return constant($arg);
}
	
//requires
// loadsub => modulname
// type => filename
// function => functionname
function sigma_createlink($loadsub='',$type='index',$function='main',$args=''){
  return join('',array("http://",
		       $_SERVER['HTTP_HOST'],
		       $_SERVER['PHP_SELF'],
		       "?".join('&',array(session_name()."=".session_id(),
					  "loadsub=".rawurldecode($loadsub),
					  "func=".rawurldecode($function),
					  "type=".rawurldecode($type),
					  rawurldecode($args)))));
}

// returns item1 if i is not %2
function sigma_toggle(){
  static $i = 1;
  return $i++ % 2? getconfigVAR('color1'): getconfigVAR('color2');
}

function sigma_toggle2(){
  static $i = 1;
  return $i++ % 2? getconfigVAR('color3'): getconfigVAR('color4');
}

function sigma_toggle3(){
  static $i = 1;
  return $i++ % 2? getconfigVAR('color5'): getconfigVAR('color6');
}

//return the image file from the theme
function sigma_getimage($image){
  return "install/img/$image";
  $theme=getconfigVAR('theme');
  if (file_exists('themes/'.$theme.'/img/'.$image)) return 'themes/'.$theme.'/img/'.$image;
  else                                              return 'themes/'.$theme.'/img/blank.gif';
}
