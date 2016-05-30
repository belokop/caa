<?php
locateAndInclude('bMailer');
locateAndInclude('APImenu_prp');

class bMailer_prp extends bMailer{
  
  var $debug  = False;
  var $mailID = "Visits Manager API";
  var $isMailAutomatic = False;
  private $prp_formDB = array();
  
  public function __construct(){
    locateAndInclude('bForm_prp');  
    parent::__construct();
  }
  
  /*
   * Welcome mail to the accept a new preprint registration
   */
  function submitter($action='Reserved',$preprint){
    if (cnf_CLI)    return;
    if ($preprint instanceof bForm) $this->prp_formDB = $preprint->formDB;
    elseif(!empty($preprint))       $this->prp_formDB = $preprint;
    $owner = myPear::getInstance('bForm_Avatar',$this->prp_formDB['prp_avid']);
    $this->send($this->packPWD_axi(),
		$owner->getEmail(),
		'Preprint number '.bForm_prp::_serial($this->prp_formDB).' is reserved for you',
		$this->prp_info($action).'

When you have published this preprint in a Public Archive (i.e. you know the URL to the abstract) please return to
{login_url}
and complete the form.

You can correct eventual typos at any time (button "edit"), you may also cancel (button "delete") the preprint reservation.

If you want to publish ONLY at '.myOrg_name.' you can upload your paper now ("edit" -> "local archive") from the same link.

',
		$syslog_thisMail=False,
		$no_preview =True);
  }

  /*
   * Inform the censors about new preprints
   */
  function censor($action,$formDB=array()){
    if (cnf_CLI)    return;
    $owner = myPear::getInstance('bForm_Avatar',$formDB['prp_avid']);
    foreach(PRP_censors()->getUnitMemberIDs() as $av_id){
      $av = myPear::getInstance('bForm_Avatar',$av_id);
      $this->send($this->packPWD_axi($av),
		  $av->getEmail(),
		  bForm_prp::_serial($formDB)." $action for ".b_fmt::unescape($owner->fmtName('fl')),
		  $this->prp_info($action,$av,$formDB),
		  $syslog_thisMail=False,
		  $no_preview =True);
    }
  }

  /*
   * Remind the censor to approve pending endorsements
   */
  function reminder(){
    if (cnf_CLI)    return;
    
    if (!myPear_taskManager()->exec_request){
      if (!@$_SESSION[__METHOD__]++) myPear_taskManager()->addTask(get_class($this),__function__);
      return;
    }
    
    $action = 'reminder';
    $q = myPear_db()->query("SELECT * ".
			    " FROM prp_list LEFT JOIN zzz_logs ON log_form = prp_id WHERE ".
			    join(' AND ',array("prp_day0 < ".(time()-PRP_remindingPeriod),
					       "prp_status = '".PRP_RESERVED."'",
					       "prp_orgid = ".myOrg_ID)).
			    "  GROUP BY prp_id ORDER BY log_time DESC");
    while ( $formDB = myPear_db()->next_record($q)){
      if ((@$formDB['log_type'] !== $action) || 
	  ((int)@$formDB['log_time'] < time()-PRP_remindingPeriod)){
	myPear_logs()->add($action,$formDB['prp_id']);
	$this->censor($action,$formDB);
      }
    }
  }
  
  /*
   *
   */
  private function packPWD_axi($av=Null){
    if (empty($av)) $av = bAuth::$av; 
    return array($av,
		 array('q'=>join('/',array(PRP_MODULE,
					   APImenu::tab_code(tab_prp_mng),
					   APImenu::tab_code(tab_prp_update)))),
		 False);
  }
  
  private function prp_info($action,$av=Null,$formDB=array()){

    $this->prp_formDB = array_merge($this->prp_formDB,$formDB);
    switch (strToLower($action)){
    case 'reminder':
      $tit = 'Preprint '.$this->prp_formDB['prp_status'].' '. b_time::long_date($this->prp_formDB['prp_tm']).
	' ('.b_time::delta(time(),(int)$this->prp_formDB['prp_tm'])." ago) but has not being completed yet.\n\nURL:     {login_url}";
      $act = "Reminded by a cronjob";
      break;
      
    default:
      $tit = "Preprint $action ".b_time::long_date($this->prp_formDB["prp_tm"]);
      if (bAuth::$av) $act = "$action by ".b_fmt::unescape(bAuth::$av->fmtName('fl'));
    }
    $owner = myPear::getInstance('bForm_Avatar',$this->prp_formDB['prp_avid']);
    return join("\n",array($tit,
			   "Number:  ".bForm_prp::_serial($this->prp_formDB),
			   "Field:   ".PRP::$fields[$this->prp_formDB['prp_field']],
			   "Owner:   ".b_fmt::unescape($owner->fmtName('fl')),
			   "Title:   ".$this->prp_formDB['prp_title'],
			   "Authors: ".$this->prp_formDB['prp_authors'],
			   @$act));
  }
}
