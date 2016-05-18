<?php
/*
 * Show the "custom dialog" block. 
 * This one is about the (multiple) computer accounts for the employees.
 * Obviously myPear does not keep those, since it is very much customer-oriented,
 * we leave this for the "plug-ins" like this one.
 */
define('maildb_RO',!bAuth::authenticated() || cnf_demo || cnf_dev);

class legacy_maildb{
  
  
  private static $maildb_fields_org = array('an'     => array('m' => array(),
							      'c' => 'ALB'),
					    'fysikum'=> array('m' => array(),
							      'c' => 'FKM'),
					    'kth'    => array('m' => array(),
							      'c' => 'KTH'),
					    'nordita'=> array('m' => array('norPeriodOfStay'),
							      'c' => 'NOR'),
					    'okc'    => array('m' => array(),
							      'c' => 'FKM'),
					    );
  

  public $possible_accounts = array('acc_su'  => array('d'=>'SU account',
						       //'h'=>'/afs/su.se/home', // SU abandones AFS
						       'h'=>'/home',
						       ),
				    'acc_kth' => array('d'=>'KTH account',
						       'h'=>'/afs/kth.se/home')); 
  // 
  public $possible_hooks    = array('m_myorg' => array('d'=>'maildb name'));
								      

  public $pwd_file = array();  
  public $domains  = array(
			   'albanova.se',
			   'fysik.su.se',
			   'nordita.org',
			   );
  private $res = array();
  public  $parent_AV = Null;

  public  $debug  = False;
  public  $maildb = array();

  /*
   * The constructor is practically empty
   */
  function __construct(&$parent_AV){

    // Include accounts into possible hhoks
    foreach(array_keys($this->possible_accounts) as $acc){
      $this->possible_hooks[$acc] = $this->possible_accounts[$acc];
    } 

    if (empty(self::$maildb_fields_org[myOrg_code])) 
      self::$maildb_fields_org[myOrg_code] = array('m'=>array(),
						   'c'=>'X');
    $this->maildb_fields = array_unique(array_merge(array('name','namefam','von','edt','pers_number',
							  'institution',
							  'm_myorg',
							  'acc_kth',
							  'acc_su',
							  'name',
							  'country',
							  'address',
							  'postnummer',
							  'phone',
							  'position',
							  'office',
							  'employeeType',
							  'employeeTitle',
							  'groupid',
							  'expiration',
							  'status'),
						    self::$maildb_fields_org[myOrg_code]['m'],
						    array_keys($this->possible_hooks)));
    sort($this->maildb_fields);
    if (!empty($parent_AV)) $this->parent_AV = $parent_AV;
  }
  
