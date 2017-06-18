<?php
/*
 * get a random visit number as an example
 */
function bTpl_hook_example(){
  $q = myPear_db()->query("SELECT v_id FROM abs_visits WHERE v_eid > 0 ORDER BY v_id DESC LIMIT 1");
  while ($r = myPear_db()->next_record($q)) $reply = $r['v_id'];
  return array('v_id'=>$reply);
}

/*
 * The render for bMailer_vm templates.
 *
 * @param $what rendering mode:
 *    - SHOW_ID return the function name
 *    - SHOW_TRANSLATION  return the description of the keys
 *    - SHOW_DESCRIPTION  return the "translated" text
 * @param $args
 * @param $preview
 */
function VM_mailer_render($what,$args=array(),$preview=False){

  locateAndInclude('bForm_vm_Event');
  locateAndInclude('bForm_vm_Visit');
  
  $v = $av = $e = Null;
  foreach ($args as $key=>$id){
    $o = myPear::getClassFromID($id,'class');
    if ($o instanceof bForm_vm_Visit) { $v = $o; $av = $v->av; $e = $v->e; }
    if ($o instanceof bForm_vm_Event) { $e = $o; }
    if ($o instanceof bForm_Avatar)   {$av = $o; }
  }

  // Build the translation table.
  // By default the translation is the key itself, optionally
  // provide a test (usually arbitrary) example value of the key
  $t = array('firstname'   =>array('r'=>(is_object($av) ? $av->fmtName('f',0) : ''),
				   'd'=>'First name'),
	     'lastname'    =>array('r'=>(is_object($av) ? $av->fmtName('l',0) : ''),
				   'd'=>'Last name'),
	     'fqn'         =>array('r'=>(is_object($av) ? $av->fmtName('sfl',0) : ''),
				   'd'=>'Full name (title,fistname,lastname)'),
	     'pageurl'     =>array('r'=>'<encoded URL>', 
				   'd'=>'URL to the personal page'),
	     'org'         =>array('r'=>myOrg_name,
				   'd'=>'Organization'),
	     'event'       =>array('r'=>(is_object($e) ? $e->name(False) : ''),
				   'd'=>'Event title'),
	     'event_period'=>array('r'=>(is_object($e) ? b_time::period($e->getValue('e_start',1),$e->getValue('e_end',1)):''),
				   'd'=>'Event duration'),
	     'visit_period'=>array('r'=>(is_object($v) ? b_time::period($v->getValue('v_start',1),$v->getValue('v_end',1)):''),
				   'd'=>'Stay period'),
	     );
  
  $reply = array();
  switch($what){
    
  case 'SHOW_ID':
    $reply = __method__;
    break;
    
  case 'SHOW_DESCRIPTION': 
    foreach($t as $f=>$r) $reply[$f] = $r['d'];
    break;
    
  case 'SHOW_TRANSLATION': 
    foreach($t as $f=>$r) $reply[$f] = $r['r'];
    break;

  default:
    b_debug::internalError("unexpected argument");
  }
  return $reply;
}


locateAndInclude('bMailer');
class bMailer_vm extends bMailer{

  public static $wrapper = 'VM_mailer'; 
  protected $mailID = "Visits Manager API";

  var $isMailAutomatic = False;
  var $visit  = Null;
  var $debug = True;
  
  private $rec    = array();

  public static $isReady = 0;
  public function __construct(){

    if (self::$isReady++) b_debug::internalError("??? second call to ".$this);
    parent::__construct();

    return;
    
    /*
     * Pre-fill the templates
     */
    locateAndInclude('bTpl');
    bTpl::$owner = VM_MODULE;
    foreach (get_class_methods($this) as $method){
      if (preg_match('/^m_/',$method)) call_user_func(array($this, $method));
    }
  }


  /*
   *
   */
  function getStatus($arg,$code,$v_status=Null){
    switch($code){
    case VM_V_infoMailSent:  
      return $this->m_applicant_info_mail($arg,'status');

    case VM_V_denialMailSent:  
      $this->m_applicant_info_mail($arg,'status');
      return $this->m_applicant_deny($arg, 'status',$v_status);

    case VM_V_welcomeMailSent:   
      $this->m_applicant_info_mail($arg,'status');
      return $this->welcome_applicant($arg,'status',0,$v_status);

    case VM_V_enMailSent:
      if (empty($policyQ)) $policyQ = VM_V_enMailQueued;
    case VM_V_rdlMailSent:
      if (empty($policyQ)) $policyQ = VM_V_rdlMailQueued;

      $v = myPear::getInstance_new('bForm_vm_Visit',$arg,'fatal');
      return ($v->getPolicy($policyQ)
	      ? in_queue
	      : $v->getPolicy($code));

    case VM_E_endorsmentSent:
    case VM_V_guestAcceptSent:
    defailt:
      b_debug::internalError(myPear::getClassFromID($arg)."($code)");
    }
  }


