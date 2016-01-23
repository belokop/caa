<?php
/**
 * Visitors Manager (VM) configuration file
 */
$releaseDate = '2016-01-23';
define('VM_MODULE'   , 'vm');
define('VM_VERSION','5.78');

//
// transition problems...
//
define('modal_sendmail_dialog',cnf_dev||cnf_demo);
define('send_automatically_infoMail_to_registrants',cnf_dev);
// Office space distribution
define('distribute_offices',cnf_def);
// Final welcome mail settings
define('send_finalWelcomeEmail_to_registrants',cnf_dev||cnf_demo);
define('include_personalPage_in_finalWelcomeEmail', send_finalWelcomeEmail_to_registrants && cnf_dev||cnf_demo);
define('include_accommodation_in_finalWelcomeEmail',send_finalWelcomeEmail_to_registrants && cnf_dev||cnf_demo);
define('include_payForms_in_finalWelcomeEmail', False); //     send_finalWelcomeEmail_to_registrants && cnf_dev);
define('send_denialEmail_to_registrants',cnf_dev||cnf_demo); 
define('see_mails_exchange_with_attenders',(send_automatically_infoMail_to_registrants ||
					    send_finalWelcomeEmail_to_registrants ||
					    send_denialEmail_to_registrants));

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

/*
 * VM root directory
 */
define('vm_root', dirname(__FILE__).'/');

/*
 * Roles played in VM
 */
define('RANK_vm_manager'   , 90); // the same rights as superuser, but for VM_MODULE only
define('RANK_vm_booker'    , 30);
define('RANK_vm_observer'  , 28);
define('RANK_vm_endorser'  , 27);
define('RANK_vm_organizer' , 26);
define('RANK_vm_visitor'   , 12); // invited guest, same rights as RANK_vm_registrant
define('RANK_vm_registrant', 10); // event registrant, same rights as RANK_vm_visitor.

/*
 * Event & visit policies
 */
define('VM_E_endorsed',      1); // Flag 'event is endorsed and locked, no changes allowed'. 
define('VM_E_archived',      2); // Flag 'event is archived'. Has precedence over other locks
define('VM_E_endorsmentSent',8); // Flag 'mail to organizers "event is endorsed" is sent'

define('VM_V_payPerdiem' ,1);        // Flag 'pay per-diem'
define('VM_V_payTrip',2);            // Flag 'pay trip'
define('VM_V_denialMailSent',4);     // Flag 'application denial sent'
define('VM_V_welcomeMailSent',8);    // Flag 'application approval sent'
define('VM_V_invoiceSent',16);       // Flag 'invoice for the rent sent'
define('VM_V_scrtrInfoSent',32);     // Flag 'guest invitation communicated'
define('VM_V_payAccommodation',64);  // Flag 'cover living expenses'
define('VM_V_provideOffice',128);    // Flag 'assign office for the visit'
define('VM_V_infoMailSent',256);     // Flag 'informal informational mail sent'
define('VM_V_enMailSent',512);       // Flag '"Event is endorsed" mail to organizer sent'
define('VM_V_payOther',2048);        // Flag 'pay other expenses, (whatever "other expenses" mean)' 
define('VM_V_rdlMailSent',4096);     // Flag 'registration-deadline mail to organizer sent'

/*
 * Some policies are "human defined", like "to pay OR not to pay",
 * the other are "auto-controlled" like "was the button pushed".
 * This flag has to be set in the VM::$v_policies array - item 't', i.e. "type"
 */
define('auto_maintained',     'auto'); 

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
define('VM_booking_zapas', 30); // in days

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
				   LODGING_OA       =>('do not need|will arrange|my way|on my own|pay myself'
						 //.'|myself|accommodation arranged|other|made accommodation request upon'
						 ),
				   // 'shared_lodging' =>('shared accommodation'),
				   'default_lodging'=>join('|',array('I apply for an apartment paid for by Nordita'
								     )),
				   );