  /*
   * Keep the maildb database in cache during the session to speed-up the access.
   * This obviously has a draw-back - if maildb is changed by an "external procedure", we will not see it.
   */
  private function maildb($av_id,$key=Null,$new_value=Null,$update_maildb=True){
    bTiming()->cpu(__METHOD__);
    // Fill the cache if not yet done
    if (empty($_SESSION[__class__])){
      bTiming()->cpu(__METHOD__.'_cache');
      $_SESSION[__class__] = array();
      $this->dbg("rebuilding cache");

      // "Assemble" the Unix command to build the maildb query
      list($cmd_list,$cmd_human_readable) = self::maildb_cmd('-list',$this->maildb_fields);
      $this->dbg($cmd_list);
      
      // Do the query
      foreach(explode("\n",`$cmd_list`) as $line){
	$r = explode('-LF-',$line);
	$maildb_user = array_shift($r);
	$_SESSION[__class__][$maildb_user]['user'] = $maildb_user;
	foreach($this->maildb_fields as $f){
	  $v = array_shift($r); $v = trim($v);
	  if (!empty($v)) $_SESSION[__class__][$maildb_user][$f] = $v;
	}


	// remove bogus maildb entries by checking the vital hook fields
	$toDrop = True;
	foreach(array_keys($this->possible_hooks) as $hook){
	  if (!empty($_SESSION[__class__][$maildb_user][$hook])) $toDrop = False;
	}
	if ($toDrop){
	  @$dropped++;
	  unset( $_SESSION[__class__][$maildb_user]);
	}
      }
      
      if (!empty($dropped)) $this->dbg("$dropped bogus maildb entries are not considered");
      $this->dbg(count($_SESSION[__class__]).' active entries');
      bTiming()->cpu();
    }

    if (empty($_SESSION[__class__])){
      myPear::ERROR("Can't read the maildb, no maildb updates possible");
      return Null;
    }
    
    // Enquiry, is the maildb_user valid?
    if ($av_id === '?'){
      $maildb_user = $new_value; // new_value in this context is just a maildb user name 
      $reply = isset($_SESSION[__class__][$maildb_user][$key]);
      $this->dbg($reply);
      return $reply;
    }

    // Hook arrives...
    if (($key === 'm_myorg') && !empty($new_value)){
      $maildb_user  = $new_value;
      $old_value = '&lt;none&gt;';
      $_SESSION[__class__][$maildb_user]['m_myorg'] = ($new_value = $av_id);
      $_SESSION[__class__][$maildb_user]['user']    = $maildb_user;
      if ($this->debug) b_debug::print_r($_SESSION[__class__][$maildb_user],"hook arrives '$maildb_user'");
    }

    // build  av_id (combodb) -> user (maildb) links
    $hooks = array();
    foreach(array_keys($this->possible_hooks) as $hook){
      foreach($_SESSION[__class__] as $maildb_user=>$data){
	if(!empty($data[$hook])) $hooks[$hook][$data[$hook]] = $maildb_user;
      }
    }
    
    // combodb <=> maildb
    if ($av_id !== '?'){
      foreach(array_keys($this->possible_hooks) as $hook){
	if (($user = @$hooks[$hook][$av_id]) !== Null){
	  $this->dbg($msg="Hooked user='$user' by $hook=$av_id");
	  if (cnf_dev || $this->debug) myPear::DEBUG(__METHOD__."() - $msg");
	  break;
	}
      }
      if (empty($user)){
	myPear::WARNING("No hook for av_id=$av_id, can't get maildb data");
	return Null;
      }
    }

    // Return the answer
    if($new_value===Null){
      // Return the query answer
      if (empty($key)) $reply = @$_SESSION[__class__][$user];
      else             $reply = @$_SESSION[__class__][$user][$key];
    }else{
      // update maildb
      if (!isset($old_value)) $old_value = @$_SESSION[__class__][$user][$key];
      if ($new_value === '_RESET_' || myPear::is_empty($new_value)){
	$new_value = '';
	$_SESSION[__class__][$user][$key] = $new_value;
	$this->parent_AV->getValue_reset_forced($key,$new_value);
      }
      
      if ($new_value === $old_value){
	$reply = $old_value;
      }elseif (strToLower($new_value) != strToLower($old_value)){
	$msg = "maildb($user): " . (empty($new_value) ? x('em',"Unset $key") : x('em',"$key: $old_value --> $new_value"));
	myPear::MESSAGE($msg);
	$this->dbg($msg);
      }
      
      $_SESSION[__class__][$user][$key] = $new_value;

      list($cmd_add,$cmd_human_readable) = self::maildb_cmd($user,array(empty($new_value) 
									? "-unset $key" 
									: "-set $key=".str_replace(' ','#',$new_value)));
      if ($update_maildb){
	$this->dbg($cmd_add);
	if (maildb_RO || empty($_SESSION[__class__])) myPear::MESSAGE($cmd_human_readable);
	else	                                      b_os::exec($cmd_add,'>>>');
      }else{
	$this->dbg("Command ignored: $cmd_human_readable",cnf_dev||$this->debug);
      }
      $reply = $new_value;
    }
    bTiming()->cpu();
    $this->dbg($reply);
    return $reply;
  }


