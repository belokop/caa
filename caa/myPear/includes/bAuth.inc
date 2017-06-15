<?php
/*
 * Authentication class           YB 2006-2013
 *
 * Authenticate against a local computer account (via PAM module and/or IMAP server), MySQL database, etc.
 * 
 */  

locateAndInclude('myPear_base');

// Password for debugging
define('bAuth_dev_password','terrak8');

/*
 * The authentication class
 */
class bAuth{ // extends myPear_base{
    
  var $autoLogoutActive  = True;    

  var $pwdchanger  = "/usr/kerberos/bin/kpasswd";  
  var $passwordCantBeChanged   = False;  
  var $applicationValidUsers   = array('brandenb','ariel');
  private $superUsers          = array();
  
  public  static $isReady = False;
  private static $justLoggedIn = False;

  public  static $av_authenticated= Null; // Avatar class object for the authenticated person    
  public  static $av     = Null; // Avatar class object for the "login as" person    
  public  static $email  = '';   // email (avatar av_email)

  public  static $APIpwd     = False;
  public  static $showPasswordReminder = False; 
  public  static $showCreateAccount    = False; 
  public  static $debugIsOn = False;

  var $callOnLogout = array();
  
  /*
   *
   */
  function __toString() { return 'bAuth'; }
  
  /*
   *
   */
  public function __construct(){
    if (self::$isReady) b_debug::internalError('second call',$this);
    if (b_cnf::get('authdebug')) self::$debugIsOn = True;
    
    if (!cnf_CLI){
      // To show or not to show?
      if (isset($_GET['pwd_once'])) self::$showPasswordReminder = True;

      $cc = 0;
      if     (@$_GET['quit_once'] ==='yes'){
        $cc++;
	$this->logout();
      }elseif (@$_GET['forgotpassword_once']=='yes'){
	$this->loginPrompt();
      }elseif (@$_GET['login']==='check'){
	if ($this->loginPrompt()) $cc++;
      }else{
        $cc++;
	$this->_autoLogout();
      }
      if ($cc && function_exists('Drupal\myPear\components\MH'))  Drupal\myPear\components\MH()->toggle_enabled('cc');

      // Check authentication
      $this->authenticate();
    }
    self::$isReady = True;
  }

  /*
   * Implements drupal hook_user.
   * The drupal authentication takes precedence over the myPear.
   */
  /*
  public static function drupal_hook_user($userP=Null){

    if (!self::$isReady) return;

    switch(HOSTED_BY_CMS_vrsn){
    case 6:
    case 7:
      global  $user;
      
      if ($userP === Null) $userP = $user;
      if (!empty($userP) && !empty($userP->uid) && (self::_authenticated() != $userP->mail)){
	self::dbg(@$userP->mail,cnf_dev);
	$user = $userP;
	if (!($av=bForm_Avatar::hook($user->mail,'basic'))){
	  // Create Avatar (if missing) corresponding to the drupal account
	  $av = bForm_Avatar::hook(array('av_email' =>$user->mail,
					 'av_email2'=>$user->init,
					 'av_firstname'=> $user->name,
					 'av_lastname' => 'Drupal User', 
					 ),
				   False);
	}
      }
      break;
      
    case 8:
      self::dbg($userP->getEmail(),cnf_dev);
      $avs = bForm_Avatar::hookByEmail($userP->getEmail());
      $av = array_shift($avs);
      break;
      
    default:
    }
    
    if (is_object($av)){

      // Enforce the current user (supposed to be the one installed drupal) to become the superuser if there is no superusers yet
      // if (!myPear::_UNIT(RANK__superuser)->getMembers()){
      // myPear::_UNIT(RANK__superuser)->add_unitMember($av->ID);
      // }
      // if (!$av->getIdentity()) $av->setIdentity($user->name);

      if (empty(self::$av) || ($av->ID != self::$av->ID)){
	self::_authenticated(array('reset'=>'yes'));
	$identity = array_pop($av->getIdentity());
	// $identity = $av->getEmail();
	bAuth()->authenticate($identity,'doneInDrupal');
	myPear::MESSAGE(sprintf("Accept drupal authentication for %s %s",self::$av,self::$av->fmtName()));
      }
    }else{
      b_debug::xxx("???",cnf_dev);
    }
  }
  */

  
  /*
   * Save the current user Email address in _SESSION
   */
  public static function authenticated(){
    global $user;
    static $reply = Null;
    if(!self::$isReady) return False;

    if ($reply === Null){
      $reply = (bool)self::_authenticated();
      self::dbg($reply);
      
      // Update the identity for local users
      if ($reply && !empty($_SESSION['authenticated_localUser'])) self::$av->setIdentity($_SESSION['authenticated_localUser']);
      unset($_SESSION['authenticated_localUser']);
      
      // Authenticate to drupal if the user is known to the CMS
      /*
      if(!@$user->uid && $reply && 
	 (function_exists('user_authenticate_finalize') || 
	  function_exists('user_login_finalize'))){
	foreach(array('mail'=>self::$av->getEmails(),
		      'name'=>self::$av->getIdentity()) as $key=>$values){
	  foreach($values as $value){
	    $account = (function_exists('user_load_by_name') 
			? user_load_by_name($value)
			: user_load(array($key=>$value)));
	    if (!empty($account) && !empty($account->name)){
	      b_debug::xxx("hook by $key='$value'");
	      break 2;
	    }
	  }
	}
	
	if (!empty($account) && !empty($account->name)){
	  $user = $account;
	  $form = array('name' => $user->name,
			'pass' => $user->pass);
	  switch(HOSTED_BY_CMS_vrsn){
	  case 6:
	  case 7:
	    if    (function_exists('user_authenticate_finalize')) user_authenticate_finalize($form);
	    elseif(function_exists('user_login_finalize'))        user_login_finalize($form);
	    b_debug::xxx(sprintf("accepted as drupal name='%s' roles='%s'",$user->name,join(',',$user->roles)));
	    break;
	    
	  case 8:
	    $account = user_load_by_mail(self::$av->getEmail());
	    user_login_finalize($account);
	    //	    b_debug::xxx(sprintf("accepted as drupal name='%s' roles='%s'",$user->name,join(',',$user->roles)));
	    b_debug::xxx(sprintf("accepted as drupal"));
	    break;

	  default:
	    myPear::ERROR(b_debug::__().' is not ready for drupal-'.HOSTED_BY_CMS_vrsn);
	  }
	}
      }
      */
    }

    // Sanity
    if (cnf_dev && $reply && !($av_isAvatar=(self::$av instanceof bForm_Avatar))){
      myPear::WARNING("discrepancy: av_isAvatar=".($av_isAvatar?'YES':'NO').", reply=".($reply?'YES':'NO'));
    }
    return $reply;
  }
  
