<?php

// Load dependencies
if (function_exists('locateAndInclude')){
  locateAndInclude('APImenu_ea','fatal');
  require_once(dirname(__FILE__).'/vm_functions.inc');
  require_once(dirname(__FILE__).'/vm_updates.inc');
}elseif (file_exists($f=str_replace('/vm/','/ea/',__FILE__))){
  require_once $f;
}
		    
/*
 * Define all the tabs/menus used in VM module
 */
APItabs::define_tabs(array(
			   'tab_vm_home'              => 'My account',
			   'tab_vm_reg'               => 'My visit',
			   'tab_vm_vmatrix'           => 'Visits',
			   'tab_vm_tenants'           => 'Cotenants',
			   'tab_vm_vis_allVisits'     => 'Visits',

			   'tab_vm_acc'               => 'Accommodation', 
			   'tab_vm_acc_add'           => 'Add apartment',
			   'tab_vm_acc_assign'        => 'Auto accommodate homeless',
			   'tab_vm_acc_huts'          => 'Buildings',
			   'tab_vm_acc_rooms'         => 'Rooms',
			   'tab_vm_acc_usage'         => 'Apartments usage',

			   'tab_vm_adm'               => 'Administer VM',
			   'tab_vm_adm_acc'           => myOrg_nameS.' accommodation', 
			   'tab_vm_adm_projects'      => myOrg_nameS.' financial projects',
			   'tab_vm_adm_sc_setup'      => myOrg_nameS.' reimbursement rates',
			   'tab_vm_adm_se_setup'      => myOrg_nameS.' social events',
			   'tab_vm_adm_unitOrganizers'=> myOrg_nameS.' event organizers',
			   'tab_vm_adm_acl'           => 'ACL',
			   'tab_vm_adm_agendaList'    => 'Edit agenda list',
			   'tab_vm_adm_dup'           => 'Duplicate avatars',
			   'tab_vm_adm_fixClashes'    => 'Fix clashes',
			   'tab_vm_adm_sync'          => 'Agenda sync',

			   'tab_vm_end'               => 'Events to endorse',
			   'tab_vm_end_amatrix'       => 'Who lives where',
			   'tab_vm_end_budget'        => 'See/Approve budget',
			   'tab_vm_end_mails'         => 'Mails exchange with applicants',
			   'tab_vm_end_omatrix'       => 'Who sits where Matrix',
			   'tab_vm_end_vlist'         => 'A-Z applicant lists',
			   'tab_vm_end_vmatrix'       => 'Who arrives when',

			   'tab_vm_events'            => myOrg_nameS.' events',
			   'tab_vm_events_see'        => myOrg_nameS.' events',
			   'tab_vm_events_add'        => 'Add event',

			   'tab_vm_guests'            => 'Visits outside '.myOrg_nameS.' events',
			   'tab_vm_guests_addVisit'   => 'Register new guest',
			   'tab_vm_guests_budget'     => 'People I invite',
			   'tab_vm_guests_budgetBP'   => 'Visits budget summary (by projects)',
			   'tab_vm_guests_to_approve' => 'Resolve pending approvals',

			   'tab_vm_ogz'               => 'Events I organize',
			   'tab_vm_ogz_acc'           => 'Accommodation options',
			   'tab_vm_ogz_amatrix'       => 'Who lives where',
			   'tab_vm_ogz_budget'        => 'Budget estimation',
			   'tab_vm_ogz_dup'           => 'Duplicate applications',
			   'tab_vm_ogz_mails'         => 'Mails exchange with applicants',
			   'tab_vm_ogz_omatrix'       => 'Who sits where Matrix',
			   'tab_vm_ogz_photos'        => 'Photos of attenders',
			   'tab_vm_ogz_se_setup'      => 'Social events',
			   'tab_vm_ogz_sync'          => 'Synchronize my events with "Agenda"',
			   'tab_vm_ogz_vlist'         => 'Approve & Accommodate applicants',
			   'tab_vm_ogz_vmatrix'       => 'Who arrives when',

			   'tab_vm_vis'               => 'Visits within events',
			   'tab_vm_vis_acc'           => 'Accommodation options',
			   'tab_vm_vis_badges'        => 'Print badges',
			   'tab_vm_vis_budget'        => 'Budget estimation',
			   'tab_vm_vis_export'        => 'Export tenants Excel files',
			   'tab_vm_vis_list'          => 'List of attenders',
			   'tab_vm_vis_mails'         => 'Mails exchange with attenders',
			   'tab_vm_vis_photos'        => 'Print photos',
			   'tab_vm_vis_se_setup'      => 'Social events',
			   'tab_vm_vis_sok'           => 'Search',
			   'tab_vm_vis_vmatrix'       => 'Who arrives when',
			   'tab_vm_vis_welcome'       => 'Welcome visitor',
			   ));

/*
 * Check that the given person is related to VM module:
 * - has registered trips 
 * - is a future visitor
 * - EA member
 */
function vm_after_search_check($av_id,$_after_search=''){
  $our_man = (stripos($_after_search,'vm_Visit') !== False);
  if (!$our_man){
    $q = myPear_db()->query("SELECT v_avid FROM abs_visits WHERE v_avid = '$av_id'");
    $our_man = (myPear_db()->num_rows($q) > 0);
  }
  // EA member?
  if (!$our_man){
    $ea = new bUnit_ea_members(myOrg_ID);
    $our_man = $ea->isMember($av_id);
  }
  return $our_man;
}


/*
 *
 */
if (function_exists('locateAndInclude')) locateAndInclude('APImenu_ea');
else require_once drupal_get_path('module','ea').'/includes/APImenu_ea.inc';

class APImenu_vm extends APImenu_ea{
  var $debug = True;
  var $title  = 'Visits';
  var $q      = VM_MODULE;
  
