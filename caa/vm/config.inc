<?php
/**
 * Visitors Manager (VM) configuration file
 */
$releaseDate = '2016-05-19';
define('VM_MODULE'   , 'vm');
define('VM_VERSION','5.78');

//
// transition problems...
//
define('modal_sendmail_dialog',cnf_dev||cnf_demo); // Doesn't want to work...
define('include_payLunch_dinner',cnf_dev||cnf_demo);

// Office space allocation
define('allocate_offices',True);

// Automated mails to registrants
define('send_infoMail_to_applicants',(cnf_dev||cnf_demo));
define('send_welcomeMail_to_applicants',(cnf_dev||cnf_demo));
define('send_denialMail_to_applicants',cnf_dev||cnf_demo); 
define('include_personalPage_in_welcomeMail', (cnf_dev||cnf_demo));
define('include_accommodation_in_welcomeMail',(cnf_dev||cnf_demo));
define('include_office_in_welcomeMail',(cnf_dev||cnf_demo));
define('include_payForms_in_welcomeMail', True);
define('see_mails_exchange_with_attenders',(send_infoMail_to_applicants ||
					    send_welcomeMail_to_applicants ||
					    send_denialMail_to_applicants));

//
// VM module "knows" about those visit types
//
define('VISIT_TYPE_COLLABORATION','_COLL');
define('VISIT_OUT_OF_SCOPE',      '_EXTE');
define('VISIT_TYPE_PROGRAM',      '_PROG'); // Long conference
define('VISIT_TYPE_POSSIBLE',     '_VIRT');
define('VISIT_TYPE_RENT',         '_RENT');
define('VISIT_TYPE_OTHER',        '_OTHER');

// Old convention, to be removed...
define('cancel_LODGING_REQ_ONLY',True);
define('reimbursement_email','reimbursement@nordita.org');

/*
 * VM root directories
 */
define('vm_root', dirname(__FILE__).DIRECTORY_SEPARATOR);
define('VM_css',  vm_root.'css');

/*
 * Roles played in VM
 */
define('RANK_vm_manager'   , 90); // the same rights as superuser, but for VM_MODULE only
define('RANK_vm_program_coordinator'    , 30);
define('RANK_vm_observer'  , 28);
define('RANK_vm_endorser'  , 27);
define('RANK_vm_organizer' , 26);
define('RANK_vm_visitor'   , 12); // invited guest, same rights as RANK_vm_registrant
define('RANK_vm_registrant', 10); // event registrant, same rights as RANK_vm_visitor.

/*
 * Event & visit policies
 */
define('VM_E_endorsed',      1);   // Flag 'event is endorsed and locked, no changes allowed'. 
define('VM_E_archived',      2);   // Flag 'event is archived'. Has precedence over other locks
define('VM_E_provideOffice',4);    // Flag 'assign offices for the event'
define('VM_E_provideLunches',16);  // Flag 'provide lunches'
define('VM_E_payTrip',64);         // Flag 'pay trip'
define('VM_E_conferenceDinner',32);// Flag 'dinner'
define('VM_E_endorsmentSent',8);   // Flag 'mail to organizers "event is endorsed" is sent'

define('VM_V_payPerdiem' ,1);           // Flag 'pay per-diem'
define('VM_V_payTrip',2);               // Flag 'pay trip'
define('VM_V_provideLunches',1024);     // Flag 'pay lunch'
define('VM_V_payOther',2048);           // Flag 'pay other expenses, (whatever "other expenses" mean)' 
define('VM_V_payAccommodation',64);     // Flag 'cover living expenses'
define('VM_V_conferenceDinner',8192);   // Flag 'pay conference dinner'
define('VM_V_provideOffice',128);       // Flag 'assign office for the visit'
define('VM_V_infoMailSent',256);        // Flag 'informal informational mail sent'
define('VM_V_infoMailQueued',16384);    // Flag 'waiting in the queue'
define('VM_V_infoMailNC',32768);        // Flag 'informal informational mail is Not Concerned'
define('VM_V_denialMailSent',4);        // Flag 'application denial sent'
define('VM_V_denialMailQueued',131072); // Flag 'waiting in the queue'
define('VM_V_welcomeMailSent',8);       // Flag 'application approval sent'
define('VM_V_welcomeMailQueued',65536); // Flag 'waiting in the queue'
define('VM_V_scrtrInfoSent',32);        // Flag 'guest invitation communicated'
define('VM_V_enMailSent',512);          // Flag '"Event is endorsed" mail to organizer sent'
define('VM_V_enMailQueued',16);         // Flag '"Event is endorsed" mail to organizer sent'
define('VM_V_rdlMailSent',4096);        // Flag 'registration-deadline mail to organizer sent'
define('VM_V_rdlMailQueued',262144);    // Flag '"Event is endorsed" mail to organizer sent'

/*
 * Policies are either 
 * - "human defined", like "to pay OR not to pay"
 * - "auto-controlled" like "was an action executed or not"
 * - "auto-controlled, might be changed by human2, like "to allocate office or not" for a visitor
 * Those flag are set in the VM::$v_policies array - item 't' (stends for "type")
 */
define('auto_maintained', 'auto'); 
define('when_approved',   'w_a'); 
define('in_queue',        'queued');

/*
 * Define per-diem in the default currency
 */
define('VM_maxPerdiemAmount'        , 200);
define('VM_wordPerdiem'         , 'subsistence');
define('VM_wordPerdiem_pay'     , 'To pay '.VM_wordPerdiem.'?');

define('VM_color_budgetEstimate' , 'orangeText');
define('VM_color_budgetCredit'   , 'greenText');

/*
 * Define reasonable period which is needed to book hotels
 */
