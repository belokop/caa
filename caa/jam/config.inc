<?php

define('JAM_MODULE'  , 'jam');
define('JAM_VERSION','5.49.rc');
$releaseDate = '2016-01-08';

// Ranks and rights...
define('RANK_jam_manager'    , 59); // same rights as superuser, but for JAM_MODULE only
define('RANK_jam_posadmin'   , 58); // same rights as RANK_jam_manager, but for the current position only
define('RANK_jam_secr'       , 56); // Secretary, has rights to browse the dossiers & to communicate with applicants
define('RANK_jam_SC1'        , 54); // Selection Committee member with the right to give ratings && to nominate to "long list"
define('RANK_jam_SC2'        , 52); // Selection Committee member with the right to give ratings
define('RANK_jam_observer'   , 50); // Observer, may browse the applicants dossiers
define('RANK_jam_referee'    , 44); // Referee
define('RANK_jam_applicant'  , 40); // Applicant

// show or not the revoked applications
define('cnf_show_revoked',(b_cnf::get('showrevoked',0) ? True : False));

// expiration time (in days) for the recommendation letters.
// Referees do not have access to the expired recommendation letters  
define('JAM_rl_expiration' , 500);

// maximum age for the applicant. TBD - should be configurable;
define('JAM_age_restriction' ,  77);

// default documents type - pdf 
define('JAM_docs_ext', '.pdf');

// Define statuses
foreach(array('JAM_ap_bad_form',
	      'JAM_ap_bogus',
	      'JAM_ap_employed',
	      'JAM_ap_longlist',
	      'JAM_ap_old_phd',
	      'JAM_ap_revoked',
	      'JAM_ap_shortlist',
	      'JAM_pos_archived',
	      'JAM_pos_deadline',
	      'JAM_pos_in_preparation',
	      'JAM_pos_not_yet',
	      'JAM_review_private',
	      'JAM_review_public',
	      ) as $item) define($item,$item);

define('JAM_no',     'no');
define('JAM_yes',    'yes');
define('JAM_bonus',  'bonus');
define('JAM_yes_but','yesbut');

/*
 * Register the JAM module
 */
b_reg::add_module(JAM_MODULE,
		  array('v' =>JAM_VERSION,
			'tm'=>strToUpper(JAM_MODULE),
			'd' =>'Job Application Manager',
			'r' =>$releaseDate,
			'c' =>__FILE__,
			'i' =>'bb-doc_yes',
			
			/*
			 * JAM bForm's & bList's
			 */			
			'classes'=>array('jam_Document'             => array('t' =>'jam_documents',    
									     'i' =>'doc_id'),
					 'bForm_jam_RF'             => array('t' =>'jam_rf',     
									     'i' =>'rf_id'),
					 'bForm_jam_Review'         => array('t' =>'jam_reviews',    
									     'i' =>'r_id'),
					 'bForm_jam_Position'       => array('t' =>'jam_positions',  
									     'i' =>'pos_id'),
					 'bForm_jam_Applicant'      => array('t' =>'jam_applicants',
									     'i' =>'ap_id'),
					 'bForm_jam_Recommendation' => array('t' =>'jam_recommendations',
									     'i' =>'rec_id'),
					 'bList_jam_subPositions'   => array('p' =>'bList',
									     'd' =>'Research Fields'),
					 'bList_jam_requiredDocs'   => array('p' =>'bList',
									     'd' =>'Required Documents'),
					 'bForm_Avatar_jam'         => array('p' =>'bForm_Avatar')),

			/*
			 * JAM roles & ranks
			 * 'i' - icon  for this role
			 * 'd' - human readable role description
			 * 'u' - unit name for this role  (see bUnit_jam class) 
			 */
			'ranks'=>array(RANK_jam_manager      => array('class' => 'bUnit',
								      'i' =>'i-user3',
								      'd' =>'JAM manager'),
				       RANK_jam_posadmin     => array('class' => 'bUnit_jam',
								      'i' =>'i-user3',
								      'd' =>'JAM position responsible'),
				       RANK_jam_secr         => array('class' => 'bUnit_jam',
								      'i' =>'i-user2',
								      'd' =>'JAM secretary'),
				       RANK_jam_SC1          => array('class' => 'bUnit_jam',
								      'i' =>'i-user1',
								      'd' =>'JAM SC1 member'),
				       RANK_jam_SC2          => array('class' => 'bUnit_jam',
								      'i' =>'i-user5',
								      'd' =>'JAM SC2 member'),
				       RANK_jam_observer     => array('class' => 'bUnit_jam',
								      'i' =>'i-user2',
								      'd' =>'JAM Observer'),
				       RANK_jam_referee      => array('i' =>'i-user2_edit',
								      'd' =>'JAM referee'),
				       RANK_jam_applicant    => array('i' =>'i-usrlang',
								      'd' =>'JAM applicant'))));

