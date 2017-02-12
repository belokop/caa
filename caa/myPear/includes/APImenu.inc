<?php
if (function_exists('locateAndInclude')){
  locateAndInclude('APIaccess');
  locateAndInclude('bList');
  locateAndInclude('bUnit');
}

/*
 * Define tabs of "a general interest"
 */
define('divider','divider'); // Menu divider

function APItabs_init(){
  APItabs::define_tabs(array('tab__empty'    =>'emptytab',
			     'tab__bText'    =>'Dictionary',
			     'tab__syslog'   =>'Syslog',
			     'tab__tasks'    =>'Task Manager',
			     'tab__login'    =>'Login',
			     'tab__search'   =>'Search',
			     'tab__update'   =>'Check for updates',
			     'tab__templates'=>'Edit templates',
			     'tab__descr'    =>'Edit forms',
			     'tab__units'    =>'See All units',
			     'tab__lists'    =>'See All lists',
			     'tab__cleanup'  =>'Clean-up the database',
			     'tab__configure'=>'Configure myPear',
			     'tab__fixes'    =>'Fix database',
			     'tab__mergeAV'  =>'Merge avatars',
			     'tab__fixes_bList'=> 'bList(s)',
			     ));
}
APItabs_init();

// workaround "namespace problem"
function APItabs_code2tab($code,$module=''){
  if (!empty($module) && function_exists('drupal_get_path')) require_once drupal_get_path('module',$module)."/includes/APImenu_${module}.inc";
  return APItabs::code2tab($code);
}


function _myPear_access_callback($menuItem){
  return myPear::_MENU()->access_callback($menuItem);
}

class APItabs{

  public  static $tab_definition = array();
  
  public static function define_tabs($tabs){
    foreach($tabs as $code=>$text) APItabs::define_tab($code,$text);

    // Convert url tab codes to human readable values
    $ok = True;
    foreach(explode('/',$_GET['q']) as $k=>$v){
      $hr = ($k > 0
	     ? @$GLOBALS["code2tab"][$v]
	     : $v);
      if (empty($hr)) $ok = False;
      else $q[] = $hr;
    }
    if ($ok) myPear::DEBUG("q = ".join('/',$q));
  }

  public static function define_tab($tab,$text){
    if (!defined($tab)){
      define($tab,$tab);
      $code = (int)APImenu::tab_code($tab);
      self::$tab_definition[$tab] = $text;
      $GLOBALS["code2tab"][$code]  = $tab;
      $GLOBALS["tab2code"][$tab]   = $code;
      $GLOBALS["code2text"][$code] = $text;
    }
  }
  
  public static function get_tab($tab){
    APItabs_init();
    return (empty(self::$tab_definition[$tab])
	    ? "??? $tab"
	    : self::$tab_definition[$tab]);
  }

  public static function code2text($code){
    APItabs_init();
    print "<pre>"; debug_print_backtrace();print "</pre>";
    //    if (empty($GLOBALS["code2text"][$code])) {print "<pre>"; debug_print_backtrace();print "</pre>";}
    return $GLOBALS["code2text"][$code];
  }

  public static function code2tab($code){
    APItabs_init();
    return @$GLOBALS["code2tab"][$code];
  }
  
  public static function tab2code($tab){
    APItabs_init();
    return @$GLOBALS["tab2code"][$tab];
  }
}


class APImenu extends myPear_base{
  var $q           = myPear_MODULE;
  var $sqlBackend  = True;
  var $debug       = False;
  var $editPages   = array();
  var $static_menu = array();
  var $dynamic_menu= array();
  protected $dbg_color = 'redText';


  var $myPear_menu = array(
			   tab__configure => array(),
			   tab__mergeAV=>array(),
			   tab__bText  =>array(),
			   tab__search =>array(),
			   tab__update =>array(),
			   tab__descr  =>array(),
			   tab__units  =>array(),
			   tab__lists  =>array(),
			   tab__syslog =>array(),
			   tab__tasks  =>array(),
			   tab__fixes  => array(tab__fixes_bList,
						tab__cleanup),
			   );
  protected $tabWeight  = array();
  var $defaultTabs= array(tab__search);
  
  private $title_callback = Null;
  private $access_callback= Null;
  private $hash_parent    = Null;

  function __construct($defaultTab=Null){}
  
  /*
   * Make sure that the proper Avatar class are used
   */
  public static function set_context($module=Null){
    bForm_Avatar::set_context(bAuth::$av,$module);
  }