define('VM_lease_zapas', 30); // in days

/*
 * Reminding period for the visits approval.
 * The reminding mail is sent every VM_reminding_period days to the enforcers
 * until the visit is approved / denied
 */
define('VM_reminding_period', 3); // in days

/*
 * Infinite dates...
 */
// long in the past
define('rent0',b_time::txt2unix('1999-12-31'));
// long in the future
define('rent9',CONST_eternity_t);
// Infinite hut capacity
define('VM_capacity999',999);

/*
 * Conference default budget
 */
define('VM_conferenceBudget',400000);
define('VM_conferenceBudget_week',(int)(VM_conferenceBudget/4));
define('VM_ev_budget_TBD_limit',10); // in days
define('VM_ev_budget_TBD','Budget to be decided');
define('VM_ev_budget_D',  'Allocated budget');


/*
 * Ignore the missing information (like missing accommodation) for the old events
 */
define('AUTO_EXPIRATION', 365);  // in days
define('VM_OLD_PROJECTS',5); // in years

/*
 * How much in history should we go showing the events
 */
b_cnf::set('vm_old_visits',b_cnf::get('vm_old_visits',2*AUTO_EXPIRATION));

/*
 * Constants used:
 */
define('LIST_projects', 'projects');
define('LIST_budgetSource', 'budget sources');

define('EVT_ANY',	          -999999);         

/*
 * Define default accommodation options (Hotel, Apartment, etc.), See bForm_vm_Hut
 * Might be extended, see bList_vm_hutCodes
 */
define('HUT_BI', 'Built-in');   // Built-in hut, used internally
define('HUT_A',  'A');   // apartments
define('HUT_H',  'H');   // hotels
define('HUT_AH', 'AH');  // apartment hotels

/*
 * Accommodation codes
 */
define('LODGING_RM',      '_RM'); // remove accommodation
define('LODGING_RM_TXT',  '_remove accommodation');
define('LODGING_BR',	  '_BR'); // break in the visit
define('LODGING_BR_TXT',  '_break in visit');
define('LODGING_OA',      '_OA'); // own accommodation
define('LODGING_OA_TXT',  '_own accommodation');
define('LODGING_REQ_ONLY','_UA'); // request only (unspecified accommodation)
define('LODGING_REQ_ONLY_TXT', '_a request only'); 

//
// Parse the agenda's "accommodation wish"
//
// Note, the order of the search matters!
//?? how to parse this?
//   "I will ask Nordita to book on my behalf, but pay myself"
$GLOBALS['LODGING_FINDER'] = array(LODGING_REQ_ONLY =>('preselected|indicated by.*organi.ers|my behalf'),
				   LODGING_OA       =>('do not need|will arrange|my way|on my own'
						       .'|I will book a room'
						 //.'|myself|accommodation arranged|other|made accommodation request upon'
						 ),
				   // 'shared_lodging' =>('shared accommodation'),
				   'default_lodging'=>join('|',array('(apartment|accommodation) paid.*by |arrange.*for me|organizers.*arrange my accommodation|'
								     )),
				   );
if (cancel_LODGING_REQ_ONLY) unset($GLOBALS['LODGING_FINDER'][LODGING_REQ_ONLY]);

//
//

define('OFFICE_AUTO_ALLOCATABLE', 'visitors'); // offices with this status are auto-allocated by the bCover
define('AP_AUTO_ALLOCATABLE','auto allocate'); // rooms with this status are auto-allocated by the bCover
define('AP_SHARABLE','shared');                // rooms might be shared 

define('ICON_edit_event',  11);
define('ICON_edit_room',   13);
define('ICON_see_agenda',  14);
define('ICON_edit_visit',  17);
define('ICON_mail_tenant', 18);
define('STATUS_PENDING', 'STATUS_PENDING');
define('STATUS_YES',     'STATUS_YES'); 
define('STATUS_NO',      'STATUS_NO'); 
define('STATUS_CLASH',   'STATUS_CLASH'); 

define('REG_INVOICE', 3);          
define('REG_NYI',     0);          

define('btn_edit_accommodation' , 'modify accommodation');
define('btn_splitLease'         , 'split this accommodation');
define('btn_edit_policy'        , 'modify policy');
define('btn_edit_project', 'update payment source');
define('btn_edit_ap',      'update information');
define('btn_deleteAp',     'delete unused room');
define('btn_deleteLease',  'delete this accommodation');   
define('txt_deleteVisit',  'delete this visit');
define('btn_ok',      'OK');            

define('VM_visit_info'    , 'vInfo');
define('VM_visit_host'    , 'vHost');
define('VM_visit_policy'  , 'vPolicy');
define('VM_visit_expenses', 'vExp');
define('VM_visit_project' , 'vProject');
define('VM_visit_lease'   , 'vLease');
define('VM_visit_whatElse', 'vElse');

/*
 * Duties which VM administrators might have
 */ 
define('DUTY_unlock_events','UNLOCK');
define('DUTY_reimbursement','PAY');
define('DUTY_programs',VISIT_TYPE_PROGRAM);
define('DUTY_guests',  VISIT_TYPE_COLLABORATION);
define('DUTY_update_rooms','ROOMS');

define('VM_wordOtheExp','other expenses');

/*
 * Describe the VM module
 */
