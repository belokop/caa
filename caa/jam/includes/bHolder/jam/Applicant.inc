<?php
/*
 * Show lists of applicants
 */
locateAndInclude('bHolder_jam');
locateAndInclude('APIaccess_jam');
locateAndInclude('bForm_jam_Review');
locateAndInclude('bForm_jam_Position');
locateAndInclude('bForm_jam_Applicant');
locateAndInclude('bForm_jam_RF');
class bHolder_jam_Applicant extends bHolder_jam {
  
  var $debug = True;
  private $av_id = Null;
  private $ap_ids = array();

  /*
   * Constructor, initializes the applications lists. 
   *
   * The class "knows" the following flavors of presenting list of applications:
   * - (if $_GET['av_id'] is set) - list of applications from the avatar $av_id
   * - (if the argument $doing is_numeric) - list of applicants for the research field $rf_id=$doing 
   * - (if the argument $doing is_array) - list applications from the input array
   */
  public function __construct($doing=0){

    // Process "commands" which came from the previous "click"
    if ($ap_id = @$_GET['revoke_once']){
      // Revoke application
      $ap = new bForm_jam_Applicant($ap_id);
      $ap->setStatus(JAM_ap_revoked);
      $this->show_revoked = True;
    }

    // See What we are expected to do
    if ($av_id = @$_GET['av_id']){
      // Request to show list of applications from the avatar $this->av_id
      $this->av_id = $av_id;
    }elseif(is_array($doing)){
      // Request to show just those applications
      $this->ap_ids = $doing;
      $this->show_revoked = True;
    }elseif((is_numeric($doing) || empty($doing)) && JAM_posID()){
      // Request to list all applications for the research field $rf_id
      $rf_id = ((count($rfs = JAM_pos()->getRF_id()) == 1) 
		? $rfs[0]
		: $doing);
      if (!empty($rf_id)){
	JAM_RF($rf_id);
	parent::__construct();
	if (count($rfs) ==1){
	  locateAndInclude('b_tabs');
	  $t = new b_tabs_myPear(array('Applicants','Summary'));
	  if($t->body())	  $this->listEntries();
	  if($t->body())	  $this->showSummary($rf_id);
	  $t->close();
	}else{
	  $this->listEntries();
	}
      }else{ // print a summary
	parent::__construct();
	$this->showSummary(JAM_posID());
      }
    }
    b_url::setReferrer();
  }
  
  function drawHeader(){
    if ($this->listIsClosing)      parent::drawHeader();
  }