  /*
   * Check is the module to be used by a specific organization only
   */
  public static function _used_by_myOrg($module){
    $reply = True;
    foreach(explode('.',str_replace('/','',$module)) as $m){
      if (in_array($m,module_list())){
	myOrg();
	call_user_func("${module}_init");
	$f = "${module}_used_by_myOrg";
	if (function_exists($f) && 
	    (!in_array(    myOrg_code,call_user_func($f)) ||
	      in_array('-'.myOrg_code,call_user_func($f)))) $reply = False;
      }
    }
    if (!$reply) b_debug::xxx($reply);
    return $reply;
  }

  /*
   * Check access to the common menu items, those like editing the 
   * language translation dictionary, etc.
   */
   public function access_callback($menuItem){

     // Set the proper Avatar class, defined by the current module
     bForm_Avatar::set_context(bAuth::$av);

    if (@$_GET['login']==='yes') return False;
    if (empty($menuItem))        return False;

    if (!self::_used_by_myOrg($menuItem)) return False;

    myPear_logs()->add_access();

    switch ($menuItem){

    case myPear_MODULE:
      $reply = superUser_here;
      break;

    case tab__bText: 
      locateAndInclude('bText');
      $reply = is_object(bAuth::$av) && bAuth::$av->isManager() && bText::$doTranslation;
      break;

    case tab__search:
    case tab__update:
    case tab__cleanup:
    case tab__descr:
    case tab__units:
    case tab__lists:
    case tab__configure:
    case tab__fixes:
    case tab__fixes_bList:
    case tab__tasks:
      $reply = superUser_here || (is_object(bAuth::$av) && bAuth::$av->isManager());
      break;

      // Lets rely on the primary tab access control (this one is normally secondary)
    case tab__mergeAV:
    case tab__templates:
      return Null;

    case tab__syslog:
      $reply = (defined('EA_manager_here') && EA_manager_here);
      break;
      
    case 'bred':
      $reply = False;
      break;
      
      // Null is a 'no reply flag', the calling routine is 
      // supposed to continue the check in this case
    default:
      $reply = Null;
    }
    if ($reply !== Null)    $this->dbg($reply);
    return $reply;
   }

