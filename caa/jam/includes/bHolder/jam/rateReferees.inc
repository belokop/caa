<?php
/*
 * Quick list of all the referees and applicants for the current position.
 * The forms are not checked, only the applicant status is shown.
 */
locateAndInclude('bUnit_jam');
locateAndInclude('bHolder_jam');
locateAndInclude('bForm_Avatar');
locateAndInclude('bForm_jam_Applicant');
class bHolder_jam_rateReferees extends bHolder_jam{

  public $all_rates  = array(0,1,50,75,95);
//public $all_rates  = array(0,1,50,75,90,100);

  public function __construct($doing=''){
    parent::__construct();
    $this->doing = $doing;
  }

  function drawHeader(){
    switch ($this->doing){
    case 'rate_referees':
      if ($this->listIsClosing){
	parent::drawHeader();
	$this->t->noSort = True;
      }
      break;
      
    default:
      parent::drawHeader();
    }
  }

  function query($av_id=Null){
    $only_current_position = (JAM_posID() > 0);
    switch($this->doing){
    case 'rate_referees':
      if ($only_current_position) $where[] = "ap_posid = ".JAM_pos()->ID;
      else                        $where[] = 1;
      if (!JAM_access()->may_seeRevokedApplications()) $where[] = "ap_status != ".JAM::status(JAM_ap_revoked);

      // Superstition... But necessary...
      foreach(array('av_id'        => 'zzz_avatars',
		    'av_photo'     => 'zzz_avatars',
		    'av_firstname' => 'zzz_avatars',
		    'av_lastname'  => 'zzz_avatars',
		    'av_salutation'=> 'zzz_avatars',
		    'av_email'     => 'zzz_avatars',
		    'av_lastlogin' => 'zzz_avatars',
		    'rec_submitdate'=>'jam_recommendations',
		    'ap_status'     =>'jam_applicants') as $f=>$t) if (myPear_db()->columnExists($f,$t)) $selects[] = $f;
      
      $q = myPear_db()->query("SELECT ".join(',',$selects).
			      " FROM jam_recommendations ".
			      " LEFT JOIN zzz_avatars ON  av_id = rec_avid ".
			      " LEFT JOIN jam_applicants ON ap_id = rec_apid ".
			      " WHERE ". join(' AND ',($where))); 
      break;
      
    default:
      if ($av_id){
	$q = myPear_db()->query("SELECT * FROM zzz_avatars WHERE av_id = '$av_id'");
      }else{
	// Scan all the records for the current position and build list of IDs
	// for all persons which have some relations to this position.
	//
	// Start with the units associated with the position, i.e. secretataries & SC members
	$list = array(0);
	if ($posID=JAM_posID()){
	  foreach (bUnit_jam::getUnits($rank=False,$posID) as $r){
	    foreach (JAM_unit($r['u_rank'])->getUnitMemberIDs() as $av_id)  if ($id=(int)$av_id) $list[] = "av_id=$id";
	  }
	}
	
	// Add applicants & referees
	$where = array("ap_posid=$posID");
	if (!JAM_access()->may_seeRevokedApplications()) $where[] = "ap_status != ".JAM::status(JAM_ap_revoked);
	$q = myPear_db()->query("SELECT ap_avid,rec_avid FROM jam_applicants LEFT JOIN jam_recommendations ON  ap_id = rec_apid  WHERE ".join(' AND ',$where));
	while ($r = myPear_db()->next_record($q)){
	  if ($id=(int)$r['ap_avid'])   $list[] = "av_id=$id";
	  if ($id=(int)$r['rec_avid'])  $list[] = "av_id=$id";
	}
	
	// final query
	$q = myPear_db()->query("SELECT * FROM zzz_avatars WHERE ". join(' OR ',array_unique($list)).
				  " GROUP BY av_id ".
				  " ORDER BY $_GET[sBy] $_GET[sByDir]"); 
      }
    }
    return $q;
  }

