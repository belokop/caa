<?php
/*
 * - new applicant:
 *       $ap = new bForm_jam_Applicant('new'); 
 *        pos_id is expected in the array $_REQUEST
 * - existing applicant:
 *       $ap = new bForm_jam_Applicant($ap_id);
 */

define('Other', 'Other');
define('_supporting_documentation', 'Supporting Documentation');
/*
locateAndInclude('APIaccess_jam');
locateAndInclude('bForm_jam_Position');
locateAndInclude('bForm_Avatar_jam');
locateAndInclude('bList_jam_requiredDocs');
*/

b_os::needExecutable('pdftk',__FILE__,'../../src/pdftk.tar.gz','http://www.pdflabs.com/');

locateAndInclude('bForm_jam');
class bForm_jam_Applicant extends bForm_jam {

  var $debug = False;
  var $TABLEID = 'ap_id';
  var $pos = Null; // position class
  var $rf  = Null; // RF class  
  var $av  = Null; // avatar class
  var $extra_submits  = array();
  protected $closeForm_continue = 'continue';

  public static $ap2av = array('av_email'        => '_ap_email',
			       'av_firstname'    => '_ap_firstname',
			       'av_lastname'     => '_ap_lastname',
			       'av_birthdate'    => '_ap_year_of_birth',
			       'av_phone'        => '_ap_phone',
			       'av_institute'    => '_ap_institute',
			       'av_position'     => '_ap_position',
			       'av_residentship' => '_ap_residentship',
			       'av_citizenship'  => '_ap_citizenship',
			       'av_ms_year'      => '_ap_ms_year',
			       'av_ms_country'   => '_ap_ms_country',
			       'av_ms_institute' => '_ap_ms_institute',
			       'av_phd_year'     => '_ap_phd_year',
			       'av_phd_country'  => '_ap_phd_country',
			       'av_phd_institute'=> '_ap_phd_institute',
			       'av_pwd2'         => '_ap_pwd',
			       'av_timestamp'    => '_ap_timestamp', 
			       'av_id2'          =>  'ap_id',
			       );

  private $l_RF = Null;

  /**
   * The applicant might supply documents on his choice, if the position policy defines that.
   * This arrays keeps track of the documents needed.
   */
  private $docClasses = array();

  /*
   * Is Avatar an Applicant for this position?
   */
  public static function _isApplicant($av_id=Null){
    $reply = array();
    if (empty($av_id) && bAuth::authenticated()) $av_id = bAuth::$av->ID;
    if(!empty($av_id)) $reply = array_keys(self::get_apInfo($av_id));
    return $reply;
  }

  /*
   * Get application for the avatar
   */
  public static function get_apInfo($av_id,$pos_id=Null,$onlyActive=False,$return_AP_ID=False){
    $where = array("ap_avid = '$av_id'");
    if ( $pos_id)          $where[] = "ap_posid = '$pos_id'";
    if ( $onlyActive)      $where[] = "pos_status != ".JAM::status(JAM_pos_archived);
    if (!cnf_show_revoked) $where[] = "ap_status != ". JAM::status(JAM_ap_revoked);
    
    $q = myPear_db()->query("SELECT ap_id,ap_status,ap_posid FROM jam_applicants ".
			    " LEFT JOIN jam_positions ON pos_id = ap_posid ".
			    " WHERE ".join(' AND ',$where)." GROUP BY ap_id");
    
    $reply = array();
    while ($r = myPear_db()->next_record($q)){
      if (empty($r['ap_id']))      continue;
      if (empty($r['ap_posid']))   continue;
      if ($return_AP_ID) $reply[$r['ap_id']]    = $r['ap_status'];
      else               $reply[$r['ap_posid']] = $r['ap_status'];
    }
    return $reply;
  }


  /*
   * Look for applications which are look strange
   */
  public static function get_bogusApplications($pos_id=Null){
    $reply = array();
    if (myPear_db()->tableExists('jam_applicants')){
      // The application exists, but the flag "successfully submitted" is missing
      $where = array('ap_submitdate IS NULL');
      if ($pos_id) $where[] = "ap_posid = '$pos_id'";
      $q = myPear_db()->query("SELECT ap_id FROM jam_applicants WHERE ".join(' AND ',$where));
      while ($r = myPear_db()->next_record($q)){
	$reply[] = $r['ap_id'];
      }
    }
    return $reply;
  }
  
  /*
   * Keeps the address of a recently instantiated Applicant class
   */  
  public static $current = Null;

  /*
   *
   */
  function __construct($id, $mode='RO') {

    // Keep the last instantiated class in a cache
    self::$current = $this;

    parent::__construct($id, $mode);

    // Hook the position the applicant is applying for
    $this->hookPosition();
    
    // Get the associated avatar class and create the database record
    // if all the vital info is already available.
    // Strictly speaking it is too early to hook the avatar here, 
    // since $this->t = instance(b_table)" is not yet created at this moment,
    // hence the embedded avatar form will not inherit it.
    // The workaround is to call "hookAvatar" again when the b_table is ready, see function getForm.
    $this->hookAvatar();

    // Set the expected file types for the supporting documents
    if (!$this->myPost_bForm())    $this->apDocumentsBlock('check');

    if (!$this->myPost_bForm())  $this->setStatus();

    // Now we are ready...
    $this->isReady(True);

    // Set 'bad form', 'bogus app', etc. flags
    $this->ap_status_auto();

    $this->step();
  }


  /*
   * Define the description and types of the variable 
   */
  function defineVariables() {

    // Invoke the dynamic description of the position form
    // if ((int)$this->pos->ID)    $this->formDescription = new bList_formDescription($this->pos->ID);

    $this->defineVariable(array('_ap_email'          => 'E-mail',
				'_ap_firstname'      => 'First Name',
				'_ap_lastname'       => 'Last Name',
				'_ap_year_of_birth'  => 'Year of birth',
				'_ap_citizenship'    => 'Citizenship',
				'_ap_ms_country'     => 'Master degree country',
				'_ap_ms_institute'   => 'Master degree awarded by -SKIP-  (Institution)',
				'_ap_ms_year'        => 'Master degree year',
				'_ap_phd_country'    => 'PhD degree country',
				'_ap_phd_institute'  => 'PhD degree awarded by -SKIP-  (Institution)',
				'_ap_phd_year'       => 'PhD degree year',
				'_ap_institute'      => 'Present Institution',
				'_ap_position'       => 'Present Position',
				'_ap_phone'          => 'Phone',
				'_ap_place_of_birth' => 'Place of birth',
				'_ap_residentship'   => 'Country',
				'ap_posid'           => 'Position',
				'ap_rfid'            => 'Research field',
				'ap_rating'          => 'Rating',
				'ap_status'          => 'Application status',
				'ap_ip'              => 'Application IP-address',
				'ap_advisor_avid'    => 'PhD advisor',
				'ap_comment'         => '&nbsp;',
				'ap_avid'            => 'ap_avid',
				'ap_dossier_url'     => 'ap_dossier_url',
				'ap_submitdate'      => 'ap_submitdate',
				'DOSSIER'            => 'DOSSIER',
				'submitdate'         => 'submitdate',
				), True);

    if ((int)$this->ID) $type = 'isMBE';
    else                $type = 'isVITAL';


    $this->defineTypes(array($type    => array('_ap_email',
					       '_ap_firstname',
					       '_ap_lastname')));
    $this->defineTypes(array(  'isMBE'=> array_merge(array(), // array_keys(self::$ap2av),
						     array('ap_comment',
							   'ap_status',
							   'ap_advisor_avid',
							   '_ap_position',						
							   '_ap_phd_institute',
							   '_ap_phd_country',
							   '_ap_phd_year',
							   '_ap_place_of_birth',
							   '_ap_phone')),
			       //'isVITAL'     => array('ap_posid'),
			       'isRO'        => array('ap_ip',
						      'ap_submitdate',
						      'ap_dossier_url',
						      'ap_rating',
						      'ap_status' => !$this->isManager(),
						      ),
			       'expected_ext'=> array('DOSSIER'=>JAM_docs_ext),
			       ));
    // Define the RO & MBE depending the position settings 
    // (seem to be wrong...)
    if (!(int)$this->ID && is_object($this->pos)){
      $this->defineTypes(array('isMBE' => array('_ap_ms_year'      => !$this->pos->require_MS(),
						'_ap_ms_country'   => !$this->pos->require_MS(),
						'_ap_ms_institute' => !$this->pos->require_MS(),
						'_ap_phd_year'     => !$this->pos->require_PhD(),
						'_ap_phd_country'  => !$this->pos->require_PhD(),
						'_ap_phd_institute'=> !$this->pos->require_PhD())));
    }
  }


