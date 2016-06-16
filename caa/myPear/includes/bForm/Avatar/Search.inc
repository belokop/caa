<?php
/*
 * The search might be of the following types:
 * 1. an informational  search in the database, 
 *    no further actions expected
 * 2. search to get candidates for duplicated records in the database, 
 *    the found records will be (or might bree) merged
 * 3. search for the candidates to become a unit member,
 *    a "nomination" will/might follow
 */
locateAndInclude('bForm');
locateAndInclude('b_checkBox');
locateAndInclude(bForm_Avatar::$bForm_Avatar);

/*
 * The search engine
 */
class bForm_Avatar_Search extends bForm {

  var $debug = False;
  var $aceptAsIs_DoNotSave = True;
  var $modalRW_enabled = True;

  var $maxRows = 25;
  var $buttons = array();

  private $idv   = 'search';  // input default value

  public  $args   = array('_header' => 'Search the database');

  private $holder = Null;

  // If $global_search is set to True, then the default Avatar holder is invoked,
  // otherwise the module-specific one.
  private $global_search = False;

  private $log_widgets = array();  // search results for log records
  private $show_log_widgets = False;

  private $searchNumRows = 0;
  public  $search_for_merge = False;

  private $args_kept = array('_button','_lookFor','_after_search',
			     '_join','_where');

  public static $search_items = array('firstname_search_once'=>"First name",
				      'lastname_search_once' =>"Last name",
				      'email_search'         =>"E-mail");
  
  /*
   *
   */  
  function __construct(){
    parent::__construct('empty');
    $this->idv();
  }

  
  /*
   * Define the search form
   */
  function defineVariables(){
    foreach (self::$search_items as $name=>$descr){
      $this->defineVariable($descr, $name);
      $this->defineVariable($descr, $this->fav($name));
    }
  }
  
  /*
   * Input Default Value
   */
  public function idv($idv=''){
    if (!empty($idv))           $this->idv = $idv;
    $GLOBALS['search_string'] = $this->idv;
  }

  /*
   * Reply 'NO' if the search request is not valid.
   * The query should have at least 2 ASCII symbols and be 3 characters length
   */
  public function accepted($search_string){
    if (!is_array($search_string))   $search_string = array($search_string);
    $ok = True;
    $query_length = 0;
    foreach($search_string as $k=>$v){
      $query_length += strlen($v);
      if (strpos($v,$this->idv) !== False)          $ok = False;
      if (!empty($v) && !preg_match('/\w.*\w/',$v)) $ok = False;
    }
    if ($query_length < 3)                          $ok = False;
    if (!$ok) myPear::WARNING("Not a valid search query, ignored");
    $this->dbg($ok);
    return $ok;
  }

  /*
   * readDefRecord is dummy for the search
   */
  function readDefRecord(){
    $this->formDB = array();
  }

  /*
   * Choose the bHolder_Avatar_<module> which will display the search results.
   */
  private function init_search(){
    
    // We have reached the search engine,  hence may clean the cached URL 
    // "go to the search after creating new unit member"
    b_url::$referrer_debug = $this->debug;
    if (b_url::getReferrer('bUnit')){
      b_url::setReferrer('bUnit','reset');
      b_url::setReferrer('search_again','reset');
    }

    // Aha, the arguments were transmitted via _GET, not _POST
    if (empty($_POST) && isset($_GET['_post_bForm_ID_once'])) $_POST = $_GET;

    // _POST has precedence over the arguments
    if ($this->myPost_bForm()){
      foreach($_POST as $k=>$v){
	if (isset($this->args[$k]))  myPear::WARNING_T("Set \$args[$k] '".$this->args[$k]."' => '$_POST[$k]'");
	$this->args[$k] = $_POST[$k];
      }
    }

    //    if(isset($_GET['_after_search'])) $this->args['_after_search'] = str_replace(',','&',$_GET['_after_search']);
    foreach($this->args_kept as $a){
      if (empty($this->args[$a]) && !empty($_REQUEST[$a]))  $this->args[$a] = str_replace(',','&',$_REQUEST[$a]);
    }

    // Limit the number of Avatars to be merged to a reasonable number
    $this->search_for_merge = (@$this->args['_lookFor'] === 'X');
    if ($this->search_for_merge)      $this->maxRows = 13; 
    
    print myPear::H1(@$this->args['_header']);
  }