  /*
   * Process commonly used tabs
   */
  public function processTemplates($tab,$search_args=array()){

    //
    // Set the proper context
    //
    bForm_Avatar::set_context(bAuth::$av);
    self::set_context($this->q);

    ///
    // Process "off-line task manager" request
    ///
    if (isset($_REQUEST['myPear_taskManager'])){
      myPear_taskManager()->doTasks();
      return Null;
    }

    //
    // Process common  "non-standard" tabs 
    //
    if ($tab == tab__configure){
      // Setup request. Break the execution, go to setup
      b_url::header(b_url::same('?q=admin/settings/myPear'));
    }elseif($tab == tab__login){
      // login prompt
      $_GET['login'] = 'yes';
    }elseif (empty($tab)){
      $tab = tab__empty;
    }


    //
    // Look for a common task
    //   
    foreach(array('qry',
		  'login',
		  'download',
		  'function',
		  'function_form',
		  'tab_force',		
		  'tab_mypear',
		  'search_block_form',
		  // The arguments below might be unset by those from above,
		  // i.e. the order defines the precedence
		  'ed_bList',
		  '_post_bList_class',
		  'form',
		  '_post_bForm_class_once',
		  'holder',
		  ) as $t) if (in_array($t,array_keys($_REQUEST)) && !empty($_REQUEST[$t])){ 
      //      if (cnf_dev) myPear::WARNING_T("processing \$_REQUEST[$t] = ".$_REQUEST[$t]);
      switch($t){
	
	case 'login':
	  //
	  // Authentication request
	  //
	  if ($_GET[$t]==='yes'){
	    b_cnf::_unset(array('ed_bList',
				'form',
				'holder',
				));
	    if (!bAuth()->loginPrompt()){
	      return Null;
	    }
	  }
	  break 2;
	  
	  
	case 'qry':
	case 'search_block_form':	
	  //
	  $qry = $_POST[$t];
	  if (myPear_search()->accepted($qry)){
	    $tab = tab__search;  
	    $this->dbg($t);
	    b_cnf::_unset(array('form',
				'holder',
				'ed_bList',
				'_button'));
	    if ($cm=@b_reg::$current_module){
	      $reg = b_reg::get($cm);
	      myPear_search()->args = array_merge($search_args,
						    array('_header' => "Searching ".$reg['d']));
	    }
	    $_POST = array('_post_bForm_ID_once'   => 'new',
			   '_post_bForm_class_once'=> 'bForm_Avatar_Search',
			   '_join_as'         => 'regexp',
			   'firstname_search_once' => $qry,
			   );
	    break 2;
	  }else{
	    break 1;
	  }

      case 'function_form':	  
	if (function_exists($_GET[$t])){
	  $f=call_user_func($_GET[$t],$_GET['arg_once'],@$_GET['arg2_once']);
	  $f->getForm();
	  return Null;
	}
	break 2;

      case 'function':    
	  //
	  // Process "free template", i.e. a custom function to be executed.
	  // This is a task for the module API, it is the only one who knows what to do,
	  // hence we return from here with the "NULL" answer
	  //
	  $f = @$_GET['function'];
	  if ((strpos($f,'::') !== False) || (strpos($f,'--') !== False)){
	    list($c,$m) = preg_split('/(::|--)/',$f);
	    locateAndInclude($c);
	  }else{
	    $c = $m = 'not set';
	  }
	  if (function_exists($f) || method_exists($c,$m)){
	    myPear::WARNING_T("call_user_func($f)");
	    if (strpos($f,'--') !== False){
	      $object = new $c();
	      $object->$m(@$_GET['arg_once'],@$_GET['arg2_once']);
	    }else{
	      call_user_func($f,@$_GET['arg_once'],@$_GET['arg2_once']);
	    }
	    return Null;
	  }
	  break 2;
	  

	case 'tab_force':
	  //
	  // Forced tab, has precedence over the "q" tab
	  //
	  myPear::WARNING_T("tab_force='$_GET[$t]', unset tab='$tab'");
	  $tab = $_GET[$t];
	  b_cnf::_unset('holder');
	  break 2;
	  
      case 'download':
	  //
	  // download files for myPear
	  //
	$this->processDownload($_GET['download']);
	return Null;
	break 2;
	  
      case '_post_bForm_class_once':
	foreach(array('id'  => '_post_bForm_ID_once',
		      'form'=> '_post_bForm_class_once') as $_g=>$_p){
	  if (empty($_GET[$_g]) && !empty($_REQUEST[$_p]))  $_GET[$_g] = $_REQUEST[$_p];
	  if (($__g=@$_GET[$_g]) !== ($__p=$_REQUEST[$_p])) myPear::WARNING_T("??? $__g !== $__p",$this); 
	}
      case 'form':

	$form = str_replace('bForm_bForm','bForm','bForm_'.$_GET['form']);

	//
	// 'form templates' get precedence over the 'holder templates',
	// i.e. if both form=X & holder=Y present in the request, the 'holder' is ignored 
	//
	switch($form){
	  // Those are the forms which are NOT subject for the general template
	case 'bForm_Avatar_Search':
	  if (($tab_o=$tab) != tab__mergeAV){
	      $tab = tab__search;
	      myPear::WARNING_T("tab $tab_o ==> $tab");
	  }
	case 'bForm':
	case 'bForm_lists':
	case 'bForm_myPear_Setup':
	case 'myPear_Setup':
	  break;
	  
	default:
	  bForm_Avatar::set_context($form);
	  // Applying template
	  $id = @$_GET['id'];
	  if (!empty($id) && locateAndInclude($form)){
	    // myPear::WARNING_T("applying template '$form'");
	    $f = new $form($id,(isset($_GET['mode']) ? $_GET['mode'] : 'RO'));
	    if (!(int)$id || ($id  == $f->ID)){
	      if ($url = b_url::getReferrer('bUnit'))  $f->buttons_external[] = b_btn::submit(b_url::$getReferrer_comment, $url);
	      $f->getForm();
	      return Null;
	    }
	  }
	  if (cnf_dev) b_debug::traceBack("Can't load $form($id)");
	}
	break 2;
	
      case 'holder':
	  //
	  // holder template
	  //
	  // myPear::WARNING_T("applying template bHolder_$t");
	  locateAndInclude($h="bHolder_$_GET[holder]",'fatal');
	  $h = new $h();
	  $h->listEntries();
	  return Null;
	  break 2;
	  
      case '_post_bList_class':
      case 'ed_bList':	  
	//
	// bList / bUnit editor template
	//
	foreach(array('ed_bList'  => array('_post_bList_class'),
		      'arg1_once' => array('arg1_once','act_id','_post_bList_classID'),
		      'arg2_once' => array('arg2_once'),
		      'l_parent'  => array('act_parent'),
		      ) as $_g=>$synonym){
	  foreach($synonym as $_p){
	    if (empty($_GET[$_g])){
	      $_GET[$_g] = $_REQUEST[$_g] = (empty($_REQUEST[$_p]) ? b_cnf::get($_p) : $_REQUEST[$_p]);
	    }
	  }
	}

	// See the bList name and prepare the arguments 
	if    (stripos(@$_GET['ed_bList'],'bList') !== False) $a = array('l_id','l_parent');
	elseif(stripos(@$_GET['ed_bList'],'bUnit') !== False) $a = array('u_id','u_parent','u_rank');
	else                                                  $a = array();

	// Collect arguments to get the bList
	$args = array();
	foreach($a as $k=>$v) switch($k){
	case 0: 
	  if (!empty($_REQUEST['_post_bList_classID'])){
	    $args[$v] = $_REQUEST['_post_bList_classID']; 
	    break 2;
	  }
	  break;
	case 1: 
	  if     (!empty($_REQUEST['arg1_once'])) $args[$v] = $_REQUEST['arg1_once']; 
	  elseif (!empty($_REQUEST['l_parent']))  $args[$v] = $_REQUEST['l_parent']; 
	  break;
	case 2:
	  if (!empty($_REQUEST['arg2_once']))     $args[$v] = $_REQUEST['arg2_once']; 
	  break;
	}

	// Locate and instantiate the bList
	$L = bList::getListInstance($args,@$_GET['ed_bList']);
	if ($L === Null){
	  // Something went wrong ?
	  if (cnf_dev) b_debug::traceBack("Can't locate bList/bUnit class. args=".b_fmt::joinX(',',$args));
	  break 1;
	}else{
	  // Forget about the bForm, think about bList/bUnit now
	  b_cnf::_unset(array('form'));

	  // Show the dialog and say "the work is done"
	  $L->getForm();
	  return Null;
	}
	
	
	
      case 'tab_mypear':
	//
	// an exotic case when the API knows which tab to be pressed
	//
	myPear::WARNING_T("tab $tab===>'$_GET[tab_mypear]'");
	$tab = $$_GET['tab_mypear'];
	break 2;
      }
    }
    
    //
    // common tasks
    //
    $common = False;
    switch($tab){
      
    case tab__descr:
      $common = True;
      locateAndInclude('bList_formDescription');
      bList_formDescription::show_all();
      break;

    case tab__fixes:
    case tab__fixes_bList:
      myPear_locate_duplicate_bLists();
      break;
      
    case tab__lists:
      myPear::WARNING('TBD');
    case tab__units:
      myPear::WARNING('TBD');
      break;      

    case tab__templates:
      $common = True;
      locateAndInclude('bTpl');
      $tpl = new bTpl();
      if ($tpl_title=@$_GET['tpl_title'])  $tpl->id = bTpl::getTplID($tpl_title);
      $tpl->editor();
      break;

    case tab__update:
      $common = True;
      locateAndInclude('myPear_update');
      myPear_update();
      break;

    case tab__cleanup:
      $common = True;
      locateAndInclude('myPear_maintenance');
      myPear_doSomething('clean_names');
      break;

    case  tab__search:  
    case 'tab__search':  
      b_url::setReferrer('search_again','?tab_force=tab__search','search again');
      $common = True;
      myPear_search()->getForm();
      break;

      // editor for the translation dictionary
    case tab__bText:
      $common = True;
      bText::getForm();
      break;
      
      // locate the people which have clashing identities
    case tab__mergeAV:
      $common = True;
      locateAndInclude('b_avatarsMerger');
      $m = new b_avatarsMerger();
      $m->executor();
      break;
      
    case tab__tasks:
      $common = True;
      myPear_taskManager()->info();
      break;

      //syslog
    case tab__syslog:
      $common = True;
      myPear_logs()->listEntries();
      break;
      
    }
    if ($common){
      b_debug::xxx("common task",array('M'));
      return Null;
    }
    
    //
    // default case, a 'normal' tab
    //
    // ??????? if ($tab != @$_SESSION['oldtab'])  b_url::setReferrer();  // set the "go_back" address for the "edit pages" , see the previous lines 
    $_SESSION['oldtab'] = $tab;
    
    //b_debug::xxx("tab='$tab' referrer=".b_url::getReferrer(),array(2,True));
    return $tab;
  }
  