  /* 
   * Show the summary information:
   * - number of applicants, 
   * - expected number of recommendation letters,
   * - number of reviews written,
   * - etc.
   */
  function showSummary($id){
    
    $ngot = $napp = $nrec = $nrev = $totalRevies = array();
    if (JAM_posID()){ // protect against broken caches in CMS
      foreach (JAM_pos()->getRF_id() as $rf_id){
	$rf = new bForm_jam_RF($rf_id);
	$name = $rf->name();
	$where = array("ap_rfid = $rf_id",
		       "ap_avid > 0",
		       "ap_status != ".JAM::status(JAM_ap_revoked));
	$q = myPear_db()->query("SELECT ap_id,rec_id,rec_submitdate FROM jam_applicants ".
				" LEFT JOIN jam_recommendations ON ap_id=rec_apid WHERE ".join(' AND ',$where)." ORDER BY rec_apid");
	
	while ($r=myPear_db()->next_record($q)){
	  if ($id=@$r['rec_id']){
	    if (empty($napp[$name][$r['ap_id']]))   $napp[$name][$r['ap_id']] = 0;
	    if (empty($nrec[$name][$r['rec_id']]))  $nrec[$name][$r['rec_id']]= 0;
	    $napp[$name][$r['ap_id']]++;
	    $nrec[$name][$r['rec_id']]++;
	    if (!empty($r['rec_submitdate'])){
	      if (empty($ngot[$name][$r['rec_id']]))  $ngot[$name][$r['rec_id']]= 0;
	      $ngot[$name][$r['rec_id']]++;
	    }
	  }else{
	    myPear::WARNING_T("??? empty rec_id for ap_id=$r[ap_id]");
	    // b_debug::print_r($r,$r['ap_id']);
	  }
	}
	
	$q = myPear_db()->query("SELECT COUNT(r_apid) AS n FROM jam_applicants ".
				" LEFT JOIN jam_reviews ON ap_id=r_apid WHERE ".join(' AND ',$where).
				" GROUP BY r_apid ");
	while ($r=myPear_db()->next_record($q)){
	  if ($n = (int)$r['n']) @$totalRevies[$name] += $n;
	  if (empty($nrev[$name][(int)$r['n']])) $nrev[$name][(int)$r['n']] = 0;
	  $nrev[$name][(int)$r['n']]++;
	}
      }
    }
    
    /*
     * Print the summary
     */
    $max_reviews = 0;
    foreach ($nrev as $name=>$stat) foreach($stat as $n=>$v) $max_reviews = max($max_reviews,$n);
    
    $header = array('f' => '',
		    'a' => '<br/>applicants',
		    'r' => 'got rec.<br/>letters',
		    'm' => 'miss rec.<br/>letters',
		    'sp'=> '&nbsp;&nbsp;',
		    'rw'=> 'got<br/>reviews');
    for($n=0; $n<=$max_reviews; $n++){
      switch($n){
      case 0:  $t = "&nbsp;&nbsp;&nbsp;miss<br/>reviews"; break;
      case 1:  $t = "&nbsp;1<br/>review";    break;
      default: $t = "&nbsp;$n<br/>reviews";
      }
      $header["x$n"] = $t;
    }
    
    $t = new b_table_zebra($header,'cellpadding=3');
    foreach($header as $k=>$v) $t->th_attr[$k] = "class='align_right'";
    $t->noSort = True;
    $t->headerAlignRight = True;
    $t->css['r']['align'] = 'align_right';
    
    $total = array('f'=>'','a'=>0,'r'=>0,'m'=>0,'rw'=>0,'x0'=>0);
    arsort($napp);
    foreach ($napp as $name=>$apps){
      $total['a'] += count($napp[$name]); 
      $total['r'] += count($ngot[$name]);
      $total['m'] += count($nrec[$name])-count($ngot[$name]);
      $total['rw']+= @$totalRevies[$name];
      $rec = array('f' =>$name,
		   'a' =>x('strong',count($apps)),
		   'r' =>count($ngot[$name]).' '.x('em',(int)(100*count($ngot[$name])/count($nrec[$name])).'%'),
		   'm' =>count($nrec[$name])-count($ngot[$name]),
		   'rw'=>(int)@$totalRevies[$name],
		   'sp'=>'&nbsp;&nbsp;&nbsp;',
		   'x0'=> count($apps),
		   );
      for($n=1; $n<=$max_reviews; $n++){
	  if ($nr = @$nrev[$name][$n]){
	    $rec["x0"] -= $nr;
	    $rec["x$n"] = $nr;
	    @$total["x$n"] += $rec["x$n"];
	  }
      }
      $total["x0"]  += $rec["x0"];
      if ($n=$rec["x0"])$rec["x0"] = b_fmt::redText($n);
      else              $rec["x0"] = '';
      $t->prt($rec);
    }
    if (!$total["x0"]) $total["x0"] = '';
    else               $total["x0"] = b_fmt::redText($total["x0"]);
    $total['r'] .= ' '.x('em',(int)((100*$total['r']/(max(1,$total['r']+$total['m'])))).'%');
    $total['a']  = x('strong',$total['a']);
    $t->tr();
    if (count($napp)>1) $t->prt($total);
    $t->close();
  }
  