  /*
   * Automatically remind organizers that the deadline is reached.
   * The code needs further debugging - 
   * rather often the organizers put to agenda events which have nothing to do with us, need to have good filters...
   */
  private $info = array();
  public function cronjob_remind_organizers(){
    $verbose = cnf_dev || VM_administrator_here; // VM_observer_here;    
   
    // Do it ones per session...
    if (!@$_SESSION[__method__]++){
      locateAndInclude('bForm_vm_Event');
      // Select events where the registration deadline is over AND event has not yet started
      $q = myPear_db()->query("SELECT e_id FROM abs_events WHERE e_start > ".time()." AND e_reg_end < ".time()." ORDER BY e_start ASC");
      while ($r = myPear_db()->next_record($q)){
	$e = new bForm_vm_Event($r['e_id']);
	if (VM::outOfScope($e) || $e->isEventEndorsed()) continue;
	$e_reg_end = $e->getValue('e_reg_end');
	$e_start   = $e->getValue('e_start');
	$info_body = array();
	$msg = array(b_time::delta($e_reg_end,$e_start).' for preparations');
	if (time() > $e_reg_end) $msg[] = 'starts in '.b_time::delta(time(),$e_start);
	$info_title = ($e->name().'<br/>  reg.end / ev.start = '.
		       b_time::period($e_reg_end,$e_start).' '.x('(',join(', ',$msg)));
	if (($n=($e_start - $e_reg_end)) > 365*24*3600){
	  $n = (int)($n/30/24/3600);
	  $info_body[] = "??? Strange time of $n months between reg.end / ev.start";
	  myPear::WARNING_T("??? Strange time of $n months between reg.end & ev.start.<br/>".
			    b_time::period($e_reg_end,$e_start).'<br/>'.
			    $e->name());
	}else{
	  foreach(VM_organizers($r['e_id'])->getOrganizers() as $av_id){
	    locateAndInclude('bForm_vm_Visit');
	    $v = new bForm_vm_Visit("v_avid = $av_id AND v_eid = ".$r['e_id']);
	    if ((int)$v->ID){
	      if (!$v->getPolicy(VM_V_rdlMailSent) &&
		  !$v->getPolicy(VM_V_rdlMailQueued)){
		$info_body[] = 'Informing organizer '.$v->av->fmtName('Lf');
		$this->remind_organizer($v,$e,$mode='rdl',True);
	      }
	    }elseif(VM_manager_here){
	      $av = new bForm_Avatar_vm($av_id);
	      myPear::WARNING($msg = 'Organizer '.$av->fmtName('fl').' has not yet registered for his event '.$e->name());
	      $info_body[] = $msg;
	      $mode = 'reg';
	      if (!@$_SESSION[__method__.$mode]++)  $this->remind_organizer($av,$e,$mode,True);
	    }
	  }
	}
	if ($verbose) myPear::INFO($info_body,$info_title);
      }
    }
  }
  