  /*
   * See on which state is the application
   * - step 0: we are NOT in the "submit application" context
   * - step 1: no AP nor AV records, filling form with the general information
   * - step 2: AV ready & ap->ID ready, filling list of referees
   * - step 3: application is successfully submitted, fixing eventual typos
   */
  public function step(){
    $reply = self::_step(array('ap_id'        => $this->ID,
			       'ap_submitdate'=> $this->getValue('ap_submitdate',True),
			       ));
    $this->dbg($reply,True);
    return $reply;
  }  

  /*
   * Step helper, might be called statically
   */
  public static $step = Null;
  public static function _step($buffer=array()){
    if (self::$step === Null){
      if (empty($buffer['ap_id']))            b_cnf::set('application_step',1);
      elseif(!empty($buffer['ap_submitdate']))b_cnf::set('application_step',3);
      elseif ((int)$buffer['ap_id'])          b_cnf::set('application_step',2);
      self::$step = (int)b_cnf::get('application_step', 1);
    }
    return self::$step;
  }
  
  /*
   * Help message printed while filling the form
   */
  protected function info_message(){
    $b = 'strong';
    $b = 'zzz';
    $m = ' style="margin: 6px;"';

    switch($this->step()){
    case 1:
      if ($this->isOwner()){
	myPear::INFO(array("Please complete the form below and click on the <em>\"continue\"</em> button at the end of the form.",
			   "DO NOT submit the application form more than once, you will get a chance to correct eventual errors.",
			   "The fields marked with <span class='redText'>'*'</span> are required.",
			   "Shortly after submitting the form you will receive an automated confirmation e-mail with the access code to your application."),
		     'Step 1');
	break;
      }
      
    case 2:
      if ($this->isOwner()){
	$text = array();
	// Referees
	list($n_need,$n_ask,$n_sent) = $this->countRecLetters();
	if ($n_ask < $n_need){
	  $text[] = "<li$m><$b>".
	    "Please provide the information requested below for at least $n_need referees.<br/> ". 
	    "Your referees will be notified directly of how to submit their reference letters. <br/>".
	    "You will receive an email confirmation when each of your referees has submitted their letter of reference.<br/>".
	    //"If you do not receive a confirmation of the receipt of the reference letter please contact your referee directly to remind them of this requirement.".
	    "If you do not receive a confirmation of the receipt of the reference letter JAM will lets you remind of this requirement.".
	    "</$b></li>";
	}
	
	// 
	$docs = array_values(JAM_requiredDocs($this->pos->ID)->get_requiredDocs());
	if (!empty($docs)){
	  $docs   = x('ul stype=margin:0;',b_fmt::joinMap('li',$docs));
	  $text[] = x("li$m",
		      x($b,
			"The following set of documents must be sent in support of your application: $docs ".
			"You may upload the appropriate PDF-files by browsing your file directory and attaching them to this application."));
	}
	
	// Show the message
	if (!empty($text)){
	  // $text[] = x("li$m",x($b,"After completing the form click on the <i>\"continue\"</i> button at the end of the form."));
	  myPear::MESSAGE(bText::_("<h1>Step 2:</h1> <ul>".join("\n",$text)."</ul>"));
	}
      }elseif((int)$this->ID){
	myPear::ERROR("<b><br/>".
		      "This application form was NOT completed by the applicant, and neither the confirmation e-mail nor the access code  was communicated.<br/>".
		      "This might be a bogus application. </br/>".
		      "Please request the applicant to complete the form if you think that this application is not a fake one.".
		      "</b><br/>&nbsp;");
      }
      break;
      
    default:
    }
  }
  
  /* 
   * Clean the myPear cache
   */
  function __clean_cache(){
    self::$current = Null;
    foreach($this->my_recommendations() as $rec)      $rec->__clean_cache();
    if (is_object($this->av))  $this->av->__clean_cache();
    myPear::getInstance(get_class($this), $this->ID, 'clean');
  }

  /*
   * The applicant upload area is under the position upload area,
   */
  function uploadInit_path($module_path=array(),$module=JAM_MODULE){
    $module_path = array(sprintf('%06d',$this->pos->ID), 
			 sprintf('%06d',$this->ID));
    return parent::uploadInit_path($module_path,$module,$this->pos->getValue('pos_orgid',True));
  }

  /*
   *
   */  
  private $ap_status_auto = Null;
  private function ap_status_auto(){
    if ($this->ap_status_auto === Null && $this->hookPosition() && $this->hookAvatar()){
      $ap_status_auto = array();
      foreach(JAM::$statuses as $id=>$a){
	switch($id){
	case JAM_ap_bogus:
	  $status = in_array($this->ID,JAM::$bogusApplications);
	  break;
	  
	case JAM_ap_bad_form:
	  $status = !$this->errorless();
	  break;
	  
	case JAM_ap_old_phd:
	  $status = !$this->is_PhD_fresh();
	  break;
	  
	default:
	  $status = False;
	}
	if ($status) $ap_status_auto[] = $id;
      }
      $this->dbg(join(', ',$ap_status_auto),cnf_dev);
      $this->ap_status_auto = serialize($ap_status_auto);
    }
  }

  function getValue($name,$isIs=True,$dMBE=True){
    switch($name){
    case 'ap_status_auto':
      return $this->ap_status_auto;
      
    default:
      return parent::getValue($name,$isIs,$dMBE);
    }
  }
  

  /*
   * Helper for getValue
   */ 
  protected function getValue_validate_empty($name,&$value=Null){

    locateAndInclude('bForm_jam_RF');
    $msg = parent::getValue_validate_empty($name,$value);
    if (myPear::is_empty($value)){
      switch($name){
      case 'ap_rfid':
	locateAndInclude('bForm_jam_RF');
	if (is_null($this->l_RF)) $this->l_RF = bForm_jam_RF::all_RFs($this->pos->ID);
	if   (empty($this->l_RF)) $msg = '';
	break;

      default:
	// Cancel the "hints"
	if (strpos('<',$name) !== False) $msg = '';
      }
    }
    if ($name == 'rec_letter_file')  b_debug::traceBack();
    return $msg;
  }

  /*
   * Send a mail confirming the upload
   */
  function uploadFile_acknowledge($name) {
    if (empty($_FILES[$name]['name'])){
      if (cnf_dev) b_debug::xxx("??? empty \$_FILES\[$name\]\[name\]");
    }else{
      JAM_mailer()->uploadFile_acknowledge($this,$name, $_FILES[$name]['name']);
    }
  }