  /*
   * Interactive dialog form, customized for the Organization "Nordita",
   * see http://www.nordita.org
   */
  function getForm($block_ID='legacy_maildb'){

    if (!is_object($this->parent_AV)){
      b_debug::var_dump($this->parent_AV,'$this->parent_AV is not an object');
      return;
    }

    // Prepare the dialog    
    foreach ($this->possible_hooks as $f=>$data){
      $vf = "_virt_${f}";
      
      // See if we got back an answer from the previous "click"
      if (($new_value = @$_POST[$vf]) !== Null){
	if (in_array($f,$this->verify_identity($new_value))){
	  $this->maildb($this->parent_AV->ID,$f,$new_value);
	}else{
	  $this->maildb($this->parent_AV->ID,$f,'_RESET_');
	  myPear::ERROR("\"$new_value\" is not a valid ".$data['d']);
	}
      }

      // Build the dialog      
      $dialog[$vf] = array('textField');
      $this->parent_AV->formDB[$vf] = $this->maildb($this->parent_AV->ID,$f);
      if ($f == 'm_myorg'){
	if (!empty($this->parent_AV->formDB[$vf])) $this->parent_AV->formDB[$vf] = $this->maildb($this->parent_AV->ID,'user');	
      }
      $this->parent_AV->isRO[$vf]   = empty($data['h']) && !empty($this->parent_AV->formDB[$vf]);
      $this->parent_AV->isMBE[$vf]  = empty($data['h']);
      $this->parent_AV->defineVariable($data['d'],$vf);
    }
    
    // Show the block
    $this->parent_AV->formBlock($block_ID,
				'Maildb & Computer accounts',
				$dialog,
				$button=array('l' => $this->parent_AV->sameURL_but(array('mode'=>'RW')),
					      'd' => 'auto'),
				$extraTD='');
    // Update the maildb
    if (!($user=$this->maildb($this->parent_AV->ID,'user'))){
      list($user,$domain) = explode('@',bForm_Avatar::generate_email($this->parent_AV->getValue('av_firstname',True),
								     $this->parent_AV->getValue('av_lastname',True)));
      $this->maildb($this->parent_AV->ID,'m_myorg',$user);
      myPear::MESSAGE("No hook for the maildb record \"".$this->parent_AV->name()."\" , create a new maildb entry as \"$user\"");
    }
    $this->avatar2maildb($user);

    // Update /etc/password file
    $this->update_etc_passwd();

    // Update av_identity field
    $logins = array();
    foreach(array_keys($this->possible_accounts) as $acc){
      if ($login = $this->maildb($this->parent_AV->ID,$acc)) $logins[] = $login;
    }
    $this->parent_AV->setIdentity($logins,$verbose=True);
  }

  private function update_etc_passwd(){
    $acc_kth = $this->maildb($this->parent_AV->ID,'acc_kth');
    if (!empty($acc_kth) && !posix_getpwnam($acc_kth)){
      b_os::exec(sprintf(dirname(__file__)."/../bin/update_etc_passwd.sh '%s' '%s' '%s' ",
			 $acc_kth,
			 $this->parent_AV->fmtName('f'),
			 $this->parent_AV->fmtName('l')));
    }
  }
    