b_reg::add_module(VM_MODULE,
		  array('v' =>VM_VERSION,          // module version number
			'tm'=>'Visitors',          // module name for the "top" horizontal menu
			'r' =>$releaseDate,        // module release date
			'd' =>'Visitors Manager',  // module name for the "navigation" menu
                        'i' =>'i-traveler',        // module favicon 
			'c' =>__FILE__,            // absolute path to the configuration file 
			'l' =>array('book',        // log-file items to be shown by default
				    'expenses','sync','visit','endorse'),
	
			/*
			 * Assign the database tables to the form "persistent classes",
			 * needed for the forms auto-loading and browsing
			 *  't' - table in the database
			 *  'i' - column within the table 't' keeping the unique record ID.
			 *        If 'i' is missing, then the table is not used by any form class,
			 *        but rather contains some auxiliary information
			 *  'd' - "l_name" column. 
			 *  'dd'- "l_member_title" column. If missing, then "l_name" (converted to single) is used  
			 */
			'classes'=>array('bForm_vm_Hut'              =>array('i'=>'hut_id',
									     't'=>'abs_huts',   
									     ),
					 'bForm_vm_Room'        =>array('i'=>'a_id',
									     't'=>'abs_rooms',
									     ),
					 'bForm_vm_Expenses'         =>array('i'=>'exp_id',
									     't'=>'abs_expenses',
									     ), 
					 'bForm_vm_Office'           =>array('i'=>'o_id',
									     't'=>'abs_offices', 
					 				     ),
					 //'bForm_vm_Office'           =>array('p'=>'bForm_ea_Office'), 
					 'bForm_vm_Event'            =>array('i'=>'e_id',
									     't'=>'abs_events',
									     ),
					 'bForm_vm_Visit'            =>array('t'=>'abs_visits', 
									     'i'=>'v_id'),
					 'bForm_vm_Lease'            =>array('t'=>'abs_leases', 
									     'i'=>'lease_id'),
					 'bForm_Avatar_vm'           =>array('p'=>'bForm_Avatar'),
					 'bList_vm_agendaEvents'     =>array('p'=>'bList',
									     'd'=>'Agenda events',
									     'dd'=>'event processed by '.VM_MODULE.' module'),
					 'bList_vm_socialEvents'     =>array('p'=>'bList',
									     'd'=>'Social Events selected for the conference'),
					 'bList_vm_socialEventRates' =>array('p'=>'bList',
									     'd'=>'Social Event rates',
									     'dd'=>'rate'),
					 'bList_vm_projects'         =>array('p'=>'bList',
									     'd'=>'Financial Projects',
									     'dd'=>'project code'),
					 'bList_vm_budgetSource'     =>array('p'=>'bList',
									     'd'=>'Budget source'),
					 'bList_vm_reimbursementRates'=>array('p'=>'bList',
									      'd'=>'Reimbursement Rates',
									      'dd'=>'rate'),
					 'bList_vm_cotenants'        =>array('p'=>'bList',
									     'd'=>'Registrants sharing accommodation',
									     'dd'=>'Cotenant'),
					 'bList_vm_hutCodes'         =>array('p'=>'bList',
									     'd'=>'All accommodation options',
									     'dd'=>'Accommodation option'),
					 'bList_vm_accommodationOptions'=>array('p'=>'bList',
										'd'=>'Selected accommodation options',
										'dd'=>'Accommodation option'),
					 ),
			/*
			 * Ranks & Roles of the VM players
			 */
			'ranks'=>array(RANK_vm_manager    => array('class'=>'bUnit',
								   'd'    =>'VM manager',  
								   'i'    =>'user_manager'),
				       RANK_vm_endorser   => array('class'=>'bUnit_vm_endorses',
								   'd'    =>'VM endorser', 
								   'i'    =>'user_exclamation'),
				       RANK_vm_program_coordinator     => array('class'=>'bUnit_vm_bookers',
								   'd'    =>'VM booker', 
								   'i'    =>'user_pencil'),
				       RANK_vm_observer   => array('class'=>'bUnit_vm_observers',
								   'd'    =>'VM observer', 
								   'i'    =>'i-usrs'),
				       RANK_vm_organizer  => array('class'=>'bUnit_vm_organizers',
								   'd'    =>'VM event organizer', 
								   'i'    =>'i-usrs'),
				       RANK_vm_registrant => array('d'    =>'VM registrant', 
								   'i'    =>'i-routard_gray'),
				       RANK_vm_visitor    => array('d'    =>'VM visitor', 
								   'i'    =>'i-traveler'))));
/*
 * That is what VM knows about... 
 * It knows about other things as well, but this it shows to the humans, so 
 * lets define description and user-friendly icons for the knowledge.
 */
class VM extends myPear{

  public static $isReady = False;

  /*
   * Which forms (blankets) to send to the Visitors in the welcome mail
   * The forms are kept in the /templates directory
   */
  static $forms2send  = array();
  static $forms2admin = array();

  /*
   * Define default travel expenses in the default currency by zones
   *   n - zone definition (list of countries and/or continents)
   *   p - average price to travel from the zone
   */
  static $travel_zones = array('trip local'   =>array('p'=>1000, 'n'=>array('SE')),
			       'trip nordic'  =>array('p'=>1500, 'n'=>array('FI','DK','NO','SE')),
			       'trip europe'  =>array('p'=>2500, 'n'=>array('IL','IS','Europe')),  // Israel & Iceland is Europe :-)
			       'trip overseas'=>array('p'=>5000, 'n'=>array('Africa','Asia','North America','South America','Oceania')));
  
