<?php
// Time needed to "process" the letter (a few days), i.e. the referee should not be reminded before 
define('JAM_time_to_prepare_rl',25 * 3600);

locateAndInclude('bForm_jam');

//bForm_jam_Recommendation_sanity();
function bForm_jam_Recommendation_sanity($verbose=True){
  $q = myPear_db()->query("SELECT rec_id FROM jam_recommendations ".
			  " WHERE ( rec_askdate IS NULL OR rec_askdate=0 )");
  $n = 0;
  while ($r = myPear_db()->next_record($q)){
    $rec = new bForm_jam_Recommendation($r['rec_id']);
    $n += $rec->check_sanity();
  }
  if ($verbose) print x('p',x('strong',($n > 0 ? $n : 'No').' errors'));
}

class bForm_jam_Recommendation extends bForm_jam {
  
  public static $_debug   = False;
  var $TABLE   = 'jam_recommendations';
  var $TABLEID = 'rec_id';
  public static $_dbg_color = 'blueText';

  // Referee
  public $av = Null;

  // Applicant
  public $ap = Null;

  /*
   * Is the Avatar a Referee?
   */
  public static function _isReferee($av_id=Null){
    $reply = array();
    if (empty($av_id) && bAuth::authenticated()) $av_id = bAuth::$av->ID;
    if(!empty($av_id)) $reply = self::getRecommendations('positions',$av_id);
    return $reply;
  }

  /*
   * Get list of rec_id's
   *
   * @param $pos_id (optional) Position ID
   * @param $av_id (optional) Referee ID
   * @param $ap_id (optional) Applicant ID
   * @param return:
   *    - (int)counter if ($pos_id === 'count')
   *    - (array) rec_id's
   */
  public static function getRecommendations($pos_id=0, $av_id=0, $ap_id=0,$see_revoked=False) {

    // Small performance improvement
    if ($count_only = ($pos_id === 'count')){
      $pos_id = 0;
    }elseif ($return_positions = ($pos_id === 'positions')){
      $pos_id = 0;
    }
    
    // Protection against CMS features
    if (myPear_db()->tableExists('jam_applicants') && myPear_db()->columnExists('rec_submitdate','jam_recommendations')){
      $where = array(1);
      if (is_object($av_id)) $av_id = $av_id->ID;
      if (!empty($pos_id)) $where[] = "ap_posid = '$pos_id'";
      if (!empty($av_id))  $where[] = "rec_avid = '$av_id'";
      if (!empty($ap_id))  $where[] = "rec_apid = '$ap_id'";
      if (!$see_revoked)   $where[] = "ap_status != ".JAM::status(JAM_ap_revoked);
      
      $q = myPear_db()->qquery("SELECT rec_id,ap_posid FROM jam_recommendations ".
			       " LEFT JOIN jam_applicants ON rec_apid = ap_id ".
			       " WHERE ".join(' AND ',$where).
			       " ORDER BY rec_submitdate DESC",cnf_dev);
      if ($count_only){
	$reply = (($n=myPear_db()->num_rows($q)) ? $n : '');
      }else{
	$reply = array();
	while ($r = myPear_db()->next_record($q))$reply[] = ($return_positions 
							     ? $r['ap_posid']
							     : $r['rec_id']);
	$reply = array_unique($reply);
	sort($reply);  // Kill the association
      }
    }else{
      $reply = ($count_only ? 0 : array());
    }
    return $reply;
  }

  /*
   * Constructor. 
   */
  function __construct($id, $mode='RO'){

    if (($id instanceof bForm_jam_Applicant) && ($mode instanceof bForm_Avatar)){
      $this->ap = $id;
      $this->av = $mode;
      $this->dbg("ap=".$this->ap);       $this->dbg("av=".$this->av);
      parent::__construct('new','RW');
      $this->updateDB(array('rec_avid'=>$this->av->ID,
			    'rec_apid'=>$this->ap->ID));
    }else{
      parent::__construct($id, $mode);
      // Hook the Applicant / Referee
      $this->hook_ap_av();
    }
  }
  
