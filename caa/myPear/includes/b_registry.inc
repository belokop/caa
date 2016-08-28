<?php
/*
 * myPear modules registry
 */
class b_reg{

  /*
   * Current module
   */
  public static $current_module = Null;
  public static $current_path = Null;

  /*
   * list of registered modules 
   */
  public static $modules= array();  

  /*
   * Get the currently active module (whatever it means) 
   */
  private static $_recursion = 0;
  static public function _($set_Avatar_context=True){
    if (self::$_recursion++){
      b_debug::traceBack(__METHOD__.': recursion'.self::$_recursion);
      exit;
    }else{
      if (self::$current_module === Null){
	switch (HOSTED_BY_CMS){
	case False:
	case 'CLI':
	  // Follow the drupal convention for CLI
	  $path = explode('/',(string)@$_GET['q']);
	  break;
	  
	case 'drupal':
	  // In drupal-8 this does not work... 
	  if (!get_clean_url()) $path = (string)@$_GET['q'];
	  elseif (empty($_GET['q']))          $path = b_os::pathinfo($_SERVER["SCRIPT_URL"],PATHINFO_DIRNAME); 
	  else                                $path = $_GET['q'];

	  $drupal_system = array('block','filter','menu','node','system','update','user');
	  $drupal_user   = array_diff(array_values(module_list()),$drupal_system);
	  if (in_array($path,$drupal_system)) $path = '';
	  $path = explode('/',$path);
	  break;
	  
	case 'joomla':
	case 'wp':
	default:
	  b_debug::ERROR("Unexpected HOSTED_BY_CMS='".HOSTED_BY_CMS."'. Can't get 'current_module'");
	  $path = array();
	}
	
	// Set the current module
	self::$current_module = (empty($path[0])  // || !in_array($path[0],self::get_modules())
				 ? myPear_MODULE 
				 : $path[0]); 
	self::$current_path   = $path;
	
	if ($set_Avatar_context){
	  // Set the proper default Avatar class
	  locateAndInclude('bForm_Avatar');
	  $arg = 'bForm_Avatar';
	  bForm_Avatar::set_context($arg);
	}
      }      
    }      
    self::$_recursion--;
    b_debug::xxx(x('strong','current_module="'.self::$current_module.'"'));
    return self::$current_module;
  }

  /*
   * Add module to the registry
   * - include the module in the search path
   * - initialize database
   * - etc. (to be completed...)
   *
   * @param $info
   *  'm' - module name (if a module)
   *  't' - theme name (if a theme)
   *  'v' - version number
   *  'd' - release date
   */
  public static function add_module($module,$info=array()){
    
    self::dbg(__METHOD__."($module)");

    // store module components
    self::set_module_component($module,$info);
  }

  /*
   * remove the module sql tables from the database
   */
  public static function remove_module($module){
    self::dbg();
    if (empty(self::$_SQLidentities[$module])){
      myPear::WARNING_T("module \"$module\" is not in the registry");
    }else{
      $bLists = $bUnits = $tables = array();
      foreach(self::$_SQLidentities[$module] as $class=>$descr){
	$table = self::$_SQLidentities[$module][$class]['t'];
	if(preg_match("/bList/i",$class)){
	  foreach(bList::get_all($class,$parent_ID=Null,$rank=Null) as $l_id){
	    $bLists[$l_id] = $class;
	  }
	}elseif(preg_match("/bUnit/i",$class)){
	  foreach(b_reg_bList::get($module,$class) as $rank=>$u_name){
	    foreach(bList::get_all($class,$parent_ID=Null,$rank) as $u_id){
	      $bUnits[$u_id] = $u_name;
	    }
	  }
	}elseif(preg_match("/(Avatar)/i",$class)){
	}else{
	  //	  if (myPear_db()->tableExists($table))  $tables[] = "DROP TABLE IF EXISTS $table"; 
	  $tables[] = $table; 
	}
      }

      // Check the auxiliary tables
      $path = self::module_info('path',$module);
      if (is_dir($dir="$path/sql")) {
	if ($dh = opendir($dir)) {
	  while (($file = readdir($dh)) !== false) {
	    if (strpos($file,'.sql') !== False) $tables[] = str_replace('.sql','',$file);
	  }
	  closedir($dh);
	}
      }
      
      // remove the sql tables
      foreach($bLists as $l_id=>$l_name){
	myPear::MESSAGE("removing bList \"$l_name\"");
	bList::deleteList($l_id);
      }

      foreach($bUnits as $u_id=>$u_name){
	myPear::MESSAGE("removing bUnit \"$u_name\"");
	bUnit::deleteUnit($u_id);
      }
      
      foreach(array_unique($tables) as $t){
	if (myPear_db()->tableExists($t)){
	  myPear::MESSAGE("removing table \"$t\"");
	  myPear_db()->qquery("DROP TABLE IF EXISTS $t",True);
	}
      }
    }
  }

