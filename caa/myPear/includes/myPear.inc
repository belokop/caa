<?php
/**
 * @file
 * Implementation of the class loading & instantiation, timing, class caching, 
 * and other basic basic functions.
 */
class myPear{

  public static  $isReady = False;
  public  static $sql_identities = array();
  public  static $sql_identities_extra = array();

  // myPear classes and the DB tables they use
  // - t table name in the myPear database
  // - i the primary ID column name, ID is unique in the database  
  // - d description
  // - p shortcut for t+i
  private $identities= array(myPear_MODULE=>array('bTptl'                =>array('t'=>'zzz_templates',   
										 'i'=>'tpl_id'),
						  'bForm_Avatar'         =>array('t'=>'zzz_avatars',
										 'i'=>'av_id'),
						  'bForm_BugReport'      =>array('t'=>'zzz_bugs', 
										 'i'=>'bug_id'),
						  'bForm_Organization'   =>array('t'=>'zzz_organizations',
										 'i'=>'org_id'),
						  'bUnit'                =>array('t'=>'zzz_units',  
										 'i'=>'u_id'),
						  'bList'                =>array('t'=>'zzz_lists',     
										 'i'=>'l_id'),
						  'list_members'         =>array('t'=>'zzz_list_members',     
										 'i'=>'lm_id'),
						  'unit_members'         =>array('t'=>'zzz_unit_members',     
										 'i'=>'um_id'),
						  'bList_orgGroups'      =>array('p'=>'bList',
										 'd'=>'Groups'),
						  'bList_orgEmpTypes'      =>array('p'=>'bList',
										 'd'=>'Employee Types'),
						  'bList_formDescription'=>array('p'=>'bList',
										 'd'=>'Forms descriptors'),
						  'bForm_Avatar_Search'  =>array('p'=>'bForm_Avatar'),
						  'zzz_variables'        =>array('t'=>'zzz_variables'),
						  'zzz_countries'        =>array('t'=>'zzz_countries'),
						  'zzz_crypt'            =>array('t'=>'zzz_crypt'),
						  'zzz_logs'             =>array('t'=>'zzz_logs'),
						  ),
			     EA_MODULE   => array('bForm_Avatar_ea'      =>array('p'=>'bForm_Avatar'),
						  'bForm_ea_Office'      =>array('t'=>'abs_offices',
										 'i'=>'o_id'),
						  'bList_eaCV'           =>array('p'=>'bList',
										 'd'=>'CVs'),
						  'bList_eaContacts'     =>array('p'=>'bList',
										 'd'=>'Contact Persons for emergency cases',
										 'dd'=>'Contact Person',),
						  'bList_eaEmpRecords'   =>array('p'=>'bList',
										 'd'=>'Employment history',   // list name    ('l_name' column)
										 'dd'=>'Employment record'),  // member title ('l_member_title' column)
						  ));
  
  public static $debug  = False;
  public static $__construct_notComplited = array();
  
  public function __construct(){

    if (self::$isReady) b_debug::internalError("double invocation of 'myPear'");
    
    // Register the myPear module
    b_reg::add_module(myPear_MODULE,array('d' =>'', // the module description is empty, hence it will not be shown by default on the web
					  'tm'=>myPear_MODULE,
					  'v' =>myPear_VERSION,
					  'r' =>myPear_DATE,
					  'i' =>'i-anker',
					  'c' =>dirname(__FILE__).'/../config.inc'));
    
    // Register common bForm's and bList's
    foreach($this->identities as $module=>$descr){
      b_reg::add_SQLidentity($descr,$module);
    }
  }
  
  public static function myPear_enabled_theme(){
    static $reply = Null;
    static $theme_default = '?';
    if (($reply === Null) && function_exists('drupal_get_path')){
      $theme_default = variable_get('theme_default', NULL);
      if (!empty($theme_default)){
	$theme_path = drupal_get_path('theme', ($theme_default=variable_get('theme_default', NULL)));
	if     (file_exists($c="$theme_path/config.inc")) require_once $c;
	elseif (file_exists($c="$theme_path/config.inc")) require_once $c;
	}
      $reply = defined('myPear_enabled_theme');
      if (cnf_show_acl) b_debug::xxx(($reply ? 'YES' : 'NO').", theme_default=$theme_default");
    }
    return $reply;
  }
  