  /*
   *
   */
  function processDownload($src){
    b_debug::xxx(' to be done...',array('redText',2,True));
    print b_btn::link(CONST_go_back,b_url::same(array(),array_keys($_GET)));
  }
  
  /*
   *
   */
  private function build_tabWeight(){
    // Build the weights
    if (empty($this->tabWeight)){
      $w_l1 = 0;
      foreach($this->static_menu as $l1=>$l2_array){
	$this->tabWeight[$l1] = ++$w_l1;
	$w_l2 = $w_l1 * 100;
	foreach($l2_array as $k=>$l2){
	  if ($l2 == divider) $l2 = $k;
	  $this->tabWeight[$l2] = ++$w_l2;
	}
      }

      // Add the "update" menu item
      if (empty($this->tabWeight[tab__update]))	$this->tabWeight[tab__update] = 99999;
      
    }
  }

  /*
   *
   */
  protected function get_lowLevelMenu($tab_top,$b='strong'){

    $this->build_tabWeight();

    // Build low level menu
    foreach(array_values($this->static_menu[$tab_top]) as $tab){
      if ($this->access($tab)){
	$weight = (myPear::is_empty($w=$this->tabWeight[$tab]) 
		   ? $w
		   : APItabs::get_tab($tab).$tab);
	$acts[$weight] = x('li',x("a href='?q=$_GET[q]/".APItabs::tab2code($tab)."'",x($b,ucwords(APItabs::get_tab($tab)))));
      }
    }
    ksort($acts);
    print x('ul',join("\n",array_values($acts)));
  }