  /*
   *
   */
  static $default_hut_codes = array(HUT_H => array('prefix' =>'Hotel',
						   'position'=>'b'), // 'hotel' before the name
				    HUT_A => array('prefix' =>'Apartment',
						   'position'=>'a'), // 'Apartment' after the name
				    HUT_AH=> array('prefix' =>'Apartment hotel',
						   'position'=>''),  // not printed
				    ); 
  
  
  /*
   * Define the default set of "social events" (se)
   * 'p' - price for the se
   * 'm' - price model, see bList_vm_socialEventRates class (e.g. 'ppp' is 'Price Per Person', etc.)
   * 'c' - flag "this se is compulsory", i.e. always present
   * 'd' - flag "this compulsory event is default"
   * 'e' - flag "this event might be setup for every day" (e.g. lunch every day during the event)
   * 'f' - base price for an event with the price model 'pppf'
   */
  static $SE = array('Coffee + cookies + fruits at Nordita'=> array('p'=>200,'m'=>'ppd','c'=>1,'d'=>1), // compulsory, default
		     'Coffee + cookies at AlbaNova'        => array('p'=> 40,'m'=>'ppp','c'=>1),        // compulsory, an option
		     'Breakfast first day'                 => array('p'=> 42,'m'=>'ppp'),
		     'Lunch'                               => array('p'=> 68,'m'=>'ppp','e'=>1),        // might be "every day"
		     'Reception on a boat to archipelago'  => array('p'=>700,'m'=>'pppf','f'=>24000),
		     'Reception at Alba Nova restaurant'   => array('p'=>500,'m'=>'ppp'),
		     'Reception at a City restaurant'      => array('p'=>600,'m'=>'ppp'),
		     'Reception barbecue at Nordita'       => array('p'=> 80,'m'=>'ppp'),
		     'Reception wine and snacks at Nordita'=> array('p'=> 80,'m'=>'ppp'),
		     'Visit to  the  Vasa Museum'          => array('p'=> 80,'m'=>'ppp'),
		     'Visit to the Observatory museum'     => array('p'=> 60,'m'=>'ppp'),
		     'Visit to Skansen'                    => array('p'=> 70,'m'=>'ppp'), // 60 for >= 10 people
		     'Visit to Sandhamn'                   => array('p'=>210,'m'=>'ppp'),
		     'Visit to The Nobel Museum'           => array('p'=> 60,'m'=>'ppp'),
		     );
  

  static $endorsements = array(VISIT_TYPE_COLLABORATION =>'bList::orgGroups',
			       VISIT_TYPE_PROGRAM       =>array('All Programs'), 
			       VISIT_TYPE_RENT          =>array('Renting aps'),
			       VISIT_TYPE_OTHER         =>array('Other visits'));
  
  /*
   * Reimbursements for various type of visits, set defaults for "to pay or not to pay" 
   */
  static $reimbursable_visits = array(VISIT_TYPE_PROGRAM       =>array('d'            =>'Program/Conference attenders',
								       VM_V_payPerdiem=>'no',
								       VM_V_payOther  =>'no',
								       VM_V_conferenceDinner=>'maybe',
								       VM_V_provideLunches=>'maybe'),
				      VISIT_TYPE_COLLABORATION =>array('d'            =>'Invited researchers',
								       VM_V_payPerdiem=>'maybe',
								       VM_V_payOther  =>'maybe',
								       VM_V_provideLunches=>'maybe'),
				      VISIT_TYPE_OTHER         =>array('d'            =>'Other visits',
								       VM_V_payPerdiem=>'no',
								       VM_V_payOther  =>'no'),
				      'checkbox'               =>array(VM_V_payOther));
  

  /*
   * Event / visit policy pairs
   */
  static $E_V_policies = array(VM_V_payTrip          => VM_E_payTrip,
			       VM_V_provideOffice    => VM_E_provideOffice,
			       VM_V_conferenceDinner => VM_E_conferenceDinner,
			       VM_V_provideLunches   => VM_E_provideLunches,
			       );

  /*
   * Visit policy bits 
   */
  static $v_policies     = array();

  static $offers         = array(VM_V_provideOffice    => array('d'=>'To allocate office?',
								't'=>when_approved,
								'i'=>'i-office'));
  
  static $reimbursements = array(VM_V_payPerdiem       => array('d'=> VM_wordPerdiem_pay,
								'i'=> VM_wordPerdiem),
				 VM_V_payTrip          => array('d'=>'To pay the trip?',
								'i'=>'trip'),
				 VM_V_provideLunches       => array('d'=>'To pay the lunches?',
								'i'=>'i-burger'),
				 VM_V_conferenceDinner     => array('d'=>'To pay the conference dinner?',
								'i'=>'i-drink'),
				 VM_V_payOther         => array('d'=>'To cover other expenses?'
								),
				 VM_V_payAccommodation => array('d'=>'To pay accommodation?',
								'i'=>'i-hotel',
								't'=> auto_maintained));
  
  static $communications = array(VM_V_infoMailSent     => array('d'=>'Was a general info mail sent?',
								't'=> auto_maintained),
				 VM_V_infoMailNC       => array('d'=>'Is a general info mail needed?',
								't'=> auto_maintained),
				 VM_V_denialMailSent   => array('d'=>'Was the denial letter sent?',
								't'=> auto_maintained),
				 VM_V_welcomeMailSent  => array('d'=>'Was the approval letter sent?',
								't'=> auto_maintained),
				 VM_V_scrtrInfoSent    => array('d'=>'Was the guest invitation communicated?',
								't'=> auto_maintained),
				 VM_V_rdlMailSent      => array('d'=>'Was the appl. deadline communicated?',
								't'=> auto_maintained),
				 VM_V_enMailSent       => array('d'=>'Was the endorsement communicated?',
								't'=> auto_maintained),
				 VM_V_infoMailQueued   => array('d'=>'isMailWaitingInQueue',
								't'=> auto_maintained),
				 VM_V_welcomeMailQueued=> array('d'=>'VM_V_welcomeMailQueued',
								't'=> auto_maintained),
				 VM_V_denialMailQueued => array('d'=>'VM_V_denialMailQueued',
								't'=> auto_maintained),
);
  