  /*
   *
   */
  private function hook_ap_av(){
  
    // Re-read the DB record
    $this->readDefRecord();
  
    // Hook the applicant
    if (!($this->ap instanceof bForm_jam_Applicant) && ($rec_apid = $this->getValue('rec_apid',1))){
      if(@bForm_jam_Applicant::$current->ID == $rec_apid) $this->ap = bForm_jam_Applicant::$current;
      else                                                $this->ap = new bForm_jam_Applicant($rec_apid);
      $this->updateDB(array('rec_apid'=>$this->ap->ID));
    }
    
    // Hook the referee
    if (!($this->av instanceof bForm_Avatar_jam)    && ($rec_avid = $this->getValue('rec_avid',1))){
      $this->av = new bForm_Avatar_jam($rec_avid);
      $this->updateDB(array('rec_avid'=>$this->av->ID));
    }
    $this->check_sanity();
  }
  
  /*
   * Helper
   */
  private $check_sanity_conter = 0;
  function check_sanity(){
    $errors = array();
    if (!$this->check_sanity_conter++){    
      if (!($this->av instanceof bForm_Avatar)){
	b_debug::var_dump($this->av,''.$this->av);
	b_debug::internalError("\$this->av is not bForm_Avatar");
      }
      if (!($this->ap instanceof bForm_jam_Applicant)){
	b_debug::var_dump($this->ap);
	b_debug::internalError("\$this->ap is not bForm_jam_Applicant");
      }
      
      $isReceived = $this->isReceived();
      if ($rec_askdate   = $this->getValue('rec_askdate',1))   $rec_askdate   = b_time::short_date($rec_askdate);   else $rec_askdate    = 0;
      if ($rec_submitdate= $this->getValue('rec_submitdate',1))$rec_submitdate= b_time::short_date($rec_submitdate);else $rec_submitdate = 0;
      if (!$rec_askdate && $this->ap->isConsidered(True)){
	$errors[] = "rec_askdate=0 rec_submitdate=$rec_submitdate";
      }
      if (!$isReceived && $rec_submitdate){
	$errors[] = "isReceived()=NO, but rec_submitdate=$rec_submitdate";
      }
      foreach($errors as $e) MSG::WARNING_T("$e ".$this->ap->name().' <--- '.$this->av->name(),$this); 
    }
    return count($errors);
  }

  /*
   *
   */  
  function defineVariables() {

    $this->defineVariable(array('rec_letter_file' =>'The letter of recommendation',
				'rec_comment'     =>'Comment',
				'rec_avid'        =>'rec_avid',
				'rec_apid'        =>'rec_apid',
				'rec_askdate'     =>'rec_askdate',
				'rec_submitdate'  =>'rec_submitdate',
				'_virt_rec_gotdate'=>'Submitted',
				'_virt_rec_referee'=>'Referee',
				),True);
    
    $this->defineTypes(array('expected_ext'=>array('rec_letter_file'=>JAM_docs_ext),
			     'isMBE'       =>array('rec_comment',  // Might Be Empty
						   'rec_submitdate',						
						   '_virt_rec_referee'),
			     'isRO'        =>array('rec_askdate',  // Read Only
						   'rec_submitdate',
						   '_virt_rec_gotdate',
						   '_virt_rec_referee')));
  }
  
  /*
   *
   */
  function __clean_cache(){
    $this->hook_ap_av();
    if (is_object($this->av)) $this->av->__clean_cache();
    loader::getInstance(get_class($this), $this->ID, 'clean');
  }

  /*
   *
   */
  protected function getValue_validate_empty($name,&$value=Null){
    $msg = parent::getValue_validate_empty($name,$value);
    if (b_posix::is_empty($value)){
      if ($name == 'rec_letter_file')	$msg = '';
    }
    return $msg;
  }

  /*
   *
   */
  private $uploadInit_path_reply = Null;
  function uploadInit_path($module_path=array(),$module=Null){
    if ($this->uploadInit_path_reply === Null){
      $this->hook_ap_av();
      $this->uploadInit_path_reply = $this->ap->uploadInit_path();
    }
    return $this->uploadInit_path_reply;
  }