  /*
   * There are 3 cases when this mail is sent:
   * - 'pwd' just remind the password, might be sent at any time
   * - 'rdl' the event registration deadline is reached, invite the organizer to look into his event
   * - 'en'  the event budget is endorsed, inform the organizer about it
   * - 'reg' invitation to register for the event
   */
  public function remind_organizer($av_id,$e,$mode='pwd',$no_preview=True){
    $this->dbg(starting);
    // Parse the arguments
    if ($av_id instanceof bForm_Avatar_vm){
      $v  = Null;
      $av = $av_id;
    }elseif($av_id instanceof bForm_vm_Visit){
      $v  = $av_id;
      $av = $v->av;
    }else{
      if ($v = myPear::getInstance_new('bForm_vm_Visit',$av_id,'strict')){
	$av = $v->av;
      }else{
	$av = myPear::getInstance_new('bForm_Avatar_vm',$av_id,'fatal');
      }
    }

    // Continue parsing, the argument might be the visit ID...
    if (empty($av)){
      $this->dbg("Argument is not 'av_id', trying bForm_vm_Visit");
      $v = new bForm_vm_Visit($av_id);
      $av = $v->av;
    }

    // Parse "event" argument
    if (!($e instanceof bForm_vm_Event)) $e = myPear::getInstance_new('bForm_vm_Event',$e,'fatal');
    
    // Guess the visit if not given explicitly
    if (empty($v) && ($mode !== 'reg')){
      if (!($v_id = $e->getVisit_ID($av->ID))){
	myPear::WARNING($av->fmtName('fl').' is not yet registered for "'.$e->name().'"<br>No mail sent');
	$this->dbg(exiting);
	return False;
      }
      $v = new bForm_vm_Visit($v_id);
    }

    // Is the mail should be sent once only?
    if     ($mode == 'en')  { $policy = VM_V_enMailSent;  $policyQ = VM_V_enMailQueued;  }
    elseif ($mode == 'rdl') { $policy = VM_V_rdlMailSent; $policyQ = VM_V_rdlMailQueued; }
    else                      $policy = $policyQ = Null;


    // Don't send the reminder if it is too late
    if (!empty($policy) && !$v->getPolicy($policyQ) && (time() > $e->getValue('e_start'))){
      $v->setPolicy($policy,1);
    }
    
    // Is it already done?
    if (!empty($policy) && ($v->getPolicy($policy) || $v->getPolicy($policyQ))){
      $this->dbg("$mode already done... ");
      $this->dbg(exiting);
      return True;
    }
    
    // Put this mail into the execution queue, not waist time in the interactive session
    $fqn  = $av->fmtName('sfl',0);
    if (!myPear_taskManager()->exec_request && $no_preview && ($no_preview !== 'send_now')){
      $this->dbg("Queue the request"); 
      myPear::MESSAGE(sprintf("E-mail to %s is send to the mail queue",$fqn));
      myPear_taskManager()->addTask(get_class($this),__function__,array($av->ID,$e->ID,$mode,$no_preview));
      if (!empty($v)) $v->setPolicy($policyQ,1);
      $this->dbg(exiting);
      return;
    }

    // Send the mail (from the queue or after preview)
    $event = $e->name();
    $event_period = b_time::period($e->getValue('e_start',1),$e->getValue('e_end',1));
    
    // A copy on this email is optionally sent to "the secretary"
    $recipients = array($av->getEmail());
    if ($mode == 'en'){
      list($sec_mails,$sec_names)  = $this->get_secretaries(DUTY_reimbursement);
      //      $recipients = array_merge($recipients,$sec_mails);
    }else{
      list($sec_mails,$sec_names)  = array(array(),array());
    }
    
    foreach($recipients as $email){
      $this->dbg($email,cnf_dev);
      $body = (in_array($email,$sec_mails)
	       ?"Dear organizers of the ".myOrg_name." event \"$event\", taking place $event_period,"
	       :"Dear $fqn,

We write to you as one of the organizers of the ".myOrg_name." event \"$event\", taking place $event_period.
");

      switch($mode){
      case 'rdl':  // ----------------------- application deadline is over
	if (!isset($subject))  $subject = $event;
	$body .= "
The deadline for the applications is now over and you can estimate now the event budget.";
	
      case 'pwd': // ----------------------- simple password reminder
	if (!isset($subject))  $subject = 'Access to the visitors database';
	$body .= "
We have created a web page for your event which will help you in planning it. 
On the page you can accept or reject applicants, decide who gets travel reimbursement or ".VM_wordPerdiem." (meals) , check the costs of various social events, and also get an overview over our accommodation arrangements for the participants. 

These pages are password protected. For your convenience the password is encoded into the following URL, hence can access your web page directly as:
";
	break;
	
      case 'en': // ----------------------- the event budget is now endorsed
	if (!isset($subject))  $subject = $event;
	$body .= "
The event budget is now endorsed by ".myOrg_name." and locked.

You can see the event budget from your personal web page, which you can access directly as:
";
	break;

      case 'reg': // ----------------------- the organizer is not yet registered for the event
	if (!isset($subject))  $subject = "Please register for the event you organize";
	$body .= "
The deadline for the applications is now over and you can estimate now the event budget.
However, in order to do this you have to be registered as the event participant, please register as soon as possible."; 
	break;

      default:
	$this->dbg("Unexpected argument mode='$mode'",cnf_dev);
	$this->dbg(exiting);
	return Null;
      }
      
      if ($mode != 'reg') $body .= "{login_url}

Remember to save this mail or book-mark the web page to facilitate later access to the web page.
";
      
      if ($no_preview === 'send_now') $no_preview = True;
      if (($ok = $this->send(array($av,
				   array(),
				   in_array($email,$sec_mails)),
			     $email,
			     $subject,
			     $body,
			     $syslog_thisMail=True,
			     $no_preview)) && 
	  !in_array($email,$sec_mails)){
	// Log the mail sending 
	switch($mode){
	case 'rdl':  // ----------------------- application deadline is over
	  myPear_logs()->add('reminder',$av->ID,'applications deadline is over',myPear_cronjob()->ID);
	  break;
	case 'en': // ----------------------- the event budget is now endorsed
	  myPear_logs()->add('info',$av->ID,'event is endorsed',myPear_cronjob()->ID);
	}
	if (!empty($policy) && !empty($v)){
	  $this->dbg("setPolicy($policy,1,True,True)");
	  $v->setPolicy($policy,1);
	  $v->setPolicy($policyQ,0);
	}
      }
    }
    $this->dbg("Mail to '$email' is ".($ok ? 'OK' : 'NOT sent ...'),cnf_dev);
    $this->dbg(exiting);
    return $ok;
  }
  
  /*
   * Remind the relevant people to approve the pending endorsements
   */
  public function remind_endorser($av_id,$rec,$no_preview=True){

    $this->dbg();

    // Put this mail into the execution queue, not waist time in the interactive session
    if (!myPear_taskManager()->exec_request && $no_preview){
      foreach(array('v_id','av_lastname','av_firstname','av_salutation') as $i) $r[$i] = @$rec[$i];
      myPear_taskManager()->addTask(get_class($this),__function__,array($av_id,b_fmt::safe_encode($r),$no_preview));
      return Null;
    }

    // Might be called from the queue...
    $rec = b_fmt::safe_decode($rec);

    $av = myPear::getInstance_new('bForm_Avatar',$av_id,array('strict','fatal'));
    if (VM_administrator_here || cnf_dev) myPear::MESSAGE("Reminding ".$av->name()." to approve visit of ".bForm_Avatar::_fmtName('Lf',$rec));
    
    if ($ok = $this->send(array($av,
				array('form'=>'vm_Visit',
				      'id'  =>$rec['v_id'],
				      'mact'=>'v_endorser'),
				False),
			  $av->getEmail(),
			  b_fmt::unEscape("Visit of ".bForm_Avatar::_fmtName('sfl',$rec)),
			  'Dear '.$av->fmtName('f',0).',
There is a new visit request, which needs your approval.

At your convenience, visit:
{login_url}
to approve or deny the request.
',
		$syslog_thisMail=False,
			  $no_preview)){
      myPear_logs()->add('reminder',$av_id,'approve visit '.bForm_avatar::_fmtName('Lf',$rec),myPear_cronjob()->ID);
      $this->dbg('done');
    }
    return $ok;
  }
  

  /*
   * Inform secretaries that there a collaboration visit approved/canceled
   *
   * @param $v  - visit class
   * @param $no_preview
   */
  public function inform_about_guest_invitation($v,$no_preview=True){
    $no_preview = True;    
    if (!($v instanceof bForm_vm_Visit)) $v = new bForm_vm_Visit($v);

    $accepted = ($v->getValue('v_status') == STATUS_YES);
    $rejected = ($v->getValue('v_status') == STATUS_NO);
    
    // Is it our business?
    if (($v0=($v->getValue('v_type') != VISIT_TYPE_COLLABORATION)) ||
	($v1=(!$accepted && !$rejected)) ||
	($v2=($v->getValue('v_end',True,True) < time())) ||
	($v3=!array_intersect(array(VM_V_guestRejectSent,VM_V_guestAcceptSent),
			      array_keys($v->getPolicies()))) || 
	($v4=($rejected && $v->getPolicy(VM_V_guestRejectSent))) ||
	($v5=($accepted && $v->getPolicy(VM_V_guestAcceptSent)))){
      $this->dbg(sprintf("No need to send mail(s) v0=%s v1=%s v2=%s v3=%s v4=%s v5=%s",@$v0,@$v1,@$v2,@$v3,@$v4,@$v5),cnf_dev);
      return True;
    }
    
    // Sent the mail without queuing to:
    //       Secretaries
    foreach(VM_administrators()->membersOnDuty(VISIT_TYPE_COLLABORATION) as $s_name=>$s_email){
      $all_recepients[] = bForm_Avatar::hook($s_email);
    }
    
    //       Group leader
    foreach(VM_endorsers()->getEndorsers($v->getValue('v_group',1)) as $av_id){
      $all_recepients[] = new bForm_Avatar_vm($av_id);
    }

    // Org. managers
    $u_ea = new bUnit(myOrg_ID,RANK__ea_org_manager);
    foreach($u_ea->getMembers() as $um_id=>$r){
      $all_recepients[] = new bForm_Avatar_vm($r['um_avid']);
    }
 
    //       Host
    $v_host_avid = $v->getValue('v_host_avid');
    if (empty($v_host_avid)){
      $host = 'To be decided';
    }else{
      $all_recepients[] = $av = new bForm_Avatar_vm($v_host_avid);
      $host = $av->name();
    }
    
    //       Request owner
    $owner = 'Unknown';
    if (($date = $v->getValue('v_created',1)) && ($v_owner_avid=$v->getValue('v_owner_avid',1))){
      if ($av = myPear::getInstance_new('bForm_Avatar_vm',$v_owner_avid,'fatal')){
	$all_recepients[] = $av;
	$owner = $av->fmtName('fl').' '.b_time::medium_date($date);
      }
    }
    
    $ok = 0;
    foreach(array_unique($all_recepients) as $av){
      
      myPear::MESSAGE("Informing ".$av->name()." about visit of ".$v->av->fmtName('Lf'));
      
      // Mail text
      $subject = "Visit of ".$v->av->fmtName('fl',0).' is '.($accepted ? 'APPROVED' : 'CANCELED');
      $body = "Dear ".$av->fmtName('f').",
$subject.
".($accepted
   ? "You can access the visit record by clicking on the URL:
{login_url}
" 
   : "").

"
Visit information:
 Inviting group: ". myOrg()->orgGroups()->get_groups($v->getValue('v_group'))."
 Visit dates:    ".b_time::period($v->getValue('v_start'),$v->getValue('v_end'))."
 Host:           $host
 Visit requested by: $owner 
      ";
      
      // Submit it
      if ($this->send(array($av,
			    array('form'=>'vm_Visit',
				  'id'=>$v->ID)),
		      $av->getEmail(),
		      $subject,
		      $body,
		      $syslog_thisMail=True,
		      $no_preview)){
	$ok++;
	myPear_logs()->add('info',$av->ID,'visit of '.$v->av->fmtName('Lf'),myPear_cronjob()->ID);
      }
    }
    if ($ok > 0){
      if ($accepted){
	$v->setPolicy(VM_V_guestAcceptSent,1,True,True);
	$v->setPolicy(VM_V_guestRejectSent,0,False,True);
      }else{
	$v->setPolicy(VM_V_guestRejectSent,1,True,True);
	$v->setPolicy(VM_V_guestAcceptSent,0,False,True);
      }
    }
    return ($ok > 0);
  }  

