<?php

if (function_exists('locateAndInclude')){
  locateAndInclude('APImenu');
  locateAndInclude('b_tabs');
}

foreach(array('tab_jam_administer'      =>  'Administer JAM',
              'tab_jam_applicants'      =>  'Applicants',
              'tab_jam_rate_referees'   =>  'See referees rating',
              'tab_jam_quickStatus'     =>  'Quick overview',
              'tab_jam_admins'          =>  'Position manager',
              'tab_jam_secretaries'     =>  'Position secretary',
              'tab_jam_home'            =>  'My contact info',
              'tab_jam_myRecLetters'    =>  'My letters of recommendation',
              'tab_jam_myApplications'  =>  'My applications',
              'tab_jam_search'          =>  'Search',
              'tab_jam_whoIsWho'        =>  'Who is who',
              'tab_jam_sc'              =>  'Search & Selection Committee',
              'tab_jam_positions'       =>  'Setup positions',
              'tab_jam_managers'        =>  'Position managers',
              'tab_jam_toolbox'         =>  'Administer JAM',
              'tab_rfields'             =>  'Setup Research fields',
              'tab_reqdocs'             =>  'Setup Required documents',
              ) as $name=>$descr) APItabs::define_tab($name,$descr);

/*
 *
 */
class APImenu_jam extends APImenu{
  var $q          = JAM_MODULE;
  var $static_menu=array(//tab_jam_administer   
			 tab_jam_toolbox      => array(tab__mergeAV, tab_rfields, tab_reqdocs, tab_jam_positions, 
						       tab_jam_whoIsWho, tab__mergeAV, tab_jam_managers, tab__syslog, tab_jam_rate_referees),
			 tab_jam_home         => array(),
			 tab_jam_myRecLetters => array(),
			 tab_jam_myApplications=>array(),
			 tab_jam_quickStatus  => array(),
			 );
  var $tabWeight  =array(tab_jam_home         => -999,
         		 //tab_jam_quickStatus  => -800,

			 tab_jam_administer   => +99,
			 tab_jam_managers     => -92,
			 tab__mergeAV         => -91,
			 tab_jam_positions    => -90,
			 tab_jam_quickStatus  => +80,
			 tab_jam_whoIsWho     => -70,
			 tab_rfields          => -50,
			 tab_reqdocs          => -50,
			 tab__syslog          => +99,
			 tab_jam_myApplications=>+99,
			 tab_jam_toolbox      =>+99,
			 );

  function __construct(){
    locateAndInclude('jam_functions.inc');
    locateAndInclude('APIaccess_jam');
    locateAndInclude('bForm_jam_Position');
    parent::__construct(tab_jam_home);
  }
  

  /*
   * Build "dynamic menu".
   *
   * Complete the tabs list with the "event driven" tabs, 
   * i.e. those which are defined by current the position settings.
   * Normally this is Selection Committee structure and
   * the list of 'sub-positions' 
   */
  protected function build_dynamic_menuTree(){

    if (start_JAM() && empty($this->dynamic_menu)){
      locateAndInclude('bForm_jam_RF');
      locateAndInclude('bForm_Organization');
      
      // put all the positions into the list of tabs
      foreach (bForm_Organization::getAll() as $org_id){
	foreach(array_keys(bForm_jam_Position::getPositions($org_id)) as $pos_id){
	  $pos = myPear::getInstance('bForm_jam_Position',$pos_id);
	  if ($pos->managedByJAM()){

// /*	    
	    // (ap_posid) menu for applicants 
	    $this->dynamic_menu[tab_jam_myApplications][serialize(array('ap_posid'=>(int)$pos->ID))] = $pos->name();
	    
	    // (pos_id,rank) menu for the search committees
	    foreach(JAM::$SC_ranks as $rank){
	      $this->dynamic_menu[tab_jam_sc][$id=serialize(array('pos_id'=>(int)$pos->ID,'rank'=>(int)$rank))] = 'SC'; // $pos->SC_rank2name($rank);
	      $this->tabWeight[$id] = -$rank;
	    }
	    
	    // (pos_id,rank) menu for the secretaries & admins
	    foreach(array(RANK_jam_posadmin => tab_jam_admins,
			  RANK_jam_secr     => tab_jam_secretaries) as $rank=>$tab){
	      $this->dynamic_menu[tab_jam_sc][$id=serialize(array('pos_id'=>(int)$pos->ID,'rank'=>(int)$rank))] = APItabs::get_tab($tab);
	      $this->tabWeight[$id] = 100 - $rank;
	    }
//	    */	    

	    // (pos_id,rf_id) menu for the tab tab_jam_applicants (submenu of research fields, if any)
	    foreach ($pos->getRF_id() as $rf_id){
	      $rfClass = new bForm_jam_RF($rf_id);
	      $name = $rfClass->name();
	      $this->dynamic_menu[tab_jam_applicants][$id=serialize(array('pos_id'=>(int)$pos->ID,'rf_id'=>(int)$rf_id))] = $name;
	      if ($name=='Other') $this->tabWeight[$id] = 999;
	    }
	  }
	}
      }
      ksort($this->dynamic_menu);
      foreach(array_keys($this->dynamic_menu) as $key){
	if (is_array($this->dynamic_menu[$key])) ksort($this->dynamic_menu[$key]);
      }
    }
  }
  