  /*
   * Check that the given account has a home directory.
   * Since the same account might exist on various file-systems, all are returned
   */
  function verify_identity($new_value){
    $l1 = substr($new_value,0,1);
    $l2 = substr($new_value,1,1);
    $reply = array();
    foreach ($this->possible_hooks as $f=>$data){
      if (empty($data['h'])) {
	if ($this->maildb('?','user',$new_value)) $reply[] = $f;
      }elseif (!is_dir($data['h'])) {
	// Return back the path if there is no access to network
	$reply[] = $f;
      }elseif (is_dir($dir=$data['h']."/$l1/$l2/$new_value") ||
	       is_dir($dir=$data['h']."/$new_value")){
	$this->dbg("OK $f - $dir");
	$reply[] = $f;
      }
    }
    return $reply;
  }

		 
  /*
   * Update maildb with the data from EA Avatar class
   */
  private function avatar2maildb($user){
    
    // Employment period.
    $emp = $this->parent_AV->hook_employment();
    
    $day1 = myOrg()->eaMembers()->get_memberInfo($this->parent_AV->ID,'day1');
    $day9 = myOrg()->eaMembers()->get_memberInfo($this->parent_AV->ID,'day9');
    $day9 = min($day9, CONST_eternity_t);
    
    // edt
    if (!$this->maildb($this->parent_AV->ID,'edt')){
      if (!empty($day1)) $this->maildb($this->parent_AV->ID,'edt',date('Y-m-d',$day1));
    }
      
    
    // Check the personal number both in maildb and myPear, reject the wrong one
    $pers_number = $this->maildb($this->parent_AV->ID,'pers_number');
    $ssn      = $this->parent_AV->getValue('av_ssn',True);
    if (!SSN::valid($ssn))       $ssn = '';
    if (!SSN::valid($pers_number))  $pers_number = '';
    
    if (empty($pers_number) && !empty($ssn)){
      $this->maildb($this->parent_AV->ID,'pers_number',($pers_number=$ssn));
    }    
    if (empty($ssn) && !empty($pers_number)){
      $this->parent_AV->updateDB(array('av_ssn'=>($ssn=$pers_number)));
    }    
    
    // Match the myPear -- maildb fields
    $maildb_array = array();
    foreach ($this->maildb_fields as $field){
      switch($field){
      case 'pers_number':     $maildb_array[$field] = $ssn;  break;
      case 'm_myorg':         $maildb_array[$field] = $this->parent_AV->ID; break;
      case 'institution':     $maildb_array[$field] = self::$maildb_fields_org[myOrg_code]['c'];     break;
      case 'acc_kth':         $maildb_array[$field] = $this->maildb($this->parent_AV->ID,'acc_kth'); break;
      case 'acc_su':          $maildb_array[$field] = $this->maildb($this->parent_AV->ID,'acc_su');  break;
      case 'edt':             $maildb_array[$field] = $this->maildb($this->parent_AV->ID,'edt');     break;
      case 'address':         $maildb_array[$field] = $this->parent_AV->getValue('av_address',True); break;
      case 'postnummer':      $maildb_array[$field] = $this->parent_AV->getValue('av_zip',True);     break;
      case 'phone':           $maildb_array[$field] = $this->parent_AV->getValue('av_phone',True);   break;
      case 'von':             $maildb_array[$field] = $this->parent_AV->getValue('av_von',True);     break;
      case 'name':            $maildb_array[$field] = $this->parent_AV->getValue('av_firstname',True);    break;
      case 'namefam':         $maildb_array[$field] = $this->parent_AV->getValue('av_lastname',True);     break;
      case 'country':         $maildb_array[$field] = $this->parent_AV->getValue('av_residentship',True); break;
      case 'position':        $maildb_array[$field] = $emp->get_currentItem('lm_value');       break;
      case 'office':          $maildb_array[$field] = $emp->get_currentItem('e_off');          break;
      case 'employeeType':    $maildb_array[$field] = $emp->get_currentItem('e_rate');         break;
      case 'employeeTitle':   $maildb_array[$field] = $emp->get_currentItem('lm_value');       break;
      case 'groupid':         $maildb_array[$field] = $emp->get_currentItem('e_gid');          break;
      case 'norPeriodOfStay': $maildb_array[$field] = b_time::period($day1,$day9,'short');     break;
      case 'expiration':      $maildb_array[$field] = ($day9 ? date('Y-m-d',$day9) : '');      break;
      case 'status':          $maildb_array[$field] = ($day9 < time() ? 'Passive' : 'Active'); break;
      case 'created_by':
      case 'norGraduationCity':
      case 'norHomeInstitute':
      case 'norLocalContactPerson':
      case 'changesdate':
      case 'ortnamn':
      case 'last_login':
      case 'phone_home':
      case 'phone_mobile':
      case 'rating':
      default:
	if (cnf_dev) myPear::DEBUG("?? unexpected field '$field'");
      }
    }

    // Collect the changes
    $updates = $unsets = array();
    foreach($maildb_array as $field=>$av_value){
      if ($av_value != $this->maildb($this->parent_AV->ID,$field)){
	if (empty($av_value)){
	  if (empty($unsets)) $unsets[] = ' -unset';
	  $unsets[] = $field;
	}else{
	  $updates[$field] = str_replace(' ','#',$av_value);
	  $this->maildb($this->parent_AV->ID,$field,$av_value,False);
	}
      }
    }
    
    // Update myPear --> maildb
    if (!empty($updates) || !empty($unsets)){
      if (cnf_dev) b_debug::print_r($updates,'updates');
      list($cmd,$cmd_human_readable) = self::maildb_cmd($user,array('-add',
								    b_fmt::joinX(' ',$updates),
								    join(' ',$unsets)));
      $this->dbg($cmd_human_readable);
      if (maildb_RO || empty($_SESSION[__class__]))  myPear::MESSAGE($cmd_human_readable);
      else                                           b_os::exec($cmd,'>>>');
    }
  }
  
  /*
   * define('maildb_hlo',' -HLO iam=xxx_man | grep -vE uninitialized.value | sed "s/xxx_man/daemon /g"');
   * define('maildb_exe','maildb ');
   */
  public static function maildb_cmd($user='-list',$what_to_do=array()){
    if (!defined('maildb_hlo')) define('maildb_hlo',' -HLO iam=xxx_man | grep -vE uninitialized.value');
    $cmd = sprintf('maildb %s %s '.($user == '-list' 
				    ? ' -if lfsep '.maildb_hlo.' | grep LF | grep -v User' 
				    : maildb_hlo . ' | sed "s/xxx_man/daemon /g"'),
		   $user,
		   join(' ',$what_to_do));
    $regexp = '/.HLO.*/i';
    $cmd_human_readable = trim(str_replace('#',' ',preg_replace($regexp,'<br/>&nbsp;',$cmd)));
    if (cnf_dev|maildb_RO) myPear::DEBUG(__FUNCTION__."() ".str_replace('#',' ',$cmd));
    return array($cmd,$cmd_human_readable);
  }
  
  /*
   * debug
   */
  private function dbg($txt,$forced=False){
    if (!empty($txt) || $forced) b_debug::xxx($txt,array((bool)($forced|$this->debug),3,'orangeText',$this));
  }
  
}
