<?php
/**
 * Basic avatar class
 * 
 * Built-in Avatar roles:
 *  - employee of some (may be multiple) organization, see bForm_Organization
 *  - alumnus (i.e. ex-employee) of an organization
 */

/*
 * Social security Number treatment
 */
class SSN{
  
  /* Year */
  static private $yymmdd   =       '(\d\d)([01]\d)([0123]\d)'; // y m d
  static private $yyyymmdd = '([12]\d\d\d)([01]\d)([0123]\d)'; // Y m d

  /* Number */
  static private $n = '\w\d\d\d';
  
  /*
   * We know about the Swedish SSN only.
   */
  public static function known(){
    $reply = array('SE' => array('/^'.self::$yymmdd         .self::$n.'$/', 
				 '/^'.self::$yymmdd  .'[-+]'.self::$n.'$/', 
				 '/^'.self::$yyyymmdd       .self::$n.'$/',  
				 '/^'.self::$yyyymmdd. '-'  .self::$n.'$/',  
				 ));
    return $reply;
  }

  /*
   * Fix known mistakes when filling SSN
   */ 
  public static function fix($ssn){
    $pattern = '/^('.self::$yymmdd.')('.self::$n.')$/';
    if (preg_match($pattern,$ssn,$m)){
      $ssn = $m[1] .'-'. strToUpper($m[5]);
    }

    $pattern = '/^('.self::$yymmdd.')-('.self::$n.')\d*$/';
    if (preg_match($pattern,$ssn,$m)){
      $reply = $m[1] .'-'. strToUpper($m[5]);
    }else{
      $reply = $ssn;
    }
    if ($ssn !== $reply) myPear::WARNING_T("SSN '$ssn' ==> '$reply'");
    return $reply;
  }
  
  /*
   * SE checksum
   */
  private static function checkPnr10($pnr) {
    $pnr = str_replace(array('-','+'), '', (string)$pnr);
    if (strlen($pnr) == 12)   $pnr = substr($pnr, 2);
    if (strlen($pnr) != 10)   return False;
    if (!preg_match('/^'.self::$yymmdd.'/',$pnr,self::$matches)) return False;

    $n = 2;
    $sum = 0;
    for ($i=0; $i<9; $i++) {
      $tmp = $pnr[$i] * $n;
      ($tmp > 9) ? $sum += 1 + ($tmp % 10) : $sum += $tmp; ($n == 2) ? $n = 1 : $n = 2;
    }
    $reply = (bool)(!(($sum + $pnr[9]) % 10));
    //    if($reply)    var_dump($pnr,self::$matches);
    return (bool)$reply;
  }

  /*
   * SSN can't contain string "0000", exception is for provisional numbers
   */
  private static $matches = array();
  public static  $ssn_country = '?';
  public  static function valid($ssn,$verbose=False){

    self::$ssn_country = 'SE';
    $ssn = preg_replace('/^(......)-[pP0]000$/','\\1'.'-P123',(string)$ssn);
    $reply = !empty($ssn) && (strpos((string)$ssn,'000') === False) && (self::checkPnr10($ssn) || preg_match('/^(\d\d)?\d\d\d\d\d\d-[pP]\d\d\d$/',$ssn));
  
  /*						  
    $reply = False;
    if (strpos((string)$ssn,'000') === False){
      foreach(self::known() as $country=>$patterns){
	foreach($patterns as $pattern){
	  if (preg_match($pattern, (string)$ssn,self::$matches)){
	    
	    // Carefull if the age is > 100 years
	    if (strpos($ssn,'+') == 6)   self::$matches[1] -= 100;
	    
	    // Guess the century...
	    if(self::$matches[1] < 1900) self::$matches[1] += 1900;
	    
	    // static $ccc=0; if (++$ccc < 8) b_debug::print_r(self::$matches);
	    self::$ssn_country = $country;
	    $reply = True;
	    break 2;
	    }
	  }
	}
      } 
    */

    // Print a warning if the SSN is not valid & the record is editable
    if ($verbose === 'check_only') $verbose = False;
    if (!$reply && !empty($ssn)){
      $msg = "\"$ssn\" is not a valid Social Security Number";
      if (is_object($av=$verbose)){
	$msg .= ' for '.$av->fmtName('fl');
	if (!$av->isWritable()) unset($verbose);
      }
      if (!empty($verbose) && !empty($msg)) myPear::ERROR($msg);
      // if (cnf_dev) b_debug::xxx($reply ? "Yes, ".self::$ssn_country : "No");
    }
    return $reply;
  }
  
  /*
   * The birthdate is packed to the SSN
   * @param return 
   *   - Null if SSN is not valid
   *   - unix time if SSN is ok
   */
  public static function ssn2birthDate($ssn,$av=False){
    $reply = (self::valid($ssn,$av)
	      ? b_time::noon(self::$matches[1],self::$matches[2],self::$matches[3])
	      : False);
    return $reply;
  }
}

/*
 * Avatar
 */
locateAndInclude('bForm');
class bForm_Avatar extends bForm {

  // Valid types for the portrait photo
  public static $validMimeTypes = array('jpg','jpeg','png','gif');

  public static $vitalFields = array('av_email',
				     'av_firstname',
				     'av_lastname');
  public static $bForm_Avatar = 'bForm_Avatar';
  
  var $TABLE     = 'zzz_avatars';
  var $TABLEID   = 'av_id';
  var $salutation= array(''     =>'',     // Direct salutation to avoid reference tables
			 'Mr.'  =>'Mr.', 
			 'Ms.'  =>'Ms.', 
			 'Mrs.' =>'Mrs.', 
			 'Dr.'  =>'Dr.', 
			 'Ass.Prof.'=>'Ass.Prof.', 
			 'Prof.'=>'Prof.', 
			 );

  private $roles = array();

  /*
   * Sanity. Sort of
   */
  public static function set_context(&$classAvatar,$for_module=Null){

    // What do we have?
    $classAvatarName = (is_object($classAvatar) ? get_class($classAvatar) : $classAvatar);
    if (stripos($classAvatarName,'bForm_Avatar') === False) return;
    
    // The default avatar instance should be the one provided by the current module, 
    // however, we might want a basic Avatar, say for the initial authentication 
    $keep_basicAvatar = (is_bool($for_module) && $for_module);
    if ($keep_basicAvatar) $for_module = myPear_MODULE;
    
    if (!empty(b_reg::$current_module)){
      if (empty($for_module))  $for_module = b_reg::$current_module;
      $wanted_avatar = str_ireplace('_'.myPear_MODULE,'','bForm_Avatar_'.b_reg::$current_module);
      if ((self::$bForm_Avatar != $wanted_avatar) && locateAndInclude($wanted_avatar)){ 
	self::$bForm_Avatar = $wanted_avatar;
	//if (cnf_dev) b_debug::xxx("impose self::\$bForm_Avatar=$wanted_avatar (current_module=".b_reg::$current_module.")");
      }
    }else{
      if (cnf_dev) b_debug::xxx("!!!!! no current module yet");
    }
    
    // Re-instantiate the avatar to the proper module.
    $wanted_avatar = str_ireplace('_'.myPear_MODULE,'',"bForm_Avatar_$for_module");
    if (!locateAndInclude($wanted_avatar)) $wanted_avatar = 'bForm_Avatar';
    
    if ($classAvatarName != $wanted_avatar){
      if (cnf_dev) b_debug::xxx("$classAvatarName --> $wanted_avatar");
      if (is_object($classAvatar)){
	$av_id = $classAvatar->ID;
	$classAvatar= new $wanted_avatar($av_id);
      }else{ 
	$classAvatar = $wanted_avatar;
      }
    }
  }

