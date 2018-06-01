<?php

if (function_exists('locateAndInclude')){
  locateAndInclude('APImenu');
}

/*
 * define all the menu tabs
 */
APItabs::define_tabs(array('tab_ea'              => 'People at '.(defined('myOrg_name') ? myOrg_name : 'here'),
			   'tab_ea_home'         => 'My account',
			   'tab_ea_home_contacts'=> 'My Contact Persons',
			   'tab_ea_a'            => (defined('myOrg_nameS') ? myOrg_nameS : '').' alumni',
			   'tab_ea_a_years'      => 'As the years pass...',
			   'tab_ea_e'            => 'Staff',
			   'tab_ea_e_cv'         => 'CV\'s',
			   'tab_ea_e_groups'     => 'Groups',
			   'tab_ea_admin'        => 'Administer Organization',
			   'tab_ea_admin_rb'     => 'Rebuild EA',
			   'tab_ea_admin_dup'    => 'Find duplicate avatars',
			   'tab_ea_admin_org'    => (defined('myOrg_nameS') ? myOrg_nameS : 'Organization').' structure',
			   'tab_ea_admin_org_mngr'=> 'Organization managers',
			   'tab_ea_admin_org_structure'=> 'Employee types&groups',
			   'tab_ea_admin_orgs'         => 'Other Organizations',
			   
			   'tab_ea_off'               => 'Offices',
			   'tab_ea_off_assign'        => 'Allocate offices',
			   'tab_ea_off_wipe'          => 'Wipe allocations',
			   'tab_ea_off_door_labels'   => 'Print door labels',
			   'tab_ea_off_desk_labels'   => 'Print desk labels',
			   'tab_ea_off_list'          => 'Who sits where List',
			   'tab_ea_off_omatrix'       => 'Who sits where Matrix',
			   ));

class APImenu_ea extends APImenu{
  var $q    = EA_MODULE;
  var $title= 'Employees&Alumni';
  var $static_menu   = array(
			     tab_ea_e     => array(tab_ea_e_groups,
						   tab_ea_e_cv,
						   tab_ea_off_list,
						   tab_ea_off_door_labels,
						   ),
			     tab_ea_a     => array(tab_ea_a_years),
			     divider,
			     tab_ea_home  => array(), // (tab_ea_home_contacts),
			     divider,
			     tab_ea_admin => array(
						   tab_ea_admin_org,
						   tab_ea_admin_org_structure,
						   tab_ea_admin_orgs  => divider,
						   tab__mergeAV       =>divider,
						   tab_ea_admin_dup,
						   tab_ea_admin_rb    => divider,
						   tab__syslog        => divider,
						   // tab_ea_admin_org_mngr,
						   ),
			     // tab_ea_a_years=> array(),
			     // tab_ea_admin_org  => array(tab_ea_admin_org_mngr,tab_ea_admin_org_structure),
			     // tab_ea_admin_orgs => array(),
			     );

  function __construct(){
    parent::__construct();
    $this->defaultTabs[] = tab_ea_e;
  }

  static function startup() {
    static $initDone = Null;

    locateAndInclude('bForm_Avatar_ea');
    APImenu::set_context(EA_MODULE);
    if (is_null($initDone)) $initDone = (bool)((int)myOrg_ID);
    return $initDone;
  }