  /*
   *
   */  
  function uploadedFile_name($name,$args=array(),$ext=True){
    $this->expected_ext[$name] = JAM_docs_ext;
    if (strpos($name,'DOSSIER')===False){
      $fn = parent::uploadedFile_name($name,$args,$ext);
    }elseif($this->hookAvatar()){
      $args = array($name,
		    strToUpper(b_crypt::urlize($this->av->getValue('av_lastname'))),
		    strToLower(b_crypt::urlize($this->av->getValue('av_firstname'))));
      $fn = join('_',$args) . ($ext ? $this->expected_ext[$name] : '');	
    }else{
      $fn = $name;
    }
    $this->dbg($fn);
    return str_replace(JAM_docs_ext.JAM_docs_ext,JAM_docs_ext,$fn);
  }
  

  function getInfo() {    
    if ($s=$this->isStatus(JAM_ap_revoked)) $s = bIcons()->get($s);
    myPear::H1($s . $this->av->name(),array('reset','noTranslate'));
    $this->getFormOpen();
    $this->apPublicInfoBlock();
    $this->blockTimeStamp();
    $this->closeForm();
  }

  /*
   * Return 'OK' icon
   */
  private function ok_icon($icon='ok',$d='OK') {
    return array('i'=>$icon,
		 'd'=>$d,
		 'm'=>'forced',
		 'f'=>'RO',
		 'X'=>'yes');
  }

  protected function blockTimeStamp(){
    if ($sd=$this->application_submit_date()){  
      locateAndInclude('bText');
      $this->formSignature(bText::_('Application submitted').' '.b_time::medium_date($sd));
    }
  }
  
  /*
   * Some of the status values are auto-calculated, hence they are removed from the popup menu,
   * but stay there for the RO mode
   */
  protected function apStatusBlock(){
    if (!self::$inputToPDF && $this->pos->hasSCrank()){
      $this->statusBlock('ap_status');
    }
  }

  /*
   * Check the supplied documents & referees information
   */  
  public function check_formDB($setStatus=True){

    // Add referees information
    $this->countRecLetters();

    // Add documents information
    $this->apDocumentsBlock('check');

    // Check what parents think
    parent::check_formDB($setStatus);
    
    if ($this->debug) b_debug::print_r($this->errorsInTheForm,
				       __METHOD__."($this->ID) errorsInTheForm - ".$this->name());
  }

  /*
   * This widget is a getForm without referees information
   */
  protected function getFormWidget(){
    $this->getFormOpen();
    $this->apStatusBlock();
    $this->apPublicInfoBlock();
    $this->apDocumentsBlock();
    $this->blockTimeStamp();
    $this->closeForm('');
  }

  /*
   * The main dialog, consists of 2 steps:
   * - step 1: fill form with the general information
   * - step 2: fill list of referees
   * - step 3: application is successfully submitted, fixing eventual typos
   */
  private $inside_getForm = 0;
  function getForm() {

    // Sanity & Superstition
    $this->dbg();
    if ($this->inside_getForm++){
      myPear::WARNING_T("?? recursion ".$this->inside_getForm);
      return;
    }

    if (bAuth::authenticated()){
      if (!$this->av) $this->av = bAuth::$av;
      $this->hookAvatar();
    }elseif(False && locateAndInclude('b_recaptchalib')){
      // Ask for the "captcha" to protect against spammers
      $captcha = b_reCAPTCHA();
      if ($captcha !== True){
	myPear::H1('Please type the text','reset');
	print $captcha;
	return;
      }
    }

    // Let the owner to complete the form
    if (!$this->errorless() && 
	$this->isOwner() && 
	$this->isConsidered(True)){
      $this->dbg("Force MODE=RW",True);
      $this->MODE = 'RW';
    }

    /*
     * Start the interactive dialog
     *
     * First show an informational message
     */
    if (!(int)$this->ID || !is_object($this->av)){
      // The application is not yet filled
      $this->info_message();
    }else{
      // The application record exists, print the page header
      if (self::$inputToPDF){
	print x('h1',$this->av->name());
      }elseif (cnf_inside_fb){
	myPear::H1($this->av->name());
      }elseif($this->isOwner()){
	myPear::H1($this->pos->name());
	print x('h3',$this->av->name());
      }else{
	myPear::H1($this->av->name());
	$message[] = x('h3',$this->pos->name());
	$message[] = x('h4',$this->get_RFname());
	// Build "next/previous candidate" buttons for the SC members
	if ($this->pos->hasSCrank() && $this->MODE === 'RO') {
	  $buttons = array();
	  $d = $this->uploadedFile_url('DOSSIER',$onlyIfExists=True);
	  if ($l=$this->getPrev())      $buttons[] = b_btn::link('previous applicant',$this->sameURL_but(array('id'=>$l)));
	  if ($d)                       $buttons[] = b_btn::link('see the dossier',$d);
	  if ($r=$this->getNext())      $buttons[] = b_btn::link('next applicant',    $this->sameURL_but(array('id'=>$r)));
	  //	  if ($buttons) myPear::MESSAGE(x("div class='centralized'",join('&nbsp;&nbsp;',$buttons)));
	  if (!empty($buttons)) $message[] = x("div class='centralized'",join('&nbsp;&nbsp;',$buttons));
	}
	if (!empty($message))  myPear::MESSAGE(join("\n",$message));
      }
    }
    
    
    /*
     * Start the dialog
     */
    $this->getFormOpen();
    
    switch(@$_GET['apBlock_once']){
      
    case 'add_referee':
      $this->apRefereesBlock(True);
      break;
      
    case 'doc':
    case $this->blockID_hash(_supporting_documentation):
      $this->apDocumentsBlock();
      break;
      
    default:
      locateAndInclude('bForm_jam_RF');
      if (is_null($this->l_RF)) $this->l_RF = bForm_jam_RF::all_RFs($this->pos->ID);

      if (!$this->getValue_validate_empty('ap_rfid')){
	$this->MBE['ap_rfid'] = True;
      }else{
	$this->isRO['ap_rfid'] = $this->pos->isStatus(JAM_pos_deadline);
	if ($this->MODE == 'RW') $this->formBlock('','Research field',
						  array('ap_rfid'=>array('selectMenu',$this->l_RF)),
						  'auto');
      }
      
      $this->apStatusBlock();
      if (!$this->isOwner() && $this->hookAvatar())    $this->av->getForm();
      $this->apPublicInfoBlock();
      $this->apRefereesBlock();
      $this->apDocumentsBlock();
      $this->apCommentsBlock();
      $this->blockTimeStamp();
    }
    
    $this->textHidden('ap_posid',$this->pos->ID);
    $this->closeForm('continue',$this->extra_submits);
    bIcons()->explain();

    $this->inside_getForm = 0;
  }

  /* 
   * Replace the detailed error messages by a more readable short statement 
   */
  protected $displayErrors_heading = 'The Application Form is not complete';
  protected function displayErrors_preflight(){

    if (!$this->isConsidered() || (($this->step() < 3) && $this->isOwner())){

      if($n = count($this->errorsInTheForm)) myPear::WARNING_T("Ignore ALL $n errors on step ".$this->step());
      $errors = array();
      
    }else{
      
      // Replace the detailed referee errors by just one line
      list($n_need,$n_ask,$n_sent) = $this->countRecLetters();
      
      // Check what the parent thinks 
      $errors = parent::displayErrors_preflight();

      // Inform the applicant about missing referees and/or letters
      if ($this->isOwner()){
	if ((($nw=($n_ask-$n_sent))>0) && ($d = $this->application_submit_date()) && ((time() - $d)/84600 > 3)){
	  if ($n_sent < $n_need)  myPear::WARNING("Still waiting for $nw recommendation letter".($nw>1?'s':'')." to arrive"); 
	}
      }
    }
    return $errors;
  }