  /*
   * Find the "duplicated" avatars in the database, i.e. those with the same av_firstname & av_lastname
   */
  public static function findDuplicates($extra_select='',$only_counter=False){
    static $modal_button = 1;

    $q = myPear_db()->qquery("SELECT COUNT(*), av_firstname, av_lastname, av_id, av_email, av_email2, ".
			     " CONCAT(LOWER(av_lastname),LOWER(av_firstname)) AS name FROM zzz_avatars ". 
			     " $extra_select ".
			     " GROUP BY name ".
			     " HAVING COUNT(*)>1 ORDER BY COUNT(*) DESC,name ASC LIMIT 444",cnf_dev);
    if (!$only_counter){
      $t = new b_table_zebra(array('n'  =>'N<br>copies',
				   'btn'=>' ','ln'=>' ','fn'=>' ','email'=>' '));
      $t->class['email'] = 'b_wrap';
    }
    $url  = array('function'       =>'b_avatarsMerger--executor',
		  'resetcache_once'=>1);
    
    $n_duplicates = 0;
    while ($r=myPear_db()->next_record($q)){
      if (myPear::is_empty($av_lastname =preg_replace('/\W/','.*',$r['av_lastname'])) ||
	  myPear::is_empty($av_firstname=preg_replace('/\W/','.*',$r['av_firstname']))){
	if (cnf_dev) b_debug::var_dump($av_firstname,__METHOD__."(av_id=$r[av_id] av_firstname)");
	if (cnf_dev) b_debug::var_dump($av_lastname, __METHOD__."(av_id=$r[av_id] av_lastname)");
	continue;
      }
      
      // Filter out the un-mergeable avatars
      if (strToLower($r['av_lastname'])  == 'cronjob') continue;      
      if (strToLower($r['av_firstname']) == 'cronjob') continue;      
      // Check that the SSNs are not the same
      if (self::findDuplicates_checkSSN($r['av_firstname'],$r['av_lastname'])) continue;
      // Check module-dependant filters (obsolete)
      if (0) if (call_user_func(array(self::$bForm_Avatar,'findDuplicates_reject'),
				array('av_lastname'  =>$av_lastname,
				      'av_firstname' =>$av_firstname)))  continue;
      $n_duplicates++;
      if (!$only_counter){
	$email = array_unique(array_merge(array($r['av_email']),explode(',',$r['av_email2'])));
	
	$dialog_args = array('_post_bForm_class_once'=>'bForm_Avatar_Search',
			     '_post_bForm_ID_once'   =>'new',
			     'lastname_search_once'  =>$av_lastname,
			     'firstname_search_once' =>$av_firstname);
	if ($modal_button){
	  $form = bJS()->modal_iframe(b_url::same(array_merge($url,$dialog_args)),bIcons()->get('i-arrow_join'),Null,True);
	}else{
	  $dialog = array(bIcons()->get(array('i'=>'i-arrow_join','d'=>'Merge !!'),'submit'));
	  foreach($dialog_args as $k=>$v) $dialog[] = "<input type='hidden' name='$k' value='$v' />";
	  $form = join("\n",array("<form action='".b_url::same($url)."' method='post' enctype='multipart/form-data' name='".b_crypt::hash($r['name'])."'>",
				  join("\n",$dialog),
				  "</form>",''));
	}	
	$t->prt(array('btn'=> $form,
		      'x'  => $r['name'],
		      'n'  => $r['COUNT(*)'],
		      'ln' => mb_strToUpper($r['av_lastname'],'UTF-8'),
		      'fn' => b_fmt::ucWords($r['av_firstname']),
		      'email'=>join(' ',$email)));
      }
    }
    if (!empty($t)) $t->close('<br/>');
    return ($n_duplicates > 0);
  }
  
  /*
   * Check that everyone has SSN and those are unique
   */
  protected static function findDuplicates_checkSSN($av_firstname,$av_lastname){
    $q = myPear_db()->query("SELECT av_id,av_ssn FROM zzz_avatars WHERE ".myPear_db()->quote($av_firstname,1,'av_firstname')." AND ".myPear_db()->quote($av_lastname,1,'av_lastname'));

    $av_id = $av_ssn = array();
    while ($r=myPear_db()->next_record($q)){
      if (!empty($r['av_id']))      $av_id[]  = $r['av_id'];
      if (SSN::valid($r['av_ssn'])) $av_ssn[] = $r['av_ssn'];
    }
    $reply = (bool)(count(array_unique($av_id)) == count(array_unique($av_ssn)));
    return $reply;
  }
    
  /*
   *
   */
  public static function findDuplicates_reject($record){ 
    return False;
  }


  /*
   * Wrong concept, to be removed (see $this->isManager)
   */
  public static function isValidUser($av_id){
    static     $users = array();
    if (!isset($users[$av_id])){    
      if ($av = bForm_Avatar::hook($av_id,True))$users[$av_id] = $av->ID;
      else                                      $users[$av_id] = '';
    }
    return $users[$av_id];
  }

  /*
   * generate an unique within myPear e-mail
   */
  static function generate_email($av_firstname,$av_lastname,$must_be_unique=False,
				 $zzz_avatars = 'zzz_avatars',
				 $domain='somewhere.earth', $user='unknown'){
    $name = array();
    $av_firstname = str_replace(' ','.',b_crypt::urlize($av_firstname,' '));
    $av_lastname  = str_replace(' ','.',b_crypt::urlize($av_lastname, ' '));
    if (!empty($av_firstname)) $name[] = $av_firstname;
    if (!empty($av_lastname))  $name[] = $av_lastname;
    if ( empty($name))         $name[] = $user;

    $recipient = join('.',$name);
    $counter = 0;

    while(True){
      $email = strToLower($recipient . ($counter++ ? ".$counter" : '') . "@$domain");
      if ($must_be_unique){
	$w = array();
	foreach(array('av_email','av_email2') as $field){
	  if (myPear_db()->columnExists($field,$zzz_avatars)) $w[] = $field.'  REGEXP \'[[:<:]]'.$email.'[[:>:]]\'';
	}
	
	$q = myPear_db()->query("SELECT av_id FROM $zzz_avatars WHERE ".join(' OR ',$w));
	if (!myPear_db()->num_rows($q)) return $email;
      }else{
	return $email;
      }
    }
  }
  
  /**
   * Search the database for an Avatar corresponding to the 'login' string.
   * The 'login' is interpreted as (in this order)
   *   - av_id (the unique class identifier)
   *   - local account
   *   - email address
   * Create (if not found) and optionally send an email with the account activation request.
   *
   * However, the new record is not created if the 'login' string is already known.  
   *
   * There is often a problem with user provided e-mail(s): people often provide multiple emails,
   * but we need a guaranteed the unique identification. 
   * This is done by using 2 email fields:
   * - av_email keeps the 'main' address (i.e. current), used for sending mails to this person
   * - av_email2 keeps the list of all other addresses declared by the person during the myPear history.
   * The match of any address from av_email + av_email2 defines the Avatar.
   *
   * A typical examples of the "same" address: a@x.com,a@y.com <-> a@y.com,a@x.com <-> a@x.com, etc.
   */
  private static $basic_Avatar  = False;
  private static $return_asArray= False;
  public static function hook($data, $doNotCreate=False){
    bTiming()->cpu(__METHOD__);
    // if (preg_grep('/printPolicies/',bTiming()->call_stack)) b_debug::traceBack();

    // Request to create a "basic" (i.e. without module flavor) Avatar.
    // Mainly needed for the authentication
    if ($doNotCreate === 'basic'){
      $doNotCreate = True;
      self::$basic_Avatar = True;
    }

    // Request to return all the (duplicated) instances which satisfy the query
    self::$return_asArray = ($doNotCreate === 'array');
    if (self::$return_asArray)      $doNotCreate = True;


    // Cancel "create" if there is not enought data
    if (!$doNotCreate &&
	(count(array_intersect(bForm_Avatar::$vitalFields,
			       array_keys(is_array($data) ? $data : array()))) != 
	 count(bForm_Avatar::$vitalFields))){
      if (cnf_CLI && is_array($data) && (count($data)>1)) b_debug::print_r($av_data,"Cancel 'Create Avatar' flag");
      $doNotCreate = True;
    }

    // Replace the emails on input by the tidy values
    $emails = self::tidy_email(is_array($data)
			       ? @$data['av_email'] . ',' . @$data['av_email2']
			       : $data);
    if (!empty($emails)){
      if (!is_array($data)) $data = array();
      $data['av_email']  = $emails['av_email'];
      $data['av_email2'] = $emails['av_email2'];
    }
    
    //
    // Auto-merge avatars with "clashing" e-mails
    //
    $av_pivot = Null;
    foreach (explode(',',@$emails['av_email2']) as $email){
      $avs = self::hookByEmail($email);
      if ($av_pivot === Null)  $av_pivot = array_shift($avs);
      while(count($avs)){
	if ($avs[0]->ID != $av_pivot->ID){
	  if (cnf_show_hook) myPear::DEBUG($avs[0]->getEmail().' >> '.$av_pivot->getEmail());
	  self::merge($avs[0]->ID,$av_pivot->ID);
	}
	array_shift($avs);
      }
    }

    // Hook the avatar if there was no duplicated avatar found
    if (empty($av_pivot)){
      $av_pivot = self::hookExecutor($data, $doNotCreate);
    }else{
      $av_pivot = array($av_pivot);
    }
    bTiming()->cpu();

    self::$basic_Avatar = False;
    if (self::$return_asArray) return $av_pivot;
    else                       return array_pop($av_pivot);
  }


