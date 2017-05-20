<?php
/*
 * auto-update the accommodations database from the agenda system
 */
locateAndInclude('agenda');
locateAndInclude('bUnit_vm');
locateAndInclude('bForm_vm_Visit');
locateAndInclude('bForm_vm_Lease');
locateAndInclude('bList_vm_agendaEvents');

// The applications deadline should be at least a month before the event starts 
define('application_deadline',30 * 24 * 3600); 

class agenda_vm extends agenda{
  
  public $lookBackFor = 30; // by default look back for 30 days, ignore the previous events
  var $nUpdates = 0;

  private $e_start, $e_end, $r_start, $r_end;

  function __construct(){
    define('dummy_OA',$GLOBALS['LODGING_FINDER'][LODGING_OA]);
    parent::__construct();
  }
  
   /*
    *
    */
   public function getCategoryURL($categId){
     return $this->agenda_url."/categoryDisplay.py?categId=$categId";
   }

   /*
    *
    */
   public function getEventURL($confId){
     return $this->agenda_url."/conferenceDisplay.py?confId=$confId";
   }
   
   /*
    *
    */
   public function getRegistrantURL($confId,$registrantId=Null){
     if (empty($registrantId)) return $this->agenda_url."/conferenceDisplay.py?confId=$confId";
     else return $this->agenda_url."/confModifRegistrants.py/modification?registrantId=$registrantId&confId=$confId";
   }
   
  /*
   * Get list of conferences for the Category directly from the indico server
   */
  public function getConferences($cat){
    $reply = array();
    foreach($this->getCategory($cat) as $conf) $reply[] = $conf['id'];
    return $reply;
  }

