<?php

//
// Limit the number of mails sent per "click", some smtp servers are crazy about too many mails
//
define('bMailer_Max_mails_per_click',113);
define('bMailer_dev_address','yb.mypear@gmail.com');

/*
 * mailer class
 */
locateAndInclude('myPear_base');
if (locateAndInclude(array('Mail',
			   'PEAR/Mail'),
		     'any')){
  locateAndInclude(array('Net_Socket',
			 'Net_SMTP'),
		   'fatal');
  
  class bMailer extends myPear_base{
    
    // used during the initial/development stage
    var $bcc           = array(bMailer_dev_address);

    var $template      = False;
    var $debug         = False;
    var $signature     = '';
    var $from          = '';
    var $cc            = '';
    var $object        = Null;
    var $isMailAutomatic = True;
    protected $mailID      = 'web application';
    private   $smtp_factory= array();
    protected $attachments = array();
    protected $no_preview  = False;

    private   $recipients        = array();
    private   $recipientsEscaped = array();
    private   $hideURL = False;

  /*
   * send mail(s)
   */
    public  $send_error = '';    
    private $subject = 'No subject';
    protected $dontSendThisMail = False;
    public  $sent_mails_counter = 0;

    // Wrapper function, if any
    public static $wrapper = Null;

    public static function getWrapper(){
      return self::$wrapper;
    }

    /*
     *
     */
    public function send($packPWD_args=array(),
			 $to=nobody_at_nowhere,
			 $subject='mail subject',
			 $message='mail body',
			 $syslog_thisMail=True,
			 $no_preview=False){
      global $listOfSentEmails, $listOfUnsentEmails;

      $this->recipients = $this->headersEscaped = $this->recipientsEscaped = array();
      
      // Skip the installation phase
      if (!$this->isReady){
	if (cnf_dev) myPear::DEBUG('!!!!! '.get_class($this). ' is not initialized, can\'t send mails');
	if (cnf_dev) myPear::ERROR('!!!!! '.get_class($this). ' is not initialized, can\'t send mails');
	return Null;
      }

      if ($no_preview || $this->no_preview || (@$_GET['sendmail_once'] === 'send')){
	$this->no_preview = True;
	$_GET['sendmail_once'] = 'send';
      }
      $this->hideURL = (strpos((string)@$_GET['sendmail_once'],'send') === False);
      
      $this->subject = strip_tags($subject);
      if (!preg_match('/\w.*@/',$this->from)) $this->from = bMailer_noreply;
      $this->_dbg("to=$to,from=".$this->from.",subject=".$this->subject);
      
      if ($n=@$GLOBALS['mailDejaVu'][$to][$this->from][$this->subject]++){
	if ($this->debug||cnf_dev||cnf_demo)  myPear::DEBUG("SKIP dejavu".($n+1).": to=$to subject=".$this->subject);
	return Null;
      }
      
      // Do not remove 'cnf_localhost'...
      $realTO = (cnf_localhost || cnf_dev || cnf_demo 
		 ? cnf_demo_email 
		 : $to);
      // See myPear_taskManager
      if ($task_report_to = @$GLOBALS['task_report_to']){
	myPear::WARNING("Replace recipient by a developer: $realTO ==> $task_report_to");
	//	$realTO = $task_report_to;
      }
      
      if (($realTO != cnf_demo_email) && (cnf_localhost || cnf_dev || cnf_demo)){
	myPear::ERROR("To be sent to <em>$realTO</em>, not to <em>".cnf_demo_email."</em>... Correct?");
	return False;
      }
      
      $semi_rand = md5(time());
      $charset   = 'iso-8859-1';
      $charset   = 'utf8';
      $mime_boundary = "==Multipart_Boundary_x{$semi_rand}x";
      if ($n=count(array_unique($this->attachments))) $this->subject .= " ($n attachment".($n>1?'s':'').")";
      
      // Is an html-formatted message (from internalError)?
      $html_message = (stripos($message,'<html>') !== False);

      $headers_syslog = array('Subject'=> $this->subject_prefix.$this->subject,
			      'To'     => $realTO,
			      'From'   => $this->from);
      $addressLine = array('To'=>$to);
      if ($realTO == $to) {
	if ($this->cc)  $headers_syslog['Cc']  = join(',',array_unique($this->cc));
	if ($this->bcc) $headers_syslog['Bcc'] = join(',',array_unique($this->bcc));
      }else{
	if ($this->cc)  $addressLine['Cc']  = join(',',array_unique($this->cc));
	if ($this->bcc) $addressLine['Bcc'] = join(',',array_unique($this->bcc));
      }
      $headers = $headers_syslog + array('MIME-Version' => '1.0',
					 'Content-Type' => ($html_message
							    ? "text/html; charset=iso-8859-1" 
							    : "multipart/mixed; boundary=\"{$mime_boundary}\""));
      
      // Clean $this->from, a the next message might be waiting
      $this->from = '';
      
      // Finalize the headers, make it easy for debugging
      foreach ($headers as $k=>$v) $headersEscaped[$k] = b_fmt::escape($v); 
      if ($realTO !== $to) {
	$this->_dbg("replace recipient: $to ==> $realTO");
	$headersEscaped['To'] = b_fmt::escape("$realTO<$to>");
	$headers_syslog['To'] = "$realTO(".$to.")";
      }   
      
      foreach (array('To','Cc','Bcc') as $i){  
	if (!empty($headers[$i])){
	  $this->recipients[]        = $headers[$i]; 
	  $this->recipientsEscaped[] = $headersEscaped[$i];
	}
      }

      /*
       * Check that we are able to send mails, smtp server might apply restrictions on the number of mails sent
       */
      if ($this->sent_mails_counter > bMailer_Max_mails_per_click){
	if (cnf_dev) $this->error_message("Maximum number of ".bMailer_Max_mails_per_click." mails per click is reached...");
	$listOfUnsentEmails[] = $headersEscaped['To'];
	return Null;
      }
      
      // Do we want to send this mail?
      if (cnf_dontSendMails_Point || !empty($this->dontSendThisMail)){
	$this->error_message(cnf_dontSendMails_Point
			     ? 'Sending mails is canceled by the configuration'
			     : $this->dontSendThisMail);
	return Null;
      }      
      // Reset to default
      $this->dontSendThisMail = False;

      /*
       * Build the message body
       */
      if ($html_message){
	$messageBody = array(b_fmt::unEscape($message));
      }else{
	$messageBody = ($this->no_preview && !$html_message
			? array("--{$mime_boundary}",
				"Content-Type: text/plain; charset=\"$charset\"",
				"Content-Transfer-Encoding: 7bit",'')
			: array());
	if (cnf_dev||cnf_localhost)$messageBody=array_merge($messageBody,array("--- DISCLAIMER",
								      "--- This E-mail is sent by the test version of the ",
								      "--- '$this->mailID' software and should be ignored ",
								      "--- unless you participate in the debugging and expect it.",
								      "--- Sorry if you got it my mistake...",
								      "--- ".b_fmt::joinX(', ',$addressLine,':'),
							              ''));
	elseif (cnf_demo)          $messageBody=array_merge($messageBody,array("--- DISCLAIMER",
								      "--- This E-mail is sent to You by the trial version of the ",
								      "--- '$this->mailID', everything here is just a test.",
								      "--- However, we need your help in finalizing the software,",
								      "--- hence we encourage you to 'click every button' :-)",
								      "--- Please don't hesitate.",
							              "--- ".b_fmt::joinX(', ',$addressLine,':'),
								      ''));
	
	// Massage packPWD arguments
	if (!isset($packPWD_args[0])) $packPWD_args[0] = Null;
	if (!isset($packPWD_args[1])) $packPWD_args[1] = array();
	if (!isset($packPWD_args[2])) $packPWD_args[2] = False;
	
	$messageBody[] = b_fmt::unEscape(str_replace('{login_url}',$this->packPWD($packPWD_args[0],
										$packPWD_args[1],
										$packPWD_args[2]),$message));
	
	$autoSignature = "this ".($this->isMailAutomatic ? 'is an automatic e-mail' : 'e-mail is')." sent by the $this->mailID software"; 
	$autoSignatureDelimeter = ''; $l=0;  while ($l++ < strlen($autoSignature)) $autoSignatureDelimeter .= '-';
	$messageBody[] = $autoSignatureDelimeter;
	$messageBody[] = $autoSignature;
      }
      

      // Either preview OR send OR imitate send
      $sendmail_once = @$_GET['sendmail_once'];
      $this->_dbg("sendmail_once=".var_export($sendmail_once,True));
      switch($sendmail_once){
      case 'send':  // ----------- sending the mail
	$messageBody[] = '';
	foreach(array_unique($this->attachments) as $file){
	  if(is_file($file) && ($fp = @fopen($file,"rb"))){
	    $data = @fread($fp,filesize($file));
	    @fclose($fp);
	    $data = chunk_split(base64_encode($data));
	    $messageBody = array_merge($messageBody,array("--{$mime_boundary}",
							  "Content-Type: application/octet-stream; name=\"".basename($file)."\"",
							  "Content-Description: ".basename($file),
							  "Content-Disposition: attachment;\n" . " filename=\"".basename($file)."\"; size=".filesize($file).";",
							  "Content-Transfer-Encoding: base64",
							  "",
							  $data,
							  ""));
	  }else{
	    myPear::ERROR("Declared attachment '$file' is not accessible");
	  }
	}
	$this->attachments = array();

	if (!$html_message) $messageBody[] = "--{$mime_boundary}--";
	$headers['X-Mailer']= get_class($this);

	ob_start();
	$pear = new PEAR();
	$mail_object = @Mail::factory('smtp', $this->smtp_factory);
	if($pear->isError($mail_object)){
	  // Connect to the SMTP server fails
	  $this->error_message($mail_object->getMessage(),True);
	  $output = ob_get_contents();
	  ob_end_clean();
	  $this->_dbg($output);
	  myPear_logs()->syslog('sendmail FAIL '.str_replace(array("\n","\r"),'; ',$output));
	  return False;
	}

	// Workaround, problem in  Mail_smtp & Net_SMTP
	// ( Non-static method PEAR::isError() should not be called statically...) 
	$error_reporting = error_reporting(0);
	$send = $mail_object->send(join(',',array_unique($this->recipients)), 
				   $headers,
				   b_crypt::encode_utf8(join("\n",$messageBody)));
	error_reporting($error_reporting);

	$output = ob_get_contents();
	ob_end_clean();
	if (!empty($output)) $this->_dbg(x('pre',str_replace("\n\n","\n",$output)));
	
	if($pear->isError($send)) { 
	  // Sending mail fails
	  // Remove CC/BCC from the error message
	  $err_msg = $send->getMessage();
	  foreach(array('Cc','Bcc') as $h) if ($s=@$headers[$h]) $err_msg = str_replace($s,'',$err_msg);
	  $this->error_message($err_msg,True);
	  myPear_logs()->syslog('sendmail FAIL '.b_fmt::joinX(' ',$headers_syslog,'"'));
	  return False;
	}

	  // Sleep after sending a mail, protect against paranoiac smtp servers
	sleep(1);
	$this->sent_mails_counter++;
	
	myPear_logs()->syslog('sendmail OK '.b_fmt::joinX(' ',$headers_syslog,'"'));
	if (cnf_dev || $syslog_thisMail) $listOfSentEmails[$headersEscaped['To']][] = $this->subject_prefix.$this->subject;
	$this->_dbg("send OK");
	
      case 'mimic_send': // ----------- do not send the mail, but set a flag as if it was sent

	// This piece of code as kept for sentimental reasons only. YB
	/*
	if ($this->callAgainOnSuccess){
	  // invoke again the calling function to complete the post-send actions (receipt)
	  $callStack = debug_backtrace(false);
	  $args  = $callStack[1]['args'];
	  $method= $callStack[1]['function'];
	  $this->receipt = True;
	  $this->_dbg("callAgainOnSuccess: $method(".join(', ',$args).")");
	  switch(count($args)){
	  case 0: $this->{$method}();                          break;
	  case 1: $this->{$method}($args[0]);                  break;
	  case 2: $this->{$method}($args[0],$args[1]);         break;
	  case 3: $this->{$method}($args[0],$args[1],$args[2]);break;
	  }
	  $this->receipt = False;
	}
	*/

	if (!$this->no_preview) print x("div class='width100 align_center'",
					$this->getLink('continue',b_url::getReferrer(array('modal_once'=>0))));
	static $debug_counter=0; $this->_dbg("---------------------------------------------------------$sendmail_once ".(++$debug_counter));
	return True;
	
      default:  // ------------------- preview
	$sendmail_once = 'preview';
	
	$buttons = array();
	if (empty($_GET['preview_once'])){ 
	  // Button "send it"
	  $buttons[]  =  $this->getLink('send this email',
					b_url::same(array('modal_once'  =>bJS()->modal_once(@$_GET['modal_once']),
							  'sendmail_once'=>'send'),array(),'keepAll')); 
	  $buttons[]  =  '';
	  
	  // Button "imitate sending"
	  if (superUser_here){
	    $buttons[] = $this->getLink('imitate sending',
					b_url::same(array('modal_once'  =>bJS()->modal_once(@$_GET['modal_once']),
							  'sendmail_once'=>'mimic_send'),array(),'keepAll')); 
	    $buttons[] = '';
	  }
	}
	
	// Edit email template 
	// not yet fully debugged
	if (False) if (cnf_dev && ($tpl=$this->template)){ 
	    $this->_dbg('edit email...  TBD');
	    $buttons[] = $this->getLink('edit email',b_url::same("?tab_force=".tab__templates."&tpl_title=$tpl"));
	    $buttons[] = '';
	  }
	
	// Say "cancel this email"
	$close = (cnf_inside_fb_iframe 
		  ? $this->getLink('Cancel',bJS()->close_modalWindow('href'))
		  : $this->getLink('Cancel',b_url::same(array(),array(),'keepAllBut')));
	if (!empty($close)) $buttons[] = $close;
	
	$id = 'PREVIEW';
	foreach(array('MIME-Version','Content-Type','Cc','Bcc') as $i)   unset($headersEscaped[$i]);
	if (empty($_GET['preview_once']))      print x('h3','The following E-mail is prepared, but is NOT yet sent:');
	new b_table_simple(array(join('<br>',$buttons),
				 x('strong',join('<br>',array(b_fmt::joinX('<br>',$headersEscaped,':'),'',''))).$this->tt($messageBody)));
	static $debug_counter=0; $this->_dbg("---------------------------------------------------------$sendmail_once ".(++$debug_counter));
	return Null;
      }
    }

    /*
     *
     */
    private function error_message($msg,$add_SMTP_info=False){
      $SMTP_info = ($add_SMTP_info
		    ? '<br/>'.x('strong','SMTP server: ').b_fmt::joinX(', ',$this->smtp_factory)
		    : '');
      myPear::WARNING(join('<br>',array(x('strong','E-mail is NOT sent!!'),
					x('strong','To: ').join(', ',array_unique($this->recipientsEscaped)),
					x('strong','Subject: ').$this->subject . $SMTP_info,
					x('em',x('strong','Reason: ').$msg),
					'&nbsp;')));
    }
    
    /*
     * Format the mail body for preview
     */    
    private function tt($messageBody){
      if (True){
	return x('tt',str_replace("\n","<br>",str_replace("\n\n\n","\n\n",b_fmt::escape(join("\n",$messageBody)))));
      }else{
	return x('pre',b_fmt::escape(join("\n",$messageBody)));
      }
    }    
    

    /*
     *
     */
    public function remind_password($av_id,$asReminder=False){

      $this->dbg();      
      if ($av_id instanceof bForm_Avatar) $av = $av_id;
      else                                $av = myPear::getInstance_new(bForm_Avatar::$bForm_Avatar,$av_id,array('strict','nocache'));
      $this->no_preview = True;
      $this->send(array($av),
		  $av->getEmail(),
		  "Your $this->mailID password",
		  "Dear ".$av->fmtName('sfl').",
".($asReminder 
   ? ("We have received the password reminder request.\n".
      "The request came from the web interface from computer '".gethostbyaddr($_SERVER['REMOTE_ADDR'])."'\n\n".
      "Here is the requested information")
   : ("Welcome to $this->mailID,\n\n".
      "We have created a password protected page for you on our servers."))."

For your convenience the password is encoded into the following URL, hence can access your web page directly as:
{login_url}

Remember to save this mail or book-mark the URL to facilitate later access to the web page.
",
		  $syslog_thisMail=True);
    }
    
    /*
     * Pack the username and password into an encrypted URL
     */
    private function packPWD($av, $args=array(), $hideURL=False) {
      
      $this->dbg('$this->hideURL='.var_export($this->hideURL,True),$this->debug||cnf_dev);
      if (!($av instanceof bForm_Avatar)){
	$reply = '';
      }elseif ($hideURL || $this->hideURL){ 
	$reply ='&lt;encoded address of the web page&gt;';
      }else{
	
	// Get the drupal path
	$queryString = array('q' => (empty($args['q'])
				     ? b_reg::$current_module
				     : $args['q']));
	// Build the "hidden URL"
	$toBeEncoded = $args;
	unset($toBeEncoded['q']);
	
	// Add login/pwd, flavor, organization to the hidden part of the URL & encode it
	foreach(array($av->ID => $av->setPWD(),
		      'org'   => myOrg_code,
		      'flavor'=> b_cnf::get('flavor'),
		      ) as $k=>$v) $toBeEncoded[$k] = $v;

	// Add currently authenticated user in the demo mode
	if (bAuth::authenticated() && (cnf_demo || cnf_dev)) $toBeEncoded['email_to_autologged'] = bAuth::$av->getEmail(); 

	$queryString['x'] = b_crypt::encodeX($toBeEncoded);  
	
	// Get the absolute URL 
	$reply = b_url::complete_url(b_fmt::joinX('&',$queryString));
      }
      $this->dbg($reply,$this->debug||cnf_dev);
      return $reply;
    }
    
    private function getLink($text,$url){
      return b_btn::submit($text,$url,False,'bg_green whiteText');
    }
    
    /*
     * Supposed to be called at the end of the page processing
     */
    function showMailsSent($admin_here=False){
      global $listOfUnsentEmails, $listOfSentEmails;
      
      // Report the unsent mails
      if ($admin_here){
	if (empty($listOfUnsentEmails))   $listOfUnsentEmails = array();
	$listOfUnsentEmails = array_unique($listOfUnsentEmails);
	sort($listOfUnsentEmails);
	$n = count($listOfUnsentEmails);
	if (!empty($listOfUnsentEmails)) myPear::WARNING(x('strong',"The following $n email".($n>1?'s are':' is ')." still to be sent:").'<br/>'.
							 join('<br/>',$listOfUnsentEmails));
      }

      // Report the sent mails
      if (empty($listOfSentEmails))     $listOfSentEmails = array();
      $many = (($n=count($listOfSentEmails)) > 1);
      
      if ($n>0) $this->dbg("$n mails sent");

      if ($many){      
	ob_start();
	foreach ($listOfSentEmails as $to=>$v) {
	  if (empty($v) || is_numeric($to))    continue;
	  foreach ($v as $k=>$subject) {
	    if (!myPear::$messageBuffer_active){
	      myPear::MESSAGE_open('We have sent the following email'.($many?'s':'').' with an important information:','strong');
	      $t = new b_table("width='100%'");
	      $t->tro();
	      $t->th('&nbsp;');
	      $t->th($t->x('i',"To:"));
	      $t->th($t->x('i',"Subject:"));
	      $t->trc();
	    }
	    if (!empty($t)){
	      $t->tro();
	      $t->td(array('',$to,$subject));
	    $t->trc();
	    }
	    $to = '';
	  }
	}
	if (!empty($t)) $t->close();
	$fp = ob_get_contents();
	ob_end_clean();
	
	if (myPear::$messageBuffer_active){
	  myPear::MESSAGE_add($fp);
	  myPear::MESSAGE_close();
	}
      }else{
	foreach ($listOfSentEmails as $to=>$v) {
	  if (empty($v) || is_numeric($to))    continue;
	  foreach ($v as $k=>$subject) {
	    myPear::MESSAGE(join('<br>',array('We have sent an important email',
					      x('i',"To: ").$to,
					      x('i',"Subject: ").$subject)));
	  }
	}
      }
      if (!empty($listOfSentEmails) && cnf_inside_fb){
	print x('div width=100% style="margin:20px;text-align:center"',bJS()->close_modalWindow('Close this window'));
      }
      $listOfSentEmails = $listOfUnsentEmails = array();
    }
    
    /*
     * Do not show the argument, it might be too long
     */
    function _dbg($txt=' ',$forced=False){
      if ($forced || $this->debug || cnf_debug){
	$callStack = debug_backtrace(false);
	$method= (empty($callStack[3]['function'])
		  ? $callStack[1]['function']
		  : $callStack[3]['function']);
	$txt = get_class($this)."->".$method."(): $txt";
	if (cnf_CLI) printf("%s\n",$txt);
	else myPear::DEBUG(x('span class="orangeText"',$txt));
      }
    }
    
    /*
     *
     */    
    function __toString(){
      if (is_object($this->object)) return  get_class($this).x('(',get_class($this->object).' '.$this->object->name());
      else                          return  get_class($this);
    } 

    /*
     * Mailer constructor
     */
    private $isReady = False;
    private $subject_prefix  = '';
    function __construct($bMailer_host='',$bMailer_port='',$bMailer_user='',$bMailer_passwd='') {
      
      if (b_cnf::get('smtpdebug')) $this->debug = True;
      $this->_dbg('------------------------------------- '.__FUNCTION__);
      
      $reg = b_reg::get();
      if ($id = @$reg['d'])     $this->mailID = $id;
      
      // Get the mail server variables
      foreach(array('bMailer_host',
		    'bMailer_port',
		    'bMailer_user',
		    'bMailer_passwd',
		    ) as $item){
	if (empty($$item)) $$item = @$GLOBALS[$item];
      }

      // arguments for Mail::factory
      $this->smtp_factory = array('driver'   => 'smtp',
				  'host'     => $bMailer_host,
				  'port'     => $bMailer_port,
				  'timeout'  => 10,
				  'localhost'=> 'localhost', //optional Mail_smtp parameter
				  // 'debug'    => $this->debug
				  );
      if ($auth = (!empty($bMailer_user) && !empty($bMailer_passwd))){
	$this->smtp_factory += array('auth'    => $auth,
				     'username'=> $bMailer_user,
				     'password'=> $bMailer_passwd);
 	ini_set("SMTP","ssl:/".$this->smtp_factory['host']);
	ini_set("smtp_port",   $this->smtp_factory['port']);
      }
      $this->_dbg(b_fmt::joinX(', ',$this->smtp_factory));
      
      $this->isReady = (!empty($this->smtp_factory['host']) && ($this->smtp_factory['host'] !== 'localhost'));
    }
    
    /*
     * Handle attachments
     */
    function multi_attach_mail($to, $files, $sendermail){
      // email fields: to, from, subject, and so on
      $from = "Files attach <".$sendermail.">";
      $subject = date("d.M H:i")." F=".count($files);
      $message = date("Y.m.d H:i:s")."\n".count($files)." attachments";
      $headers = "From: $from";
      
      // boundary
      $semi_rand = md5(time());
      $mime_boundary = "==Multipart_Boundary_x{$semi_rand}x";
      
      // headers for attachment
      $headers .= "\nMIME-Version: 1.0\n" . "Content-Type: multipart/mixed;\n" . " boundary=\"{$mime_boundary}\"";
      
      // multipart boundary
      $message = join("\n",array("--{$mime_boundary}",
				 "Content-Type: text/plain; charset=\"$charset\"",
				 "Content-Transfer-Encoding: 7bit",
				 '',
				 $message,
				 ''));
      
      // preparing attachments
      for($i=0;$i<count($files);$i++){
	if(is_file($files[$i])){
	  $fp =    @fopen($files[$i],"rb");
	  $data =    @fread($fp,filesize($files[$i]));
	  @fclose($fp);
	  $data = chunk_split(base64_encode($data));
	  $message .=
	    "--{$mime_boundary}\n" .
	    "Content-Type: application/octet-stream; name=\"".basename($files[$i])."\"\n" .
	    "Content-Description: ".basename($files[$i])."\n" .
	    "Content-Disposition: attachment;\n" . " filename=\"".basename($files[$i])."\"; size=".filesize($files[$i]).";\n" .
	    "Content-Transfer-Encoding: base64\n\n" . $data . "\n\n";
	}
      }
      $message .= "--{$mime_boundary}--";
      $returnpath = "-f" . $sendermail;
      $ok = @mail($to, $subject, $message, $headers, $returnpath);
      if($ok){ return $i; } else { return 0; }
    }
  }
}else{
  myPear::ERROR("php class 'PEAR/Mail' not found, <a href='http://pear.php.net/search.php?q=mail&in=packages'>please upgrade</a>");
  class bMailer extends myPear_base{
    function __construct(){}
    
    public function send($packPWD_args=array(),
			 $to=nobody_at_nowhere,
			 $subject='mail subject',
			 $message='mail body',
			 $syslog_thisMail=True,
			 $no_preview=False){
      b_debug::xxx(' ');
      return False;
    }
    function showMailsSent(){
      b_debug::xxx(' ');
    }
  }
}
