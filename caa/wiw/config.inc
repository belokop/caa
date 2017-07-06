<?php

define('WIW_MODULE'   , 'wiw');
define('WIW_VERSION','5.2.10');
$releaseDate = '2017-07-06';

define('wiw_acl_public', 'public');
define('wiw_acl_public_text','Anyone on the web can see the trip info');

define('wiw_acl_protected','protected');
define('wiw_acl_protected_text','Only the local and/or authenticated users can see the trip info');

define('wiw_acl_default',wiw_acl_protected);

define('RANK_wiw_admin', 11);
define('RANK_wiw_manager', 12);

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

    static $menu = Null;
    if ($menu === Null){
      locateAndInclude('APImenu_wiw.inc'); 
      $menu = new APImenu_wiw();
    }
    return $menu;
  }

  /*
   * Shortcuts "who can do what"
   */
  static public function has_rightToModifyTrips($t_avid=0){
    $reply = (bAuth::authenticated() && (WIW_manager_here || ($t_avid == bAuth::$av->ID)));
    return $reply;
  }
}

/*
 *
 */
function WIW_managers(){
  static $unit = Null;
  if ($unit === Null){
    locateAndInclude('bUnit');
    $unit = new bUnit(myOrg_ID,RANK_wiw_manager);
  }
  return $unit;
}