  /*
   *
   */
  public function sync($conferences_to_sync=array()){
    static $alreadySyncing = 0;
    if ($alreadySyncing++) return;

    bTiming()->cpu(__FUNCTION__);
    set_time_limit(0);
    $this->dbg(starting);
    myPear::H1("Syncing visitors database with Agenda");

    // Update list of events
    VM_agendaEvents()->syncEventsInCategories(False);

    myPear_logs()->doNotLog = True;

    $this->noCache = True; 
    
    if (empty($conferences_to_sync)){
      if (($conf=@$_GET['conf'])){
	$conferences_to_sync[] = "conf=$conf&show_reg=all";
      }else{
	// Get the list of events which we need from the Agenda holder,
	$events = array();
	foreach(VM_agendaEvents()->getMembers() as $m) $events[] = ($m['lm_key'].'='.$m['lm_value']);

	foreach ($events as $a){
	  foreach ($this->parser($a) as $ev_id=>$program) {
	    if ($p_id = @$program['id']){
	      if (b_time::txt2unix($program['end']) + $this->lookBackFor * 3600*24 < b_time::noon()){ // ignore the past
		if (empty($_GET['refresh_once']) && myPear::getInstance_new('bForm_vm_Event',array('e_code'=>$p_id),array('nocache','strict'))) continue;
	      }elseif(b_time::txt2unix($program['end']) < b_time::noon(2009,11,1)){ // ignore the pre-ABS era...
		if (empty($_GET['refresh_once']) && myPear::getInstance_new('bForm_vm_Event',array('e_code'=>$p_id),array('nocache','strict'))) continue;
	      }
	      $conferences_to_sync[] = join('&',array("conf=$p_id",
						      "show_reg=all"));
	    }
	  }
	}
      }	
    }	
    
    //... test  $conferences_to_sync = array('conf=1125&show_contrib=all');
    //    $c = array_shift($conferences_to_sync);
    //    $conferences_to_sync = array($c);

    /* now process the recent events only */
    foreach ($conferences_to_sync as $args){
      myPear_db()->noAutoCommit('start');
      foreach ($this->parser($args) as $ev_id=>$program) {
	$this->dbg(starting);
	$program['title'] = strip_tags($program['title']);
	if (count($conferences_to_sync) == 1) myPear::MESSAGE(x('h3',"Syncing ".x('em',$program['title'])));

	$allRegistrants = array();

	// The applications deadline should be at least a month before the event starts 
	//	$this->r_end   = min(($this->e_start-application_deadline) , b_time::txt2unix($program['regend']));
	//	$this->r_start = min(($this->r_end  -application_deadline) , b_time::txt2unix($program['regstart']));
	$this->e_start = b_time::txt2unix($program['start']);
	$this->e_end   = b_time::txt2unix($program['end']);
	$this->r_start = b_time::txt2unix($program['regstart']);
	$this->r_end   = b_time::txt2unix($program['regend']);
	
	// update or create the Event instance 
	$this->e = myPear::getInstance_new('bForm_vm_Event',array('e_code'=>$program['id']));
	$this->newE = !$this->e->getValue('e_orgid');
	$this->e->defineVariables();
	$this->e->updateDB(array('e_orgid'    => myOrg_ID,
				 'e_code'     => $program['id'],
				 'e_name'     => $program['title'],
				 'e_reg_start'=> $this->r_start,
				 'e_reg_end'  => $this->r_end,
				 'e_start'    => $this->e_start,
				 'e_end'      => $this->e_end));
	$this->title = join(' ',array($this->e->name(),
				      x('(',(int)@$program['attenders'].' attenders')));
	$this->av = 0;
	if ($this->newE) $this->printInfo('New event');
	$this->dbg($this->e,'e_start','e_end');

	// Get & update the time of the last sync
	$lastSync = b_vars::set('agendaSync_'.$this->e->ID,time(),VM_MODULE); 
	if (empty($lastSync)) $lastSync = $this->r_start;
	$this->dbg("Previous sync ".date('Y-m-d H:i',$lastSync));
	
	// Update event organizers unit
	$allOrganizers = array();
	if (empty($program['chairs']))   $program['chairs'] = array();
	foreach($program['chairs'] as $chair) {
	  $name = bForm_Avatar::_fmtName('sfl',$chair);
	  if (myPear::is_empty($email = @$chair['av_email'])){
	    $email=$chair['av_email'] = bForm_Avatar::generate_email($chair['av_firstname'],$chair['av_lastname']);
	    myPear::WARNING(sprintf("%s<br>E-mail missing for organizer %s, impose %s",
				    $program['title'],$name,$email));
	  }
	  if (empty($email)){
	    myPear::WARNING("??? empty email for '$name'");
	  }else{
 	    $av = bForm_Avatar::hook($chair);
	    VM_organizers($this->e->ID)->add_unitMember($av,$verbose='if_new');
	    $allOrganizers[] = $av->ID;
	  }
	}
	
	/* update Registrants */
	if (empty($program['registrant'])) $program['registrant'] = array();
	foreach  ($program['registrant'] as $reg){
	  // $this->dbg(starting);

	  $v_code = (int)$program['id'] * 100000 + (int)$reg['id'];
	  $allRegistrants[] = $v_code;
	  
	  $reg['email'] = preg_replace("/\s/","",mb_strToLower(trim($reg['email'])));
	  if (empty($reg['arrivaldate']))   $reg['arrivaldate']   = $program['start'];
	  if (empty($reg['departuredate'])) $reg['departuredate'] = $program['end'];
	  if (empty($reg['institution']))   $reg['institution']   = CONST_noname;
	  
	  //	Registrant_RegistrationDate=12-January-2015-20:29
	  $registrationDate = b_time::txt2unix($reg['registrationdate']);

	  // Hook or create the avatar class 
	  if ($avs = bForm_Avatar::hookByEmail($reg['email'])){
	    $this->av = array_pop($avs);
	    $this->av->updateDB(array('av_institute'=> $reg['institution'],
				      'av_lastname' => b_fmt::ucwords($reg['familyname'],True),
				      'av_firstname'=> b_fmt::ucwords($reg['firstname'], False)));
	  }else{
	    $this->av = bForm_Avatar::hook(array('av_email'    => $reg['email'],
						 'av_institute'=> $reg['institution'],
						 'av_lastname' => b_fmt::ucwords($reg['familyname'],True),
						 'av_firstname'=> b_fmt::ucwords($reg['firstname'], False)));
	  }
	  // Hook the GSF (General Submit Form) - optional fields added by the event organizers
	  // Filter out answer(s) "no" & "for myself"
	  $this->gsf = array();
	  foreach(preg_grep('/^gsf_/i',array_keys($reg)) as $k){
	    if (preg_match('/CAPTION:(.*)VALUE:(.*)$/',$reg[$k],$m)){
	      $q = trim(str_ireplace('(participant name)','',$m[1]));
	      $v = trim($m[2]);
	      // strip out wish "for myself"
	      if ((stripos($v,'share') !== False) &&
		  (stripos($v,$reg['familyname']) !== False) &&
		  (stripos($v,$reg['firstname'])  !== False)) unset($v);
	      if (!empty($q) && !empty($v) && 
		  !preg_match(';^(-|no|no.?o?ne( else)?|n/?a|nil|not app?licable|no preference)\.?$;i',strToLower($v))) $this->gsf[$q] = $v;
	    }
	  }

	  // Eventually update the personal information
	  $upd = array();
	  foreach (array('av_residentship'=> b_cc::getCC(@$reg['country']),
			 'av_phone'       => @$reg['phone'],
			 'av_city'        => @$reg['city'],
			 'av_http'        => @$reg['homepage'],
			 'av_salutation'  => @$reg['salutation'],
			 //'av_address'   => @$reg['address'],
			 ) as $k=>$v) if (!empty($v)) $upd[$k] = $v;
	  $this->av->updateDB($upd);
	  
	  $_POST['v_avid'] = $this->av->ID; // interface with 'bForm_vm_Visit'
	  $this->v = myPear::getInstance_new('bForm_vm_Visit',array('v_code'=>$v_code),'nocache');
	  $this->v->isVITAL = array();
	  $this->v->defineVariables();
	  if (0){
	    $this->v->debug = True;
	    $this->v->updateDB(array('v_code'=>$v_code,
				     'v_avid'=>$this->av->ID,
				     'v_eid' =>$this->e->ID));
	    $this->v->hookAvatar();
	    $this->v->hookEvent();
	  }
	  
	  $time1 = b_time::txt2unix($reg['arrivaldate'],  CONST_TIME_CI);
	  $time2 = b_time::txt2unix($reg['departuredate'],CONST_TIME_CO);
	  
	  // Parse the accommodation wish
	  if (empty($reg['accommodation'])) $reg['accommodation'] = dummy_OA;
	  $accommodation_wish = $this->get_accommodation_wish($reg['familyname'],$reg['accommodation']);
	  
	  // Tidy up the arrival/departure time for the local participants,
	  // for them the arrival/departure must be within the event period
	  $VM_V_wish_OA = in_array(LODGING_OA,$accommodation_wish);
	  if(False)	if ($VM_V_wish_OA){
	    $time1 = max($time1,$this->e_start);
	    $time2 = min($time2,$this->e_end);
	  }

	  // Update the Visit form
	  $v_type = (bList_vm_agendaEvents::eventOutOfScope($program['id']) ? VISIT_OUT_OF_SCOPE : VISIT_TYPE_PROGRAM);
	  $data = array('v_type'  => $v_type,
			'v_avid'  => $this->av->ID,
			'v_eid'   => $this->e->ID,
			'v_code'  => $v_code,
			'v_start' => $time1,
			'v_end'   => $time2,
			'v_gsf'   => serialize($this->gsf),
			);
	  if (($v_type == VISIT_TYPE_PROGRAM) && ($reg['accommodation'] != dummy_OA)) $data['v_acc_wish'] = $reg['accommodation'];

	  $this->updateIfChanged($data,$v_code);
	  $this->v->hookAvatar();
	  $this->v->hookEvent();

	  $this->dbg($this->v,'v_start','v_end');
	  if (!b_time::inInterval($time1,$time2,$this->e_start,$this->e_end)){
	    myPear::WARNING($program['title'].' '.b_time::period($this->e_start,$this->e_end).
			    ": application <i>$reg[familyname] ".b_time::period($time1,$time2)." is outside the event dates</i>");
	  }

	  //
	  // Send a welcome mail to the NEWLY REGISTERED applicants
	  if ((VM_mailer()->m_applicant_info_mail($this->v,'status') === False)){
	    // This function expects "v_status" being empty for the newly created visits
	    if (send_infoMail_to_applicants) VM_mailer()->m_applicant_info_mail($this->v,$no_preview=True);
	    else   $this->v->setPolicy(VM_V_infoMailNC,1);
	  }
	  
	  // Set the visit status to 'pending' if the visit is not approved
	  if (!$this->v->getValue('v_status',True,True)) $this->v->updateDB(array('v_status' => STATUS_PENDING));

	  // Adjust the leases
	  foreach(bForm_vm_Lease::adjust_all($this->v,0,True) as $message){
	    $this->printInfo($message);
	  }
	  
	  /*
	  // Detect changes in the 'own accommodation' wish 
	  $accommodation_now = $this->v->hut_codes();
	  if ($accommodation_now !== $accommodation_wish){
	    $verbose = True;
	    if (cnf_dev)myPear::DEBUG("$reg[familyname]: \"$reg[accommodation]\" now=".join(',',$accommodation_now)." wish=".join(',',$accommodation_wish));
	    if (empty($accommodation_wish)){
	      if($accommodation_now === array(LODGING_OA)){
		$this->printInfo('');
		$this->v->deleteLease(Null,$verbose);
	      }
	    }else{
	      foreach($accommodation_wish as $wish){
		switch($wish){
		case LODGING_OA:
		  if (!in_array($wish,$accommodation_now)){
		    $this->printInfo('');
		    if (!empty($accommodation_now)) $this->v->deleteLease(Null,$verbose);
		    $this->v->setBooking(VM::$OA->ID,$verbose);
		  }
		  break;
		default:
		  b_debug::internalError("??? unexpected wish \"$wish\"");
		}
	      }
	    }
	  }
	  */
	  
	  // Follow the accommodation wish
	  $accommodation_now = $this->v->hut_codes();
	  foreach($accommodation_wish as $wish){
	    switch($wish){
	    case LODGING_OA:
	      if (0) if (!in_array($wish,$accommodation_now)){
		  $this->printInfo('');
		  if (!empty($accommodation_now)) $this->v->deleteLease(Null,$verbose);
		  $this->v->setBooking(VM::$OA->ID,$verbose);
		}
	      if (empty($accommodation_now)){
		$this->printInfo('');
		$this->v->setBooking(VM::$OA->ID,$verbose);
	      }
	      break;
	    default:
	      if (0) b_debug::internalError("??? unexpected wish \"$wish\"");
	    }
	  }
	  
	  // Keep the existing leases, just adjust the arrival/departure dates
	  if (!empty($this->dataUpdates['v_start']) || 
	      !empty($this->dataUpdates['v_end'])){	  
	    $this->v->adjust_dates();
	  }
	  $this->v->__clean_cache();
	  // $this->dbg(exiting);
	}
	
	/* remove from the database the registrants who are not any more in agenda */
	$q = myPear_db()->query("SELECT * FROM abs_visits WHERE v_eid=".$this->e->ID);
	while ($r = myPear_db()->next_record($q)){
	  if (!in_array($r['v_code'],$allRegistrants)) {
	    $this->nUpdates++;
	    $v = myPear::getInstance_new('bForm_vm_Visit',$r['v_id'],array('fatal','nocache'));
	    $v->defineVariables();
	    $v->hookAvatar();
	    $this->av = $v->av;
	    $this->printInfo(b_fmt::redText("Removed from Agenda"));
	    $v->__clean_cache();
	    $v->delete();
	  }
	}

	/* remove from the database the organizers who are not any more in agenda */
	$q = myPear_db()->query("SELECT * FROM zzz_unit_members LEFT JOIN zzz_units ON u_id=um_uid WHERE u_parent=".$this->e->ID);
	while ($r = myPear_db()->next_record($q)){
	  if (!in_array($r['um_avid'],$allOrganizers)) {
	    $this->av = new bForm_Avatar_vm($r['um_avid']);
	    $this->printInfo(b_fmt::redText("Removed from organizers"));
	    //myPear_db()->qquery("DELETE FROM zzz_unit_members WHERE um_id = $r[um_id]",cnf_dev);
	    VM_organizers($this->e->ID)->deleteMember($r['um_id'],cnf_dev);
	  }
	}
	$this->dbg(exiting);
      }
      myPear_db()->noAutoCommit('end');
    }
    myPear::MESSAGE_close();
    
    if (!$this->nUpdates && !empty($events)) print x('h2','The accommodation database is up to date').
					       x('em','No updates from the agenda system at this time.');
    if (is_object(APIaccess::$cache))  APIaccess::$cache->reset();
    b_cache_file::emptyCache();

    // 

    myPear_logs()->doNotLog = False;
    if ($this->nUpdates) myPear_logs()->add('sync');  
    $this->dbg(exiting);
    bTiming()->cpu();
    $alreadySyncing = 0;
  }

