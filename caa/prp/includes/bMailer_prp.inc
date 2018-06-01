<?php
locateAndInclude('bMailer');
locateAndInclude('APImenu_prp');

class bMailer_prp extends bMailer{
  
  protected $mailID = "Preprints Database";
  private $prp_formDB = array();
  
  public function __construct(){
    locateAndInclude('bForm_prp');  
    parent::__construct();
  }
  
  /*
   * Welcome mail to the accept a new preprint reservation
   */
  function informSubmitter($action='Reserved',$preprint){
    if (cnf_CLI)    return;
    $this->dbg();
    if ($preprint instanceof bForm) $this->prp_formDB = $preprint->formDB;
    elseif(!empty($preprint))       $this->prp_formDB = $preprint;
    $owner = loader::getInstance('bForm_Avatar',$this->prp_formDB['prp_avid']);
    switch($action){
    case 'Reserved':
      $login = $this->packPWD_axi(bAuth::$av);
      $subject = 'Preprint number '.bForm_prp::_serial($this->prp_formDB).' is reserved for you';
      $msg = "

When you have published this preprint in a Public Archive (i.e. you know the URL to the abstract) please return to
{login_url}
and complete the form.

You can correct eventual typos at any time (button \"edit\"), you may also cancel (button \"delete\") the preprint reservation.\n"
	// . "If you want to publish ONLY at '.myOrg_name.' you can upload your paper now (\"edit\" -> \"local archive\") from the same link.\n"
	;
      break;

    default:
      $login = '';
      $subject = 'Preprint number '.bForm_prp::_serial($this->prp_formDB).' is '.strToLower($action);
      $msg = "";
      break;
    }
    // Send the mail
    $this->send($login,
		$owner->getEmail(),
		$subject,
		$this->prp_info($action)."\n$msg\n",
		$syslog_thisMail=False,
		$no_preview =True);
  }
  
  /*
   * Inform the censors about new preprints
   */
  function informCensor($action,$preprint){
    if (cnf_CLI)    return;
    $this->dbg();
    $owner = loader::getInstance_new('bForm_Avatar',$preprint->getValue('prp_avid'));
    foreach(PRP_censors()->getUnitMemberIDs() as $av_id){
      $av = loader::getInstance('bForm_Avatar',$av_id);
      if ($av->ID != $owner->ID) $this->send($this->packPWD_axi($av),
					     $av->getEmail(),
					     bForm_prp::_serial($preprint->formDB)." $action for ".b_fmt::unescape($owner->fmtName('fl')),
					     $this->prp_info($action,$av,$preprint->formDB),
					     $syslog_thisMail=False,
					     $no_preview =True);
    }
  }

  /*
   * Remind the censor to approve pending endorsements
   */
  function reminder(){
    $reply = True;
    
    $this->dbg();
    if (!myPear_taskManager()->exec_request){
      myPear_taskManager()->addTask(get_class($this),__function__);
      return True;
    }
    
    $action = 'reminder';
    $q = myPear_db()->query("SELECT * ".
			    " FROM prp_list LEFT JOIN zzz_logs ON log_form = prp_id WHERE ".
			    join(' AND ',array("prp_day0 < ".(time()-PRP_remindingPeriod),
					       "prp_status = '".PRP_RESERVED."'",
					       "prp_orgid = ".myOrg_ID)).
			    "  GROUP BY prp_id ORDER BY log_time DESC");
    while ($r = myPear_db()->next_record($q)){
      if ((@$r['log_type'] !== $action) || 
	  ((int)@$r['log_time'] < time()-PRP_remindingPeriod)){
	$preprint = new bForm_prp($r['prp_id']);
	myPear_logs()->add($action,$preprint->ID);
	$reply = $this->informCensor($action,$preprint);
      }
    }
    return $reply;
  }
  
  /*
   *
   */
  private function packPWD_axi($av,$tab=tab_prp_update){
    return array($av,
		 array('q'=>join('/',array(PRP_MODULE,
					   APImenu::tab_code($tab)))),
		 False);
  }
  
  /*
   *
   */
  private function prp_info($action,$av=Null,$formDB=array()){

    $this->prp_formDB = array_merge($this->prp_formDB,$formDB);
    if (empty($this->prp_formDB['prp_tm'])) $this->prp_formDB['prp_tm'] = $this->prp_formDB['prp_day0'];
    switch (strToLower($action)){
    case 'reminder':
      $tit = 'Preprint '.$this->prp_formDB['prp_status'].' '. b_time::long_date($this->prp_formDB['prp_tm']).
	' ('.b_time::delta(time(),(int)$this->prp_formDB['prp_tm'])." ago) but has not being completed yet.\n\nURL:     {login_url}";
      $act = "Reminded by a cronjob";
      break;
      
    default:
      $tit = "Preprint $action ".b_time::long_date($this->prp_formDB["prp_tm"]);
      if (bAuth::$av) $act = "$action by: ".b_fmt::unescape(bAuth::$av->fmtName('fl'));
    }
    $owner = loader::getInstance('bForm_Avatar',$this->prp_formDB['prp_avid']);
    return join("\n",array($tit,
			   "Number:  ".bForm_prp::_serial($this->prp_formDB),
			   "Field:   ".PRP::$fields[$this->prp_formDB['prp_field']],
			   "Owner:   ".b_fmt::unescape($owner->fmtName('fl')),
			   "Title:   ".$this->prp_formDB['prp_title'],
			   "Authors: ".$this->prp_formDB['prp_authors'],
			   @$act));
  }
}