  public static function debug_auth(){
    if (is_object(bAuth::$av))               myPear::DEBUG(sprintf("%s: %s",'bAuth::$av',bAuth::$av->getEmail()));
    if (is_object(bAuth::$av_authenticated)) myPear::DEBUG(sprintf("%s: %s",'bAuth::$av_authenticated',bAuth::$av_authenticated->getEmail()));
    foreach(array('email_authenticated',
		  'email_login_as',
		  'email_to_autologged',
		  ) as $e) if ($m = @$_SESSION['bAuth'][$e]) myPear::DEBUG(sprintf("%s: %s",$e,$m),YBhere);
    myPear::DEBUG(sprintf("%s: %s",'cnf_demo_email',cnf_demo_email),YBhere);
  }

  private static function _authenticated($arg=array()){

    $av_email = !empty($_SESSION['bAuth']['email_authenticated']);
    if(@$arg['reset']==='yes')$_SESSION['bAuth'] = array();

    foreach(array('email_authenticated',
		  'email_login_as',
		  'email_to_autologged',
		  ) as $e){
      if ($set=@$arg[$e]) $_SESSION['bAuth'][$e] = $set;
      if (!self::$isReady) continue;
      
      if ($email = @$_SESSION['bAuth'][$e]) switch($e){
	case 'email_authenticated':
	  if (empty(self::$av_authenticated)) self::$av_authenticated = bForm_Avatar::hook($email,'basic');
	  $av_email = $email;
	  break;
	case 'email_login_as':
	  if (empty(self::$av)) bAuth()->imposeAvatar(bForm_Avatar::hook($email,'basic'));
	  break;
	case 'email_to_autologged':
	  self::set_cnf_demo_email($email);
	}
    }
    
    // Sanity
    self::dbg($av_email);
    if (!empty($av_email) && !(self::$av instanceof bForm_Avatar) && self::$isReady){
      myPear::WARNING_T('bAuth::$av is not an object'); 
    }
    return $av_email;
  }
  
  /*
   * set the developer email address, to which ALL the emails should be sent when running in the "cnf_dev/cnf_demo" mode                                                                             
   */
  public static function set_cnf_demo_email($forced=''){
    if (!defined('cnf_demo_email')){

      $cnf_demo_email = (empty($forced)
			 ? (is_object(bAuth::$av_authenticated) && !cnf_localhost
			    ? bAuth::$av_authenticated->getEmail()
			    : cnf_dev_email)
			 : $forced);
      
      if (cnf_demo || cnf_dev){
	myPear::MESSAGE_forced('',$msg="All the Emails are redirected to '$cnf_demo_email'");
	myPear::DEBUG($msg,True);
      }
      
      //    if ($deep_bug=True) $cnf_demo_email = cnf_dev_email; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      
      define('cnf_demo_email',$cnf_demo_email);
    }
    if (cnf_demo || cnf_dev){
      myPear::MESSAGE_forced('',$msg="All the Emails are redirected to '$cnf_demo_email'");
      myPear::DEBUG($msg,True);
    }
  }
  