  var $static_menu   = array(
			     divider,
			     tab_vm_guests    => array(tab_vm_guests_to_approve,
						       tab_vm_guests_addVisit,
						       tab_vm_guests_budget =>divider,
						       tab_vm_guests_budgetBP,
						       ),
			     tab_vm_home      => array(),
			     tab_vm_reg       => array(),
			     divider,
			     tab_vm_vis       => array(tab_vm_ogz_vmatrix,
						       tab_vm_ogz_omatrix,
						       tab_vm_ogz_amatrix,
						       tab_vm_vis_list, 
						       tab_vm_vis_budget, 

						       tab_vm_vis_se_setup=>divider,
						       tab_vm_vis_acc, 

						       tab_vm_vis_mails   =>divider,
						       tab_vm_vis_export,
						       tab_vm_vis_badges, 
						       tab_vm_vis_photos,
						       // tab_vm_vis_welcome, // tab_vm_vis_sok,
						       ),
			     divider,
			     tab_vm_end       => array(tab_vm_end_vlist,
						       tab_vm_end_mails,
						       tab_vm_end_budget,
						       tab_vm_end_vmatrix,
						       tab_vm_end_omatrix,
						       tab_vm_end_amatrix,
						       ),
			     divider,
			     tab_vm_ogz       => array(tab_vm_ogz_vlist,
						       tab_vm_ogz_budget,

						       tab_vm_ogz_se_setup=>divider,
						       tab_vm_ogz_acc,

						       tab_vm_ogz_mails=>divider,
						       						       
						       tab_vm_ogz_sync,

						       tab_vm_ogz_photos,
						       tab_vm_ogz_vmatrix,
						       tab_vm_ogz_amatrix,
						       tab_vm_ogz_omatrix,

						       tab_vm_ogz_dup,
						       ),
			     tab_ea_off       => array(
						       tab_ea_off_omatrix,
						       tab_ea_off_list,
						       tab_ea_off_desk_labels,
						       tab_ea_off_door_labels,
						       tab_ea_off_assign,
						       tab_ea_off_wipe, 
						       ),
			     tab_vm_vmatrix   => array(),
			     divider,
			     // Replace this whole tree by one button "tab_vm_adm_acc" below...
			     //tab_vm_acc       => array(      
			     //			       tab_vm_acc_rooms, tab_vm_acc_assign, tab_vm_acc_huts, tab_vm_acc_add, tab_vm_acc_usage,
			     //			       ),
			     tab_vm_adm       => array(
						       tab_vm_adm_acc,
						       tab_vm_adm_se_setup,
						       tab_vm_adm_sc_setup,
						       tab_vm_adm_projects, 
						       // tab_vm_events_add, 

						       tab_vm_events_see =>divider,
						       tab_vm_adm_unitOrganizers,

						       tab_vm_adm_sync   =>divider,
						       tab_vm_adm_agendaList,

						       tab_vm_adm_dup    =>divider,
						       tab_vm_ogz_dup,
						       tab__mergeAV,

						       tab__templates    =>divider,
						       tab__tasks,
						       tab__syslog,

						       tab_vm_ogz_vmatrix=>divider,
						       tab_vm_ogz_omatrix,
						       tab_vm_ogz_amatrix,
						       //tab_vm_adm_acl,tab_vm_adm_fixClashes,   // too early...
						       ),
			     );
  
  function __construct(){
    APImenu::set_context(VM_MODULE);
    parent::__construct();
  }


  /*
   * Start the VM module
   */
  function start(){
    static $dejaVu = 0;    
    if (!$dejaVu++){
      
      foreach (array('bForm_Avatar_vm',
		     'bForm_vm_Room',
		     ) as $object) locateAndInclude($object);
      
      /*
       * Set the current event environment, 
       * decide what to show when the page is visited for the first time
       */
      if (isset($_REQUEST['vm_eventSelector'])){
	if ($_REQUEST['vm_eventSelector'] <= 0)  $_REQUEST['vm_eventSelector'] = EVT_ANY;
	VM::e_ID($_REQUEST['vm_eventSelector']);
      }
      
      $events = array_keys(self::list_of_events());
      $e_id = VM::e_ID();
      if ($e_id == EVT_ANY && !in_array(EVT_ANY,$events)) unset($e_id);
      if (empty($e_id)){
	if (empty($events)) $e_id = 0;
	else                $e_id = $events[0];
	VM::e_ID($e_id);
      }
      
      $margin = 3 * 86400; // a few days margin before&after the event
      if (VM::e_ID() > 0){
	// Event dates have precedence over vm_day1 / vm_day9
	list($day1,$day9) = array(VM::$e->getValue('e_start')-$margin,
				  VM::$e->getValue('e_end')  +$margin);
      }else{
	$day1 = @$_REQUEST['vm_day1'];
	$day9 = @$_REQUEST['vm_day9'];
      }
      
      // limit the graph size by something reasonable, say a year
      $day1 = b_time::txt2unix($day1);
      $day9 = b_time::txt2unix($day9);
      if (empty($day1))   $day1 = time() - $margin;
      if (empty($day9) || $day9<=$day1) $day9 = $day1 + CONST_ical_cells_to_show * 86400;
      $day9 = min($day9, $day1 + 180 * 86400);

      // Flag "initialization of date/time is done",
      define('vm_day1',$day1);
      define('vm_day9',$day9);

      // used by bIcal_matrix
      b_cnf::set(VM_MODULE.'_day1',vm_day1);
      b_cnf::set(VM_MODULE.'_day9',vm_day9);
      
      /*
       * well...
       */
      b_cnf::set("vm_seeOldPrograms", False);
    }
  }
  
