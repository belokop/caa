<?php
/*
 * JAM avatar
 */
//locateAndInclude('bForm_jam_Position');

locateAndInclude('bForm_Avatar');
class bForm_Avatar_jam extends bForm_Avatar{

  public static $_debug = False;

  var $tips  = array('av_sex' => 'Used only for statistics',
		     );
  
  /*
   * Helper to bForm_Avatar::delete
   */
  public static function delete_deps($where){
    b_debug::xxx($where);
    $n = 0;      
    foreach(array('_old','') as $old){
      $avatars = (empty($old) ? 'zzz_avatars' : 'jam_old_avatars');
      if (!myPear_db()->tableExists($avatars)) continue;
      $q = myPear_db()->qquery("SELECT av_id,".join(',',bForm_Avatar::$vitalFields)." FROM $avatars WHERE $where",True);
      while ($r = myPear_db()->next_record($q)){
	// Exclude "zzz_avatars", that will be done by the "parent" function
	$table_affected = array($avatars  => 'av_id',
				"zzz${old}_unit_members"=>'um_avid',
				"jam${old}_reviews"     =>'r_avid',
				"jam${old}_recommendations" => 'rec_avid');
	unset($table_affected['zzz_avatars']);
	if ($avatars !== 'zzz_avatars') MSG::MESSAGE_forced(sprintf(__METHOD__."(AV %s) <em>%d</em>",self::_fmtName('Lf',$r),(++$n)));
	
	// Delete the dependencies 
	foreach($table_affected  as $table=>$id){
	  if(!myPear_db()->tableExists($table)) continue;
	  myPear_db()->qquery("DELETE FROM $table WHERE $id = $r[av_id]",True);
	}
      }
    }

    // Delete aplicants if applicable
    if ((strpos($where,'av_lastname') !== False) || (strpos($where,'av_email') !== False)){
      // Get search for the applicants field
      foreach(explode(' AND ',str_replace('av_email','ap_email',
					  str_replace('av_lastname','ap_lastname',
						      str_replace('av_firstname','ap_firstname',$where)))) as $i){
	if (strpos($i,'av_institute') === False) $w[] = $i;
      }
      $where = join(' AND ',$w);

      foreach(array('_old','') as $old){
	$avatars = (empty($old) ? 'zzz_avatars' : 'jam_old_avatars');
	if (!myPear_db()->tableExists($avatars)) continue;
	if (!myPear_db()->tableExists("jam${old}_applicants")) continue;
	if (!myPear_db()->columnExists("ap_firstname","jam${old}_applicants")) continue;
	if (!myPear_db()->columnExists("ap_email",    "jam${old}_applicants")) continue;

	$query = "SELECT ap_id,ap_avid,ap_firstname,ap_lastname FROM jam${old}_applicants WHERE $where";
	$qq = myPear_db()->qquery($query,True);
	while ($r = myPear_db()->next_record($qq)){
	  MSG::MESSAGE_forced(sprintf(__METHOD__."(AP %s %s) <em>%d</em>",strToUpper($r['ap_lastname']),$r['ap_firstname'],(++$n)));
	}
	$qq = myPear_db()->qquery($query,True);
	while ($r = myPear_db()->next_record($qq)){
	  myPear_db()->qquery($q="DELETE FROM $avatars WHERE av_id = '$r[ap_avid]'",True);
	  myPear_db()->qquery($q="DELETE FROM jam${old}_recommendations WHERE rec_apid = $r[ap_id]",True);
	  myPear_db()->qquery($q="DELETE FROM jam${old}_reviews         WHERE   r_apid = $r[ap_id]",True);
	}
	myPear_db()->qquery($q="DELETE FROM jam${old}_applicants      WHERE   $where",True);
      }
    }
  }
  