  /*
   * Has av_id a duty in the organization?
   */
  public static function org_duty($av_id,$ranks,$parent_ID=myOrg_ID){

    $duty = RANK__anonymous;
    if (in_array(RANK__superuser,$ranks) && myPear::_UNIT(RANK__superuser)->isMember($av_id)){
      $duty = RANK__superuser;
    }else{
      // Get the list of "organization-level" bUnits where the user is registered
      $units_org = array();
      foreach(array_values(bUnit::get_unitsMembership($parent_ID,Null,$av_id)) as $recs){
	foreach($recs as $r)  $units_org[$r['u_rank']][] = $r['u_parent'];
      }

      foreach($ranks as $r){
	if(isset($units_org[$r])) $duty = max($duty,$r);
      }
    }
    return $duty;
  }

  /*
   * (string)'0' & (int)0 are not empty value for this function
   */
  public static function is_empty($value){
    $reply = empty($value) && ($value !== 0) && ($value !== '0');
    return $reply;
  }

  /*
   * 0 is not int for this function
   */
  public static function is_int($value){
    $reply = !empty($value) && (is_int($value) || is_numeric($value));
    return $reply;
  }


  /*
   * Compare two values (start '===')
   */
  public static function is_equal($v1,$v2){
    if (is_string($v1)) $v1 = trim($v1);
    if (is_string($v2)) $v2 = trim($v2);
    $reply = ($v1 === $v2);
    if (($v1 !== Null) && ($v2!==Null)){
      if (!$reply && (is_numeric($v1) || is_numeric($v2))) $reply = ((string)$v1 === (string)$v2);
      if (!$reply) $reply = ($v1 == $v2);
    }
    return $reply;
  }

  /*
   *
   */
  public static function H1($text, $mode=array()){
    static $dejaVu = array();
    static $title = Null;

    b_debug::xxx();
    if ($text == 'get')   return $title;
    
    if (!empty($text) && ($text != b_reg::$current_module)){
      if (!is_array($mode)) $mode = array($mode);
      if ($text=='empty')   $mode[] = 'noTranslate';
      
      if (!empty($text)){
	if ($text=='empty')                    $text='';
	elseif(!in_array('noTranslate',$mode)) $text = bText::_($text);

	if (is_null($title) || in_array('reset', $mode)){
	  self::setPageHeader($title = trim(b_fmt::unEscape($text)));
	}
      }
    }
    return '';
  }

  /*
   *
   */
  static function set_main_selector($input,$noPrint=False){
    static $n = 0;
    
    $li = array('');
    $enctype = (1 ? 'application/x-www-form-urlencoded' : 'multipart/form-data');
    foreach($input as $label=>$selection){
      if (!is_numeric($label)) $li[] = x("li role='label'",$label);
      $li[] = x("li",
		(preg_match('/<(select|input)/i',$selection)
		 ? x(sprintf("form action='%s' method='POST' enctype='$enctype' class='only_online' name='%s%d' accept-charset='UTF-8'",
			     b_url::same(),'selector',++$n),
		     $selection)
		 : $selection));
    }
    $reply = (x("div id='mypear_ul_selector'",x('ul',join("\n",$li))).
	      x("div style='clear:both'",''));
    if ($noPrint)   return $reply;
    else            print  $reply;
  }
      
  /*
   *
   */
  static function setPageHeader($header,$strip_tags=True){
    global $myPear_custom_title_striped,
           $myPear_custom_title;

    b_cache_file::CMS(__METHOD__,array($header));
    $myPear_custom_title_striped = $header;

    if (function_exists('drupal_set_title')) { 
      // Drupal does not let html tags to be in the header
      if (($header_striped = strip_tags($header)) != $header){
	$myPear_custom_title = $header;
	$myPear_custom_title_striped = $header_striped;
	drupal_set_title($header_striped);
      }else{
	drupal_set_title($header);
      }
    }else{
      $header = "<h1>$header</h1>\n"; 
    }
    b_debug::xxx($header);
    return $header;
  }  

  static function H2($text,$noTranslate=False)  { 
    static $dejaVu = array();
    $reply = (empty($text) || @$dejaVu[mb_strToLower($text)]++ || (mb_strToLower($text) == mb_strToLower(self::H1('get')))
	      ? ''
	      : '<h2>'.b_fmt::escape($noTranslate?$text:bText::_($text)).'</h2>'); 
    b_debug::xxx($reply);
    return $reply;
  }
  
  static function H3($text,$noTranslate=False)  { 
    return '<h3>'.b_fmt::escape($noTranslate?$text:bText::_($text)).'</h3>'; 
  }
  
  function __toString()      { return get_class($this); }
  
  function noSuccess($txt=NO_ACCESS,$icon='32-noRight',$class='error',$noprint=False){ 
    if (empty($icon))  $icon = '32-noRight';
    if (empty($class)) $class= 'error';
    $msg = x("div class='$class'",
	     x('table style="border-collapse:separate"',
	       x('tr',
		 x('td class="align_top"',
		   bIcons()->get($icon)).x('td',$txt)))); 
    if ($noprint) return $msg;
    else          print  $msg;
  }
  