if (cancel_LODGING_REQ_ONLY) unset($GLOBALS['LODGING_FINDER'][LODGING_REQ_ONLY]);

//
//
define('VM_TEXT_oo',  'Own Office');

define('OFFICE_AUTO_ALLOCATABLE', 'visitors'); // offices with this status are auto-allocated by the bCover
define('AP_AUTO_ALLOCATABLE','auto allocate'); // apartments with this status are auto-allocated by the bCover
define('AP_SHARABLE','shared');                // apartments might be shared 

define('ICON_edit_event',       11);
define('ICON_edit_apartment',   13);
define('ICON_see_agenda',       14);
define('ICON_edit_visit',       17);
define('ICON_mail_tenant',      18);
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
define('btn_deleteAp',     'delete unused apartment');
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
 * Duties which bookers might have
 */ 
define('DUTY_unlock_events','UNLOCK');
define('DUTY_reimbursement','PAY');
define('DUTY_programs',VISIT_TYPE_PROGRAM);
define('DUTY_guests',  VISIT_TYPE_COLLABORATION);
define('DUTY_update_rooms','ROOMS'); 

define('VM_wordOtheExp','other expenses');

define('VM_maybe',(cnf_dev ? 'maybe' : 'no'));

/*
 * Describe the VM module
 */