  function getDescr($k){
    if (!($d = $this->v->getDescr($k))) $d = $k;
    return $d;
  }
  
  /*
   *
   */
  private function printInfo($txt){
    static $av = 0;
    if ($t=$this->title)  myPear::MESSAGE_open("$t<br/>".
					       "Application: ".b_time::period($this->r_start,$this->r_end).'<br/>'.
					       "Event: ".b_time::period($this->e_start,$this->e_end));
    $this->title = '';
    
    $av_id = is_object($av) ? $av->ID : 0;
    if (is_object($this->av) && ($this->av->ID != $av_id)) myPear::MESSAGE_add('<br/>'.x('strong',$this->av->name()));
    $av = $this->av;
    if (!empty($txt)) myPear::MESSAGE_add($txt);
  }

  private function updateIfChanged($newData,$v_code){
    $form = get_class($this->v);
    $this->dataUpdates = array();

    $r = array();
    foreach(array_keys($newData) as $k){
      if (!myPear::is_empty($vv=@$this->v->formDB[$k])) $r[$k] = $vv;
    }

    if (empty($r)){
      $this->printInfo('Application');
      if (!empty($this->v->formDB)) b_debug::print_r($this->v->formDB,'formDB');
    }else{
       foreach ($newData as $k=>$v) {
	 $newData[$k] = trim($v);
	 if (myPear_db()->columnExists($k,$this->v->TABLE)){
	   if (($n=trim($v)) != ($o=trim(@$r[$k]))) {
	     $this->v->defineVariables();
	     if (empty($this->dataUpdates[$k])) $this->dataUpdates[$k] = 0;
	     $this->dataUpdates[$k]++;
	     if (!empty($r[$k])) $this->printInfo(x('em',$this->getDescr($k)).' '.
						  $this->unix2txt($k,$r[$k]).
						  ' ----> '.
						  $this->unix2txt($k,$v));
	   }
	 }else myPear::WARNING("column '$k' does not exist in '$form'");
       }
     }

     if (count($this->dataUpdates) || empty($r)){
       $this->nUpdates++;
       $this->v->updateDB($newData);
     }
   }

   private function unix2txt($column,$value){
     if (preg_match('/_.*id$/',$column)) return $value;
     else                                return b_fmt::strcut(b_time::unix2txt(strip_tags($value),'Y-m-d H:i'),33,'...');
   }
   
   /*
    * Parse the "accommodation wish" from the application form
    */
   private function get_accommodation_wish($familyname,$wish_string){

     $default_lodging = False;
     $reply = array();
     foreach($GLOBALS['LODGING_FINDER'] as $lodging=>$regexp){
       if (($regexp != $wish_string) && !preg_match("/$regexp/i",$wish_string)) continue;
       
       switch($lodging){
       case LODGING_OA: // 'do not need|will arrange|my way|on my own'
	 $reply[] = $lodging;
	 break;
       case 'default_lodging': 
	 $default_lodging = True; 
	 break;
       default:
	 $reply[] = $lodging;
       }
     } 

     // See what we got...
     sort($reply);
     if (empty($reply) && !$default_lodging){
       $msg = b_fmt::redText("Unexpected wish from applicant $familyname:<br/><em>$wish_string</em>");
       myPear::WARNING($msg);
     }
     return $reply; 
   }
}