  /*
   * Return the default Menu item for the module
   */
  public function title_callback(){
    $d = b_reg::get(b_reg::$current_module);
    return $d['d'];
  }

  /*
   * Build the dynamic menu if needed
   */
  protected function build_dynamic_menuTree(){}

  /*
   * Called from <hook>_menu
   * The function is written for drupal in mind, but might be used for any environment
   */
  public function build_menuTree($page_callback,$access_callback='') {
    global $dividers;
    global $bred;    if (empty($bred)) $bred = 13;
    bTiming()->cpu(get_class($this).'.'.__FUNCTION__);

    // Reset the cache since the menu will be rebuild
    unset($_SESSION[myPear_cache]);

    if (!defined('MENU_CALLBACK')){
      define('MENU_CALLBACK',          'MENU_CALLBACK');
      define('MENU_NORMAL_ITEM',       'MENU_NORMAL_ITEM');
      define('MENU_DEFAULT_LOCAL_TASK','MENU_DEFAULT_TAB');
      define('MENU_LOCAL_TASK',        'MENU_TAB');
    }

    // Do whatever is needed
    $this->build_dynamic_menuTree();
    
    // Sanity check
    $this->access_callback = (empty($access_callback) ? '_'.$this->q.'_access_callback' : $access_callback);;
    $this->title_callback  = '_'.$this->q.'_title_callback';
    if (!function_exists($this->access_callback)) b_debug::internalError("missing $this->access_callback");
    if (!function_exists($this->title_callback))  $this->title_callback = 'APImenu_title_callback';

    // Navigation item

    $menu = array();
    $type = MENU_NORMAL_ITEM; 

    $r = b_reg::get($this->q);
    //APItabs::$tab_definition[$this->q] = t($r['d']);
    //APItabs::define_tab($this->q,t($r['d']));
    $menu[$this->q] = array('menu_name'       => 'navigation',
			    'page callback'   => $page_callback, 
			    'page arguments'  => array($this->q),
			    'title'           => t(@$r['d']),
			    //'title callback'  => $this->title_callback,
			    //'title arguments' => array($this->q),
			    'access callback' => 'myPear__menu_access',
			    'access arguments'=> array($this->q),
			    //'type'            => $type,
			    );
    if (empty($this->static_menu) && ($this->q != myPear_MODULE)) $menu[$this->q]['type'] = MENU_CALLBACK;
      
    $menus = ($this->q == myPear_MODULE
	      ? array('st'=>$this->myPear_menu)
	      : array('st'=>$this->static_menu,
		      'dt'=>$this->dynamic_menu));
    foreach ($menus as $id=>$menu_tree){
      foreach($menu_tree as $tab=>$secondary){
	if ($tab == divider){
	  $divider_here = True;
	  continue;
	}
	if (@$divider_here) $dividers[] = $tab;
	//printf("%stab=$tab secondary=(%s)<br>\n",(@$divider_here?'DIVIDER ':''),join(',',$secondary));
	$divider_here = False;

	if (empty($tab)) b_debug::internalError("empty tab");
	if ($id=='dt' && !is_array($secondary)){ // dynamic tab without children
	  $dTab = $tab;
	  $tab  = $secondary;
	}else{
	  $dTab = '';
	}
	
	$this->hash_parent = self::tab_code($tab.$dTab);
	$menu[join('/',array($this->q,$this->hash_parent))] = array('page callback'   => $page_callback,
								    'title callback'  => $this->title_callback,
								    'access callback' => $this->access_callback,
								    'page arguments'  => (empty($dTab) ? array($tab) : array($tab,$dTab)),
								    'title arguments' => (empty($dTab) ? array($tab) : array($tab,$dTab)),
								    'access arguments'=> (empty($dTab) ? array($tab) : array($tab,$dTab)),
								    'weight'          => $this->weight($tab,$dTab),	
								    'type'            => MENU_NORMAL_ITEM,
								    'menu_name'       => 'navigation',
							    );
	
	$this->build_menuTree_2($menu, $id, $page_callback, $secondary);
      }
    }
    bTiming()->cpu();
    if (cnf_show_m) b_debug::print_r($menu,'menu');
    if (!empty($dividers)){
      $dividers = array_unique($dividers);
      sort($dividers);
      b_debug::print_r($dividers,'dividers');
    }
    return $menu;
  }  