  /*
   * Find the user by his Email. The "old" Emails are also checked
   */
  public static function hookByEmail($emailString,$reply_with_ID=False,$basic_Avatar = False){
    bTiming()->cpu(__FUNCTION__);

    if (cnf_show_hook) b_debug::xxx('');
    if ($basic_Avatar) self::$basic_Avatar = True;
 
    $reply = array();
    // Make search in two steps, try to profit from the indexed column av_email,
    // and only if it fails, then go to the slow regexp search
    $mails = b_fmt::RFC_2822($emailString);
    foreach ($mails as $email){
      foreach(array(" av_email = '$email'",
		    ' av_email2 REGEXP \'[[:<:]]'.$email.'[[:>:]]\'') as $query){
	if (!empty($reply))  continue;
	myPear_db()->debug = cnf_show_hook;
	$q = myPear_db()->query("SELECT av_id,av_email,av_email2,av_firstname,av_lastname FROM zzz_avatars WHERE ".$query);
	myPear_db()->debug = False;
	while ($r=myPear_db()->next_record($q)){
	  if (!empty($reply[$r['av_id']]))  continue;
	  // SICK! The dot is a legal character inside an e-mail address, 
	  // but the MySQL REGEXP interprets it as the word boundary.
	  // Hence we have to check explicitly that the e-mail matches
	  if (!in_array($email,b_fmt::RFC_2822("$r[av_email],$r[av_email2]"))) continue;
	  if ($reply_with_ID){
	    $reply[$r['av_id']] = $r;
	  }else{
	    if (self::$basic_Avatar) $reply[$r['av_id']] = new        bForm_Avatar($r['av_id']);
	    else                     $reply[$r['av_id']] = new self::$bForm_Avatar($r['av_id']);
	  }
	}
      }
    }
    self::$basic_Avatar = False;
    
    // Check the result
    switch ($n=count(array_keys($reply))){
    case 0:  
      if (cnf_show_hook) b_debug::xxx('not found');
      break;

    case 1:    
      break;         

    default: 
      if (cnf_dev||cnf_show_hook) b_debug::xxx("$n objects found - ".join(',',array_keys($reply)));
      // Merge the duplicated avatars
      $ids = array_keys($reply);
      $av_pivot = new bForm_Avatar(array_shift($ids));
      while(count($ids)){
	$av_id = array_shift($ids);
	self::merge($av_id,$av_pivot->ID);
	unset($reply[$av_id]);
      }
    }
    bTiming()->cpu();
    if ($reply_with_ID)  return $reply;
    else                 return array_values($reply);
  }

  /*
   *
   */
  public static $hook_matches = array();
  private static function hookExecutor($data, $doNotCreate){

    $reply = array();
    if (($data === 'new') || ($data === 'empty')){
      $reply['new'] = new bForm_Avatar('empty');
    }elseif(!empty($data)){
      if (is_array($data)) {
	$av_data = $data;
      }elseif (is_string($data) && (strpos($data,'@')!==False)){
	$av_data = array('av_email' => $data); 
      }elseif((is_string($data) || is_int($data)) && ($id=(int)$data)){
	$av_data = array('av_id' => $id); 
      }elseif (is_string($data) && ($localAccount=b_posix::getpwnam($data))){
	// look for the first/last name
	preg_match('/^(.*) (\S+)$/',trim(b_crypt::encode_utf8($localAccount['gecos'])),$m);
	$av_data = (empty($m)
		    ? array()
		    : array('av_firstname'=> $m[1],
			    'av_lastname' => $m[2]));
	$av_data['av_identity'] = $localAccount['name'];
	if (is_object(myOrg())) $av_data['av_email']    = $localAccount['name'].'@'.myOrg_domain;
	$localAccount = ' local account';
      }else{
	$av_data = array();
	if (cnf_dev||cnf_show_hook){
	  b_debug::xxx("Can't interpret the arguments");
	  b_debug::var_dump($data);
	  if (cnf_CLI)debug_print_backtrace();
	  else        b_debug::traceBack('???');
	}
      }
      
      /*
       * av_id is the unique class id, 
       * av_email is also unique, 
       * av_identity is the local username, i.e. is unique on the server,
       * hence those items might be used to identify the Avatar.
       */
      foreach (array('av_id','av_email','av_identity','av_ssn') as $item){
	if (!($hooks=@$av_data[$item])) continue;
	if ($item == 'av_email') $hooks = b_fmt::RFC_2822($hooks);
	else                     $hooks = array($hooks);
	foreach($hooks as $hook){
	  $hook = trim($hook);
	  if ($item=='av_email')      $searchString = "`$item` = '".$hook.'\' OR `av_email2` REGEXP \'[[:<:]]'.$hook.'[[:>:]]\'';
	  elseif($item=='av_identity')$searchString = "`$item` = '".$hook.'\' OR `'.$item.'` REGEXP \'[[:<:]]'.$hook.'[[:>:]]\'';
	  else	                      $searchString = "`$item`='$hook'";
	  
	  // Instantiate the Avatar if it is found in the database
	  $q = myPear_db()->query("SELECT av_id,av_email,av_email2 FROM zzz_avatars WHERE $searchString");
	  while($r = myPear_db()->next_record($q)){
	    if (!empty($reply[$r['av_id']])) continue;
	    if ($item=='av_email'){ 
	      if (!in_array($hook,b_fmt::RFC_2822("$r[av_email],$r[av_email2]"))) continue;
	    }
	    $av = (self::$basic_Avatar
		   ? new        bForm_Avatar($r['av_id'])
		   : new self::$bForm_Avatar($r['av_id']));
	    $reply[$r['av_id']] = $av;
	    
	    $updates = array();
	    foreach ($av_data as $key=>$value){
	      $av->defineVariable(array($key=>$key),'only_if_not_defined');
	      if (!$av->getValue($key,True,True) && $value) $updates[$key] = $value;
	    }
	    if ($updates)   $av->updateDB($updates); // "if" must NOT be removed here (workaround) !!!
	  }
	}
      }
      
      if (empty($reply)){
	//
	// The avatar was not reliably hooked...
	// Now either create the Avatar object if all the vital fields are provided,
	// or try to match using the non-reliable information
	//
	$av = Null;
	self::$hook_matches = array();
	if (!empty($av_data)){
	  // check sanity
	  $av_data['av_email'] = strToLower(trim(@$av_data['av_email']));
	  if ($v=@$av_data['av_firstname']) $av_data['av_firstname'] = b_fmt::ucwords($v);
	  if ($v=@$av_data['av_lastname'])  $av_data['av_lastname']  = b_fmt::ucwords($v,True);
	  
	  switch($doNotCreate) {
	  case True:        // Catch the closes match
	    $search = array();
	    foreach($av_data as $k=>$v) if (!empty($v)) $search[] = myPear_db()->quote($v,1,$k);
	    if (!empty($search)){
	      $q = myPear_db()->query("SELECT * FROM zzz_avatars WHERE ".join(' AND ',$search). " LIMIT 33");
	      while($r = myPear_db()->next_record($q)){
		if (empty($r['av_id'])) continue;
		self::$hook_matches[$r['av_id']] = $r;
		$reply[$r['av_id']] = (self::$basic_Avatar
				       ? new        bForm_Avatar($r['av_id'])
				       : new self::$bForm_Avatar($r['av_id']));
	      }
	    }
	    break;
	    
	  case False:	// Create the avatar object from the input data
	    $missings = array();
	    foreach (bForm_Avatar::$vitalFields  as $item){
	      if (empty($av_data[$item])) $missings[] = $item;
	      $dataDbg[$item] = @$av_data[$item];
	    }
	    if (empty($missings)){
	      $av = new bForm_Avatar('empty');
	      $av->updateDB($av_data);
	      locateAndInclude('APIaccess');
	      if (is_object(APIaccess::$cache))  APIaccess::$cache->reset();
	      if (cnf_show_hook) myPear::DEBUG(@$localAccount . ' ' . b_fmt::joinX(',',$dataDbg)." ==> $av");
	      $reply[$av->ID] = $av;
	    }
	  }
	}
      }
    }
    // Print a warning if the answer is ambigious
    if (($n=count($reply)) > 1){
      if (is_array($data)) $data = b_fmt::joinX(',',$data);
      myPear::WARNING_T('??? '.__METHOD__."($data) gives back $n answers");
      if (cnf_dev) foreach($reply as $k=>$av) b_debug::xxx("??? reply-$k = $av");
    }
    return array_values($reply);
  }
  
  /*
   *
   */
  public static function delete($where){
    if (!myPear::$isReady){
      myPear_startup_queue(__METHOD__,$where,True);
    }else{
      $n = 0;
      $q = myPear_db()->query("SELECT av_firstname,av_lastname FROM zzz_avatars WHERE $where");
      while($r=myPear_db()->next_record($q)) myPear::MESSAGE_forced(sprintf(__METHOD__."(%s) <em>%d</em>",self::_fmtName('Lf',$r),(++$n)));
      
      // Invoke the module specific function
      foreach(bMaster()->get_masterModules() as $module){
	$c = "bForm_Avatar_$module";
	$f = __FUNCTION__.'_deps';
	if (locateAndInclude($c)){
	  // Delete the dependances
	  if (is_callable("$c::$f")) call_user_func("$c::$f",$where);
	}
      }
      myPear_db()->qquery("DELETE FROM zzz_avatars WHERE $where",True);
    }
  }