  /*
   *
   */
  function access($tab,$dynamicab='') {
    
    $this->dbg();
    $this->start();

    if (!self::_used_by_myOrg(VM_MODULE)){
      // Organization restrictions
      $reply = False;
    }elseif ((b_reg::$current_module === VM_MODULE) && ($_GET['q'] !== VM_MODULE) && !bAuth()->loginPrompt()){
      // Need authorization to see the levels deeper then 1
      $reply = False;
    }elseif (($reply = $this->access_callback($tab))===Null){
      
      switch ($tab){

      case VM_MODULE:
	$reply = True;
	break;
	
      case tab__templates:
	$reply = superUser_here;
	break;

      case  tab__mergeAV:
      case  tab_vm_adm_dup:
      case  tab_vm_adm_fixClashes:
	$reply = VM_manager_here;
	break;
	
      case tab_vm_events:
      case tab_vm_events_add:
      case tab_vm_events_see:
	$reply = VM_manager_here;
	break;

      case tab_vm_acc:  // ========================================================================================================= tab_vm_acc
      case tab_vm_acc_add:
      case tab_vm_acc_huts:
      case tab_vm_acc_rooms:
      case tab_vm_acc_usage:
	$reply = False;
	break;

      case tab_vm_adm:  // ========================================================================================================= tab_vm_adm
	$reply = VM_observer_here;
	break;
	
      case tab__tasks:
	$reply = superUser_here;
	break;

      case tab_vm_adm_acc: 
      case tab_vm_adm_agendaList:
	$reply = VM_program_coordinator_here;
	break;
	
      case tab_vm_adm_projects:
      case tab_vm_adm_unitOrganizers:
      case tab_vm_vis_welcome:
	$reply = !VM::outOfScope() && VM_observer_here;
	break;

      case tab_vm_vis_badges:    
	// Use Hans's script instead...
	$reply = False;
	break;

      case tab_vm_ogz:  // ========================================================================================================= tab_vm_ogz
	$reply = VM_organizer_here;
	break;

      case tab_vm_ogz_vlist:
	if (VM::outOfScope() || VM::$e->isArchived()){
	  $reply = False;
	  break;
	}

      case tab_vm_ogz_mails:
	$reply = cnf_dev || (!VM::outOfScope() && 
			     VM::$e->isArchived() && 
			     VM::$e->attenders() && see_mails_exchange_with_attenders && !cnf_dontSendMails_Point);
	break;

      case tab_vm_ogz_photos:
      case tab_vm_ogz_budget:
	$reply = VM::$e->attenders();
	break;

      case tab_vm_ogz_amatrix:
      case tab_vm_ogz_vmatrix:
      case tab_vm_ogz_omatrix:
      case tab_vm_ogz_se_setup:
	$reply = (!VM::$e->isArchived() && VM::$e->attenders() && !VM::outOfScope());
	break;

      case tab_vm_ogz_dup:
	$reply = (VM::e_ID() > 0) && cnf_dev && bForm_Avatar::findDuplicates(" LEFT JOIN abs_visits ON v_avid=av_id WHERE v_eid=".VM::e_ID(),True);
	break;
	
      case tab_vm_ogz:  // ========================================================================================================= tab_vm_ogz
      case tab_vm_end_mails:
	if (!see_mails_exchange_with_attenders || cnf_dontSendMails_Point){
	  $reply = False;
	  break;
	}
	if (VM::e_ID() && VM::$e->getValue('e_end',1) < time()){
	  $reply = False;
	  return $reply;
	}
      case tab_vm_end:
      case tab_vm_end_vlist:
      case tab_vm_end_budget:
      case tab_vm_end_vmatrix:
      case tab_vm_end_amatrix:
      case tab_vm_end_omatrix:
	$reply = 
	  is_object(bAuth::$av) &&
	  !myPear::is_empty(bAuth::$av->eventsToEndorse()) &&
       	  bAuth::$av->isEndorser(VISIT_TYPE_PROGRAM);
	break;
	
      case tab_vm_vis_mails:
	if (!see_mails_exchange_with_attenders){
	  $reply = cnf_dev;
	  break;
	}
	if (VM::e_ID() && VM::$e->getValue('e_end',1) < time()){
	  $reply = False;
	  return $reply;
	}
	$reply = VM_administrator_here && !VM::outOfScope();
	break;

      case tab_vm_adm_sync:
      case tab_vm_ogz_sync:
      case tab_vm_ogz_acc:
	$reply = (VM_program_coordinator_here || VM_organizer_here);
	break;

      case tab_vm_adm_acl:
      case tab_vm_acc_assign:
	$reply = False;
	break;

      case tab_ea_off:  // ========================================================================================================= tab_vm_off
      case tab_ea_off_list:
      case tab_ea_off_omatrix:
      case tab_ea_off_door_labels:
      case tab_ea_off_desk_labels:
      case tab_ea_off_assign:
      case tab_ea_off_wipe:
	$reply = allocate_offices && VM_administrator_here && !VM::outOfScope() && (VM::e_ID()>0) && VM::$e->getPolicy(VM_E_provideOffice);
	break;

      case tab_vm_vis:  // ========================================================================================================= tab_vm_off
	$reply = (bAuth::authenticated() && !bAuth::$av->isOrganizer());
	break;

      case  tab_vm_vis_photos:    
	$reply = True;
	break;
	
      case tab_vm_vis_list:
      case tab_vm_vis_export:
      case tab_vm_vis_acc:
	$reply = !VM::$e->isArchived();
	  break;

      case tab_vm_vis_sok:
      case tab_vm_vis_budget:
	$reply = is_object(VM::$e) && VM_observer_here && !VM::outOfScope();
	break;
	
      case tab_vm_reg:
	$reply = VM_external_registrant_here;
	break;
	
      case tab_vm_home:
	$reply = bAuth::authenticated() && !VM_employee_here;
	break;
	
      case tab_vm_vmatrix:
	$reply = VM_stranger_here;
	break;
	
      case tab__empty:	
	$reply = bAuth::authenticated();
	break;
      
      case tab_vm_vis_vmatrix:
      //$reply = (bAuth::authenticated() && !bAuth::$av->isOrganizer() && !bAuth::$av->isEndorser(VISIT_TYPE_PROGRAM));
	$reply = (bAuth::authenticated() && !bAuth::$av->isOrganizer());
	break;
	
      case tab_vm_adm_sc_setup:
      case tab_vm_adm_se_setup:
	$reply = (!VM::$e->isArchived() && !VM::outOfScope() && VM_manager_here);
	break;

      case tab_vm_vis_se_setup:
	$reply = (!VM::$e->isArchived() && !VM::outOfScope() && VM_administrator_here);
	break;
	
      case tab_vm_guests_budget:
	if ($reply = VM::hasRightTo('request_visits')){
	  $q = myPear_db()->qquery("SELECT * FROM abs_visits WHERE (v_eid = 0 OR v_eid IS NULL) AND v_host_avid = '".bAuth::$av->ID."'",cnf_dev);
	  $reply = (myPear_db()->num_rows($q) > 0);
	}
	break;

      case tab_vm_guests_budgetBP:
	$reply = VM_observer_here || (is_object(bAuth::$av) && bAuth::$av->isEndorser());
	break;

      case tab_vm_guests_to_approve:
	$reply = bAuth::authenticated() && (count(array_keys(VM_endorsers()->toEndorse(bAuth::$av->ID))) > 0);
	break;
	
      case tab_vm_guests:
      case tab_vm_guests_addVisit:
	$reply = VM::hasRightTo('request_visits');
	break;

      default:
	if (!empty($tab)) b_debug::internalError("unpredicted tab \"$tab\"");
	$reply = False;
      }
    }
    //    $this->dbg($reply,True);
    return $reply;
  }
  