  /*
   * Show the search results
   */
  function getForm($args=array()){
    $this->dbg();

    $this->args = array_merge($args,$this->args);

    // Initialise the search
    $this->init_search();
    
    $this->getFormOpen();
    switch($this->got_search_request = (bool)!myPear::is_empty(array_intersect(array_keys($_POST),
									       array_keys(self::$search_items)))){
    case False:
      /*
       * Start the bForm dialog, prompt the user for the search query.
       * Save the arguments for the future use.
       */
      
      foreach ($this->vars as $name=>$descr){
	$fav = $this->fav($name);
	if (myPear_db()->columnExists($fav,'zzz_avatars')) $this->textField($name);
      }
      
      foreach($this->args_kept as $a)  $this->textHidden($a,@$this->args[$a]);
      $this->textHidden('_post_bForm_ID_once',$this->ID);
      $this->closeForm('search');
      break;
      
    case True:
      /*
       * We got the search query.
       * Perform the search and print the results (1 line per avatar)
       */
      print x('h2','The search gives the following result');
      $this->show_log_widgets = (myPear_access()->isManager() &&
				 (!$this->search_for_merge or empty($this->args['_after_search'])));
      
      // Run the search engine
      $this->do_search();
      $this->closeForm('');
      
      // Print the log file search results
      foreach ($this->log_widgets as $widget)  print $widget;
      break;
    }
  }

  /*
   * Supply the "$action" URL for 
   * <form action="$action" ...
   */
  protected function bForm_action($myGet=array(),$drop=array()){
    if ($this->search_for_merge) {
      $reply = b_url::same('?function=b_avatarsMerger--executor&resetcache_once=1');
    }else{
      $drop[] = 'ed_bList';
      $this->_add_get_once($myGet);
      $reply = parent::bForm_action($myGet, $drop);
    }
    return $reply;
  }

  /*
   * bForm_action helper, copy arg1_once, etc to the URL
   */
  private function _add_get_once(&$myGet=array()){
    $reply = '';
    foreach(b_url::get_GET('arg._once',True) as $arg){
      $reply.="&$arg=".$_GET[$arg];
      $myGet[$arg]  =  $_GET[$arg];
    }
    return $reply;
  }
  
  /*
   * Decide which "submit" buttons should be shown for the search dialog
   */
  protected function closeForm_buttons($buttons=array()){

    if ($this->got_search_request){
      $reply = array();
      if ($this->search_for_merge){ // Search to get duplicated avatar records to be merged
	if($this->searchNumRows > 1){
	  myPear::INFO("Please check the records which You believe belong to the same person");
	  $reply[] = b_btn::submit('merge the selected records');
	}
      }else{
	$args_kept = array();
	foreach($this->args_kept as $arg){
	  if(!empty($this->args[$arg])) $args_kept[$arg] = str_replace('&','-AMP-',$this->args[$arg]);
	} 
	if($lf=@$this->args['_lookFor']){
	  $reply[] = b_btn::submit("\"create\" a new $lf", 
				   b_url::same(array_merge(array('form'=>bForm_Avatar::$bForm_Avatar,
								 'id'  =>'empty'),
							   $args_kept),
					       array()));
	}
	if ($url = b_url::getReferrer('search_again')){
	  // don't offer "search again" for the search started from the theme template
	  if (empty($_POST['firstname_search_once'])) $reply[] = b_btn::submit(b_url::$getReferrer_comment,
									       b_url::repack($url,$args_kept));
	  b_url::setReferrer('search_again','reset');      
	}
      }
    }else{
      // Regular search dialog, i.e. prompt for first / last name / email
      $reply = $buttons;
    }
    return $reply;
  }
  

  /*
   * Empty for the search
   */
  protected function formOK() {}

  /*
   * Helper
   */
  private function fav($f){
    return str_replace(array('_search','_once'),'',"av_$f");
  }

  /*
   * This function is called after a new avatar is created.
   * Do the search without prompting for the criteria, since they are known.
   */
  public static function show_selection(){

    // Populate _POST array values like they were entered during dialog
    foreach(array_intersect(array_keys(self::$search_items),
			    array_keys($_GET)) as $item){
      if (!empty($_GET[$item])) $_POST[$item] = $_GET[$item];
    }

    // Show the newly created Avatar
    $av = new bForm_Avatar::$bForm_Avatar($_GET['av_id_once']);
    $av->isLocked(True);
    $av->getForm();

    // Show selection
    myPear_search()->search_for_merge = False;
    myPear_search()->show_log_widgets = False;
    myPear_search()->args = array('_after_search' => str_replace(array(',','-AMP-'),'&',$_REQUEST['_after_search']),
				  '_button'       => @$_REQUEST['_button']);
    // Do the search.
    // In principal the search is not needed since we already know the candidate.
    // However, there might be people with the similar names, let's show them all.
    //??? why?    myPear_search()->global_search = True;
    myPear_search()->do_search();
  }

