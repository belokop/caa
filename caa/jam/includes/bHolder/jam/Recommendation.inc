<?php
locateAndInclude('bHolder_jam');
locateAndInclude('bForm_jam_Recommendation');
locateAndInclude('b_checkBox');

class bHolder_jam_Recommendation extends bHolder_jam{
  
  private $nRejected = 0;
  var $show_letters_for_old_positions = True;
  var $sortBy_default = 'ap_posid';
  var $noSort = True;
  var $where  = array("ap_posid > 0"); // sanity

  /*
   *
   */
  public function __construct($av_id=0,$rf_id=0){
    if ($rf_id) JAM_RF($rf_id);
    parent::__construct();
    if ((bool)$av_id) {     /* list all the recommendations for the referee */
      $this->av = myPear::getInstance('bForm_Avatar_jam',$av_id,array('fatal','strict'));
      $this->where[] = "rec_avid = ".$this->av->ID;
      $this->title = bText::_("Letters of recommendation");
    }elseif ($this->rf_id = $rf_id){
      $this->title .= '<br/>'.JAM_RF()->name();
      $this->where[]= "ap_rfid = ".JAM::$rf_id;
    }else{
      if (!JAM_posID()) b_debug::internalError('no managedPosition',$this);
      $this->where[] =  "ap_posid = ".JAM_posID();
    }
    if (!JAM_access()->may_seeRevokedApplications()) $this->where[] = "ap_status != ".JAM::status(JAM_ap_revoked);
  }
  
  /*
   *
   */
  function header(){
    if (True)      $header['ap_name']  = 'Applicant';
    if ($this->av) $header['pos_name'] = 'Position';
    else           $header['ref_name'] = 'Referee';
    $header['rec_askdate']= 'Rec.letter<br/>requested';
    $header['ts_nosort']  = 'Referee<br/>replied';
    return $header;  
  }
  
  /*
   *
   */
  function query(){
    $q = myPear_db()->query("SELECT * FROM jam_recommendations ".
			      " LEFT JOIN jam_applicants ON ap_id = rec_apid ".
			      " LEFT JOIN zzz_avatars    ON av_id = rec_avid ".
			      " LEFT JOIN jam_positions  ON ap_posid = pos_id ".
			       " WHERE ".join(' AND ',$this->where)." ORDER BY rec_askdate DESC");
    return $q; 
  }

  /*
   *
   */
  function rejectRecord(){ 
    $this->r  = new bForm_jam_Recommendation($this->rec['rec_id']);
    if ($this->show_letters_for_old_positions){
      $reply = False;  // show all letters, not just those for the considered positiions
    }else{
      if (!$this->r->ap->pos->isConsidered() && !b_checkBox::isOn('see_old'))	$reply = ++$this->nRejected;
      else	$reply = 0;
    }
    return $reply;
  }

  protected function _open(){
    if (is_object(bAuth::$av) && ($this->av->ID != bAuth::$av->ID)){
      myPear::MESSAGE($this->av->getWidget(bText::_('Referee').' '.$this->av->name()));
    }
  }

  protected function _close(){
    if (!$this->show_letters_for_old_positions){
      if ($this->nRejected || b_checkBox::isOn('see_old')){
	print b_checkBox::create(array('see_old'=>bText::_('show the previous letters')));
      }
    }
  }

  /*
   *
   */
  function format() {
    $name = x('i',bForm_Avatar::_fmtName('Lf,s',$this->r->ap->av->formDB)); // span class="bold-text"'
    $this->rec['pos_name']= b_fmt::strcut($this->r->ap->pos->name(),25,'...');
    // Lets do it via ajax, then the "clicks" from the modal will "kill" modal window and transfer to the main one
    $this->rec['ap_name'] = bJS()->modal_ajax("?id=".$this->r->ap->ID."&form=jam_Applicant",$name);
    if ($d=$this->rec['rec_askdate']) $this->rec['rec_askdate'] = '&nbsp;'.x('tt',date('Y-m-d',b_time::txt2unix($d)));
    
    switch (@$this->av->ID > 0) { 
    case True: // List of recommendations for the given referee
      $info = ($this->r->ap->isStatus(JAM_ap_employed) ? bIcons()->get(JAM::status(JAM_ap_employed,'i')) : '');
      $this->t->extraTD[] = $info;
      if ($this->rec['ts_nosort'] = $this->r->isReceived()){
	$this->rec['ts_nosort'] = bIcons()->get('ok');
	if ($this->r->ap->isConsidered() && !$this->r->ap->pos->isStatus(JAM_pos_archived) && $this->r->isWritable()) $action = 'see/update the letter';
	elseif ($this->r->isReadable())$this->t->extraTD[] = b_btn::link('see the letter',$this->r->uploadedFile_url());
      }else{ // the letter is not received
	if    ($this->r->ap->pos->isStatus(JAM_pos_archived)) $this->rec['ts_nosort'] = ''; // bIcons()->get('no');
	elseif($this->r->ap->isStatus(JAM_ap_revoked))       $this->rec['ts_nosort'] = bIcons()->get('trash_cross');
	else                                    $this->rec['ts_nosort'] = b_fmt::redText("not yet");

	if ($this->r->ap->isConsidered(True) && $this->r->isWritable()) $action = 'submit recommendation';
      }
      if (!empty($action)) $this->t->extraTD[] = b_btn::link($action,b_url::same("?form=jam_Recommendation&id=".$this->r->ID));
      break;
      
    case False: // global list of all referees <-> applicanes
      $this->rec['ref_name'] = $this->r->av->name();
    }
  }
}