  /*
   * Merge 2 Avatar objects
   */
  public static function merge($avDrop_ID,$avKeep_ID,$dryRun=False,$tablesToConsider=array()){
    $verbose = (bool)(cnf_show_db||cnf_show_mrg||cnf_CLI);
    if ($avDrop_ID === $avKeep_ID) return;
    
    $avKeep = new bForm_Avatar::$bForm_Avatar($avKeep_ID); 
    $avKeep->setPWD($pwd='',$updateDB=False);

    // Delete the "toDrop" record
    if (is_array($avDrop_ID)){
      $avDrop_formDB = $avDrop_ID;
      $avDrop_ID     = $avDrop_formDB['av_id'];
      $toDelete = 0;
    }elseif (is_object($avKeep) && is_object($av = myPear::getInstance(bForm_Avatar::$bForm_Avatar,$avDrop_ID,array('strict','nocache')))){
      $avDrop_formDB = $av->formDB;
      $avDrop_ID = $toDelete = $av->ID;
    }else{
      return;
    }

    // Check that the data will fit the newly merged record
    $dataNotFit = array();
    foreach(array('av_id','av_pwd','av_email') as $f){
      $values = array();
      foreach(array(@$avDrop_formDB[$f],
		    @$avDrop_formDB[$f.'2'],
		    @$avKeep->formDB[$f],
		    @$avKeep->formDB[$f.'2']) as $v){
	if (myPear::is_empty($v = trim($v))){
	  $values = array_merge($values,preg_split('/[ ,;]+/',$v));
	}
      }
      $value = join(',',array_unique($values));
      if ($msg = myPear_db()->dataDoesNotFit('zzz_avatars',$f.'2',$value)) $dataNotFit[] = $msg;
    }
    if (!empty($dataNotFit)){
      myPear::WARNING("Sorry, but it is not possible to merge Avatars $avDrop_ID & $avKeep_ID, the merged data does not fit the database".
		      x("ul",b_fmt::joinMap("li",$dataNotFit)));
      return;
    }
    
    if ($toDelete){
      myPear_db()->qquery("DELETE FROM zzz_avatars WHERE av_id='$toDelete'",$verbose);
    }

    // Fill the empty new record
    if ($avKeep_ID == 'new'){
      $avKeep->updateDB($avDrop_formDB);
      $avKeep_ID = $avKeep->ID;
    }

    // Merge the ID and passwords to the new record
    $changes = $updates = $data = array();
    foreach (array('av_id'  =>'av_id2',
		   'av_pwd' =>'av_pwd2') as $k=>$d){
      $kk = ($k=='av_pwd' ? 'av_pwd' : 'av_id');
      foreach(array($avKeep->getValue($k,1,1),
		    $avKeep->getValue($d,1,1),
		    @$avDrop_formDB[$d],
		    @$avDrop_formDB[$k]) as $item){
	if ($item=trim($item)) @$data[$kk] .= ' '.$item;
      }
    }

    foreach ($data as $k=>$v) {
      $data[$k] = array_unique(preg_split('/[ ,;]+/',trim($v)));
      $updates[$k] = array_shift($data[$k]);
      if ($data[$k]) $updates[$k.'2'] = join(',', $data[$k]);
    }
    foreach($updates as $k=>$v) $changes[] = "<li>".$avKeep->getDescr($k).": $v</li>";
    
    $set_max_values = array('av_lastlogin',
			    'av_timestamp',
			    );
    // reuse information from the dropped record
    foreach($avDrop_formDB as $key=>$vDrop){
      if (empty($vDrop) || !empty($updates[$key])) continue;
      if ($vKeep=$avKeep->getValue($key,1,1)){
	if (in_array($key,$set_max_values)) $updates[$key] = max($vKeep,$vDrop);
      }else{
	$updates[$key] = $vDrop;
      }
      if (!empty($updates[$key])) $changes[] = "<li>".$avKeep->getDescr($key).": ".$updates[$key]."</li>";
    }
    
    // Say "Merging avatars"
    $msg = "Merging avatars:".
      "<br/>keep $avKeep:".$avKeep->getEmail()." ".$avKeep->fmtName('fl').
      "<br/>drop $avDrop_ID:".$avDrop_formDB['av_email']." ".$avDrop_formDB['av_firstname']." ".$avDrop_formDB['av_lastname'];
    if ($changes) myPear::MESSAGE((cnf_CLI 
				   ? "Merging avatars $avDrop_ID -> $avKeep_ID" 
				   : "$msg<ul>".join('',$changes)."</ul>"));
    
    
    // Merge e-mail
    $tidy_email = self::tidy_email(join(',',array($avKeep->getValue('av_email', 1,1),
						  $avKeep->getValue('av_email2',1,1),
						  @$avDrop_formDB['av_email'],
						  @$avDrop_formDB['av_email2'])));
    // merge in this order (the last one "wins")
    $avKeep->updateDB(array_merge($updates,
				  $tidy_email),$verbose);
    
    // Check the lists, catch the situation when both the "old" and the "new" objects are parents 
    // Reassign the "old members" to the "new parent"
    if (myPear_db()->num_rows($qq=myPear_db()->query("SELECT l_id FROM zzz_lists WHERE l_parent = $avKeep_ID"))){
      while($rr=myPear_db()->next_record($qq)){
	$l_id = $rr['l_id'];
	$q = myPear_db()->query("SELECT l_id FROM zzz_lists WHERE l_parent = $avDrop_ID");
	while($r=myPear_db()->next_record($q)){
	  myPear_db()->qquery("UPDATE zzz_list_members SET lm_lid = $l_id WHERE lm_lid = $r[l_id]",$verbose);
	  myPear_db()->qquery("DELETE FROM zzz_lists WHERE l_id = $r[l_id]",$verbose);
	}
      }
    }
    
    // Update all the other objects in the database
    $a = array_merge(array('zzz_lists'       =>array('l_parent'), 
			   'zzz_units'       =>array('u_parent')),
		     self::avid_tables($tablesToConsider));
    foreach($a as $table=>$keys){
      foreach($keys as $key) myPear_db()->qquery("UPDATE $table SET $key = '$avKeep_ID' WHERE $key = '$avDrop_ID'",$verbose); 
    }

    // Update CV lists
    locateAndInclude('bList_eaCV');
    locateAndInclude('bForm_Organization');
    foreach (bForm_Organization::getAll() as $org){
      $eaCV = new bList_eaCV($org);
      myPear_db()->qquery("UPDATE zzz_list_members SET lm_key = '$avKeep_ID' WHERE lm_key = '$avDrop_ID' AND lm_lid = ".$eaCV->ID,$verbose); 
    }


    // Eventually change the maildb reference 
    if (locateAndInclude('legacy_maildb')){
      list($cmd,$cmd_h) = legacy_maildb::maildb_cmd('-list',array('user','m_myorg'));
      $cmd .= " | grep -E LF.$avDrop_ID\$";
      if ($maildb_entry = trim(`$cmd`)){
	list($maildb_name,$x,$y) = explode('-LF-',$maildb_entry);
	list($cmd,$cmd_h) = legacy_maildb::maildb_cmd($maildb_name,array('-set',"m_myorg=$avKeep_ID"));
	b_os::exec($cmd,'>>>');
      }
    }
  }
  
  /*
   * build the list of _avid fields
   */
  public static function avid_tables($tablesToConsider=array()){

    if (empty($tablesToConsider)) $tablesToConsider = myPear_db()->getTables();

    $columnsToConsider = array('_avid','_parent','log_form');
    foreach ($tablesToConsider as $table){
      foreach (myPear_db()->getColumns($table) as $c){
	if (preg_match('/('.join('|',$columnsToConsider).')$/',$c)) $list[$table][] = $c;
      }
    }
    return $list;
  }
    
  /*
   * Avatar constructor
   */
  function __construct($id='empty', $mode='RO') {
    parent::__construct($id, $mode);

    // Lock the modal form editing unless there just one block requested
    $this->isLocked(cnf_inside_fb && !isset($_GET['block_once']));

    // define the Avatar variables
    $this->defineVariables();
  }
  