  /*
   * Main processing function
   */
  function process($tab,$dynamicTab=''){
    
    vm_init();

    // There was an explicit request to send password. Do it
    if (isset($_GET['mo_once'])){
      $e = myPear::getInstance_new('bForm_vm_Event',$_GET['e_id'],array('strict','fatal'));
      VM_mailer()->remind_organizer($_GET['av_id'],$e,'pwd','send_now');
    }

    bTiming()->cpu(__FUNCTION__);
    //    print bHolder_Logs::wic();
    
    if (isset($_REQUEST['vm_day1'])) b_cnf::set('vm_day1',(b_time::txt2unix($_REQUEST['vm_day1'])));
    if (isset($_REQUEST['vm_day9'])) b_cnf::set('vm_day9',(b_time::txt2unix($_REQUEST['vm_day9'])));
    unset($_REQUEST['vm_day1'],$_GET['vm_day1'],$_GET['vm_day1']);
    unset($_REQUEST['vm_day9'],$_GET['vm_day9'],$_GET['vm_day9']);
    $this->dbg(b_time::period(b_cnf::get('vm_day1'),b_cnf::get('vm_day9')));

    set_time_limit(900);
    $this->dbg();
    
    $this->start();
    $this->set_buildInObjects();

    // Process updates of the rates, if any
    locateAndInclude('bList_vm_reimbursementRates');
    bList_vm_reimbursementRates::process_updates();
    
    // Resolve the problem of host
    if (isset($_POST['v_host_avid']) && !is_numeric($v_host_avid = $_POST['v_host_avid'])){
      if ($av = bForm_Avatar::hook($v_host_avid,$doNotCreate=True))$_POST['v_host_avid'] = $av->ID;
    }
    
    // Preset the "general search" attributes
    myPear_search()->idv('search for visitors');
    $search_args = array('_lookFor' => '', 
			 '_button'  => 'show visits',
			 '_header'  => 'Search for visitors',
			 '_join'    => 'abs_visits on av_id:v_avid',
			 '_where'   => 'v_avid>0',
			 '_after_search'=> 'tab_force=tab_vm_vis_allVisits&av_id=AV_ID');
    
    /*
     * Process the request if the configuration is completed, 
     * i.e. if the database, SMTP, etc. are ready
     */
    $tab = $this->processTemplates($tab,$search_args);
    switch ($tab){
      
    case Null:     
      // the processing is done by '$this->processTemplates($tab)', nothing else to do...
      break; 

    case tab_vm_home:
      bAuth::$av->getForm();
      break;
      
    case tab_vm_reg:
      $this->eventSelector();
      $v = new bForm_vm_Visit('v_avid='.bAuth::$av->ID.' AND v_eid="'.VM::$e->ID.'"');
      $v->getForm();
      break;
      
    case VM_MODULE:
    case tab__empty:
      if(is_object(bAuth::$av)){
	if    (VM_organizer_here) $m = tab_vm_ogz;
	elseif(VM_endorser_here)  $m = tab_vm_end;
	else                      $m = tab_vm_guests;
	if (VM::_MENU()->access($m)){
	  bTiming()->cpu();
	  $this->dbg(sprintf('Redirect %s --> %s',$tab,$m));
	  b_url::header(b_url::same('?q='.join('/',array(VM_MODULE,APImenu::tab_code($m)))));
	  return; 
	}
      }
      
    case tab_vm_guests:
      if (empty($engine)){
	$engine    = 'bIcal_matrix_visits';
	// Redirect to "program attenders" if this is selected in the "guests" context
	if (!isset($_REQUEST['vm_eventSelector'])){
	  VM::e_ID(EVT_ANY);
	}elseif(VM::e_ID() > 0 && !headers_sent()){
	  foreach(array(tab_vm_end,
			tab_vm_ogz,
			tab_vm_vis) as $m){
	    if (VM::_MENU()->access($m)){
	      bTiming()->cpu();
	      $this->dbg(sprintf('Redirect %s --> %s',$tab,$m),array('redText',2,cnf_dev));
	      b_url::header(b_url::same('?q='.join('/',array(VM_MODULE,APImenu::tab_code($m)))));
	      return;
	    }
	  }
	}
      }
    case tab_vm_vmatrix:
    case tab_vm_vis_vmatrix:
    case tab_vm_ogz_vmatrix:
    case tab_vm_end_vmatrix:
      if (empty($engine)){
	$engine    = 'bIcal_matrix_visits';
	self::redirect_as_selected();
      }
    case tab_ea_off:
    case tab_ea_off_omatrix:
    case tab_vm_ogz_omatrix:
    case tab_vm_end_omatrix:
      if (empty($engine)) {
	$engine    = 'bIcal_matrix_offices';
      }
    case tab_vm_ogz_amatrix:
    case tab_vm_end_amatrix:
      if (empty($engine)){
	$engine    = 'bIcal_matrix_abs'; 
      }
      
      $this->eventSelector();
      locateAndInclude($engine);
      $m = new $engine();
      if (VM::e_ID() > 0)  $m->noNavig_arrows = $m->noNavig_months = True;
      $m->show();
      break;
      
    case tab_vm_ogz_acc:
    case tab_vm_vis_acc:
	VM_accommodationOptions(VM::$e)->update();
	VM_accommodationOptions(VM::$e)->show();
      break;
      
    case 'login':
      bAuth()->loginPrompt();
      break;
      
    case tab_vm_vis_welcome:
      $this->selectVisitAndVisitor();
      break;

    case tab_vm_ogz_dup:
      bForm_Avatar::findDuplicates(" LEFT JOIN abs_visits ON v_avid=av_id WHERE v_eid=".VM::e_ID());
      break;
      
    case tab_vm_adm_dup:
      bForm_Avatar::findDuplicates();
      break;
      
    case tab_vm_adm_fixClashes:
      $_POST['showClashingVisits'] = 'yes';
      locateAndInclude('bHolder_vm_Visits');
      $holder = new bHolder_vm_Visits();
      $holder->listEntries();
      break;
      
    case tab_vm_ogz:
    case tab_vm_end:
    case tab_vm_vis:
      $this->eventSelector();
      VM::$e->show_policies();
      break;
      
    case tab_ea_off_desk_labels:
      if (empty($doing)){
	$doing = 'deskLabels';
	if (function_exists('drupal_add_css')) drupal_add_css('./'.drupal_get_path('module',VM_MODULE).'/css/doorlabel.css');
      }
    case tab_ea_off_door_labels:
      if (empty($doing)){
	$doing = 'doorLabels';
	if (function_exists('drupal_add_css')) drupal_add_css('./'.drupal_get_path('module',VM_MODULE).'/css/doorlabel.css');
      }
      
    case tab_vm_vis_badges:
      if (empty($doing)){ 
	$doing = 'badges';      
	// for labels, badges
	bJS()->add_css_file('badges.css');
      }
      
    case tab_vm_ogz_photos:
    case tab_vm_vis_photos:
      if (empty($doing))    $doing = 'photos';
      
    case tab_vm_guests_budgetBP:
      if (empty($doing))    $doing = 'budget_byProjects';
      
    case tab_vm_vis_list:   
    case tab_vm_ogz_vlist:
    case tab_vm_end_vlist:
      if (empty($doing))    $doing = 'lists';
      
    case tab_vm_guests_to_approve:
      if (!is_object(bAuth::$av)) if (!bAuth()->loginPrompt())	break;
      if (empty($doing)){
	$doing = 'endorsing';
	$recs = VM_endorsers()->toEndorse(bAuth::$av->ID);
	if (count($recs) == 1){
	  $visit = new bForm_vm_Visit($recs[0]['v_id']);
	  $visit->getForm();
	  break;
	}
      }
      
    case tab_vm_guests_budget:
      if (empty($doing)){
	$doing = 'myguests';
	$_GET['host_avid_once'] = bAuth::$av->ID;
      }
      
    case tab_vm_vis_mails:
    case tab_vm_ogz_mails:
    case tab_vm_end_mails:
      if (empty($doing)){
	$doing = 'show_mails_exchange';
	if (VM::e_ID() && VM::$e->getValue('e_end',1) < time()){
	  $this->eventSelector();
	  myPear::WARNING("Sorry, the event is from the past, the mail exchange page is expired...");
	  return;
	}
      }
      
    case tab_vm_vis_budget:
    case tab_vm_ogz_budget:
    case tab_vm_end_budget:
      if (empty($doing))    $doing = 'budget';
      
    case tab_vm_vis_export:
      if (empty($doing))    $doing = '2excel';
      
      locateAndInclude('bHolder_vm_Visitors');
      $holder = new bHolder_vm_Visitors($doing);
      $holder->listEntries();
      break;
      
    case tab_vm_tenants:   // printing search results
    case tab_vm_vis_allVisits:
      locateAndInclude('bHolder_vm_Visitors');
      $holder = new bHolder_vm_Visitors();
      $holder->listEntries();
      break;
      
    case tab_vm_adm_projects:
      VM_projects()->getForm();
      break;
      
    case  tab_vm_adm_sc_setup:
      bList::getListInstance(myOrg_ID,'bList_vm_reimbursementRates')->getForm();
      break;
      
    case  tab_vm_adm_se_setup:
      VM_socialEventRates(myOrg_ID)->getForm();
      break;

    case  tab_vm_ogz_se_setup:
    case  tab_vm_vis_se_setup:
      VM::$e->socialEvents()->getForm();
      break;
      
    case tab_vm_adm_agendaList:
      VM_agendaEvents()->getForm();
      break;
      
    case tab_vm_adm:
      locateAndInclude('bUnit');
      bUnit::b_tabs_bUnits(VM_MODULE,myOrg_ID,array(RANK_vm_organizer));
      break;
      
    case  tab_vm_adm_unitOrganizers:
      locateAndInclude('bUnit_vm_organizers');
      $units = bUnit_vm_organizers::get_all_events(@$_GET['avid_once']);
      b_fmt::accordion('open',bUnit_vm_organizers::$e_index);
      foreach($units as $e_id){
	VM_organizers($e_id)->getUnitMemberIDs();
	$u = VM_organizers($e_id);
	b_fmt::accordion($u->e->name());
	$u->getForm();
      }
      b_fmt::accordion('close');
      break;
      
    case tab_vm_acc_add:
      if (empty($_GET['form'])) $_GET['form'] = 'vm_Room';
      if (empty($_GET['id']))   $_GET['id']   = 'new';
    case 'formTPL':
      if  (bAuth::authenticated()){
	$f = myPear::getInstance_new(str_replace('bForm_bForm_','bForm_','bForm_'.$_GET['form']),$_GET['id'],array('strict','fatal'));
	$f->getForm();
      }else{ 
	bAuth()->loginPrompt();
      }
      break;
      
      // case tab_vm_guests:
      //ZZ  b_url::header(b_url::same('?q='.join('/',array(VM_MODULE,APImenu::tab_code(tab_vm_guests),APImenu::tab_code(tab_vm_guests_addVisit)))));
      //ZZ  return;
      //ZZ  break;
      
    case tab_vm_guests_addVisit:
      myPear::INFO_once(array(x('p','The person whom you want to invite might very well be already registered in our Database').
			      x('p','Try first to search for the person using the name and/or e-mail address, <br>regular expressions are supported.'),
			      x('p','Create the new entry for the visitor only if you can\'t find him/her in the database')),
			'Tips for registering new visitors');
      myPear_search()->getForm(array('_lookFor' => 'visitor',
				     '_button'  => 'select for the visit',
				     '_header'  => 'Select the new visitor from the database',
				     '_after_search'=> "form=vm_Visit&id=empty&v_avid=AV_ID", 
				     '_after_search_check' => 'vm_after_search_check',
				     ));
      break;
      
      
    case 'tab_vm_acc_sok': // tab is not used any more, keep for reference
      myPear_search()->getForm(array('_lookFor' => '',
				     '_header'  => 'Search accommodation database',
				     '_button'  => 'show accommodation',
				     '_after_search'=> 'tab_force='.tab_vm_tenants.'&av_id=AV_ID',
				     '_join'    => 'abs_visits on av_id:v_avid',
				     '_where'   => 'v_avid>0'));
      break;
      
    case tab_vm_vis_sok: 
      myPear_search()->getForm(array('_lookFor' => '',
				       '_button'  => 'show visits',
				       '_header'  => 'Search for visitors',
				       '_after_search'=> 'tab_force=tab_vm_vis_allVisits&av_id=AV_ID',
				       '_join'    => 'abs_visits on av_id:v_avid',
				       '_where'   => 'v_avid>0'));
      break;
      
    case 'listSearch':
      myPear_search()->getForm();
      break;
      
    case tab_vm_adm_sync:
    case tab_vm_ogz_sync:
      locateAndInclude('agenda_vm',
		       'bUnit_vm_organizers',
		       'b_cache_file');
      b_cache_file::emptyCache();

      switch(b_btn::confirm()){
      case 'yes':
	// Extract list of relevant events for the organizers
	$events = array();
	if (bAuth::$av->isOrganizer()){
	  $list = bUnit_vm_organizers::getEvents(bAuth::$av->ID,array(),True);
	  foreach(array_keys($list) as $e_id) $events[] = "conf=$e_id&show_reg=all";
	  if (empty($events)) $events = array("conf=0");
	  else myPear::MESSAGE('Synchronizing agenda entries:'.x('ul',b_fmt::joinMap('li',array_values($list))));
	}
	VM_agenda()->sync($events);
	break;
	
      case 'no':
	b_url::header(b_url::same('?q='.join('/',array(VM_MODULE,
						       APImenu::tab_code(tab_vm_vis)))));
      case Null:
	b_btn::confirm("Synchronization is a long procedure, please confirm that you want to do it");
        return True;
      }
      break;
      
    case tab_ea_off_wipe:
      $this->eventSelector();
      $remove_msg = sprintf("wipe the office allocations for <em>%s</em>",VM::$e->name());
      switch(b_btn::confirm()){
      case 'yes':
	locateAndInclude('bCover_vm','b_cache_file');
	b_cache_file::emptyCache();
	bCover_vm::wipeAllocation('Offices',VM::$e);
	break;
	
      case 'no':
	b_url::header(b_url::same('?q='.join('/',array(VM_MODULE,
						       APImenu::tab_code(tab_ea_off),
						       APImenu::tab_code(tab_ea_off_list)))));
      case Null:
	b_btn::confirm("Are you sure you want to $remove_msg?");
        return True;
      }
      break;

    case tab_ea_off_assign:
      if (!isset($whatToAssign)) $whatToAssign = 'Offices';
      $this->eventSelector();

    case tab_vm_acc_assign:
      if (!isset($whatToAssign)) $whatToAssign = 'Apartment';

      locateAndInclude('bCover_vm','b_cache_file');
      bCover_vm::autoAllocate($whatToAssign,VM::$e);
      b_cache_file::emptyCache();
      break;
      
    case tab_ea_off_list:
      $this->eventSelector();
      locateAndInclude('bHolder_vm_Offices');
      $holder = new bHolder_vm_Offices();
      $holder->listEntries();
      break;

    case tab_vm_acc:
    case tab_vm_acc_huts:
      locateAndInclude('bHolder_vm_Huts');
      $holder = new bHolder_vm_Huts();
      $holder->listEntries();
      break;
      
    case tab_vm_acc_usage:
      $doing = 'usage';
      $title = 'Rooms balance';
    case tab_vm_acc_rooms:
      if (empty($doing))  $doing = 'list';
      if (empty($title))  $title = 'All apartments';
      locateAndInclude('bHolder_vm_Rooms');
      $holder = new bHolder_vm_Rooms($doing,$title);
      $holder->listEntries();
      break;
      
    case tab_vm_events:
    case tab_vm_events_see:
      locateAndInclude('bHolder_vm_Events');
      $holder = new bHolder_vm_Events();
      $holder->listEntries();
      break;
      
    case tab_vm_adm_acl:
      myPear::H1("Access Control List");
      VM_access()->show();
      if (b_cnf::get('merge')){ // checked, works... however lets wait a bit YB 2011-04-20 
	$q = myPear_db()->query("SELECT *,CONCAT(av_firstname,av_lastname) AS fn  FROM zzz_avatars GROUP BY fn");
	while($r=myPear_db()->next_record($q)){
	  $qq = myPear_db()->query("SELECT * FROM zzz_avatars WHERE ".
				     " av_firstname = \"$r[av_firstname]\" AND ".
				     " av_lastname  = \"$r[av_lastname]\" AND ".
				     " av_id != '$r[av_id]'");
	  $n = 0;
	  while($rr=myPear_db()->next_record($qq)){
	    if (!$n++)	  myPear::WARNING("$r[av_firstname] $r[av_lastname] $r[av_email] -----------------------------------------------------------------");
	    myPear::WARNING("$rr[av_firstname] $rr[av_lastname] $rr[av_email]");
	    if (True ||
		stristr($r['av_email'],'gmail')   || stristr($rr['av_email'],'gmail') ||
		stristr($r['av_email'],'nordita') || stristr($rr['av_email'],'nordita')){
	      $k = $rr['av_id']; 
	      $d =  $r['av_id']; 
	      if (stristr( $r['av_email'],'gmail')){$k = $rr['av_id']; $d =  $r['av_id']; }
	      if (stristr($rr['av_email'],'gmail')){$k =  $r['av_id']; $d = $rr['av_id']; }
	      if (stristr( $r['av_email'],'nordita.org')){$k = $r['av_id']; $d = $rr['av_id']; }
	      if (stristr($rr['av_email'],'nordita.org')){$k =$rr['av_id']; $d =  $r['av_id']; }
	      bForm_Avatar::merge($d,$k);
	      break;
	    }
	  }
	}
      }
      break;
      
    default:
      myPear::WARNING_T(bIcons()->get('32-p_pc')." '$tab' is not yet implemented, work in progress");
    }
    
    // Avoid infinite loop;
    if (!myPear_taskManager()->exec_request){
      // Remind organizers o the applications deadline is reached
      VM_mailer()->cronjob_remind_organizers();
      
      // Remind endorses about pending visits
      VM_endorsers()->remind();
      
      // Finally show the mails sent
      VM_mailer()->showMailsSent(VM_manager_here);
    }
    bTiming()->cpu();
  }
  