JAM_posID();
		  

/*
 * Description of the JAM internals:
 * - State consists of Organization + optionally position + optionally research field
 * - Roles of humans taking part in the hiring procedure - from anonymous job seeker to the search committee members
 */
class JAM extends myPear{
  public static $pos_id = 0;
  public static $rf_id  = 0;

  public static $bogusApplications = array();

  public static $pos_statuses = array();
  public static $review_statuses = array();


  /**
   * JAM application status
   * 'c' - code. The codes < 10 are for the "auto-assigned" statuses, those >= 10 are assigned manually 
   * 'i' - icon  for this status
   * 'd' - human readable description (this is shown in the status pop-up menu)
   * 'f' - database field (if any) where the status is saved
   * 'if' - condition when this status makes sense
   * The other statuses (see the above list of statuses) are assigned automatically
   */

  public static $statuses=array(JAM_ap_revoked        => array('c' => 13,
							       'i' =>'trash_cross',
							       'f' =>'ap_status',
							       'd' =>'revoked application'),
				JAM_ap_longlist       => array('c' => 12,
							       'i' =>'star_b1',
							       'd' =>'long-listed',
							       'f' => 'ap_status',
							       'if'=> 'deadline_reached'),
				JAM_ap_shortlist      => array('c' => 11,
							       'i' =>'star_b2',
							       'd' =>'short-listed (position offered)',
							       'f' => 'ap_status',
							       'if'=> 'deadline_reached'),
				JAM_ap_employed       => array('c' => 10,
							       'i' =>'star_b3',
							       'd' =>'employed',
							       'f' => 'ap_status',
							       'if'=> 'deadline_reached'),
				JAM_ap_bogus          => array('c' =>  1,
							       'i'=>'i-avatar',
							       'f' => 'ap_status_auto',
							       'd'=>'application without receipt (bogus?)'),
				JAM_ap_bad_form       => array('c' =>  2,
							       'i'=>'bb-doc_broken',
							       'f' => 'ap_status_auto',
							       'd'=>'application form with errors'),
				JAM_ap_old_phd        => array('c' =>  3,
							       'i'=>'i-usradd',
							       'f' => 'ap_status_auto',
							       'd'=>'applicant with "too old" PhD'),
				
				JAM_pos_in_preparation=> array('c'=> 11,
							       'i'=>'i-notok',
							       'f'=>'pos_status',
							       'd'=>'position is in preparation',
							       'if'=>'!deadline_reached'),
				JAM_pos_archived      => array('c'=> 13,
							       'i'=>'i-lock',
							       'f'=>'pos_status',
							       'd'=>'archived position',
							       'if'=>'deadline_reached'),
				JAM_pos_not_yet       => array('c'=> 1,
							       'i'=>'i-door-closed',
							       'f'=>'pos_status',
							       'd'=>'not yet started'),
				JAM_pos_deadline      => array('c'=> 2,
							       'i'=>'i-portfolio_cross',
							       'f'=>'pos_status',
							       'd'=>'applications deadline is reached'),
				
				JAM_review_private    => array('c'=>13,
							       'i'=>'i-lock_on',
							       'd'=>'reviews are private and are not readable yet'),
				JAM_review_public     => array('c'=>10,
							       'i'=>'i-lock_off',
							       'd'=>'reviews are public'),
				);
  
  /*
   * Get the status variables
   */
  static function status($name,$what='c',$status_field='dummy'){
    $reply = Null;
    if (isset(self::$statuses[$name][$what])){
      $reply = self::$statuses[$name][$what];
    }else{
      foreach(array_keys(self::$statuses) as $id){
	if (empty(self::$statuses[$id]['f'])){
	  if (self::$statuses[$id]['c'] == $name) $reply = @self::$statuses[$id][$what];
	}else{
	  if (self::$statuses[$id]['f'] !== $status_field) continue;
	  if ($name == self::$statuses[$id]['c']) $reply = @self::$statuses[$id][$what];
	}
	if (!empty($reply)) break;
      }
    }
    if (cnf_dev) b_debug::xxx($reply);
    return $reply;
  }
  
  static $SC_ranks = array(RANK_jam_SC1,
			   RANK_jam_SC2,
			   RANK_jam_observer);
  static public function SC_ranks(){
    $reply = array();
    if (JAM_posID()){
      foreach(JAM::$SC_ranks as $rank){
	if (mb_strToLower(JAM_pos()->SC_rank2name($rank)) !== CONST_noname) $reply[$rank] = JAM_posID();
      }
    }
    return $reply;
  }

//static public function j_MENU()                { start_JAM(); return myPear::_fromCache('APImenu_jam'); } 
  static public function j_MENU()                { return myPear::_fromCache('APImenu_jam'); } 

}