  /*
   * Helper, process module components
   */
  public static function set_module_component($module,$info){
    if (empty(self::$modules[$module])) self::$modules[$module] = array();
    self::$modules[$module] = array_merge(self::$modules[$module],
					  $info,
					  array('m'=>$module));
    foreach($info as $k=>$v){
      switch($k){
      case 'classes': 
	// register the SQL tables used by bList's / bForm's
	b_reg::add_SQLidentity($v,$module);
	//	self::checkConsistancy($module);
	break;

      case 'ranks':
	// register bUnits
	b_reg_bUnit::register($module,$v);
	break;

      case 'c':
	// add the search path for this module
	myPear_register_config($v);
	if ($module != 'myPear'){
	  bMaster()->add_masterModule($module,$v);
	}
	
      default:
	self::$modules[$module][$k] = $v;
      }
    }
  }
  
  /*
   * Debugging tool, not really important now...
   */
  public static function checkConsistancy($module){

    $b_reg_bList = b_reg_bList::get($module);
    if (empty($b_reg_bList)){
      if (cnf_dev) b_debug::xxx("??? empty b_reg_bList for module '$module'");
    }else{
      foreach($b_reg_bList as $class=>$info){
	$l_name = $info[0];
	$l_member_title = $info[1];
	$problems = $updates = array();
	foreach(array('l_member_title','l_name') as $column){
	  if (!myPear_db()->columnExists($column,'zzz_lists')) continue;
	  $q=myPear_db()->query("SELECT l_class,$column FROM zzz_lists WHERE l_class = '$class'");
	  while($r=myPear_db()->next_record($q)){
	    if (strToLower($r[$column]) != strToLower($$column)){
	      $problems[$column] = sprintf("%s ---> %s",$r[$column],$$column);
	      $updates[]  = sprintf("%s = '%s'",$column,$$column);
	    }
	  }
	}
	if (!empty($updates)){
	  $cmd = "UPDATE zzz_lists SET ".join(',',array_unique($updates))." WHERE l_class = '$class'";
	  myPear_db()->query($cmd);
	  $problems = array_unique($problems);
	  b_debug::print_r($problems,$cmd);
	}
      }
    }
  }
  
  /*
   * Register the database tables used 
   */
  private static $_SQLidentities = array();
  public static function add_SQLidentity($definition,$module){
    locateAndInclude('bList');

    foreach($definition as $class=>$identities){
      self::dbg(__METHOD__."($class)");
      
      // Substitute name by the known identity
      if (is_string($identities)) $identities = myPear::$sql_identities[$identities];
      
      // Get the SQL table and record ID from the parent class (bList | bForm_Avatar) 
      if (empty($identities['t']) && !empty($identities['p'])){
	if (empty(myPear::$sql_identities[$identities['p']])) b_debug::traceBack("Can't match $class");
	foreach(array_diff(array_keys(myPear::$sql_identities[$identities['p']]),
			   array_keys($identities)) as $k){
	  $identities[$k] = myPear::$sql_identities[$identities['p']][$k];
	}
      }

      // Create the table if it does not exist
      self::_createIfNotExists($identities['t'],$module);    

      // Register the SQL tables & record_IDs.
      if (!empty($identities['i'])){      
	// Register the EA objects both in EA_MODULE & myPear_MODULE
	// since EA functionality (except API) is "built-in" in myPear 
	// and might be used by other modules without installing EA_MODULE 
	$modules = ($module == EA_MODULE
		    ? array($module,myPear_MODULE)
		    : array($module));
	// Register bList & bUnit classes
	foreach($modules as $m){
	  b_reg_bList::register($m,$class,$identities);
	}    
      }    
      
      // Report if something went wrong
      if (!($ok = (!myPear_db()->isReady() ||  
		   (myPear_db()->isReady() && myPear_db()->tableExists($identities['t']))))){
	myPear::WARNING("Can't start ".strToUpper($module).": SQL table '$identities[t]' does not exist");
      }
    }
  }