  /*
   * Build the sql query
   */
  function query(){

    $where = array();
    if ($this->av_id){
      // Applications list for the given person
      $where[] = " ( av_id = ".$this->av_id.' ) ';
      $orderBy = 'pos_deadline DESC';
    }elseif($this->ap_ids){
      // List of bogus applications
      foreach($this->ap_ids as $ap_id)	$w[] = " ( ap_id = '$ap_id' ) ";
      $where[] = ' ( ' . join(' OR ',$w) . ' ) ';
      $where[] = " ( pos_orgid = '".myOrg_ID."' ) ";
      $orderBy = 'pos_deadline DESC,av_lastname ASC,ap_status ASC';
    }else{
      // Normal list
      $where[] = " ( ap_posid = '".JAM_posID()."' ) ";
      $where[] = " ( ap_rfid  = '".JAM::$rf_id."' ) ";
      if (isset($_GET['sBy'])) $orderBy = "$_GET[sBy] ".@$_GET['sByDir'];
      else                     $orderBy = 'av_lastname ASC';
    }
    
    $cmd = "SELECT *  FROM jam_applicants ".
      " LEFT JOIN zzz_avatars         ON  ap_avid=av_id ".
      " LEFT JOIN jam_positions       ON  ap_posid=pos_id ".
      " LEFT JOIN jam_recommendations ON  ap_id=rec_apid ".
      " WHERE ".join(' AND ',$where).
      " ORDER BY $orderBy";

    return myPear_db()->qquery($cmd,cnf_dev);
  }
  
  protected function _open(){
    if ($this->av_id){
      $av = myPear::getInstance('bForm_Avatar_jam',$this->av_id);
      myPear::MESSAGE($av->getWidget(bText::_('Applicant').' '.$av->name()));
    }
  }

  /*
   *
   */
  private $show_rating = False;
  function header(){

    $JAM_posID = JAM_posID();

    $header = array();
    if ($this->av_id){
      $JAM_posID = 0;
      $header['_pos_deadline'] = ' ';
      $header['_pos_name'] = 'Position';
    }elseif($JAM_posID){
      $header['_ap_rating']   = 'Rating';	      
      $header['_av_lastname'] = 'Applicant';
      $this->title = JAM_pos($JAM_posID)->name();
      if (JAM_pos($JAM_posID)->numberOfReseachFields() > 1){
	if (JAM_RF())                      $this->title = JAM_RF()->name();
      }
    }
    
    if($JAM_posID){
      if (JAM_pos($JAM_posID)->require_PhD()) {
	$header['_av_phd_year'] = 'PhD';
	$header['_av_phd_institute'] = ' ';
      }
      if ((int)JAM_pos($JAM_posID)->formDB['pos_n_referees']>0) $header['n_nosort']    = 'Rec.<br/>letters';
    }

    //
    // Sort the applicants by rating and decide should the ratings be shown or not
    //
    if ($this->listIsClosing){

      // Sort applicants  by rating
      // Be sure that all the reviewers open their reviews for reading

      krsort($this->applicants_array_rating);
      $all_reviewers_readable = True;
      if ($JAM_posID){
	foreach(array(RANK_jam_SC1,RANK_jam_SC2) as $rank){
	  foreach (JAM_unit($rank,$JAM_posID)->getUnitMemberIDs(JAM::status(JAM_review_private)) as $av_id){
	    if ($av_id == bAuth::$av->ID) continue;
	    $n_reviews = 0;
	    foreach($this->applicants_array_rating as $rating=>$applicants){
	      foreach(array_unique($applicants) as $ap_id){
		$n_reviews += bForm_jam_Review::getReviews('count', $av_id, $ap_id);
	      }
	    }
	    if ($n_reviews > 0){
	      if (!JAM_access()->may_seePrivateReviews($av_id))    $all_reviewers_readable = False;
	      $rec = JAM_unit($rank,$JAM_posID)->getUnitMember($av_id);
	      $name = bForm_Avatar::_fmtName('sfl',$rec); 
	      myPear::WARNING("The $n_reviews reviews of <em>$name</em> are private and not readable yet");
	    }
	  }
	}
      }

      // Suppress the "ratings" column in the final table if there are unreadable private ratings 

      $all_ratings = array_keys($this->applicants_array_rating);
      $this->show_rating = $all_reviewers_readable && ((count($all_ratings) > 2) || ((int)$all_ratings[0] > 0));

      if ($this->show_rating){
	  $_GET['sBy']   = 'ap_rating';
	  $_GET['sByDir']= 'DESC';
      }else{
       	unset($header['_ap_rating']);
      }
    }

    if (empty($header)){
      $header['_av_lastname'] = 'Applicant';
      $header['_av_ms_year']  = 'MS';
      $header['_av_phd_year'] = 'PhD';
      $header['_av_phd_institute'] = ' ';
      $header['n_nosort']    = 'Rec.<br/>letters';
   }
    return $header;
  }
  