  function _()               { return $this; }

  function _NYI()            { print bIcons()->get('sorry').MSG_NYI; }
    
  function warn($text)       { self::WARNING($text,$this); }
  
  static public function authenticated() { if (bAuth::$isReady) return bAuth()->authenticated(); }

  static public function _GET_and_unset($name) { static $tmp; $tmp = @$_GET[$name]; b_cnf::_unset($name); return $tmp; }
        
  static public function _UNIT($rank,$module=myPear_MODULE,$parent=Null) { 
    if (empty($parent)){
      myOrg();
      $parent = myOrg_ID;
    }
    $d = b_reg_bUnit::get_description($rank,$module); 
    return self::_fromCache($d['class'],$parent,$rank);
  }

  /*
   * Initialise myPear database & myPear Mailer
   */
  static public function _SETUP($what=Null,$msg=''){ 
    static $setup = Null;
    if ($setup === Null){
      locateAndInclude('myPear_Setup');
      $setup = new myPear_Setup();
    }
    $setup->what = $what; 
    $setup->msg  = $msg; 
    $setup->modalDialog = True;
    $setup->show_dialog();
    return $setup;
  }

  /*
   *
   */
  static public function _MENU(){
    return self::_fromCache('APImenu'); 
  } 

  /**
   * Get the object for class(id) from the cache.
   * Instantiate a new one if the cache is empty (see '$mode' below).
   *
   * @param $class
   *   Class name to instantiate 
   * @param $args
   *   Class arguments
   * @param $mode
   *   Additional optional array of attributes. Recognized attributes:
   *     'RO' or 'RW'- access mode for the class instantiation  
   *     'strict'    - do not create an instance if the ID is unknown to the database 
   *                   (otherwise a 'new' class is instantiated)
   *     'fatal'     - like 'strict', but an internal error is generated if the instance is unknown
   *     'nocache'   - do not save the object in the cache
   *     'array'     - the output is expected as an array of classes, i.e. the instance might be not unique
   *     'clean'     - remove instance from the cache and return NULL
   *     'delete'    - remove instance from the cache AND from the database
   *     ''(default) 
   * @param $helper  - optional array($table,$id), needed when module is not yet ready...
   * @return
   *   Reference to the class object
   */
  public static function getInstance_new($class,$args,$mode=array(),$helper=array()){

    // Escape the bogus invocation    
    if (!myPear_db()->isReady()) return Null;

    // Sanity...
    if (!is_array($mode))      $mode = array($mode);
    if (empty($args)){
      if (in_array('fatal', $mode)) b_debug::internalError("??? Empty args, can't find $class($args)");
      return Null;
    }

    if (myPear::is_int($args)) $args = (int)$args;
    if(!in_array('nocache',$mode)) bCount()->add(__FUNCTION__,$class);

    // Optionally get the class name from class_ID
    if (empty($class)){
      $class = self::getClassFromID($args,False,True);
      b_debug::xxx("getClassFromID($args) - $class",'I');
      bCount()->add(__FUNCTION__,'getClassFromID');
    }

    // Workaround...
    //    $class = str_replace('bForm_Avatar_myPear','bForm_Avatar_ea',$class);

    // Keep the instances in a local cache
    static $reply = array();
    $hash = b_crypt::hash($class.serialize($args));
    
    // Save memory, remove instance from the cache 
    if (in_array('clean',$mode)){ 
      unset($reply[$hash]);
      if ($class === 'all') $reply = array();
      return Null;
    }elseif(in_array('delete',$mode)){
      unset($reply[$hash]);
    }
    
    // Get the instance if it is not cached yet
    if (empty($reply[$hash])){
      locateAndInclude($class);
      list($table,$id) = (empty($helper)
			  ? array_values(b_reg::sql_ids($class,b_reg::$current_module))
			  : $helper);

      // Parse class arguments, it might be class_ID, av_email or query
      if (is_numeric($args))            $args = array($id=>$args);
      elseif ($m=b_fmt::RFC_2822($args))$args = array('av_email'=>x("'",array_shift($m)));
      if (is_array($args)){
	foreach($args as $k=>$v) $where[] = myPear_db()->quote($v,True,$k);
      }else{
	$where = array($args);
      }

      // Query the database
      $where_str = join(' AND ',$where);
      if (empty($table) || empty($id)){	// The module is not yet initialised
	$qquery = sprintf("can't locate table for %s::%s",b_reg::$current_module,$class);
	$num_rows = 0;
      }else{
	$q = myPear_db()->qquery($qquery="SELECT $id FROM $table WHERE $where_str",$verbose=cnf_show_i);
	$num_rows = myPear_db()->num_rows($q);
      }

      // Analasise the query result
      switch($num_rows){
      case 0:
	if ( in_array('fatal', $mode)) b_debug::internalError("??? Can't find $class($where_str) q=$qquery");
	if ( in_array('delete',$mode)) return;
	if (!in_array('strict',$mode)){
	  $instance = new $class('empty');
	  b_debug::xxx('creating '.$instance,'I');
	  $isVITAL = $instance->isVITAL;
	  $instance->isVITAL = array();
	  $instance->updateDB($args);
	  $instance->isVITAL = $isVITAL;
	  if (in_array('nocache',$mode)){
	    b_debug::xxx($instance,'I');
	    return $instance;
	  }
	  $reply[$hash] = $instance;
	}
	break;

      case 1:
	if (in_array('delete',$mode)){
	  myPear_db()->qquery("DELETE FROM $table WHERE ".($id=join(' AND ',$where)),cnf_dev||cnf_show_i);
	  myPear::MESSAGE("Removing class $class($id)");
	}else{
	  while ($r = myPear_db()->next_record($q)){
	    $instance = new $class($r[$id]); 
	    if (in_array('nocache',$mode)){
	      b_debug::xxx($instance,'I');
	      return $instance;
	    }
	    $reply[$hash] = $instance;
	  }
	}
	break;

      default:
	b_debug::internalError("$num_rows answers for $qquery");
      }
    }else{
      bCount()->add(__FUNCTION__,"$class <span class='greenText'>inCache</span>");
    }
    b_debug::xxx(@$reply[$hash],'I');
    return @$reply[$hash];
  }

