<?php
locateAndInclude('bHolder_vm');
class bHolder_vm_Visits extends bHolder_vm{

  var $Clashes = array();
  var $clashingID  = Null;
  
  function __construct(){ 
    parent::__construct();

    $this->where = array(1);
    if ($this->av_id=@$_GET['av_id'])  $this->where[] = "v_avid = '$this->av_id'";
    if (b_cnf::getValue('showClashingVisits')) myPear::H1(b_fmt::redText("Clashing entries in the database"));
  }

  function header() {
    if ($av = bForm_Avatar::hook($this->av_id))  $this->title = $av->name();
    $header = array('icon_nosort_afirst' => ' ',
		    'v_id'      => 'ID',
		    'name'      => 'who',
		    'v_start'   => "when",
		    'v_host_avid'=> "host",
		    'v_created' => "EDT",
		    'v_type'    => "type",
		    );
    if ($av)      unset($header['name']);
    return $header;
  }

  function rejectRecord(){
    if (b_cnf::getValue('showClashingVisits')){
      $visit = myPear::getInstance_new('bForm_vm_Visit',$this->rec['v_id'],array('strict','fatal'));
      $this->clashingID = $visit->getClashingVisits_byDate($this->rec['v_start'],$this->rec['v_end']);
      return (!empty($this->clashingID));
    }
  }
  
  function query() {
    $h = $this->header();
    if (empty($h[@$_GET['sBy']])) { $_GET['sByDir']='DESC'; $_GET['sBy']   ='v_start'; }
    if (empty(    $_GET['sByDir'])) $_GET['sByDir']='DESC';
    $q = myPear_db()->query("SELECT *,av_lastname AS name FROM abs_visits ".
			    " LEFT JOIN zzz_avatars ON av_id = v_avid ".
			    " LEFT JOIN abs_leases ON v_id=lease_vid ".
			    " LEFT JOIN abs_rooms ON a_id=lease_aid ".
			    " WHERE ".join(' AND ',$this->where). 
			    " GROUP BY v_id ".
			    " ORDER BY $_GET[sBy] $_GET[sByDir] ");
    return $q;
  }
  
  function format() {
    $url = b_url::same("?form=vm_Visit&mode=RO&id=".$this->rec['v_id']);
    $this->rec['name'] = bForm_Avatar::_fmtName('Lf',$this->rec);
    if ($c=$this->clashingID) {
      $visit = myPear::getInstance_new('bForm_vm_Visit',$this->rec['v_id'],array('strict','fatal'));
      $this->Clashes[$visit->ID] = $c;
      $exp = $visit->getExp();
      if ($visit->exp_arePaid() && (int)$exp->ID) $exp->unlock(); // reopen the clashing object
      $url = b_url::same("?form=vm_Visit&mode=RO&id=".$this->rec['v_id']);
    }
    foreach ($this->Clashes as $pear=>$cl) if (in_array($visit->ID,$cl) || $pear==$visit->ID) {
      $this->t->extraTD[] = bIcons()->getButton(array('l'=>$url,
						      'd'=>'edit clash', 
						      'i'=>'i-edit'));
      $this->t->extraTD[] = b_btn::submit_icon('i-drop',
					       txt_deleteVisit,
					       b_url::same("?action_once=".txt_deleteVisit."&function=vm_edit_accommodation&resetcache_once=1&v_id=$visit->ID"));
    }
    $this->rec['v_type'] = bIcons()->get($this->rec['v_type']); 
    $this->rec['v_start']   = x("a href='$url'",b_time::period($this->rec['v_start'],$this->rec['v_end']));
    
    $this->rec['icon_nosort_afirst'] = bForm_Avatar::_getThumbnail($this->rec);
    $av_host = bForm_Avatar::hook($this->rec['v_host_avid']);
    $this->rec['v_host_avid'] = x('i',($av_host ? $av_host->fmtName('fl') : ''));
  }
}