  /*
   *
   */
  private function apCommentsBlock(){
    if (is_object($this->av) && (($this->MODE == 'RW') || $this->getValue('ap_comment',True))){
      $this->formBlock('','Further information',
		       array('ap_comment'=>'textArea'),
		       'auto');
    }
  }
  
  /*
   * Small optimization
   */
  private $requiredDocs = Null;
  private function get_requiredDocs(){
    locateAndInclude('jam_Document');
    if (!(int)$this->ID || !bAuth::authenticated()) return array();

    if ($this->requiredDocs === Null){
      $this->requiredDocs = array();
      $docs = JAM_requiredDocs($this->pos->ID);
      foreach($docs->get_requiredDocs() as $lm_id=>$doc_name){
	$this->requiredDocs[$doc_name] = new jam_Document($this,$lm_id);
	// Force the file uploading (sanity, needed for recovery after failures)
	$this->uploadedFile_path($doc_name);
      }
    }
  }
  
  /*
   * Request the PDF document files
   * - $action == 'show' : show the upload dialog
   * - $action == 'check'  : verify the submitted documents
   */
  private $errorsInTheForm_docs = Null;
  private $apDocumentsBlock_dialog = Null;
  private function apDocumentsBlock($action='show'){
    static $icon_allDone = Null;
    
    if (!(int)$this->ID || !bAuth::authenticated()) return; 
    
    bTiming()->cpu(__FUNCTION__);
    if ($this->apDocumentsBlock_dialog === Null){
      $this->dbg(starting);
      $this->hookPosition();
      
      $icon_allDone = $this->ok_icon('ok','All documents submitted');
      $this->apDocumentsBlock_dialog = array();
      $this->errorsInTheForm_docs    = array();
      
      $this->get_requiredDocs();
      foreach(array_keys($this->requiredDocs) as $doc_name){
	if ($this->debug) $this->requiredDocs[$doc_name]->debug = $this->debug;
	$filename = $this->requiredDocs[$doc_name]->filename();
	if ($this->requiredDocs[$doc_name]->isUploaded()){
	  $this->formDB[$filename] = $filename . JAM_docs_ext;
	}else{
	  $icon_allDone = '';
	  if ($action === 'check') $this->errorsInTheForm_docs[$filename] = 'file is missing';
	}
	$this->defineVariable(array($filename=>$doc_name),True);
	$this->isMBE[$filename] = !JAM_requiredDocs($this->pos->ID)->is_documentCompulsory($filename);
	$this->expected_ext[$filename] = JAM_docs_ext;
	$this->apDocumentsBlock_dialog[$filename]= 'uploadFile';
      }

      // Store the documents errors
      foreach($this->errorsInTheForm_docs as $k=>$v){
	$this->errorsInTheForm[$k] = $v;
      }
    }
    
    // Show the dialog
    if (($action === 'show') && 
	  ($this->pos->hasSCrank() || $this->isOwner() || bAuth::$av->isReferee() || self::$inputToPDF)){
      $this->formBlock('',_supporting_documentation,
		       $this->apDocumentsBlock_dialog,
		       $icon_allDone);
    }
    bTiming()->cpu();
  }
  
    
  /*
   * This block might be seen by the owner, SCmembers and admins only
   */
  private function apRefereesBlock($askForMore=False){
    if (!(int)$this->ID || ($this->step() < 2))  return;
    if (!$this->isOwner() && !$this->pos->hasSCrank() && !self::$inputToPDF) return;
    
    bTiming()->cpu(__FUNCTION__);
    locateAndInclude('bForm_Avatar');

    $this->dbg();    
    $this->info_message();
    
    /*
     * Convert the referees information provided by the applicant
     * into bForm_jam_Recommendation and (if the referee is not known to the system) bForm_Avatar_jam classes
     */
    if ($this->myPost_bForm()){
      for($n=1; $n < 10; $n++){
	
	$av_data = array();
	foreach(array('av_firstname'=> "_virt_referee${n}_firstname",
		      'av_lastname' => "_virt_referee${n}_lastname",
		      'av_email'    => "_virt_referee${n}_email",
		      'av_institute'=> "_virt_referee${n}_institute") as $t=>$f){
	  $v = $this->getValue($f,True,True);
	  if (empty($v)) continue;
	  if ($t == 'av_email')  foreach(bForm_Avatar::tidy_email($v) as $k=>$v) $av_data[$k] = $v;
	  else  $av_data[$t] = $v;
	}
	if (empty($av_data)) continue;
	if ($this->debug) b_debug::print_r($av_data,"_virt_referee${n}");	
	
	// Catch multiple recommendations from the same referee
	if (empty($av_data['av_email2'])) {
	  myPear::ERROR("'_virt_referee${n}_email' is not a valid e-mail address");
	  continue;
	}
	
	// Check if the referee is known
	$av = Null;
	if ($avs = bForm_Avatar::hookByEmail($av_data['av_email2'])){
	  $av = array_pop($avs);
	  if (!empty($avs)) myPear::ERROR("Can't uniquely identify the referee with e-mail '".join(',',$av_data['av_email'])."', there are many...");
	}
	
	// Create the Avatar for referee if all the vital fields (email,name,etc.)  are there
	$OK = True;
	locateAndInclude('bForm_jam_Recommendation');
	foreach(bForm_Avatar::$vitalFields as $vf)  if (empty($av_data[$vf])) $OK = False;
	
	// Create the req. letter request
	if ($OK || is_object($av)){
	  if (!is_object($av)) $av = bForm_Avatar::hook($av_data);
	  if (!is_object($av)) b_debug::internalError("Can't create av for the referee ");

	  if ($recs = bForm_jam_Recommendation::getRecommendations($this->pos->ID,$av->ID,$this->ID)) {
	    // Catch multiple recommendations from the same referee
	    myPear::WARNING("You have already requested ".((($n=count($recs)) == 1) ? "a recommendation" : "$n recommendations").
			    " from ".$av->fmtName('sfl')."<br/>This request is ignored"); 
	  }else{
	    $av->debug = $this->debug;
	    $av->setPWD();
	    $av->updateDB($av_data);
	    
	    // $r = new bForm_jam_Recommendation('new','RW');
	    // $r->ap = $this;
	    // $r->av = $av;
	    $r = new bForm_jam_Recommendation($this,$av);
	    $r->set_requested();
	    JAM_mailer()->ask_for_recLetter($av,$this);
	    $this->my_recommendations($reset=True);
	  }
	}elseif(!empty($av_data)){
	  $this->errorsInTheForm['_referee'] = 'missing information';
	}
      }
      $this->dbg(exiting);
    }
    
    /*
     * Show information about the referees already nominated as the RO block
     */
    foreach ($this->my_recommendations() as $n=>$recClass){
      // if (!is_object($recClass->av)) var_dump($recClass);	
      $dialog = $rButton = array();
      $nRows = 1;
      foreach (array(' ', ' ','Comment') as $nk=>$descr){
	$field = "_virt_ref_$n$nk";
	$this->defineVariable(array($field => $descr),True); 
	$this->defineTypes(array('isRO' => array($field => True)));

	$dialog[$field] = array('textField');
	switch($nk){
	case 0: // Referee. 
	  $info = array(($recClass->isReadable() 
			 ? x("a href='".b_url::same("?function=jam_listRecommendations&id=".$recClass->av->ID)."'",$recClass->av->name())
			 : x('strong',$recClass->av->name()) //  bJS()->modal_iframe("?form=Avatar_jam&id=".$recClass->av->ID,$recClass->av->name())
			 ),
			$recClass->av->getEmail(),
			$recClass->av->getInst());
	  $this->formDB[$field0 = $field] = join('-BR-',$info);
	  break;
	case 1: // Got the letter?
	  if ($date = $recClass->isReceived()){
	    $this->formDB[$field] = (self::$inputToPDF ? '' : x('strong',"Referee replied ".b_time::medium_date($date)));
	    if (!myPear::is_empty($this->formDB[$field])) $nRows++;
	  }else{ 
	    $this->formDB[$field] = b_fmt::redText(x('strong',bText::_('No reply yet...')),'blueText');
	    $nRows++;
	  }
	  break;
	case 2: // Comment
	  if ($recClass->isReadable()){
	    $this->formDB[$field] = $recClass->getValue('rec_comment',1,1);
	    if (myPear::is_empty(@$this->formDB[$field])){
	      unset($dialog[$field]);
	    }else{
	      $nRows++;
	    }
	  }
	}
      }

      // Show the referee photo (if available) as the first row
      $photo  = $recClass->av->getPhoto('','',2); // - scaled 1/2 
      if (!empty($photo)) $dialog[$field0][] = array('width="'.bForm_Avatar::$w.
						     '" rowspan="'.$nRows.'" style="text-align:right;"'=>$photo);
      
      // Put an explanation icon in front of the block
      if    ( $recClass->isReceived())           $rButton=$this->ok_icon('i-mail-open','Letter arrived');
      elseif(!$recClass->is_reminderNeeded())$rButton=$this->ok_icon('i-mail','Waiting for the delivery...');
      else                                   $rButton=array('i' =>'i-mail-send',
							    'm' =>'forced',
							    'li'=>"?function=jam_mailRemindRecommendation&id=$recClass->ID",
							    'd' =>'Nicely remind about the recommendation letter',
							    'f' =>'RO');
      
      $this->formBlock('',$this->n($n+1).' referee', // " Letter Of Recommendation",
		       $dialog,
		       $rButton);
    }
    
    /*
     * Ask to provide the remaining referees
     */
      $nLettersRequired =  max($this->pos->getValue('pos_n_referees',1) , ($nLettersAsked=count($this->my_recommendations())));
      if ($askForMore) $nLettersRequired++;
      
      for ($ne=$nLettersAsked+1; $ne <= $nLettersRequired; $ne++){
	switch($this->MODE){
	case 'RW':
	  $dialog = array(); 
	  foreach (array("Referee E-mail"      =>"_virt_referee${ne}_email",
			 "Referee first name"  =>"_virt_referee${ne}_firstname",
			 "Referee last  name"  =>"_virt_referee${ne}_lastname",
			 "Referee institution" =>"_virt_referee${ne}_institute") as $f=>$p){
	    $dialog[]  = $p; 
	    $this->defineVariable(array($p=>$f),True);
	  }
	  break;
	  
	case 'RO':
	  $this->formDB["_virt_referee${ne}"] = b_fmt::redText('missing');
	  $this->defineVariable(array("_virt_referee${ne}" => 'Referee'),True);
	  $dialog = array("_virt_referee${ne}");
	}
	$this->formBlock('',$this->n($ne).' referee', // ' Letter Of Recommendation',
			 $dialog);
      }
      
      /*
       * Ask for an extra referee (if it makes sense)
       */
      if (!$this->pos->isStatus(JAM_pos_deadline) &&
	  $nLettersAsked >= $nLettersRequired &&
	  $nLettersAsked < 9 && 
	  $this->isConsidered() && 
	  $this->MODE == 'RO' && $this->isWritable()){
	$this->extra_submits['add an extra Referee'] = $this->sameURL_but(array('mode'=>'RW','apBlock_once'=>'add_referee'));
      }    
      bTiming()->cpu();
  }    