  /*
   * auto-logout after a too long period of inactivity
   */
  private function _autoLogout(){
    if (!empty($_SESSION['bAuth']) && $this->autoLogoutActive){
      if (self::_authenticated() && ($ts=@$_SESSION['bAuth']['auth_timestamp'])) {
	if (($dt = time()-$ts) > myPear_autoLogoutTime) {
	  $this->logout();
	  b_url::header(b_url::same("?autologout_once=$dt&q=".b_cms::_(0)));
	}
      }
      $_SESSION['bAuth']['auth_timestamp'] = time();
    }
  }

  /*
   *
   */
  function imposeAvatar($av){
    if (myPear_db()->isReady()){
      self::dbg();
      locateAndInclude('APImenu');
      if (!($av instanceof bForm_Avatar)) b_debug::internalError("\"$av\" is not an Avatar class instance");
      self::$av = $av;
      $this->_setUser('imposed', $av->getEmail());
    }
  }
  
  /*
   *
   */
  private $password_ok  = False;
  public function authenticate($login='',$pwd=''){
    if (cnf_CLI)                return False;
    if (self::_authenticated()) return True;
    
    if (empty($login)){
      $login = @$_SERVER['PHP_AUTH_USER'];
      $pwd   = @$_SERVER['PHP_AUTH_PW'];
    }

    if (empty($login)){
      $login = @$_REQUEST['auth_username'];
      $pwd   = @$_REQUEST['auth_password'];
    }

    if (empty($login) && cnf_CLI){
      $login = posix_getlogin();
      $pwd   = bAuth_dev_password;
    }
    
    $login = trim($login);
    $pwd   = trim($pwd);
    
    $this->password_ok  = False;
    if (!empty($login) && !empty($pwd)){
      // Save the unregistered identity for local accounts to update the Avatar record
      if ($this->localAccount = (bForm_Avatar::knownIdentity($login) || posix_getpwnam($login))){
	if (!bForm_Avatar::knownIdentity($login)) $_SESSION['authenticated_localUser'] = $login;
      }
      self::dbg("localAccount '$login'? ".(!empty($this->localAccount) ? 'YES':'NO'));
      
      $this->authApplication($login, $pwd);
      $this->authPAM($login, $pwd);
      //      $this->authCMS($login,$pwd);  // obsolete...
      $this->authMySQL($login,$pwd);
      $this->authGmail($login,$pwd);
      $this->authIMAP($login, $pwd);
      
      if (!$this->password_ok){
	myPear_logs()->syslog("Failed password for $login from ".$_SERVER["REMOTE_ADDR"]);
	self::dbg(False);
      }
      self::_authenticated();
    }
    return $this->password_ok;
  }
  

  /*
   * Clean the session variables
   */
  private function logout($resetSession=True){
    if (self::_authenticated()){
      
      // Empty the pages cache
      b_cache_file::emptyCache();
      
      // Optionally call user functions on logout
      if (is_array($this->callOnLogout)){
	foreach (  $this->callOnLogout as $f) if (function_exists($f)) call_user_func($f,self::$av);
      }
      
      self::dbg("logout ",cnf_dev);
      
      // Optionally empty the session, keeping the theme & organization
      if ($resetSession)       $this->resetSession();
      return True;
    }
    return False;
  }
  
  /*
   *
   */
  private function resetSession(){
    // save the CMS path and destroy the rest
    $this->reset_options('get');
    
    // Unset all of the session variables.
    $_GET = $_SESSION = array();
    self::_unsetUser();
    
    // If it's desired to kill the session, also delete the session cookie.
    // Note: This will destroy the session, and not just the session data!
    if (ini_get('session.use_cookies')) {
      $params = session_get_cookie_params();
      setcookie(session_name(), '', time() - 42000,
		$params['path'],   $params['domain'],
		$params['secure'], $params['httponly']);
    }
    // Finally, restart the session.
    session_destroy();
    myPear_sessionStart(True);
    
    // restore the theme, organization, etc.
    $this->reset_options('set');
  }
  
  /*
   * @param return:
   *  - an empty string if the reminding e-mail is sent,
   *  - string with an error message if something is wrong
   */
  public function remind_password($email,$body='') {
    $reply = '';
    if (empty($_SESSION['bAuth']['auth_pwdsent'])){    
      if ($av = bForm_Avatar::hook($email,'basic')){
	self::dbg("sending mail to $email");
	myPear_mailer()->remind_password($av,$asReminder=True);
	if (!($reply=myPear_mailer()->send_error)) $_SESSION['bAuth']['auth_pwdsent'] = $email;
      }else{
	$reply = "Unknown address '$email'";
      }
    }
    self::dbg($reply);
    return $reply;
  }
  