  /*
   * Eventually redirect to the event closest in time if we got here by pressing the menu item 
   */
  private static function redirect_as_selected(){
    if (!isset($_REQUEST['vm_eventSelector'])){
      if (VM::e_ID() <= 0){
	$events = array_keys(self::list_of_events(True));
	$e_id = array_shift($events);
	if (empty($e_id)){
	  myPear::WARNING_T('Empty reply from '.__CLASS__.'::list_of_events()');
	}else{
	  VM::e_ID($e_id);
	  myPear::WARNING_T("Impose ".VM::$e->name());
	}
      }
    }
  }
  
  /*
   * Generate list of events customized for the logged-in user
   */
  private static $dates = array();
  private static function list_of_events($need_only_one_event=False){
    static $events = array();
    
    VM_access();
    if (empty($events) && bAuth::authenticated()){
      
      // See which menu item was pressed and decide the action
      switch(APItabs::code2tab(b_cms::_(1))){
      case tab_vm_guests:	// Only "any event"
	if (defined('vm_day1'))  $events[EVT_ANY] = myOrg_nameS.' guests '.date('Y-m-d',vm_day1) .' - '. date('Y-m-d',vm_day9);
	else                     $events[EVT_ANY] = myOrg_nameS.' guests';      
	break;
	
      case tab_vm_end:
	$events = bAuth::$av->eventsToEndorse();
	break;
	
      case tab_vm_ogz:
      case tab_vm_vis:
	$events = array();
	if (VM_organizer_here){
	  // The event organizer would probably want to see his event only
	  $events = bUnit_vm_organizers::getEvents(bAuth::$av->ID,$events);
	  break;
	}
	
      default:
	if (VM_registrant_here){
	  // The registrant sees only the events he is/was attending
	  $visits = bAuth::$av->getVisits();
	  foreach($visits as $v_id=>$name){
	    $v = myPear::getInstance_new('bForm_vm_Visit',$v_id,array('strict','fatal'));
	    $v->hookEvent();
	    if (is_object($v->e))  $events[$v->e->ID] = $v->e->name();
	  }
	}else{
	  $where = array();
	  if (!b_cnf::get("vm_seeOldPrograms")) $where[] = "e_end > ".(time() - AUTO_EXPIRATION*86400);
	  $DESC = $need_only_one_event ? 'ASC' : 'DESC';
	  $q = myPear_db()->query("SELECT e_code,e_id,e_start,e_end,e_name,COUNT(v_id) AS n FROM abs_events ".
				    " LEFT JOIN abs_visits ON v_eid = e_id ".
				    " WHERE ".join(' AND ',$where). 
				    " GROUP BY v_eid ORDER BY e_start $DESC");
	  while ($r = myPear_db()->next_record($q)){
	    if ($r['n']>0 && $r['e_code']>0){
	      locateAndInclude('bForm_vm_Event');
	      $events[$r['e_id']] = b_fmt::strcut(strip_tags(bForm_vm_Event::_name($r)),40,'...');
	      self::$dates[$r['e_id']]  = array($r['e_start'],$r['e_end']);
	    }
	  }
	}
      }
    }

    //
    // Find the "today event"
    //
    $closest_eID = self::closest_eID($events);
    
    // Build the reply array
    if ($need_only_one_event){
      $reply = array($closest_eID => $events[$closest_eID]);
    }else{    
      // Check the "inforced" event
      $vm_eventSelector = @$_REQUEST['vm_eventSelector'];
      if (($vm_eventSelector > 0) && empty($events[$vm_eventSelector])){
	b_debug::xxx("Set vm_eventSelector=$vm_eventSelector");
	b_cnf::set('vm_eventSelector',$vm_eventSelector);
	locateAndInclude('bForm_vm_Event');
	$e = myPear::getInstance_new('bForm_vm_Event',$vm_eventSelector,array('strict','fatal'));
	//	self::$dates[$vm_eventSelector]  = array($e->getValue('e_start'),$e->getValue('e_end'));
	//	VM::e_ID($e->ID);
	$_SESSION['event_wish_list'][$vm_eventSelector] = b_fmt::strcut(strip_tags($e->name()),40,'...');
      }
      if (!empty($_SESSION['event_wish_list'])){
	foreach($_SESSION['event_wish_list'] as $e_id=>$e_name) $events[$e_id] = $e_name;
      }
      $reply = $events;
    }

    // Sanity
    if (!b_cnf::get('vm_eventSelector')  || (count($k=array_keys($reply))==1)){
      if (!empty($closest_eID)) VM::e_ID($_REQUEST['vm_eventSelector'] = $closest_eID);
    }

    // Switch to the visits outside events
    if (VM::e_ID() && is_object(VM::$e) && empty($reply[VM::$e->ID])){
      b_debug::xxx(sprintf('attempt to reset VM::$e(%s) to NULL',VM::$e->ID));
      VM::$e = Null;
    }

    //    if (cnf_dev) b_debug::print_r($reply,__FUNCTION__.' '.@$events[$closest_eID],'nosort');
    if (!defined('vm_day1') || $need_only_one_event)      $events = array();

    // b_debug::print_r($events,'events','nosort');    
    return $reply;
  }
  