  private function n($n){
    switch ($n){
    case 1:  return $this->pos->formDB['pos_n_referees'] == 1 ? '' : 'First'; 
    case 2:  return 'Second';
    case 3:  return 'Third'; 
    default: return $n."-th";
    }
  }
  
  /*
   * Get the next/previous application
   */
  private function getNext() { return $this->getNextOrPrev('>'); }
  private function getPrev() { return $this->getNextOrPrev('<'); }
  private function getNextOrPrev($sign,$trigger='av_lastname') {

    $where = array('ap_posid = '. $this->getValue('ap_posid',1,1),
		   'ap_rfid  = '. $this->getValue('ap_rfid', 1,1),
		   myPear_db()->quote($this->av->getValue($trigger),$sign,$trigger),
		   );
    if (!JAM_access()->may_seeRevokedApplications()) $where[] = "ap_status != ".JAM::status(JAM_ap_revoked);
    $q = myPear_db()->qquery("SELECT ap_id FROM $this->TABLE ".
			       " LEFT JOIN zzz_avatars ON ap_avid = av_id ".
			       " WHERE ".join(' AND ',$where).
			       " ORDER BY $trigger ".($sign=='>'?'ASC':'DESC')." LIMIT 0,1");
    
    $reply = False;
    while ($r = myPear_db()->next_record($q)) $reply = $r['ap_id'];
    $this->dbg($sign=='>'?'getNext ':'getPrev '.$reply);
    return $reply;
  }
    
  
  /**
   * The applicant dossier is a PDF file containing all the relevant documents.
   *
   * Check the time-stamps of all the supporting documents & recommendation letters,
   * compare it with the time-stamp of the dossier.
   * If the dossier timestamp is older then the timestamps of the components, recreate it.
   */
  function createDossier($silent=False,$forced=False){

    // this is not an error
    if (self::$inputToPDF)  return;
    if ($this->isStatus(JAM_ap_revoked)) return;

    $dossier_url = $this->getValue('ap_dossier_url',True);

    if ((int)$this->ID && $this->MODE == 'RO' && $this->hookAvatar()){
      bTiming()->cpu(__FUNCTION__);
      $dossier = array();
      $savehide_explainIcons = bIcons()->hide_explainIcons;   
      bIcons()->hide_explainIcons = True;
      
      if (empty($dossier_url) || $forced)  $dossier_url= $this->uploadedFile_url('DOSSIER',$onlyIfExists=False);
      $dossier_file = b_os::realpath(dirname($dossier_url)) . DIRECTORY_SEPARATOR . basename($dossier_url);
      
      // check the dossier time stamp
      $dossierTS = (file_exists($dossier_file) ? filemtime($dossier_file) : 0);

      // List first the recommendation letters
      $toBeReplaced = array();
      if ($dossierTS <= 0) $toBeReplaced[] = 'NO dossier';
      if ($forced)         $toBeReplaced[] = 'Forced recreate';
      
      foreach ($this->my_recommendations() as $rec){
	$rec->debug = $this->debug;
	if ($ts = $rec->isReceived()){
	  $file = $rec->uploadedFile_path();	
	  if ($ts > $dossierTS) $toBeReplaced[] = 'New '.$rec->__toString();
	  $dossier[$rec->getDescr_file()] = $file;
	}
      }

      /*
       * Append the documents provided by the applicant
       */
      $this->get_requiredDocs();
      foreach(array_keys($this->requiredDocs) as $doc_name){
	$this->requiredDocs[$doc_name]->debug = $this->debug;
	if ($timestamp=$this->requiredDocs[$doc_name]->isUploaded()){
	  if ($timestamp > $dossierTS){
	    $toBeReplaced[] = 'New '.$this->requiredDocs[$doc_name]->__toString();
	  }
	  $dossier[$this->requiredDocs[$doc_name]->name()] = $this->requiredDocs[$doc_name]->uploadedFile_path();
	}
      }
      
      /*
       * Replace the dossier if needed, i.e. if its timestamp is not the latest one
       * between all the files
       */
      if (!empty($toBeReplaced)){
	$msg="Updating dossier of ".$this->av->fmtName('sLf');
	if (!$silent) myPear::MESSAGE($msg);
	if (cnf_dev) b_debug::print_r($toBeReplaced,$msg);

	// check the files for validity
	foreach($dossier as $k=>$file){
	  if (!$this->valid_fileType($file,'.pdf',(is_numeric($k) ? basename($file) : $k))){
	    myPear::Error($this->valid_fileType_error);
	    unset($dossier[$k]);
	  }
	}
	
	//  Extract the application form as an html string
	ob_start();
	self::nameCounter('resetCounter');
	self::$inputToPDF = True;  // open "writing to PDF" mode
	self::$t_formOpened = new b_table();
	$this->getForm();
	if (self::$t_formOpened instanceof b_table){
	  self::$t_formOpened->close();
	  self::$t_formOpened = $this->t = Null;
	}
	$formDB = ob_get_contents();
	ob_end_clean();

	// Create the cover page from the applicant database record.
	// Set the PDF header to the Position applied
	$this->get_RFname();
	$subHeader = array();
	if ($dnr = $this->pos->getValue('pos_refnumber')) $subHeader[] = "DNR: $dnr";
	$subHeader[] = $this->pos->name();
	if ($this->rf) $subHeader[] = $this->rf->name();

	b_pdf::html2pdf($coverPage_file = tempnam(sys_get_temp_dir(),"delete.me-$this->ID").'.pdf',
			$formDB,
			x('strong',join('<br/>',$subHeader)));

	array_unshift($dossier, $coverPage_file);
	$this->dbg(count($dossier).' input files merged to '.$dossier_file);
	self::$inputToPDF = False;  // close "writing to PDF" mode

	// Finally merge all the files into the dossier
	b_pdf::join(array_values($dossier),$dossier_file);
	if ($this->debug) b_debug::print_r($dossier,$dossier_file);

	// Update the dossier location
	$this->updateDB(array('ap_dossier_url'=>$dossier_url));
      }else{
	$this->dbg('dossier is up-to-date',(bool)($this->debug));
      }
      
      bIcons()->hide_explainIcons = $savehide_explainIcons;   
      bTiming()->cpu();
    }

    if (!file_exists($dossier_url)){
      $this->updateDB(array('ap_dossier_url' => ''));
      myPear::WARNING("Can't create dossier for ".$this->av->fmtName());
      $reply = Null;
    }else{
      $reply = $dossier_url;
    }
    return $reply;
  }

