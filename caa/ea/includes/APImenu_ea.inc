<?php
/*
 * define all the menu tabs
 */
locateAndInclude('APImenu');

class APImenu_ea extends APImenu{
  var $q    = EA_MODULE;
  var $title= 'Employees&Alumni';
  var $static_menu   = array(
			     tab_ea_home  => array(), // (tab_ea_home_contacts),
			     tab_ea_e     => array(tab_ea_e_groups,tab_ea_e_cv,tab_ea_off_door_labels,tab_ea_off_list),
			     tab_ea_a     => array(tab_ea_a_years),
			     tab_ea_admin => array(tab__mergeAV,tab_ea_admin_dup,tab_ea_admin_rb,
						   // tab_ea_admin_org_mngr,
						   tab_ea_admin_org_structure,
						   tab_ea_admin_org,
						   tab_ea_admin_orgs,
						   tab__syslog),
			     // tab_ea_a_years=> array(),
			     // tab_ea_admin_org  => array(tab_ea_admin_org_mngr,tab_ea_admin_org_structure),
			     // tab_ea_admin_orgs => array(),
			     );
  var $tabWeight     = array(

			     tab_ea_home      => 10,
			     tab_ea_e         => 20,
			     tab_ea_e_groups  => 21,
			     tab_ea_e_cv      => 22,
			     tab_ea_a         => 30,
			     tab_ea_a_years   => 31,
			     tab_ea_admin     => 40,
			     tab__mergeAV     => 41,
			     tab_ea_admin_dup => 42,
			     tab_ea_admin_rb  => 43,
			     tab_ea_admin_org => 44,
			     tab_ea_admin_org_structure => 45,
			     tab_ea_admin_orgs=> 46,
			     tab_ea_off_list   => 88,
			     tab_ea_off_door_labels=>90,
			     tab__syslog      => 99,

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
	$reply = $ea_members && !EA_manager_here;
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
	$reply = EA_manager_here;
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
	$reply = EA_manager_here;
	break;

      case tab_ea_off_list:
	$reply = True;
	break;

      case tab_ea_off_door_labels:
	$reply = EA_manager_here;
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
    // if ($reply) b_debug::xxx($reply);
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
					       'bList_orgTrades'=>myOrg_ID));
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

    case tab_ea_a:
      $see_only = EA_role_alumnus;

    case EA_MODULE:
    case tab__empty:
      //      if (!empty($_GET['z']) || !empty($_GET['x'])) b_url::header(b_url::same('?q='.join('/',array(EA_MODULE,APImenu::tab_code(tab_ea_home)))));
    case tab_ea:
    case tab_ea_e:
      
      if ($av_id = @$_GET['act_parent']){
	locateAndInclude('bForm_Avatar_ea');
	$av = new bForm_Avatar_ea($av_id);
	$av->getForm();
      }else{
	if (empty($see_only))$see_only = EA_role_employee;
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
    case EA_MODULE:
      $menuItem = myOrg_nameS .' '. EA::_MENU()->title_callback();
      break;
    case tab_ea:
      $menuItem = 'People at '.myOrg_nameS;
      break;
    case tab_ea_a: 
      $menuItem = 'Alumni'; 
      break;
    case tab_ea_e: 
      $menuItem = "Staff members";
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
		 : t(bText::_(array('key' => "MENU_$tab",
				    'item'=> $menuItem))));
  }
  return $menuItem;
}