  /**
   * Collect applicants into an array.
   * Print it later when all the info is collected, see function "listEntries_closing".
   */
  private $applicants_array = array();
  private $applicants_array_rating = array();
  function format() {

    $ap_id = $this->rec['ap_id'];
    @$this->applicants_array[$ap_id]['rec_nt']++;
    if (!empty($this->rec['rec_letter_file'])) @$this->applicants_array[$ap_id]['rec_ns']++;
    foreach(array_keys($this->rec) as $k){
      $this->applicants_array[$ap_id][$k] = $this->rec[$k];
    }

    // Save the records by rating
    if ($ignore_rating = (!empty($this->ap_ids) || !empty($this->av_id))){
      $this->applicants_array_rating[$this->rec['pos_deadline']][] = $ap_id;
    }else{
      $this->applicants_array_rating[$this->rec['ap_rating']][] = $ap_id;
    }
    $this->dbg("ignore_rating = ".var_export($ignore_rating,True));
    $this->rec = array();
  }

  /**
   * Print the applicants list from the collected arrays
   */
  protected function listEntries_closing() {

    $this->drawHeader();
    $this->t->css['_ap_rating']['align'] = 'align_right';
    $this->t->showLineCounter = !$this->show_rating;      

    // Show the rated list
    foreach($this->applicants_array_rating as $rating=>$applicants){
      foreach(array_unique($applicants) as $ap_id){
	$rec = $this->applicants_array[$ap_id];

	static $pos_id = 0,$av_id=0;
	if (!empty($this->ap_ids)){
	  $this->t->ignoreColor_for_changes = True;
	  if ($rec['pos_id'] != $pos_id){
	    $this->t->reset_changes_cache();
	    $this->t->tr(x('strong',$rec['pos_name']),'colspan=6');
	    $pos_id = $rec['pos_id'];
	  }elseif ($rec['ap_avid'] != $av_id){
	    $this->t->tr();
	    $av_id = $rec['ap_avid'];
	  }
	}
	
	$av_lastname = bForm_Avatar::_fmtName('Lf',$rec);
	// Do "modal view" via ajax, then the "clicks" to the other pages will be properly handled,
	// i.e. be shown in the "normal" view.
	if($this->ap_ids)$rec['_av_lastname']= bJS()->modal_ajax("?form=jam_Applicant&id=0&av_id=0&ap_id=$ap_id&id=$ap_id", $av_lastname);
	else             $rec['_av_lastname']= x("a href=".x("'",b_url::same("?form=jam_Applicant&id=0&av_id=0&ap_id=$ap_id&id=$ap_id")),$av_lastname);

	if (!is_object($this->pos) || ($this->pos->ID != $rec['pos_id'])) $this->pos = myPear::getInstance('bForm_jam_Position',$rec['pos_id']);
	if ($this->av_id && $this->pos) $rec['pos_logo'] = $this->pos->getLogo();

	// Collect the application status as icons
	foreach(bForm_jam::_allStatusIcons($rec,'ap_status') as $icon){
	  $this->t->extraTD[] = $icon;
	}
	
	$rec['_pos_name'] = b_btn::a(b_fmt::escape(b_fmt::strcut($rec['pos_name'],30)),
				    "?form=jam_Applicant&id=$rec[ap_id]"); 
	
	$rec['_av_phd_institute'] = $rec['av_phd_institute'];
	if (!is_numeric($rec['av_phd_year'])){
	  myPear::WARNING_T("av_phd_year='$rec[av_phd_year]'");
	}elseif (b_time::is_valid($rec['av_phd_year']) > 0){
	  $rec['_av_phd_year'] = x('tt',b_time::_date('%b %Y',$rec['av_phd_year'])) .'&nbsp;&nbsp; '.b_fmt::escape(b_fmt::strcut($rec['av_phd_institute'],30));
	  $rec['_av_phd_institute'] = '';
	}
	$rec['_pos_deadline'] = date('M Y',b_time::txt2unix($rec['pos_deadline']));

	if (is_numeric($rec['av_ms_year']) && ($rec['av_ms_year'] !== CONST_year_not_considered)){
	  $rec['_av_ms_year'] = date('Y',$rec['av_ms_year']);
	}else{
	  $rec['_av_ms_year'] = '';
	}

	// Number of rec. letters	
	$nt = (int)@$rec['rec_nt'];
	$ns = (int)@$rec['rec_ns'];
	$nt = max($nt,$rec['pos_n_referees']);
	$rec['n_nosort'] =  "$ns/$nt";
	if ($ns < $rec['pos_n_referees']) $rec['n_nosort'] = x("spawn class='redText'",$rec['n_nosort']);
	
	$this->t->class = array();
	if(bForm_jam_Applicant::_isStatus(JAM_ap_revoked,$rec)){

	  // discarded applicant are "considered" by admins
	  foreach($this->header() as $k=>$v) $this->t->class[$k] = 'overstrike redText';

	}elseif (bForm_jam_Applicant::_isStatus(JAM_ap_bogus,$rec)){
	    // Non-completed applications
	  if (!$this->pos->isStatus(JAM_pos_archived,$rec) && JAM_access()->mayRemindApplicants()){
	    $this->t->extraTD[] = b_btn::link('revoke',
					      b_url::same("?resetcache_once=1&revoke_once=".$rec['ap_id'],array(),'keepAllBut'),
					      True);
	    $aska = "?function=jam_mailErrorsInApplication&ap_id=$ap_id";
	    $this->t->extraTD[] = bJS()->modal_iframe($aska,'[ ask to fix ]','');
	  }
	}else{ // if (!$this->ap_ids){ // possibly a bogus application	  
	  // link to dossier
	  $dossier = $rec['ap_dossier_url'];
	  if (!is_file($dossier)) unset($dossier);
	  
	  // 'create dossier' (debug, to be removed)
	  $this->t->extraTD[] = (empty($dossier) 
				 ? bJS()->modal_iframe("?function=jam_doSomething&resetcache_once=1&jds_act_once=dossier&id_once=$ap_id",
						       '[ create dossier ]','')
				 : b_btn::link('dossier',$dossier));
	  
	  // link to reviews
	  if (JAM_access()->may_seeReviews() || JAM_access()->may_writeReviews()){
	    $r = $rec['ap_rating'];
	    if (($r === 0) || ((int)$r > 0)){ 
	      // There are some reviews
	      $rec['_ap_rating'] = x('span class="b_important_number"',sprintf('%1.1F &nbsp;&nbsp;&nbsp;',$r/10));
	      $link_review = bJS()->modal_iframe("?holder=jam_Reviews&id=0&ap_id=$ap_id", '[ review ]','',True);
	    }elseif(JAM_access()->may_writeReviews() && is_object($this->pos) && !$this->pos->isStatus(JAM_pos_archived)){
	      // No reviews, 'give rating (write a review)'
	      $rec['_ap_rating'] = 'no&nbsp;rating';
	      $link_review = bJS()->modal_iframe("?function=jam_giveRating&r_apid=$ap_id&r_avid=".bAuth::$av->ID,'[ review ]','');
	    }else{
	      $link_review = '';
	    }
	    $this->t->extraTD[] = $link_review;
	  }
	  
	  if ($this->pos && !$this->pos->isStatus(JAM_pos_archived,$rec) && bForm_jam_Applicant::_isStatus(JAM_ap_bad_form,$rec)){
	    if (JAM_access()->mayRemindApplicants()){
	      if (0)$this->t->extraTD[] = b_btn::link('send a reminder',
						      b_url::same("?function=jam_mailErrorsInApplication&id=$ap_id"));
	      else  bJS()->modal_iframe("?function=jam_mailErrorsInApplication&id=$ap_id",'[ send reminder ]','');
	    }
	  }
	}
	$this->t->prt($rec);
      }
    }
  }
}