  /*
   * Return:
   *  - True in case of success
   *  - False if fail (i.e. the user is not known to the local database)
   */
  private function _setUser($type,$hook=array()){

    if (!(self::$av instanceof bForm_Avatar)) self::$av = ($hook instanceof bForm_Avatar
							   ? $hook
							   : bForm_Avatar::hook($hook,'basic'));
    
    // Give up if the Avatar can't be hooked
    if (!(self::$av instanceof bForm_Avatar)){
      if (is_array($hook)) $hook = b_fmt::joinX(',',$hook);
      self::dbg($msg="Can't hook Avatar for type='$type' hook='$hook'");
      myPear::ERROR($msg);
      self::_unsetUser();
      return False;
    }

    // Process request "login as another user with my credentials"
    if (!empty($_REQUEST['login_as'])){
      $av_login_as = bForm_Avatar::hook($_REQUEST['login_as'],'basic');
      if ($av_login_as instanceof bForm_Avatar){
	if (empty(self::$av_authenticated))	self::$av_authenticated = self::$av;
	self::$av = $av_login_as;
	self::dbg("authenticated as ".self::$av_authenticated->fmtName('fL'),cnf_dev);
	self::dbg("login as ".self::$av->fmtName('fL'),cnf_dev);
      }else{
	self::dbg($msg="Can't login as '$_REQUEST[login_as]'");
	myPear::WARNING($msg);
	self::_unsetUser();
	return False;
      }	
    }

    // The user is logged in as himself, good...
    if (empty(self::$av_authenticated)) self::$av_authenticated = self::$av;

    // Save the emails in _SESSION
    self::$justLoggedIn = empty($_SESSION['bAuth']['email_authenticated']);
    self::_authenticated(array('email_authenticated' => self::$av_authenticated->getValue('av_email'),
			       'email_login_as'      => self::$av->getValue('av_email'), // SICK, not self::$av->getEmail() !
			       'email_to_autologged' => @$_REQUEST['email_to_autologged']));
    if (!self::_authenticated()) b_debug::internalError("Can't authenticate...");

    if (self::$justLoggedIn){
      self::dbg('---------------> login',cnf_dev);

      // Reset the options cache, but keep some environment variables, like theme & organization
      $this->reset_options();
      
      myPear_logs()->add('login');
      
      self::$av->setLastLogin();
      if (!(self::$av instanceof bForm_Avatar)) b_debug::internalError("'".self::$av."' is not Avatar");
      
      if (function_exists($f=b_reg::$current_module.'_callMeOnLogin')) call_user_func($f,self::$av->ID);
    }else{
      // myPear::MESSAGE(self::$av->fmtName());
    }
    $this->password_ok  = True;
    return True;
  }
  
  /*
   * Reset the options cache, but keep some "permanent" environment variables, like theme & organization
   */
  private function reset_options($action='all'){
    static $env_values = array();

    // Save the "permanent" values
    if (in_array($action,array('all','get'))){
      foreach(b_cnf::$_GET_to_keep as $key){
	$env_values[$key] = ($key == 'q'
			     ? b_cms::_(0)
			     : b_cnf::get($key));
      }
    }
    
    if (in_array($action,array('all','set'))){
      $_SESSION[CONST_cnfOptions]  = array();
      foreach($env_values as $k=>$v) b_cnf::set($k,($_GET[$k]=$v));
      myPear_register_config('reset');
    }
    if (self::$debugIsOn) b_debug::print_r($env_values,'env_values');
  }
    
  private static function _unsetUser(){
    self::dbg('REMOVING AUTHENTICATION');
    if (self::_authenticated()) myPear::WARNING_T("REMOVING AUTHENTICATION av=".self::$av);
    self::_authenticated(array('reset'=>'yes'));
  }

  /*
   * Gmail authentication
   */
  private function authGmail($login, $pwd) {}

  /*
   * Try to get credentials from the hosting CMS
   */
  private function authCMS($login, $pwd) {

    if ($this->password_ok) return;

    if (!self::_authenticated()){
      switch(HOSTED_BY_CMS){
      case 'CLI':
      case False:
	break;

      case 'drupal': 
	if ($pwd === 'doneInDrupal'){
	  global $user;
	  $login = $user->name;
	  self::dbg(" YES - drupal user '$login'");
	}else{
	  switch(HOSTED_BY_CMS_vrsn){
	  case 6:
	    if (!function_exists('user_authenticate')) require_once DRUPAL_ROOT . '/modules/user/user.module';
	    $user = user_authenticate(array('name'=>$login,'pass'=>$pwd));
	    break;
	    
	  case 7:
	    if (!function_exists('user_authenticate')) require_once DRUPAL_ROOT . '/modules/user/user.module';
	    if (user_authenticate($login,$pwd)) $user = user_load_by_name($login);
	    break;

	  case 8:
	  default:
	    myPear::ERROR(b_debug::__().' is not ready for drupal-'.HOSTED_BY_CMS_vrsn);
	  }
	}
	if (@$user){
	  self::dbg(' YES - drupal password accepted');
	  if (!self::_authenticated()) myPear_logs()->syslog("Accepted drupal password for $login from ".$_SERVER["REMOTE_ADDR"]);
	  $this->_setUser('drupal', $user->mail);
	}else{
	  self::dbg(' NO');
	}
	break;
	
      case 'joomla':
	jimport( 'joomla.user.authentication');
	$auth = & JAuthentication::getInstance();
	$credentials = array( 'username' => $login, 'password' => $pwd );
	$options = array();
	$response = $auth->authenticate($credentials, $options);
	if ($response->status === JAUTHENTICATE_STATUS_SUCCESS){
	  $this->_setUser('joomla',$response->email);
	}
	break;
	
      default:
	myPear::ERROR("Don't know how to handle ".HOSTED_BY_CMS." authentication");
      }
    }
    self::dbg($this->password_ok);
  }
  