  /*
   * Helper. To Be Done Better...
   */
  static function _fill_SQL_ids($identities,$module,$class,$myPear_also=False){
    if (myPear_db()->isReady()){
      $modules = (($module == EA_MODULE) && $myPear_also
		  ? array($module,myPear_MODULE)
		  : array($module));
      foreach($modules as $m){
	self::$_SQLidentities[$m][$class] = array('t'=>$identities['t'],
						  'i'=>$identities['i']);
	$identities['m'] = $m;
	foreach($identities as $k=>$v) myPear::$sql_identities[$class][$k] = $v;
      }
    }
  }

  
  /*
   * Helper
   */
  private static function _createIfNotExists($table,$module){
    // Careful, the module might be in the installation stage
    if (!cnf_CLI && myPear_db()->isReady()){
      // Create the sql table if it not yet done
      // The sql files (one per table) are stored in the <module>/sql/ directory.
      if (!myPear_db()->tableExists($table)){
	$path = self::module_info('path',$module);
	if (file_exists($table_sql = "$path/sql/$table.sql")){
	  $sql = trim(file_get_contents($table_sql));
	  if (empty($sql)) b_debug::internalError("SQL definition file \"$table_sql\" empty");
	  myPear::MESSAGE(sprintf('%s - creating SQL table `%s` on %s@%s',strToUpper($module),$table,myPear_db()->Database,myPear_db()->Host));
	  foreach(explode(';',$sql) as $query) if (!empty($query)) myPear_db()->qquery($query,cnf_dev);
	  myPear_db()->reset_cache();
	}else{
	  myPear::ERROR("The expected file \"$table_sql\" does not exist");
	}
      }
    }
  }    

  /*
   * Get the module information block
   */
  public static function get($module=Null){
    if (empty($module)) $module = b_reg::$current_module;
    $reply = @self::$modules[$module];
    if (empty($reply))  $reply = array();
    return $reply;
  }
  
  /*
   * Get back the SQL table & record_ID for the given class/module
   */
  public static function sql_ids($class,$module=myPear_MODULE){
    $reply = array();
    if (myPear_db()->isReady()){
      if (True){
	// sql identities DO NOT DEPEND ON THE MODULE !!!
	// Get the first one which is not emty...
	foreach(self::$_SQLidentities as $m=>$ids){
	  if (!myPear::is_empty($t=@$ids[$class]['t']) &&
	      !myPear::is_empty($i=@$ids[$class]['i'])) return array('t'=>$t,'i'=>$i);
	}
      }else{
	if (empty(self::$_SQLidentities[$module][$class]['t'])){
	  b_debug::traceBack("self::\$_SQLidentities[$module][$class][t] empty for ".__FUNCTION__."$class,$module");
	}else{
	  $reply = array('t'=>self::$_SQLidentities[$module][$class]['t'],
			 'i'=>self::$_SQLidentities[$module][$class]['i']);
	}
      }
    }
    return $reply;
  }

  /*
   * Load the module configuration file if not yet done.
   * I would assume that the modules loading should be guaranteed by the CMS, but (at least for Drupal) this is not the case.
   */
  public static function load_module($module=Null){
    static $already_done = array();
    if (!empty($module) && ($config = bMaster()->get_masterModule($module))){
      if (!@$already_done[$module]) require_once $config; 
      $already_done[$module] = True;
    }else{
      foreach (self::get_modules(True) as $m=>$config){
	if (!empty($module) && (($m != $module) || class_exists($module,False))){
	  b_debug::xxx('ok');
	  continue;
	}
	if (!@$already_done[$m] && file_exists($config)){ 
	  self::dbg($config);
	  require_once $config; 
	  $already_done[$m] = True;
	}
      }
    }
  }