b_reg::add_module(VM_MODULE,
		  array('v' =>VM_VERSION,          // module version number
			'tm'=>'Visitors',          // module name for the "top" horizontal menu
			'r' =>$releaseDate,        // module release date
			'd' =>'Visitors Manager',  // module name for the "navigation" menu
			'i' =>'i-airplane',        // module favicon 
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
			'classes'=>array('bForm_vm_Hut'              =>array('t'=>'abs_huts',   
									     'i'=>'hut_id'),
					 'bForm_vm_Apartment'        =>array('t'=>'abs_apartments',
									     'i'=>'a_id'),
					 'bForm_vm_Expenses'         =>array('t'=>'abs_expenses', 
									     'i'=>'exp_id'),
					 'bForm_vm_Office'           =>array('t'=>'abs_offices', 
									     'i'=>'o_id'),
					 'bForm_vm_Event'            =>array('t'=>'abs_events',
									     'i'=>'e_id'),
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
									     'dd'=>'VCB code'),
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
				       RANK_vm_booker     => array('class'=>'bUnit_vm_bookers',
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
  static $forms2send  = array('Payment_details',
			      'Application_re_Travel_Scholarship',
			      );

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
   * Reimbursements, defaults for "to pay or not to pay" 
   */
  static $reimbursable_visits = array(VISIT_TYPE_PROGRAM       =>array('d'            =>'Program/Conference attenders',
								       VM_V_payPerdiem=>'no',
								       VM_V_payOther  => VM_maybe),
				      VISIT_TYPE_COLLABORATION =>array('d'            =>'Invited researchers',
								       VM_V_payPerdiem=> VM_maybe,
								       VM_V_payOther  => VM_maybe),
				      VISIT_TYPE_OTHER         =>array('d'            =>'Other visits',
								       VM_V_payPerdiem=> VM_maybe,
								       VM_V_payOther  => VM_maybe),
				      'checkbox'               =>array(VM_V_payOther));
  
  static $reimbursements = array(VM_V_payTrip,
				 VM_V_payOther,
				 VM_V_payPerdiem,
				 VM_V_payAccommodation,
				 );
  
  /*
   * Visit policy bits 
   */
  static $v_policies   = array(VM_V_payPerdiem       => array('d'=> VM_wordPerdiem_pay,
							      'i'=> VM_wordPerdiem),
			       VM_V_payTrip          => array('d'=>'To pay the trip?',
							      'i'=>'trip'),
			       VM_V_payOther         => array('d'=>'To cover other expenses?'),
			       
			       VM_V_payAccommodation => array('d'=>'To pay accommodation?',
							      'i'=>'i-hotel',
							      't'=> auto_maintained),
			       VM_V_provideOffice    => array('d'=>'To allocate office?',
							      'i'=>'trip'),
			       VM_V_infoMailSent     => array('d'=>'Was a general info mail sent?',
							      't'=> auto_maintained),
			       VM_V_denialMailSent   => array('d'=>'Was the denial letter sent?',
							      't'=> auto_maintained),
			       VM_V_welcomeMailSent  => array('d'=>'Was the approval letter sent?',
							      't'=> auto_maintained),
			       VM_V_invoiceSent      => array('d'=>'Was the invoice sent?',
							      'i'=>'i-invoice',
							      't'=> auto_maintained),
			       VM_V_scrtrInfoSent   => array('d'=>'Was the guest invitation communicated?',
							     't'=> auto_maintained),
			       VM_V_rdlMailSent     => array('d'=>'Was the appl. deadline communicated?',
							     't'=> auto_maintained),
			       VM_V_enMailSent      => array('d'=>'Was the endorsement communicated?',
							     't'=> auto_maintained),
			       );
  
  static $e_policies   = array(VM_E_archived       => array('d'=>'Is the event archived?',
							    'i'=>'i-wallet'),
			       VM_E_endorsed       => array('d'=>'Is the event endorsed & changes are locked?',
							    'i'=>'i-lock'),
			       VM_E_endorsmentSent => array('d'=>'Was the endorsement communicated?',
							    't'=> auto_maintained),
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
								   'p'=>array(VM_V_payTrip,VM_V_payAccommodation,// VM_V_payPerdiem, 
									      VM_V_scrtrInfoSent)),
				   VISIT_TYPE_PROGRAM      =>array('d'=>'Conference attender',
								   'i'=>'a_conf',
								   'f'=>'debit',
								   'p'=>array(VM_V_payTrip,VM_V_payAccommodation, 
									      VM_V_denialMailSent,VM_V_welcomeMailSent,VM_V_infoMailSent,
									      VM_V_rdlMailSent,VM_V_enMailSent)),
				   VISIT_TYPE_POSSIBLE     =>array('d'=>'Virtual visit, used to estimate budget',
								   'i'=>'i-avatar',
								   'f'=>'debit',
								   'p'=>array()),
				   VISIT_TYPE_RENT         =>array('d'=>'Tenant renting out apartment, to be invoiced',
								   'i'=>'bb-home',
								   'f'=>'credit',
								   'p'=>array(VM_V_invoiceSent)),
				   VISIT_TYPE_OTHER        =>array('d'=>'Other',
								   'i'=>'a_meeting',
								   'f'=>'debit',
								   'p'=>array(VM_V_payTrip,VM_V_payAccommodation, // VM_V_payPerdiem,VM_V_payOther,
									      VM_V_scrtrInfoSent)),
				   VISIT_OUT_OF_SCOPE      =>array('d'=>'Out of VM scope',
								   'i'=>'a_meeting',
								   'p'=>array()),

				   // Accommodation options
				   LODGING_OA              =>array('d'=>'Own accommodation, no fin. support'),
				   LODGING_BR              =>array('d'=>'Break in the visit, no fin. support'),
				   );

  static public $visit_status = array(STATUS_PENDING => 'pending',
				      STATUS_YES     => 'approved',
				      STATUS_NO      => 'refused');

  /*
   * Some useful icons
   */
  static public $icons     = array(ICON_edit_apartment=> array('d'=>'update the apartment information',
							       'i'=>'i-edit'),
				   ICON_edit_event    => array('d'=>'update the event information',
							       'i'=>'i-edit'),
				   ICON_edit_visit    => array('d'=>'update accommodation for this tenant',
							       'i'=>'bb-home'),
				   ICON_mail_tenant   => array('d'=>'send an e-mail to the tenant',
							       'i'=>'mail'),
				   ICON_see_agenda    => array('d'=>'see the event in the agenda system',
							       'i'=>'nordita'));
  static public $e         = Null;       // Selected event class
  static public $visitTypes=array();
  static public $formDummy = Null;
  static public $hut_bi    = 'hut_bi';   // "Built-in" hut
  static public $REQ_ONLY  = 'REQ_ONLY'; // "request only" apartment
  static public $OA        = 'OA';       // "own accommodation" apartment

  static public $auto_maintained = array(); // Filled from the previous array
  
  static public function _MENU(){
    start_VM();
    return self::_fromCache('APImenu_vm'); 
  } 

  static function start(){
    static $dejaVu = 0;
    if (!$dejaVu++){
      
      // Needed only for the menu build, since it is built without a context
      bForm_Avatar::set_context(bAuth::$av,VM_MODULE);
      
      // Initialise the access control mechanism
      VM_access();

      // Fill auto_maintained array      
      foreach(self::$v_policies as $policy=>$data){
	if (@$data['t'] === auto_maintained) self::$auto_maintained[] = $policy;
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
    if (!is_object(VM::$hut_bi)){
      foreach(array(// "the build-in hut" 
		    self::$hut_bi  =>array('class' =>'bForm_vm_Hut',
					   'key'   =>'hut_code',
					   'fields'=>array('hut_name'=>'Built-in',
							   'hut_code'=>HUT_BI)),
		    // "own accommodation" apartment
		    self::$OA      =>array('class' =>'bForm_vm_Apartment',
					   'key'   =>'a_name',
					   'fields'=>array('a_name'    => LODGING_OA_TXT,
							   'a_price'   => 0.01,
							   'a_capacity'=> 999)),
		    // request-only apartment
		    self::$REQ_ONLY=>array('class' =>'bForm_vm_Apartment',
					   'key'   =>'a_name',
					   'fields'=>array('a_name'     => LODGING_REQ_ONLY_TXT,
							   'a_price'    => 0.001,
							   'a_capacity' => 999))) as $instance=>$args){
	locateAndInclude($class_name=$args['class'],'fatal');
	
	// Hook the "build-in apartments" to the "build-in hut"
	if ($class_name == 'bForm_vm_Apartment') $args['fields']['a_hutid'] = self::$hut_bi->ID;
	
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
    if ($e_ID===Null) $e_ID = (int)b_cnf::get('vm_eventSelector');
    else                           b_cnf::set('vm_eventSelector',$e_ID);

    if ((int)$e_ID > 0){
      if (!is_object(self::$e))	self::$e = myPear::getInstance('bForm_vm_Event',$e_ID);
      if ($e_ID != ($id=self::$e->ID)){
	self::$e = myPear::getInstance('bForm_vm_Event',$e_ID);
	myPear::WARNING_T("Change VM::e_ID $id -> $e_ID ".self::$e->name());
      }
    }else{
      self::$e = Null;
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
    if ($e === Null) $e = VM::$e;
    $reply = (bool)(is_object($e) && bList_vm_agendaEvents::eventOutOfScope($e->getValue('e_code')));
    bTiming()->cpu();
    return $reply;
  }

  static function isArchived($e=Null){
    if ($e === Null)  $e = VM::$e;
    return (bool)(is_object($e) && $e->isArchived());
  }

  static function isEventEndorsed($e=Null){
    if (is_null($e)) $e = VM::$e;
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

    if (VM_manager_here)       return True;
    if (superUser_here)        return True;
    if (!is_object(bAuth::$av))return False;
    
    bTiming()->cpu(__FUNCTION__);
    switch($what){
      
    case 'request_visits': 
      $reply = (VM_employee_here || VM_lt_visitor_here);
      break;
      
    case 'setup_SE':
      $reply = (VM_booker_here || VM_organizer_here);
      if (VM::isArchived())       $reply = False;
      break;
      
    case 'book_ah': 
      if (!($reply = (VM_booker_here || (VM_organizer_here && is_object(VM::$e) && bAuth::$av->isOrganizer(VM::$e->ID))))){
	$reply = (is_object($visit=$argv) && $visit->isWritable());
      }
      break;
      
    case 'book':
      $reply = (VM_booker_here || (is_object($visit=$argv) && $visit->isWritable()));
      break;
      
    case 'select_budget':  
      $reply = VM_administrator_here;
      break;
      
    case 'send_info_mail':  
      if (VM::isArchived())$reply = False;
      else                 $reply = (!VM::isEventEndorsed() && (VM_observer_here || VM_organizer_here));
      break;
      
      case 'send_finalWelcomeEmail':  
	$reply = VM_booker_here || self::hasRightTo('endorse_event');
	// $reply = cnf_dev;	myPear::WARNING_T("pending problem");
	break;
	
    case 'endorse_event':
      if (VM::isArchived()) $reply = False;
      else                  $reply = bAuth::$av->isEndorser(VISIT_TYPE_PROGRAM);
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
      $reply = (VM_organizer_here || VM_manager_here) && !VM::isArchived();
      break;
      
    case 'edit_visit':
      if (is_object($visit=$argv)){
	$reply = ($visit->isHost() || self::hasRightTo('approve_guest_invitation',$visit) || self::hasRightTo('approve_registrant_application'));
	break;
      }
      $e = Null;
    case 'setup_reimbursement':
      if (!isset($e)) $e = VM::$e;
      $reply = (!VM::isEventEndorsed($e) && ( ((int)@$argv['v_eid'] && VM_organizer_here) ||
					      (@$argv['v_host_avid'] == bAuth::$av->ID)) );
      break;
      
    default:
      b_debug::xxx('unexpected argument',array(cnf_dev));
    }
    
    if (!isset($reply) && cnf_dev) b_debug::internalError("unexpected case");
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
    b_debug::xxx(starting);
    bTiming()->cpu(__FUNCTION__);
    VM::start();
    VM::_MENU()->start();
    VM::$isReady = True;
    bTiming()->cpu();
    b_debug::xxx(exiting);
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
 * The visits associated with an event keep the policies in the Event class,
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

  // Check for the decommissioned policies
  if ($bogus=array_diff($reply,array_keys(VM::$v_policies))){
    $reply = array_diff($reply,$bogus);
    b_debug::xxx("DROP BOGUS policy ".join(', ',$bogus));
  }

  if (cnf_dev){
    foreach($reply as $p) $reply_P[$p] = VM::$v_policies[$p]['d'];
    b_debug::xxx($reply_P,cnf_dev);
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
 * The VM module is specific for Nordita, for the time being we don't see other customers...
 */
function vm_select_organization(){
  $reply = 'nordita';
  b_debug::xxx($reply);
  return $reply;
}

function VM_managers()       { start_VM(); return myPear::_UNIT(RANK_vm_manager,  VM_MODULE); }

function VM_bookers()        { start_VM(); return myPear::_UNIT(RANK_vm_booker,   VM_MODULE); }

function VM_observers()      { start_VM(); return myPear::_UNIT(RANK_vm_observer, VM_MODULE); }

function VM_endorsers()      { start_VM(); return myPear::_UNIT(RANK_vm_endorser, VM_MODULE); }

function VM_organizers($e_id){ start_VM(); return myPear::_UNIT(RANK_vm_organizer,VM_MODULE,$e_id); }

function VM_unit($rank,$module=VM_MODULE,$org=myOrg_ID){ 
  switch($rank){
  case RANK_vm_manager:   return VM_managers();
  case RANK_vm_booker:    return VM_bookers();
  case RANK_vm_observer:  return VM_observers();
  case RANK_vm_endorser:  return VM_endorsers();
  case RANK_vm_organizer: return VM_organizers(VM::$e);
  default:  b_debug::internalError("Unexpected rank '$rank'");
  }
}