  /*
   * Define basic variables. 
   * See also method "defineVariables_fine_tuning"
   */
  protected $_defineVariables_Avatar = 0;
  function defineVariables() {
    if ($this->_defineVariables_Avatar++) return;
    $this->defineVariable(array('av_ssn'             => 'Social Security Number',
				
				'av_salutation'      => 'Salutation',
				'av_firstname'       => 'First Name',
				'av_lastname'        => 'Last Name',
				'av_von'             => 'Nobiliary particle',
				'av_photo'           => 'Portrait Photo',
				
			       '_av_life'            => ' ',
				'av_sex'             => 'Gender',
				'av_birthdate'       => 'Birth Date',
				'av_ddate'           => 'Death',
				'av_citizenship'     => 'Citizenship',
				'av_birthcountry'    => 'Birth Country',
				'av_birthplace'      => 'Birth City',
				
				'av_residentship'    => 'Country',
				'av_city'            => 'City',
				'av_zip'             => 'Zip/Postal code',
				'av_address'         => 'Street address',
			       '_av_address'         => 'Street address',
				
				'av_ms_year'         => 'Year',
				'av_ms_country'      => 'Country',			
				'av_ms_institute'    => 'Institution',
				
				'av_phd_year'        => 'Year',
				'av_phd_country'     => 'Country',
				'av_phd_institute'   => 'Institution',
				
				'av_email'           => 'E-mail',
				'av_institute'       => 'Present Institution',
				'av_position'        => 'Present Position',
				'av_http'            => 'Personal web page',
				'av_phone'           => 'Telephone Number',
				
				'av_email2'          => 'E-mails',
				
				'av_id'              => 'av_id',
				'av_id2'             => 'av_id2',
				'av_pwd'             => 'av_pwd',
				'av_pwd2'            => 'av_pwd2',
				'av_identity'        => 'av_identity',
				'av_timestamp'       => 'av_timestamp',
				'av_lastlogin'       => 'av_lastlogin',
				),True);
    
    $this->defineTypes(array('isVITAL' =>bForm_Avatar::$vitalFields, 
			     'isMBE'   =>array('av_von',
					       'av_ssn',
					       'av_id2',
					       'av_email2',
					       'av_pwd',
					       'av_pwd2',
					       'av_phone',
					       'av_birthdate',
					       'av_birthcountry',
					       'av_birthplace',
					       'av_ddate',
					       'av_salutation',
					       'av_http',
					       'av_sex',
					       'av_position',
					       'av_residentship',
					       'av_citizenship',
					       'av_address',
					       'av_zip',
					       'av_city',
					       'av_ms_year',
					       'av_ms_country',
					       'av_ms_institute',
					       'av_phd_year',
					       'av_phd_country',
					       'av_phd_institute',
					       'av_photo'),
			     'isRO'    =>array('av_pwd',
					       'av_pwd2',
					       'av_identity',
					       'av_lastlogin',
					      '_av_life'),
			     'isINTERNAL'=>array(//'av_email',
						 'av_zip',
						 'av_residentship',
						 'av_address'), 
			     'isPRIVATE' =>array('av_sex',
						 'av_ssn',
						 '_av_address'
						 ), 
			     'isFORMAT'  =>array('av_institute'=>'b_fmt::longString',
						 'av_address'  =>'b_fmt::longString'),
			     'expected_ext'  =>array('av_photo' => ''), // no restriction
			     ));
  }

  /*
   * Do this afterwards, when MBEs are defined
   */
  protected $_defineVariables_fine_tuning = 0;
  protected function defineVariables_fine_tuning(){    
    if ($this->_defineVariables_fine_tuning++) return;

    $this->dbg();
    $this->defineVariables();
 
    $compulsory = bForm_Avatar::$vitalFields;
    if ($this->askSex())          $compulsory[] = 'av_sex';
    foreach($compulsory as $item){
      $this->isMBE[$item] = False; 
    }
    
    $this->expected_ext['av_photo'] = self::$validMimeTypes;
    
    // Replace the word 'Year' by 'Status' for the missing study 
    foreach(array('av_ms_year','av_phd_year') as $f){
      $v = parent::getValue($f,1,1);
      if (($this->MODE == 'RO') && ($v == CONST_year_not_considered)) $this->defineVariable('Status',$f);
    }
    
    // Force the study information to be provided
    if ($this->askMS()){
      //      $this->getValue('av_ms_year',1,1);
      $this->isMBE['av_ms_year'] = False;
      //      $this->isMBE['av_ms_institute'] = $this->isMBE['av_ms_country'] = !$this->year2date_isset;
    }
      
    if ($this->askPhD() && $this->MS_completed()){
      //      $this->getValue('av_phd_year',1,1);
      $this->isMBE['av_phd_year'] = False;
      //      $this->isMBE['av_phd_institute'] = $this->isMBE['av_phd_country'] = !$this->year2date_isset;
    }
  }
  
  /*
   *
   */
  protected function _recordCreated() {
    myPear_logs()->add('newcomer',$this->ID);
  }

  /*
   * Set the primary Email address.
   * Check the consistency of e-mail and split multi-address to 2 fields.
   * Example -  av_email=>"a@x.com, b@y.com" 
   * becomes    av_email=>"a@x.com", av_email2=>a@x.com a@y.com" (yes, av_email2 contains av_email - it might be changed)                 
   */
  public function setEmail($email='',$only_checkValidity=False){
    $reply = False;
    $emails = array();
    if ($e0 = $email)                             $emails[] = $e0;
    if (!$only_checkValidity){
      if ($e1 = $this->getValue('av_email', 1,1))   $emails[] = $e1;
      if ($e2 = $this->getValue('av_email2',1,1))   $emails[] = $e2;
    }
    if (count($emails) == 1)                      $emails[] = $emails[0];

    if ($tidy_emails = self::tidy_email($emails)){
      $already_ok = True;
      foreach($tidy_emails as $key=>$email){
	if (!$only_checkValidity && $this->getValue($key,1) != $email) $already_ok = False;
	foreach(self::hookByEmail($email,$reply_with_ID=True) as $av_id=>$r){
	  if ((int)$av_id !== (int)$this->ID){
	    $errors[] = ($msg="Email '$email' is already used by ".x('em',self::_fmtName('fL',$r)));
	    if (cnf_CLI) print "$msg\n";
	    else         myPear::ERROR($msg,$this);
	  }
	}
      }

      $no_errors = empty($errors);
      if ($only_checkValidity){
	$reply = $no_errors;
      }elseif ($no_errors && !$already_ok){
	foreach($tidy_emails as $key=>$email){
	  $this->getValue_reset_forced($key,$email);
	  $this->dbg("$key=$email");
	}
      }
    }elseif ($only_checkValidity){
      $reply = False;
    }elseif((int)$this->ID){
      $url = bJS()->modal_ajax(b_url::same("?form=".self::$bForm_Avatar."&id=$this->ID&mode=RO"),'[see]');
      myPear::WARNING_T("non-valid email '".join(',',array_values($tidy_emails))."' $url",$this);
    }
    return $reply;
  }
  
  /*
   * Validate fields specific for the Avatar class
   */
  protected function getValue_validate_empty($k,&$v){
    $err_msg = '';
    if (myPear::is_empty($v)){
      switch($k){
      case 'av_photo':
	// No new photo means "keep the existing photo",
	// so set the cache to the existing value 
	if ( empty($this->formDB[$k])) $this->formDB[$k] = $this->getPhoto(array());
	if (!empty($this->formDB[$k])) $this->getValue_reset($k,$this->formDB[$k]);
	unset($_POST[$k]);
	break;
	
      default:
	$err_msg = parent::getValue_validate_empty($k,$v);
      }
    }
    return $err_msg;
  }

  /*
   *
   */
  protected $bad_SSN = False;
  protected function getValue_validate_value($k,&$v){
    $err_msg = '';
    if (!myPear::is_empty($v)){
      $err_msg_parent = $err_msg = parent::getValue_validate_value($k,$v);
      
      switch($k){
      case 'av_photo':
	// The file name of the portrait photo is the e-mail, hence the latter must exist 
	if (empty($this->formDB['av_email']) || !(int)$this->ID){
	  $err_msg = 'Email is not known, cant get photo';
	}else{
	  $img = $this->getPhoto(array('av_photo'=>$v));
	  if (empty($img)){
	    myPear::WARNING_T("'$v' is not a valid image file");
	    $this->getValue_reset($k,'');
	  }
	  $err_msg = '';
	}
	break;
	
      case 'av_email':
	if (!$this->setEmail($v,$only_checkValidity=True)) $err_msg = "Not valid";
	break;
	
      case 'av_ssn':
	if (!SSN::valid($v,$this)){
	   $err_msg = "SSN '$v' is not valid ";
	   $this->bad_SSN = $v;
	   if ($this->show_gv||$this->debug)$this->dbg($err_msg,True);
	}
	break;
	
      default:
      }

      if (!empty($err_msg) && ($err_msg_parent !== $err_msg)){
	$this->isRO[$k] = False;
	$this->dbg(empty($err_msg)?'OK':$err_msg,cnf_dev);
      }
    }
    if (!empty($err_msg)) $v = Null;
    return $err_msg;
  }

  /*
   *
   */
  function __clean_cache(){
    myPear::getInstance(get_class($this),$this->ID,'clean');
  }

  /*
   * s - salutation
   * f - first name
   * l - lastname
   * F - FIRSTNAME
   * L - LASTNAME
   */
  function name($fmt='fl') {
    $reply = $this->fmtName($fmt);
    return $reply;
  }

  public function fmtName($format='Lf',$escape=True){
    return self::_fmtName($format,
			  array('av_id'        => $this->ID,
				'av_salutation'=> $this->getValue('av_salutation',1,1),
				'av_firstname' => $this->getValue('av_firstname',1,1),
				'av_lastname'  => $this->getValue('av_lastname',1,1),
				'av_von'       => $this->getValue('av_von',1,1)),
			  $escape);
  }