/*
 *
 */
function JAM_unit($rank,$p=0,$a=0){ 
  return myPear::_UNIT($rank,JAM_MODULE,(!$p ? JAM_posID() : $p)); 
}


/*
 *
 */
function JAM_access(){
  static $_ACCESS = Null;
  if ($_ACCESS === Null){
    locateAndInclude('APIaccess_jam');
    $_ACCESS = new APIaccess_jam();
  }
  return $_ACCESS;
}

  
/*
 *
 */
function JAM_mailer(){
  static $mailer = Null;
  if ($mailer === Null){
    locateAndInclude('bMailer_jam');
    if (class_exists('bMailer_jam',0)){
      $mailer = new bMailer_jam();
    }else{
      myPear::WARNING_T("????????????????????????????? Can't load bMailer_jam");
      b_debug::traceBack();
    }
  }
  return $mailer;
}

/*
 * Return the current position class as it saved in the cache
 */
function JAM_posID($pos_id=0){
    $reply = b_cnf::get('JAM_posID');
    if (!empty($pos_id) && ($pos_id != $reply)){
      b_cnf::set('JAM_posID',($reply = $_GET['JAM_posID'] = $_POST['JAM_posID'] = $pos_id));
    }
    b_debug::xxx($reply,array('limeText',2));
    return  (int)$reply;
}

/*
 * Get and/or Set the current position
 */
function JAM_pos($pos_id=Null) { 
  start_JAM();
  if (!(int)$pos_id) $pos_id = JAM_posID();
  $reply = ((int)$pos_id
	    ? myPear::getInstance('bForm_jam_Position',$pos_id,array('strict','fatal'))
	    : Null);
  b_debug::xxx((is_object($reply)?$reply->name():$reply),array('limeText',2));
  return $reply;
} 

/* 
 * Get and/or Set the current Research Field
 */
function JAM_RF($rf_id=Null) { 
  if (empty($rf_id)) $rf_id = b_cnf::get('jam_rf');
  if (($jam_rf=b_cnf::get('jam_rf')) != $rf_id) $jam_rf = b_cnf::get('jam_rf',($_GET['jam_rf']=$rf_id));
  if (!(int)$jam_rf)  b_debug::internalError("empty 'jam_rf'");
  $rf = myPear::getInstance('bForm_jam_RF',$jam_rf); 
  JAM::$rf_id = $rf->ID;
  return $rf;
} 


/*
 * Set the login flavor for the applicant/referee
 */
function JAM_callMeOnLogin($av_id){}

/*
 * Triggers for caching the pages
 */
function jam_b_cache_file(){
  locateAndInclude('APIaccess_jam');
  return array_merge($_POST,
		     $_GET,
		     $_FILES,
		     array('rank'=>JAM_access()->getRank(),
			   'pos' =>JAM_posID(),
			   'date'=>b_time::short_date()));
}

/*
 * Setup the environment and pre-load some classes which we will use.
 * We do not to pre-load all of them, since they are "auto loaded" when requested,
 * but it is a good idea to load first 'manually' some root objects
 */
function start_JAM(){
  static $dejaVu = 0;

  if (!defined('superUser_here')) return False;
  if (!$dejaVu++){

    locateAndInclude('jam_functions.inc');
    
    JAM_access();

    // This is needed only for the menu build, since it is built outside the context
    bForm_Avatar::set_context(bAuth::$av,JAM_MODULE);

    locateAndInclude('APImenu');
    locateAndInclude('APIaccess_jam');

    // Set context
    APImenu::set_context(JAM_MODULE);

    // Set the shortcuts
    $rank = JAM_access()->getRank();
    
    // By default the reviews are not readable by the other committee members        
    b_cnf::set('JAM_review_default',JAM::status(JAM_review_private));
    
    JAM_pos();

    // apply updates, if any
    locateAndInclude('jam_updates.inc');

    // Check that we got the needed software
    b_os::needExecutable('iconv', __FILE__,0,'http://en.wikipedia.org/wiki/Iconv');
    b_os::needExecutable('python',__FILE__,0,'http://www.python.org/');
    b_os::needExecutable('pdftk' ,__FILE__,'src/pdftk.tar.gz','http://www.pdflabs.com/');

    // Locate the bogus applications
    if (!cnf_CLI && locateAndInclude('bForm_jam_Applicant')){
      JAM::$bogusApplications = bForm_jam_Applicant::get_bogusApplications();
    }
  }
  return True;
}