  /*
   * Event policies
   */  
  static $e_policies   = array(VM_E_provideOffice  => array('d'=>'Provide offices for attenders?',
							    'i'=>'i-office'),
			       VM_E_provideLunches => array('d'=>'Provide lunch tickets for attenders?',
							    'i'=>'i-burger'),
			       VM_E_payTrip        => array('d'=>'Reimburse travel expenses for some attenders/speakers?',
							    'i'=>'plane'),
			       VM_E_conferenceDinner=>array('d'=>'Organize a conference dinner?',
							    'i'=>'i-drink'),
			       VM_E_endorsed       => array('d'=>'Is the event endorsed & changes are locked?',
							    'i'=>'i-lock'),
			       VM_E_endorsmentSent => array('d'=>'Was the endorsement communicated?',
							    't'=> auto_maintained),
//			       VM_E_archived       => array('d'=>'Is the event archived?',
//							    'i'=>'i-lock'),
			       );
  
  /*
   * Define the objects.
   *  d - object description
   *  i - icon corresponding to the object (if it makes sense)
   *  p - policy (if applicable) 
   */
  static public $description=array(VISIT_TYPE_COLLABORATION=>array('d'=>'Scientific collaboration',
								   'i'=>'a_lecture',
								   'f'=>'debit', 
								   'p'=>array(VM_V_payTrip,VM_V_payAccommodation,VM_V_provideLunches,VM_V_provideOffice,
									      // VM_V_payPerdiem, 
									      VM_V_scrtrInfoSent)),
				   VISIT_TYPE_PROGRAM      =>array('d'=>'Program attender',
								   'i'=>'a_conf',
								   'f'=>'debit',
								   'p'=>array(VM_V_payTrip,VM_V_payAccommodation,VM_V_provideOffice, 
									      VM_V_provideLunches,VM_V_conferenceDinner,
									      VM_V_denialMailSent,   VM_V_welcomeMailSent,  VM_V_infoMailSent,  VM_V_infoMailNC,
									      VM_V_denialMailQueued, VM_V_welcomeMailQueued,VM_V_infoMailQueued,
									      VM_V_rdlMailSent,VM_V_enMailSent)),
				   VISIT_TYPE_POSSIBLE     =>array('d'=>'Virtual visit, used to estimate budget',
								   'i'=>'i-avatar',
								   'f'=>'debit',
								   'p'=>array()),
				   VISIT_TYPE_RENT         =>array('d'=>'Tenant renting out apartment, to be invoiced',
								   'i'=>'bb-home',
								   'f'=>'credit',
								   'p'=>array()),
				   VISIT_TYPE_OTHER        =>array('d'=>'Other',
								   'i'=>'a_meeting',
								   'f'=>'debit',
								   'p'=>array(VM_V_payTrip,VM_V_payAccommodation, // VM_V_payPerdiem,VM_V_payOther,
									      VM_V_scrtrInfoSent)),
				   VISIT_OUT_OF_SCOPE      =>array('d'=>'Out of VM scope',
								   'i'=>'a_meeting',
								   'p'=>array()),

				   // Accommodation options
				   LODGING_OA              =>array('d'=>'Own accommodation, no fin. support', 'p'=>array()),
				   LODGING_BR              =>array('d'=>'Break in the visit, no fin. support','p'=>array()),
				   );

  static public $visit_status = array(STATUS_PENDING => 'pending',
				      STATUS_YES     => 'approved',
				      STATUS_NO      => 'refused');

  /*
   * Some useful icons
   */
  static public $icons     = array(ICON_edit_room  => array('d'=>'update the accommodation',
							    'i'=>'i-edit'),
				   ICON_edit_event => array('d'=>'update the event information',
							    'i'=>'i-edit'),
				   ICON_edit_visit => array('d'=>'update accommodation for this tenant',
							    'i'=>'bb-home'),
				   ICON_mail_tenant=> array('d'=>'send an e-mail to the tenant',
							    'i'=>'mail'),
				   ICON_see_agenda => array('d'=>'see the event in the agenda system',
							    'i'=>'nordita'));
  static public $e         = Null;       // Selected event class
  static public $visitTypes=array();
  static public $formDummy = Null;
  static public $hut_bi    = 'hut_bi';   // "Built-in" hut
  static public $REQ_ONLY  = 'REQ_ONLY'; // "request only" room
  static public $OA        = 'OA';       // "own accommodation" room

  static public $auto_maintained = array(); // Helper, filled from the previous array
  static public $when_approved   = array(); // Helper, filled from the previous array
  
  static public function _MENU(){
    if (b_reg::$current_module == VM_MODULE) start_VM();
    return self::_fromCache('APImenu_vm'); 
  } 