  public static function _fmtName($format='Lf',$rec=array(),$escape=True){
    if (empty($rec['av_firstname']) && empty($rec['av_lastname'])) return '';
    $reply = array();
    $rec['av_firstname'] = trim(strip_tags($rec['av_firstname']));
    $rec['av_lastname']  = trim(strip_tags($rec['av_lastname']));
    for($n=0; $n<strlen($format); $n++){
      switch($item=substr($format,$n,1)){
      case 's': if ($s=@$rec['av_salutation'])$reply[] = bText::_(array('item'=>$s,'key'=>"_$s")); break;
      case ',': $reply[] = ',';                                          break;
      case 'i': $reply[] =          'id='.$rec['av_id'];                 break;
      case 'f': $reply[] = b_fmt::ucWords($rec['av_firstname']);         break;
      case 'F': $reply[] =  mb_strToUpper($rec['av_firstname'],'utf-8'); break;
      case 'X': $reply[] = '<br/>';                                      break;
      case '<': if (stristr($format,'<br/>')!==False)$reply[] = '<br/>'; break;
      case 'l': 
      case 'L': 
	if ($v = @$rec['av_von']) $reply[] = $v;
	switch($item){
	case 'l': $reply[] = b_fmt::ucWords($rec['av_lastname'],True);     break;
	case 'L':
	  $text =  mb_strToUpper($rec['av_lastname'],'utf-8');  
	  if (strpos($text,"'T ") === 0) $text = preg_replace("/^.T /","'t ",$text);  // 't Hooft ... 
	  $reply[] = $text;
	  break;
	}
	break;
      default:
      }
    }
    
    if ($reply[count($reply)-1] == '&#44;') array_pop($reply);
    if ($reply[count($reply)-1] == ',')     array_pop($reply);
    $name = str_replace('- ','-',str_replace(' ,',',',str_replace('> ','>',join(' ',$reply))));
    if (cnf_CLI) $escape = False;
    if ($escape) $name = no_wrap(b_fmt::escape($name,'<'));
    return $name;
  }
  
  /*
   * In the new approach the each method must have its own Avatar class
   */
  function role(){
    return array();
  }

  function roles(){
    return array();
  }

  function isSuperUser(){
    return myPear::_UNIT(RANK__superuser)->isMember($this);
  }
  
  function isOwner($av_id=Null) { 
    if(empty($av_id))return b_cnf::check_conditions(array(True=>array('auth'=> (is_object(bAuth::$av) && ($this->ID == bAuth::$av->ID))),
							  'default' => parent::isOwner()),
						    $this);
    else             return b_cnf::check_conditions(array(True=>array('auth'=> (is_object(bAuth::$av) && ($this->ID == $av_id))),
							  'default' => False),
						    $this);
  }
  
  function isReadable() { 
    $reply='always readable';
    return @$reply;
  }

  function getInst(){
    return $this->getValue('av_institute',True,True); 
  }

  function getEmail(){
    return $this->getValue('av_email',True,True); 
  }
  
  function getEmails(){
    return b_fmt::RFC_2822($this->getValue('av_email',True,True).','.
			   $this->getValue('av_email2',True,True));
  }
  
  function getLastLogin(){
    return $this->getValue('av_lastlogin',True,True);
  }
  
  function setLastLogin(){
    $this->updateDB(array('av_lastlogin'=>time()));
  }
  
  /*
   * Check that the user has a local account
   */
  static function knownIdentity($av_identity){
    $reply = False;
    $av_identity = trim($av_identity);
    if (!empty($av_identity) && preg_match('/^[a-zA-Z0-9]*$/',$av_identity)){
      $q = myPear_db()->query("SELECT av_id,av_identity FROM zzz_avatars WHERE av_identity REGEXP '$av_identity'");
      while ($r=myPear_db()->next_record($q)){
	if ($reply = in_array($av_identity,self::_getIdentity($r))) break;
      }
    }
    b_debug::xxx($reply);
    return $reply;
  }

  /*
   * Return local account(s) username(s)
   */
  function getIdentity(){
    return self::_getIdentity(array('av_identity'=>$this->getValue('av_identity',True,True))); 
  }
  
  static function _getIdentity($buffer){
    return (empty($buffer['av_identity'])
	    ? array()
	    : explode(',',$buffer['av_identity']));
  }
  
  /*
   * Set local account(s)username(s).
   */
  function setIdentity($identity=array(),$verbose=False){
    if (!is_array($identity)) $identity = explode(',',$identity);

    // Sanity
    $args_ok = False;
    foreach($identity as $login) if (posix_getpwnam($login) !== False) $args_ok = True;
    if (!$args_ok){
      myPear::WARNING_T("Refuse to set the identity ".x('"',join(',',$identity)));
      return;
    }

    // Reset the identity if changed
    $identity = array_unique(array_merge($identity,$this->getIdentity()));
    sort($identity);
    
    if (($old_identity=$this->getIdentity()) != $identity){
      $reply = $this->updateDB(array('av_identity'=>implode(',',$identity)));
      if (cnf_dev || $verbose) myPear::MESSAGE($this->fmtName().": av_identity '".implode(',',$old_identity)."'->'".implode(',',$identity)."'");
    }
    return $old_identity;
  }
  
  function isLocal() { 
    $reply = False;
    foreach($this->getIdentity() as $identity){
      if (b_posix::getpwnam($identity)) $reply = True;
    }
    return $reply;
  }

  function remind_password(){
    if ($msg=bAuth()->remind_password($this->getEmail())){
      myPear::ERROR($msg);
    }
  }
  
  /*
   * The form Header
   */
  protected function getFormHeader(){

    // Complete the definition of variables used during the dialog
    $this->defineVariables_fine_tuning();

    // SSN has precedence over other fields, hence get it first
    $this->getValue('av_ssn');

    if ((int)$this->ID){
      $this->defineTypes(array('isRW_ifMissing'=>array('av_ssn'      => $this->ask_av_SSN(),
						       'av_birthdate'=>!$this->ask_av_SSN(),
						       'av_photo'    => True),
			       'isRO'          =>array('av_birthdate'=>!myPear::is_empty($this->getValue('av_ssn'))),
			       ));
      if (!$this->isEmbedded()){
	myPear::H1($this->name(),array('noTranslate','reset'));
      }
    }
  }
  
  /*
   * The form Body
   */
  protected function getFormBody(){
    $this->whoAmIBlock();
    $this->contactsBlock();
    $this->studyBlock();
    $this->residentshipBlock();
  }
  
  protected function needFullInformation(){ 
    return False;
  }