  /*
   *
   */
  function name() {
    if ($this->hookAvatar()) return $this->av->name(); 
    else                     return 'New application';
  }

  /*
   * The applicant is the only owner of the application form
   */
  function isOwner(){
    b_debug::xxx(bAuth::$av."  ID=".@$this->av->ID);
    if (bAuth::authenticated()){   $this->hookAvatar();     $this->dbg($this->av->ID,True);}
    return b_cnf::check_conditions(array('True'=>array('own'=>(bAuth::authenticated() && 
							       $this->hookAvatar() && 
							       (bAuth::$av->ID == $this->av->ID))),
					 'default' => parent::isOwner()),
				   $this);
  }
  
  function isManager() {
    if ($this->hookPosition())     return $this->pos->isManager();
  }
  
  function isReadable() { 
    $reply = JAM_access()->may_seeApplicantInfo() || $this->isOwner();
    $this->dbg($reply,True);
    return $reply;
  }

  /*
   * Save the application status in the database
   */
  public function setStatus($what=Null){
    //
    // Set the status if requested
    //
    if (!empty($what)){
      $name = x('em',$this->av->fmtName('sfL'));
      if ($old_status = $this->getValue('ap_status',1,1)){
	myPear::MESSAGE("$name - cancel status \"".JAM::status($old_status,'c2d','ap_status').'"');
      }
      myPear::MESSAGE("$name - set status \"".JAM::status($what,'d').'"');
      $this->getValue_reset('ap_status',JAM::status($what));
    }
  }


  /*
   * Should the applicant's PhD be fresh?
   * Impose a "reasonable sanity limit" to 10 years
   */
  private function is_PhD_fresh(){
    switch($this->pos->getValue('pos_req_phd')){
    case JAM_yes_but:
      $av_phd_year = $this->av->getValue('av_phd_year',1);
      switch(b_time::is_valid($av_phd_year)){
      case -2:	//   -2 - empty input value
      case -1:	//   -1 - "not considered"
	$reply = True;
	break;
      case 0:	//    0 - not a valid time (probably just a year number)
	$reply = b_time::txt2unix($av_phd_year.'-07-01');
	break;
      case 1:	//    1 - integer unix time
	$reply = $av_phd_year;
	break;
      case 2:	//    2 - string "YYYY-MM-DD" or "YYYY-MM-DD HH:MM"
      case 3:	//    3 - SSN (personnummer)
	$reply = b_time::txt2unix($av_phd_year);
	break;
      }
      if (!is_bool($reply)){
	if ($this->debug) b_debug::xxx(b_time::period($this->pos->getValue('pos_deadline',1),$reply));
	$reply = (($this->pos->getValue('pos_deadline',1) - $reply) < $this->pos->getValue('pos_req_phd_year',1)*366*24*3600);
      }
      break;

    default:
      $reply = True;
    }
    $this->dbg($reply);
    return $reply;
  }


  /*
   * Get status as a set of icons
   */  
  function allStatusIcons(){
    $this->setStatus();
    $reply = join("",self::_allStatusIcons($this->formDB,'ap_status'));
    return $reply;
  }

  /*
   * Check status from "ap_status" & "ap_status_auto" fields
   */
  public function isStatus($id){
    return self::_isStatus($id,array('ap_status'      => $this->getValue('ap_status'),
				     'ap_status_auto' => $this->getValue('ap_status_auto')));
  }

  /*
   * Get status from "ap_status" & "ap_status_auto" fields
   */
  public static function _isStatus($id,$formDB){
    return self::__isStatus($id,$formDB,'ap_status');
  }

  /**
   * The application is NOT considered if either:
   * - the application is revoked
   * - the position is archived
   * 
   * Note, the superuser has an unlimited access, 
   * hence for him the application is ALWAYS considered
   */
  private $isConsidered = Null;
  function isConsidered($exclude_superUser=False){
    if ($this->isConsidered === Null){
      $this->isConsidered = (bool)b_cnf::check_conditions(array(True => array('SU'  => (superUser_here && !$exclude_superUser)),
								False=> array('arch'=>$this->pos->isStatus(JAM_pos_archived),
									      'rvkd'=>$this->isStatus(JAM_ap_revoked)),
								'default' => True),
							  $this,
							  $this->debug||cnf_show_a);
    }
    return $this->isConsidered;
  }
  
  /*
   * The application is not writable after the deadline (see the comment inside)
   */
  function isWritable() { 
    if (!($reply = in_array($this->ID,array('new','empty')))){
      if ($reply = bAuth::authenticated()){
	if (($reply = parent::isWritable()) && !superUser_here){
	  if ($reply = $this->isConsidered()){
	    // Let applicants fix the errors in the form after the deadline.
	    // Careful, the form is filled without authentication!
	    if (bAuth::authenticated() && ($this->av->ID == bAuth::$av->ID)){  
	      $reply = !($this->errorless() && $this->pos->isStatus(JAM_pos_deadline)); 
	    }
	  }
	}
      }
    }
    $this->dbg($reply, (bool)$this->debug||cnf_show_a);
    return $reply;
  }
  