  static function start(){
    static $dejaVu = 0;
    if (!$dejaVu++){
      
      if (!include_payLunch_dinner){
	foreach(array(VM_V_conferenceDinner, VM_E_conferenceDinner,
		      VM_V_provideLunches,   VM_E_provideLunches) as $item){ 
	  foreach (self::$reimbursable_visits as $type=>$x) unset(self::$reimbursable_visits[$type][$item]);
	  foreach (self::$description as $type=>$x) self::$description[$type]['p'] = array_diff(self::$description[$type]['p'],array($item));
	  unset(self::$e_policies[$item]);
	  unset(self::$E_V_policies[$item]);
	  unset(self::$reimbursements[$item]);
	}
      }

      // Visit policies is a combination of all offers we provide
      self::$v_policies = (self::$reimbursements + 
			   self::$communications + 
			   self::$offers);

      // Needed only for the menu build, since it is built without a context
      bForm_Avatar::set_context(bAuth::$av,VM_MODULE);
      
      // Initialise the access control mechanism
      VM_access();

      // Fill auto_maintained array      
      foreach(self::$v_policies as $policy=>$data){
	if (@$data['t'] === auto_maintained) self::$auto_maintained[] = $policy;
	if (@$data['t'] === when_approved)   self::$when_approved[]   = $policy;
      }

      // Complete the description
      foreach(array(VISIT_TYPE_COLLABORATION,
		    VISIT_TYPE_PROGRAM,
		    VISIT_TYPE_RENT, 
		    VISIT_TYPE_OTHER,
		    ) as $type){
	if (($type == VISIT_TYPE_OTHER) && !VM_manager_here && !VM_endorser_here) continue; 
	$d = @self::$description[$type]['d'];
	$i = @self::$description[$type]['i'];
	self::$visitTypes[$type] = $d;
	bIcons()->setDescription(array('d'=>$d, 'i'=>$i));
      }
      bIcons()->setDescription(self::$icons);

      // Create internal auxiliary forms
      self::$formDummy  = new bForm('new','RO',False);
      self::$formDummy->defineVariable('vm_eventSelector','vm_eventSelector');
      
      self::start_HUT_BI();

    }
  }
  
  /*
   * Instantiate the Build-in accommodation objects
   */
  static function start_HUT_BI(){
    if (!is_object(self::$hut_bi)){
      foreach(array(// "the build-in hut" 
		    self::$hut_bi  =>array('class' =>'bForm_vm_Hut',
					   'key'   =>'hut_code',
					   'fields'=>array('hut_name'=>'Built-in',
							   'hut_code'=>HUT_BI)),
		    // "own accommodation" 
		    self::$OA      =>array('class' =>'bForm_vm_Room',
					   'key'   =>'a_name',
					   'fields'=>array('a_name'    => LODGING_OA_TXT,
							   'a_price'   => 0.01,
							   'a_capacity'=> 999)),
		    // request-only 
		    self::$REQ_ONLY=>array('class' =>'bForm_vm_Room',
					   'key'   =>'a_name',
					   'fields'=>array('a_name'     => LODGING_REQ_ONLY_TXT,
							   'a_price'    => 0.001,
							   'a_capacity' => 999))) as $instance=>$args){
	locateAndInclude($class_name=$args['class'],'fatal');
	
	// Hook the "build-in rooms" to the "build-in hut"
	if ($class_name == 'bForm_vm_Room') $args['fields']['a_hutid'] = self::$hut_bi->ID;
	
	// Instantiate the build-in class
	self::$$instance = new $class_name(b_fmt::joinX(' AND ',$args['fields'],"'"));
	self::$$instance->defineVariables();
	if (!(int)self::$$instance->ID){
	  self::$$instance->debug = True;
	  self::$$instance->updateDB($args['fields']);
	}
	if (False && cnf_dev){
	  b_debug::xxx(self::$$instance->__toString());
	  self::$$instance->show_gv = True;
	  foreach($args['fields'] as $k=>$v) self::$$instance->getValue($k);
	  self::$$instance->show_gv = False;
	}
      }
    }	      

    if (cancel_LODGING_REQ_ONLY){
      locateAndInclude('bForm_vm_Lease');
      bForm_vm_Lease::cancel_LODGING_REQ_ONLY();
    }
  }
  
  
  /*
   * Is av_id an Event Organizer?
   */
  public static function isOrganizer($av_id=Null,$e_id=Null){
    static $was = array('?','?','?');
    static $events = array();
    start_VM();

    $reply = False;
    if ( empty($av_id)) $av_id = @bAuth::$av->ID;
    if (!empty($av_id)){
      if (@$events[$av_id] === Null){
	locateAndInclude('bUnit_vm_organizers');
	$events[$av_id] = bUnit_vm_organizers::getEvents($av_id);
      }
      if ($e_id === Null) $reply = !empty($events[$av_id]);
      else                $reply = in_array($e_id,array_keys($events[$av_id]));
    }
    
    //    if ($was !== ($n=array($reply,$av_id,$e_id))) b_debug::traceBack(var_export($was[0],True)." av=$av_id,e=$e_id ---> ".var_export($reply,True));
    $was = @$n;
    return $reply;
  }
  
  /*
   * Is av_id a registrant?
   */
  public static function isRegistrant($av_id=Null,$e_id=Null){
    static $visits = array();
    if (empty($av_id)) $av_id = bAuth::$av->ID;
    if (!empty($av_id)){
      if (@$visits[$av_id] === Null){
	locateAndInclude('bForm_vm_Visit');
	$visits[$av_id] = bForm_vm_Visit::getVisits($av_id,array(VISIT_TYPE_PROGRAM));
      }
      if ($e_id === Null) $reply = !empty($visits[$av_id]);
      else                $reply = in_array($e_id,$visits[$av_id]);
    }
    return @$reply;
  }
  
  /*
   * Currently selected event ID
   */
  static function e_ID($e_ID=Null){
    start_VM();
    if ($e_ID===Null) $e_ID = (isset($_REQUEST['vm_eventSelector'])
			       ? $_REQUEST['vm_eventSelector'] 
			       : (int)b_cnf::get('vm_eventSelector'));
    
    if ((int)$e_ID > 0){
      if (!is_object(self::$e))	self::$e = myPear::getInstance_new('bForm_vm_Event',$e_ID,'strict');
      if ($e_ID != ($id=@self::$e->ID)){
	self::$e = myPear::getInstance_new('bForm_vm_Event',$e_ID,'strict');
	myPear::WARNING_T("Change VM::e_ID $id -> $e_ID ");
      }
    }
    
    if (is_object(self::$e)){
      $e_ID = self::$e->ID;
      b_cnf::set('vm_eventSelector',$e_ID);
    }else{
      $e_ID = self::$e = Null;
      b_cnf::set('vm_eventSelector',0);
    }
    $name = (is_object(self::$e) ? ' = '.self::$e->name() : '-no-event-');
    b_debug::xxx(var_export($e_ID,True).' '.$name, array(2,'orangeText',cnf_dev));
    return $e_ID;
  }
  