  /**
   * Get the object for class(id) from the cache.
   * Instantiate a new one if the cache is empty (see '$mode' below).
   *
   * @param $class
   *   Class name to instantiate 
   * @param $id
   *   Class ID
   * @param $mode
   *   Additional optional array of attributes. Recognized attributes:
   *     'RO' or 'RW'- access mode for the class instantiation  
   *     'strict'    - do not create an instance if the ID is unknown to the database 
   *                   (otherwise a 'new' class is instantiated)
   *     'fatal'     - generate internal error if the class/id is unknown
   *     'nocache'   - do not save the object in the cache
   *     'array'     - the output is expected as an array of classes
   *     ''(default) 
   * @return
   *   Reference to the class object
   */
  public static function getInstance($class,$id,$mode=array()){

    // Escape the bogus invocation
    if (!myPear_db()->isReady() || empty($id)) return Null;
    
    // Convert argument to array
    if (!is_array($mode))         $mode = array($mode);
    if ( in_array('array',$mode)) $mode[] = 'nocache';
    if (!in_array('RW',   $mode)) $mode[] = 'RO';
    $mode = array_unique($mode);
    
    // Keep the Avatar class as the current module wants
    bForm_Avatar::set_context($class,in_array('basic_avatar',$mode));
    
    // Get the class name
    if ($class_toBeCreated=in_array($id,array('new','empty'))){
      // The instance does not exists yet
      $class_name = $class;
      $class_IDs  = array($id);
      $mode[] = 'RW';
      $mode = array_diff($mode,array('RO'));

    }else{

      // The instance might already exist, check it
      $class_name = self::isIdentified($id,(bool)(stripos($class,'Avatar') !== False),$class);
      $class_IDs  = self::$isIdentified_ID;
      if ($class === 'x') $class = $class_name;
      if (empty($class_name))        return Null;

      // This is not normal
      if (($n=count($class_IDs)) > 1) 	b_debug::traceBack("$n records found");
      
      // Sanity, check that the class name is what is is expected to be
      if (!empty($class_name) && (stripos($class_name,$class)===False) && (stripos($class,$class_name)===False)){
	if (cnf_dev){
	  $msg = __METHOD__."($class,$id) getClassFromID($id) == '$class_name'";
	  if (in_array('fatal',$mode)) b_debug::internalError($msg);
	  else                         b_debug::traceBack($msg);
	}
      }
      
      // Remove the instance from the cache (save memory) if needed
      if (in_array('clean',$mode)){ 
	$cache = new b_cache(__METHOD__);
	if ($cache->wasSet($class_name,$id)){
	  $o = $cache->get();
	  b_debug::xxx((method_exists($o,'name')?$o->name():''),array(3,'greenText',(bool)(cnf_show_i||cnf_debug),$o));
	  bCount()->add(__FUNCTION__,"$class_name <span class='greenText'>removed</span>");
	  $cache->remove($class_name,$id);
	}
	return Null;
      }
    }
    
    //
    // Need the form instance, find or create it
    //
    bTiming()->cpu(__FUNCTION__); // ."($class_name)");
    if(!in_array('nocache',$mode)) bCount()->add(__FUNCTION__,$class_name);
    
    // Check the cache and get the instance from the cache if avalaible,
    // otherwise search the database
    locateAndInclude('b_cache');
    $cache = new b_cache(__METHOD__);
    $cacheReady = ($class_toBeCreated
		   ? False
		   : $cache->wasSet($class_name,$id));
    if ($cacheReady){  
	
      bCount()->add(__FUNCTION__,"$class_name <span class='greenText'>inCache</span>");
      $reply = $cache->get();
      if (!($reply instanceof $class_name)) b_debug::internalError(__METHOD__." got wrong class from the cache: $reply instead of $class_name($id)");
      
    }else{
      
      // Fix the e-mail on the fly
      if ($m=b_fmt::RFC_2822($id)){
	$id = array("av_email = '".array_shift($m)."'");
	foreach($m as $email) $id[] = "av_email2 REGEXP '$email'";
	$id = join(' OR ',$id);
      }
      
      // Get new instances
      $reply = array();
      locateAndInclude($class_name,in_array('fatal',$mode));
      foreach($class_IDs as $r_id){ 
	$c = new $class_name($r_id,(in_array('RW',$mode) ? 'RW' : 'RO'));
	// Initialise the cache for the newly created class
	if ($class_toBeCreated) $cache->wasSet($class_name,$c->ID);
	$reply[] = $c;
      }   
    }
    
    // Re-pack the output according to the request "expected as an array"
    // (see avatar_merge)
    $str = array();
    if (empty($reply))       $str[] = '?not found';
    if (in_array('array',$mode)){
      if (empty($reply))     $reply = array();
      if (is_array($reply))  $reply = array_unique($reply);
      else                   $reply = array($reply);
      foreach($reply as $o)  $str[] = $o->__toString(); 
    }else{
      if (is_array($reply))  $reply = @$reply[0];
      if ($o=$reply)         $str[] = $o->__toString(); 
    }
    
    if(empty($reply) && in_array('fatal',$mode)){
      b_debug::internalError(__METHOD__." can't get instance $class_name(id='$id')");
    }
    if (is_object($reply) && !in_array('nocache',$mode))      $cache->set($reply);
    bTiming()->cpu();
    
    b_debug::xxx(join(' ',$str),'I');
    return $reply;
  }
  

