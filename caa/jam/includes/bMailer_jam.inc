<?php
locateAndInclude('bMailer');
class bMailer_jam extends bMailer{

  var $mailID = 'Job Application Manager';
  private $ap = Null;
  
  public function send($to=nobody_at_nowhere,
		       $subject='mail subject',
		       $message='mail body',
		       $syslog_thisMail=True,
		       $no_preview=False,
		       $pos=Null){
    
    if(!JAM_access()->may_sendMails()){
      myPear::ERROR('Mail sending cancelled by JAM_access()->may_sendMails()');
      b_debug::traceBack();
    }elseif(is_object($pos) && !$pos->isConsidered(True)){    
      myPear::ERROR("Position '".$pos->getValue('pos_nickname')."' is not considered, email to '$to' not sent");
    }else{      
      parent::send($to, $subject, $message, $syslog_thisMail, $no_preview);
    }
  }

  /*
   *
   */
  function acknowlegeExtraReferee() {}

  /*
   *
   */
  function rejectionMail($ap_id) {
    static $n2n = array(1 => 'A',
			2 => 'Two',
			3 => 'Three',
			4 => 'Four',
			5 => 'Five',
			6 => 'Six',
			);
    // We do not need to preview this message
    $this->no_preview = True;
    $this->dbg();

    $ap = myPear::getInstance('bForm_jam_Applicant',$ap_id);
    $applicant_name = $ap->av->fmtName('fl');
    $position_title = $ap->pos->name();
 
    // Count applications
    $q=myPear_db()->query("SELECT COUNT(ap_id) AS n FROM jam_applicants WHERE ap_posid = ".$this->pos->ID);
    while ($r = myPear_db()->next_record($q)) $n_applicants = $r['n'];

    // Count employed applicants
    $q=myPear_db()->query("SELECT COUNT(ap_id) AS n FROM jam_applicants WHERE ap_posid = ".$this->pos->ID
			    . " AND ap_status = '".JAM::status(JAM_ap_employed)."'");
    while ($r = myPear_db()->next_record($q)) $n_employed = $r['n'];
    $n_employed_text =  (isset($n2n[$n_employed]) ? $n2n[$n_employed] : $n_employed);
    $s = ($n_employed > 1 ? 's' : '');

    $this->send($ap->av->getEmail(),
		$position_title,
		"Dear ${applicant_name},

I am sorry to inform you that your application for the \"${position_title}\" was not successful. 

${n_employed_text} position${s} were awarded out of ${n_applicants} applications.

Thank you for your interest in Nordita.
Best regards,

". myOrg($ap->pos->getValue('pos_orgid',1))->getValue('org_signature_dir',1),
		$syslog_thisMail=True);
  }
      
  /*
   * Mail to the applicant - application received
   */
  function acknowlegeApplication($ap) {
    
    $this->dbg();
    $this->ap = $ap;

    // We do not need to preview this message
    $this->no_preview = True;
    
    if ($ap->isStatus(JAM_ap_revoked)){
      myPear::WARNING_T("Revoked application");
    }else{
      $this->informAdmin('application');
      $this->send(array($ap->av),
		  $ap->av->getEmail(),
		  "Job application receipt",
		  "Dear ".$ap->av->fmtName('fl').",

You recently applied for the position of \"".$ap->pos->name()."\" here at ".myOrg_name.", and this e-mail is to acknowledge that your application has been received and registered.

We have created a web page for you on our Applications Processing server, which you can access here:
<packPWD>

You can use this page to check the status of your application, correct any errors, refresh/change the documents you submitted, etc.

We have also contacted the referees you indicated in your application for letters of recommendation.

Good luck with your application.
",
		  $syslog_thisMail=True);
    }
  }
  
  /*
   * Ask to fix errors in the form
   */
  function errorsInApplication($ap_id) {

    // Do not to preview this email    
    $this->no_preview = True;
    $this->dbg();

    $ap = myPear::getInstance('bForm_jam_Applicant',$ap_id);
    $ap->check_formDB();

    // re-pack the errors list
    $errors = array();   
    foreach ($ap->errorsInTheForm as $k=>$msg) {
      if ($d = $ap->getDescr($k)){
	if (stripos($d.$k,'referee ')!==False)  $d = 'Referee';
	else if ($k == 'hidden' || @$dejaVu[$d]++)   continue;
	if ($d == 'Referee') $errors[] = "missing Referee information";
	else                 $errors[] = "$msg for '$d'";
      }
    }

    if (!$errors) {
      myPear::WARNING("No errors in the application, the mail is NOT send");
      return;
    }

    sort($errors);
    array_unshift($errors,'');
    $this->send(array($ap->av),
		$ap->av->getEmail(),
		"Job application - please check your application form",
		"Dear ".$ap->av->fmtName('fl').",

You recently applied for the position of \"".$ap->pos->name() ."\" ". $ap->pos->getDNR()." here at ".myOrg_name.
",  and your application has been successfully processed.

During the processing we have noticed that some important information is missing in your application form:"
		.join ("\n  - ",array_unique($errors)).
"

We have created a web page for you on our Applications Processing server, which you can access here:
<packPWD>

Please use this page to correct the errors, refresh/change the documents you submitted, check the status of your application, etc.

Good luck with your application.
",              
		$syslog_thisMail=True);
  }
  