  /*
   * Return the registered modules list
   */
  private static $_registered_modules = Null;
  public static function get_modules($include_config=False){
    if (!function_exists('myPear_db') || !myPear_db()->isReady()) return array();
    
    // read the list of registered modules from the database
    if (self::$_registered_modules === Null){
      self::$_registered_modules = array();
      foreach (bMaster()->get_masterModules() as $m){
	self::$_registered_modules[$m] = bMaster()->get_masterModule($m);
      }
    }

    if($include_config) return self::$_registered_modules;
    else                return array_keys(self::$_registered_modules);
  }

  /**
   * Get information about the currently active module
   */
  public static function module_info($what=Null,$module=Null){
    static $P = 'PATH=/opt/CollabNet_Subversion/bin/:$PATH ; ';

    if (is_null($module)) $module = b_cms::_(0);
    $info  = @self::$modules[$module];

    switch($what){
      
    case 'c2m': // class -> module
      return @myPear::$sql_identities[$module]['m'];
      break;

    case 'path':
      // path to the module root directory
      switch (HOSTED_BY_CMS){
      case 'drupal': 
	return  b_os::url2path(my_drupal_get_path('module',$info['m']));
      default: 
	return dirname($info['c']);
      }
      
    case 'version':
      $path = self::module_info('path',$module);

      // Get the UID for the repository and save in SESSION.
      // This is not changed, at least during the session
      $uid = trim(`$P svn info $path/ | awk '/^Repository UUID:/ {print \$NF}'`);
      if (empty($_SESSION['svn_version'][$uid])){
	$_SESSION['svn_version'][$uid] = strtr(trim(`$P svn info $path/.@HEAD | awk '/^Last Changed Rev:/ {print \$NF}'`),'M','');
      }

      $c_version = strtr(trim(`$P svnversion -n $path`),'M','');
      return array($path,$c_version,$_SESSION['svn_version'][$uid]);
      
    case 'signature':
    default:
      $reply = array();
      $reply[] = strToUpper($info['m']).'&nbsp;'.$info['v'];
      if ($d=$info['d'])  $reply[] = $d;
      return join('<br/>',$reply);
    }
  }

  private static function dbg($text=' ',$force=False){
    // b_debug::xxx($text,array(3,'oliveText',(bool)(cnf_dev||cnf_show_reg)));
    if (cnf_show_reg || $force) myPear::DEBUG($text);
  }
}


/*
 * Registry of the bList / bUnit classes
 */
class b_reg_bList{

  private static $_bLists = array();
  
  /*
   * Add API bList classes to the registry.
   * The registry will help us to find the proper class from the database record
   */
  public static function register($module, $class, $identities){

    if (cnf_show_reg) myPear::DEBUG(__METHOD__."($module, $class)");
    if (!empty($class)){
      // Store the SQL ids
      b_reg::_fill_SQL_ids($identities,$module,$class);
      
      if (!empty($identities['d'])){
	$is_bList = (strpos($class,'bList') !== False);
	$is_bUnit = (strpos($class,'bUnit') !== False) && !empty($identities['rank']);
	
	if ($is_bList){
	  if (empty($identities['dd'])) $identities['dd'] = preg_replace("/s$/","",$identities['d']);
	  self::$_bLists[$module][$class][0] = $identities['d'];
	  self::$_bLists[$module][$class][1] = $identities['dd'];
	}elseif($is_bUnit){
	  self::$_bLists[$module][$class][$identities['rank']] = $identities['d'];
	}elseif(cnf_dev){
	  b_debug::var_dump($identities,'??');
	  b_debug::traceBack('??? no identities');
	}
      }
    }
  }
  
  /*
   * Get list of the registered bLists
   */
  public static function get($module=Null,$list=Null,$what=Null){
    if    ($module===Null) return @self::$_bLists;
    elseif($list  ===Null) return @self::$_bLists[$module];
    elseif($what  ===Null) return @self::$_bLists[$module][$list];
    else                   return @self::$_bLists[$module][$list][$what];
  }