  function access($tab) {
    static $ea_members = Null;
    if ($ea_members === Null) $ea_members = (bAuth::authenticated()
					     ? (bool)myOrg()->eaMembers()->isMember(bAuth::$av->ID)
					     : False);
    // load if not yet done
    ea_init();

    locateAndInclude('ea_updates.inc');
    if (!self::startup()){
      $reply = False;
    }elseif (($reply = $this->access_callback($tab))===Null){
      $rank = EA_access()->getRank();
      switch($tab){

      case EA_MODULE:
	$reply = True;
	break;

      case tab_ea_home:
	$reply = is_object(bAuth::$av) && RANK__ea_member;
	break;
      case tab_ea_home_contacts:
	$reply = $ea_members && !EA_ORG_manager_here;
	break;

      case tab_ea_admin_orgs:
	$reply = superUser_here;
	break;
	
      case tab_ea_admin_rb:
	if (@$_GET['cron']==='yes'){
	  $reply = True;
	  break;
	}
	myPear_access(EA_MODULE);
	$reply = EA_ORG_manager_here;
	break;

      case tab_ea_e_cv:
	if (!$groups = myOrg()->orgGroups()->get_groups()){
	  $reply = False;
	  break;
	}
      case tab_ea_admin_org_mngr:
      case tab_ea_admin_org_structure:
      case tab_ea_admin_org:
      case tab_ea_admin:
      case tab_ea_admin_dup:
      case tab__mergeAV:
      case tab__syslog:
	myPear_access(EA_MODULE);
	$reply = EA_ORG_manager_here;
	break;

      case tab_ea_off_list:
	$reply = True;
	break;

      case tab_ea_off_door_labels:
	$reply = EA_ORG_manager_here;
	break;

      case tab_ea_e_groups:
	$groups = myOrg()->orgGroups()->get_groups();
	$reply = !empty($groups);
	break;

      case tab_ea_a:
	$reply = in_array(EA_role_alumnus,myOrg()->getAffiliation(True));
	break;

      case tab_ea_a_years:
      case tab_ea_e:
	$reply = True;
	break;
	
      default:
	$reply = $ea_members;
      }
    }
    // if ($reply) $this->dbg($reply);
    return $reply;
  }

