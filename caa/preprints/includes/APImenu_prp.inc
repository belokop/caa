<?php

locateAndInclude('APImenu');

class APImenu_prp extends APImenu{
  var $q    = PRP_MODULE;
  var $static_menu  = array(
			    tab_prp_see => array(),
			    //tab_prp_mng => array(tab_prp_pending,tab_prp_new,tab_prp_update), 
			    //   tab_prp_mng => array(),
			    //   tab_prp_see => array(tab_prp_pending,tab_prp_new,tab_prp_update), 
			    tab_prp_pending => array(),
			    tab_prp_new     => array(),
			    tab_prp_update  => array(),
			    tab_prp_adm     => array(tab__mergeAV),
			    );
  
  var $tabWeight    = array(tab_prp_see     =>   0,

			    tab_prp_new     => +10,
			    tab_prp_mng     => +12,
			    tab_prp_pending => +14,
			    tab_prp_update  => +16,

			    tab_prp_adm     => +20,
			    tab__mergeAV    => +30,
			    );
  
  function __construct(){
    locateAndInclude('prp_updates.inc');
    parent::__construct();
  }
  
  static function startup(){

    if (!(int)myOrg_ID) return False;

    static $dejaVu = 0;
    if (!$dejaVu++){

      PRP_access();
      b_cnf::set("PRP_remindingPeriod", 7 * 24 * 3600); // a week

      /* 
       * Find out the earliest records in the db
       */
      if (!($prp_year0=b_cnf::get('prp_year0'))){
	$q = myPear_db()->qquery("SELECT MIN(prp_day0) AS day0 FROM prp_list WHERE prp_day0 > 0 AND prp_orgid = '".myOrg_ID."'");
	while ($r = myPear_db()->next_record($q)) if ($day0 = (int)$r['day0']) $prp_year0 = date('Y',$day0);
	b_cnf::set('prp_year0',$prp_year0);
      }
      define('prp_year9', date('Y',time()));
      define('prp_year0', (empty($prp_year0) ? prp_year9 : $prp_year0));
      
    }
    return True;
  }

  function access($tab) {

    PRP_access();
    if (!self::startup())  return False;

    if (($reply = $this->access_callback($tab))===Null){
      
      switch ($tab){
		
      case PRP_MODULE:
	$reply = (b_reg::$current_module === PRP_MODULE);
	break;

      case tab_prp_update:
	$reply = (PRP::_HOLDER()->count('my_all_preprints') > 0);
	break;

      case tab_prp_pending:
	$reply = (PRP::_HOLDER()->count('my_pending_preprints') > 0);
	break;

    case tab_prp_adm:
	$reply = PRP_censor_here;
	break;

      case tab__mergeAV:
	$reply = PRP_manager_here;
	break;
	
      case tab_prp_mng:
	$reply = PRP_submitter_here;
	break;

      case tab_prp_new:
	$reply = !PRP_submitter_here;
	$reply = True;
	break;
	
      case tab_prp_see:
      default:
	$reply = True;
      }
    }
    b_debug::xxx($reply,array(2,'A'));
    return $reply;
  }


  /*
   * processing the request 
   */
  function process($tab){

    // Create the organization if not yet done
    if (!self::startup()) return;

    bAuth::$showPasswordReminder = False; // cnf_dev;
    bAuth::$showCreateAccount    = False; // cnf_dev;
    
    // Preset the "general search" attributes
    myPear_search()->idv('search for authors');
    $search_args = array('_lookFor' => '',
			 '_button'  => 'show preprints',
			 '_header'  => 'Search for authors',
			 '_join'    => 'prp_list on av_id:prp_avid',
			 '_where'   => (bAuth::authenticated() ? '0' : '0'),
			 '_after_search'=> 'tab_force=tab_prp_see&av_id=AV_ID');
    /*
     * Send reminders (if any)
     */
    PRP_mailer()->reminder();
    
    switch ($tab=$this->processTemplates($tab,$search_args)){
      
      // the processing is done by '$this->processTemplates($tab)'
    case Null:     
      break; 
      
    case PRP_MODULE:
    case tab__empty:
    case tab_prp_see:
      if (empty($doing)) $doing = 'list';
    case tab_prp_pending:
      if (empty($doing)) $doing = 'managing';
      if (($doing == 'list') || bAuth()->loginPrompt()){
	locateAndInclude('bHolder_prp_preprints');
	$h = new bHolder_prp_preprints($doing);
	if ($doing == 'managing')      $h->managingWhat = $tab;
	$h->listEntries();
      }else{
	myPear::WARNING('Please authenticate with your username and password');
      }
      break;
      
    case tab_prp_adm:
      bUnit::b_tabs_bUnits(PRP_MODULE);
      break;

    case tab_prp_mng:
      if (bAuth()->loginPrompt()){
	b_debug::xxx(' ');
	$this->get_lowLevelMenu(tab_prp_mng);
      }else{
	myPear::WARNING('Please authenticate with your username and password');
      }
      break;

    case tab_prp_update:
      if (bAuth()->loginPrompt()){
	locateAndInclude('bHolder_prp_preprints');
	myPear::setPageHeader('Preprints management');
	$h = new bHolder_prp_preprints('managing');
	$h->listEntries();
      }else{
	myPear::WARNING('Please authenticate with your username and password');
      }
      break;
      
    case tab_prp_new:
      if (bAuth()->loginPrompt()){
	locateAndInclude('bForm_prp');
	$f = new bForm_prp('new');
	$f->getForm();
      }else{
	myPear::WARNING('Please authenticate with your username and password');
      }
      break;
      
    default:
      print "<h3>$tab</tab>";
    }
  }
}

function _prp_title_callback($tab, $dTab='',$returnText=False){

  if ($dTab){
    $menuItem = ($returnText
		 ? $tab
		 : t($tab));
  }else{
    if(isset(APItabs::$tab_definition[$tab]))  $tab = APItabs::$tab_definition[$tab];
    $menuItem = ($returnText
		 ? $tab
		 : t(bText::_(array('key' => "MENU_$tab",'item'=> $tab))));
  }
  b_debug::xxx($menuItem,array('M',2));
  return $menuItem;
}