  /*
   * Get the bForm class name from the record_ID.
   * All the records in the database(s) of bForm/myPear classes have unique ID (see 'bMasterClass' class),
   * so it is feasible.
   */ 
  public static $_getClassFromID = array();
  public static function getClassFromID($id,$noFormPrefix=False,$current_module_only=False) {
    $reply = Null;
    if ($id instanceof bForm){
      if    ($noFormPrefix==='class') $reply = $id;
      elseif($noFormPrefix)           $reply = str_replace(array('form','bForm_'),'',get_class($id));
      else                            $reply = get_class($id);
    }elseif(myPear::is_int($id)){
      if ($reply = self::isIdentified($id,$current_module_only)){
	self::$_getClassFromID = myPear::$sql_identities[$reply];
	if    ($noFormPrefix==='class') $reply = self::getInstance_new($reply,$id,'strict');
	elseif($noFormPrefix)           $reply = str_replace(array('form','bForm_'),'',$reply);
      }
    }
    return $reply;
  }
  
  /*
   *
   */  
  public static function replaceInstance($class){
    $cache = new b_cache('myPear::instance');
    if ($cache->wasSet(get_class($class),$class->ID)){
      $cache->remove(get_class($class),$class->ID);
    }
    if ($cache->wasSet(get_class($class),$class->ID)){
      b_debug::internalError('something wrong');
    }else{
      $cache->set($class);
    }
  }
  
  /*
   * Get back the widget HTML code
   *
   * @param - the query string
   */
  public static function getWidget(){
    if ($g=$_GET['holder']){
      
      locateAndInclude($h = "bHolder_$g");
      $h = new $h(($arg=@$_GET['id']) ? $arg : @$_GET['mode']);
      $w = $h->getWidget();

    }elseif ($class=$_GET['form']){
      
      if (strpos($class,'bForm') === False) $class = "bForm_$class";
      $f = myPear::getInstance_new($class,$_GET['id'],'fatal');
      $w = $f->getWidget();
      if (cnf_dev) b_debug::xxx("expect class '$class', got ".$f->__toString());
    }elseif ($f=$_GET['file_once']){
      
      ob_start();
      include $f;
      $reply = ob_get_contents();
      ob_end_clean();
      $w = x('center',b_fmt::compact_html($reply));

    }else{
      $w = $_SERVER['REQUEST_URI'];
    }
    return $w;
    return "<div id='myPear_widget'>$w</div>";
  }