  /*
   * Check that the currently selected event is (or is not) out of VM scope, i.e.
   * is it marked in the list events Agenda events as "not my event"
   */
  static function outOfScope($e=Null){
    bTiming()->cpu(__FUNCTION__);
    locateAndInclude('bList_vm_agendaEvents');
    self::e_ID();
    if ($e === Null) $e = self::$e;
    $reply = (is_object($e) && bList_vm_agendaEvents::eventOutOfScope($e->getValue('e_code')));
    bTiming()->cpu();
    if (is_object($e)) b_debug::xxx($e->name().'? '.x('strong',var_export($reply,True)),array(2,cnf_show_out));
    return $reply;
  }

  static function isArchived($e=Null){
    if ($e === Null)  $e = self::$e;
    return (bool)(is_object($e) && $e->isArchived());
  }

  static function isEventEndorsed($e=Null){
    if (is_null($e)) $e = self::$e;
    return (is_object($e) && $e->isEventEndorsed());
  }

  /*
   *
   */
  static function hasRightTo($what,$argv=array()){

    // Visits with the closed budget can be edited by the superuser only
    if (is_array($argv)){
      locateAndInclude('bForm_vm_Expenses');
      if (bForm_vm_Expenses::_exp_arePayed($argv)) $reply = False;
    }

    if (!defined('VM_manager_here') || VM_manager_here) return True;
    if (superUser_here)        return True;
    if (!is_object(bAuth::$av))return False;
    
    bTiming()->cpu(__FUNCTION__);
    switch($what){
      
    case 'request_visits': 
      $reply = (VM_employee_here || VM_lt_visitor_here);
      break;
      
    case 'setup_SE':
      $reply = (VM_program_coordinator_here || VM_organizer_here);
      if (self::isArchived())       $reply = False;
      break;
      
    case 'book_ah': 
      if (!($reply = (VM_program_coordinator_here || (VM_organizer_here && is_object(self::$e) && bAuth::$av->isOrganizer(self::$e->ID))))){
	$reply = (is_object($visit=$argv) && $visit->isWritable());
      }
      break;
      
    case 'book':
      $reply = (VM_program_coordinator_here || (is_object($visit=$argv) && $visit->isWritable()));
      break;
      
    case 'assign_offices':
    case 'select_budget':  
      $reply = VM_administrator_here;
      break;
      
    case 'send_info_mail':  
      if (self::isArchived())$reply = False;
      else                 $reply = (!self::isEventEndorsed() && (VM_observer_here || VM_organizer_here));
      break;
      
      case 'send_welcomeMail':  
	$reply = VM_program_coordinator_here || self::hasRightTo('endorse_event');
	break;
	
    case 'endorse_event':
      if (self::isArchived()) $reply = False;
      else                    $reply = VM_unlocker_here || VM_manager_here || bAuth::$av->isEndorser(VISIT_TYPE_PROGRAM);
      break;
      
    case 'approve_guest_invitation':
      if (is_object($visit=$argv) && (int)$visit->ID){
	$reply = $visit->formBlock_isWritable(VM_visit_policy);
      }else{
	$reply = EA_member_here;
      }
      // $reply = (is_object($visit=$argv) && $visit->formBlock_isWritable(VM_visit_policy));
      break;
      
    case 'approve_registrant_application':
      $reply = (VM_organizer_here || VM_manager_here) && !self::isArchived();
      break;
      
    case 'edit_visit':
      if (is_object($visit=$argv)){
	$reply = ($visit->isHost() || self::hasRightTo('approve_guest_invitation',$visit) || self::hasRightTo('approve_registrant_application'));
	break;
      }
      $e = Null;
    case 'setup_reimbursement':
      if (!isset($e)) $e = self::$e;
      $reply = (!self::isEventEndorsed($e) && ( ((int)@$argv['v_eid'] && VM_organizer_here) ||
			   		      (@$argv['v_host_avid'] == bAuth::$av->ID)) );
      break;
      
    default:
      b_debug::xxx('unexpected argument',array(cnf_dev));
    }
    
    if (!isset($reply) && cnf_dev) b_debug::internalError("unexpected case ".var_export($what,True));
    if (cnf_show_a||cnf_show_acl) myPear::DEBUG(__METHOD__."($what) <em>".($reply?'YES':'NO')."</em>");
    bTiming()->cpu();
    return $reply;
  }
}

// if we are not in the CMS environment...
if (file_exists($f='../myPear/config.inc'))  require_once $f;

/*
 *
 */
function start_VM(){
  static $dejaVu = 0;
  if (!VM::$isReady && !$dejaVu++){
    b_debug::xxx(starting,array(2,'redText',cnf_dev));
    bTiming()->cpu(__FUNCTION__);

    VM::start();
    VM::_MENU()->start();

    if (is_object(VM::$e)){
      // From 2016 we are administrated by SU :-/
      $swap = mktime(12,0,0,1,1,2016);
      VM::$forms2admin= (VM::$e->getValue('e_start') < $swap
			 ? array('KTH_Payment_details',
				 'KTH_Application_re_Travel_Scholarship')
			 : array('SU_9009',
				 'SU_Travel_reimbursement_form'));
      VM::$forms2send = (VM::$e->getValue('e_start') < $swap
			 ? array('KTH_Payment_details',
				 'KTH_Application_re_Travel_Scholarship')
			 : array('SU_Travel_reimbursement_form'));
    }
    VM::$isReady = True;
    bTiming()->cpu();
    b_debug::xxx(exiting,array(2,'redText',cnf_dev));
  }
}

