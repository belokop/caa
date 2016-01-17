<?php
/*
 * Quick list of all the referees and applicants for the current position.
 * The forms are not checked, only the applicant status is shown.
 */
locateAndInclude('bHolder_jam');
locateAndInclude('bForm_Avatar');
locateAndInclude('bForm_jam_Applicant');
locateAndInclude('bForm_jam_Recommendation');
class bHolder_jam_Referees extends bHolder_jam{

  var $sortBy_default = 'ap_id';
  private $ap = Null;

  public function __construct(){
    parent::__construct();
    if (JAM_pos()){
      $this->where[] = "ap_posid = ".JAM_posID();
      if (!JAM_access()->may_seeRevokedApplications()) $this->where[] = "ap_status != ".JAM::status(JAM_ap_revoked);
    }else{
      myPear::WARNING('No position selected');
      $this->where[] = 0;
    }
  }

  function header(){
    $header = array();
    $header['ap_id']     = 'Applicant';
    $header['ref_name']  = 'Referee'; 
    if (@$_GET['sBy']   != 'ref_name')    $header['ap_submitdate']  = 'Applied';
    $header['ts_nosort'] = 'Letter sent';
    return $header;
  }

  /*
   *
   */
  function query(){
    if (empty($_GET['sBy'])) $_GET['sBy'] = 'ap_id';
    if ( $_GET['sBy']=='ap_id') $_GET['sByDir'] = 'DESC';

    $tmp = array("ap_id AS t_apid",
		 "av_id AS t_avid");
    foreach(array('salutation',
		  'firstname',
		  'lastname') as $t) $tmp[] = "av_$t AS t_$t";
    
    myPear_db()->qquery("CREATE TEMPORARY TABLE tmp_avatars ENGINE=MEMORY AS ".
			  " (select ".join(',',$tmp)." FROM jam_applicants ".
			  " LEFT JOIN zzz_avatars on ap_avid=av_id ".
			  " WHERE ".join(' AND ',$this->where).")",False);
    return myPear_db()->qquery("SELECT *, av_lastname AS ref_name ".
				 " FROM jam_recommendations ".
				 " LEFT JOIN zzz_avatars    ON  av_id = rec_avid ".
				 " LEFT JOIN jam_applicants ON  ap_id = rec_apid ".
				 " LEFT JOIN tmp_avatars    ON t_apid = rec_apid ".
				 " WHERE ".join(' AND ',$this->where).
				 " ORDER BY $_GET[sBy] $_GET[sByDir]",False); 
  }
  
  function rejectRecord(){
    return empty($this->rec['ap_id']);
  }
  
  /*
   *
   */
  function format() {
    static $dejaVu = array();

    $this->t->css['ts_nosort']['align'] = 'align_right';

    foreach(array('salutation',
		  'firstname',
		  'lastname') as $t) $ap["av_$t"] = $this->rec["t_$t"];
    if (0)    if (!@$GLOBALS['hgjhgjhg']++){
	b_debug::print_r($this->rec);
	b_debug::print_r($ap);
      }
    
    $this->rec['ts_nosort'] = (($s=$this->rec['rec_submitdate']) ? b_time::short_date($s) : b_fmt::redText("not yet"));
    if (empty($this->rec['pos_nickname'])) $this->rec['pos_nickname'] = @$this->rec['pos_name'];
    
    $this->rec['ref_name'] = x("a href='".b_url::same("?function=jam_listRecommendations&id=".$this->rec['av_id'])."'",bForm_Avatar::_fmtName('Lf,s',$this->rec));
    $this->rec['ap_id']    = x("a href='".b_url::same("?form=jam_Applicant&id=".$this->rec['ap_id'])."'",bForm_Avatar::_fmtName('Lf,s',$ap));
    $this->rec['ap_submitdate'] = ($sd=$this->rec['ap_submitdate']) ? b_time::short_date($sd) : b_fmt::redText('???');
    if (@$dejaVu[$this->rec['ap_id']]++){
      unset($this->rec['ap_submitdate']);
    }
    if (!@$this->dejaVuSt[$this->rec['ap_avid']]++ && !myPear::is_empty($status=$this->rec['ap_status'])){
      $this->t->extraTD = bForm_jam::_allStatusIcons($this->rec,'ap_status');
    }
  }
}
