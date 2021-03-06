<?php

if (function_exists('locateAndInclude')) locateAndInclude('APImenu');

APItabs::define_tabs(array('tab_wiw'        => 'Who Is Where?',
			   'tab_wiw_adm'    => 'Administer WIW',
			   'tab_wiw_admTT'  => 'Edit Trip Colors',
			   'tab_wiw_adm_all'=> 'See all Trips',
			   'tab_wiw_myTrips'=> 'My Trips',
			   'tab_wiw_sok'    => 'Search Trips',
			   'tab_wiw_edit'   => 'Modify Trip',
			   'tab_wiw_reg'    => 'Register new trip',
			   ));

function _wiw_title_callback($tab, $dTab=Null,$returnText=False){
  if(isset(APItabs::$tab_definition[$tab]))  $tab = APItabs::$tab_definition[$tab];
  if ($tab == WIW_MODULE) $tab = 'Who Is Where?';
  $menuItem = ($returnText
	       ? $tab
	       : t(b_t::_(array('key' => "MENU_$tab",'item'=> $tab))));
  return $menuItem;
}

/*
 * Check that the given person has registered trips
 */
function wiw_after_search_check($av_id,$_after_search=''){
  if (cnf_dev) b_debug::traceBack();
  $q = myPear_db()->query("SELECT t_id FROM wiw_trips WHERE t_avid = '$av_id'");
  $reply = (myPear_db()->num_rows($q) > 0);
  return $reply;
}

class APImenu_wiw extends APImenu{
  
  var $title  = 'Who Is Where';
  var $q      = WIW_MODULE;
  var $static_menu = array(
			   tab_wiw_reg    =>array(),
			   tab_wiw_myTrips=>array(),
			   tab_wiw_adm    =>array(tab_wiw_admTT,tab_wiw_adm_all),
			   // tab_wiw_sok    =>array(),
			   );
  
  function __construct(){
    parent::__construct();
    // workaround for the horizontal menu
    if (defined('AN_THEME_VERSION') || (b_cnf::get('flavor') == 'an')) $this->static_menu[tab_wiw] = array(); 
  }
  
  function process($tab){ 

    $search_args = array('_header'  => 'Search the travel database',
			 '_button'  => 'show trips',
			 '_join'    => 'wiw_trips ON av_id:t_avid LEFT JOIN zzz_unit_members ON um_avid:av_id  LEFT JOIN zzz_units ON um_uid:u_id',
			 '_where'   => 'u_parent = '.myOrg_ID,
			 '_after_search'=> 'tab_force=tab_wiw_edit&av_id=AV_ID',
			 '_after_search_check' => 'wiw_after_search_check');
    
    $tab_processed = $this->processTemplates($tab,$search_args);
    switch($tab_processed){
      
    case Null: // the processing is completed inside '$this->processTemplates($tab)'
      break; 
      
    case tab_wiw:
    case tab__empty:
    case WIW_MODULE:
      locateAndInclude('bIcal_matrix_wiw');
      $m = new bIcal_matrix_wiw();
      $m->show();
      break;
      
    case 'tab_wiw_edit':
    case  tab_wiw_edit:
    case 'tab_wiw_myTrips':
    case  tab_wiw_myTrips:
      if (empty($_GET['av_id']))  $_GET['av_id'] = @bAuth::$av->ID;
      locateAndInclude('bHolder_wiw_trip');
      $holder = new bHolder_wiw_trip();
      $holder->listEntries();
      break;

    case  tab_wiw_adm:
      // Make sure that there is an administrator appointed
      // Add ea_org_manager(s) if the unit is empty
      if (!WIW_managers()->getMembers()){
	$u_ea = new bUnit(myOrg_ID,RANK__ea_org_manager);
	foreach($u_ea->getMembers() as $um_id=>$r){
	  $this->dbg("RANK__ea_org_manager member ".$r['um_avid'],cnf_dev);
	  WIW_managers()->add_unitMember($r['um_avid']);
	}
      }

      // Draw the members
      //      bUnit::b_tabs_bUnits(WIW_MODULE,myOrg_ID);
      WIW_managers()->getForm();
      break;      

    case tab_wiw_adm_all:
      $_GET['all_trips_once'] = 1;
      locateAndInclude('bHolder_wiw_trip');
      $holder = new bHolder_wiw_trip();
      $holder->listEntries();
      break;

    case tab_wiw_admTT:
      WIW::_TT()->getForm();
      break;

    case tab_wiw_sok:
      myPear_search()->getForm($search_args);
      break;
      
    case tab_wiw_reg:
      if (bAuth()->loginPrompt()){
	if (empty($_GET['id'])) $_GET['id'] = 'new';
	$f = loader::getInstance('bForm_wiw_trip',$_GET['id']);
	$f->av = bAuth::$av;
	$f->getForm();
      }
      break;
      
    case 'formTPL':
      $form = str_replace('bForm_','',$_GET['form']);
      if ($form == 'bForm') $f = new bForm($_GET['id'],'RO',False);
      else                  $f = loader::getInstance("bForm_$form",$_GET['id'],'strict');
      $f->getForm();
      break;
      
    default:
      $reply = $this->access_callback($tab);
      if ($reply === Null){
	MSG::WARNING_T("'$tab' to be implemented...");
      }
      if (!bAuth::authenticated())	bAuth()->loginPrompt();
    }
  }
  
  function access($tab){
    locateAndInclude('APImenu');

    // Start the access mechanizm
    WIW_access();

    if (($access = $this->access_callback($tab))===Null){
      switch ($tab){

      case WIW_MODULE:
	$access = (b_reg::$current_module == WIW_MODULE);
	break;

      case tab_wiw_sok:
	$access = False;
	break;

      case tab_wiw:
      case tab_wiw_reg:
      case tab__empty:
	$access = True; // (myOrg()->isOnMyNetwork($_SERVER['REMOTE_ADDR']) || bAuth()->loginPrompt());
	break;
	
      case tab_wiw_edit:
	// show this only if there are entries in the database (???)
	// (the db access should not be too early, however)
	$access = bAuth::authenticated();
	break;
	
      case tab_wiw_adm:
      case tab_wiw_admTT:
      case tab_wiw_adm_all:
	$access = WIW_manager_here;
	break;

      case tab_wiw_myTrips:
	locateAndInclude('bForm_wiw_trip');
	$access = WIW::has_rightToModifyTrips(@bAuth::$av->ID) && bForm_wiw_trip::getTrips(array('t_avid='.bAuth::$av->ID));
	break;
	
      default:
	$access = !empty($tab) && bAuth::authenticated();
      }
    }
    // b_debug::xxx((bool)$access , array('M',2,cnf_dev));
    $this->dbg((bool)$access);
    return (bool)$access;
  }
}