/*
 *
 */
function VM__menu(){
  static $_MENU = Null;
  if ($_MENU === Null){
    require_once(dirname(__FILE__).'/includes/vm_updates.inc');
    require_once(dirname(__FILE__).'/includes/APImenu_vm.inc');
    $_MENU = new APImenu_vm();
  }
  return $_MENU;
}

/*
 *
 */
function VM_access(){
  static $_ACCESS = Null;
  if ($_ACCESS === Null){
    require_once(dirname(__FILE__).'/includes/vm_updates.inc');
    require_once(dirname(__FILE__).'/includes/APIaccess_vm.inc');
    $_ACCESS = new APIaccess_vm();
  }
  return $_ACCESS;
}

/*
 *
 */
function VM_reimbursable_visits(){
  return array_diff(array_keys(VM::$reimbursable_visits),array('checkbox'));
}


/*
 *
 */
function VM_mailer(){
  static $mailer = Null;
  if ($mailer === Null){
    start_VM();
    locateAndInclude('bMailer_vm');
    $mailer = new bMailer_vm();
    bMailer_vm::$wrapper = __function__;
  }
  return $mailer;
}


/*
 * Get/Set the visit policies
 * The visits of VISIT_TYPE_PROGRAM keep the policies in the Event class,
 * the other visit types keep the policies in the database as variables
 */
function VM_visit_policies($visit_type,$new_value=Null,$e=Null){
  if (empty($visit_type)) b_debug::internalError("Empty visit_type arg");
  
  if ($visit_type == VISIT_TYPE_PROGRAM){
    
    if (!is_object($e))        $e = $new_value;
    if (!is_object($e))        $e = VM::$e;
    if (!($e instanceof bForm_vm_Event))      b_debug::internalError("Empty event arg");
    $e->set_e_v_policy($new_value);
    $reply = $e->get_e_v_policy();
    
  }else{
    
    // Update the variable
    $var_id = "v_policy_$visit_type";
    if (is_array($new_value)) b_vars::set($var_id,$new_value,VM_MODULE);
    
    // Initialise the variable storage if not yet done
    if (!b_vars::get($var_id,VM_MODULE)){
      b_vars::set($var_id,VM::$description[$visit_type]['p'],VM_MODULE);
    }
    $reply = b_vars::get($var_id,VM_MODULE);
  }

  // Check for the de-commissioned policies
  if ($bogus=array_diff($reply,array_keys(VM::$v_policies))){
    $reply = array_diff($reply,$bogus);
    b_debug::xxx("DROP BOGUS policy ".join(', ',$bogus));
  }

  if (cnf_dev){
    foreach($reply as $p) $reply_P[$p] = VM::$v_policies[$p]['d'];
    //    b_debug::xxx($reply_P,array(True,'blueText',2));
  }
  return $reply;
}


/*
 * myOrg bList wrappers
 */
function VM_projects()      { return bList::getListInstance(myOrg_ID,'bList_vm_projects'); }

function VM_hutCodes()      { return bList::getListInstance(myOrg_ID,'bList_vm_hutCodes'); }

function VM_cotenants()     { return bList::getListInstance(myOrg_ID,'bList_vm_cotenants'); }

function VM_agendaEvents()  { return bList::getListInstance(myOrg_ID,'bList_vm_agendaEvents'); }

function VM_socialEventRates($id) { return bList::getListInstance($id,'bList_vm_socialEventRates'); }


/*
 * Triggers for caching the web page
 */
function vm_b_cache_file(){
  $triggers = array();
  if (defined('myOrg_ID')){
    start_VM();
    $triggers = array_merge($_POST,
			    $_GET,
			    $_FILES,
			    array('rank'=>VM_access()->getRank(),
				  'e'   =>VM::e_ID(),
				  'date'=>b_time::short_date(),
				  ));
  }
  return $triggers;
}

/*
 * The VM module is specific for Nordita (see http://www.nordita.org), for the time being we don't see other customers...
 */
function vm_select_organization(){
  $reply = 'nordita';
  b_debug::xxx($reply);
  return $reply;
}

function VM_managers()       { start_VM(); return myPear::_UNIT(RANK_vm_manager,  VM_MODULE); }

function VM_administrators() { start_VM(); return myPear::_UNIT(RANK_vm_program_coordinator,   VM_MODULE); }

function VM_observers()      { start_VM(); return myPear::_UNIT(RANK_vm_observer, VM_MODULE); }

function VM_endorsers()      { start_VM(); return myPear::_UNIT(RANK_vm_endorser, VM_MODULE); }

function VM_organizers($e_id){ start_VM(); return myPear::_UNIT(RANK_vm_organizer,VM_MODULE,$e_id); }

function VM_unit($rank,$module=VM_MODULE,$org=myOrg_ID){ 
  switch($rank){
  case RANK_vm_manager:            return VM_managers();
  case RANK_vm_program_coordinator:return VM_administrators();
  case RANK_vm_observer:           return VM_observers();
  case RANK_vm_endorser:           return VM_endorsers();
  case RANK_vm_organizer:          return VM_organizers(VM::$e);
  default:  b_debug::internalError("Unexpected rank '$rank'");
  }
}