  /*
   * processing the request 
   */
  function process($tab){

    //
    // Disaster search...
    //
    if (False){
      bTiming()->cpu('disaster_search');
      myPear_db()->reset_cache();
      $primaryKeys = myPear_db()->get_primaryKeys('list');
      $primaryKeys = array('zzz_unit_members'=>'um_id',
			   'zzz_lists' => 'l_id',
			   'abs_visits' => 'v_id',			   
			   //'prp_list'=>'prp_id',
			   );
      b_debug::print_r($primaryKeys);

      set_time_limit(0);
      //for($n = 260455; $n<260457; $n++){
      for($n = 1; $n<300000; $n++){

	$ww = $where = array();
	foreach(array_values($primaryKeys) as $id) $ww[] = "$id=$n";
	foreach($ww as $w)
	  $where[] = x('(',join(' AND ',array($w,x('(',join(' OR ',array_diff($ww,array($w)))))));
	
	$select="SELECT * FROM ".join(', ',array_keys($primaryKeys))." WHERE ".join(' OR ',$where);
	$q = myPear_db()->qquery($select,True);
	while ($r = myPear_db()->next_record($q)){
	  b_debug::print_r($r,"duplicate ID $n");
	}
      }
      print (int)(strlen($select)/1024)."KB $select";
      bTiming()->cpu();
      return;
    }

    // Create the Organization if not yet done
    if (!self::startup()) return;

    bTiming()->cpu(__FUNCTION__);
    myPear_search()->idv('search staff & alumni');
    $search_args = array('_join'  =>'zzz_unit_members on um_avid:av_id LEFT JOIN zzz_units ON um_uid:u_id',
			 '_where' =>'u_parent = '.myOrg_ID);
    
    /*
     * Process the "click"
     */
    switch ($tab = $this->processTemplates($tab,$search_args)){
      // the processing is done by '$this->processTemplates($tab)'
    case Null:     
      break; 
      
    case tab_ea_admin_org_structure:
      locateAndInclude('bList');
      bList::b_tabs_bLists(myPear_MODULE,array('bList_orgGroups'=>myOrg_ID,
					       'bList_orgEmpTypes'=>myOrg_ID));
      break;
      
    case  tab_ea_admin_org_mngr:
      $exclude = (superUser_here
		  ? array(RANK__ea_member,RANK__ea_contactPerson)
		  : array(RANK__ea_member,RANK__ea_contactPerson,RANK__superuser));
      bUnit::b_tabs_bUnits(myPear_MODULE,myOrg_ID,$exclude);
      break;

    case tab_ea_admin_org:
      myOrg()->getForm();
      break;
      
    case  tab_ea_a_years:
      set_time_limit(0);
      locateAndInclude('bIcal_matrix_ea');
      $m = new bIcal_matrix_ea();
      $m->show();
      break;
      
    case tab_ea_admin_dup:
      locateAndInclude('bForm_Avatar');
      bForm_Avatar::findDuplicates();
      break;

    case tab_ea_home:
      ea_myPage(bAuth::$av);
      break;
      
    case tab_ea_home_contacts:
      locateAndInclude('bUnit_ea_contacts');
      if (!$av_id = @$_GET['av_id']) $av_id = bAuth::$av->ID;
      $c = new bUnit_ea_contacts($av_id);
      $c->getForm();
      break;


    case EA_MODULE:
    case tab__empty:
    case tab_ea:
    case tab_ea_e:
      if (empty($see_only))$see_only = EA_role_employee;
    case tab_ea_a:
      if (empty($see_only))$see_only = EA_role_alumnus;

      if ($av_id = @$_GET['act_parent']){
	locateAndInclude('bForm_Avatar_ea');
	$av = new bForm_Avatar_ea($av_id);
	$av->getForm();
      }else{
	myOrg()->eaMembers()->see_only = $see_only;
	myOrg()->eaMembers()->getForm();
      }
      break;

    case tab_ea_e_cv:
      myOrg()->eaCV()->show();
      break;
      
    case tab_ea_off_list:
      if (!isset($doing)) $doing = 'summary';
    case tab_ea_off_door_labels:
      if (!isset($doing)) $doing = 'doorLabels';
      locateAndInclude('bHolder_ea_Offices');
      $h = new bHolder_ea_Offices($doing);
      $h->listEntries();
	break;

    case tab_ea_e_groups:
      locateAndInclude('bList_orgGroups');
      bList_orgGroups::b_tab_groupMembers(myOrg_ID);
      break;

    case 'ea_remind_password':
      myOrg()->eaMembers()->remind_password($_GET['av_id']);
      //EA_mailer()->remind_password($_GET['av_id']);
      break;
      
    case tab_ea_admin:
      bUnit::b_tabs_bUnits(myPear_MODULE,myOrg_ID,array(RANK__ea_member,RANK__ea_contactPerson));
      break;
      
    case tab_ea_admin_rb:
      myOrg()->eaMembers()->rebuild();
      break;

    case tab_ea_admin_orgs:
      locateAndInclude('bHolder_Organizations');
      $holder = new bHolder_Organizations();
      $holder->listEntries();
      break;
    }
    bTiming()->cpu();
  }
}

function _ea_title_callback($tab, $dTab=Null,$returnText=False){
  if ($dTab){
    $menuItem = ($returnText
		 ? $tab
		 : t($tab));
  }else{
    switch($tab){
    case tab_ea_a: 
      $menuItem = myOrg_nameS.' Alumni'; 
      break;
    case EA_MODULE:
    case tab_ea:
    case tab_ea_e: 
      $menuItem = myOrg_nameS." Staff members";
      break;
    case tab_ea_admin:
      $menuItem = 'Administer '.myOrg_nameS;
    case tab_ea_home:
      if (0) $menuItem = (method_exists(bAuth::$av,'isA') && bAuth::$av->isA() 
			  ? 'My alumnus page'
			  : 'My employment page');
    default:
      $menuItem = APItabs::get_tab($tab);
    }
    $menuItem = ($returnText
		 ? $menuItem
		 : t(b_t::_(array('key' => "MENU_$tab",
				    'item'=> $menuItem))));
  }
  return $menuItem;
}