  /*
   * Return rec.letter file path only if the file exists
   */
  function uploadedFile_path($name='rec_letter_file',$onlyIfExists=True){
    return parent::uploadedFile_path($name,True);
  }

  function uploadedFile_url($name='rec_letter_file',$onlyIfExists=True){
    return parent::uploadedFile_url($name,True);
  }

  /*
   * Get the file name from the Applicant class
   */
  function uploadedFile_name($name='rec_letter_file',$argsP=array(),$ext=True){
    $this->defineVariables();

    $this->hook_ap_av();
    $this->ap->vars['rec_letter_file']   = @$this->vars['rec_letter_file'];
    $this->ap->formDB['rec_letter_file'] = @$this->formDB['rec_letter_file'];
    $reply = $this->ap->uploadedFile_name('rec_letter_file',array($this->ID),True);
    $this->dbg($reply);
    return $reply;
  }

  public function set_requested($date=Null) {
    if ($date === Null) $date = time();
    $this->updateDB(array('rec_askdate'=>$date));
    return $date;
  }
  
  /*
   * Leave the referee in peace for a couple of days
   */
  public function is_reminderNeeded(){
    $this->hook_ap_av();
    switch ($this->ap->isConsidered() && !$this->ap->isStatus(JAM_ap_revoked)){
    case True:
      if ($reply = !$this->isReceived()){
	if ($date = $this->is_requested())  $reply = ((b_time::txt2unix($date) + JAM_time_to_prepare_rl) < time());
      }
      break;
      
    case False:
      $reply = False;
    }
    $this->dbg($reply);
    return $reply;
  }