  public static function setIdentity($id) {
    foreach($id as $class=>$i) myPear::$sql_identities[$class] = $i;
  }
  
  /*
   * Check that the class instance has the corresponding record in the database. 
   */
  public static $isIdentified_ID = 0;
  public static $isIdentified_debug = False;
  static function isIdentified($id,$current_module_only=False,$class_name=''){
    static $specific_classes = array('l_class','u_class');
    static $cache = array();
    
    // Since this function is called very often, the answer is cached 
    if (is_object($id)){
      // Exotic call with the argument as an object
      $cache[$id->ID] = array('ID'       => array($id->ID),
			      'generic'  => get_class($id),
			      b_cms::_(0)=> get_class($id));
      $id = $id->ID;
    }elseif(is_array($id)){
      // Exotic call with the argument as an array
      if (cnf_dev) b_debug::var_dump($id,b_debug::__());
      return Null;
    }

    if (empty($id))      return False;
    if ($id === 'new')   return True;
    if ($id === 'empty') return True;
    if (stristr($id,'missing')!==False){
      if (cnf_dev) b_debug::traceBack('Broken invocation '.b_debug::__());
      return False;
    }

    // Normal call
    bCount()->add(__FUNCTION__,($id_count = (myPear::is_int($id) ? 'this->ID' : preg_replace('/( OR ).*/','',$id))));
    if (isset($cache[$id])){
      bCount()->add(__FUNCTION__,"$id_count <span class='greenText'>inCache</span>");
    }else{
      bTiming()->cpu(__METHOD__);
      // Scan the database for this record
      foreach(myPear::$sql_identities as $_class=>$_identities){
	// b_debug::xxx("$_class - ".b_fmt::joinX(',',$_identities));
	if (empty($_identities['i'])){
	  continue;
	}elseif (!empty($class_name)){
	  if ((strpos($_class,$class_name) === False) && (strpos($class_name,$_class) === False)) continue;
	  // if (cnf_dev) b_debug::xxx("waiting for '$class_name', got '$_class'");
	}else{
	  static $cont = 0;
	  // if ($cont++ < 5) b_debug::traceBack();
	}
	
	// Read the record ID and (if presents) the class name which uses this record
	$selects = array();
	foreach(array_merge($specific_classes,array($_identities['i'])) as $field){
	  if (myPear_db()->columnExists($field,$_identities['t'])) $selects[] = $field;
	}
	if (empty($selects))  continue;

	$query = "SELECT ".implode(',',$selects)." FROM ".$_identities['t']." WHERE ".(myPear::is_int($id) 
										       ? $_identities['i']."='$id'" 
										       : $id);
	// myPear::DEBUG($query);
	$q = myPear_db()->query($query,$stopIfFails=cnf_dev);
	if (empty($q))  continue;
	
	unset($identified_class); 
	while($r = myPear_db()->next_record($q)){
	  $IDs[] = $r[$_identities['i']];
	  $identified_class = $_class;
	  // Eventually replace the generic class name (like bList / bUnit) by the actual class name (bList_eaEmpRecords / bUnit_ea_members)
	  foreach($specific_classes as $c){
	    if (isset($r[$c])){
	      if (cnf_dev || self::$isIdentified_debug) myPear::DEBUG(__METHOD__."($id): $identified_class --> ".$r[$c]);
	      $identified_class = $r[$c];
	    }
	  }
	}
	if (!isset($identified_class)) continue;

	// Save result in the cache
	// Count all the modules/classes where the found SQL table is used
	$cache[$id]['ID'] = $IDs;
	$cache[$id]['generic'] = $identified_class;
	foreach(myPear::$sql_identities as $cls=>$ids){
	  if ($ids['t'] == $_identities['t'])  $cache[$id][$ids['m']] = $cls;
	}
	break;
      }
      if (!isset($cache[$id])) $cache[$id] = False;
      bTiming()->cpu();
    }

    if (empty($cache[$id])){
      self::$isIdentified_ID = array();
      $reply = Null;
    }else{
      // Make the ID's available for the calling function
      self::$isIdentified_ID = $cache[$id]['ID'];
      
      // The caller asks for the class_name, optionally filtered by the current myPear module 
      if($current_module_only){
	// During the authentication the proper Avatar class might not yet being loaded,
	// hence give back the generic one instead.
	$reply = @$cache[$id][b_cms::_(0)];
	if (empty($reply) && @$cache[$id]['myPear']) $reply = $cache[$id]['myPear'];
	if (empty($reply) && (stripos(@$cache[$id]['generic'],'Avatar') !== False)){
	  $reply = 'bForm_Avatar';
	}
      }else{
	$reply = @$cache[$id]['generic'];
      }
    }
    
    // Sanity && debugging
    if (cnf_dev || self::$isIdentified_debug){
      if(($n=count(self::$isIdentified_ID))>1)b_debug::xxx("Search gives $n replies");
    }
    if (self::$isIdentified_debug || cnf_show_ii || (cnf_dev && empty($reply)))     b_debug::xxx($reply,'I');
    return $reply;
  }

/*
 * Get the DB class from cache
 */
  protected static $_cacheF = array();
  static function _fromCache($c,$arg0=Null,$arg1=Null,$arg2=Null){
    foreach(self::$__construct_notComplited as $reset_me){
      foreach(array_merge(array($reset_me),
			  preg_grep("/^$reset_me\./",array_keys(self::$_cacheF))) as $i){
	if (cnf_dev)  b_debug::xxx("Reset cache '$i'");
	unset(self::$_cacheF[$i]);
      }
    }
    self::$__construct_notComplited = array();
    
    locateAndInclude($c,'fatal');
    if (class_exists($c,False)){
      switch($n=func_num_args()){
      case 4: if (!isset(self::$_cacheF["$c.$arg0.$arg1.$arg2"])) self::$_cacheF["$c.$arg0.$arg1.$arg2"] = new $c($arg0,$arg1,$arg2); return self::$_cacheF["$c.$arg0.$arg1.$arg2"];
      case 3: if (!isset(self::$_cacheF["$c.$arg0.$arg1"]))       self::$_cacheF["$c.$arg0.$arg1"]       = new $c($arg0,$arg1);       return self::$_cacheF["$c.$arg0.$arg1"];
      case 2: if (!isset(self::$_cacheF["$c.$arg0"]))             self::$_cacheF["$c.$arg0"]             = new $c($arg0);             return self::$_cacheF["$c.$arg0"];
      case 1: if (!isset(self::$_cacheF["$c"]))                   self::$_cacheF["$c"]                   = new $c();                  return self::$_cacheF["$c"];
      default:	b_debug::internalError("strange call with $n arguments");
      }
    }
  }
  