  function readDefRecord(){
    $this->formDB = array();
    $q = myPear_db()->query("SELECT * FROM jam_applicants WHERE ap_id = '$this->ID'");
    while ($r = myPear_db()->next_record($q)) $this->formDB = $r;
  }

  /*
   * Get the research field name for the Applicant.
   * @return current Research Field name or an empty string if the position has only one RF
   */
  function get_RFname() {
    $reply = '';
    if (count($this->pos->getRF_id()) > 1){
      $this->getRF();
      if (is_object($this->rf)) $reply = $this->rf->name();
    }
    $this->dbg($reply);
    return $reply;
  }

  /*
   *
   */
  private function getRF() {
    if (!($this->rf instanceof bForm_jam_RF)){
      if ($rf_id = $this->getValue('ap_rfid',True)) $this->rf = new bForm_jam_RF($rf_id,'RO',$this->pos);
    }
  }

  /*
   *
   */ 
  protected function apPublicInfoBlock(){
    if ($this->hookAvatar()){
      if (!$this->isOwner())      $this->av->getForm();
    }else{
      if ($this->MODE == 'RO') b_debug::internalError("Avatar in the RO mode must exist, but it does not",$this);
      /*
       * Contact info - Name, email, etc. 
       */
      $this->formBlock('','General Information',
		       array('_ap_firstname',
			     '_ap_lastname',
			     '_ap_email',
			     //'_ap_phone',
			     '_ap_institute',
			     '_ap_position',
			     '_ap_residentship'=>array('textFieldCountry')));
      /*
       * Personal Details - Nationality, birthday
       */
      $dialog = $this->year2date('_ap_year_of_birth','dialog2');
      // $dialog['_ap_place_of_birth'] = 'textField';
      if ($this->pos->askCitizenship()) $dialog['_ap_citizenship'] = array('textFieldCountry');
      $this->formBlock('','Personal Details',$dialog);
      
      /*
       * Education
       */
      $year_ms  = $this->year2date('_ap_ms_year', 'dialog2');
      $year_phd = $this->year2date('_ap_phd_year','dialog2');
      $dialog = array();
      if ($this->pos->require_PhD()){
	$dialog = array_merge($dialog,array('_ap_phd_institute'=>array('textField'),
					    '_ap_phd_country'  =>array('textFieldCountry'),
					    '_ap_phd_year'     =>$year_phd['_ap_phd_year'],
					    '_spacer'          =>array('textField'),
					    ));
      }
      if ($this->pos->require_MS() || $this->pos->require_PhD()){
	$dialog = array_merge($dialog,array('_ap_ms_institute' =>array('textField'),
					    '_ap_ms_country'   =>array('textFieldCountry'),
					    '_ap_ms_year'      =>$year_ms['_ap_ms_year'],
					    )); 
      }
      $this->formBlock('',"Education",$dialog);
    }
  }
  