  /*
   * This mail has to be sent only once
   */
  public function m_applicant_info_mail($v_id,$no_preview=False,$debug=Null){

    $debug_sv = $this->debug;
    if ($debug !== Null) $this->debug = $debug;

    // Return if the visitor "is not concerned"
    $this->visit = ($v_id instanceof bForm_vm_Visit
		    ? $v_id
		    : myPear::getInstance_new('bForm_vm_Visit',$v_id,'fatal'));
    
    // Not concerned
    if($this->visit->getValue('v_type',1) !== VISIT_TYPE_PROGRAM){
      $case = 'not_VISIT_TYPE_PROGRAM';

    }elseif ($this->visit->getPolicy(VM_V_infoMailNC)){
      $case = 'flag already set';
      
    }elseif (!$this->visit->getValue('v_status',1) &&
	 $this->visit->getValue('v_start',1) < time()){
      $case = 'visit started';
    
    }elseif($this->visit->getPolicy(VM_V_denialMailSent) ||
	    $this->visit->getPolicy(VM_V_welcomeMailSent)){
      $case = 'welcome_mail_sent';

    }elseif ($this->visit->av->isOrganizer($this->visit->e->ID)){
      $case = 'is_organizer';

    }elseif ($this->visit->av->isE(True)){
      $case = 'employee';
    }
    
    // Exit if the visitor is not concerned
    if (!empty($case)){
      if (in_array(VM_V_infoMailNC,array_keys($this->visit->getPolicies()))){
	$this->visit->setPolicy(VM_V_infoMailNC,1);
      }
      $reply = CONST_not_concerned;
      $this->dbg("$reply, $case ".x('em',$this->visit->av->fmtName('fl')));
      $this->debug = $debug_sv;
      return $reply;
    }
    
    // Is it just an enquiry?
    $VM_V_infoMailSent = (bool)$this->visit->getPolicy(VM_V_infoMailSent);
    if ($no_preview === 'status'){
      $reply = ($this->visit->getPolicy(VM_V_infoMailQueued)
		? in_queue
		: $VM_V_infoMailSent);
      $this->dbg("$reply, ".x('em',$this->visit->av->fmtName('fl')));
      $this->debug = $debug_sv;
      return $reply;
    }
    
    // No preview requested, the mail is just to be send, and only once
    if ($no_preview && !myPear_taskManager()->exec_request){
      if ($VM_V_infoMailSent){
	// This mail has to be sent only once
	$this->dbg("Mail to ".$this->visit->av->name()." was already sent",cnf_dev);
      }else{
	// Put this mail into the execution queue, not waist time in the interactive session
	myPear_taskManager()->addTask(get_class($this),__function__,array($this->visit->ID,'send_now'));
	$this->visit->setPolicy(VM_V_infoMailQueued,1);
      }
      $this->debug = $debug_sv;
      return True;
    }
    
    //
    // Preview and/or send the mail
    //
    locateAndInclude('bTpl');
    $tpl = new bTpl(array('title' => __method__,
			  'render'=> VM_mailer_render('SHOW_ID'),
			  'body'  => "
Dear <firstname/> <lastname/>,
(Please disregard this message if your application for \"<event/>\" is already approved). 

Thank you for your recent application for the <org/> event
\"<event/>\".

We are considering your application and will inform you whether it is accepted or not. 

After your application is approved, within a month before the start of the event, we will send out all the information about your stay. For practical issues check our web page http://www.nordita.org/guests, or contact the event organizers." .
// " or our program coordinator $secretary[name], $secretary[mail].".
"

Best regards,
Organizing committee for \"<event/>\".
"));
    
    $this->template = __method__;
    if($ok = $this->send(array(),
			 $this->visit->av->getEmail(),
			 'Your application for '.myOrg_name.' event',
			 $tpl->render(array('v_id'=>$v_id)),
			 $syslog_thisMail=True,
			 $no_preview)){
      $this->visit->setPolicy(VM_V_infoMailSent,1,True,True);
      $this->visit->setPolicy(VM_V_infoMailQueued,0);
      myPear::MESSAGE(sprintf('Info mail is sent to %s',$this->visit->av->name()));
    }
    $msg = sprintf('%s(%s) %s',__function__,$this->visit->av->getEmail(),($ok ? 'OK' : 'NOT sent ...'));
    $this->dbg($msg, cnf_dev||cnf_CLI);
    myPear_logs()->syslog($msg);
    $this->debug = $debug_sv;
    return (bool)$ok;
  }
  
  
  /*
   *
   */
  public function m_applicant_deny($v_id,$no_preview=False,$v_status=Null,$bcc=array()){
    
    $this->dbg('',True);
    $this->visit = myPear::getInstance_new('bForm_vm_Visit',$v_id,'fatal');

    //
    // Check is the visitor subject of getting the "denial Email" 
    if (!send_denialMail_to_applicants){
      $case = "NO DENIAL MAILS configured";
    }elseif($this->visit->getValue('v_type',1) !== VISIT_TYPE_PROGRAM){
      $case = 'not_VISIT_TYPE_PROGRAM';
    }elseif (!empty($v_status) && ($v_status !== STATUS_NO)){
      // Shortcut to save time
      $case = $v_status;
    }else{
      $name = $this->visit->av->fmtName('fl');

      // Catch the approved/pending visits
      if ($this->visit->getValue('v_status',1) !== STATUS_NO) $case = $v_status;
    }

    // Exit if the visitor is not concerned
    if (!empty($case)){
      $reply = CONST_not_concerned;
      if (empty($name)) $name = 'visitor';
      $this->dbg("%s, %s, <em>%s</em>",var_export($reply,True),$case,$name);
      return True;
    }
    
    // Is it just an enquiry?
    if (empty($this->visit)) $this->visit = myPear::getInstance_new('bForm_vm_Visit',$v_id,'fatal');
    $VM_V_denialMailSent = (bool)$this->visit->getPolicy(VM_V_denialMailSent);
    if ($no_preview === 'status'){
      $this->m_applicant_info_mail($this->visit,'status',False);
      $reply = ($this->visit->getPolicy(VM_V_denialMailQueued)
		? in_queue
		: $VM_V_denialMailSent);
      $this->dbg($reply);
      return $reply;
    }

    // This mail has to be sent only once
    if ($no_preview && $VM_V_denialMailSent){
      $this->dbg("Mail to ".$this->visit->av->name()." was already sent",cnf_dev);
      return True;
    }
    
    // Put this mail into the execution queue, not waist time in the interactive session
    if ($no_preview && !myPear_taskManager()->exec_request){
      myPear_taskManager()->addTask(get_class($this),__function__,array($this->visit->ID,
									True,
									Null,
									(is_object(bAuth::$av)
									 ? array(bAuth::$av->getEmail())
									 : array())));
      $this->visit->setPolicy(VM_V_denialMailQueued,1);
      return True;
    }elseif (empty($bcc) && is_object(bAuth::$av)){
      $bcc[] = bAuth::$av->getEmail();
    }
    
    $this->dbg("Sending mail",cnf_dev);
    locateAndInclude('bTpl');
    $tpl = new bTpl(array('title' => __method__,
			  'render'=> VM_mailer_render('SHOW_ID'),
			  'body'  => "
Dear <firstname/> <lastname/>,

Thank you for your recent application for the Nordita event 
\"<event/>\".

We regret to inform you that we are not able to accommodate your request to participate.

Sincerely,
Organizing committee for \"<event/>\"."));
      
    $this->template = __method__;
    if ($ok = $this->send(array(),
			  $this->visit->av->getEmail(),
			  "Application denied",
			  $tpl->render(array('v_id'=>$v_id)),
			  $syslog_thisMail=True,
			  $no_preview,
			  array(),
			  $bcc)){
      $this->visit->setPolicy(VM_V_denialMailSent,1,True,True);
      $this->visit->setPolicy(VM_V_denialMailQueued,0);
      $this->m_applicant_info_mail($this->visit,'status',False);
      myPear::MESSAGE(sprintf('Denial mail is sent to %s',$this->visit->av->name()));
    }
    $this->dbg('Mail to '.$this->visit->av->fmtName('fl').' is '.($ok ? 'OK' : 'NOT sent ...'),cnf_dev);
    return (bool)$ok;
  }
  
  /*
   * The visit confirmation mail
   */
  public function welcome_applicant($v_id,$lease_id,$no_preview=False,$v_status=Null,$bcc=array()){

    if ($lease_id != 'status') $lease = myPear::getInstance_new('bForm_vm_Lease',$lease_id,array('strict'));
    $this->visit = (empty($lease) || empty($lease->parentForm)
		    ? myPear::getInstance_new('bForm_vm_Visit',$v_id,'fatal')
		    : $lease->parentForm);

    //
    if (!send_welcomeMail_to_applicants){
      // Check is the visitor subject of getting the "welcome Email" 
      $case = "NO WELCOME MAILS configured";
    }elseif($this->visit->getValue('v_type',1) !== VISIT_TYPE_PROGRAM){
      $case = 'not_VISIT_TYPE_PROGRAM';
    }elseif (!empty($v_status) && ($v_status !== STATUS_YES)){
      // Shortcut to save time
      $case = $v_status;
    }else{
      $name = $this->visit->av->fmtName('fl');
      
      // Catch the refused/pending visits
      if ($this->visit->getValue('v_status',1) !== STATUS_YES) $case = $v_status;
      
      // This mail should not be sent to the local organizers
      if ($this->visit->av->isOrganizer($this->visit->e->ID) && $this->visit->av->isE(True)) $case = 'isOrganizer';
    }

    // Exit if the visitor is not concerned
    if (!empty($case)){
      $reply = CONST_not_concerned;
      if (empty($name)) $name = 'visitor';
      $this->dbg("%s, %s, <em>%s</em>",var_export($reply,True),$case,$name);
      return $reply;
    }
    
    // Is it the status request?
    if ($lease_id === 'status'){
      $this->m_applicant_info_mail($this->visit,'status',False);
      $reply = ($this->visit->getPolicy(VM_V_welcomeMailQueued)
		? in_queue
		: $this->visit->getPolicy(VM_V_welcomeMailSent));
      $this->dbg("$reply ".x('em',$this->visit->av->fmtName('fl')));
      return $reply;
    }

    // This mail to be sent once, but there is no harm to send it again
    if ($no_preview && $this->visit->getPolicy(VM_V_welcomeMailSent) === True){
      $this->dbg("Mail to ".$this->visit->av->name()." was already sent");
    }
        
    // Put this mail into the execution queue, not waist time in the interactive session
    if (!myPear_taskManager()->exec_request && $no_preview){
      myPear_taskManager()->addTask(get_class($this),__function__,array($this->visit->ID,
									$lease_id,
									True,
									Null,	
									(is_object(bAuth::$av)
									 ? array(bAuth::$av->getEmail())
									 : array())));
      $this->visit->setPolicy(VM_V_welcomeMailQueued,1);
      return True;
    }elseif (empty($bcc) && is_object(bAuth::$av)){
      $bcc[] = bAuth::$av->getEmail();
    }
    
    /*
     * Accommodation block (not for the local people)
     */
    $blabla = array();
    $include_accommodation_in_welcomeMail = (include_accommodation_in_welcomeMail && !empty($lease_id) && $this->visit->hut_codes($without_BI=True));
    $this->dbg("include_accommodation_in_welcomeMail=".var_export($include_accommodation_in_welcomeMail,True),cnf_dev);
    if ($include_accommodation_in_welcomeMail){

      $a = myPear::getInstance_new('bForm_vm_Room',$lease->getValue('lease_aid'),array('strict'));
      $a_name = bForm_vm_Room::_a_name($lease->formDB,'for_applicant');      
      if (strpos($a_name,'[Shared] ')) $a_name = str_replace('[Shared] ','',str_replace('Room ','Shared room ',$a_name));
      $billedTo = (empty($a_name)
		   ? array()
		   : array($a_name.'.'));

      // Get aaccompaning persons 
      var_dump($this->visit->getValue('v_accompaning',True));
      $v_accompaning = (($n = $this->visit->getValue('v_accompaning',True))
			? array('',"You are expected to be accompanied by $n person".($n>1?'s':'').'.','')
			: array());

      // Get cotenants
      $cotenant = (($ct = $this->visit->cotenants())
		   ? array('','You will share the room with ' . join(', ',$ct).'.','')
		   : array());
      if (!empty($cotenant)) $v_accompaning = array();
      
      $blabla[] = '
ACCOMMODATION
-------------
Your accommodation will be at '.$a->hut->name().' in '.$a->hut->getValue('hut_city',1).'. '.join(", ",$billedTo).'
'.join("\n",$cotenant).join("\n",$v_accompaning).'
You will receive a separate confirmation of the room reservation from '.$a->hut->name().'. '.
'Any changes in dates are subject to availability and should be taken up directly with '.$a->hut->name(). // ', '.myOrg_nameS.' can\'t help with that '. 
' (see '.$a->hut->getValue('hut_url').' for contact information).

Please read carefully the cancellation policy. The violation of the cancellation policy will be on Your charge.
';
    }
    
    
    /*
     * Reimbursement block
     */
    $exp = $this->visit->getExp();
    $VM_V_payTrip    = $this->visit->getPolicy(VM_V_payTrip);
    $VM_V_payPerdiem = $this->visit->getPolicy(VM_V_payPerdiem);
    
    $include_payForms_in_welcomeMail = (include_payForms_in_welcomeMail && ($VM_V_payTrip || $VM_V_payPerdiem));
    $this->dbg("include_payForms_in_welcomeMail=".var_export($include_payForms_in_welcomeMail,True),cnf_dev);
    $this->attachments = array();
    $cc = array();
    if ($include_payForms_in_welcomeMail){
      $cc[] = reimbursement_email;
      foreach(VM::$forms2send as $pdf_tpl){
	$this->attachments[] = $exp->pdfReceipt($pdf_tpl,sys_get_temp_dir() . "/${pdf_tpl}_" . $this->visit->ID . '.pdf');
      }
    }
    
    $pd = array();
    $this->dbg("VM_V_payTrip=".var_export($VM_V_payTrip,True),cnf_dev);
    if ($VM_V_payTrip){
      $trip  = $exp->getValue('exp_travel_est', True);
      $pay[] = 'contribute up to '.b_fmt::money($trip).' Swedish Crowns towards the cost of your return travel to '.myOrg_name;
    }
    
    $this->dbg("VM_V_payPerdiem=".var_export($VM_V_payPerdiem,True),cnf_dev);
    if ($VM_V_payPerdiem){
      $perdiem = $exp->getValue('exp_perdiem_est',True);
      $pay[] = 'provide a '.VM_wordPerdiem.' of '.b_fmt::money($perdiem).' Swedish Crowns towards local expenses';
      $pd = array('','If you are receiving a '.VM_wordPerdiem.' the signed forms need to be received by us three weeks in advance of your visit.');
    }   
    
    if (!empty($pay)){
      $blabla[] = '
REIMBURSEMENT
-------------
We will '.join(" and ",$pay).'.

';
      if ($n=count($this->attachments)) $blabla[] = "For this we need you to fill the attached forms.

Please provide your name, home address, date of birth, the dates that you are traveling, and the date of your PhD (the last item is required by local university regulations).

It is important to fill in the bank information also, with name and address of the bank, IBAN number, and Swift code. Otherwise the money transfer for your reimbursement cannot be issued.

Bring the forms together with copies of your tickets to the Nordita administration. You can also mail it to reimbursement@nordita.org .
".join("\n",$pd);
    }
    
    
    //
    // personal web site
    //
    $this->dbg("include_personalPage_in_welcomeMail=".var_export(include_personalPage_in_welcomeMail,True),cnf_dev);
    if (include_personalPage_in_welcomeMail){
      if (!$this->visit->av->isE(True)) $blabla[] = '
PERSONAL WEB PAGE
-----------------
We have created a web page for you which you can access here:
{login_url}

Please use this page to update your contact information. We also ask for the following:
- your birth date (needed for registration of wireless Internet access),
- a photograph of yourself so that we, and your fellow participants, can put a face together with your name.

Via your personal page you obtain more information about the event, including photos and dates of stay of other participants.';
      if (!empty($pay)) $blabla[] = '
Note, that the forms for the reimbursement get your personal information, like the address, affiliation, email, etc.  from (naively saying) "your web page". Please check that your record is correct.'; 
    }
    
    //
    // Gather all the pieces...
    //
    $messageBody = 'Dear '.$this->visit->av->fmtName('sfl',0).',
 
'.(is_object($this->visit->e)
   ?'This is to confirm that your application for the upcoming '.myOrg_name.' Event "'.$this->visit->e->name(False).'" which takes place '.b_time::period($this->visit->e->getValue('e_start',1),$this->visit->getValue('e_end',1),'h').' has been accepted.

We look forward '.($this->visit->av->isE(True) ? '' : 'to seeing you at '.myOrg_name).' and hope you will have a pleasant and productive meeting.
'.($this->visit->av->isE(True) ? '' : 'For directions to '.myOrg_name.", see http://www.nordita.org/map\n")
   .join("\n",$blabla).'

Sincerely,
Organizing committee for "'.$this->visit->e->name(False).'".'
   :'');
    
    // Now send the mail
    if ($ok = $this->send(array($this->visit->av,
				array('q'=>sprintf("%s/%s/",VM_MODULE,APImenu::tab_code(tab_vm_home))),
				False),
			  $this->visit->av->getEmail(),
			  (is_object($this->visit->e)?$this->visit->e->name():'Your visit to '.myOrg_name),
			  $messageBody,
			  $syslog_thisMail=True,
			  $no_preview,
			  $cc,
			  $bcc)){
      $this->visit->setPolicy(VM_V_welcomeMailSent,1,True,True);
      $this->visit->setPolicy(VM_V_welcomeMailQueued,0);
      $this->m_applicant_info_mail($this->visit,'status',False);
      myPear::MESSAGE(sprintf('Confirmation mail is sent to %s',$this->visit->av->name()));
    }
    $this->dbg('Mail to '.$this->visit->av->fmtName('fl').' is '.($ok ? 'OK' : 'NOT sent ...'),cnf_dev);
    return $ok;
  }
  

  /*
   * @param - return array(array($emails),array($names))
   */
  private function get_secretaries($duty=DUTY_guests){
    $this->dbg();
    $secretaries = VM_administrators()->membersOnDuty($duty);
    if (empty($secretaries)){
      myPear::WARNING("Please update the administrators list and nominate a person responsible for the claims reimbursement");
    }
    return array(array_values($secretaries),  // emails
		 array_keys($secretaries));   // names
  }
}