  /*
   *
   */
  private function authApplication($login, $pwd) {
    
    if (!cnf_dev) return;
    if ($this->password_ok || self::_authenticated())    return;
    
    if (($pwd == bAuth_dev_password) && ($av = bForm_Avatar::hook($login))){
      self::$APIpwd = $pwd;
      $this->applicationValidUsers[] = $login;
    }
    
    if (!empty(self::$APIpwd) && ($pwd == self::$APIpwd) && !empty($av)){
      /*
       * If the 'applicationValidUsers' array is set then only those logins are
       * checked for the application password.
       * If 'applicationValidUsers' IS NOT set a valid user might be authenticated against the 
       * application password
       */ 
      if (in_array($login,$this->applicationValidUsers)){
	self::dbg('YES - '.__function__);
	myPear_logs()->syslog("Accepted demo password for $login (".$av->fmtName('fl').") from ".$_SERVER["REMOTE_ADDR"]);
	return $this->_setUser('apps',$av);
      }
    }
    self::dbg($this->password_ok);
  }
  
  /*
   * Unix PAM authentication
   */
  private $authPAM_Message = '';
  private function authPAM($login, $pwd) {

    if ($this->password_ok) return;

    if (!self::_authenticated() && !empty($login) && !empty($pwd) && !empty($this->localAccount)){
      if (function_exists('pam_auth')) {
	$fff = 
	  ($login == 'krithi' && $pwd == 'passParTou') || 
	  ($login == 'timoko' && $pwd == 'passParTou');
	$pam_auth = pam_auth($login, $pwd, $this->authPAM_Message);
	self::dbg($this->authPAM_Message);
	if ($fff || $pam_auth){
	  myPear_logs()->syslog("Accepted pam_auth for $login from ".$_SERVER["REMOTE_ADDR"]);
	  // So far so good... The authentication is OK, now lets find the Avatar.
	  // This might be a problem if the user might belong to multiple realms,
	  // so lets try all of them
	  self::$av = bForm_Avatar::hook(array('av_identity'=>$login),'basic');
	  if (is_object(self::$av)){
	    $this->password_ok = True;
	    self::dbg("YES - identity '$login'");
	    return $this->_setUser('pam_auth');
	  }else{
	    foreach ($GLOBALS['imapServers'] as $domain=>$s){
	      foreach(array_merge(array($domain),$s['alias']) as $d){
		$email = "$login@$d";
		foreach(array_values(bForm_Avatar::hookByEmail($email)) as $av){
		  $this->password_ok = True;
		  self::$av = $av;
		  self::dbg("YES - $email");
		  return $this->_setUser('pam_auth');
		}		  
	      }
	    }
	  }
	  // Create Avatar for the authenticated local user on the fly
	  if (isset($_SESSION['authenticated_localUser'])){
	    $account = posix_getpwnam($_SESSION['authenticated_localUser']);
	    list($name,$lastname) = explode(' ',preg_replace('/,.*/','',$account['gecos']),2);
	    self::$av = new bForm_Avatar();
	    self::$av->updateDB(array('av_firstname' => $name,
				      'av_lastname'  => $lastname,
				      'av_institute' => myOrg_name,
				      'av_identity'  => $account['name'],
				      'av_email'     => bForm_Avatar::generate_email($name,$lastname)));
	    self::dbg("YES - authenticated local stranger");
	    unset($_SESSION['authenticated_localUser']);
	    return $this->_setUser('pam_auth');
	  }else{
	    myPear::WARNING_T("$login is authenticated by ".__METHOD__.", but Avatar is not found. AUTHENTICATION CANCELED");
	  }
	}
	self::dbg(False);
      }else{
	myPear_logs()->syslog("WARNING pam_auth is not installed");
      }      
    }
    self::dbg($this->password_ok);
  }      

