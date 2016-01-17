<?php
locateAndInclude('bHolder');
class bHolder_jam extends bHolder{

  var $t = Null;
  var $sortBy_default = 'av_lastname';
  protected $listIsClosing = False;
  protected $pos = Null;
  public $show_revoked = False;
  
  public function __construct(){
    parent::__construct();
  }
  
  /*
   * Reject the revoked applications
   */
  function rejectRecord(){ 
    if        (cnf_show_revoked) $reply = False;
    elseif ($this->show_revoked) $reply = False;
    elseif(bForm_jam_Applicant::_isStatus(JAM_ap_revoked,$this->rec)) $reply = !JAM_access()->may_seeRevokedApplications();
    else                  $reply = False; 
    return $reply; 
  }
  
  private $dejaVulistEntries = 0;
  function listEntries(){
    if ($n=$this->dejaVulistEntries++) myPear::WARNING_T("$n-th entry to ".$this."->listEntries");
    $this->_open();
    
    $header = $this->header();  // sets also title
    if (!isset($_GET['sBy']) || empty($header[$_GET['sBy']])){
      if(isset($_GET['sBy'])) myPear::WARNING_T("sBy: \"$_GET[sBy]\" ==> \"$this->sortBy_default\"");
      $_GET['sBy'] = $this->sortBy_default;
    }
    
    $nLine = 0;
    if ($q = $this->query()){
      while ($this->rec = myPear_db()->next_record($q)){
	if (!$this->rejectRecord()){
	  $this->drawHeader();
	  $this->format();
	  if ($this->t instanceof b_table)	  $this->t->prt($this->rec);    
	  if (++$nLine > $this->maxLines){
	    print b_fmt::redText($this->maxLines." lines printed... Output stops");
	    break;
	  }
	}
      }
    }
    if ($nLine){
      $this->listIsClosing = True;
      $this->listEntries_closing();
      $this->t->close();
      $this->_close();
      bIcons()->explain();
    }else{
      if (!empty($q) && myPear_db()->num_rows($q)) $this->noSuccess();
      else                                         $this->_emptySelection();
      $this->_close();
    }
  }
  
  protected function listEntries_closing() {}

  function getAllPositions($org_id,$IDonly=False, $onlyConsidered=False){
    $q = myPear_db()->query("SELECT pos_id FROM jam_positions ".
			    " LEFT JOIN zzz_organizations ON pos_orgid = org_id ".
			    " WHERE org_id = '$org_id'".
			    " ORDER BY pos_deadline DESC");
    $list = array();
    while ($r = myPear_db()->next_record($q)){ 
      $pos = myPear::getInstance('bForm_jam_Position',$r['pos_id']);
      if (!$onlyConsidered || $pos->isConsidered()){
	if ($IDonly) $list[] =  $pos->ID;
	else         $list[] =  $pos;
      }
    }
    return $list;
  }

  function format() {}

}
