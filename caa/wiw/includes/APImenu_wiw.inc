<?php

locateAndInclude('APImenu');

function _wiw_title_callback($tab, $dTab=Null,$returnText=False){
  if(isset(APItabs::$tab_definition[$tab]))  $tab = APItabs::$tab_definition[$tab];
  if ($tab == WIW_MODULE) $tab = 'Who Is Where?';
  $menuItem = ($returnText
	       ? $tab
	       : t(bText::_(array('key' => "MENU_$tab",'item'=> $tab))));
  return $menuItem;
}

/*
 * Check that the given person has registered trips
 */
function wiw_after_search_check($av_id,$_after_search=''){
  $q = myPear_db()->query("SELECT t_id FROM wiw_trips WHERE t_avid = '$av_id'");
  $reply = (myPear_db()->num_rows($q) > 0);
  return $reply;
}

class APImenu_wiw extends APImenu{
  
  var $title  = 'Who Is Where';
  var $q      = WIW_MODULE;
  var $static_menu = array(tab_wiw_adm    =>array(tab_wiw_admTT),
			   // tab_wiw_sok    =>array(),
			   tab_wiw_myTrips=>array(),
			   tab_wiw_reg    =>array(),
			   );

  var $tabWeight   = array(tab_wiw        => -90,
			   tab_wiw_reg    => -20,
			   tab_wiw_myTrips=> -10,
			   tab_wiw_adm    =>   0,
			   tab_wiw_admTT  =>  30,
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
      if (myOrg()->isOnMyNetwork($_SERVER['REMOTE_ADDR']) || bAuth()->loginPrompt()){
	locateAndInclude('bIcal_matrix_wiw');
	$m = new bIcal_matrix_wiw();
	$m->show();
      }
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
      locateAndInclude('bUnit');
      bUnit::b_tabs_bUnits(WIW_MODULE,myOrg_ID);
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
	$f = myPear::getInstance('bForm_wiw_trip',$_GET['id']);
	$f->av = bAuth::$av;
	$f->getForm();
      }
      break;
      
    case 'formTPL':
      $form = str_replace('bForm_','',$_GET['form']);
      if ($form == 'bForm') $f = new bForm($_GET['id'],'RO',False);
      else                  $f = myPear::getInstance("bForm_$form",$_GET['id'],'strict');
      $f->getForm();
      break;
      
    default:
      $reply = $this->access_callback($tab);
      if (is_null($reply)){
	myPear::WARNING_T("To be implemented...");
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
	$access = (myOrg()->isOnMyNetwork($_SERVER['REMOTE_ADDR']) || bAuth()->loginPrompt());
	break;
	
      case tab_wiw_edit:
	// show this only if there are entries in the database (???)
	// (the db access should not be too early, however)
	$access = bAuth::authenticated();
	break;
	
      case tab_wiw_adm:
      case tab_wiw_admTT:
	$access = WIW_manager_here;
	break;

      case tab_wiw_myTrips:
	$access = WIW::has_rightToModifyTrips(@bAuth::$av->ID);
	break;
	
      default:
	$access = !empty($tab) && bAuth::authenticated();
      }
    }
    // b_debug::xxx((bool)$access , array('M',2,cnf_dev));
    return (bool)$access;
  }
}

