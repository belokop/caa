<?php
locateAndInclude('bHolder_jam');
locateAndInclude('APIaccess_jam');

/*
 *
 */
class bHolder_jam_Positions extends bHolder_jam{

  var $applicantMode = True;
  var $noSort        = True;
  
  /**
   * Manage list of bForm_jam_Position classes
   *
   * @param $mode
   *   Additional optional array of attributes. Recognized attributes:
   *     - 'init'  ignore the user rank, used for the initialization procedure 
   */
  function __construct($mode=''){
    parent::__construct();
    // avoid an infinite loop when called from the APIaccess, which lists positions to get the rank
    if ($mode=='init') $this->applicantMode = True;
    else               $this->applicantMode = (JAM_access()->getRank() < RANK_jam_observer);
  }
  
  /*
   */
  function header(){
    $this->title  = myOrg_name. ' positions';
    $header = array('pos_name'     => 'Position',
		    'pos_deadline' => 'Deadline for<br/>applications',
		    'pos_refnumber'=> ' ',
		    'pos_e'        => 'Results',
		    'isManaged'    => ' ');
    if(bAuth::authenticated())  unset($header['org_logo']);
    if ($this->applicantMode)   unset($header['pos_refnumber'],$header['pos_e'],$header['isManaged']); 
    
    return $header;
  }

  protected function _close(){
    if (JAM_manager_here){
      print b_btn::link('create new position',$action=b_url::same("?form=jam_Position&id=new"));
    }
  }

  /*
   *
   */
  function format(){
    $this->t->showLineCounter = False;
    $this->t->class['org_logo'] = 'bg_transp';
    $header = $this->header();

    $this->rec['org_logo'] = $this->pos->getLogo();
    $this->rec['pos_name'] = nbsp($this->pos->name());
    $this->rec['pos_deadline'] = b_time::medium_date($this->rec['pos_deadline']);
    if (JAM_posadmin_here){
      foreach(bForm_jam::_allStatusIcons($this->rec,'pos_status') as $icon) $this->t->extraTD[] = $icon;
    }
    if ($this->pos->managedByJAM()){
      if($this->applicantMode){
	if ($this->pos->isStatus(JAM_pos_not_yet)){
	  $this->t->extraTD[] = x('i',"applications campaign starts ".b_time::medium_date($this->pos->getValue('pos_startup')));
	}else{
	  $this->t->extraTD[] = b_btn::link('apply',$action=b_url::same("?form=jam_Applicant&id=new&JAM_posID=".$this->pos->ID."&auth=no"));
	}
	if ($url=trim($this->rec['pos_url'])) {
	  if (cnf_demo){
	    $this->t->extraTD[] = bJS()->popup('see announcement',$url);
	  }else{
	    if (strpos($url,'http')===0 && stripos($url,'.pdf')===False) $this->t->extraTD[] = bJS()->modal_iframe($url,'&nbsp;[ see announcement ]&nbsp;',$this->pos->name());
	    else                                                         $this->t->extraTD[] = b_btn::link('see announcement',$url);
	  }
	}
      }else{
	$this->rec['pos_e'] = $this->getEmployed($this->pos->ID);
	$this->rec['pos_name'] = x("a class='mypear_a' href=".x("'",b_url::same("?form=jam_Position&id=".$this->pos->ID)),
				   $this->pos->name());
      }
    }
  }
  
  /*
   *
   */
  function getEmployed($rec_id){
    locateAndInclude('bForm_Avatar');
    $list = array();
    foreach(array(JAM_ap_employed,JAM_ap_shortlist,JAM_ap_longlist) as $rank){
      $q = myPear_db()->query("SELECT av_firstname,av_lastname FROM jam_applicants LEFT JOIN zzz_avatars ON av_id=ap_avid ".
				" WHERE ap_status = '".JAM::status($rank)."' AND ap_posid=$rec_id ".
				myPear_db()->orderBy());
      while ($r = myPear_db()->next_record($q)){
	$list[] = bIcons()->get(JAM::$statuses[$rank]).bForm_Avatar::_fmtName('Lf',$r);
      }
    }
    return join('<br/>',$list);
  }

  /*
   * Read only the pos_id, the complete record will come later (see 
   */
  function query(){
    $q = myPear_db()->query("SELECT pos_id FROM jam_positions ".
			      " LEFT JOIN zzz_organizations ON pos_orgid = org_id ".
			      " WHERE org_id = '".myOrg_ID."'".
			      " ORDER BY pos_deadline DESC");
    return $q;
  }

  /*
   * Check that the position is visible by the logged in person
   */
  function rejectRecord(){ 
    $this->pos = loader::getInstance('bForm_jam_Position',$this->rec['pos_id'],'fatal');
    if(!$this->pos->isReadable()){
      $reply = True;
    }elseif ($this->pos->isStatus(JAM_pos_in_preparation) ||
	     $this->pos->isStatus(JAM_pos_deadline) ||
	     $this->pos->isStatus(JAM_pos_archived)){
      $reply = !$this->pos->isManager();
    }else{
      $reply = False;
    }
    if (!$reply) $this->rec = $this->pos->formDB;
    return (bool)$reply;
  }

  /*
   *
   */
  function noSuccess(){
    new b_table_simple(x('h3',join('<br/>',array(
						 "Thank you for your interest in ".myOrg_nameS.".",
						 '',
						 "At this time there are no positions open for application.",
						 "Please come back later.",
						 ))));
  }   
}