  static public function TBD($text='.',$class=Null)  { self::MESSAGE($text, $class, 'TBD'); }
  
  static public function ERROR($text,$class=Null){
    //    b_debug::traceBack();
    if (stripos($text,'missing value')!==False)  self::mail_diagnostics(Null,strip_tags($text));
    self::MESSAGE($text, $class, 'error', 'ERROR'); 
  }

  static public function WARNING  ($text,$class=Null){ self::MESSAGE($text, $class, 'warning','WARNING'); }

  static public function WARNING_T($text,$class=Null){ if (cnf_dev) self::MESSAGE($text, $class, 'SICK');  }
  
  static public function MESSAGE2 ($text,$class=Null){ self::MESSAGE($text, $class, 'message_bottom'); }

  static public function MESSAGE_once($text,$class=Null,$id='status',$heading=''){ self::MESSAGE($text,$class,'status',$heading,True); }
  
  static public function INFO_once($text,$heading=''){ self::INFO($text,$heading,True); }

  static public function INFO($text,$heading='',$only_once=False){ 
    if (empty($text))      $text = array();
    if (!is_array($text))  $text = array($text);
    if (is_array($heading))$heading = $heading[0];
    if (!empty($heading))  $heading = x('span style="font-weight:bold"',$heading); 
    
    self::$show_this_message_in_NOUSER_context = True;
    self::MESSAGE($heading . (empty($text) ? '' : x('ul',b_fmt::joinMap('li',$text))),
		  Null,
		  'info',
		  '',
		  $only_once);
    self::$show_this_message_in_NOUSER_context = False;
  }

  static public function DEBUG($text='',$class=Null){ 
    $forced = ($class === 'forced');
    if (($class === False) || (!YBhere && !cnf_dev && !$forced))  return;
    if (!is_object($class)) $class = Null;
    if ($forced) self::MESSAGE_forced($text?$text:b_debug::__(2), $class, 'debug'); 
    else                self::MESSAGE($text?$text:b_debug::__(2), $class, 'debug'); 
  }

  private static $show_this_message_in_NOUSER_context = False;
  static public function MESSAGE_forced($text,$class=Null,$id='warning',$heading=''){
    self::$show_this_message_in_NOUSER_context = True;
    self::MESSAGE($text,$class,$id,$heading);
    self::$show_this_message_in_NOUSER_context = False;
  }