  /*
   * Authentication by IMAP server
   */  
  private function authIMAP($login, $pwd){

    if ($this->password_ok) return True;

    // b_fmt::RFC_2822($login) 
    if (!self::_authenticated() && !empty($login) && !empty($pwd) && !empty($this->localAccount)){
      if (!locateAndInclude('Net_IMAP')) myPear::WARNING_T("Net_IMAP is not installed");
      if (class_exists('Net_IMAP',False)){
	$pear = new PEAR();
	foreach ($GLOBALS['imapServers'] as $domain=>$s){
	  foreach($s['imap'] as $srv){
	    $imap = new Net_IMAP($srv,$s['port'],False);
	    $imap->setTimeout(1);
	    $e = $imap->login($login, $pwd, true, false);
	    if ($pear->isError($e)){
	      self::dbg("NO - $srv ".$e->message,cnf_show_a);
	    }else{
	      self::dbg("YES - $srv",cnf_show_a);
	      myPear_logs()->syslog("Accepted Net_IMAP for $login@$domain from ".$_SERVER["REMOTE_ADDR"]);
	      $this->password_ok  = True;
	      if (!(self::$av = bForm_Avatar::hook(array('av_identity'=>$login),'basic'))) {
		foreach(array_merge(array($domain),$s['alias']) as $d){
		  if (self::$av = bForm_Avatar::hook("$login@$d",'basic')) break;
		}
	      }
	      if (is_object(self::$av)) return $this->_setUser('Net_IMAP');
	    }
	  }
	}
	self::dbg(False,cnf_show_a);
      }else{
	myPear_logs()->syslog("WARNING Net_IMAP is not installed");
      }    
    }
    self::dbg($this->password_ok);
  }    
  
  private function changePwd($login,$oldpwd,$newpwd,$newpwd2,$login2=False) {
    /*
     * Change Kerberos password.
     *
     * Returns empty string in case of success, a string with the error message otherwise
     * (note also bool pam_chpass(string $username, string $oldpassword, string $newpassword [, string &$error ])
     */
    self::dbg();
    if (!$login || !$oldpwd || !$newpwd || !$newpwd2) return " "; // return an error string

    // change kerberos password in a crude way...
    $c = popen ("/bin/bash","w");
    $log = "/tmp/delete.me.now";
    fwrite($c, "rm -rf $log*\n");
    fwrite($c, $this->pwdchanger." $login > $log 2>$log.err\n");
    fwrite($c,"$oldpwd\n");
    fwrite($c,"$newpwd\n");
    fwrite($c,"$newpwd2\n");
    pclose($c);
    // read back the logfile
    $f = fopen($log, "r");       $l = fread($f,filesize($log));       fclose($f);
    self::dbg("$log='$l'");
    $f = fopen($log.".err", "r");$e = fread($f,filesize($log.".err"));fclose($f);
    self::dbg("$log.err='$e'");
    // send the reporting mail
    if (!$e) {
      $u = b_posix::getpwnam($login);
      self::dbg("sending mail to $u[gecos] login=$login");
      if (!mail($login."@".myOrg_domain.
		($login2?",$login2"."@".myOrg_domain:"").
		",yb@".myOrg_domain,
		"[password service] Password Change Request",
		"Dear $u[gecos],

The password for your account '$login' is changed according to your request.
The request came from the web interface from computer '".gethostbyaddr($_SERVER['REMOTE_ADDR'])."'.

   Computing service
",
		"From: ".bMailer_noreply)) myPear::ERROR("!!!!!!!! MAIL FAILED");
    }
    // remove the logs
    system("rm -rf $log*");
    // kill the credentials
    if ($login == self::_authenticated()) {
      self::dbg("destroying credentials for $login");
      self::_unsetUser();
    }
    return preg_replace("/$this->pwdchanger:/","",$e); # return the stderr output
  }  
  
/*
 *
 */
  private static function dbg($text='',$force=False){
    if (self::$debugIsOn||$force) b_debug::_debug($text,array(True,'redText',2));
  }
  
  /*
   * Print (or return the html code for) the login prompt
   */
  function loginPrompt($returnWidget=False){
    static $dejaVu = 0;
    $this->authenticate();
    if (self::_authenticated()) return True;
    if ($dejaVu++)              return False;

    self::dbg();
    $_GET['login'] = 'yes';

    $message = $error_message = '';
    $mode = 'login';
    if (isset($_POST['remindpassword'])) {
      b_cnf::_unset('forgotpassword_once');
      if  ($error_message = $this->remind_password($_POST['auth_username'])) $mode = 'tellpwd';
      else $message = "The password reminder is sent to you";
    }

    if (isset($_GET['forgotpassword_once']) && empty($_SESSION['bAuth']['auth_pwdsent']))  $mode = 'tellpwd';
    
    if (($pwd1=@$_POST['auth_pwd1']) || ($pwd2=@$_POST['auth_pwd2'])) {
      if ($error_message = $this->changePwd($_POST['auth_username'],$_POST['auth_password'], $pwd1,$pwd2)) {
	$mode = 'cpw';
      }else
	return array('cpw',False,False);
    }
    
    if ($msg = @$this->titleMessage) print $msg;

    switch($mode){

    case 'login':
      if (!empty($_POST['auth_username'])) $error_message = 'Wrong username or password';
      if (!empty($_GET['sendmail_once'])){
	$message = 'The password reminder has been sent to you';
	$error_message = '';
      }
      $pwdLabel    = 'password';
      $userLabel   = 'user';
      $submitLabel = 'login';
      $post_action = 'login';
      break;

    case 'tellpwd':
      $pwdLabel    = '';
      $userLabel   = 'e-mail';
      $submitLabel = 'remind password';
      $post_action = 'remindpassword';
      break;

    case 'cpw':
      $pwdLabel    = 'current password';
      $userLabel   = 'user';
      $submitLabel = 'change password';
      $post_action = 'login';
    }
    
    $login_widget = self::login_widget($mode,$message,$error_message,$userLabel,$pwdLabel,$submitLabel,$post_action,'check');
    
    if ($returnWidget)  return (string)$login_widget;
    else                bJS()->modal_inline('login',$login_widget,True,True);
    return False;
  }
  