  private static function closest_eID($events){
    $all_ids = array_keys($events);
    $closest_eID = @$all_ids[0];
    foreach(array_keys(self::$dates) as $e_id){
      if ((self::$dates[$e_id][0]>time()) || (self::$dates[$e_id][0] <= time() && self::$dates[$e_id][1] >= time())) $closest_eID = $e_id;
    }
    return $closest_eID;
  }

  /*
   * Set the event selected for viewing as the "current event"
   */
  function eventSelector(){
    $this->dbg(__METHOD__);
    if (!bAuth::authenticated()) return;

    // Get list of events customized for the logged-in user
    $events = self::list_of_events();
	
    switch($showCalendar = (((@$_REQUEST['vm_eventSelector']==EVT_ANY) && VM_administrator_here) ||
			    (count($events) == 1 && isset($events[EVT_ANY])))){
    case True: // Visits outside events
      if (!defined('vm_day1')) b_debug::internalError("'vm_day1' is not defined");
      locateAndInclude('b_datePicker.inc');
      bDatePicker()->message = ' ';
      bDatePicker()->imposeConversion = True;
      bDatePicker()->reset(); // reset to defaults
      
      $GLOBALS['myPear_custom_title'] = 
	x('h1',
	  x("form action='".b_url::same()."' method='POST' enctype='application/x-www-form-urlencoded' class='only_online' name='X1' accept-charset='UTF-8'",
	    myOrg_nameS.' guests ' . join(' ',array_merge(bDatePicker()->get2('vm_day1',vm_day1,
									      'vm_day9',vm_day9,
									      True),
							  array('<input type="hidden" name="vm_eventSelector" value="'.EVT_ANY.'"/>',
								'<input type="submit" class="page_load_progress" value="Set new time range"/>',
								)))
	    ));
      break;
      
    case False:      // Visits within events
      if (count($events)>1){
	self::redirect_as_selected();
	if (empty($events[b_cnf::get('vm_eventSelector')])){
	  $closest_eID = self::closest_eID($events);
	  b_cnf::set('vm_eventSelector',$closest_eID);
	  VM::e_ID($closest_eID);
	}
	VM::$formDummy->formDB['vm_eventSelector'] = b_cnf::get('vm_eventSelector');
	VM::$formDummy->submitOnChange['vm_eventSelector'] = True;
	$GLOBALS['myPear_custom_title'] = myPear::set_main_selector(array(VM::$formDummy->selectMenu_exec('vm_eventSelector',
													  $events,
													  "class='evHeader'",
													  $onlyRW=True)),
								    True);
      }else{
	if (is_object(VM::$e)) myPear::H1(VM::$e->name(True),'reset');
      }
    }
  }