  /*
   * Get the bList description back from the registry.
   * Update the list columns 'l_member_title'
   */
  public static function set__l_name__l_member_title($bList,$keys){

    $list_name = get_class($bList);
    $isbUnit = (strpos($list_name,'bUnit') !== False);
    
    $current_module = b_reg::$current_module;
    if (!empty($current_module) && !empty(self::$_bLists[b_reg::$current_module])){
      
      if(in_array($list_name,array_keys(self::$_bLists[myPear_MODULE])) ||
	 in_array($list_name,array_keys(self::$_bLists[b_reg::$current_module]))){

	// However, we might be in the installation pass, check it
	if (b_reg::$current_module == myPear_MODULE){
	  foreach(self::$_bLists as $module=>$classes){
	    if (in_array($list_name,array_keys($classes))){
	      myPear::WARNING_T("$list_name found in module '$module', current_module='".b_reg::$current_module."'");
	      $current_module = $module;
	    }
	  }
	}
	
	// Check the registry for the current module & myPear
	foreach(array($current_module,myPear_MODULE) as $m){
	  $l_name         = ($isbUnit
			     ? @self::$_bLists[$m][$list_name][$bList->rank]
			     : @self::$_bLists[$m][$list_name][0]);
	  $l_member_title = ($isbUnit
			     ? 'member' 
			     : @self::$_bLists[$m][$list_name][1]);
	  if (empty($l_name))        $l_name         = 'name ???';
	  if (empty($l_member_title))$l_member_title = 'member ???';
	  return array($l_name,$l_member_title);
	}
      }else{
	if(cnf_dev){myPear::WARNING("$list_name is not in the registry !");
	  myPear::WARNING("$list_name is not in the registry !!");
	  myPear::WARNING("$list_name is not in the registry !!!");
	  b_debug::xxx("???? Где $list_name");
	  b_debug::traceBack("$list_name is not in the registry !!!!");
	}
      }
    }
    
    // The bList / bUnit is unknown to the registry,
    // which might happen when the unit is invoked during the start-up by the auto-login.
    // Inform myPear and the developer that the class is not properly initialized
    myPear::$__construct_notComplited[] = get_class($bList);
    $msg = "no registry information (yet?) for ".$bList->__toString();
    myPear::WARNING_T($msg);
    // b_debug::traceBack($msg);
    return array('???','???');
  }
}

/*
 * Registry of roles, played by avatars.
 */
class b_reg_bUnit{
  /*
   * Add roles to the registry.
   *
   * The role might have an associated bUnit class (see configuration file config.inc).
   * Fill then the SQL registry - we know the bUnit class name from the roles description
   */
  public static $list = array();
  public static $_bUnits = array();
  public static function register($module,&$ranks){

    // Register the SQL tables & record_IDs.
    // Since EA module is "built-in" in myPear, register EA objects both in EA_MODULE & myPear_MODULE
    $modules = ($module == EA_MODULE
		? array($module,myPear_MODULE)
    		: array($module));
    foreach($modules as $m){
      foreach($ranks as $rank=>$descr){
	self::$_bUnits[$m][$rank] = $descr;
	b_reg_bList::register($m,
			      @$descr['class'],
			      array_merge(array('rank'=>$rank,
						'd'   =>$descr['d'].'s'),
					  b_reg::sql_ids('bUnit',myPear_MODULE)));
      }
    }
  }
  
  /*
   * Get the role(s) description
   */
  public static function get_description($rank=Null,$module=Null){

    if     (empty($rank) && empty($module)){
      $reply = self::$_bUnits;
    }elseif(empty($rank)){
      foreach(array($module,myPear_MODULE) as $m) if ($reply = @self::$_bUnits[$m]) break;
    }else{
      foreach(array($module,myPear_MODULE) as $m) if ($reply = @self::$_bUnits[$m][$rank]) break;
    }
    return $reply;
  }

  /*
   * Get the title corresponding to the rank
   */
  public static function get_title($rank=Null,$module=Null){
    if (empty($module)) $module = b_reg::$current_module;
    if (empty($rank))   $rank   = myPear_access($module)->getRank();
    $d = self::get_description($rank,$module);
    return @$d['d'];
  }

  /*
   * Get the fav-icon corresponding to the rank
   */
  public static function get_icon($rank=Null,$module=Null){
    if (empty($module)) $module = b_reg::$current_module;
    $d = self::get_description($rank,$module);
    if ($d) return bIcons()->get($d);
    else    return '';
  }
}