  /*
   * Check, can we create a new record in the database
   * Protect against duplicated applications
   */
  function updateDB_preflight(&$updates=array()) {    
    if ($reply = (parent::updateDB_preflight($updates) && is_object($this->av))){
      switch($n=count($ids=$this->av->isApplicant($this->pos->ID,$onlyActive=False))) {
	
	/*
	 * ok, if the application processing is not yet started
	 */
      case 0:    
	$reply = True;  
	break;
	
	/*
	 * There must be one application per position
	 * if there is just 1 processed applications, that is presumably the user who tries again
	 */
      case 1:    
	$reply = isset($ids[$this->ID]);
	if (!$reply){
	  myPear::WARNING_T($this->av->name()." already applied for ".$this->pos->nickname());
	  if (!isset($ids[$this->ID]))	$ids[$this->ID] = 'current';
	}
	break;
	
	/*
	 * If there is more then 1 processed applications, that is presumably my fault
	 */
      default:
	myPear::WARNING_T($this->av->name()." $n tries for ".$this->pos->nickname());
	if (!isset($ids[$this->ID]))	$ids[$this->ID] = 'current';
	$reply = False;
      }
    }else{
      $reply = True;
    }
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * get the position the applicant is looking for
   */
  private function hookPosition(){
    if (!is_object($this->pos)){
      if (!($ap_posid = $this->getValue('ap_posid',1)))     $ap_posid = JAM_posID();
      $this->pos = myPear::getInstance('bForm_jam_Position',$ap_posid, 'strict');
      if (!($this->pos instanceof bForm_jam_Position)) b_debug::internalError("'$this->pos' is not 'bForm_jam_Position($ap_posid)' class",$this);
      $this->dbg($this->pos->__toString());
      $this->isLocked($this->pos->isStatus(JAM_pos_archived));
    }
    return (is_object($this->pos));
  }

  /*
   * create (if not yet known) the avatar class for the applicant 
   */
  private function hookAvatar(){
    
    $this->hookPosition();
    if ($this->av instanceof bForm_Avatar_jam) {
      $this->updateDB(array('ap_avid'=>$this->av->ID));
      $this->av->isEmbedded($this);
      return True;
    }else{

      $av_id   =$this->getValue('ap_avid', 1,1);
      $av_email=$this->getValue('_ap_email',1,1);
      if ($this->av = bForm_Avatar::hook(array('av_id'   =>$av_id,
					       'av_email'=>$av_email),$doNotCreate=True))      $this->av->defineVariables();
      
      // If there are multiple myPear modules installed on the host, 
      // there might be switch between the modules, hence the Avatar might be random.  
      // Hence better be sure that we got the proper Avatar class
      bForm_Avatar::set_context($this->av,JAM_MODULE);

      if (count($_POST)>1 && !($this->av instanceof bForm_Avatar) && !$this->vitalFieldMissing()){
	$this->av = new bForm_Avatar_jam('empty');
	$this->av->defineVariables();
	$this->av->debug = $this->debug;
	
	// The form ID was 'new' when the form was filled, make it the current one
	$_POST['_post_bForm_ID_once'] = $this->ID;
	
	$updates = array();
	foreach(self::$ap2av as $t=>$f){
	  if (!myPear::is_empty($v=$this->getValue($f,True))) $updates[$t] = trim($v);
	}
	// Sick! Don't remove "if(...)" !
	if (!empty($updates)) $this->av->updateDB($updates);
      }
    }
    
    // Link the Applicant record to Avatar & Position if not yet done
    // Hook the Avatar as an embedded form
    if ($this->av instanceof bForm_Avatar){
      $this->dbg($this->av->__toString());
      $this->av->isEmbedded($this);
      
      // Copy the Applicant vital fields from Avatar 
      foreach(array_keys($this->isVITAL) as $f){
	$av_f = str_replace('ap_','av_',str_replace('_ap_','av_',$f));
	if (myPear_db()->columnExists($av_f,'zzz_avatars')){
	  $_POST[$f] = $this->formDB[$f] = $this->av->getValue($av_f,True);
	}
      }

      // Save the avatar_id & position_id in the database
      // sanity first...
      if (!(int)($this->ID) && ($n=count(self::get_apInfo($this->av->ID,$this->pos->ID,$onlyActive=True)))){
	b_debug::traceBack("already applied $n time(s)");
      }else{
	$this->updateDB(array('ap_avid' =>$this->av->ID, 
			      'ap_posid'=>$this->pos->ID)); 
      }

      // Refine the avatar MS / PhD requirements
      $this->av->defineTypes(array('isMBE'=>array('av_phd_year'     => !$this->pos->require_PhD(),
						  'av_phd_country'  => !$this->pos->require_PhD(),
						  'av_phd_institute'=> !$this->pos->require_PhD(),
						  'av_ms_year'      => !$this->pos->require_MS(),
						  'av_ms_country'   => !$this->pos->require_MS(),
						  'av_ms_institute' => !$this->pos->require_MS(),
						  )));
      // dangerous trick...
      myPear::replaceInstance($this);

      return True;
      
    }elseif ((int)$this->ID){
      //      b_debug::internalError("can't hookAvatar to ".$this);
    }
    return False;
  }

  private  function already_got_the_application(){
    return True;
    if ($n=$this->messInTheDatabase()){
      myPear::WARNING(x('div class="redText centralized"',
			join('<br/>',
			     array(bText::_('Dear').' '.$this->av->name().',',
				   '',
				   bText::_("We have already got an application from you"),
				   '('.bText::_('i.e. from the e-mail address')." '".$this->av->getEmail()."')",
				   bText::_('for').' '.$this->pos->name().'.',
				   '',
				   bText::_('Please check your mailbox for the Email which we sent to you')." $date,",
				   bText::_('this Email contains all the information to access your application via the Internet.'),
				   '',
				   "<div class='align_right'>".
				   "<font size=-1>".bText::_(array('key'=>'_Sincerely','item'=>'Sincerely')).
				   ",<br/><i>".$this->pos->name()."<br/>".bText::_('search committee')."</i></font>"))));
    }
    return !$this->messInTheDatabase();
  }
  
  /*
   * Rating is a number from 1 (the lowest) to 5 (the highest)
   *
   * @param return (float) rating
   */  
  function getRating() {
    return $this->getValue('ap_rating') / 10.;
  }


  /*
   * Save the rating in the database.
   * Note, the rating is kept in the column "ap_rating" as integer = $rate*10
   * to be able to easily sort applicants by the rating
   */
  function setRating(){
    locateAndInclude('bForm_jam_Review');

    // Get the average rating from all the reviews
    $n = $rate = 0;
    foreach(bForm_jam_Review::getReviews($this->pos->ID,Null,$this->ID) as $r){
      // skip the abstained reviews
      if (empty($r['r_rating'])) continue;
      $rate += (int)$r['r_rating']; 
      $n++;
    }

    // Save the rating in the database
    if ($n > 0)	$this->updateDB(array('ap_rating'=>($rate=(int)($rate * 10 / $n))));
    $this->dbg($rate/10);
  }
  
  /*
   *
   */
  private function application_submit_date(){
    return $this->getValue('ap_submitdate',True,True);
  }

  /*
   * Call-back function
   */
  function deadline_reached(){
    return $this->pos->deadline_reached();
  }

  /*
   * Limit the submit date by the pos_deadline 
   */
  function setSubmitDate(){
    $date = $this->application_submit_date();
    if (empty($date)) $date = time();
    $reply = min($date, b_time::txt2unix($this->pos->getValue('pos_deadline',True)));
    $this->dbg(b_time::sql_timestamp($reply));
    return $reply;
  }

  /*
   * Give 1 hour to submit the application,
   * don't bother the applicant during this period with error messages
   */
  function just_submitted(){
    if ($reply = $this->application_submit_date()){
      $reply = ($reply + 3600 > time());
    }
    $this->dbg((bool)$reply);
    return (bool)$reply;
  }

  /*
   *
   */
  function formOKspecific(){
    $this->dbg();
    if (!$this->hookAvatar() || !$this->av->getValue('av_email',1)) return;
    if (!$this->myPost_bForm())  b_debug::internalError(__METHOD__.' invoked, but !myPost_bForm');
    
    //
    // auto-login the applicant
    //
    if (!bAuth::authenticated()){
      $this->dbg('auto-login');
      bAuth()->imposeAvatar($this->av);
      JAM_access()->getRank();
    }
    
    //
    // upload the supporting documents updates if any
    //
    $this->apDocumentsBlock('check');
    if (!empty($_FILES)) foreach(array_keys($_FILES) as $name) $this->getValue($name,1,1);
    $dossierToRefresh = !empty($this->_FILES_uploaded);
    
    //
    // Acknowledge the application if not yet done
    //
    if ($this->errorless()){
      if (!$this->application_submit_date()) {
	// send mails to the applicant and admins
	$this->updateDB(array('ap_submitdate' => $this->setSubmitDate(),
			      'ap_ip'         => $_SERVER['REMOTE_ADDR']));
	myPear::MESSAGE("Registering application from ".$this->av->name());
	JAM_mailer()->acknowlegeApplication($this);
	
	myPear_logs()->add('application',$this->ID);

	$dossierToRefresh = True;
      }
      
      //
      // eventually refresh the dossier
      //
      if ($dossierToRefresh){
	$ap = new bForm_jam_Applicant($this->ID);
	$ap->MODE = 'RO';
	$ap->createDossier();
	$this->_FILES_uploaded = array();
      }	  

      // inform the referees
      foreach ($this->my_recommendations() as $recClass){
	if (!$recClass->is_requested()){
	  $recClass->set_requested();
	  JAM_mailer()->ask_for_recLetter($recClass->av,$this);
	}
      }
    }
    
    // Set 'bad form', 'bogus app', etc. flags for
    $this->setStatus();
  }
  

  private $recClasses = Null;
  function my_recommendations($reset=False){
    locateAndInclude('bForm_jam_Recommendation');
    if ($reset) $this->recClasses = Null;
    if ((int)$this->ID){
      if ($this->recClasses === Null){
	$this->recClasses = array();
	$q = myPear_db()->query("SELECT rec_id FROM jam_recommendations WHERE rec_apid=$this->ID ORDER BY rec_submitdate ASC");
	while ($r=myPear_db()->next_record($q)){
	  $rec = new bForm_jam_Recommendation($r['rec_id'],'RO');
	  $rec->uploadInit();
	  $this->recClasses[] = $rec;
	}
	$this->dbg(count($this->recClasses).' letter(s) requested');
      }
      return $this->recClasses;
    }else{
      return array();
    }
  }

  /*
   * Helper, count how many rec. letters are needed, requested, sent
   */
  private $errorsInTheForm_referees = Null;
  function countRecLetters(){
    static $n_sent, $n_ask, $n_need;
    if ($this->errorsInTheForm_referees === Null){

      // Count the letters
      $n_sent = $n_ask =0;
      $n_need = $this->pos->getValue('pos_n_referees',1);
      foreach($this->my_recommendations() as $rec) {
	$n_ask++;
	if ($rec->isReceived()) $n_sent++;
      }
      
      // Pop up an error if there is not enough letters.
      // Remove all the error messages concerning the referees and replace that by just 1 line
     foreach($this->errorsInTheForm as $key=>$value){
	if (stripos($key,'referee') !== False){
	  myPear::WARNING_T("Cancel errors message '$value' for '$key'",$this);
	  unset($this->errorsInTheForm[$key]);
	}
      }

      $this->errorsInTheForm_referees = array();
      if (($n = $n_need-$n_ask) > 0){
	$msg = ($n==1 ? 'A':$n) . ' referee' . ($n>1 ? 's are' : ' information is') . ' missing';
	$this->errorsInTheForm_referees[]  = $msg;
	$this->errorsInTheForm['_referee'] = $msg;
      }
    }
    return array($n_need,$n_ask,$n_sent);
  }
  
  /*
   *
   */
  function getEmail(){
    return $this->av->getEmail();
  }
}
