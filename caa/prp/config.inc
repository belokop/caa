<?php

$releaseDate = '2017-05-20';
define('PRP_MODULE'   , 'prp');
define('PRP_VERSION','5.2.7');

define('PRP_OK'          , 'ok');
define('PRP_CANCELED'    , 'canceled');
define('PRP_RESERVED'    , 'reserved');
define('PRP_TEXT_reserve', 'reserve new preprint number');

define('RANK_prp_submitter' , 10);
define('RANK_prp_censor' , 12);
define('RANK_prp_manager', 14);

define('PRP_remindingPeriod' , 604800); // a week = 7 * 24 * 3600

// The SQL tables used by the preprints module
// register the module in myPear registry
b_reg::add_module(PRP_MODULE,
		  array('v' =>PRP_VERSION,
			'r' =>$releaseDate,
			'tm'=>'Preprints',  
			'd' =>'Preprints database',  
			'i' =>'bb-book',
			'c' =>__FILE__,
			'l' =>array('reserve','accept','cancel'),
			
			'classes'=>array('bForm_prp'       => array('t'=>'prp_list',
								    'i'=>'prp_id'),
					 'bForm_Avatar_prp'=> array('p'=>'bForm_Avatar')),
			
			'ranks'=>array(RANK_prp_manager=> array('class'=>'bUnit',
								'd'    =>'Preprints manager',  
								'i'    =>'i-usrcheck'),
				       RANK_prp_censor => array('class'=>'bUnit_prp_censors',
								'd'    =>'Preprints censor',  
								'i'    =>'i-user2_no'),
				       RANK_prp_submitter => array('class'=>'bUnit_prp_submitters',
								'd'    =>'Preprints submitter', 
								'i'    =>'bb-edit'))));

if (function_exists('drupal_get_path') && is_file($f=drupal_get_path('module','myPear').'/includes/drupal8_compat.inc')) require_once $f;

// The research fields change name with years...
if ($year = b_cnf::get('_prp_year')){
  if ((int)$year > 2009) unset(PRP::$fields['NP']);
}

/*
 * That is what PRP knows about... 
 * It knows about other things as well, but this it shows to the humans, so 
 * lets define description and user-friendly icons for the knowledge.
 */
class PRP extends myPear{

  static $fields = array('all'=> 'All preprints',
			 'AP' => 'Astrophysics',
			 'CM' => 'Condensed Matter',
			 'HE' => 'High Energy Physics',
			 'NP' => 'Nuclear Physics');

  static public function _MANAGERS_UNIT(){ return myPear::_UNIT(RANK_prp_manager,PRP_MODULE); }

  static public function _MENU()         { return self::_fromCache('APImenu_prp'); } 

  static public function _HOLDER()       { return self::_fromCache('bHolder_prp_preprints'); } 

}

function PRP_access(){
  static $_ACCESS = Null;
  if ($_ACCESS === Null){
    locateAndInclude('APIaccess_prp');
    $_ACCESS = new APIaccess_prp();
  }
  return $_ACCESS;
}

function PRP_mailer(){
  static $mailer = Null;
  if ($mailer === Null){
    locateAndInclude('bMailer_prp');
    $mailer = new bMailer_prp();
  }
  return $mailer;
}

function PRP_censors(){
  static $unit = Null;
  if ($unit === Null){
    locateAndInclude('bUnit_prp_censors');
    $unit = new bUnit_prp_censors();
  }
  return $unit;
}

function PRP_submitters(){
  static $unit = Null;
  if ($unit === Null){
    locateAndInclude('bUnit_prp_submitters');
    $unit = new bUnit_prp_submitters();
  }
  return $unit;
}

function prp_used_by_myOrg(){
  return array('nordita');
}