  /*
   * Mail to the selection committee members with a request to start processing applications
   */
  function inviteFacultyMember($av_id,$pos_id) {

    // We do not need to preview this message
    $this->no_preview = True;
    $this->dbg();

    $av  = myPear::getInstance('bForm_Avatar_jam',$av_id);
    $pos = myPear::getInstance('bForm_jam_Position',$pos_id);
    $this->send($av->getEmail(),
		'Applications for the "'.$pos->name().'"',
		'Dear '.bForm_Avatar::_fmtName('f',$av->formDB).",

as you know the deadline for the applications for
'".$pos->name().$pos->getDNR()."'
has passed, now we are going to process them.

You can access the application data by this link:
".dirname($_SERVER['SCRIPT_URI'])."/ 
Please authenticate with your usual username & password.

For your convenience, we have assembled the application materials for
each applicant (including recommendation letters, personal information,
publication list and research statement) into a single pdf file.

On this secure web-page you can also find contact information for the
members of the Research Committee, who will evaluate the applicants.

". myOrg($pos->getValue('pos_orgid',1))->getValue('org_signature_dir',1),
		$syslog_thisMail=True);
  }

  
  /*
   * Ask a referee to provide the reference letter
   */
  function ask_for_recLetter($av,$ap,$no_preview=True) {

    // This letter might be requested both with/without the preview
    $this->no_preview = $no_preview;
    $this->dbg();

    if ($ap->isStatus(JAM_ap_revoked)){
      myPear::WARNING_T("Revoked application");
    }else{
      $this->send(array($av),
		  $av->getEmail(),
		  "Request for a recommendation letter",
		  "Dear Professor ".$av->fmtName('fl').",

we have received an application for the position of 
".$ap->pos->name()." here at ".myOrg_name."
".
"from ".$ap->av->fmtName('fl').",
and they have given your name as a reference for the position.

In connection with this application, we have created a secure web-page 
where you can upload your letter of recommendation:
<packPWD>

Here you can also check your personal data the applicant provided, 
and you can correct errors in this information by clicking on the 'modify my personal data' link.

If for some reason you do not wish to provide a reference for this candidate, 
you can also indicate this from the above web-page.

All correspondence is treated confidentially.",
		  $syslog_thisMail=True);
    }
  }
  
  /*
   * Confirm the upload of the reference letter
   */
  function acknowlegeRecLetter($file,$av,$ap) {

    // We do not need to preview this message
    $this->no_preview = True;
    $this->dbg();

    $this->ap = $ap;
    $this->av = $av;
    $this->informAdmin("recommendation");
    $this->send(array($av),
		$av->getEmail(), 
		"Recommendation letter receipt", 
		"Dear Professor ".$av->fmtName('fl').",

this e-mail is to acknowledge receipt of your letter of recommendation for ".$av->name()."

You uploaded this file as '$file' and can always access it through the web-page we created for you:
<packPWD>
",
		$syslog_thisMail=True);
  }
  
  /*
   * A mail to the applicant about successful upload
   */
  function uploadFile_acknowledge($ap,$field,$file){
    // do not confirm upload for the initial application 
    if (!$ap->just_submitted()){
      $this->dbg();
      $this->no_preview = True;
      $this->send(array($ap->av),
		  $ap->av->getEmail(), 
		  $ap->getDescr($field).' receipt', 
		  "Dear ".$ap->av->fmtName('fl').",

you recently applied for the position at ".myOrg_name.", and this e-mail is to acknowledge receipt of the supporting document.

You uploaded this file as '$file' and can always access it through the web-page we created for your application
<packPWD>

Good luck with your application.
",
		  $syslog_thisMail=True);
    }
  }
  
  /*
   * The following persons get the administrative mails:
   * - secretaries (might be many), $list = $pos->getEmail_secretary() 
   * - "position Email officer"     $mail = $pos->getEmail()
   * - position responsible        $list = $pos->getEmail_positionAdmin()
   */
  function informAdmin ($about) {

    $this->no_preview = True;
    $this->dbg();

    if (!is_object($this->ap))      b_debug::internalError('empty applicant');
    if (!is_object($this->ap->pos)) b_debug::internalError('empty position');

    $recipients = array_merge(array($this->ap->pos->getEmail()),
    				    $this->ap->pos->getEmail_secretary(),
				    $this->ap->pos->getEmail_positionAdmin());
    
    switch (mb_strToLower($about)) {
    case 'application':
      $this->send(join(',',array_unique(array_values($recipients))),
		  "New applicant in ".$this->ap->pos->name().$this->ap->pos->getDNR(),
		  "There is a new applicant in '".$this->ap->pos->name().$this->ap->pos->getDNR()."'

Family Name:   ".$this->ap->av->getValue('av_lastname',1)."
First Name:    ".$this->ap->av->getValue('av_firstname',1).
		  (($name = $this->ap->get_RFname()) ? "
Research field: $name" : "")."
Institute :    ".$this->ap->av->getValue('av_institute',1)." 
Year of birth: ".$this->ap->av->getValue('av_birthdate',1)."
",
		  $syslog_thisMail=False);
      break;
      
    case "recommendation":
      $recipients[$this->ap->av->name()] = $this->ap->av->getEmail();
      b_debug::print_r($recipients,'recipients');
      $this->send(join(',',array_unique(array_values($recipients))),
		  "New letter of recommendation in '".$this->ap->pos->name()."'",
		  "There is a new letter of recommendation in '".$this->ap->pos->name().$this->ap->pos->getDNR()."'

Applicant: ".$this->ap->av->fmtName('Lf,s')."
Referee:   ".$this->av->fmtName('Lf,s')."
Position:  ".$this->ap->pos->name().$this->ap->pos->getDNR()."
",
                  $syslog_thisMail=False);
      break;
    }
  }
  
  /*
   * The constructor
   */
  function __construct(){
    parent::__construct();
  }
}