  private function build_menuTree_2(&$menu, $id, $page_callback, $secondary){
    global $dividers;
    global $bred;    if (empty($bred)) $bred = 13;
    
    $deftabSet = False;
    if (is_array($secondary)){
      //      foreach (array_merge($secondary) as $id_2=>$tab_2){
      foreach ($secondary as $id_2=>$tab_2){
	if ($tab_2 === divider){
	  $tab_2 = $id_2;
	  $dividers[] = $tab_2;
	  //printf("%stab_2=$tab_2 <br>\n",(@$divider_here?'DIVIDER ':''));
	}

	$dTab_2 = ($id==='dt' ? $id_2 : '');	    // treat the dynamic tabs
	$hash   = self::tab_code($tab_2.$dTab_2);
	$menu[join('/',array($this->q,
			     $this->hash_parent,
			     $hash))] = array('page callback'   => $page_callback,
					      'title callback'  => $this->title_callback,
					      'access callback' => $this->access_callback,
					      'weight'          => $this->weight($tab_2,$dTab_2),
					      'page arguments'  => (empty($dTab_2) ? array($tab_2) : array($tab_2,$dTab_2)),
					      'title arguments' => (empty($dTab_2) ? array($tab_2) : array($tab_2,$dTab_2)),
					      'access arguments'=> (empty($dTab_2) ? array($tab_2) : array($tab_2,$dTab_2)),
					      'menu_name'       => 'navigation',
					      );
      }
    }
  }
  

  public static function tab_code($path){
    $reply = sprintf("%u",crc32($path));
    return $reply;
  }
  
  private function weight($tab,$dTab){
    $this->build_tabWeight();
    $reply = (isset(  $this->tabWeight[(string)$tab]) ? $this->tabWeight[(string)$tab]
	      :(isset($this->tabWeight[(string)$dTab])? $this->tabWeight[(string)$dTab]
		: 0));
    return $reply;
  }

  function __toString() { return get_class($this); }

  protected function ZZZdbg($txt=' ',$force=False){
    if ($this->debug||$force) b_debug::xxx($txt,array(3,'orangeText',$this));
  }
}


/*
 * Channel the language translation 
 */
function APImenu_title_callback($tab, $dTab='',$returnText=False){
  if ($dTab){
    $menuItem = ($returnText
		 ? $tab
		 : t($tab));
  }else{
    $menuItem = APItabs::get_tab($tab);
    $menuItem = ($returnText
		 ? $menuItem
		 : t(bText::_(array('key' => "MENU_$tab",'item'=> $menuItem))));
  }
  return $menuItem;
}