  function is_requested() {
    if ($date = $this->getValue('rec_askdate',True)) $reply = $date;
    elseif ($this->isReceived())                     $reply = $this->set_requested();
    else                                             $reply = 0;
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * Due to various reasons we cross-check that the rec.letter REALLY exists,
   * hence we look for the letter file on disks.
   */
  private $_isReceived = '???';
  function isReceived() {
    if ($this->_isReceived === '???'){
      if ($this->getFile()) $this->_isReceived = $this->timestamp;
      else                  $this->_isReceived = False;
      $this->dbg($this->_isReceived);
    }
    return $this->_isReceived;
  }
  
  function date(){
    if (!($date = $this->getValue('rec_submitdate',1))) $date = $this->getValue('rec_askdate',1);
    return $date;
  }

  /*
   *
   */
  private $_getFile = '???';
  function getFile() {
    if ($this->_getFile === '???')      $this->_getFile = $this->uploadedFile_url('rec_letter_file',True);
    return $this->_getFile;
  }
  
  /*
   *
   */
  function readDefRecord(){ // getValue is called, DANGEROUS !!! 

    if (b_posix::is_int($this->ID)) {
      $w[] = $this->TABLEID.'='.$this->ID;
    }elseif (is_object($this->av) && is_object($this->ap)){
      $w[] = "av_id = ".$this->av->ID;
      $w[] = "ap_id = ".$this->ap->ID;
    }elseif (($rec_avid=$this->getValue('rec_avid',1,1)) && ($rec_apid=$this->getValue('rec_apid',1,1))){ 
      $w[] = "av_id = $rec_avid";
      $w[] = "ap_id = $rec_apid";
    }
    
    if (empty($w)){
      $this->dbg("Cant hook neither applicant nor referee",cnf_dev||self::$_debug);
      return;
    }
    $q = myPear_db()->query("SELECT * FROM $this->TABLE ". 
			    " LEFT JOIN zzz_avatars    ON rec_avid  = av_id ".
			    " LEFT JOIN jam_applicants ON rec_apid  = ap_id ".
			    " WHERE ".join(' AND ',$w));
    $this->formDB = array();
    while ($r = myPear_db()->next_record($q)){
      $this->formDB = $r;
      $this->ID = $this->formDB[$this->TABLEID];
    }
  }

  /*
   *
   */
  function getInfo(){
    $this->hook_ap_av();
    $this->getFormOpen();
    $this->formDB['Referee']    = $this->av->name();
    $this->formDB['Applicant']  = $this->ap->av->name(); 
    $this->formDB['Letter sent']= b_time::medium_date($this->isReceived());
    $this->formBlock('','Letter of recommendation',array('Referee','Applicant','Letter sent'));
    $this->closeForm();
  }

  function getDescr_file($name=''){
    return $this->name('ar');
  }

  function name($pattern='a'){
    $this->hook_ap_av();
    $name = array('Rec. letter'); 
    if (stripos($pattern,'a') !== False){
      if ($this->ap->av instanceof bForm_Avatar) $name[] = 'for '. x('em',$this->ap->av->fmtName('Lf'));
      elseif(cnf_dev) b_debug::var_dump($this->ap->av,'bForm_Recommendation Applicant');      // Sanity ...
    }
    if (stripos($pattern,'r') !== False){
      if ($this->av     instanceof bForm_Avatar) $name[] = 'from '.x('em',$this->av->fmtName('Lf'));
      elseif(cnf_dev) b_debug::var_dump($this->av,'bForm_Recommendation Referee');
    }
    return join(' ',$name);
  }

  function isManager() {
    $this->hook_ap_av();
    return $this->ap->pos->isManager();
  }

  function isOwner() {
    $this->hook_ap_av();
    $reply = (@bAuth::$av->ID == $this->av->ID);
    $this->dbg($reply,(bool)($reply && (self::$_debug || cnf_show_a)));
    return $reply;
  }

  /*
   * admin should be allowed to submit recommendation on behalf of the referee
   */
  function isWritable() {
    $this->hook_ap_av();
    return b_cnf::check_conditions(array(False=>array('arch'=>(!$this->ap->pos->isConsidered() || !$this->isReadable())),
					 'default' => parent::isWritable()),
				   $this);
  }

  function isReadable() {
    return b_cnf::check_conditions(array(False=>array('prnt'=> !parent::isReadable(),
						      'arch'=>(!$this->isOwner() && ($this->getFile() && ((time() - $this->timestamp)/24/3600 > JAM_rl_expiration)))),
					 True =>array('own' => $this->isOwner(),
						      'may' => JAM_access()->may_seeReviews()),
					 'default' => False),
				   $this);
  }
  
  function uploadFile_acknowledge($name) {
    $this->hook_ap_av();
    if ($name != 'rec_letter_file') b_debug::internalError("Expect 'rec_letter_file', got'$name'");
    myPear_logs()->add('recommendation',$this->ID);
    JAM_mailer()->acknowlegeRecLetter($_FILES[$name]['name'],
					$this->av,
					$this->ap);
    $this->updateDB(array('rec_submitdate'=>time(),
			  'rec_ip' =>$_SERVER['REMOTE_ADDR']));
  }
  
  function getForm() {
    $this->hook_ap_av();
    if (is_object($this->ap)){
      print MSG::H1(b_t::_("Recommendation for").' '.(is_object($this->ap)?$this->ap->av->fmtName('Lf'):'????'),'noTranslate');
      if (!$this->getFile())	$this->MODE = 'RW';
      
      $this->formDB['_virt_rec_referee'] = $this->av->name();
      $this->formDB['_virt_rec_gotdate'] = b_time::medium_date($this->isReceived());
      
      $this->getFormOpen();    
      $this->formBlock('','Recommendation letter',
		       array('rec_letter_file' =>'uploadFile',
			     'rec_comment'     =>'textArea',
			     '_virt_rec_referee',
			     '_virt_rec_gotdate'));
      //      $this->closeForm('submit recommendation');
      $this->closeForm();
    }else{
      MSG::ERROR("Sorry, can not upload the reference letter");
    }
  }

  function formOKspecific() {
    $this->hook_ap_av();
    $this->dbg('no errors');
    if ($this->myPost_bForm()){
      $this->ap->MODE = 'RO';
      $this->ap->createDossier();
    }
  }
}
