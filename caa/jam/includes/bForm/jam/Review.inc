<?php
locateAndInclude('bForm_jam');
locateAndInclude('APIaccess_jam');
class bForm_jam_Review extends bForm_jam{
  
  static public $rcodes = array(5=>'5 - Certainly YES',
				4=>'4 - To be considered',
				3=>'3 - No opinion',
				2=>'2 - Week candidate',
				1=>'1 - Certainly No',
				0=>'<span style=color:red;font-weight:bold>Abstain</span>-SKIP- (please comment)',
				);  

  var $isVITAL= array('r_rating' => True);
  var $TABLE  = 'jam_reviews';
  var $TABLEID= 'r_id';
  var $modalRW_enabled = True;

  public $av = Null;  // Reviewer object
  public $ap = Null;  // Applicant object
  
  /*
   * Get the raw review database records   
   *
   * @param $pos_id (optional) Position ID
   * @param $av_id (optional) Referee ID
   * @param $ap_id (optional) Applicant ID
   * @param return:
   *    - (int)counter if ($pos_id === 'count') 
   *    - (array) raw database records
   */
  public static function getReviews($pos_id=Null, $av_id=Null,$ap_id=Null) {
    $count_only = ($pos_id === 'count');
    if (empty($pos_id) || $count_only) $pos_id = JAM_posID();
    $where = array("ap_posid = '$pos_id'");
    if (!empty($av_id)) $where[] = "r_avid = '$av_id'";
    if (!empty($ap_id)) $where[] = "r_apid = '$ap_id'";
    
    $q = myPear_db()->qquery("SELECT r_id,r_rating FROM jam_reviews ".
			       " LEFT JOIN jam_applicants ON r_apid = ap_id ".
			       " WHERE ".join(' AND ',$where),False);
    if ($count_only){
      $reply = (($n=myPear_db()->num_rows($q)) ? $n : '');
    }else{
      $reply = array();
      while ($r = myPear_db()->next_record($q)) $reply[] = $r;
    }
    return $reply;
  }
  
  private $av_id = Null;
  private $ap_id = Null;
  public function __construct($id, $mode='RO', $ap_id=Null, $av_id=Null){
    $this->ap_id = $ap_id;
    $this->av_id = $av_id;
    parent::__construct($id, $mode);
    $this->_init();
  }

  /*
   *
   */
  private function _init(){
    if (empty($this->ap_id)) $this->ap_id = $this->getValue('r_apid',True,True);
    if (empty($this->av_id)) $this->av_id = $this->getValue('r_avid',True,True);

    $this->ap = myPear::getInstance('bForm_jam_Applicant', $this->ap_id, array('fatal','strict'));
    $this->av = myPear::getInstance('bForm_Avatar_jam',    $this->av_id, array('fatal','strict'));
    
    $this->dbg($this->av);
    $this->dbg($this->ap);
    $this->readDefRecord();

    // Create the Review record if it is not yet done
    if (empty($this->formDB)){
      if ((int)$this->ID) b_debug::internalError($this." something wrong with reviews");
      $this->isVITAL = array();
      $this->updateDB($where);
    }
    if (empty($this->formDB['r_time']))  $this->formDB_modified['r_time'] = time();
  }

  /*
   *
   */
  function __clean_cache(){
    $this->av->__clean_cache();
    myPear::getInstance(get_class($this), $this->ID, 'clean');
  }

  /*
   * Get the review record, create on the fly if missing
   */
  function readDefRecord(){
    if ((int)$this->ID) $where = array("r_id  = '".$this->ID."'");
    else                $where = array("r_apid= '".$this->ap->ID."'",
				       "r_avid= '".$this->av->ID."'");

    $this->formDB = array();
    $q = myPear_db()->qquery("SELECT * FROM jam_reviews LEFT JOIN jam_applicants ON r_apid = ap_id WHERE ".join(' AND ',$where),self::$_debug);
    while($r = myPear_db()->next_record($q)) $this->formDB = $r;
    
  }

  /*
   *
   */
  function defineVariables() {
    $this->defineVariable(array('r_comment'=> 'My review',
				'r_rating' => 'My rating',
				'__rating' => 'My rating',
				'r_file'   => 'r_file',
				'r_time'   => '',
				'r_avid'   => 'Reviewer',
				'r_apid'   => 'Applicant',
				'Reviewer' => '',
				),True);

    $this->defineTypes(array('isMBE' => array('r_comment',
					      'r_file'),
			     'isRO'  => array('r_time',
					      'Reviewer')));
  }

  /*
   *
   */
  function isOwner() {
    if (!(int)$this->ID)                                             $reply = 'new report';
    elseif(is_object(bAuth::$av) && $this->av->ID == bAuth::$av->ID) $reply = $this->av->name();
    else                                                             $reply = False;
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Get the review name
   *
   * @param - 'a' for applicant, 'r' for referee 
   */
  function name($pattern='a'){
    $this->_init();
    $name = array();
    if (stripos($pattern,'a') !== False) $name[] = $this->ap->av->fmtName();
    if (stripos($pattern,'r') !== False) $name[] = '(reviewer '.$this->av->fmtName('sfL').')';
    return join(', ',$name);
  }

  /* 
   * is the review readable by the currently logged in person? 
   */
  function isReadable(){
    if (!($reply = $this->isOwner()))
      if (JAM_access()->may_seeReviews()){
	foreach(array_keys(JAM::SC_ranks()) as $rank){
	  if ($reply=(JAM_unit($rank)->getUnitMemberStatus() == JAM::status(JAM_review_public))) break;
	}
      }
    $this->dbg($reply,(bool)(cnf_show_a||self::$_debug));
    return $reply;
  }

  /*
   * Writable only by the owner
   */
  function isWritable() {
    return $this->isOwner();
  }
  
  /*
   * The Review dialog
   */
  function getForm(){

    $this->embeddedRW_enabled = True;
    myPear::H1($this->ap->av->fmtName());
    
    // When the form is opened, all the values are updated. Hence the time is to update the ratings    
    $this->getFormOpen();    
    $this->ap->setRating();
    
    // Prepare the dialog
    if ($this->MODE == 'RW'){
      self::$rcodes[0] = str_replace('-SKIP-','',self::$rcodes[0]);
      $dialog = array('r_rating' => array('radioButton',self::$rcodes,Null,Null));
    }else{
      $r = $this->getValue('r_rating',1);
      $this->formDB['__rating'] = preg_replace('/-SKIP-.*/','',self::$rcodes[$r]);
      $dialog = array('__rating' => array('textField'));
    }
    
    $this->formBlock('','My review '.b_time::short_date($this->getValue('r_time')),
		     array_merge($dialog,
				 array('r_comment'=>array('textArea',11,60),
				       'r_avid'   =>array('textHidden',$this->av->ID),
				       'r_apid'   =>array('textHidden',$this->ap->ID))));
    $this->formSignature($this->av->name());

    if ($this->MODE == 'RO'){
      $d = $this->ap->createDossier();
      $extra_buttons = array();
      //if ($this->isWritable() && !empty($d))  $extra_buttons['see the applicant dossier'] = $d;
      $this->closeForm('Update review',$extra_buttons);
    }else{
      $this->closeForm('Submit review');
    }
 }
}
