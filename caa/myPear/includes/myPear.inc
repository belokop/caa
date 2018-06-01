<?php
/**
 * @file
 * Implementation of the class loading & instantiation, timing, class caching, 
 * and other basic basic functions.
 */
class myPear{

  public static  $isReady = False;

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
						  ));

  public static $debug  = False;
  
  function __construct(){

    if (self::$isReady) b_debug::internalError("double invocation of 'myPear'");

    b_debug::_debug(starting);    
    // Register the myPear module
    b_reg::add_module(myPear_MODULE,array('d' =>'', // the module description is empty, hence it will not be shown by default on the web
					  'tm'=>myPear_MODULE,
					  'v' =>myPear_VERSION,
					  'r' =>myPear_DATE,
					  'i' =>'i-anker',
					  'c' =>dirname(__FILE__).'/../config.inc'));
    
    // Optionally add the EA_MODULE
    if (defined('EA_MODULE')){
      $this->identities[EA_MODULE] = array('bForm_Avatar_ea'      =>array('p'=>'bForm_Avatar'),
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
					   );
    }
    
    
    // Filter out the objects which do not have a corresponding database table,
    foreach($this->identities as $module=>$descr){
      $table_exists = True;
      foreach ($descr as $object=>$data){
	if ($table=@$data['t']){
	  if (!myPear_db()->tableExists($table)){
	    unset($descr[$object]);
	    b_debug::_debug("Can't find SQL table for class '$object'",array(cnf_dev,'redText'));
	  }
	}elseif($parent=@$data['p']){
	  $table = @$this->identities[myPear_MODULE][$parent]['t'];
	  if (!$table || !myPear_db()->tableExists($table)){
	    unset($this->identities[myPear_MODULE][$parent]);
	    unset($descr[$object]);
	    b_debug::_debug("Can't find SQL table for the parent object '$parent'",array(cnf_dev,'redText'));
	  }
	}else{
	  b_debug::internalError("Can't find neither SQL table nor parent for object '$object'");
	}
	// if (!empty($descr[$object])) b_debug::_debug("Registering SQL table '$table' for object '$object'",cnf_dev);
      }
      // Register SQL identity if the database is ok
      if (!empty($descr)){
	b_reg::add_SQLidentity($descr,$module);
      }else{
	MSG::WARNING("Hmmm... no sql tables for the module '$module'");
      }
    }
    b_debug::_debug(exiting);    
  }
  
  /*
   *
   */
  static function myPear_enabled_theme(){
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
  static function org_duty($av_id,$ranks,$parent_ID=myOrg_ID){

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
    
  static function authenticated() { if (bAuth::$isReady) return bAuth()->authenticated(); }
        
  static function _UNIT($rank,$module=myPear_MODULE,$parent=Null) { 
    if (empty($parent)){
      myOrg();
      $parent = myOrg_ID;
    }
    $d = b_reg_bUnit::get_description($rank,$module); 
    return loader::_fromCache($d['class'],$parent,$rank);
  }

  /*
   * Initialise myPear database & myPear Mailer
   */
  static function _SETUP($what=Null,$msg=''){ 
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
  static function _MENU(){
    return loader::_fromCache('APImenu'); 
  } 
}

/*
 * EA place-keeper
 */
class EA extends myPear{

  static function _MENU()      { return loader::_fromCache('APImenu_ea'); } 

  static function _MAILER()    { return EA_mailer(); } 

}

