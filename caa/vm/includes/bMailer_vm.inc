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
    $o = myPear::getInstanceFromID($id,'class');
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
	     'event'       =>array('r'=>(is_object($e) ? $e->nameSimple() : ''),
				   'd'=>'Event title'),
	     'event_period'=>array('r'=>(is_object($e) ? b_time::period($e->getValue('e_start',1),$e->getValue('e_end',1)):''),
				   'd'=>'Event duration'),
	     'visit_period'=>array('r'=>(is_object($v) ? b_time::period($v->getValue('v_start',1),$v->getValue('v_end',1)):''),
				   'd'=>'Stay period'),
	     );
  
  $reply = array();
  switch($what){
    
  case 'SHOW_ID':
    $reply = __METHOD__;
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

  var $mailID = "Visits Manager API";
  var $isMailAutomatic = False;
  var $visit  = Null;
  var $debug  = False;
  
  private $rec    = array();

  public static $isReady = 0;
  public function __construct(){

    self::$wrapper = 'VM_mailer'; 

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
   * Automatically remind organizers that the deadline is reached.
   * The code needs further debugging - 
   * rather often the organizers put to agenda events which have nothing to do with us, need to have good filters...
   */
  private $info = array();
  public function cronjob_remind_organizers(){
    $verbose = cnf_dev;    
    $this->dbg(starting); 
   
    // Do it ones per session...
    if (!@$_SESSION[__METHOD__]++){
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
	      if (!$v->getPolicy(VM_V_rdlMailSent)){
		$info_body[] = 'Informing organizer '.$v->av->fmtName('Lf');
		$this->remind_organizer($v,$e,$mode='rdl',$no_preview=True);
	      }
	    }elseif(VM_manager_here){
	      $av = new bForm_Avatar($av_id);
	      myPear::WARNING($msg = 'Organizer '.$av->fmtName('fl').' has not yet registered for his event '.$e->name());
	      $info_body[] = $msg;
	    }
	  }
	}
	if ($verbose) myPear::INFO($info_body,$info_title);
      }
    }
    $this->dbg(exiting); 
  }
  
  /*
   * There are 3 cases when this mail is sent:
   * - 'pwd' just remind the password, might be sent at any time
   * - 'rdl' the event registration deadline is reached, invite the organizer to look into his event
   * - 'en'  the event budget is endorsed, inform the organizer about it
   */
  public function remind_organizer($av_id,$e,$mode='pwd',$no_preview=True){
    $this->dbg(starting); 

    // Parse the arguments
    if ($av_id instanceof bForm_Avatar_vm){
      $av = $av_id;
    }elseif($av_id instanceof bForm_vm_Visit){
      $v  = $av_id;
      $av = $v->av;
    }else{
      $av = myPear::getInstance('bForm_Avatar_vm',$av_id,array('strict'));
    }

    // Continue parsing, the argument might be the visit ID...
    if (empty($av)){
      $this->dbg("Argument is not 'av_id', try bForm_vm_Visit");
      $v = new bForm_vm_Visit($av_id);
      $av = $v->av;
    }

    // Parse "event" argument
    if (!($e instanceof bForm_vm_Event)) $e = myPear::getInstance('bForm_vm_Event',$e);
    
    // Guess the visit if not given explicitly
    if (empty($v)){
      if (!($v_id = $e->getVisit_ID($av->ID))){
	myPear::WARNING($av->fmtName('fl').' is not yet registered for "'.$e->name().'"<br>No mail sent');
	return;
      }
      $v = new bForm_vm_Visit($v_id);
    }

    // Is the mail should be sent once only?
    // Don't send it if it is too late...
    $policy = Null;
    if ($mode == 'en')  $policy = VM_V_enMailSent;
    if ($mode == 'rdl') $policy = VM_V_rdlMailSent;
    if (!empty($policy) && (time() > $e->getValue('e_start'))) $v->setPolicy($policy,1);
    
    if (!empty($policy) && $v->getPolicy($policy)){
      $this->_dbg("Already done... ".exiting);
      return;
    }
    
    // Put this mail into the execution queue, not waist time in the interactive session
    $fqn  = $av->fmtName('sfl',0);
    if (!myPear_taskManager()->request && $no_preview && ($no_preview !== 'send_now')){
      $this->dbg("Queue the request"); 
      myPear::MESSAGE(sprintf("E-mail to %s is send to the mail queue",$fqn));
      myPear_taskManager()->addTask(get_class($this),__function__,array($av->ID,$e->ID,$mode,$no_preview));
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
      default:
	$this->dbg("Unexpected argument mode='$mode'",cnf_dev);
	return Null;
      }
      
      $body .= "<packPWD>

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
	switch($mode){
	case 'rdl':  // ----------------------- application deadline is over
	  myPear_logs()->add('reminder',$av->ID,'applications deadline is over',myPear_cronjob()->ID);
	  break;
	case 'en': // ----------------------- the event budget is now endorsed
	  myPear_logs()->add('info',$av->ID,'event is endorsed',myPear_cronjob()->ID);
	}
	if (!empty($policy) && !empty($v)){
	  $this->_dbg("setPolicy($policy,1,True,True)");
	  $v->setPolicy($policy,1);
	}
      }
    }
    $this->dbg("Mail to '$email' is ".($ok ? 'OK' : 'NOT sent ...'),cnf_dev);
    $this->dbg(exiting); 
  }
  
  /*
   * Remind the relevant people to approve the pending endorsements
   */
  public function remind_endorser($av_id,$rec,$no_preview=True){

    // Put this mail into the execution queue, not waist time in the interactive session
    if (!myPear_taskManager()->request && $no_preview){
      foreach(array('v_id','av_lastname','av_firstname','av_salutation') as $i) $r[$i] = @$rec[$i];
      myPear_taskManager()->addTask(get_class($this),__function__,array($av_id,b_fmt::safe_encode($r),$no_preview));
      return;
    }

    // Might be called from the queue...
    $rec = b_fmt::safe_decode($rec);

    $av = myPear::getInstance('bForm_Avatar',$av_id);
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
<packPWD>
to approve or deny the request.
',
		$syslog_thisMail=False,
			  $no_preview)){
      myPear_logs()->add('reminder',$av_id,'approve visit '.bForm_avatar::_fmtName('Lf',$rec),myPear_cronjob()->ID);
      $this->dbg('done');
    }
  }
  

  /*
   * Inform secretaries that there a collaboration visit approved/canceled
   *
   * @param $v  - visit class
   * @param $av - secretary avatar class
   * @param $v_status - reject or accept
   */
  public function inform_secretary_about_visit($v,$s_email,$v_status,$endorser,$no_preview=True){

    $this->dbg();
    if (!($v instanceof bForm_vm_Visit)) $v = new bForm_vm_Visit($v);

    // Is it our business?
    if ($v->getValue('v_end',True,True) < time())                    return;
    if (!in_array(VM_V_scrtrInfoSent,array_keys($v->getPolicies()))) return;
    if ($v->getPolicy(VM_V_scrtrInfoSent))                           return;
    
    // Put this mail into the execution queue, not waist time in the interactive session
    if (!myPear_taskManager()->request && $no_preview){
      myPear_taskManager()->addTask(get_class($this),__function__,array($v->ID,$s_email,$v_status,$endorser,$no_preview));
      return;
    }

    // Sent the mail
    $secretaries = bForm_Avatar::hookByEmail($s_email,$reply_with_ID=False);
    $secretary = array_pop($secretaries);
    if (VM_administrator_here || cnf_dev){
      myPear::MESSAGE("Informing administrator ".$secretary->name()." about visit of ".$v->av->fmtName('Lf'));
    }      
    $subject = "Visit of ".$v->av->fmtName('fl',0).' is '.($v_status == STATUS_YES ? 'APPROVED' : 'CANCELED');
    $body = "Dear ".$secretary->fmtName('f',0).",
$subject by $endorser.
".($v_status==STATUS_YES 
   ? "You can access the visit record by clicking on the URL:
<packPWD>
" 
   : "");
    if ($ok = $this->send(array($secretary,
				array('form'=>'vm_Visit',
				      'id'=>$v->ID)),
			  $s_email,
			  $subject,
			  $body,
			  $syslog_thisMail=True,
			  $no_preview)){
      myPear_logs()->add('info',$secretary->ID,'visit of '.$v->av->fmtName('Lf'),myPear_cronjob()->ID);
      $v->setPolicy(VM_V_scrtrInfoSent,1,True,True);
    }
  }  

  /*
   * This mail has to be sent only once
   */
  public function m_registrant_info($v_id,$no_preview=False){
    
    $this->dbg();
    if (empty($v_id)) return;

    //    if (!send_automatically_infoMail_to_registrants)    $this->dontSendThisMail = 'NOT send_automatically_infoMail_to_registrants';
    // Get visit class    
    if ($v_id instanceof bForm_vm_Visit) $this->visit = $v_id;
    else                                 $this->visit = myPear::getInstance('bForm_vm_Visit',$v_id);
    if (!($this->visit  instanceof bForm_vm_Visit)){
      myPear::WARNING_T("??? Wrong argument function m_registrant_info(v_id=$v_id)");
      return;
    }

    // Sanity, don't be too verbose
    if($this->visit->getPolicy(VM_V_denialMailSent))  $this->visit->setPolicy(VM_V_infoMailSent,1);
    if($this->visit->getPolicy(VM_V_welcomeMailSent)) $this->visit->setPolicy(VM_V_infoMailSent,1);
    
    // Is it just an enquiry?
    $VM_V_infoMailSent = $this->visit->getPolicy(VM_V_infoMailSent);
    if ($no_preview === 'status') return $VM_V_infoMailSent;    

    // This mail has to be sent only once
    if ($no_preview && $VM_V_infoMailSent){
      $this->dbg("Mail to ".$this->visit->av->name()." was already sent",$this->debug||cnf_dev);
      return;
    }

    // This mail should not be sent to the organizers, hence set the flag "mail is already sent"
    if ($this->visit->av->isOrganizer($this->visit->e->ID)){
      myPear::WARNING_T("Don't send  info mail to organizer ".$this->visit->av->fmtName());
      $this->visit->setPolicy(VM_V_infoMailSent,1,True,True);
      return;
    }

    // Don't send it to the employees
    if ($this->visit->av->isE()){
      $this->visit->setPolicy(VM_V_infoMailSent,1,True,True);
      return;
    }
    
    // Put this mail into the execution queue, not waist time in the interactive session
    if (!myPear_taskManager()->request && $no_preview){
      myPear_taskManager()->addTask(get_class($this),__function__,array($this->visit->ID,$no_preview));
      return;
    }
    
    locateAndInclude('bTpl');
    $tpl = new bTpl(array('title' => __METHOD__,
			  'render'=> VM_mailer_render('SHOW_ID'),
			  'body'  => "
Dear <firstname/> <lastname/>,
(Please ignore this message if your application for \"<event/>\" is already approved by <org/>). 

Thank you for your recent application for the <org/> event
\"<event/>\".

We are considering your application and will inform you whether it is accepted or not. 

After your application is accepted, within a month before the start of the event, we will send out all the information about your stay. For practical issues check our web page http://www.nordita.org/guests, or contact the event organizers." .
// " or our program coordinator $secretary[name], $secretary[mail].".
"

Best regards,
Organizing committee for \"<event/>\".
"));
    
    $this->template = __METHOD__;
    $ok = $this->send(array(),
		      $this->visit->av->getEmail(),
		      'Your application for '.myOrg_name.' event',
		      $tpl->render(array('v_id'=>$v_id)),
		      $syslog_thisMail=True,
		      $no_preview);
    if ($ok) $this->visit->setPolicy(VM_V_infoMailSent,1,True,True);
    $this->dbg('Mail is '.($ok ? 'OK' : 'NOT sent ...'),cnf_dev);
  }
  
  
  /*
   *
   */
  public function m_registrant_deny($v_id=0,$no_preview=False){
    
    $this->visit = myPear::getInstance('bForm_vm_Visit',$v_id);

    $VM_V_denialMailSent = $this->visit->getPolicy(VM_V_denialMailSent);
    if ($no_preview === 'status'){
      $this->m_registrant_info($this->visit,'status');
      return $VM_V_denialMailSent; 
    }

    // This mail has to be sent only once
    if ($no_preview && $VM_V_denialMailSent){
      $this->dbg("Mail to ".$this->visit->av->name()." was already sent",$this->debug||cnf_dev);
      return;
    }
    
    // Put this mail into the execution queue, not waist time in the interactive session
    if (!myPear_taskManager()->request && $no_preview){
      myPear_taskManager()->addTask(get_class($this),__function__,array($this->visit->ID,$no_preview));
      return;
    }

    locateAndInclude('bTpl');
    $tpl = new bTpl(array('title' => __METHOD__,
			  'render'=> VM_mailer_render('SHOW_ID'),
			  'body'  => "
Dear <firstname/> <lastname/>,

Thank you for your recent application for the Nordita event 
\"<event/>\".

We regret to inform you that we are not able to accommodate your request to participate.

Sincerely,
Organizing committee for \"<event/>\"."));
      
    $this->template = __METHOD__;
    if ($ok = $this->send(array(),
			  $this->visit->av->getEmail(),
			  "Application denied",
			  $tpl->render(array('v_id'=>$v_id)),
			  $syslog_thisMail=True,
			  $no_preview)){
      $this->visit->setPolicy(VM_V_denialMailSent,1,True,True);
      $this->m_registrant_info($this->visit,'status');
    }
    $this->dbg('Mail to '.$this->visit->av->fmtName('fl').' is '.($ok ? 'OK' : 'NOT sent ...'),cnf_dev);
  }
  
  /*
   *
   */
  public function registrant_welcome($v_id,$hut_id,$a_name='',$no_preview=False){

    $this->dbg();

    $this->visit = myPear::getInstance('bForm_vm_Visit',$v_id,array('strict','fatal'));
    if ($hut_id === 'status'){
      $this->m_registrant_info($this->visit,'status');
      return $this->visit->getPolicy(VM_V_welcomeMailSent);
    }

    if (!send_finalWelcomeEmail_to_registrants){
      return;
      // $this->dontSendThisMail = 'NOT send_finalWelcomeEmail_to_registrants';
    }

    // This mail to be sent once, but there is no harm to send it again
    if ($no_preview && $this->visit->getPolicy(VM_V_welcomeMailSent) === True){
      $this->dbg("Mail to ".$this->visit->av->name()." was already sent");
    }
        
    // Put this mail into the execution queue, not waist time in the interactive session
    if (!myPear_taskManager()->request && send_finalWelcomeEmail_to_registrants && $no_preview){
      myPear_taskManager()->addTask(get_class($this),__function__,array($this->visit->ID,$hut_id,$a_name,$no_preview));
      return;
    }
    
    /*
     * Accommodation block (not for the local people)
     */
    $blabla = array();
    if (include_accommodation_in_finalWelcomeEmail && !empty($hut_id) && $this->visit->hut_codes($without_BI=True)){
      $hut = myPear::getInstance('bForm_vm_Hut',$hut_id,array('strict','fatal'));
      if (!($hut instanceof bForm_vm_Hut)){
	// Sanity... there might be(?) a problem in "myPear::getInstance"...
	myPear::WARNING_T("Can't get myPear::getInstance('bForm_vm_Hut',$hut_id)");
	$hut = new bForm_vm_Hut($hut_id);
      }
      if (($hut instanceof bForm_vm_Hut) && ($hut->ID == $hut_id)){
	$hut->defineVariables();
	
	// Who pays the accommodation (TBD)?
	if (!empty($a_name)){
	  if (strpos($a_name,';')===False) $a_name .= ';the cost will be billed to '.myOrg_name;
	  list($n,$condition) = explode(';',str_replace(array('[',']'),'',$a_name),2);
	  if (is_numeric($n)) $n .= ' inhabitant'.($n>1?'s':'');
	  $billedTo = array($n,$condition.'.');
	}else{
	  $billedTo = array();
	}

	// Get cotenants
	if ($ct = $this->visit->cotenants()) $cotenant = array('','You will share the room with ' . join(', ',$ct),'');
	else                                 $cotenant = array();
	
	// Get the paying conditions
	$accommodation_cost = 'The cost will be billed to '.myOrg_name;
	$blabla[] = '
ACCOMMODATION
-------------
Your accommodation will be at '.$hut->name().' in '.$hut->getValue('hut_city',1).'. '.join(", ",$billedTo).'
'.join("\n",$cotenant).'
You will receive separate confirmation of the room reservation from '.$hut->name().'. '.
'Any changes in dates are subject to availability and should be taken up directly with '.$hut->name().', 
(see '.$hut->getValue('hut_url').' for contact information).

Please read carefully the cancellation policy. The violation of the cancellation policy will be on Your charge.
';
      }else{
	myPear::WARNING_T("Can't find hut($hut_id) for ".$this->visit->av->name());
      }
    }
    
    
    /*
     * Reimbursement block
     */
    $exp = $this->visit->getExp();
    $VM_V_payTrip    = $this->visit->getPolicy(VM_V_payTrip);
    $VM_V_payPerdiem = $this->visit->getPolicy(VM_V_payPerdiem);
    
    $this->attachments = array();
    if (include_payForms_in_finalWelcomeEmail && ($VM_V_payTrip || $VM_V_payPerdiem)){
      if (!cnf_dev) $this->cc[] = 'reimbursement@nordita.org';
      foreach(VM::$forms2send as $pdf_tpl){
	$this->attachments[] = $exp->pdfReceipt($pdf_tpl,sys_get_temp_dir() . "/${pdf_tpl}_" . $this->visit->ID . '.pdf');
      }
    }
    
    $pd = array();
    if ($VM_V_payTrip){
      $trip  = $exp->getValue('exp_travel_est', True);
      $pay[] = 'contribute up to '.b_fmt::money($trip).' Swedish Crowns towards the cost of your return travel to '.myOrg_name;
    }
    
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
    
    
    /*
     * personal web site
     */
    if (include_personalPage_in_finalWelcomeEmail){
      if (!$this->visit->av->isE()) $blabla[] = '
PERSONAL WEB PAGE
-----------------
We have created a web page for you which you can access here:
<packPWD>

Please use this page to update your contact information. We also ask for the following:
- your birth date (needed for registration of wireless Internet access),
- a photograph of yourself so that we, and your fellow participants, can put a face together with your name.

Via your personal page you obtain more information about the event, including photos and dates of stay of other participants.';
      if (!empty($pay)) $blabla[] = '
Note, that the forms for the reimbursement get your personal information, like the address, affiliation, email, etc.  from (naively saying) "your web page". Please check that your record is correct.'; 
    }elseif(cnf_dev){
      myPear::WARNING("Block 'Personal web page' is not included in the Email");
    }
    
    /*
     * Have a nice stay in Stockholm
     */
    if ($this->visit->av->isE()) $blabla[] = '
Hope you will have a pleasant and productive meeting.
';
    else  $blabla[] = '
We look forward to seeing you at '.myOrg_name.' and hope you will have a pleasant and productive meeting.
';

    // Gather all the pieces...
    $messageBody = 'Dear '.$this->visit->av->fmtName('sfl',0).',
 
'.(is_object($this->visit->e)
   ?'You are registered to participate in the upcoming '.myOrg_name.' Event "'.$this->visit->e->name(False).'" which takes place '.b_time::period($this->visit->getValue('v_start',1),$this->visit->getValue('v_end',1),'h').'.

For directions to '.myOrg_name.', see http://www.nordita.org/map

'.join("\n",$blabla).'
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
			  $no_preview)){
      $this->visit->setPolicy(VM_V_welcomeMailSent,1,True,True);
      $this->m_registrant_info($this->visit,'status');
    }
    $this->dbg('Mail to '.$this->visit->av->fmtName('fl').' is '.($ok ? 'OK' : 'NOT sent ...'),cnf_dev);
  }
  

  /*
   * @param - return array(array($emails),array($names))
   */
  private function get_secretaries($duty=DUTY_guests){
    $secretaries = VM_bookers()->membersOnDuty($duty);
    if (empty($secretaries)){
      myPear::WARNING("Please update the administrators list and nominate a person responsible for the claims reimbursement");
    }
    return array(array_values($secretaries),  // emails
		 array_keys($secretaries));   // names
  }
}
