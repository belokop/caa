<?php
/*
 * Tricks:
 * - to endorse an event WITHOUT sending emails to the organizers add "&showen=1&" to the URL 
 */
locateAndInclude(array('bForm_vm_Lease',
		       'bForm_vm_Visit'));
// Define tab names
foreach(array('tab_accommodation_options'=> 'Accommodation options',
              'tab_available_budget'     => 'Available budget',
              'tab_reimbursement_rates'  => 'Reimbursement rates',
              'tab_social_events'        => 'Social events',
              'tab_participants'         => 'Participants',
              'tab_visitors'             => 'Visitors',
              'tab_reimbursement'        => 'Reimbursement',
              'tab_summary'              => 'Summary',
              'tab_budget_summary'       => 'Budget summary',
              ) as $tab=>$v)  define($tab,$v);

define('ICON_mail_sent_y',  bIcons()->get(array('d'=>'The mail was sent',  
						'i'=>'i-ok', 
						'c'=> 'opacity_off')));
locateAndInclude('bHolder_vm');
class bHolder_vm_Visitors extends bHolder_vm{
  
  var $debug = True;
  var $autoOpenHeader = False;
  
  var $photosPerRow =   5;   // number of photos per printing row
  var $photos_Nrows =  99;   // number of rows
  var $badgesPerRow =   2;   // number of badges per printing row
  var $badges_Nrows = 555;   // number of rows
  var $doorLabelsPerRow = 2; // number of door labels per printing row
  var $doorLabels_Nrows = 3; // number of rows
  
  var $labels      = array();
  var $total       = array();
  var $sortBy_default= 'name';
  var $fnc_beslut  = False;  // formalities not completed
  var $se     = Null;   // associated social_events_list class
  var $bs     = Null;   // associated budgetSource class
  var $av     = Null;
  var $v      = Null;
  var $noSort = True;

  var $money      = array('scholarshipT_e', 'scholarshipT_r', 'scholarship_e',
			  'scholarshipP_e', 'scholarshipP_r', 'scholarship_r',
			  'total_e','total_r','other','livingA','livingB','livingH');
  var $money_bold = array('scholarship_e','total_e',
			  'scholarship_r','total_r');
  var $money_est  = array('scholarship_e','scholarshipP_e','scholarshipT_e','total_e',);

  
  var $budgetHeader = array('name'         => '',
			    'period'       => '',
			    
			    'space0'       => '',
			    'livingA'      => 'Ap',
			    'livingB'      => 'ApHotel',
			    'livingH'      => 'Hotel',
			    
			    'space1'       => '',
			    'scholarshipP_e'=> 'Subsist. est.',
			    'scholarshipP_r'=> 'Subsist. cost',
			    'scholarshipT_e'=> 'Travel est.',
			    'scholarshipT_r'=> 'Travel cost',
			    
			    'space2'       => '',
			    'other'        => 'Other',

			    'space3'       => '',
			    'total_e'      => 'Estimation',
			    'total_r'      => 'Cost',
			    );
  
  private $doing='lists';
  private $ac2h = array(HUT_A  =>'livingA',
			HUT_AH =>'livingB',
			HUT_H  =>'livingH');
  private $biz  = array();
  private $organizers = array();
  private $tabs_toShow = array();
  private $b_tabs = Null;
  //  private $excel_input = Null;

  // True if the event has no budget assigned
  private $event_is_bankrupt = true; 
  
  private $collected_highlights = array();

  /*
   * List of "non-complete formalities"
   *
   * 'z' - human readable "problem description"
   * 'd' - human readable "recovery action"
   * 'h' - the color to highlight the visit row in the list
   * 'i' - icon
   * 's' - status - event can't be endorsed if this problem occurs
   */
  var $highlights = array('v_status' =>array(STATUS_CLASH      =>array('s'=>'fatal',
								       'h'=>'highlightRed',
								       'i'=>'i-avatar',
								       'z'=>'clashing visit request'),
					     STATUS_PENDING    =>array('s'=>'fatal',
								       'h'=>'highlightYellow',
								       'i'=>'i-avatar',
								       'z'=>'pending approval request'),
					     STATUS_NO         =>array('h'=>'', 
								       'i'=>'',
								       'z'=>'Application denied'),
					     STATUS_YES        =>array('h'=>'',
								       'i'=>'')),
			  'fnc'      =>array(STATUS_NO         =>array('z'=>'pending "rejection" letters',
								       'h'=>'highlightRed',
								       'd'=>'send mail "REFUSE application"',
								       'i'=>'i-mail-send'),
					     STATUS_YES        =>array('h'=>'highlightRed',
								       'z'=>'pending "confirmation" letter',
								       'd'=>'send mail "welcome to Nordita"',
								       'i'=>'i-mail-send'),
					     LODGING_REQ_ONLY  =>array('s'=>'fatal',
								       'h'=>'highlightRose',
								       'z'=>'unresolved accommodation request (_a request only)',
								       'd'=>'unresolved accommodation request (_a request only)',
								       'i'=>'i-avatar'),
					     STATUS_PENDING    =>array('s'=>'fatal',
								       'h'=>'highlightYellow',
								       'z'=>'pending approval request',
								       'd'=>'pending approval request',
								       'i'=>'i-avatar'),
					     ''                =>array('h'=>'')));
  var $rec_visit = array(); // array of all the visits, one row per visit (all the leases are summarized)
  var $rec_project = array();
  private $exp;  // pointer to class "Expenses"
  
  /*
   * Send bulk information mail to the applicants
   */
  private function process_emails(&$doing){

    if (isset($_GET['mail2all_once'])){
      //
      // ----------------------- Send deny/Send welcome E-mails to all attenders of the event
      //
      $q = $this->query();
      while($rec = myPear_db()->next_record($q)){
	if(@$rec['v_status'] === STATUS_NO){
	  if(!VM_mailer()->m_registrant_deny($rec['v_id'],'status'))
	      VM_mailer()->m_registrant_deny($rec['v_id'],$no_preview=True);
	}elseif(@$rec['v_status'] === STATUS_YES){
	  if(!VM_mailer()->registrant_welcome($rec['v_id'],'status'))
	    VM_mailer()->registrant_welcome($rec['v_id'],$rec['hut_id'],$rec['a_name'],$no_preview=True);
	}
      }
      
    }elseif (isset($_GET['mail_once'])){
      //
      // ----------------------- Preview e-mails 
      //
      $doing = 'send_accept_deny';
      switch($_GET['mail_once']){        
      case 'vm_mail_yes':
	VM_mailer()->registrant_welcome($_GET['v_id'],$_GET['hut_id'],$_GET['a_name']);
	break;
	
      case 'vm_mail_no':
	VM_mailer()->m_registrant_deny($_GET['v_id']);
	break;
	
      case 'vm_mail_info':
	VM_mailer()->m_registrant_info($_GET['v_id']);
	break;

      case 'vm_mail_pwd':
	VM_mailer()->remind_organizer($_GET['v_id'],VM::$e,'pwd',False);
	break;
      }
    }elseif(isset($_GET['sendmail_once'])){
      //
      // ----------------------- Sending the e-mail after preview
      //
      switch($sendmail_once=$_GET['sendmail_once']){
      case 'send':
	myPear::DEBUG("sendmail_once='$sendmail_once'",$this);
	break;
      default:
	myPear::DEBUG("sendmail_once='$sendmail_once'... What to do???",$this);
	
      }
    }
  }
  
