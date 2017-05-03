<?php
/*
 * Root bForm for the prp module
 */
locateAndInclude('bForm');
locateAndInclude('bForm_Avatar');

define('prp_IDV','http://arxiv.org/abs/');

/*
 * Load the debug helper
 */
if (cnf_dev) locateAndInclude('bForm_debug.inc');

class bForm_prp extends bForm{

  protected $normalLine = 999;
  
  /*
   * Check does the person have submitted preprint(s) in any organization
   */
  public static function has_submittions($av_id){
    static $cache = Null;
    if ($cache === Null){
      list($table,$id) = array_values(b_reg::sql_ids('bForm_prp',PRP_MODULE));
      $q = myPear_db()->query("SELECT prp_avid FROM $table WHERE 1 GROUP BY prp_avid");
      while ($r = myPear_db()->next_record($q)){
	$cache[] = $r['prp_avid'];
      }
    }
    return in_array($av_id,$cache);
  }
  
  /*
   *
   */
  private function get_freePreprintNumber(){
    $prp_report = 1;
    $q = myPear_db()->qquery("SELECT prp_report FROM prp_list WHERE (YEAR(FROM_UNIXTIME(prp_day0)) = YEAR(FROM_UNIXTIME(UNIX_TIMESTAMP()))) ".
			     " AND prp_status != '".PRP_CANCELED."'".
			     " AND prp_status != '".PRP_RESERVED."'".
			     " ORDER BY prp_report",True);
    while ($r = myPear_db()->next_record($q)) $prp_reports[] = $r['prp_report'];
    while(in_array($prp_report,$prp_reports)) { $prp_report++; }
    $this->dbg($prp_report,cnf_dev);
    return $prp_report;
  }