  /*
   *
   */
  function access($tab,$dynamicTab='') {
    locateAndInclude('bAuth');   
    if (!start_JAM()) return False;

    return True; //////////////////////////////////////////////////////////////////////////////////// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if (($reply = $this->access_callback($tab))===Null){
      $rank = JAM_access()->getRank();
      
      switch($tab){
	
      case tab_jam_whoIsWho:
      case tab_rfields:
      case tab_reqdocs:
	$reply = (JAM_posID() > 0) && JAM_manager_here;
	break; 
	
      case tab_jam_quickStatus:  
	$reply = (JAM_posID() > 0) && superUser_here;
	break; 
	
      case tab__mergeAV:
      case tab_jam_positions:
	$reply = JAM_manager_here;
	break; 

      case tab_jam_toolbox:
      case tab_jam_administer:
      case tab_jam_search:
      case tab_jam_rate_referees:
	$reply = JAM_posadmin_here;
	break; 
	
      case tab_jam_sc:
      case tab_jam_managers:
      case tab_jam_applicants: 
	$reply = (JAM_posID() > 0) && JAM_observer_here;
	break; 
	
      case tab_jam_myApplications:
	$isApplicant = (is_object(bAuth::$av) && method_exists(bAuth::$av,'isApplicant') && bAuth::$av->isApplicant());
	$reply = ($rank == RANK_jam_applicant)  || $isApplicant;
	break;
	
      case tab_jam_myRecLetters:
	$isReferee = (is_object(bAuth::$av) && method_exists(bAuth::$av,'isReferee') && bAuth::$av->isReferee());
	$reply = ($rank == RANK_jam_referee)  || $isReferee;
	break; 
	
      case tab_jam_home:
	$reply = ($rank == RANK_jam_referee) || ($rank == RANK_jam_applicant);
	break; 
	
      default:
	if (!empty($dynamicTab) && is_array($items = unserialize($dynamicTab))){
	  foreach ($items as $key=>$value){
	    // myPear::DEBUG("$key - $value");
	    switch($key){
	    case 'ap_posid':
	      $reply = ((int)@bAuth::$av->ID && method_exists(bAuth::$av,'isApplicant') && bAuth::$av->isApplicant($value,$onlyActive=True));
	      break;
	    case 'pos_id':  
	      $reply = ($value == JAM_posID());
	      break;
	    case 'rf_id': // research fields
	      $reply = $reply && is_object($rfClass=myPear::getInstance('bForm_jam_RF',$value,'strict')) && ($rfClass->name()!=CONST_noname) && count($rfClass->applicants()); 
	      break;
	    case 'rank': // SC,admins,secretaries units
	      switch($value){
	      case RANK_jam_SC1:
	      case RANK_jam_SC2:
	      case RANK_jam_observer:
		$reply = $reply && JAM_observer_here && cnf_b_tabs_JQ;
		break;
	      case RANK_jam_posadmin:
	      case RANK_jam_secr:
		//		$reply = $reply &&  && cnf_b_tabs_JQ;
		break;
	      default:
		$reply = $reply && ($rank >= RANK_jam_posadmin);
	      }
	      if (!JAM_pos() || (mb_strToLower(JAM_pos()->SC_rank2name($value))==CONST_noname))  $reply = False;
	    }
	  }
	}else{
	  if (empty($tab))      $reply = False;
	  else myPear::WARNING_T(__METHOD__." unexpected tab '$tab'");
	}
      }
      if ($reply)      b_debug::xxx($reply,array(2,$this,'A'));
    }
    return $reply;
  }

  /*
   * The processor
   */  
  function process($tab,$dynamicTab=''){

    if (!start_JAM()) return False;
    // print bJS()->modal_iframe(b_url::same("?function=bHolder_Logs::wic"),'&nbsp;'.'x');

    $this->dbg();

    bTiming()->cpu(__FUNCTION__);
    locateAndInclude('bTpl');   
        
    // Set "input default value" for the search box
    myPear_search()->idv('search for applicants');

    /*
     * see what we can do, see which menu tab is pressed
     */
    $rank = JAM_access()->getRank();
    if (JAM_posID()) myPear::setPageHeader(JAM_pos()->name());
    else             myPear::setPageHeader('Job Applications Manager (JAM)');

    $tab = $this->processTemplates($tab);
    switch ($tab){
      
      // the processing is done already in processTemplates($tab)
    case Null:
      break;
      
    case JAM_MODULE:
    case tab__empty:
      if ($rank == RANK_jam_applicant){
	$this->getForm_applicant();
	break;
      }
      if (!empty($_POST['JAM_posID'])){
	if (b_url::header("?q=jam/".APItabs::tab2code(tab_jam_applicants)))	return;
      }

    case tab_jam_administer:
    case tab_jam_home: 
      switch ($rank){
      case RANK__superuser:
      case RANK_jam_posadmin:
      case RANK_jam_secr:
      case RANK_jam_SC1:
      case RANK_jam_SC2:
      case RANK_jam_observer:
	locateAndInclude('about_jam');   
	break;

      case RANK_jam_referee:
      case RANK_jam_applicant:
	$positions = array_keys(bAuth::$av->isApplicant($pos_id=False,$onlyActive=False));
	if (!empty($positions)){
	  bAuth::$av->getForm();
	}else{
	  bAuth::$av->getForm();
	}
	if (cnf_dev) bAuth::$av->get_all_uploads();
	break;

      default:
	print bHolder::loadWidget('?holder=jam_Positions');
	break;
      }
      break;
      
    case tab_jam_toolbox:
      locateAndInclude('jamToolBox');
      $tb = new jamToolBox();
      $tb->getForm();
      break;

    case tab_jam_rate_referees:
      locateAndInclude('bHolder_jam_rateReferees');
      $holder = new bHolder_jam_rateReferees('rate_referees');
      $holder->listEntries();
      break;

    case tab_jam_positions: 
      print bHolder::loadWidget('?holder=jam_Positions');
      break;
      
    case  tab_jam_applicants:
      locateAndInclude('bHolder_jam_Applicant');
      $holder = new bHolder_jam_Applicant();
      break;

    case tab_reqdocs:
      JAM_requiredDocs()->getForm();
      break;

    case tab_rfields:
      JAM_subPositions()->getForm();
      break;

    case tab_jam_myRecLetters: 
      jam_listRecommendations(bAuth::$av->ID);
      break;
      
    case tab_jam_search:  
      myPear_search()->getForm();
      break;
      
    case tab_jam_whoIsWho: 
      // Do it as quick status...
      // Would be nice to have "virtual units" 
      // (those which keep info in memory, without flashing it on the disk), 
      // but that is for the next life... 
      /*
      if(JAM_posID()){
	$unit = new bUnit(JAM_posID(),'virtual','Applicants & Referees');
	if (myPear::is_empty($unit->getMembers())){
	  locateAndInclude('bHolder_Avatar_jam');
	  $holder = new bHolder_Avatar_jam();
	  $holder->where[] = "ap_posid = ".JAM_posID();
	  $q = $holder->query();
	  while($r = myPear_db()->next_record($q)){
	    $unit->get_memberInfo($r['av_id'],$r,'update');
	  }
	}
	$unit->getForm();
      }
      break;
      */
      
    case tab_jam_quickStatus:    
      locateAndInclude('bHolder_jam_Referees');
      $holder = new bHolder_jam_Referees();
      $holder->listEntries();
      break;
      
    case tab_jam_managers:    
      locateAndInclude('bUnit');
      bUnit::b_tabs_bUnits(JAM_MODULE,Null,array(),array(RANK_jam_manager  => myOrg_ID,
							 RANK_jam_posadmin => JAM_posID(),
							 RANK_jam_secr     => JAM_posID(),
							 RANK_jam_observer => JAM_posID()));
      break;
      
    case tab_jam_sc:
      locateAndInclude('bUnit');
      myPear::H1('Search & Selection Committee');
      bUnit::b_tabs_bUnits(JAM_MODULE,Null,array(),JAM::SC_ranks());
      break;
      
    case tab_jam_myApplications:
      // Discover the position for the logged in person
      myPear::setPageHeader("My applications");
      $table = array();
      $n_positions = 0;
      foreach(bAuth::$av->positionsConserned($asReferee=False) as $pos_id=>$ap_id){
	if ($pos = myPear::getInstance('bForm_jam_Position',$pos_id,'strict')){
	  $table[$pos->isStatus(JAM_pos_archived) ? 'Archived' : 'Current'][$ap_id] = $pos;
	  $n_positions++;
	  $latest_ap = $ap_id;
	}
      }

      if ($n_positions == 1){
	$ap = new bForm_jam_Applicant($latest_ap);
	$ap->getForm();
      }else{
	foreach(array('Current','Archived') as $s){
	  if (!myPear::is_empty($aps=@$table[$s])) {
	    print x('h3',"$s application".(count($aps)>1 ? 's' : '').':').'<ul>';
	    foreach($aps as $ap_id=>$pos){
	      print x('li',x('a href="'.b_url::same("?id=$ap_id&form=jam_Applicant").'"',x('strong',$pos->name())));
	    }
	    print '</ul>';
	  }
	}
      }
      break;
      
      // Dynamic menu
      // - show list of applicants by reseach fields
      // - show applicants his applications
      // -
    default: 
      if (is_array($items = unserialize($dynamicTab))){
	foreach ($items as $key=>$value){
	  // myPear::WARNING_T("$key: $value");
	  switch($key){
	  case 'ap_posid': 
	    $this->getForm_applicant($value);
	    break;
	    
	  case 'rf_id':
	    locateAndInclude('bHolder_jam_Applicant');
	    $holder = new bHolder_jam_Applicant($value);
	    break;
	    
	  case 'rank':
	    JAM_unit($value)->getForm();
	    break;
	  }
	}
      }else{
	if (empty($tab))       print bHolder::loadWidget('?holder=jam_Positions');
	else  myPear::ERROR(b_debug::__()." unexpected tab '$tab'");
      }
    }
    
    if (is_object(JAM_mailer())) JAM_mailer()->showMailsSent(JAM_manager_here);
    bTiming()->cpu();
  }
  
  /*
   * bForm_jam_Applicant::getForm() for the logged-in applicant
   */
  private function getForm_applicant($ap_posid=Null){
    locateAndInclude('bForm_jam_Applicant');
    foreach (bAuth::$av->isApplicant($ap_posid) as $ap_id=>$ap_status){
      $ap = new bForm_jam_Applicant($ap_id,@$_GET['mode']);
      JAM_pos($ap->pos->ID);
      $ap->getForm();
      break; 
    }
  }
}

function _jam_title_callback($tab, $dTab='',$returnText=False){
  if (!empty($dTab)){
    $items = unserialize($dTab);
    if (($pos_id=@$items['pos_id']) && ($rank=@$items['rank']) && ($rank<=RANK_jam_SC1) && ($rank>=RANK_jam_observer)){
      $menuItem = JAM_pos($pos_id)->SC_rank2name($rank);
    }elseif($rf_id=@$items['rf_id']){
      $menuItem = JAM_RF($rf_id)->name();
    }else{
      $menuItem = ($returnText
		   ? $tab
		   : t($tab));
    }
  }else{
    if(isset(APItabs::$tab_definition[$tab]))  $tab = APItabs::$tab_definition[$tab];
    $menuItem = ($returnText
		 ? $tab
		 : t(bText::_(array('key' => "MENU_$tab",'item'=> $tab))));
  }
  return $menuItem;
}