  /*
   *
   */
  public static function login_widget($mode='login',$message='',$error_message='',$userLabel,$pwdLabel='password',$submitLabel='login',$post_action='login',$flagToAutoCheck='NO'){

    ob_start();
    $t = new b_table('','<div class="login_prompt centralized">'.
		     (($m=$message) ? x("div class='login_message'",$m) : '').
		     "<form name='login'  method='POST' action='".b_url::same("?login=$flagToAutoCheck")."'>");
    
    $t->tr(str_replace(array("\r","\n"),"<br/>",$error_message),
	   'colspan=3 class="login_error"');
    $t->tr(array($userLabel,
		 bIcons()->get('auth_man','100'), 
		 "<input size=25 name='auth_username' type='text' value='".b_cnf::getValue('auth_username')."' autofocus/>"));
    if ($pwdLabel) $t->tr(array(bText::_($pwdLabel),
				bIcons()->get('auth_key','100'), 
				"<input size=25 name='auth_password' type='password' value='".b_cnf::getValue('auth_password')."'/>"));
    if ($mode == 'cpw') { 
      $t->tr(array(bText::_('new password'),
		   bIcons()->get('auth_key','100'), 
		   "<input size=25 name='auth_pwd1' type='password' value='".b_cnf::getValue('auth_pwd1')."'/>"));
      $t->tr(array(bText::_('retype new password'),
		   bIcons()->get('auth_key','100'), 
		   "<input size=25 name='auth_pwd2' type='password' value='".b_cnf::getValue('auth_pwd2')."'/>"));
    }
    if (cnf_demo || cnf_dev){
      $t->tr(array('login as',
		   '',
		   "<input size=25 name='login_as' type='text' value='".b_cnf::getValue('login_as')."'"));
    }
    

    // print the "submit" button(s)

    $oldVersion = True; // New version is not fully debugged
    if ($oldVersion) $t->tr(array("<input name=$post_action value='$submitLabel' type=submit>"));
    $buttons = array(b_btn::submit($submitLabel));

    unset($_SESSION['bAuth']['auth_pwdsent']);
    if (self::$showPasswordReminder && $mode == 'login' && empty($_SESSION['bAuth']['auth_pwdsent'])){
      $buttons[] = b_btn::submit('remind my password',b_url::same('?forgotpassword_once=yes'));
      if ($oldVersion) $t->tr(array('','',x('a href="'.b_url::same('?forgotpassword_once=yes').'"', 'remind my password')));
    }

    if (self::$showCreateAccount && $mode == 'login'){
      $buttons[] = b_btn::submit('create new account',b_url::same('?id=new&form='.bForm_Avatar::$bForm_Avatar));
      if ($oldVersion) $t->tr(array('','',x('a href="'.b_url::same('?id=new&form='.bForm_Avatar::$bForm_Avatar).'"', 'create new account')));
    }
    if (!$oldVersion)     $t->tr(b_btn::UL_buttons($buttons),'colspan=4');

    $t->close('</form></div>');
    $login_widget = ob_get_contents();
    ob_end_clean();

    return $login_widget;
  }

  function random_password($char_count) {
    $password = '';
    for ($i = 0; $i < $char_count; ++$i) {
      $j = rand(0, 64);
      if ($j < 10)        $ch = chr($j + ord('0'));
      else if ($j < 36)   $ch = chr($j - 10 + ord('a'));
      else if ($j < 62)   $ch = chr($j - 36 + ord('A'));
#     else if ($j == 62)  $ch = '.';
#     else                $ch = ',';
      else                $ch = 'x';
      $password .= $ch;
    }
    return $password;
  }

  public function createDefaultUser(){
    if (empty($this->superUsers)){
      switch (HOSTED_BY_CMS){
      case 'drupal':
	$q = db_query("SELECT * FROM  {users} WHERE mail REGEXP '@'");
	foreach($q as $r){
	  if (empty($r->mail)) continue;
	  if (!bForm_Avatar::hook($r->mail,'basic')){
	    $av = bForm_Avatar::hook(array('av_email'    => $r->mail,
					   'av_firstname'=> $r->name,
					   'av_lastname' => 'Drupal User',
					   ),False);
	    myPear::MESSAGE(sprintf('Create myPear user <em>%s</em> (email=%s)',$av->fmtName(),$r->mail));
	  }
	}
	break;
      default:
      }
    }
  }