  /*
   * Check the built-in projects,
   * eventually fix the project numbers in the visit/lease objects 
   */
  private function set_buildInObjects(){
    // this should be called only once
    if (!@$_SESSION[__FUNCTION__]++){
      
      locateAndInclude('bForm_vm_Visit');
      foreach(VM_projects()->get_projects($buildInOnly=True) as $id=>$type){
	switch($type){
	case VISIT_TYPE_RENT:
	  myPear_db()->query("UPDATE abs_visits SET v_projectid=$id WHERE v_type = '".VISIT_TYPE_RENT."'");
	  break;
	  
	case LODGING_OA:
	  // assign visits without the project number to OA project
	  $q = myPear_db()->query(sprintf("SELECT * FROM abs_leases LEFT JOIN abs_visits ON lease_vid=v_id WHERE v_projectid=0 AND lease_aid='%s'",VM::$OA->ID));
	  while ($r = myPear_db()->next_record($q)){
	    $visit = new bForm_vm_Visit($r['v_id']);
            $visit->defineVariables();
	    $visit->updateDB(array('v_projectid' => $id));
	  }
	  break;
	  
	case LODGING_BR:
	  break;
	  
	default:
	  myPear::WARNING_T("Unexpected $type");
	}
      }
    }
  }
    
  private function selectVisitAndVisitor($selectOnlyEvent=False){
    static $allAvatars = array();
    
    if ($v_id = b_cnf::getValue('_selected_attender')){
      $v = myPear::getInstance_new('bForm_vm_Visit',$v_id,array('strict','fatal'));
      $v->defineVariables();
      print myPear::H1($v->name());
      $v->getForm();
    }else{

      print myPear::H1('Welcome');
      /*
       * Select the event
       */
      $this->eventSelector();
      
      /*
       * Select the attender 
       */
      $q = myPear_db()->query("SELECT * FROM abs_visits ".
				" LEFT JOIN zzz_avatars on v_avid = av_id ".
				" WHERE v_eid = ".VM::e_ID().myPear_db()->orderBy('av_lastname'));
      while ($r = myPear_db()->next_record($q)){
	$allAvatars[$r['v_id']] = mb_strToUpper($r['av_lastname']).' '.$r['av_firstname'];
      }
      $e = myPear::getInstance_new('bForm_vm_Event',VM::e_ID(),array('strict','fatal'));
      if (empty($allAvatars)) print x('h3','No attenders'.(is_object($e) ? ' for '.x('i',$e->nameSimple()) : ''));
      else print x("form action='".b_url::same()."' method='post'",
		   x('strong','Select the attender').
		   VM::$formDummy->selectMenu_exec('_selected_attender',$allAvatars,"onchange='submit()'",$onlyRW=True));
    }
  }
}

function _vm_title_callback($tab, $dTab=Null,$returnText=False){
  if ($dTab){
    $menuItem = ($returnText
		 ? $tab
		 : t($tab));
  }else{
    if (is_object(bAuth::$av)){
      
      if (VM_external_registrant_here){
	switch($tab){
	case tab_vm_vis:         $tab = 'Event attenders';            break;
	case tab_vm_home:        $tab = 'My contact information';     break;
	case tab_vm_vis_vmatrix: $tab = 'Who arrives when';           break;
	case tab_vm_vis_photos:  $tab = 'Photos';                     break;
	default:
	}
      }elseif(is_object(VM::$e)){
	switch($tab){
	  //case tab_vm_vis:         $tab = VM::$e->name(0);              break;
	default:
	}
      }
    }
    if(isset(APItabs::$tab_definition[$tab]))  $tab = APItabs::$tab_definition[$tab];
    else myPear::ERROR(sprintf("??? undefined tab \"%s\"\n",$tab));
    $menuItem = ($returnText
		 ? $tab
		 : t(bText::_(array('key' => "MENU_$tab",'item'=> $tab))));
  }
  return $menuItem;
}