  /*
   * Get free report number
   */
  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode,$SQL_backend=True);
  }

  /*
   * Define the area where the local copies of preprints are kept (local database)
   */ 
  function uploadInit_path($module_path=array(),$module=PRP_MODULE){
    $module_path = array(date('Y',$this->getValue('prp_day0')));
    return parent::uploadInit_path($module_path,$module);
  }

  /*
   * The filename in the local database is just the preprint number
   */
  function uploadedFile_name($name,$args=array(),$ext=True){
    switch($name){
    case 'prp_local':
      if ($reply = @$this->formDB[$name]){
	if (!$ext) $reply = preg_replace('/\.[a-zA-Z]*(\.(gz|bz.?|zip))?$/','',$reply);
      }else{
	$reply = sprintf("%s%02d%s",
			 date('Y',$this->getValue('prp_day0')),
			 $this->getValue('prp_report'),
			 ($ext ? ".pdf" : ""));
      }
      break;

    default:
      $reply = '???';
      myPear::ERROR('???'.b_debug::__());
    }
    $this->dbg($reply,True);
    return $reply;
  }

  /*
   *
   */
  public function getValue($name, $asIs=True, $d_MBE=True){
    return parent::getValue($name,True,True);
  }

  /*
   *
   */
  function defineVariables() {

    $this->defineVariable(array(
				'prp_avid'     => '&nbsp;',
				'prp_title'    => 'Title',
				'prp_authors'  =>'Authors',
				'prp_doi'      =>'Digital Object Identifier (DOI®)',
				'prp_field'    =>'Research field',
				'prp_publisher'=>'Published in',
				'prp_archive'  =>'URL in external archive',
				'prp_local'    =>'Local archive copy',
				'prp_status'   =>' ', 
				'prp_day0'     =>' ',
				'prp_tm'       =>' ',),
			  $inversed=True);
    $this->defineTypes(array('isVITAL'     =>array('prp_field',     // All those must present
						   'prp_title',
						   'prp_authors'),
			     'isMBE'       =>array('prp_publisher', // Might Be Empty
						   'prp_archive',
						   'prp_local',
						   'prp_doi',
						   'prp_tm',),
			     'isRO'        =>array('prp_avid'),
			     'IDV'         =>array('prp_archive'=>prp_IDV),
			     'expected_ext'=>array('prp_local'  =>''), // no restriction
			     ));
  }

  /*
   *
   */
  function url_local(){
    if ($f = $this->getValue('prp_local')){
      $this->uploadInit();
      $reply = $this->uploadsURL . $f;
    }else{
      $reply = $this->getValue('prp_archive');
    }
    $this->dbg($reply);
    return $reply;
  }

  protected function _recordCreated() {
    if (!cnf_CLI){
      PRP_mailer()->informCensor('Reserved',$this);
      PRP_mailer()->informSubmitter('Reserved',$this);
      myPear::MESSAGE("Allocated preprint ".$this->serial());
      myPear_logs()->add('reserve',$this->ID);
    }
  }

  protected function _recordUpdated() {
    if (!cnf_CLI){
      myPear_db()->query("UPDATE $this->TABLE SET prp_tm = ".time()." WHERE $this->TABLEID='$this->ID'");
      $this->readDefRecord();
    }
  }
  
  /*
   *
   */
  function formOKspecific() {
    // Check the request to create a new record
    if (!(int)$this->ID && ($this->getValue('prp_title') &&
			    $this->getValue('prp_authors') &&
			    $this->getValue('prp_field'))){
      
      /*
      // Get the maximum preprint number
      $prp_report = 1;
      $q = myPear_db()->qquery("SELECT MAX(prp_report) as N FROM prp_list WHERE prp_orgid = ".myOrg_ID." AND YEAR(FROM_UNIXTIME(prp_day0)) = ".prp_year9);
      while ($r = myPear_db()->next_record($q)) $prp_report = $r['N'] + 1;
      */
      
      // Set the preprint information
      foreach(array('prp_report' => self::get_freePreprintNumber(), // $prp_report + 1,
		    'prp_avid'   => $this->getValue('prp_avid'),
		    'prp_orgid'  => myOrg_ID,
		    'prp_status' => PRP_RESERVED,
		    'prp_day0'   => time(),
		    'prp_tm'     => time()) as $k=>$v) $this->getValue_reset($k,$v);
    }
    // Workaround
    if ($this->getValue('prp_archive') === prp_IDV) $this->getValue_reset_forced('prp_archive');
  }

  function isWritable(){
    if ($this->getValue('prp_status') == PRP_CANCELED) $reply = PRP_manager_here;
    else                                               $reply = parent::isWritable();
    return    $reply;
  }

  function isManager(){
    $reply = PRP_censor_here;
    $this->dbg($reply);
    return    $reply;
  }

  function isOwner(){
    $reply = (is_object(bAuth::$av) && (bAuth::$av->ID == $this->getValue('prp_avid')));
    $this->dbg($reply);
    return    $reply;
  }

  protected function getFormHeader(){
    if ((int)$this->ID){
      myPear::setPageHeader($this->serial());
    }else{
      myPear::setPageHeader('Reservation of new Preprint Number');
    }
  }

  private function update_button($icon='book'){
    return ((int)$this->ID && $this->isWritable() 
	    ? array('d'=>'update',
		    'i'=>$icon,
		    'l'=>b_url::same("?form=prp&id=$this->ID&mode=RW")) 
	    : array());
  }
    

  protected function getFormBody(){

    if (is_object(bAuth::$av)){
      /*
       * Main block - title & authors
       */
      if (!$this->getValue('prp_avid'))  $this->formDB['prp_avid'] = bAuth::$av->ID;
      $this->formBlock('p1',
		       ((int)$this->ID ? $this->serial() : 'Allocating preprint number'),
		       array('prp_field'  => array('selectMenu',array_diff(PRP::$fields,array('all'=>PRP::$fields['all']))),
			     'prp_authors'=> array('textArea',1),
			     'prp_title'  => array('textArea',1)),
		     $this->update_button());
      
      /*
       * Preprint location
       */
      if (($this->MODE == 'RW') && (int)$this->ID){
	// Do not offer "local storage"
	if (False) myPear::MESSAGE("You may store the preprint in a ".
				   "<br/>&nbsp;* external world-wide archive AND/OR".
				   "<br/>&nbsp;* ".myOrg_name." local archive<br/>");
	myPear::MESSAGE("Please don't forget to update \"".$this->vars['prp_publisher']."\" field when the preprint is published");
      }
      
      $dialog = array(
		      'prp_publisher'=>array('textField'),
		      'prp_doi'      =>array('textField'),
		      'prp_archive'  =>array('textField'),
		      'prp_local'    =>array('uploadFile'));
      // Do not offer the "local copy", not used any more
      if (!$this->getValue('prp_local')) unset($dialog['prp_local']);

      if ((int)$this->ID) $this->formBlock('p2',
					   'Preprint location',
					   $dialog,
					   $this->update_button('000031'));  
      
      /*
       * Preprint owner
       */
      if (PRP_manager_here && !$this->getValue('prp_avid')){
	$this->isRO['prp_avid'] = False;
	$submitters = PRP_submitters()->get();
	if (empty($submitters[bAuth::$av->ID]))  $submitters[bAuth::$av->ID] = bAuth::$av->name();
	$this->formBlock('','Preprint submitter', array('prp_avid' => array('selectMenu',$submitters)));
      }else{
        $this->textHidden('prp_avid',bAuth::$av->ID);
      }
      
      $signature = bAuth::$av->name();
      if ($tm = $this->getValue('prp_tm'))      $signature .= date(' Y-m-d H:i',$tm);
      $this->formSignature($signature);
    }
  }

  private function serial(){
    return self::_serial(array('prp_id'    =>$this->getValue('prp_id'),
			       'prp_day0'  =>$this->getValue('prp_day0'),
			       'prp_report'=>$this->getValue('prp_report'),
			       'prp_field' =>$this->getValue('prp_field')));
  }
  
  /*
   *
   */
  static function _serial($formDB=array(),$prefix=True){
    $serial = ((int)$formDB['prp_id']
	       ? sprintf("%s %s-%03d%s",
			 ($prefix ? mb_strToUpper(myOrg_name) : ''),
			 date('Y',(int)$formDB['prp_day0']),
			 (int)$formDB["prp_report"],
			 ((date('Y',(int)$formDB['prp_day0']) <= 2003) ? $formDB["prp_field"] : ""))
	       : 'to be allocated'); 
    return trim($serial);
  }

  function setAccepted(){
    $this->updateDB(array('prp_status' => PRP_OK,
			  'prp_tm'     => time()));
    myPear::MESSAGE("Preprint ".$this->serial()." is ACCEPTED");
    PRP_mailer()->informCensor('Accepted',$this);
    PRP_mailer()->informSubmitter('Accepted',$this);
    myPear_logs()->add('accept',$this->ID);
  }
  
  function setCanceled(){
    $this->updateDB(array('prp_status' => PRP_CANCELED,
			  'prp_tm'     => time()));
    myPear::MESSAGE("Preprint ".$this->serial()." is CANCELED");
    PRP_mailer()->informCensor('Canceled',$this);
    PRP_mailer()->informSubmitter('Canceled',$this);
    myPear_logs()->add('cancel',$this->ID);
  }
}