  /*
   * Auxiliary function to 'findDuplicates'
   *
   * Reject the candidate for merging if he is unknown to JAM
   */
  public static function findDuplicates_reject($r){
    $reply = True;
    foreach($r as $key=>$value) $where[] = myPear_db()->quote($value, 'R', $key);
    $q = myPear_db()->qquery("SELECT ap_avid,rec_avid  FROM zzz_avatars ".
                               " LEFT JOIN jam_applicants      ON  ap_avid = av_id ".
                               " LEFT JOIN jam_recommendations ON rec_avid = av_id ".
                               " WHERE ".join(' AND ',$where),cnf_show_mrg);
    while($r = myPear_db()->next_record($q)){
      if (!empty($r['ap_avid']) || !empty($r['rec_avid'])){
        $reply = False;
        break;
      }
    }
    return $reply;
  }
  
  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
  }

  /*
   *
   */
  function defineVariables(){
    parent::defineVariables();  

    $this->defineVariable(array('av_http'=>($this->MODE == 'RO' ? ' ' : 'Personal web page'),
				),True);

  }


  /*
   * Personal information
   */
  protected function getFormBody(){

    //    if ($this->MODE != 'RW') 
    $this->tips = array();
 
    // Placing the definition below into "function defineVariables()" creates problems,
    // since "isApplicant()" is not yet ready before the constructor finishes his job 
    // ("defineVariables" is called within the constructor).
    // So, lets do it here, since we anyway need "isVITAL" in the interactive mode only.
    $isApplicant = (count($this->isApplicant()) > 0);    
    $this->defineTypes(array('isVITAL'=>array('av_birthdate'   =>$isApplicant,
					      //'av_salutation'  =>$isApplicant && $this->askSalutation(),
					      'av_residentship'=>$isApplicant,
					      'av_citizenship' =>$isApplicant,
					      //'av_birthcountry'=>$isApplicant,
					      ),
			     ));
    $this->avInfoBlock();
    $this->studyBlock();
    $this->residentshipBlock();
  }

  /*
   *
   */
  function role($asIcons=False){
    $reply = $role = $roleIcon = array();
    //if (JAM_access()->getRank() > RANK_jam_referee){
    if (bAuth::authenticated()){
      if ($this->isSuperUser())  $role[] = RANK__superuser;
      if ($this->isApplicant())  $role[] = RANK_jam_applicant;
      if ($this->isReferee())    $role[] = RANK_jam_referee;
      if ($this->isSecretary())  $role[] = RANK_jam_secr;
      foreach(JAM::$SC_ranks as $r) if ($this->isSCmember(Null,$r)) $role[] = $r;
      if ($asIcons){
	foreach($role as $r) $roleIcon[] = bIcons()->get($r);
	$reply = $roleIcon;
      }else{
	$reply = $role;
      }
      $this->dbg(join(', ',$reply));;
    }
    return $reply;
  }
  
  /**
   * list positions where the avatar is either a referee or an applicant
   */
  function positionsConserned($asReferee=False){
    $list = array();
    if ((int)$this->ID && myPear_db()->tableExists('jam_recommendations')){
      $q = myPear_db()->query(($asReferee
				  ?"SELECT ap_posid,ap_id FROM jam_recommendations ".
				  " LEFT JOIN jam_applicants ON ap_id = rec_apid WHERE rec_avid = ".$this->ID." ORDER BY ap_posid"
				  :"SELECT ap_posid,ap_id FROM jam_applicants ".
				  " LEFT JOIN jam_positions ON pos_id = ap_posid ".
				 " WHERE ap_avid  = ".$this->ID." ORDER BY pos_deadline DESC"));
      while($r = myPear_db()->next_record($q)) $list[$r['ap_posid']] = $r['ap_id'];
      $this->dbg('positionsConserned',join(' ',$list));
    }
    return $list;
  }
  
  /*
   * get list of positions where the avatar is a referee
   */
  function getPos($asReferee=True){
    $reply = array();
    foreach ($this->positionsConserned($asReferee) as $pos_id=>$ap_id) $reply[] = $pos_id;
    return $reply;
  }
  
  protected function needFullInformation(){ 
    if ($pos_id=JAM_posID()) return count($this->isApplicant($pos_id)); 
    return parent::needFullInformation();
  }
  
  /*
   * Show MS record for applicants if the position require it
   */
  function askMS(){
    return $this->ask_PhD_MS('MS') || $this->askPhD();
  }
  
  /*
   * Show PhD record for applicants if the position require it
   */
  function askPhD(){
    return $this->ask_PhD_MS('PhD');
  }

  /*
   * Ask salutation from the PhD 
   */
  protected function askSalutation()  { 
    return $this->askPhD();
  }

  /*
   * Helper for askPhD / askMS
   * Loop thru all the applications (i.e. positions applied) and check the requirements,
   * show the highest level required 
   */
  private $ask_PhD_MS_cache = array();
  private function ask_PhD_MS($what='PhD'){
    
    if ($this->isReady() && !isset($this->ask_PhD_MS_cache[$what])){
      $reply = False;
      foreach($this->isApplicant(0,0,1) as $pos_id){
	$pos = loader::getInstance('bForm_jam_Position',$pos_id,'fatal');
	$r_p = $pos->require_PhD();
	$r_m = $pos->require_MS();	  
	$r = (($what == 'PhD') ? $r_p : $r_m);
	if ($r) $reply = True;
      }
      $this->ask_PhD_MS_cache[$what] = $reply;
    }
    $this->dbg(@$this->ask_PhD_MS_cache[$what]);
    return @$this->ask_PhD_MS_cache[$what];
  }
  
  /**
   * Check that the person is an applicant for a position
   *
   * @param $pos_id (optional)
   *   If present, only this position is checked.
   *   If empty (default), check that the person ever acted as an applicant
   *
   * @return
   *   Array(app_id => ap_status) 
   */
  function isApplicant($pos_id=False,$onlyActive=False,$return_pos_ids=False) {
    $reply = bForm_jam_Applicant::get_apInfo($this->ID,$pos_id,$onlyActive,!$return_pos_ids);
    if ($return_pos_ids)  $reply = array_keys($reply);
    return $reply;
  }

  /*
   *
   */
  function isSecretary($pos_id=null) {
    $reply = False;
    if (empty($pos_id))    $pos_id = JAM_posID();
    if(!empty($pos_id) && JAM_unit(RANK_jam_secr,$pos_id)->isMember($this->ID)) $reply = RANK_jam_secr;
    if (!empty($reply)) $this->dbg($reply,(bool)self::$_debug||cnf_show_a);
    return $reply;
  }                                                                                                                                                                                              
  
  /**
   * Check that the person is a member of a search committee
   *
   * @param $pos_id (optional)
   *   If present, only this position is checked.
   *   If empty (default) check that the person ever acted as a SC member
   *
   * @return array(rank => pos_id)
   */
  function isSCmember($pos_id=False,$ranks=Null) {
    if (empty($pos_id)) $positions = array_keys(bForm_jam_Position::getPositions());
    else                $positions = array($pos_id);
    
    if (empty($ranks))  $ranks = JAM::$SC_ranks;
    else                $ranks = array($ranks);
    
    $reply = array();
    locateAndInclude('bUnit_jam');
    foreach ($ranks as $rank){
      foreach($positions as $pos_id){
	if ($pos_id instanceof bForm_jam_Position) { $pos = $pos_id; $pos_id = $pos->ID; }
	else    $pos = loader::getInstance('bForm_jam_Position',$pos_id);
	if ($pos->isConsidered()){
	  foreach (bUnit_jam::getUnits($rank,$pos_id) as $r){
	    if (JAM_unit($rank,$pos_id)->isMember($this->ID)) {
	      $reply[$rank] = $pos_id;
	    }
	  }
	}
      }
      krsort($reply);
    }
    if (!empty($reply)) $this->dbg((empty($reply) ? False : join(', ',array_keys($reply))),(bool)self::$_debug||cnf_show_a);
    return $reply;
  }
  
  /**
   * Check that the person is a referee for the given position
   *
   * @param $pos_id (optional)
   *   If present, only this position is checked.
   *   If empty (default) check that the person ever acted as a referee
   *
   * @return array of the rec_id's
   */
  function isReferee($pos_id=0) {
    locateAndInclude('bForm_jam_Recommendation');
    $rec_ids = bForm_jam_Recommendation::getRecommendations($pos_id, $this->ID);
    if (!empty($rec_ids)) $this->dbg(join(', ',$rec_ids),(bool)self::$_debug||cnf_show_a);
    $reply = !empty($rec_ids);
    return $reply;
  }
  
  /*
   * Might be useful one day...
   */
  function get_all_uploads(){
    locateAndInclude('jam_Document');
    jam_Document::avatar_docs($this->ID);
  }
}