  /*
   *
   */
  private $bList_parent = Null;
  private $keep_thisRec = False;
  function __construct($doing='lists'){
    $this->dbg('----------------------------------------------------------------');
    parent::__construct();

    // Catch the list of visits for a given person
    if (isset($_GET['av_id'])) $this->av = myPear::getInstance('bForm_Avatar_vm',$_GET['av_id'],array('nocache','strict'));

    // Define some useful shortcuts
    define('split_by_huts',(!is_object($this->av) &&
			    !VM::outOfScope() &&			
			    (VM_organizer_here || VM_booker_here) &&
			    in_array($doing,array('2excel','lists'))));
    $this->dbg('split_by_huts - '.(split_by_huts ? 'YES' : 'NO')); 
    
    if (is_object(VM::$e) && !is_object($this->av)){
      // Process pending requests
      $this->bList_parent = VM::$e->ID;

      // Warn if there is no applicants for the event
      $e_reg_end = VM::$e->getValue('e_reg_end',1);
      $e_start   = VM::$e->getValue('e_start',1);
      if (!VM::$e->attenders()) myPear::MESSAGE_once(x('strong',"No applications yet for <em>".VM::$e->name()."</em>"));

      // Say (only once per session) when the event starts
      if ($e_start > time() && $e_start < time() + 365*24*3600){
	myPear::MESSAGE_once(x('strong',b_time::delta(time(),$e_start).' before the event starts'));
	if ($e_reg_end > time()) myPear::MESSAGE_once(x('strong',b_time::delta(time(),$e_reg_end).' before the applications deadline'));
      }
      if (VM::outOfScope())	myPear::MESSAGE("This event is out of VM scope");
    }else{
      $this->bList_parent = myOrg_ID;
    }
    if (in_array($doing,array('budget_byProjects','myguests'))){
      $_GET['sBy'] = 'v_start';
      $this->bList_parent = myOrg_ID;
    }

    //
    // Complete the reimbursement update
    //
    foreach(preg_grep('/^R:/',array_keys($_POST)) as $k){
      if (($amount = @$_POST[$k]) > 0){
	list($x,$column,$v_id) = explode(':',$k);
	// Update the database
	$visit = new bForm_vm_Visit($v_id);
	$exp = $visit->getExp();
	$exp->updateDB(array($column=>$amount));
	// Show a message
	$msg = $visit->av->fmtName('Lf').x('em'," - $column ".b_fmt::money($amount)).CONST_currency;  
	foreach(array('exp_'   => '',
		      'travel' => 'travel reimbursement',
		      'perdiem'=> VM_wordPerdiem,) as $f=>$t) $msg = str_replace($f,$t,$msg); 
	myPear::MESSAGE($msg);
      }
    }

    //
    // Complete sending mails
    //
    $this->process_emails($doing);
    
    if (isset($_GET['doing_once'])) $doing = $_GET['doing_once'];

    //
    // Complete the highlights array
    if (VM::e_ID() > 0)  $this->organizers = VM::$e->getOrganizers();
    foreach ($this->highlights as $level=>$l_data){
      foreach($l_data as $status=>$data){
	if (!empty($data['i'])) $this->highlights[$level][$status]['i'] = bIcons()->get($data);
      }
    }

    if ($doing != 'send_accept_deny'){
      
      //
      // Endorse the event budget, inform organizers about this.
      //
      if (isset($_GET['e_endorse_once'])){
	// Endorse (i.e. "lock" the event budget) OR de-endorse (i.e. unlock the budget, let changes to happend)
	$lock = (bool)$_GET['e_endorse_once'];
	VM::$e->setPolicy(VM_E_endorsed,($lock ? 1 : 0));
	if ($lock){
	  myPear_logs()->add('endorse',VM::$e->ID);
	  foreach($this->organizers as $av_id){
	    // Send the e-mails to organizers if not yet done
	    //	    $v = bForm_vm_Visit::getVisit($av_id,VM::$e->ID);
	    $v = myPear::getInstance("bForm_vm_Visit","v_avid='$av_id' AND v_eid=".x("'",VM::$e->ID),array('nocache','strict'));
	    if (empty($v)){
	      $av = new bForm_Avatar_vm($av_id);
	      myPear::WARNING(sprintf("Organizer <em>%s</em> is not registered for <em>%s</em>",$av->fmtName('Lf'),VM::$e->name()));
	    }else{
	      VM_mailer()->remind_organizer($v,VM::$e,'en');
	    }
	  }
	}
      }
      
      // Do not show icons explanation, rely on tool-tips
      bIcons()->toggleExplainIcons(False);
      bIcons()->reset();

      b_url::setReferrer(); 
      
      if(is_object($this->av)){
	myPear::H1($this->av->name());
      }elseif($doing != 'endorsing'){
	myPear::H1('empty');
	if (empty($_GET['host_avid_once']) && 
	    !preg_grep('/bList.*_once/i',array_keys($_GET)) && 
	    !preg_grep('/_post.*_class/',array_keys($_POST))){
	  VM::_MENU()->eventSelector();
	}
      }
    }
    
    /*
    // Hide the administration approvals from the organizers
    //
    // YB 2012-04-05 
    // We now send the Welcome/Refuse letters semi-automatically, when the budget is approved
    // by the endorser
    if (VM_organizer_here){
      foreach(array('v_status'=>array(STATUS_NO,),
		    'fnc'     =>array(STATUS_NO,STATUS_YES,)) as $c=>$d){
	foreach($d as $st) unset($this->highlights[$c][$st]);
      }
    }
    */
    if (VM::isEventEndorsed() || VM::isArchived()) $this->styles = 'class="opacity_off"';

    //
    // Build tabs, will be filled later
    //
    switch ($this->doing = $doing){
    case 'budget':

      // Check that the event has any budget      
      $this->bs = True;
      if (is_object(VM::$e)){
	VM::$e->budgetSource()->get_budgetSource($default_only=False,$verbose=True);
	$this->event_is_bankrupt = (VM::$e->budgetSource()->total <= 0);
	//	$this->event_is_bankrupt = False;
      }

      // Build tabs used for budget
      $this->tabs_toShow = (is_object(VM::$e)
			    ? array(tab_available_budget     => tab_available_budget,
				    tab_reimbursement_rates  => tab_reimbursement_rates,
				    tab_social_events        => tab_social_events,
				    tab_participants         => 'print_collected_visits_budget',
				    tab_reimbursement        => 'print_collected_visits_budget',
				    tab_summary              => tab_summary,
				    )
			    : array(tab_reimbursement_rates  => tab_reimbursement_rates,
				    tab_visitors             => 'print_collected_visits_budget',
				    tab_budget_summary       => tab_budget_summary,
				    ));
      if (is_object(VM::$e) && !VM::$e->attenders(True)){
	unset($this->tabs_toShow[tab_summary]);
	unset($this->tabs_toShow[tab_social_events]);
	unset($this->tabs_toShow[tab_reimbursement_rates]);
      }

      if (VM_reimberser_here){
	unset($this->tabs_toShow[tab_reimbursement_rates]);
	unset($this->tabs_toShow[tab_participants]);
	$def_tab = 2;
      }else{
	unset($this->tabs_toShow[tab_reimbursement]);
	$def_tab = 3;
      }
      break;

    case 'myguests':
      $this->tabs_toShow = array(tab_reimbursement_rates  => tab_reimbursement_rates,
				 tab_visitors             => 'print_collected_visits_budget',
				 tab_budget_summary       => tab_budget_summary,
				 );
      $def_tab = 1;
      break;

    case 'budget_byProjects':
      // See "listEntries_summary()", the tabs are created after reading the database
      break;
      
    case 'lists':
      $this->tabs_toShow = array((is_object(VM::$e) ? tab_participants : tab_visitors)  => 'print_collected_visitors',
				 tab_accommodation_options                              => tab_accommodation_options);
      if (VM::outOfScope() || !split_by_huts || is_object($this->av)){
	unset($this->tabs_toShow[tab_accommodation_options]);
      }
      $def_tab = 0;
      break;
    }
    if (!empty($this->tabs_toShow)){
      locateAndInclude('b_tabs');
      $this->b_tabs = new b_tabs_myPear(array_keys($this->tabs_toShow),$def_tab);
    }

    //    
    // Complete the constructor
    //
    switch ($this->doing ){
    case 'photos':
    case 'badges':
    case 'doorLabels':
      break;
      
    case 'budget_byProjects':
      if (!defined('vm_day1')) b_debug::internalError("'vm_day1' is not defined");

    case 'myguests':
      VM::$e = Null;
      myPear::H1('People I invite','reset');
      
    case 'show_mails_exchange':
    case 'endorsing':
    case 'budget':
    case 'lists':
      if ($v_id = @$_GET['refuse_once']){
	
	$this->v = myPear::getInstance('bForm_vm_Visit',$v_id);
	$this->v->setStatus(STATUS_NO);

      }elseif ($v_id = @$_GET['approve_once']){

	$this->v = myPear::getInstance('bForm_vm_Visit',$v_id);
	$this->v->setStatus(STATUS_YES);

      }elseif ($policy = @$_GET['toggle_once']){

	$this->v = myPear::getInstance('bForm_vm_Visit',$_GET['v_id'],'strict');
	if ($this->v) $this->v->setPolicy($policy,!$this->v->getPolicy($policy));

      }elseif(@$_GET['invoice_once']){

	myPear::WARNING('Invoice NYI');

      }elseif($p = preg_grep('/_c_id:\d+$/',array_keys($_POST))){

	// The co-tenant is selected, create a list entry
	VM_cotenants()->set_cotenants(array($_POST[$p[0]],
					      $_POST['lease_id']));

      }elseif($p = preg_grep('/_a_id:\d+$/',array_keys($_POST))){

	// Book an apartment according to the selection
	$v = new bForm_vm_Visit($_POST['v_id']);
	if ($p[0] == LODGING_RM){
	  $l = bForm_vm_Lease($_POST['lease_id']);
	  $l->hookApartment(LODGING_RM);
	}else{
	  $v->setBooking($_POST[$p[0]]);
	}

      }
      if ($this->doing=='endorsing'){
	myPear::H1('Pending visit approvals','reset');
      }elseif(!VM::$e && $this->doing!='myguests'){
	myPear::H1('Visitors outside conferences','reset');
	// print x('h4',b_time::period(vm_day1,vm_day9));
      }
      break;

    case '2excel':
      $this->autoOpenHeader = True;
    case 'send_accept_deny':
    case 'reception_attenders':
      break;
      
    default:
      b_debug::internalError("Do not know how to do $doing");
    }
    $this->dbg($this->av.' doing '.$this->doing);
  }
  
  private function current_tab_in_array($tabs_to_skip=array()){
    if (is_object($this->b_tabs)){
      $reply = in_array($this->b_tabs->active_tab_name(),$tabs_to_skip);
    }else{
      $reply = False;
    }
    if ($this->debug) b_debug::xxx($reply); 
    return $reply;
  }
  

  // No need to read the list of participants when processing those tabs
  private function current_tab_does_not_need_applicants(){  
    $reply = $this->current_tab_in_array(array(tab_social_events,
					       tab_available_budget,
					       tab_reimbursement_rates,			 
					       tab_accommodation_options,
					       ));
    $this->dbg(($reply ? 'SKIP' : 'PROCESS'). ' applicants',cnf_dev);
    return $reply;
  }