  /*
   * Perform the search
   */
  protected function do_search(){

    // Locate the holder which will print the search results. Try (in this order) 
    // -  current module  
    // -  default holder 
    // Note that for the "global search" (i.e. when creating or merging the Avatar)
    // the holder is always generic one provided by myPear

    if (!b_reg::$current_module)                                       $this->global_search = True;
    if (stripos(@$_REQUEST['_after_search'],'global_search')!== False) $this->global_search = True;
    if (stripos(@$_REQUEST['_after_search'],'add_m_once')   !== False) $this->global_search = True;
    $this->dbg(sprintf("search_for_merge='%b' global_search='%b' show_log_widgets='%b'",$this->search_for_merge,$this->global_search,$this->show_log_widgets),cnf_dev||$this->debug);

    if ($this->holder === Null){
      $holder_ext = (!bAuth::authenticated() || $this->search_for_merge || $this->global_search 
		     ? array('')
		     : array('_'.b_reg::$current_module, ''));
      foreach($holder_ext as $ext){
	$holder = 'bHolder_Avatar'.$ext;
	if(locateAndInclude($holder)){
	  $this->holder = new $holder;
	  $this->dbg('module='.b_reg::$current_module.', holder='.$this->holder,cnf_dev);
	  $this->holder->debug = $this->debug;
	  $this->holder->show_icons = bAuth::authenticated();
	  $this->holder->report_emptySelection = False;
	  $this->holder->noSort = True;
	  break;
	}else{
	  $this->dbg("Unknown holder $holder",cnf_dev);
	}
      }
      if (empty($this->holder)) b_debug::internalError("can't find any holder to show search results");
    }

    if ($this->debug) b_debug::print_r($_POST,__METHOD__."(_POST)");
    if ($this->debug) b_debug::print_r($this->args,"Search args");

    locateAndInclude(bForm_Avatar::$bForm_Avatar);
    bTiming()->cpu(__METHOD__);

    $too_many = False;    
    $leftJoin = (($q = @$this->args['_join']) 
		 ? str_replace(':','=',"LEFT JOIN $q")
		 : '');
    $ident = myPear::$sql_identities[bForm_Avatar::$bForm_Avatar]; 
    $table = $ident['t'];
    
    //
    // Convert _POST to the SQL "WHERE" clause 
    //
    $w_from_post = $where = array();
    if (@$_POST['_join_as'] === 'regexp'){	
      // "wild search", all the elements are merged into a string,
      // i.e. it is not known is the search string a name, an email, etc.
      // So, check everything
      $arg = $_POST['firstname_search_once'];
      if (is_numeric($arg)){
	$w_from_post[] = "av_id = $arg";
      }elseif(SSN::valid($arg,'check_only')){
	$w_from_post[] = "av_ssn = '$arg'";
      }elseif(posix_getpwnam($arg)){
	$w_from_post[] = 'av_identity REGEXP \'[[:<:]]'.$arg.'[[:>:]]\'';
      }else{
	foreach(explode(' ',$arg) as $v){
	  $v  = trim($v);	    if (empty($v)) continue;
	  $ww = array();
	  foreach (array_merge($this->vars,array('email2'=>'')) as $f=>$descr) {
	    $fav = $this->fav($f);
	    if (myPear_db()->columnExists($fav,$table)) $ww[] = myPear_db()->quote($v,'RR',$fav); 
	  }
	  $w_from_post[] = '(' . join(' OR ',$ww) . ')';
	}
      }
    }else{	
      // The search elements are sorted by columns,
      // i.e. we know what is the first name, what is the e-mail
      if ($q = @$this->args['_where']) $where[]= $q; 
      foreach ($this->vars as $f=>$descr) {
	$fav = $this->fav($f);
	if (($v=trim(@$_POST[$f])) && myPear_db()->columnExists($fav,$table)){
	  //	  $str = "LOWER($fav) REGEXP ".($qq = myPear_db()->quote($v,'RR'));
	  //	  if ($f === 'email_search') $str = "($str OR LOWER(av_email2) REGEXP $qq)";
	  $str = myPear_db()->quote($v,'RR',$fav);
	  if ($f === 'email_search') $str = x('(',"$str OR ".myPear_db()->quote($v,'RR','av_email2'));
	  $w_from_post[] = $str;
	}
      }
    }
    // un-escape the string came thru the 'post' method
    if ($w_from_post) $where[] = b_fmt::unEscape('('.join(" AND ",$w_from_post). ')');
    
    if (!empty($where)){
      if ($this->show_log_widgets){
	$this->log_widgets = array(myPear_logs()->loadWidgets_array('open'));
      }
      
      $_GET['sBy']    = 'av_lastname';
      $_GET['sByDir'] = 'ASC';
      $this->holder->where[] = join(' AND ',$where);
      myPear_db()->debug = $this->debug;
      $res = $this->holder->query($only_my_avatars=False);
      myPear_db()->debug = False;
      
      // Walk thru the selected records, print them
      $n_dropped = 0;
      while ($this->holder->rec = myPear_db()->next_record($res)) {
	if ($too_many = ($this->searchNumRows > $this->maxRows)){
	  $n_dropped++;
	}else{
	  $ok = $this->do_print_avatar(); 		
	  if ($ok) $this->searchNumRows++;
	}
      }
      if ($too_many){
	$this->holder->t->prt(array('av_institute'=>b_fmt::redText("... $n_dropped entries are not shown")));
      }
      if (is_object($this->holder->t))	$this->holder->t->close();
    }

    // Close the log widgets
    if ($this->show_log_widgets){
      $this->log_widgets[] = myPear_logs()->loadWidgets_array('close');
    }

    // Print messages if nothing is selected
    $this->dbg("Found $this->searchNumRows avatars");
    if (!$this->searchNumRows) myPear::WARNING("The search gives no result");
    elseif ($too_many)         myPear::WARNING("The search gives too many results, presenting only the first ".$this->searchNumRows." records out of ".($this->searchNumRows+$n_dropped));
    
    // Explain the role icons
    if (myPear_access()->authorized())  bIcons()->explain();
    
    // clean the cache
    $unsets = array_merge($this->args_kept,
			  array('form','unit_.*'));
    b_cnf::_unset('^('.join('|',$unsets).')$',True);

    bTiming()->cpu();
  }
  