  /*
   * Display the unified message.
   */
  static public function MESSAGE($text,$class=Null,$id='status',$heading='',$only_once=False){
    
    // By default we don't show any messages in no-user context, unless it is explicitly requested 
    $drop = (!self::$show_this_message_in_NOUSER_context &&
	     !cnf_dev && (!class_exists('bAuth',0) ||
			  (class_exists('bAuth',0) && !bAuth::$isReady) || 
			  (class_exists('bAuth',0) && !is_object(bAuth::$av))));
    if ($drop)  return;
    if ($only_once && @$_SESSION[__METHOD__][b_crypt::hash($text.$heading)]++) return;

    // Add the caller in debug mode
    if (cnf_dev && in_array($id,array('error'))) $text = x('strong',x('em',(b_debug::__(array(3),$class).": "))) . $text;
    
    // Go on
    $caller = '';
    if (trim(strip_tags(trim($text)))){
      $css_class = $id;
      switch($css_class){
      case 'TBD':
	$caller = b_debug::__(3,$class);
	$text   = b_fmt::redText("<em>$id: $caller</em> $text");
      case 'status':
	if (!cnf_dev && !cnf_CLI && class_exists('bAuth',0) && !is_object(bAuth::$av)) return;	
	break;
      case 'error':  
	myPear_logs()->syslog(strip_tags($text));
	break;
      case 'SICK': 
	$css_class = 'debug';
      case 'debug':
	if (is_object($class)){
	  $caller = b_debug::__(3,$class);
	  $text = $caller.' '.x('strong',x('em',$text));
	}
	//if (!b_debug::$starting_exiting)
	$text = b_debug::s_e_depth() . $text;
	break;
      default:
      }

      static $MESSAGE_dejaVu = array();
      $msg_id = b_crypt::hash($x=trim(str_replace('&nbsp;','',strip_tags($text . $id . (is_object($class)?@$class->ID:'x')))));
      $msg_id = b_crypt::hash($x=trim(str_replace('&nbsp;','',$text)));

      if (empty($MESSAGE_dejaVu[$msg_id])) $MESSAGE_dejaVu[$msg_id] = 0;
      if (b_cnf::get('noCompactMessages') || b_debug::$starting_exiting || !$MESSAGE_dejaVu[$msg_id]++){
	if ($id=='message_bottom')  $_SESSION['message_bottom'][] = $text; 
	elseif (function_exists('drupal_set_message') && !b_cnf::get('nodrupal')) drupal_set_message($text,$css_class);
	elseif (cnf_CLI) b_fmt::printt($text);
	else   print "<div class='messages $css_class'>$text</div>\n";
      }
    }
  }
  
  static public $messageBuffer_active = False;
  static public $MESSAGE_add_counter = 0;
  static public function MESSAGE_open($title,$h3='h3') {
    if (!empty($_SESSION['messages.cache'])) self::MESSAGE_close();
    $_SESSION['messages.cache'] = array("<$h3>$title</$h3>");
    self::$messageBuffer_active = True;
  }
  
  static public function MESSAGE_close($id='status'){
    if (is_array($c = @$_SESSION['messages.cache'])){
      if ($id == 'status') myPear::MESSAGE(join('<br/>',$c).'<br/>&nbsp;');
      else                 myPear::WARNING(join('<br/>',$c).'<br/>&nbsp;');
    }
    unset($_SESSION['messages.cache']);
    self::$messageBuffer_active = False;
    self::$MESSAGE_add_counter = 0;
  }
  
  static public function MESSAGE_add($text,$add_counter=False,$noCompactMessages=True){
    $noCompactMessages_sv = b_cnf::get('noCompactMessages');
    b_cnf::set('noCompactMessages',(int)$noCompactMessages);
    if ($add_counter) $text = (++self::$MESSAGE_add_counter) . " - $text";
    if (empty($_SESSION['messages.cache']))  myPear::MESSAGE($text);
    elseif(!in_array($text,$_SESSION['messages.cache']) || b_cnf::get('noCompactMessages')) $_SESSION['messages.cache'][] = $text;
    b_cnf::set('noCompactMessages',(int)$noCompactMessages_sv);
  }
  
  static public function dbg($text,$forced=False){
    if (cnf_debug || self::$debug || $forced) b_debug::xxx($text,array(3,True));
  }
}

/*
 * EA place-keeper
 */
class EA extends myPear{

  static public function _MENU()      { return self::_fromCache('APImenu_ea'); } 

  static public function _MAILER()    { return EA_mailer(); } 

}

