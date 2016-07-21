<?php
$releaseDate = '2016-07-21';
define('WIW_MODULE'   , 'wiw');
define('WIW_VERSION','5.13.rc');

define('wiw_acl_public',   'public');
define('wiw_acl_protected','protected');

define('RANK_wiw_admin', 11);
define('RANK_wiw_manager', 12);

require_once 'config.tabs.inc';
if (function_exists('drupal_get_path') && is_file($f=drupal_get_path('module','myPear').'/includes/drupal8_compat.inc')) require_once $f;

/*
 * Describe the WIW module
 *  'm'  - module name 
 *  'v'  - module version number
 *  'r'  - module release date
 *  'tm' - module name for the "top" hirizontal menu
 *  'd'  - module name for the "navigation" menu
 *  'c'  - absolute path to the configuration file 
 *  'i'  - module favicon
 *  'classes' - bUnit / bList classes used by the module
 */
b_reg::add_module(WIW_MODULE,
		  array('v' =>WIW_VERSION,
			'tm'=>'WhoIsWhere?',
			'd' =>'Who Is Where?',
			'c' =>__FILE__,
			'r' =>$releaseDate,
                        'i' =>'i-airplane',
			'classes'=>array('bList_wiw_tripTypes' => array('p'=>'bList',
									'd'=>'Trip Types'),
					 'bForm_Avatar_wiw'    => array('p'=>'bForm_Avatar'),
					 'bForm_wiw_trip'      => array('t' =>'wiw_trips',
									'i'=> 't_id')),
			'ranks' =>array(RANK_wiw_manager=>array('class'=>'bUnit',
								'd' => 'WIW manager',
								'i' => 'i-user1'))));

myOrg()->setMyNetworks(array('130.242',      // KTH
			     '130.237',      // SU
			     '77.238',       // eduroam
			     ));
			     
/*
 * WIW dispatching class
 */
class WIW extends myPear{

  /* bList_wiw_tripTypes anker
   */
  static public function _TT($org_id=Null){ 
    if (empty($org_id))  $org_id = myOrg_ID;
    return self::_fromCache('bList_wiw_tripTypes',$org_id); 
  }
  
  /* Menu anker
   */
  static public function _MENU()  { 
    locateAndInclude('APIaccess_wiw.inc');
    WIW_access();
    return self::_fromCache('APImenu_wiw'); 
  }

  /*
   * Shortcuts "who can do what"
   */
  static public function has_rightToModifyTrips($t_avid=0){
    $reply = (bAuth::authenticated() && (WIW_manager_here || ($t_avid == bAuth::$av->ID)));
    return $reply;
  }
}
if (class_exists('Drupal\myPear\menuHandler',False)) Drupal\myPear\menuHandler::_toggle_menu_items(\b_reg::get_modules());