  private $av = Null;
  private $av_array = array();
  private $t_count = array();
  private $s_count = array();
  function format() {
    if ($this->rec['ap_status'] == JAM::status(JAM_ap_revoked)){
      myPear::WARNING_T("Revoked application where referee is ".bForm_Avatar::_fmtName('Lf',$this->rec));
      $this->rec = array();
    }else{
      $av_id = $this->rec['av_id'];

      switch($this->doing){

      case 'rate_referees':
	@$this->t_count[$av_id]++;
	if (!empty($this->rec['rec_submitdate'])) @$this->s_count[$av_id]++;
	
	if (isset($this->av_array[$av_id]['rec_submitdate'])) $av_lastlogin = b_time::txt2unix($this->av_array[$av_id]['rec_submitdate']);
	else                                                  $av_lastlogin = '';
	$this->av_array[$av_id] = $this->rec;
	$this->av_array[$av_id]['rec_submitdate'] = max($av_lastlogin,
							b_time::txt2unix($this->av_array[$av_id]['rec_submitdate']));
	$this->rec = array();
	break;
	
      default:
	if (is_object($av = myPear::getInstance('bForm_Avatar_jam',$av_id,array('strict','nocache')))){
	  if (empty($this->rec['pos_nickname'])) $this->rec['pos_nickname'] = @$this->rec['pos_name'];
	  $c = @$GLOBALS['counterS'][$this->rec['ap_id']]++;
	  
	  // Name and the thumbnail
	  $this->rec['av_lastname'] = x("a href=".x("'",$av->sameURL_but()),$av->fmtName('Lf,s'));
	  $this->rec['icon_nosort_afirst'] = $av->getPhoto('','',8);
	  
	  // last login date
	  $ll = max(b_time::txt2unix($this->rec['av_lastlogin']) , b_time::txt2unix($this->rec['rec_submitdate']));
	  if ($ll > 0) $this->rec['av_lastlogin'] = x('tt',date('Y-m-d',$ll));
	  
	  // icons "who is who"
	  $this->t->extraTD = $av->roles();      
	}else{
	  // What is wrong ???
	  if (cnf_dev) if ($this->qqq++ < 4) b_debug::print_r($this->rec,'Why???');
	}  
      }
    }
  }

  function header(){
    $header['icon_nosort_afirst'] = ' ';
    $header['av_lastname']  = 'Name'; 
    
    switch ($this->doing){
    case 'rate_referees':
      $header['av_p'] = ' ';
      $header['av_x'] = ' ';
      $header['rec_submitdate'] = 'Latest<br/>login';
      break;
    default:
      $header['av_lastlogin'] = 'Login';
    }
    return $header;
  }
  
  protected function listEntries_closing() {
    locateAndInclude('b_tabs');
    
    switch($this->doing){
    case 'rate_referees':
      foreach($this->t_count as $av_id=>$c){
	// if (($c == 1)  && ($c == $this->s_count[$av_id])) continue;
	if ($c == 2 && @$this->s_count[$av_id] == 1) $s = $score[$av_id] = 49;
	else                                         $s = $score[$av_id] = (int)(@$this->s_count[$av_id] * 100 / $c);
	foreach($this->all_rates as $r) if ($s >= $r) $rate = $r;
	$table[$rate][$av_id] = $c;
      }

      arsort($score,SORT_NUMERIC);
      krsort($table);
      foreach($table as $rate=>$x) {
	arsort($table[$rate]);
	$n = count($x);
	switch($rate){
	case 100: $tab = "ALL letters replied ($n)"; break;
	case   1: $tab = "Rarely  reply $n";         break;
	case   0: $tab = "Never reply $n";       break;
	default:  $tab = "$rate% replies ($n)";
	}
	$tabs[] = $tab;
      }

      $b_tabs = new b_tabs_JQ($tabs);
      foreach($table as $rate=>$refs) {
	if ($b_tabs->body()){ // start the next tab
	  unset($this->t);
	  $this->drawHeader();
	  $this->t->showLineCounter = False;
	  foreach($refs as $av_id=>$n){
	    $prt = array();
	    $prt['icon_nosort_afirst'] = bForm_Avatar::_getPhoto($this->av_array[$av_id],'','',8);
	    
	    $url = b_url::same("?function=jam_listRecommendations&id=$av_id&auth=no");
	    $prt['av_lastname'] = x("a href='$url'",bForm_Avatar::_fmtName('Lf,s',$this->av_array[$av_id]));
	    
	    if (!($ll = $this->av_array[$av_id]['rec_submitdate']))  $ll = $this->av_array[$av_id]['av_lastlogin'];
	    if ($ll)  $prt['rec_submitdate'] = x('tt',date('Y-m-d',b_time::txt2unix($ll)));
	    
	    $prt['av_x'] = x('tt',(int)@$this->s_count[$av_id]."/".$this->t_count[$av_id]);
	    $prt['av_p'] = x('tt',$score[$av_id].'%');
	    $this->t->prt($prt);
	  }
	  $this->t->close();
	  unset($this->t);
	}
      }
      $b_tabs->close();
      $this->t = new b_table();
      return;

      foreach($score as $av_id=>$c){
	$prt = array();
	if (!($ll = $this->av_array[$av_id]['rec_submitdate']))  $ll = $this->av_array[$av_id]['av_lastlogin'];
	if ($ll)  $prt['rec_submitdate'] = x('tt',date('Y-m-d',b_time::txt2unix($ll)));
	$url = b_url::same("?form=jam_Avatar&id=$av_id&auth=no");
	$prt['av_p'] = x('tt',"$c%");
	$prt['av_x'] = x('tt',(int)$this->s_count[$av_id]."/".$this->t_count[$av_id]);
	$this->t->prt($prt);
      }
      break;
      
    default:
    }
  }
}