  /*
   * Print 1 line for the found avatar using the selected holder.
   */
  private function do_print_avatar() {
    
    bTiming()->cpu(__METHOD__);
    $ok = False;    
    if (!$this->holder->rejectRecord()){
      
      // save the syslog search results, will print it later
      if ($this->show_log_widgets){
	$this->log_widgets[] = myPear_logs()->loadWidgets_array("?av_id=".$this->holder->rec['av_id']);
      }
      
      // format the line and print it
      $this->holder->drawHeader();
      $this->holder->format();
      
      // Add "what to do with the found avatar" as the first item
      if ($this->search_for_merge){
	b_checkBox::set($this->holder->rec['av_id'],'off');
	array_unshift($this->holder->t->extraTD, 
		      b_checkBox::create(array($this->holder->rec['av_id']=>''),'',''));
      }elseif ($_after_search = @$this->args['_after_search']){
	$_after_search_check = @$this->args['_after_search_check'];
	$ea = new bUnit_ea_members(myOrg_ID);
	if ($ea->isMember($this->holder->rec['av_id'])){
	  $ok_show = True;
	}elseif ($_after_search_check && function_exists($_after_search_check)){
	  $ok_show = $_after_search_check($this->holder->rec['av_id'],$_after_search);
	}elseif(function_exists($_after_search_check = b_reg::$current_module.'_after_search_check')){
	  $ok_show = $_after_search_check($this->holder->rec['av_id'],$_after_search);
	}else{
	  $ok_show = True;
	}
	$this->dbg("_after_search='$_after_search'");
	$this->dbg("_after_search_check='$_after_search_check'");
	$this->dbg("ok_show='".var_export($ok_show,True)."'");	
	if ($ok_show){
	  $_after_search = str_replace('AV_ID',$this->holder->rec['av_id'],$_after_search);
	  $this->dbg("_after_search_check='$_after_search_check'");
	  $url = b_url::same("?$_after_search&".b_crypt_no."=1".$this->_add_get_once(),
			     b_url::get_GET('^(.*_search|after_.*)$',True));
	  array_unshift($this->holder->t->extraTD, 
			b_btn::link((($text = @$this->args['_button']) ? $text : "appoint as a new member"),
				    $url));
	}
      }
      if (!bAuth::authenticated()) $this->holder->t->extraTD = array();
      $this->holder->t->prt($this->holder->rec);
      $ok = True;    
    }

    bTiming()->cpu();
    return $ok;
  }
}