  /*
   * Ask the SSN for those countries where we know how to interpret SSN
   */
  protected function ask_av_SSN(){
    $reply = False;
    // Sick! bAuth::authenticated() should not be used, it might trigger an infinite loop
    if (class_exists('bAuth',False) && is_object(bAuth::$av)){
      $av_ssn = $this->getValue('av_ssn',True);
      $reply  = (!empty($av_ssn) || !empty($this->bad_SSN));
      if (!$reply){
	foreach(array('av_residentship','av_citizenship','av_birthcountry','av_ms_country','av_phd_country') as $f){
	  if ($c = $this->getValue($f,True,True)){
	    if (in_array(              $c, array_keys(SSN::known()))) $reply = True;
	    if (in_array(b_cc::getName($c),array_keys(SSN::known()))) $reply = True;
	    if ($reply) break;
	  }
	}
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  protected function askBirthPlace()  { return False; }

  protected function askSalutation()  { return True; }

  protected function askInstitute()   { return True; }

  protected function askSex()         { return False; }

  protected function askHomeAddress() { return False; }

  protected function askResidentship(){ return False; }

  function askPhD()                   { return False; }

  function askMS()                    { return $this->askPhD(); }
  
  /*
   *
   */  
  protected function blockTitle_skip($block_ID){ 
    switch($block_ID){
    case 'av_birth_block':			
      return !($this->isOwner() || $this->isManager()) || self::$inputToPDF;
      break;

    default: 
      return parent::blockTitle_skip($block_ID);
    }
  }
  
  /*
   * This is private information, show it only for the authorized viewers
   */  
  function residentshipBlock(){

    if (!$this->isWritable()) return;
    $this->defineVariables_fine_tuning();

    $home = array('i'=>'bb-home',
		  'l'=>$this->sameURL_but(array('mode'=>'RW','av_id'=>$this->ID)));
    if ($this->askHomeAddress()){
      foreach(array('values','methods') as $item){
	$$item = array();
	foreach(array('av_residentship'=>'textFieldCountry',
		      'av_zip'         =>'textField',
		      'av_city'        =>'textField',
		      'av_address'     =>'textArea',
		      ) as $name=>$method){
	  if ($item == 'methods'){
	    $methods[$name] = array($method);
	    $methods[$name] = $method;
	  }else{
	    $v = $this->getValue($name,True);
	    switch($name){
	    case 'av_residentship': $v = b_cc::getName($v); break;
	    case 'av_zip':          $v = trim($v .' '.$this->getValue('av_city',True)); break;
	    case 'av_city':         $v = Null; break;
	    default:	
	    }
	    if (!empty($v)) $values[] = $v;
	  }
	}
      }
      $this->formDB['_av_address'] = join('-BR-',$values);
      $this->formBlock('address',
		       'Residential Address',
		       ($this->MODE == 'RW' ? $methods : array('_av_address')),
		       $home);
    }elseif($this->askResidentship()){
      $this->formBlock('address',
		       'Residentship',
		       array('av_residentship'=>array('textFieldCountry')),
		       $home);
    }
  }
  
  /*
   *
   */
  function avInfoBlock(){
    $this->whoAmIBlock();
    $this->contactsBlock();
  }

  /*
   *
   */
  protected function passed_away(&$dialog){
    if ($reply = $this->getValue('av_ddate',True,True)){
      // The person passed_away...
      if ($this->MODE == 'RO'){
	$this->getValue_reset('_av_life',b_time::period($this->getValue('av_birthdate',1),$reply));
	foreach(array('av_birthdate','av_ddate','av_sex','av_ssn','av_institute') as $f) unset($dialog[$f]);
      }else{
	foreach(array('_av_life',) as $f) unset($dialog[$f]);
      }

      // Remove and/or redefine some questions
      foreach(array('av_phone','av_citizenship',) as $f) unset($dialog[$f]);
      foreach(array('av_institute','av_position') as $f) $this->defineVariable(array($f=>preg_replace('/Present/i','Recent',$this->vars[$f])),True);
      $this->defineVariable(array('av_email' => 'Assistant\'s E-mail'),True);

    }else{
      // Life continues...
      unset($dialog['_av_life']);
    }
    return $reply;
  }

  /*
   *
   */
  function whoAmIBlock(){

    $this->defineVariables_fine_tuning();

    $whoAmI = array('av_photo'       =>array('uploadPhoto'),
		   '_av_life'        =>array('textField'), 
		    'av_firstname'   =>array('textField'),
		    'av_von'         =>array('textField'),
		    'av_lastname'    =>array('textField'),
		    'av_salutation'  =>array('selectMenu',$this->salutation),
		    '_spacer1'       =>array('textField'),
		    '_spacer2'       =>array('textField'),
		    'av_birthdate'   =>$this->year2date('av_birthdate','dialog'),
		    'av_birthplace'  =>array('textField'),
		    'av_birthcountry'=>array('textFieldCountry'),
		    'av_ddate'       =>$this->year2date('av_ddate','dialog'),
		    'av_citizenship' =>array('textFieldCountry'),
		    '_spacer3'       =>array('textField'),
		    'av_sex'         =>array('radioButton',array('F','M')),
		    'av_ssn'         =>array('textField'),
		    );

    // Fine tuning...
    if (!EA_manager_here && $this->MODE == 'RW') unset($whoAmI['av_ddate']);
    $passed_away = $this->passed_away($whoAmI);

    if (!$this->isWritable() || $this->MODE == 'RO'){
      foreach(array('av_salutation','av_firstname','av_lastname','av_von') as $f){
	if (myPear::is_empty($v=$this->getValue($f,1,1)) && @$this->isVITAL[$f]) continue;
	unset($whoAmI[$f]);
      }
    }

    if (!$this->ask_av_SSN())              {unset($whoAmI['av_ssn']); $this->isMBE['av_ssn'] = True; }
    if (!$this->askSalutation())        unset($whoAmI['av_salutation']);
    //    if (!$this->isOwner() && !$this->isManager()) unset($whoAmI['av_sex']);
    
    $this->formBlock('av_birth_block',
		     ($passed_away ? '&nbsp;' : 'Who Am I'),
		     $whoAmI,
		     array('d'=>'update birth info',
			   'i'=>'i-stork',
			   'l'=>$this->sameURL_but(array('mode'=>'RW','av_id'=>$this->ID))));
    
    // Try the thumbnail as the avatar icon, 
    // if the latter is not available use mail-female icon from the pool
    static $thumbs=array('F'=>'user-female0',
			 'M'=>'user-male',
			 'X'=>'i-avatar');
    
    $icon = $this->getThumbnail();
    if (empty($icon))  $icon = (($s=$this->getValue('av_sex',1,1)) 
				? $thumbs[$s] 
				: $thumbs['X']); 
  }
   
  /*
   *
   */ 
  public function contactsBlock(){

    $contactInfo=array('av_photo'       =>array('uploadPhoto'),
                       'av_email'       =>array('textField'),
                       'av_phone'       =>array('textField'),
                       'av_institute'   =>array('textArea'),
                       'av_position'    =>array('textField'),
                       'av_http'        =>array('textField'),
                       );
    $passed_away = $this->passed_away($contactInfo);

    if (!$this->askInstitute()){    unset($contactInfo['av_institute']);  unset($contactInfo['av_position']); }
    if (self::$inputToPDF)          unset($contactInfo['av_http']); 
    if (!$passed_away || EA_manager_here) $this->formBlock('av_contact',
							   ($passed_away ? 'Assistant' : 'Contact information'),
							   $contactInfo,
							   array('i'=>($icon='i-mobile-phone'),
								 'l'=>$this->sameURL_but(array('mode' =>'RW',
											       'av_id'=>$this->ID))));
  }


  /*
   * Is Master study completed?
   * The non-considered study is also completed.
   */
  private function MS_completed(){
    if($this->askMS()){
      //      $reply = $this->year2date_isset || ($av_ms_year == CONST_year_not_considered);
      $reply = (b_time::is_valid(@$this->getValue('av_ms_year',1,1)) != -1);
    }else{
      $reply = True;
    }
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   * 
   */
  public function get_date($column='av_birthdate',$fmt='medium'){
    $av_date = $this->year2date($column,'unix_time');
    if (is_numeric($av_date)) $reply = b_time::_date($fmt,$av_date);
    else                      $reply = $av_date;
    $this->dbg($reply);
    return $reply;
  }

  /*
   *
   */
  protected function formBlock_isVisible($block_ID,$fields=array()) {
    
    if ($reply = parent::formBlock_isVisible($block_ID,$fields)){
      switch($block_ID){
      case 'edu_ms':
      case 'edu_phd':
	$year = $this->getValue(str_replace('edu','av',$block_ID).'_year',True,1);
	if(($year===CONST_year_not_considered) && 
	   ($this->MODE == 'RO'))        $reply = False;
	elseif($block_ID == 'edu_ms')    $reply = $this->askMS();
	elseif($block_ID == 'edu_phd')   $reply = $this->askPhD();
	else                             $reply = parent::formBlock_isVisible($block_ID,$fields);
	break;

      case 'av_birth_block':
	$reply = (empty($_GET['block_once']) || (@$_GET['block_once'] === 'av_birth_block'));
	break;
	
      case 'address':
	$reply = ($this->askHomeAddress()||
		  $this->askResidentship());
	break;
	
      default:
      }
    }
    return $reply;
  }
  
  /*
   * Avatar specific fields
   */
  function getValue($name,$asIs=False,$description_MBE=True){
    
    switch($name){
    // The following columns are treated here
    case 'av_ssn':
    case 'av_http':
    case 'av_ms_year':
    case 'av_phd_year':
    case 'av_birthdate':
      break;

    default:
      return parent::getValue($name,$asIs,$description_MBE);
    }

    // Check the cache
    if (($v = $this->getValue_fromCache($name,$asIs,$description_MBE)) !== Null) return $this->getValue_return('cache',$v);

    // well...
    if (@$this->getValue_recursion2[$name]++){
      myPear::WARNING_T("Recursion ".$this->getValue_recursion2[$name]);
      b_debug::traceBack();
      return Null;
    }
    
    // Cache is empty
    switch($name){
      
    case 'av_http':
      $reply = parent::getValue($name,True,True);
      if (!empty($reply) && (stripos($reply,'http') !== 0)){
	$reply = 'http://' . $reply;
	$this->getValue_reset($name,$reply);
      }
      break;

   case 'av_ssn':
      $ssn = parent::getValue($name,True,True);
      $reply = SSN::fix($ssn);
      if ($reply !== $ssn) $this->getValue_reset($name,$reply);
      break;

    case 'av_birthdate':
      // SSN has precedence over the birthdate
      $av_birthdate= parent::getValue($name,   True,$description_MBE);
      $ssn         = parent::getValue('av_ssn',True,$description_MBE);

      if (($reply = SSN::ssn2birthDate($ssn)) === False) $reply = $av_birthdate;

      if ($reply != $av_birthdate){
	$this->getValue_reset_forced($name,$reply);
	if (!empty($av_birthdate)){
	  $was = date('Y-m-d',$av_birthdate);
	  $now = date('Y-m-d',$reply);
	  $this->dbg("$was ==> $now",(bool)($this->show_gv||$this->debug||cnf_dev));
	}
      }
      break;

    case 'av_ms_year':
    case 'av_phd_year':
      $reply = $this->year2date($name,'unix_time', parent::getValue($name,True,$description_MBE));
      if (!empty($reply) && $reply!==CONST_year_not_considered) switch($this->MODE){
	case 'RW':	 
	  $reply = b_time::short_date($reply);
	  break;
	case 'RO':
	  $y = date('Y',b_time::txt2unix($reply));
	  if ($y > date('Y')) $reply = "$y (expected)";
	}
      if (empty($reply) && !$asIs)        $reply = parent::getValue($name,'text');
      break;

    default:
      b_debug::internalError("forgot '$name'");
    }
    
    // We got the value. Now check how good is it
    $this->getValue_recursion2[$name] = 0;
    return $this->getValue_return('',$this->getValue_toCache($name,$reply));
  }

  /*
   * Executor for uploading the photos.
   * Build and execute the command to re-scale the image to the standard size(es)
   */
  protected function saveFile_destination($name){
    switch($name){
    case 'av_photo':
      if (strlen($this->getEmail()) < 6){
	// n@d.cc - the e-mail  can't be shorter then 6 symbols
	$this->dbg("wrong email");
	$reply = False;
      }else{ 
	b_os::exec(sprintf(myPear_bin."pnmScale.sh '%s' '%s' '%s' '%d'",
			   $_FILES[$name]['tmp_name'],
			   self::photo_repo(strip_tags($this->getEmail())),
			   'mimeType',
			   b_cnf::get('portraitPhoto_size',portraitPhoto_size_default)));
	$reply = self::$photo_repo_file;
      }
      break;
      
    default:
      $reply = parent::saveFile_destination($name);
    }
    $this->dbg($reply);
    return $reply; 
 }
  
  
  /*
   * Dialog to upload/see the photo.
   * The photo itself is printed in the most right visible column.
   */
  function uploadPhoto(){
    static $name='av_photo';
    
    // Put the existing photo as the "pending object" - upper right corner
    $img = $this->getPhoto('',$this->getValue($name,True));
    if (!empty($img))  array_unshift($this->t->pendingObject, $img);
    
    // Say "upload a file" in the RW mode, just exit for the RO mode
    if ($this->MODE == 'RW' && !@$this->isRO[$name]){
      $this->defineVariable(empty($img) ? 'Photo' : 'Replace the photo',  $name);
      $this->getFormRow($name, 
			"<input type='hidden' name='MAX_FILE_SIZE' value='3000000'/>".
			"<input type='file' name='$name' />");
    }
  }
  
  /*
   *
   */
  protected function studyBlock(){
    
    if ($this->MODE == 'RO') $this->defineVariable(array('av_ms_institute' => ' ',
							 'av_ms_country'   => ' ',
							 'av_phd_institute'=> ' ',
							 'av_phd_country'  => ' '),True);
    $oxford = array('d'=>'modify studies status',
		    'i'=>'i-oxford',
		    'm'=>'forced', 
		    'l'=>$this->sameURL_but('RW'));
    
    $askPhD = $this->askPhD() && (($this->getValue('av_phd_year',True) != CONST_year_not_considered) || $this->MODE == 'RW');
    if ($askPhD){
      $dialog = $this->year2date('av_phd_year','dialog2');
      if (((int)$this->getValue('av_phd_year') > 1900) || $this->getValue('av_phd_institute',1) || $this->getValue('av_phd_country',1)){
	$dialog['av_phd_institute']= array('textField');
	$dialog['av_phd_country']  = array('textFieldCountry');
      }else{
	$this->submitOnChange['av_phd_year'] = True;
      }
      foreach($dialog as $k=>$v) $this->isRO[$k] = False;
      $this->formBlock('edu_phd','Doctoral studies',$dialog,$oxford);
    }
    
    $askMS = $this->askMS() && (($this->getValue('av_ms_year',True) != CONST_year_not_considered) || $this->MODE == 'RW');
    if ($askMS){
      $dialog = $this->year2date('av_ms_year','dialog2');
      if (((int)$this->getValue('av_ms_year') > 1900) || $this->getValue('av_ms_institute',1) || $this->getValue('av_ms_country',1)){
	$dialog['av_ms_institute'] = array('textField');
	$dialog['av_ms_country']   = array('textFieldCountry');
      }else{
	$this->submitOnChange['av_ms_year'] = True;
      }
      foreach($dialog as $k=>$v) $this->isRO[$k] = False;
      $this->formBlock('edu_ms','Master studies',$dialog,$oxford);
    }
  }
  

  public static $w=1;
  public static $h=1;
  /*
   * Find the photo file.
   * If no filename given then search for the e-mail match
   *
   * @param (array)$buffer - formDB array
   * @param (bool)$returnURL - flag to return the file URL rather the html <img > tag
   * @param (string)$photoFile
   * @param $scale - scale factor
   * @param
   */ 
  private static $photo_repo_file = '';
  public  static function photo_repo($file,$exact_match=True){
    // Check the file name for valid types
    if (preg_match('/('.join('|',self::$validMimeTypes).')$/i',$file)) self::$photo_repo_file = $file;
    else                                                               self::$photo_repo_file = $file . '.jpeg';

    // The photos are in a simple directory tree
    $repo_path = myPear_photos.substr(self::$photo_repo_file,0,1).DIRECTORY_SEPARATOR;
     return $repo_path . self::$photo_repo_file;
  }

  /*
   *
   */
  public static function _getPhoto($buffer,$returnURL='',$photoFile='x',$scale=1){
    
    // Search for all possible photo sources. The first match is accepted
    $input = array();
    if($f=@$buffer['av_photo']) $input[] = self::photo_repo($f);
    if($photoFile)              $input[] = self::photo_repo($photoFile);
    if($e=@$buffer['av_email']) $input[] = self::photo_repo($e);

    $reply = '';
    foreach($input as $url){
      if(is_file($path=b_os::url2path($url))){
	if ($returnURL){
	  $reply = $url;
	}else{
	  $info = getimagesize($path);
	  self::$w = (int)($info[0]/$scale);
	  self::$h = (int)($info[1]/$scale);
	  $reply = sprintf('<img width="%d" height="%d" src="%s" alt=""/>',self::$w,self::$h,$url);
	}
	break;
      }      
    }
    return $reply;
  }

  /*
   *
   */
  public static function _getThumbnail($buffer){
    $reply = self::_getPhoto($buffer,'','',8);
    return $reply;
  }

  /*
   *
   */
  public function getThumbnail(){
    return self::_getThumbnail(array('av_photo' => $this->getValue('av_photo',True,True),
				     'av_email' => $this->getValue('av_email',True,True)));
  }
  
  /*
   * Get the image file with the photo portrait
   */
  function getPhoto($returnURL='',$photoFile='',$scale=1){
    if (is_array($returnURL)){       // Called from getValue, avoid infinite loop 
      $hints = $returnURL;
      $returnURL = '';
    }else{
      $hints['av_photo'] = $this->getValue('av_photo',True,True);
    }
    $hints['av_email'] = $this->getValue('av_email',True,True);
    $url = self::_getPhoto($hints,
			   $returnURL,
			   $photoFile,
			   $scale);
    if(is_file($url)) $this->getValue_reset_forced('av_photo',basename($url));
    // $this->dbg(b_fmt::escape($url),cnf_dev);
    return $url;
  }

  /*
   * Get the user password
   */
  function getPWD(){
    if (!($pwd=$this->getValue('av_pwd',True,True))) $pwd = date("yd");
    return $pwd;
  }

  /*
   * Set the user password
   */
  function setPWD($pwd='',$updateDB=True){
    if ((int)$this->ID){
      $this->defineVariable(array('av_pwd'=>'av_pwd'),
			    'only_if_not_defined');
      if (empty($pwd)) $pwd = $pwdCurrent = $this->getValue('av_pwd',True,True);
      if (empty($pwd)) $pwd = $this->getPWD();
      $this->dbg("pwd '$pwd' is set");
      if ($updateDB)  $this->getValue_reset_forced('av_pwd',$pwd);
    }
    return $pwd;
  }
  
  function formOKspecific() {
    $this->dbg('');
    $this->getValue_reset('av_pwd',$this->getPWD());
    if ($this->myPost_bForm() && $this->errorless())      $this->setEmail();
  }

  /*
   * Convert a comma-separated list of emails into array.
   * On the reply the first array element contains the first VALID e-mail from the input string,
   * the second one - comma-separated remain list
   */
  static function tidy_email($email,$ignore_email=''){
    $tidy_emails = array();
    if (is_array($email)) $email = join(',',array_values($email));
    $emails = b_fmt::RFC_2822($email);
    if (!empty($ignore_email)) $emails = preg_grep("/$ignore_email/i",$emails,PREG_GREP_INVERT);
    if (!empty($emails)){
      $tidy_emails['av_email']  = $emails[0];
      sort($emails);
      $tidy_emails['av_email2'] = join(',',$emails);
    }
    return $tidy_emails;
  }
}