  private function current_tab_does_not_need_buttons(){  
    $reply = $this->current_tab_in_array(array(tab_summary,
					       ));
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Show only the own records, but
   * let the admins to see everything
   */
  function rejectRecord(){ 

    // No need to read the list of participants when processing those tabs
    if ($this->current_tab_does_not_need_applicants()) return True;

    $reply   = False;
    $dbg_text = '';
    if (is_object($this->b_tabs)){
      $active_tab_index = $this->b_tabs->active_tab();
      $tabs = array_values($this->tabs_toShow);
      $reply = (strpos($tabs[$active_tab_index],'bList') !== False);
      $dbg_text = 'b_Tabs';
    }

    $this->v = Null;
    if (!$reply && empty($this->av)) switch ($this->doing){
	
      case 'budget':
      case 'myguests':
      case 'budget_byProjects':
	break;
	
      case 'lists':
	if (bForm_vm_Visit::_getStatus($this->rec) == STATUS_NO){
	  if (VM_organizer_here && VM::$e->isEventEndorsed()){
	    $dbg_text = $this->rec['av_firstname'].' '.$this->rec['av_lastname'].' - After the approval do not show the refused applicants to the organizers';
	    $reply = True;
	  }
	}
      case 'show_mails_exchange':
	if (!cnf_dev && $this->rec['v_type'] === VISIT_TYPE_RENT){
	  $dbg_text = 'VISIT_TYPE_RENT... to be completed';
	  $reply = True;
	}elseif (empty(VM::$e)){
	  // Visits outside conferences/programs
	  if (!VM_administrator_here && ($this->rec['v_host_avid'] != @bAuth::$av->ID)){
	    $dbg_text = 'not my visit';
	    $reply = True;
	  }else{
	    // Guarantee the correct value of status&policy in the snapshot record
	    if (empty($this->rec['v_status'])){
	      $this->v = myPear::getInstance('bForm_vm_Visit',$this->rec['v_id'],array('strict'));
	      $this->rec['v_status'] = $this->v->getStatus();
	      $this->rec['v_policy'] = $this->v->getValue('v_policy',True);
	    }
	  }
	}
      }
    if ($reply) $this->dbg($dbg_text);
    return $reply;
  }
  
  /*
   * Postpone header printing for the budget display
   */
  function drawHeader(){
    switch ($this->doing){
    case 'budget_byProjects':
    case 'myguests':
      if (!(@$this->t instanceof b_table)){
	$this->dbg('CANCEL header');
	$this->t = new b_table_dummy();
      }
      break;

    default:
      parent::drawHeader();
      break;
    }
  }

  /*
   * List the entries collection
   */
  private $visit_ID = 0;
  function listEntries(){
    
    if (!is_object(bAuth::$av)) return; // sanity
    
    if (is_object($this->av)){

      // Display the search results
      $q = $this->query();
      if (myPear_db()->num_rows($q) == 1){
	while($r = myPear_db()->next_record($q)){
	  $v = new bForm_vm_Visit($r['v_id']);
	  $v->getForm();
	}
      }else{
	parent::listEntries();
      }
      
    }else{
      
      switch ($this->doing){
	// Just return, the entries list is not desired here
      case 'send_accept_deny': 
	return;
	
	// "Standard" output (i.e. parent::listEntries)
      case 'photos':
      case 'badges':
      case 'doorLabels': 
      case 'budget_byProjects':
      case 'myguests':
	parent::listEntries();
	return;
	
      case 'endorsing':
	// Endorsing guest invitations
	$pendings = VM_endorsers()->toEndorse(bAuth::$av->ID);
	if (empty($pendings)){
	  myPear::MESSAGE(x('strong',"There is no pending requests for You, everything is now resolved"));
	}else{
	  myPear::MESSAGE("Please edit the visit request form and:".
			  "<br/>- approve or deny the visit request".(count($pendings)>1?'s':'').
			  "<br/>- check the budget code assigned for this visit");
	  $this->t = new b_table_zebra($this->header());
	  $this->keep_thisRec = True;
	  foreach(VM_endorsers()->toEndorse(bAuth::$av->ID) as $this->rec){
	    $this->format();
	    $this->t->prt($this->rec);
	  }
	  $this->t->close();
	}
	return;
	

      default:
	// 'budget', 'lists'... What else gets here?
	$this->dbg("Unexpected(?) '".$this->doing."', perform a default action");
      case 'show_mails_exchange':
	parent::listEntries();
      }
    }
    
    //
    // close the tabs, everything is done
    if (is_object($this->b_tabs)) $this->b_tabs->close();
    $this->b_tabs = Null;
  }

  /*
   * Build budget summary 
   */
  private function print_tab_budgetSummary(){

    $GLOBALS['b_fmt::money'] = ' SEK';
    print x('h2','Budget summary');
    $this->dbg();

    // Use the same header for the summary as for the visitors
    $header = $this->header();
    
    // Get the price of social events    
    $word_socialEvents = '';
    $detailed_socialEvents = False;
    $total_socialEvents = 0;
    if (is_object(VM::$e)){
      VM::$e->socialEvents()->get_budget();
      if (($s = VM::$e->socialEvents()->summary)){
	ob_start();
	$t = new b_table_zebra(array('what'=>' ','skoko'=>' '));
	$t->showLineCounter = False;
	foreach($s as $what=>$v){
	  if (empty($v)) continue;
	  $total_socialEvents += $v;
	  $t->prt(array('what' =>$what,
			'skoko'=>b_fmt::money($v)));
	}
	$t->close();
	$socialEvents = ob_get_contents();
	ob_end_clean();
      }
      if ($total_socialEvents > 0){
	$detailed_socialEvents = True;
	$header['name'] = 'Social events';
	//      $this->scholar_summary['name'] = $socialEvents;
	$word_socialEvents = 'Social events';
	$header['name'] = '';
      }
      
      $this->total['name']     = $total_socialEvents;
      @$this->total['total_e'] += $total_socialEvents;
      @$this->total['total_r'] += $total_socialEvents;
      b_debug::print_r($this->total,'total');
    }

    foreach($this->total as $k=>$v){
      $this->money_bold[] = $k;
      $budget[$k]  = $v;
      $budgetP[$k] = b_fmt::money($v);
      $this->set_colorCodes($k,$budgetP[$k]);
    }

    // 
    // Add details about socialEvents
    //
    if ($detailed_socialEvents){
      $this->money_bold[] = 'name';
      $budgetP['name'] .= x("span font-style:italic'",'<br/><br/>&nbsp;'.$socialEvents);
      $word_socialEvents = 'Social events';
      $header['name'] = '';
      //      $this->set_colorCodes($k,$budgetP[$k]);
    }
    $GLOBALS['b_fmt::money'] = '';

    // 
    // Add details about accommodation  
    //
    foreach($this->ac2h as $hut_code=>$header_element){
      if (!empty($header[$header_element])){
	ob_start();
	if (is_object(VM::$e)){
	  VM::$e->accommodationOptions()->show_usage($hut_code);
	}else{
	  $av_id = @$_GET['host_avid_once'];
	  if (empty($av_id) && is_object($this->av)) $av_id = $this->av->ID;
	  if (!empty($av_id)) bList::getListInstance(myOrg_ID,'bList_vm_accommodationOptions')->show_usage($hut_code,0,0,$av_id);
	}
	$result = ob_get_contents();
	ob_end_clean();
	if (!empty($result)) $budgetP[$header_element] .= '<br><br>N. tenants'.$result;
      }    
    }

    //
    // Add details about scholarships
    //
    foreach(array_keys($this->total) as $k){
      if (!empty($this->scholar_summary[$k])){
	ob_start();
	$t = new b_table_zebra(array('n'=>' ','zone'=>' '));
	$t->showLineCounter = False;
	foreach($this->scholar_summary[$k] as $zone=>$n) $t->prt(array('n' =>$n,
								       'zone'=>$zone));
	
	$t->close();
	$budgetP[$k] .= '<br><br>N.trips'.ob_get_contents();
	ob_end_clean();
      }
    }

    if ($this->bs) print x('h4',(!myPear::is_empty($b=b_fmt::money(VM::$e->budgetSource()->total))
				 ? "Available budget $b".CONST_currency.
				 ($this->something_was_paid
				  ? ''
				  : ", estimated cost ".b_fmt::money(@$budget['total_e']).CONST_currency)  
				 : 'No known budget source'));
    
    ob_start();
    $t = new b_table_zebra($header);
    foreach (array_keys($header) as $k) $t->css[$k]['align'] = 'align_right';    

    $this->set_colorCodesTH($header,$t->th_attr);

    b_debug::print_r($this->header_cache,'header',False);

    $t->preHeaders = $this->build_budget_preHeaders($header,$word_socialEvents);
    $t->noSort = True;
    $t->showLineCounter = False;
    
    foreach($header as $k=>$v) $t->class[$k] = array('highlightWhite'); 
    $t->prt($budgetP);
    
    if (is_object(VM::$e) && !is_object($this->av)){
      if     (VM::$e->isArchived())      $t->extraTD[] = bIcons()->get('b-archive');
      elseif (VM::$e->isEventEndorsed()) $t->extraTD[] = bIcons()->get('b-approved');
      //      $t->prt(array('space0'=>' '));
    }
    $t->close();
    $table = ob_get_contents();
    ob_end_clean();
    print x("div class='messages status-no-image'",$table);
  }

  private function build_mail_preHeaders(&$header){
    return array(array('3:',
		       "2:Password",
		       '1:',
		       "2:Confirmation",
		       '1:',
		       "2:Denial",
		       "1:",
		       '2:Information'));
  }

  private function build_budget_preHeaders(&$header,$header_col0=''){

    // Strip out sequential spaces
    $prev_was_space = False;
    foreach(array_keys($header) as $k){
      if (strpos($k,'space') === False){
	$prev_was_space = False;
      }else{
	if ($prev_was_space) unset($header[$k]);
	$prev_was_space = True;
      }
    }

    $n_Accommodation = count(preg_grep('/living/',array_keys($header)));
    $n_Other         = count(preg_grep('/other/', array_keys($header)));
    $n_Total         = count(preg_grep('/total_/',array_keys($header)));
    $n_Reimbursement = count(preg_grep('/scholarship/',array_keys($header)));
    $n_name = count(array_keys($header)) - $n_Accommodation - $n_Other - $n_Total - $n_Reimbursement - count(preg_grep('/space/',array_keys($header)));
    
    $pre_header = array("$n_name:$header_col0");
    if (isset($header['space0'])) $pre_header[] = "1:";
    if ($n_Accommodation > 0)     $pre_header[] = "$n_Accommodation:Accommodation";
    if (isset($header['space1'])) $pre_header[] = "1:";
    if ($n_Reimbursement > 0)     $pre_header[] = "$n_Reimbursement:Reimbursement";
    if (isset($header['space2'])) $pre_header[] = "1:";
    if ($n_Other > 0)             $pre_header[] = "1:";
    if (isset($header['space3'])) $pre_header[] = "1:";
    if ($n_Total > 0)             $pre_header[] = "$n_Total:<span class='biggerText'>Total</span>";

    return array($pre_header);
  }

  /*
   *
   */
  private function set_backgroundApproved(){
    static $something_printed = 0;
    if ($this->doing == 'myguests') $something_printed++;
    if (is_object(VM::$e) && !is_object($this->av) && !$something_printed++){
      if    (VM::$e->isArchived())      $this->t->pendingObject[] = bIcons()->get('b-archive');
	elseif(VM::$e->isEventEndorsed()) $this->t->pendingObject[] = bIcons()->get('b-approved-vert');
    }
  }
    
  /*
   * Check what is wrong with the visit
   */
  private $no_accommodation = array();
  private $fatal_fnc = array();
  private function collectFNC(){
    static $LODGING_REQ_ONLY = Null;

    if (!VM_administrator_here) return;

    bTiming()->cpu(__FUNCTION__);
    $this->fnc_beslut = bForm_vm_Visit::_formalitiesNotCompleted($this->rec);
    $this->highlight = '';

    // Catch non-resolved accommodation
    if ($LODGING_REQ_ONLY === Null) $LODGING_REQ_ONLY = bForm_vm_Lease::LODGING_REQ_ONLY();
    $is_LODGING_REQ_ONLY = in_array($this->rec['v_id'],$LODGING_REQ_ONLY);
    
    // Highlight the visitors where something is wrong
    if (bForm_vm_Visit::_getClashes($this->rec)){
      list($c,$d) = array('v_status',STATUS_CLASH);
    }elseif($is_LODGING_REQ_ONLY){
      list($c,$d) = array('fnc',     LODGING_REQ_ONLY);
    }elseif (!empty($this->fnc_beslut)){
      list($c,$d) = array('fnc',     $this->fnc_beslut);
    }else{                                      
      list($c,$d) = array('v_status',bForm_vm_Visit::_getStatus($this->rec,$check_clashes=True));
    }
    
    if (empty($c) || empty($d)){
      // strange... TBU
    }else{
      if ($this->highlight = @$this->highlights[$c][$d]['h']){
	b_debug::print_r($this->highlights[$c][$d],"$c $d");
	@$this->collected_highlights[$this->highlights[$c][$d]['z']][$this->highlight]++;
	if (@$this->highlights[$c][$d]['s'] === 'fatal') @$this->fatal_fnc[$this->highlights[$c][$d]['d']]++;
      }
      
      // Highlight visitors without accommodation
      $is_LODGING_REQ_ONLY = False;
      if (($this->rec['v_status'] == STATUS_YES) && (empty($this->rec['lease_aid']) || $is_LODGING_REQ_ONLY)){
	$action_message = ((VM::hasRightTo('book_ah') || $is_LODGING_REQ_ONLY) && $this->rec['v_end']>time()
			   ? 'Please '.($is_LODGING_REQ_ONLY ? 'resolve' : 'select').' accommodation for '
			   : 'Missing accommodation for');
	$this->no_accommodation[$action_message][] = $this->rec['name'];
      }
    }
    bTiming()->cpu();
  }  

  /*
   * Build summary of the non-complete formalities for all visits 
   */
  private function build_FNCsummary(){
    $reply = array();

    ob_start();
    ksort($this->collected_highlights);
    $title = x('strong','Not all the formalities are completed:');
    foreach($this->collected_highlights as $reason=>$info){
      if (!empty($title)) {
	print $title; $title = '';
	$t = new b_table("class='width100'");
      }
      $h = array_keys($info);
      $n = array_values($info);
      $a = '&nbsp;';
      if (in_array($h[0],array('highlightRose','highlightYellow')) && 
	  $this->doing =='lists' && 
	  VM::hasRightTo('endorse_event')){
	$a = 'Please '.(is_object(VM::$e) && VM::$e->isEventEndorsed()?'UNLOCK the budget and':'').
	  ($h[0] == 'highlightRose' ? 'resolve the issue' : 'approve or reject the applicants');
      }	
      $t->tro();
      $t->td($n[0],"class='$h[0] align_right'");
      $t->td(($message=$reason.(($n[0]>1 && strpos($reason,')')===False)?'s':''))."<br/>$a","class='$h[0]'");
      if (!isset($FNC_message)) $FNC_message = "$n[0] $message";
      $t->trc();
    }
    if (empty($title)) $t->close();
    $summary = ob_get_contents();
    ob_end_clean();
    if (isset($FNC_message)) $reply[$FNC_message] = $summary;

    // accommodation
    foreach($this->no_accommodation as $action_message=>$list){
      $list = array_unique($list);
      sort($list);
      $title = ($n=count($list)) . ' approved visitor'.($n>1?'s':'').' without accommodation';
      if ($n > 1){
	$summary = "$action_message:" . x('ul',b_fmt::joinMap('li',$list));
      }else{
	$summary = "$action_message $list[0]";
      }
      $reply[$title] = $summary;
    }
    return $reply;
  }
  
  /*
   *
   */
  private function boss_button(){
    if (VM::e_ID()<=0 || is_object($this->av)) return;

    $this->dbg();
    $to_show = array();
    $e_reg_end = VM::$e->getValue('e_reg_end',1);
    $e_start   = VM::$e->getValue('e_start',1);
    $e_end     = VM::$e->getValue('e_end',1);
    
    VM::$e->budgetSource()->get_budgetSource($default_only=False,$verbose=False);
    $mightBeEndorced = ($e_reg_end < time()) && ($e_end > time()) && (VM::$e->budgetSource()->total > 0);

    switch ($this->doing){
      
    case 'lists':
    case 'budget':
      if ($mightBeEndorced){
	// Build the FNC message if something goes wrong
	$fnc_message = '';
	if (!empty($this->fatal_fnc) && (time()+VM_booking_zapas*24*3600 > $e_start) &&
	    (VM_organizer_here || VM_manager_here || VM::hasRightTo('endorse_event'))){
	  $fatal_p = "style='font-weight:bold;color:red;'";
	  foreach($this->fatal_fnc as $msg=>$n) $li[] = x("li $fatal_p","$n $msg");
	  $fnc_message = (VM::$e->isEventEndorsed()
			  ? (x("p $fatal_p","The event is already approved, however some formalities are NOT completed:".x('ul',join("\n",$li))).
			     x("p $fatal_p","Please unlock the budget and resolve the issue"))
			  : (x("p $fatal_p","It is time now to approve &amp; lock the budget.<br>".
			       "The event starts in ".b_time::delta(time(),$e_start).
			       " and we need time to reserve accommodation for the visitors.").
			     x("p $fatal_p","However, the budget can't be approved since some formalities are NOT yet completed:".x('ul',join("\n",$li))).
			     x("p $fatal_p","Please resolve the issue asap.")));
	  if (time() + VM_booking_zapas*24*3600 < $e_start) $fnc_message = '';
	}
	$this->boss_button_endorse($fnc_message);
      }

    case 'show_mails_exchange':
      
      $this->boss_button_endorse();

      //
      // Big-Button "Send E-mails to all attenders"
      //
      b_debug::print_r($this->mails_which_are_not_yet_sent);
      if ((!empty($this->mails_which_are_not_yet_sent[VM_V_welcomeMailSent])  ||
	   !empty($this->mails_which_are_not_yet_sent[VM_V_denialMailSent]))
	  && (VM::$e->getValue('e_start',1) > time()) 
	  &&  VM::hasRightTo('send_finalWelcomeEmail') 
	  &&  VM::isEventEndorsed()
	  // && !empty($this->collected_highlights)
	  ){ 
	$to_show[] = b_btn::big_button('Send welcome/denial Emails to the attenders',array('mail2all_once'=>1,'resetcache_once'=>1));
      }
      break;
      
    case '2excel':
      //
      // Button "Export as an Excel file
      //
      if(empty($_GET['e_excel_once'])){
	$to_show[] = (VM::$e->isEventEndorsed() || VM::hasRightTo('endorse_event') 
		      ? b_btn::big_button('Export to Excel file',     
					  array('resetcache_once'=>1,'e_excel_once'=>1),'green')
		      : b_btn::big_button('The program is NOT yet approved, no export possible yet',
					  array('resetcache_once'=>0),'blue'));
      }
      break;

    default:
      break;
    }

    if (!empty($to_show)){
      $t = new b_table("style='width:100%'");
      $t->tr($to_show);
      $t->close();
    }
  }

  /*
   *
   */
  private function boss_button_endorse($fnc_message=''){
    static $dejaVu = 0;
    if ($dejaVu++) return;
    
    // Show button "Release lock"
    if (VM::$e->isEventEndorsed()){ 
      if (!empty($fnc_message)) myPear::WARNING($fnc_message);
      if (VM::hasRightTo('endorse_event')) $to_show[] = b_btn::big_button('Unlock the budget & allow changes',array('e_endorse_once'=>0,
														    'resetcache_once'=>1),'red');
      // Show button "Endorse the event and lock changes" 
      // for the events where the applications deadline is over
    }elseif(empty($this->fatal_fnc) && VM::hasRightTo('endorse_event')){
      $message = (VM::$e->getValue('e_start',1) > time()
		  ? '&nbsp;&nbsp;&nbsp;'.b_time::delta(time(),VM::$e->getValue('e_start',1)) . ' until the event starts'
		  : 'By some reason the event is not endorsed');
      $to_show[] = b_btn::big_button("$message. &nbsp;&nbsp;&nbsp; ".
				     'Approve & Lock the budget',array('e_endorse_once'=>1,
								       'resetcache_once'=>1));
      // Warning message for the organizers
    }elseif (!empty($fnc_message)){
      myPear::WARNING($fnc_message);
    }
  }
  
  /*
   *
   */
  private function help_text(){

    $reply = array();
    if (VM_administrator_here && 
	!VM::outOfScope() && 
	!VM::isArchived() && !is_object($this->av))  switch($this->doing){
	
      case 'show_mails_exchange':
	$reply['How to see/preview/send mails to the attenders'] =
	  join('<br/>',array('This page shows status of the mail exchange with the applicants.',
			     '',
			     'All the applicants get 2 Emails from the system:'.
			     x('ul',
			       x('li','Informational email, sent after the application form submittion').
			       x('li','"Welcome email", sent by the '.myOrg_name.' staff shortly after endorsing of the program budget'.
				 x('ul',
				   x('li','EITHER the confirmation email with all the needed information if application was successful').
				   x('li','OR a refusal email if the application was denied')))).
			     '',
			     'From this page you can see which emails were already sent out, and preview the emails which are in the queue.'					
			     ));
	break;

      case 'lists':	
      case 'budget':
	
	// See what is not yet done
	$dejaVu = (VM::hasRightTo('endorse_event') && is_object(VM::$e) ? 0 : 99);
	if (VM::isEventEndorsed())                                     $dejaVu++;    
	if (is_object(VM::$e) && VM::$e->getValue('e_end',1) < time()) $dejaVu++;
	foreach($this->build_FNCsummary() as $key=>$text){
	  if (!$dejaVu++)
	    $text .= join('<br/>',
			  array('',
				'Note, you can "approve" and/or "reject" any applicant as many times as you need in order to adjust the budget. ',
				'The main parts of the budget are accommodation and travel costs. Please select the accommodation with a "pop-menu" and eventually the travel reimbursement with button'.
				bIcons()->get(array('i'=>'trip_on','d'=>'toggle the trip reimbursement')).'.',
				'',
				'Only when you are happy with the selection AND the event budget is approved by the '.myOrg_name.' administration, '.
				'the applicants will be informed about the decision.'));
	  $reply["ATTENTION: $key"] = $text;
	}
      default:
      }
    $this->dbg(count(array_keys($reply)).' lines');
    return $reply;
  }
  

  /*
   *
   */
  function query(){

    if ($this->current_tab_does_not_need_applicants()) return myPear_db()->query("SELECT v_id FROM abs_visits WHERE 0");

    $where = array(1);
    if (is_object($this->av)){
      $this->dbg("Doing ".$this->av->name());
      $_GET['sBy']    = 'v_start';
      $_GET['sByDir'] = 'DESC';
      $where[] = "av_id = ".$this->av->ID;
    }else{
      if ($v_host_avid = @$_GET['host_avid_once']){
	$where[] = "v_host_avid = $v_host_avid";
      }elseif(VM::$e){
	$where[] =  "e_id = ".VM::$e->ID;
      }else{
	if (!defined('vm_day1')) b_debug::internalError("'vm_day1' is not defined");
	$where[] = "v_end   >= ".vm_day1;
	$where[] = "v_start <= ".vm_day9;
      }
      
      switch ($this->doing){
      case 'doorLabels':   
	return VM_query($where, "LEFT JOIN abs_offices ON o_id = v_oid");
	
      case 'lists':
	$_GET['sBy']   = (split_by_huts
			  ? 'v_status,hut_code,hut_name,a_name,av_lastname,av_id'
			  : 'av_lastname,lease_start');
	$_GET['sByDir']= 'ASC';
	return VM_query($where);
	
      case '2excel':
	$_GET['sBy']   = 'hut_code,hut_name,a_name,av_lastname,av_id';
	$_GET['sByDir']= 'ASC';
	return VM_query($where);
	
      case 'myguests':
	$_GET['sBy']   = 'v_start';
	$_GET['sByDir']= 'DESC';
	$where[] = "(v_eid = 0 OR v_eid IS NULL)";
	break;
	
      case 'budget_byProjects':
	$_GET['sBy']   = 'v_projectid,av_lastname';
	$_GET['sByDir']= 'ASC';
	$where = array();
	$where[] = "(v_eid = 0 OR v_eid IS NULL)";
	$where[] = 'v_start > '.vm_day1;
	$where[] = 'v_start < '.vm_day9;
	break;
	
      case 'reception_attenders':
	$_GET['sBy']    = 'av_lastname';
	$_GET['sByDir'] = 'ASC';
	$where[] = "v_end   >= ".b_time::txt2unix($_GET['d_once']);
	$where[] = "v_start <= ".b_time::txt2unix($_GET['d_once']);
	
      case 'budget':
      case 'show_mails_exchange':
      default:    
	$_GET['sBy']   = 'name';
	$_GET['sByDir']= 'ASC';
	if (!VM::$e) $where[] = "(v_eid = 0 OR v_eid IS NULL)";
      }
    }
    return VM_query($where,'',$this->debug);
  }
  
  /*
   * Format the current record.
   * Optionally save it to a buffer and dot not print.
   */
  private $VM_reimberser_open = False;
  private $scholar_summary = array();
  function format(){

    bTiming()->cpu(__FUNCTION__);

    // Decode the default event policy and fix it on the fly if necessary    
    if (empty($this->rec['v_eid'])){
      $this->rec['e_v_policy'] = VM_visit_policies($this->rec['v_type']);
    }else{
      if (empty($this->rec['e_v_policy'])){
	$e = myPear::getInstance('bForm_vm_Event',$this->rec['v_eid']);
	$this->rec['e_v_policy'] = $e->get_e_v_policy();
      }else{
	$this->rec['e_v_policy'] = b_fmt::safe_decode($this->rec['e_v_policy']);
      }
    }

    // get the table class
    if (empty($this->t) || !($this->t instanceof b_table)){
      $this->t = new b_table_zebra($this->header(False));
    }
    
    if (!$this->current_tab_does_not_need_buttons()){
      // Replace the host av_id by the first/last name
      if ((int)$this->rec['v_host_avid'] > 0){
	$host = new bForm_Avatar($this->rec['v_host_avid']);
	$this->rec['v__host'] = x('em',$host->fmtName('fl'));
      }
      
      
      for($n=0; $n<4; $n++){
	$this->t->class["space$n"] = array('highlightWhite'); 
	$this->rec["space$n"] = '';
      }
      
      // Cut the too long names
      foreach(array('av_firstname'=>1,
		    'av_city'=>1,
		    'av_institute'=>3) as $i=>$l){
	$n = explode(' ',$this->rec[$i],$l+1);
	unset($n[$l]);
	$this->rec[$i] = join(' ',$n);
      }
      if (@$this->rec['a_area'] > 900) $this->rec['a_area'] = '';
      
      // Get to visit summary by clicking on the visitor name
      $this->rec['av_Lf'] = $name = bForm_Avatar::_fmtName('Lf',$this->rec);
      if (in_array($this->rec['av_id'],$this->organizers)) $name = x("span class='boldText'","$name (org)");
      if (1) $this->rec['name'] = bJS()->modal_iframe("?mode_once=RO&id=".$this->rec['v_id']."&form=vm_Visit",$name);
      else   $this->rec['name'] = $name;
      
      // Check non-completed formalities
      $this->collectFNC();
      
      // Preset the table style
      foreach($this->header(False) as $k=>$v) unset($this->t->class[$k]);
      for($n=0; $n<4; $n++){
	$this->t->class["space$n"] = array('highlightWhite'); 
	$this->rec["space$n"] = ' ';
      }
    }

    switch ($this->doing){
    case '2excel':
      /*
       * --------------------------------------------------------------------------------- Export list of tenants for the hotels administration
       */

      $this->t->showLineCounter = False;

      if ($v_projectid = $this->rec['v_projectid']){
	$project = VM_projects()->getMember($v_projectid);
	$this->rec['v_projectid'] = $project['lm_value'];
      }

      $v_id = $this->rec['v_id'];
      $this->rec['lease_start'] = date('Y-m-d',$this->rec['lease_start']);
      $this->rec['lease_end']   = date('Y-m-d',$this->rec['lease_end']);
      $this->rec['av_lastname'] = mb_strToUpper($this->rec['av_lastname']);
      $this->rec['av_firstname']= b_fmt::ucWords($this->rec['av_firstname']);
      $this->rec['a_comment'] = preg_replace('/ - .*/','',$this->rec['a_comment']);
      $this->t->extraTD[] = $this->rec['a_comment'];

      // Split the list by huts
      $hut_name = $this->split_by_huts($this->rec);

      if ($cotenants = VM_cotenants()->get_cotenants($this->rec['lease_id'])){
	$names = array();
	foreach($cotenants as $lm_id=>$leases){
	  foreach($leases as $lease_id){
	    $l = new bForm_vm_Lease($lease_id);
	    if (!$l->getValue('lease_vid',1)){
	      if (cnf_dev) b_debug::xxx("parentForm=$l->parentForm lease=$l ");
	      if (cnf_dev) b_debug::print_r($l->formDB);
	      if (cnf_dev) b_debug::print_r($this->rec);
	      continue;
	    }
	    $l->hookVisit();
	    $names[] = $l->name();
	  }
	}
	$this->rec['cotenant'] = b_crypt::encode_utf8(b_fmt::unEscape(join(', ',$names)));
      }
      
      if ($this->rec['hut_code'] != HUT_BI) $this->fill_excel($hut_name);
      break;

    case 'photos':
      /*
       * --------------------------------------------------------------------------------- Collect photos
       */
      static $dejaVu = array();
      $av = myPear::getInstance('bForm_Avatar_vm',$this->rec['av_id'],array('nocache','strict'));
      if (!@$dejaVu[$av->ID]++){
	if (!($img = $av->getPhoto())) $img = "<img src='".myPear_images."avatar.png'>"; 
	$this->labels[] = array($img,
				x('strong',bForm_Avatar::_fmtName('sfXL',$this->rec)),
				x('em',b_cc::getName($this->rec['av_residentship'])),
				'<br/>'
				);
      }
      $this->rec = array();
      break;
      
    case 'badges':
      /*
       * --------------------------------------------------------------------------------- Badges
       */
      $this->labels[] = array(x('strong',bForm_Avatar::_fmtName('sfL',$this->rec)),
			      '&nbsp;',
			      $this->rec['av_institute'],
			      $this->rec['av_city'],
			      b_cc::getName($this->rec['av_residentship']),
			      );
      $this->rec = array();
      break;
     
    case 'doorLabels':
      /*
       * --------------------------------------------------------------------------------- Door labels
       */
      if ($o=$this->rec['v_oid']) {
	if (empty($this->labels[$o])){
	  $oc = myPear::getInstance('bForm_vm_Office',$o);
	  $this->labels[$o][] = x('h1',$oc->name(False));
	}
	$this->labels[$o][] = $this->rec['name'];
      }
      $this->rec = array();
      break;
      
    case 'budget':
      /*
       * --------------------------------------------------------------------------------- Budget estimation
       * Do not bother if the event has no budget, just return
       */
      if ($this->event_is_bankrupt){
	$this->rec = array();
	bTiming()->cpu();
	return;
      }
    case 'budget_byProjects':
    case 'myguests':
      

      $this->exp = Null;

      // The policy value might be auto-adjusted by the Visit class,
      // so it is safer to use the fresh value
      if (empty($this->v)) $this->v = new bForm_vm_Visit($this->rec['v_id']); 
      $this->rec['v_policy'] = $this->v->getValue('v_policy',1,1);
      $this->rec['period']   = x('em',b_time::period($this->rec['v_start'],$this->rec['v_end']));
      $this->t->class['period'] = 'b_nowrap';
      
      // Careful about changes of the lease during the visit, do not count the visit twice
      if ($this->visit_ID != $this->rec['v_id']){
	if (empty($this->rec['scholarship_e'])) $this->rec['scholarship_e'] = 0;
	if (empty($this->rec['scholarship_r'])) $this->rec['scholarship_r'] = 0;
	foreach(array('scholarshipT'=>array('f'=>'exp_travel',
					    'p'=>VM_V_payTrip),
		      'scholarshipP'=>array('f'=>'exp_perdiem',
					    'p'=>VM_V_payPerdiem)
		      ) as $pay=>$d){
	  $to_pay = bForm_vm_Visit::_getPolicy($d['p'],$this->rec,True);
	  $this->dbg($this->rec['av_lastname'].": to pay $d[f] - ".($to_pay?'YES':'NO'));
	  if ($to_pay){
	    if ($this->exp === Null) $this->exp = $this->v->getExp();
	    $this->rec[$pay.'_r'] = $this->exp->getValue($d['f'],       1,1); // really payed
	    $this->rec[$pay.'_e'] = $this->exp->getValue($d['f'].'_est',1,1); // Organizer estimate
	    if(empty($this->rec[$pay.'_e']))$this->rec[$pay.'_e'] = $this->exp->{$pay}(False,True);  // Default value
	    
	    $this->rec['scholarship_e']  += $this->rec[$pay.'_e'];
	    $this->rec['scholarship_r']  += $this->rec[$pay.'_r'];
	    
	    foreach(array('_r','_e') as $z){
	      if (empty($this->rec[$pay.$z])) continue;
	      if ($d['p'] == VM_V_payTrip){
		$reimbursementRates = ($this->bList_parent == myOrg_ID
				       ? bList::getListInstance(myOrg_ID,'bList_vm_reimbursementRates')
				       : VM::$e->reimbursementRates());
		$reimbursementRates->get_rates($this->rec);
		@$this->scholar_summary[$pay.$z][trim(str_ireplace('from','',$reimbursementRates->tripCostFromResidentshipCountry['zone']))]++;
	      }
	    }
	    
	    // Let the secretary introduce the payed amount
	    if (VM_reimberser_here && (stripos($this->doing,'budget')!==False)){
	      if (empty($this->rec[$pay.'_r'])){
		if ($this->header_counters === Null) $this->header(False);
		$this->rec[$pay.'_r'] = "<input style='text-align:right;' size=4 type='text' name='".join(':',array('R',$d['f'],$this->rec['v_id']))."'/>\n";
		$this->VM_reimberser_open = True;
		$this->header_counters[$pay.'_r']++;
	      }
	    }
	  }
	}
      }

      if ($lease_aid = $this->rec['lease_aid']){
	$hut_code = bForm_vm_Lease::_hut_code($this->rec);
	if ($pay = @$this->ac2h[$hut_code]){
	  if ($this->exp === Null) $this->exp = $this->v->getExp();
	  $this->rec[$pay] = $this->exp->living_ABC($this->rec['lease_id'],$hut_code);
	  if (empty($pay)) print $hut_code;
	  @$this->rec['living'] += $this->rec[$pay];
	  
	  $debit_credit = @VM::$description[$this->rec['v_type']]['f'];
	}
      }
      
      $this->printPolicies();
      $this->rec2buffer();
      $this->v->__clean_cache();
      break;
      
    case 'show_mails_exchange':
      /*
       * --------------------------------------------------------------------------------- Mails...
       */
      // Replace the "b_table" by a standard class to avoid printing the table
      $this->t = new stdClass();
      $this->t->extraTD = array();
      $this->t->class   = array();
      
      //
      // See the mails (to be) sent,
      // start from the informational one (which might be send at any time, disregarding the visit status),
      // then the password reminder for the organizers
      //
      if (in_array($this->rec['av_id'],$this->organizers)){
	$this->get_sendMail_buttons(VM_V_rdlMailSent,'vm_mail_pwd','password reminder','m_pwd');
      }else{
	$this->get_sendMail_buttons(VM_V_infoMailSent,'vm_mail_info','general information','m_info');
      }
      
      //
      // Now the confirm/deny
      //
      switch(bForm_vm_Visit::_getStatus($this->rec)){
      case STATUS_NO:
	$this->rec['status'] = $this->say('Ni');
	$this->get_sendMail_buttons(VM_V_denialMailSent,'vm_mail_no','DENIAL','m_no');
	$this->t->class['name'] = array('overstrike','redText');
	break;
	
      case STATUS_YES:
	$this->rec['status'] =  $this->say('Yi');
	$this->get_sendMail_buttons(VM_V_welcomeMailSent,'vm_mail_yes','APPROVAL','m_yes');
	break;	
	
      default:
	$this->rec['status'] = $this->say('Pi');
	break;
      }
      
    case 'lists':
      // Check non-completed formalities
      //      $this->collectFNC();
      
    case 'endorsing':
    case 'reception_attenders':
      /*
       * --------------------------------------------------------------------------------- Various lists
       */
      
      $this->printPolicies();
      
      // Strike out the refused applicants
      if (bForm_vm_Visit::_getStatus($this->rec)==STATUS_NO){
	foreach(array_keys($this->header(False)) as $k) $this->t->class[$k] = array('overstrike','redText');
      }
      
      // Do the record formatting
      $this->t->class['v__stay'][] = 'b_nowrap';
      if(@$this->rec['lease_start']) $this->rec['v__stay'] = x('em',b_time::period($this->rec['lease_start'],$this->rec['lease_end']));
      else                           $this->rec['v__stay'] = x('em',b_time::period($this->rec['v_start'],$this->rec['v_end']));
      $this->rec['_e_name'] = b_fmt::strcut($this->rec['e_name'],25,'...');
      $this->rec['_a_name'] = $this->select_accommodation();
      $this->rec['_o_name'] = @$this->rec['o_name'];
      $this->rec['space1']  = ' ';
      if (time() < $this->rec['e_end']){
	$this->rec['cotenant']= x('em',bForm_vm_Lease::select_cotenant('_c_id:',$this->rec,(VM::hasRightTo('book_ah')?'auto':'RO')));
      }      

      if ($this->av) $this->rec['name'] = $this->rec['_e_name'];
      if (empty($this->rec['name'])) $this->rec['name'] = VM::$description[$this->rec['v_type']]['d'];

      // Save the record to the buffer
      $this->rec2buffer();
    } 
        
    // clean memory
    if (is_object($this->v)) $this->v->__clean_cache(); 
    bTiming()->cpu();
  }

      /*
       * Helper, split the list by huts
       */
  private function split_by_huts($rec){
    static $hut_name = '';

    if (!split_by_huts) return '';

    switch($status=bForm_vm_Visit::_getStatus($rec)){
    case STATUS_NO:  
      $rec_hut_name = '_REFUSED_applications'; 
      break; 
    case STATUS_YES: 
      // if (empty($rec['hut_code']) && cnf_dev) myPear::DEBUG("?? Empty hut_code for ".$rec['av_lastname']);
      if (empty($rec['hut_name'])) $rec['hut_name'] = '_missing accommodation';
      $rec_hut_name = (@$rec['hut_code']===HUT_BI ? $rec['a_name'] : $rec['hut_name']);
      break;
    case STATUS_PENDING: 
    default:
      $rec_hut_name = '_PENDING_applications'; 
    }

    if ($rec_hut_name != $hut_name){
      $hut_name = $rec_hut_name;
      $this->t->td(x('h3',$hut_name),'colspan=5');
    }
    return $hut_name;
  }
  
  /*
   * Helper
   */  
  private function fill_excel($hut_name){
    static $export_data = array('v_projectid' => 'Project',
				'av_lastname' => 'Name',
				'av_firstname'=> '',
				'av_email'    => 'Email',
				'av_city'     => 'City',
				'lease_start' => 'Arrival',
				'lease_end'   => 'Departure',
				'cotenant'    => 'Co-tenant',
				'a_area'      => 'Rum (kvm)',
				'a_comment'   => '');
    $data = $header = array();
    foreach(array_keys($export_data) as $i){
      $data[] = @$this->rec[$i];
      $header[] = $export_data[$i];
    }
    if (empty($this->excel_input[$hut_name])) $this->excel_input[$hut_name][] = $header;
    $this->excel_input[$hut_name][] = $data;
  }
  
  /*
   *
   */
  private $mails_which_are_not_yet_sent = array();
  private function get_sendMail_buttons($policy,$id,$title,$vm_f){

    // Count the mails which are still in a queue
    if (!($already_sent = (bool)bForm_vm_Visit::_getPolicy($policy,$this->rec,True))){
      if (empty($this->mails_which_are_not_yet_sent[$policy])) $this->mails_which_are_not_yet_sent[$policy] = 0; 
      $this->mails_which_are_not_yet_sent[$policy]++;
    }
    
    // Get the button
    $rec = array();
    if ($already_sent === True){
      //
      // The mail was sent, print "see email"
      //
      $rec["${vm_f}_1"] = ICON_mail_sent_y;
      $rec["${vm_f}_2"] = $this->sendMail_btn($id,'',True,$already_sent);
      
    }elseif($already_sent === False && ($this->rec['v_end'] > time())){
      //
      // The mail was NOT sent, print "send/preview email".
      // Note, there is no reason to send the "information" e-mail after the event is endorsed.
      //
      switch(VM::isEventEndorsed()){
      case True:
	if ((send_finalWelcomeEmail_to_registrants && ($vm_f=='m_yes')) ||
	    (send_denialEmail_to_registrants       && ($vm_f=='m_no'))) $rec["${vm_f}_2"] = $this->sendMail_btn($id,$title,!VM::hasRightTo('send_finalWelcomeEmail'),$already_sent);
	break;
      case False:
	$rec["${vm_f}_2"] = $this->sendMail_btn($id,$title,($policy != VM_V_infoMailSent),$already_sent);
      }
    }
    $this->dbg(join(', ',array_keys($rec)).', already_sent='.var_export($already_sent,True));
    // b_debug::var_dump($rec);
    if (!empty($rec)) $this->rec = array_merge($rec,$this->rec);
  }      

  /*
   *
   */  
  private function sendMail_btn($id,$title,$preview_only,$already_sent){

    $text = (empty($title) 
	          ? 'See the email' 
	          : "Preview ".($preview_only?'':'/ Send')." the $title mail");
    $args = array('mail_once'      =>$id,
		  'v_id'           =>$this->rec['v_id'],
		  'hut_id'         =>$this->rec['hut_id'],
		  'a_name'         =>$this->rec['a_name'],
		  'resetcache_once'=>1,
		  'preview_once'   =>$preview_only,
		  );
    $reply = (modal_sendmail_dialog
	      ? bJS()->modal_iframe(b_url::same($args),
				    bIcons()->get(array('d'=>$text,
							'i'=>($already_sent?'i-mail-open':'i-mail-send'))))
	      : bIcons()->getButton(array('d'=>$text,
					  'i'=>($already_sent?'i-mail-open':'i-mail-send'),
					  'l'=>b_url::same($args)))
	      );
    return $reply;
  }
  
  /*
   * Save the current record into a buffer instead of printing it.
   * The print will be done later after analyzing the complete sample.
   */
  private function rec2buffer(){
    static $dejaVu_visit = array();

    if ($this->keep_thisRec) return;

    bTiming()->cpu(__FUNCTION__);
    switch($this->doing){

    case 'budget_byProjects':
      $this->rec_project[$this->rec['v_projectid']][] = $this->rec['v_id']; 
    case 'show_mails_exchange':
    case 'budget':
      $this->visit_ID = $this->rec['v_id'];
      break;

    case 'myguests':
    default:
      if (empty($this->rec['lease_id'])) $this->visit_ID = $this->rec['v_id'];
      else                               $this->visit_ID = $this->rec['lease_id'];
      if (@$dejaVu_visit[$this->rec['v_id']]++) $this->t->extraTD = array();
    }
    if (empty($this->visit_ID))         $this->visit_ID = $this->rec['v_id'];

    // Combine all the leases into one record per visit
    if (empty($this->rec_visit[$this->visit_ID])){
      $this->rec_visit[$this->visit_ID]['v_status'] = @$this->rec['v_status'];
      $this->rec_visit[$this->visit_ID]['total_e']  = @$this->rec['other'];
      $this->rec_visit[$this->visit_ID]['total_r']  = @$this->rec['other'];
      $this->rec_visit[$this->visit_ID]['all_buttons']  = $this->t->extraTD;
      $this->rec_visit[$this->visit_ID]['all_classes']  = $this->t->class;
      $this->rec_visit[$this->visit_ID]['all_highlight']= $this->highlight;
    }
    
    foreach ($this->rec as $k=>$v){
      if (myPear::is_empty($v)) continue;
      if (preg_match('/(living|scholarship)[A-Z]/',$k)){
	if (empty($this->rec_visit[$this->visit_ID][$k])) $this->rec_visit[$this->visit_ID][$k] = 0;
	if (is_numeric($v)){
	  $this->rec_visit[$this->visit_ID][$k] += $v;
	  $this->item2total($k,$this->rec_visit[$this->visit_ID],$v);
	}else{
	  $this->rec_visit[$this->visit_ID][$k] = $v;
	}
      }elseif (preg_match('/(living|scholarship)/',$k)){
	continue;
      }else{
	$this->rec_visit[$this->visit_ID][$k]  = $v;
      }
    }
    $this->set_header_counters();
    $this->rec = array();
    bTiming()->cpu();
  }
  
  private function set_header_counters(){
    if ($this->header_counters === Null) $this->header();
    foreach($this->rec_visit[$this->visit_ID] as $k=>$v){
      if (isset($this->header_counters[$k]) && !empty($v)) ++$this->header_counters[$k];
    }
  }

  private function item2total($item,&$total,$value,$code='total'){
    if (!empty($value) && is_numeric($value)){
      if (empty($total["${code}_r"]))  $total["${code}_r"] = 0;
      if (empty($total["${code}_e"]))  $total["${code}_e"] = 0;
      
      if (preg_match('/(_r|_e)$/',$item,$m)){
	$total[$code.$m[1]] += $value;
      }else{
	$total["${code}_e"] += $value;
	$total["${code}_r"] += $value;
      }
    }
  }

  private function set_colorCodes($item,&$value){
    $c = array();
    if (in_array($item,$this->money_bold))  $c[] = 'boldText';
    if ($value < 0)                         $c[] = VM_color_budgetCredit;
    elseif(in_array($item,$this->money_est))$c[] = VM_color_budgetEstimate;
      if (stripos($item,'total') !== False) $c[] = 'biggerText';
    if (!empty($c)){
      $value = x("span class='".join(' ',$c)."'",$value);
    }
  }

  private function set_colorCodesTH($header,&$th){
    foreach (array_keys($header) as $k){
      $c = array('align_right');
      if (in_array($k,$this->money_est))  $c[] = VM_color_budgetEstimate;
      if (stripos($k,'total') !== False)  $c[] = 'biggerText';
      $th[$k] = "class='".join(' ',$c)."'"; 
    }
  }

  /*
   *
   */
  private function getGSF($what='.*',$include_accommodation_wish=False){
    $reply = '';
    if (VM_administrator_here){
      $reply = bForm_vm_Visit::_getGSF($what,$this->rec,'li');
      if ($include_accommodation_wish && !empty($this->rec['v_comment']))    $reply[] = x('li',$this->rec['v_comment']);
      if (empty($reply)) $reply = '';
      else $reply = x('ul style="font-size:smaller"',join(' ',$reply));
    }
    return $reply;
  }

  private function select_accommodation(){
    static $n_select = 0;
    
    if (VM::outOfScope()){
      $reply = $this->getGSF('.*');
    }elseif ($a = @$this->rec['a_name']){
      $reply = $a . str_ireplace('<br>',' ',str_ireplace('<br/>',' ',$this->getGSF('share|travel.*support|accompan')));
    }elseif ($this->rec['v_end'] < time()){
      $reply = '';
    }elseif(bForm_vm_Visit::_getStatus($this->rec) != STATUS_YES){
      $reply = $this->getGSF('.*',VM::e_ID());
    }elseif (is_object(VM::$e) && VM::hasRightTo('book_ah')){
      $rooms = bForm_vm_Lease::_freeRooms(VM::$e->accommodationOptions(),$this->rec['v_start'],$this->rec['v_end']);
      if (myPear::is_empty($n_select=@$this->rec['lease_id']))  $n_select = $this->rec['v_id'];
      VM::$formDummy->submitOnChange["_a_id:$n_select"] = True;
      $reply =  x("span class='only_online'",
		  x("form action='".b_url::same()."' method='post' enctype='multipart/form-data' name='ACC$n_select'",
		    VM::$formDummy->selectMenu_exec("_a_id:$n_select",$rooms,'',$onlyRW=True).
		    "<input type='hidden' name='lease_id' value='".$this->rec['lease_id']."'/>".
		    "<input type='hidden' name='v_id'     value='".$this->rec['v_id']."'/>".
		    "<noscript><input type='submit' value='go'/></noscript>"),
		  'POST');
      $reply .= $this->getGSF('share|accompan',VM::e_ID());
    }else{
      $reply = b_fmt::redText('no accommodation'); 
    }
    return $reply;
  }  

  /*
   * Print approve/reject button, auxiliary to function 'btn_approve_reject'
   */
  private function say($say){
    static $cache = array();

    // Convert buttons to info icons for the events in the past
    if ($this->rec['v_start'] < time()){
      switch($say){
      case 'Y':       // pending visit
      case 'N':       // pending visit
	$say = 'Pi';
	break;
      case 'YN':      // accepted visit
	$say = 'Yi';
	break;
      case 'NY':      // rejected visit
	$say = 'Ni';
	break;
      }
    }
    
    // Get the buttons & icons
    if (!isset($cache[$say])){
      switch($say){
      case 'Y':       // pending visit
	return bIcons()->getButton(array('class'=>'page_load_progress',
					 'd'=>'Click to ACCEPT the application',
					 'c'=>'only_online',
					 'i'=>'i-finger_up',
					 'l'=> b_url::same("?resetcache_once=1&approve_once=".$this->rec['v_id'])));
      case 'N':       // pending visit
	return bIcons()->getButton(array('class'=>'page_load_progress',
					 'd'=>'Click to DENY the application',
					 'c'=>'only_online',
					 'i'=>'i-finger_down',
					 'l'=> b_url::same("?resetcache_once=1&refuse_once=".$this->rec['v_id'])));
      case 'YN':      // accepted visit, toggle
	return bIcons()->getButton(array('class'=>'page_load_progress',
					 'd'=>'Application accepted, click to DENY',
					 'i'=>'i-accepted_yes',
					 'l'=> b_url::same("?resetcache_once=1&refuse_once=".$this->rec['v_id'])));
	
      case 'NY':      // rejected visit, toggle
	return bIcons()->getButton(array('class'=>'page_load_progress',
					 'd'=>'Application denied, click to ACCEPT',
					 'i'=>'i-accepted_no',
					 'l'=> b_url::same("?resetcache_once=1&approve_once=".$this->rec['v_id'])));
	
	
      case 'Yx':      // pending visit, example
	$cache[$say] = bIcons()->getButton(array('d'=>'Click to ACCEPT the application',
						 'i'=>'i-finger_up'));
	break;
	
      case 'Nx':      // pending visit, example
	$cache[$say] = bIcons()->getButton(array('d'=>'Click to DENY the application',
						 'i'=>'i-finger_down'));
	break;
	
      case 'Pi':      // pending visit, info
	$cache[$say] = bIcons()->get(array('d'=>'Pending application',
					   'i'=>'i-avatar'));
	break;
	
      case 'Yi':      // accepted visit, info
	$cache[$say] = bIcons()->get(array('d'=>'Application accepted',
					   'i'=>'i-accepted_yes'));
	break;
	
      case 'Ni':      // rejected visit, info
	$cache[$say] = bIcons()->get(array('d'=>'Application denied',
					   'i'=>'i-accepted_no'));
	break;

      default:
	b_debug::internalError('?');
      }
    }
    return $cache[$say];
  }
  
  /*
   * Reject / Approve the applicant
   * @return array of 2 buttons/icons (yes/no)
   */
  private function btn_approve_reject(){

    // Skip the unduly cases
    if ($not_my_case = (VM::outOfScope()
			|| ($this->rec['v_type'] == VISIT_OUT_OF_SCOPE) 
			|| !in_array($this->doing,array('lists','budget',)) 
			)){
      $this->dbg('not my case? '.($not_my_case ? 'YES' : 'NO'));
      return array();
    }

    // The event is endorsed (i.e. changes are locked), show only pending applicants and return
    if (VM::isEventEndorsed() || ($this->rec['v_end'] < time())){
      if (False){ // ? Strange... To be changed?
	$reply = array('');
	if ($this->fnc_beslut) $reply[] = $this->highlights['fnc'][$this->fnc_beslut]['i'];
	else                   $reply[] = '';
	if (stripos(serialize($reply),'send mail') !== False) var_dump('fnc_beslut',$this->fnc_beslut,date('Y-m-d',$this->rec['v_end']),$reply);
	if (stripos(serialize($reply),'send mail') !== False) b_debug::internalError();
      }else{   
	$reply = array('','');
      }
      return $reply;
    }

    bTiming()->cpu(__FUNCTION__);

    // 
    // See what should we do, depends on the visit status   
    //
    $reply = array('','');
    $v_status = bForm_vm_Visit::_getStatus($this->rec,$check_clashes=True);
    switch($v_status){
      
    case STATUS_CLASH:
      break;

    case STATUS_NO:      
      $this->rec['_a_name'] = $this->rec['_o_name'] = '';
      switch ($this->rec['v_type']){
      case VISIT_TYPE_RENT:
      case VISIT_OUT_OF_SCOPE:
	break;
	
      case VISIT_TYPE_OTHER:
      case VISIT_TYPE_COLLABORATION:
	if (empty($this->v)) $this->v = new bForm_vm_Visit($this->rec['v_id']); 
	$reply = (VM::hasRightTo('approve_guest_invitation',$this->v)
		  ? array('','') 
		  : array('',$this->say('Ni')));
	break;
	
      case VISIT_TYPE_PROGRAM: 
	$reply = (VM::hasRightTo('approve_registrant_application')
		  ? array('',$this->say('NY'))
		  : array('',$this->say('Ni')));
      }
      break;
      
    case STATUS_PENDING:
      if (@$this->rec['_a_name'] !== LODGING_OA_TXT) $this->rec['_a_name'] = $this->rec['_o_name'] = '';
      switch ($this->rec['v_type']){
      case VISIT_TYPE_RENT:  // auto-accept the rents
	if (empty($this->v)) $this->v = new bForm_vm_Visit($this->rec['v_id']); 
	$this->v->setStatus(STATUS_YES);
	$this->v->setPolicy(VM_V_welcomeMailSent,True,False,True);
	$this->rec['v_status'] = STATUS_YES;
	break;
	
      case VISIT_OUT_OF_SCOPE:
	break 2;
	
      case VISIT_TYPE_OTHER:
      case VISIT_TYPE_COLLABORATION:
	if (empty($this->v)) $this->v = new bForm_vm_Visit($this->rec['v_id']); 
	$reply = (VM::hasRightTo('approve_guest_invitation',$this->v)
		  ? array('','') 
		  : array('',$this->say('Pi')));
	break;
	
      case VISIT_TYPE_PROGRAM: 
	$reply = (VM::hasRightTo('approve_registrant_application')
		  ? array($this->say('Y'),
			  $this->say('N'))
		  : array('',$this->say('Pi')));
	break;
	
      default:
	b_debug::internalError('?? v_type="'.$this->rec['v_type'].'"');
      }
      break;
      
    case STATUS_YES:
      switch ($this->rec['v_type']){
      case VISIT_OUT_OF_SCOPE:
	break;

      case VISIT_TYPE_RENT:
	if (empty($this->v)) $this->v = new bForm_vm_Visit($this->rec['v_id']); 
	$this->v->setPolicy(VM_V_welcomeMailSent,True,False,True);
	$this->rec['v_policy'] = $this->v->getValue('v_policy',1,1);
	break;

      case VISIT_TYPE_OTHER:
      case VISIT_TYPE_COLLABORATION:
	if (!$this->v) $this->v = new bForm_vm_Visit($this->rec['v_id']); 
	$reply = (VM::hasRightTo('approve_guest_invitation',$this->v)
		  ? array('','') 
		  : array('',$this->say('Yi')));
	break;
	
      case VISIT_TYPE_PROGRAM: 
	$reply = (VM::hasRightTo('approve_registrant_application')
		  ? array('',$this->say('YN'))
		  : array('',$this->say('Yi')));
	break;
      }
    }
    bTiming()->cpu();
    return $reply;
  }


  /*
   * To pay or not to pay the scholarship
   * @return array of 2 icons: trip & perdiem
   */
  private function btn_payments(){
    $reply = array('','');
    if ($not_my_case = VM::outOfScope()
	|| ($this->rec['v_type'] === VISIT_OUT_OF_SCOPE) 
	|| ($this->rec['v_status']!= STATUS_YES)         
	|| !in_array($this->doing,array('lists','budget',)) 
	|| (is_object(VM::$e) && VM::$e->isEventEndorsed())
	){
      //      $this->dbg('not_my_case '.$this->rec['av_lastname']);
      return array();
    }

    bTiming()->cpu(__FUNCTION__);
    $reply = array();
    foreach(bForm_vm_Visit::_getPolicies($this->rec) as $p=>$descr){
      if (empty($descr['i'])) continue; 
      $yes_no = (bool)bForm_vm_Visit::_getPolicy($p,$this->rec);
      $this->dbg($yes_no);
      // don't draw useless button for the passed events
      if (($yes_no || ($this->rec['v_end'] > time()-30*86400))){
	$already_payed = Null;
	switch($p){
	case VM_V_invoiceSent:
	  if (VM_reimberser_here)
	    $icon=bIcons()->getButton(array('i'=>'i-invoice',
					    'd'=>'print invoice',
					    'l'=>b_url::same("?invoice_once=yes&v_id=".$this->rec['v_id'])));
	  break; 
	    
	case VM_V_payPerdiem:
	  if ($already_payed === Null) $already_payed = (bool)(is_object($this->exp) && $this->exp->scholarshipP(False,False));
	case VM_V_payTrip:
	  if ($already_payed === Null) $already_payed = (bool)(is_object($this->exp) && $this->exp->scholarshipT(False,False));

	  // Show either a click-able button or just an icon
	  $img  = @$descr['i'] . ($yes_no ? '_on' : '_off');
	  $icon = (!$already_payed && VM::hasRightTo('setup_reimbursement',$this->rec)
		   ? bIcons()->getButton(array('class'=>'page_load_progress',
						       'i'=>$img,
						       'c'=>($yes_no ? 'only_online' : 'only_online opacity_on'),
						       'd'=>$descr['i'].($yes_no ? '' : ' not').' to be payed, click to change',
						       'l'=>b_url::same("?resetcache_once=1&toggle_once=$p&v_id=".$this->rec['v_id'])))
		   : bIcons()->get      (array('i'=>$img,
						       'c'=>($yes_no ? 'only_online' : 'only_online opacity_on'),
						       'd'=>$descr['i'].($yes_no ? '' :' not').' payed')));
	  break;
	default:
	}
      }else{
	$icon   = ''; 
	$this->dbg('too old visit '.$this->rec['av_lastname']);
      }
      $reply[] = $icon;
    }
    bTiming()->cpu();
    return array_unique($reply);
  }

  /*
   * Button 
   * $reply = array($edit_visit,$delete_v,$exp_arePayed);
   */
  private function btn_admin(){

    // no admin buttons for some lists
    if (VM::outOfScope() || in_array($this->doing,array('show_mails_exchange',)))   return array();
    
    bTiming()->cpu(__FUNCTION__);
    locateAndInclude('bForm_vm_Expenses');    
    
    $notTooLate = (!VM::isEventEndorsed() || $this->rec['v_end'] < time() || 
		   (VM::isEventEndorsed() && bForm_vm_Visit::_getStatus($this->rec) == STATUS_PENDING));
    
    $exp_arePayed= (bForm_vm_Expenses::_exp_arePayed($this->rec)
		    ? bIcons()->get(array('d'=>'Paid up',
					  'c'=>'only_online',
					  'i'=>'i-bundle'))
		    : '');
    
    $edit_visit  = ($notTooLate
		    ? bIcons()->getButton(array('d'=>'see/update the visit information',
						'c'=>'only_online',
						'i'=>'i-edit',
						'l'=>b_url::same("?form=vm_Visit&id=".$this->rec['v_id'])))
		    : '');
    
    $delete_v    = ($notTooLate
		    ? ((!bForm_vm_Visit::_isVisitType_program($this->rec) && 
			VM::hasRightTo('book') && 
			bForm_vm_Visit::_getStatus($this->rec)==STATUS_NO)
		       ? b_btn::submit_icon('i-drop',
					    txt_deleteVisit,
					    b_url::same("?function=vm_cancel_visit&v_id=".$this->rec['v_id']),
					    $confirm=True)
		       : '')
		    : '');
    
    $reply = array($edit_visit,$delete_v,$exp_arePayed);
    if ($this->doing == 'budget_byProjects'){
      if (empty($this->rec['v_projectid'])) $reply[] = b_btn::submit_icon('i-wallet',
									  'set project number',
									  b_url::same("?action_once=".VM_visit_project."&form=vm_Visit&id=".$this->rec['v_id']));
    }
    bTiming()->cpu();
    return $reply;
  }

  /*
   * Draw control buttons depending on the status of the visit
   */
  private function printPolicies(){ 
    if (($this->rec['v_type'] == VISIT_OUT_OF_SCOPE) || $this->current_tab_does_not_need_buttons()) return;
    
    // Gather the buttons
    $this->t->extraTD = array_merge($this->btn_approve_reject(),
				    $this->btn_payments(),
				    $this->btn_admin());
    if (empty(VM::$e)) array_unshift($this->t->extraTD,
				     bIcons()->get(array('i'=>VM::$description[$this->rec['v_type']],
							 'c'=>'only_online')));
  }

  /*
   * Cancel ambiguous message "really paid" when nothing was yet paid
   */
  private function calculate_totals($project='events'){
    
    // Set flag 'something_was_paid', calculate totals
    $this->total = array();
    foreach ($this->rec_visit as $v_id=>$rec){
      foreach($this->money as $item){
	if (!($v = (is_numeric(@$rec[$item]) && (int)$rec[$item]
		    ? $rec[$item]
		    : 0))) continue;
	if ((strpos($item,'_r') == (strlen($item)-2)) && ($item != 'total_r')) $this->something_was_paid = True;
	if (($project === 'events') || ($project == @$rec['v_projectid'])) @$this->total[$item] += $v;
      }
    }
  }
  
  private $something_was_paid = False;
  function listEntries_summary(){
    //     $this->dbg($this->b_tabs->active_tab_name(),True);
    bTiming()->cpu(__FUNCTION__);

    // Initialise the counters... TBD better...
    $this->header();
    if ($this->doing != 'budget_byProjects') $this->calculate_totals();
    if (!in_array($this->doing,array('myguests','budget_byProjects'))) unset($this->header_cache['period']); // Small pleasure...
    if(!$this->something_was_paid) unset($this->header_cache['total_r']);

    //
    // Get the page headers
    //
    $header = $this->header();
    
    
    // Open the form if needed
    if ($this->VM_reimberser_open){
      print "<form method='post' class='only_online' action='".b_url::same()."' name='vm-reimberser' enctype='multipart/form-data'>\n";
    }

    
    //
    // Show the "help pages" on the top of the listing
    //
    if ($helpPages = $this->help_text()){
      // "Open" the accordion when first viewed, then start with the "closed" accordion
      ob_start();
      $select = 99*((int)@$_SESSION[__CLASS__][b_crypt::hash($this->doing.b_fmt::joinX(',',$helpPages))]++);
      if (VM::e_ID() > 0 && VM::$e->getValue('e_end',1)<time()) $select = 99;
      b_fmt::accordion('open',$select," class='error'");       
      foreach($helpPages as $title=>$body){
	b_fmt::accordion($title,'h4-accordion');             
	print $body;
      }
      b_fmt::accordion('close');              
      myPear::MESSAGE(ob_get_contents());
      ob_end_clean();
    }

    // Show "big button"
    $this->boss_button();

    switch($this->doing){
      //
      // Extract the collected list as an Excel file
      //
    case '2excel':
      if (!empty($_GET['e_excel_once'])){
	if (empty($this->excel_input)){
	  myPear::WARNING("There is no accommodation assigned yet");
	}else{
	  foreach($this->excel_input as $hut=>$data){
	    b_os::write_excel_file(b_crypt::urlize($hut).'_'.b_crypt::hash(VM::$e->name()).'.csv',$data);
	  }
	}
      }
      break;
      
      //
      // Print the collected list of Photos / Badges / Labels
      //
    case 'photos':
      if (!isset($N)){
	$N=($nv=$this->photos_Nrows)  * ($nh=$this->photosPerRow);
	$css_class = 'photos';
      }
      
    case 'badges':  
      if (!isset($N)){
	$N=($nv=$this->badges_Nrows)  * ($nh=$this->badgesPerRow);
	$css_class = 'badges';
      }
      
    case 'doorLabels': 
      if (!isset($N)){
	$N=($nv=$this->doorLabels_Nrows)*($nh=$this->doorLabelsPerRow);
	$css_class = 'doorlabels';
      }
      
      $countLabels = 0;
      for ($i=count($this->labels); $i < $N; $i++) $this->labels[$i] = array();
      $t = new b_table("class='$css_class'");
      $t->highlightRows = False;
      for ($v=1; $v <= $nv; $v++){
	  $t->tro();
	  for ($h=1; $h <= $nh; $h++)  $t->td(join('<br/>',$this->labels[$countLabels++]));
	  $t->trc();
      }
      $t->close();
      $this->labels = array();
      break;

    case 'show_mails_exchange':
    case 'endorsing':
    case 'reception_attenders':
      //    case 'lists':      
      $this->print_collected_visitors();
      break;
      
    case 'budget_byProjects':
      if (is_object(VM::$e) && !VM::$e->attenders(True)) return;
      //
      // Calculate the total budget, show it.
      // Print list of collected visits
      //
      // Create tabs only here, since until now we do not have the list of projects
      $project_ids = $tabs_toShow = array();
      foreach(array_keys($this->rec_project) as $id){
	list($code,$name) = VM_projects()->getProjectName($id);
	if (strpos($name,'_') === 0) list($code,$name) = array($name,$code);
	if (empty($code))            list($code,$name) = array('?','Unknown projects');
	$tabs_toShow[$code] = $name;
	$project_ids[$code] = $id;
      }

      if (!empty($tabs_toShow)){
	$this->mysort($tabs_toShow);
	if (count($tabs_toShow) > 1) $tabs_toShow['Total'] = b_time::period(vm_day1,vm_day9);
	
	locateAndInclude('b_tabs');
	$this->b_tabs = new b_tabs_myPear(array_keys($tabs_toShow),0);
	// print x('h3',$this->b_tabs->active_tab_name().' '.$tabs_toShow[$this->b_tabs->active_tab_name()]);
	foreach($tabs_toShow as $code=>$name){
	  if (!$this->b_tabs->body()) continue;
	  $this->b_tabs->body_prefix = x('h3',"$code $name");
	  $t_sv = $this->t;
	  $this->calculate_totals($code == 'Total' ? 'events' : $project_ids[$code]);
	  $this->print_collected_visits_budget($code == 'Total' ? 'events' : $project_ids[$code]);
	  $this->t = $t_sv;
	}
	$this->b_tabs->close();
	unset($this->b_tabs);
      }
      break;
      
    default:
      break;
    }
    
    //
    // Show the tabs
    //
    if (!empty($this->b_tabs) && is_object($this->b_tabs)){
      
      bIcons()->toggleExplainIcons(False);
      bIcons()->reset();
      
      foreach($this->tabs_toShow as $tab=>$method){
	if (!$this->b_tabs->body()){
	  $this->dbg("SKIP b_tab($tab -> $method)");
	  continue;
	}
	$this->dbg("DO b_tab($tab -> $method)");
	
	// _GET is cleaned in case of editing, so save it for the later use below
	$c = Null;
	$arg_get = $_GET;
	$arg = $method.'_holder';
	if (isset($arg_get['w_once'])) print x('h2',$arg_get['d_once'].' - '.$arg_get['w_once']);
	
	switch($method){
	case tab_accommodation_options:
	  if (is_object(VM::$e) && !VM::$e->attenders(True)) return;
	  if (VM::outOfScope())     return;
	  // Initialise vm_accommodationOptions if not yet done
	  if(is_object(VM::$e)){
	    VM::$e->accommodationOptions()->update();
	    VM::$e->accommodationOptions()->show();
	  }else{
	    $av_id = @$_GET['host_avid_once'];
	    if (empty($av_id) && is_object($this->av)) $av_id = $this->av->ID;
	    if (!empty($av_id)){
	      bList::getListInstance(myOrg_ID,'bList_vm_accommodationOptions')->update();
	      bList::getListInstance(myOrg_ID,'bList_vm_accommodationOptions')->show($av_id);
	    }
	  }
	  break;
	  
	case 'print_collected_visitors':
	  $this->print_collected_visitors();
	  break;
	  
	case 'print_collected_visits_budget':
	  $this->print_collected_visits_budget();
	  break;
	  
	case tab_summary:
	case tab_budget_summary:
	  $this->print_tab_budgetSummary();
	  break;
	  
	case tab_social_events:
	  if (is_object(VM::$e) && !VM::$e->attenders(True)) return;
	  $c = VM::$e->socialEvents();
	  if (!empty($this->total['name'])) $header['name'] = tab_social_events;
	  if ($summary=VM::$e->socialEvents()->summary)      foreach($summary as $what=>$v)  @$this->total['name'] += $v;
	  break;
	  
	case tab_reimbursement_rates:
	  $c = ($this->bList_parent == myOrg_ID
		? bList::getListInstance(myOrg_ID,'bList_vm_reimbursementRates')
		: VM::$e->reimbursementRates());
	  break;
	  
	case tab_available_budget:
	  $c = VM::$e->budgetSource();
	  if ($this->event_is_bankrupt){
	    $_GET[$arg] = 'yes';
	    myPear::WARNING('<br/>The event '.x('em',x('"',VM::$e->name())).'<br/> has no known budget source.'.
			    '<br/><br/>Please check the budget code with '.myOrg_name.
			    ' administration and/or indicate other budget source using the dialog below');
	  }
	  break;

	default:
	  print x('h2',$tab);
	}
	
	if (is_object($c)){
	  //
	  // Show the dialog buffer and return, if we are editing the list.
	  // Otherwise continue collecting the relevant lists.
	  //
	  $editing = b_cnf::get($arg) || ((@$arg_get['act_id'] == $c->ID) &&
					  (@$arg_get['act_code'] === 'drop'));
	  if ( $editing || ($c->MODE == 'RW')){
	    if($editing && ($c->MODE == 'RO')) $c->extra_buttons[] = b_btn::submit('back to the event review',
										   b_url::same('?resetcache_once=1&continue_once=yes'));
	    $c->getForm();
	    b_cnf::set($arg,($editing ? 'no' : 'yes'));
	    return;
	  }else{
	    $c->getForm();
	  }	
	}	
      }
      $this->b_tabs->close();
      $this->b_tabs = Null;
    }
    if ($this->VM_reimberser_open){
      print "<input class='mdl_button bg_green only_online' type='submit'>\n";
      print "</form>\n";
    }
    print "\n<!-- exiting \"".__FUNCTION__."($this->doing)\" =========================================== -->\n";
    bTiming()->cpu();
  }
  
  /*
   * "numeric" (i.e. ignore text) sort
   */
  function mysort(&$array){
    foreach($array as $k=>$v) $temp[$k] = preg_replace('/[a-zA-Z]/','',$k);
    asort($temp, SORT_NUMERIC);
    foreach(array_keys($temp) as $k) $temp[$k] = $array[$k];
    $array = $temp;
  }
  

  /*
   * Print all the collected visits
   */
  private function print_collected_visitors(){
    $this->dbg();

    unset($this->t);
    $this->drawHeader();
    $headers = array_keys($this->header());
    foreach(array('name','av_Lf') as $c){ 
      if (in_array($c,$headers)) $this->t->repetValuesCol1only = $c;
    }

    // Print all the collected visits
    foreach ($this->rec_visit as $v_id=>$rec){

      $this->split_by_huts($rec);

      if (isset($rec['all_buttons'])) $this->t->extraTD = $rec['all_buttons'];
      if (isset($rec['all_classes'])) $this->t->class   = $rec['all_classes'];
      $this->set_backgroundApproved();
      if ($this->doing == 'reception_attenders'){
	if (!isset($excel_input)){
	  $l = array(); foreach(array_values($this->header(False)) as $item) $l[] = $item;
	  $excel_input[] = $l;
	}
	$this->t->extraTD = array();
	$rec['all_highlight'] = '';
      }
      // Print the row
      $this->t->prt($rec,@$rec['all_highlight']);
      
      // Collect input for the excel file
      $l = array();
      foreach($headers as $item) $l[] = b_fmt::unEscape(strip_tags(@$rec[$item]));
      $excel_input[] = $l;
    }
    
    // Complete the Excel file
    if ($this->doing == 'reception_attenders'){
      b_os::write_excel_file(b_crypt::hash(VM::$e->name()).'_participants.csv',@$excel_input);
    }
    // Complete the screen output
    $this->t->close();
    unset($this->t);
  }
  
  /*
   * Print the collected budget matrix
   */
  private function print_collected_visits_budget($project='events'){
    $this->dbg();
    if (empty($this->t) || !($this->t instanceof b_table_zebra)) return;

    // Save the header (it is modified by this function)
    static $saved_budgetHeader = array();
    if (empty($saved_budgetHeader))  $saved_budgetHeader = $this->budgetHeader;
    $this->budgetHeader = $saved_budgetHeader;

    // Now print the records
    $something_printed = 0;
    unset($this->t);
    foreach ($this->rec_visit as $v_id=>$rec){
      if (($project !== 'events') && ($project != @$rec['v_projectid'])) continue;
      
      if (!is_object(@$this->t)){
	parent::drawHeader();
	foreach (array_keys($this->header_cache) as $k) $this->t->css[$k]['align'] = 'align_right';    
	$this->t->css['name']['align'] = 'align_left';    
	for($n=0; $n<4; $n++){
	  $this->t->class["space$n"] = 'highlightWhite'; 
	  $rec["space$n"] = '';
	}
      }
      
      foreach($this->money as $item){
	if (empty($rec[$item]))	  continue;
	if (is_numeric($v = $rec[$item])){
	  $rec[$item] = b_fmt::escape(b_fmt::money($v));
	  $this->set_colorCodes($item,$rec[$item]);
	}
      }
	
      $this->t->extraTD = @$rec['all_buttons'];
      $this->set_backgroundApproved();
      if ($rec['v_status'] == STATUS_NO){
	foreach($this->header() as $k=>$v) $this->t->class[$k] = 'overstrike redText';
      }else{
	$this->t->class = array();
	$this->t->class = @$rec['all_classes'];
      }
      $this->t->prt($rec,@$rec['all_highlight']);
    }
    
    // Print the total values
    if (isset($this->t) && $this->t instanceof b_table_zebra){
      $totalP = array();
      foreach(array_keys($this->header()) as $c) $this->t->class[$c] = 'highlightWhite';
      $this->highlight = 'highlightWhite';
      $this->t->tr();
      foreach($this->total as $k=>$v){
	$totalP[$k] = x('strong',x('i',b_fmt::escape(b_fmt::money($v))));
	$this->set_colorCodes($k,$totalP[$k]);
      }
      $this->t->prt($totalP);	
      $this->t->close();
      unset($this->t);
    }
    print "\n<!-- exiting \"".__FUNCTION__."($project)\" =========================================== -->\n";
  }
  
  /*
   *
   */
  private $header_cache = Null;
  private $header_counters = Null;

  private $clean_empty_items = True;
  function header($clean_empty_items=True){

    // Protection against lazy users
    if (!bAuth::authenticated() || VM::outOfScope()) return array('name'=>'','_a_name'=>'');
    
    if ($this->header_cache === Null){
      
      $this->title();
      
      switch ($this->doing){
      case 'budget_byProjects':
      case 'myguests':
      case 'budget':
	
	$header = $this->budgetHeader;
	$this->set_colorCodesTH($header,$this->th_attr);
	break;
	
      case 'show_mails_exchange':

	$this->clean_empty_items = False;
 	
	$header = array('status'     => '',
			'name'       => '',
			'space1'     => '',
			'm_pwd_1'    => '',
			'm_pwd_2'    => '',
			'space2'     => '',
			'm_yes_1'    => '',
			'm_yes_2'    => '',
			'space3'     => '',
			'm_no_1'     => '',
			'm_no_2'     => '',
			'space4'     => '',
			'm_info_1'   => '',
			'm_info_2'   => '',
			);
	if (!send_denialEmail_to_registrants)           unset($header['m_no_1'],  $header['m_no_2'], $header['space4']);
	if (!send_finalWelcomeEmail_to_registrants)     unset($header['m_yes_1'], $header['m_yes_2'],$header['space3']);
	if (!send_automatically_infoMail_to_registrants)unset($header['m_info_1'],$header['m_info_2']);

	foreach(array('pwd','yes','no','info') as $c) $this->css["m_${c}_1"]["style"] = "width:0;";

	$this->preHeaders = $this->build_mail_preHeaders($header);
	break;
	
      case '2excel':

	$this->clean_empty_items = False;

	$header = array(//'v_id'     => 'Code',
			'v_projectid'=> 'Project',
			'name'       => '',
			'lease_start'=> 'Arrival',
			'lease_end'  => 'Departure',
			'cotenant'   => 'Cotenant',
			'a_area'     => 'Room<br/>(kvm)',
			);
	if (!VM_projects()->getProjectID(VM::$e->ID)) unset($header['v_projectid']);
	break;

      case 'reception_attenders':

	$this->clean_empty_items = False;

	$header = array('av_Lf'    => '',
			'av_email' =>'',
			'av_institute' =>'',
			'av_city'  =>'',
			'av_residentship'=>'',
			'v__stay'  =>'',
			);
	break;
	
      default: 

	$this->clean_empty_items = False; 	// ??

	$header = array('name'     => '',
			'_a_name'  => 'accommodation',
			'cotenant' => '',
			// '_o_name'  => 'office',
			'v__stay'  => 'stay',
			'v__host'  => 'host',
			);
	if (is_object(VM::$e))	unset($header['v__host']);
	if (is_object($this->av)) $header['name'] = 'event';
      }
      $this->header_cache = $header;
    }

    // setup the counters
    if ($this->header_counters === Null){
      $this->header_counters = array();
      foreach(array_keys($header) as $k){
	if (strpos($k,'space') === False) $this->header_counters[$k] = 0;
      }
    }elseif($clean_empty_items && $this->clean_empty_items){

      // Update the header, remove the unused columns, mainly for the budget pages.

      foreach($this->header_counters as $k=>$v){
	if (empty($v) && isset($this->header_cache[$k])){
	  unset($this->header_cache[$k]); 
	}
      }
      
      // Rebuild pre-headers
      if (empty($this->header_cache))  $this->header_cache = array();
      $this->preHeaders = $this->build_budget_preHeaders($this->header_cache);
    }
    return $this->header_cache;
  }
  
  function title($showBoxes=True) {}
}