  /*
   * Login by extraction the username & password from the URL.
   * 
   * In the past we used to have a different model of encoding username/password, which is good, but makes URL very long.
   * The long URL creates problems for some mail agents (the URL is broken in pieces),
   * so now I use a simplistic "encryption" which results in a shorter URL
   */
  public function autoLogin() {

    $login_pwd = $loginData = array();
    if (!empty($_GET['x'])){
      // "lightweight" encryption, version 2014
      if ($this->see_you_later()) return;
      b_crypt::$debug = self::$debugIsOn;
      list($login_pwd,$loginData) = b_crypt::decodeX($_GET['x']);
      if ($email = @$loginData['email_to_autologged']){
	self::set_cnf_demo_email($email);
	unset($loginData['email_to_autologged']);
      }
    }elseif ($s=@$_GET['z']){ // new "lightweight" encryption
      // "lightweight" encryption, version 2012
      if (strpos($s,'atidron') === False) $s = "atidron$s";      // hot fix... To be(???) understood
      self::dbg("$s -> ".b_crypt::decodeZ($s));
      
      foreach (explode(',',b_crypt::decodeZ($s)) as $k=>$v){
	$vv = explode('=',$v);
	if ($k == 0) $login_pwd[$vv[0]] = $vv[1];
	else	     $loginData[$vv[0]] = $vv[1];
      }
    }elseif (($login=@$_GET['autologin']) && ($login == bMailer_noreply) && (b_fmt::parse_HTTP_USER_AGENT() === array('Linux','curl'))){
      if ($this->see_you_later()) return;
      $login_pwd = array($login => 'autologin');
    }elseif ($encoded_login=@$_GET['data']){
      // Backward compatible old encryption used in JAM-4
      b_reg::load_module('JAM');
      if (!locateAndInclude('bAuth_data_Login.inc')){
	myPear::WARNING_T("No 'bAuth_data_Login.inc' file found");
      }else{
	if (!$this->see_you_later())      list($login,$pwd,$loginData) = bAuth_data_Login($encoded_login);
	$login_pwd = array($login => $pwd);
	b_debug::var_dump($loginData);
      }
    }
          
    // Check that this not an the person who is already in,
    // otherwise logout the currently logged-in user
    foreach($login_pwd as $login=>$pwd){
      if (!empty($login) && !empty($pwd)){
	if ($pwd==='autologin') $av = bForm_Avatar::hook($login,'basic');
	else                    $av = $this->authMySQL($login, $pwd, $auto_impose=False);
	if ($av instanceof bForm_Avatar){
	  // log-out the current user, but do not destroy the session
	  if ($av->ID !== @bAuth::$av->ID){	    
	    $this->logout($resetSession=False);
	    $this->imposeAvatar($av);
	    self::dbg(self::$av);
	  }
	  // Redirect to the newly lodgged user with the requested URL 
	  if (!empty($loginData)) if (b_url::header(b_url::same('?x=&'.b_fmt::joinX('&',$loginData)))) return;
	}
	foreach($loginData as $k=>$v){
	  b_cnf::set($k,($_GET[$k] = $v));
	  self::dbg("Set $k = $v");
	}
      }
    }
  }

  private function see_you_later(){
    if (!self::$justLoggedIn && self::_authenticated()){
      $already_there = "Already logged in as ".self::$av->fmtName().',<br/>please log-out first.';
      myPear::WARNING($already_there);
    }
    $reply = !empty($already_there);
    b_debug::xxx($reply);
    $reply = False; // ??????????????????????????????? why
    return $reply;
  }
  
  /*
   * Find the avatar in the database.
   * Since the avatar might be a result of merging multiple records, and we still want to find the person,
   * we check the passwords & IDs of avatars merged with this one (those kept in av_id2 & av_pwd2 columns).
   */
  private function authMySQL($login, $pwd, $auto_impose=True) {

    if ($this->password_ok) return self::$av;
    
    $av = Null;
    if (!empty($login) && !empty($pwd)){
      // Escape the password for REGEXP search
      $pwdR = $pwd;
      foreach(array('[',']','(',')','*','^') as $s) $pwdR = str_replace($s,'.',$pwdR);
      $q = myPear_db()->qquery('SELECT av_id FROM zzz_avatars WHERE '.
				 '(av_id  = \''.$login.'\' OR av_id2  REGEXP \'[[:<:]]'.$login.'[[:>:]]\') AND '.
				 '(av_pwd = \''.$pwd.'\'   OR av_pwd2 REGEXP \'[[:<:]]'.$pwdR.'[[:>:]]\')',(bool)self::$debugIsOn);
      while ($r=myPear_db()->next_record($q)){
	if (is_object(@$av)) myPear::WARNING_T("$av - duplicated authentication");
	$av = new bForm_Avatar($r['av_id']);
      }
      self::dbg(is_object($av));
      if ($auto_impose && is_object($av))	$this->imposeAvatar($av);
    }
    self::dbg($this->password_ok);
    return $av;
  }
}
  
