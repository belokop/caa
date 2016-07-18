<?php
locateAndInclude('bForm');

// Use or not to use Templates class for setting up the signature.
// Templates are experimental, so lets not use it for production
define('signature_as_template', False);

class bForm_Organization extends bForm {
  
  public static $default_organization = array('org_name' =>          'Example Organization',
					      'org_name_short'=>     'Org',
					      'org_country'=>        'PG',
					      'org_code' =>          'organization_1',
					      'org_signature_dir' => '------- Director',
					      'org_domain' =>        'example.com',
					      );

  public $debug = False;
  public $tips = array('org_code'  => 'A unique short name (like "CNN", "Harvard", etc.)&nbsp;for the Organization',
		       'org_domain'=> 'Domain name for the Employees E-mail addresses - user\@domain',
		       );
  
  /*
   * Delete organization
   */
  static public function delete($org_id){
    if (cnf_dev) b_debug::xxx(' ');
    foreach(array('zzz_lists' => 'l_parent',
		  'zzz_units' => 'u_parent',
		  'zzz_organizations'=>'org_id',
		  ) as $table=>$id){
      $q = myPear_db()->query("SELECT * FROM $table WHERE $id = '$org_id'");
      while ($r = myPear_db()->next_record($q)){
	switch($table){
	case 'zzz_lists': bList::deleteList($r['l_id']); break;
	case 'zzz_units': bUnit::deleteUnit($r['u_id']); break;
	}
      }
      if ($table == 'zzz_organizations') myPear_db()->qquery("DELETE FROM $table WHERE $id = '$org_id'",cnf_dev);
    }
  }

  /*
   * Get org_id for all the known organizations
   */
  public static function getAll(){
    $reply = array();
    $q = myPear_db()->query("SELECT org_id FROM zzz_organizations ORDER BY org_name");
    while ($r = myPear_db()->next_record($q)){
      $reply[] = $r['org_id'];
    }    
    return $reply;
  }

  /*
   * Get the organization ID from the code.
   * Create an example organization if the database is empty.
   * 
   * @param org - The organization code. 
   *              If empty, the organization is defined by $_GET['org'] argument
   * @param arbitrary - to_use OR not_to_use an arbitrary organization if the the arguments are empty
   *  
   */
  public static function get_myOrg($org_id,$arbitrary=False){
    static $organizations = array();
    
    if (!myPear_db()->isReady())  return Null;
    
    // Check the cache
    $org_id = self::org_code_2_org_id($org_id);
    if (empty($org_id)){
      if (!($all_organizations=self::getAll())){
	// The database is empty, create a dummy Organization instance
	$org = new bForm_Organization('empty');
	$org->updateDB(self::$default_organization);
	myPear::INFO("Creating example Organization \"".$org->getValue('org_name',1)."\"");
	return self::get_myOrg($org->ID);
      }else{
	// The database is not empty, but the organization does not exist.
	// Ask for an arbitrary one
	return self::get_myOrg($all_organizations[0],True);
      }
    }

    // Finally store the instance in cache, since the organization is the most frequently accessed object
    if (empty($organizations[$org_id])){
      $organizations[$org_id] = new bForm_Organization($org_id);
      b_debug::xxx("Organization \"".$organizations[$org_id]->getValue('org_name',1)."\"",array(cnf_dev));
    }
    if ($arbitrary) myPear::INFO("Loading organization \"".$organizations[$org_id]->getValue('org_name',1)."\"",
				 "No organization declared, getting an arbitrary one");
    b_debug::xxx("Organization \"".$organizations[$org_id]->name()."\"",array(cnf_dev));
    return $organizations[$org_id];
  }

  private static function is_orgID_valid($org_id){
    $org_id = trim($org_id);
    $reply  = (myPear::is_int($org_id) || (is_string($org_id) && preg_match('/^[a-zA-Z0-9]*$/',$org_id)));
    b_debug::xxx($reply,array(2));
    return $reply;
  }

  /*
   * get org_id from org_code
   */
  private static $code2id = array(); 
  public static function org_code_2_org_id($org_id,$default='nordita'){
    locateAndInclude('b_registry');
    b_reg::_(False);
    b_reg::load_module(b_reg::$current_module);

    // Be shure that the org code is not fake
    if (!self::is_orgID_valid($org_id)) $org_id = Null;

    if (empty($org_id) && ($o=b_cnf::get('org')) && self::is_orgID_valid($o)) $org_id = $o;
    if (empty($org_id) && function_exists($f=b_reg::$current_module.'_select_organization')) $org_id = $f();
    if (empty($org_id) && !empty($default)) $org_id = $default;

    if (!empty($org_id)){
      if (empty(self::$code2id[$org_id])){
	$q = myPear_db()->query("SELECT org_id,org_code FROM zzz_organizations WHERE org_code='$org_id' OR org_id='$org_id' LIMIT 1");
	while($r = myPear_db()->next_record($q)){
	  if (empty($r['org_id'])) continue;
	  self::$code2id[$r['org_id']]   = $r['org_id'];
	  self::$code2id[$r['org_code']] = $r['org_id'];
	}
      }
      $org_id = @self::$code2id[$org_id];
    }
    return $org_id;
  }

  /*
   *
   */
  private $default_affiations = array();
  function __construct($id, $mode='RO') {
    static $list_themes = array();
    parent::__construct($id, $mode, True);
    $this->default_affiations = myPear_set_roles('affiliation');

    // Set the theme defaulted to the "organization code"
    if (!$this->getValue('org_theme')){
      if (function_exists('list_themes')){
	foreach(list_themes() as $theme=>$desc) $list_themes[$theme] = $desc->status;
      }
      if (!empty($list_themes[$org_code=$this->getValue('org_code')])){
	$this->updateDB(array('org_theme'=>$org_code));
      }
    }
  }

  /*
   * Set the affiliation used in the organization
   */
  function setAffiliation($org_affil=array()){
    if ((int)$this->ID){
      
      // Use a default affiliation if the input is empty
      if (empty($org_affil)){
	// Set the default affiliation as Alumni + Staff
	if (!$this->getValue('org_affil',True)){
	  foreach($this->default_affiations as $role=>$d) $org_affil[$role] = $d['active'];
	}
      }
      
      // Save the affiliation directly to the database, since the dialog uses "virtual check-boxes"
      if (!empty($org_affil)){
	$this->updateDB(array('org_affil'=>serialize($org_affil)));
      }
    }
  }

  /*
   *
   */
  private $myNetworks = array('127.0.0.1',
			      '::1');
  function setMyNetworks($networks=array()){
    foreach($networks as $net) $this->myNetworks[] = $net;
  }

  function getMyNetworks(){
    if (cnf_dev) b_debug::print_r($this->myNetworks,'this->myNetworks','nosort');
    return $this->myNetworks;
  }

    function isOnMyNetwork($REMOTE_ADDR=Null){
      if (empty($REMOTE_ADDR)) $REMOTE_ADDR = $_SERVER['REMOTE_ADDR'];
      $remote_addr_a = explode('.',$REMOTE_ADDR);

      $reply = False;
      foreach($this->myNetworks as $net){
	$a = explode('.',$net);
	if (array_intersect($a,$remote_addr_a) === $a){
	  $reply = True;
	  break;
	}
      }
      $this->dbg($reply,$this->debug||cnf_dev);
      return $reply;
    }

  /*
   * Get the affiliations 
   */
  function getAffiliation($only_active=False){
    $reply = array();
    if ((int)$this->ID){
      $this->setAffiliation();
      $org_affil = $this->getValue('org_affil',True);
      foreach(unserialize($org_affil) as $role=>$s){
	if ($only_active){
	  if ($s) $reply[] = $role;
	}else{
	  $reply[$role] = ($s ? 1 : 0);
	}
      }
    }
    return $reply;
  }
  
  /*
   *
   */
  function defineVariables() {
    if ($this->defineVariable_counter) return;
    $this->defineVariable(array('_virt_favicon'    => 'Favicon',
				'org_code'         => 'Code <SKIP> (code)',
				'org_name'         => 'Full name of the organization',
				'org_name_short'   => 'Short name',
				'org_country'      => 'Country',
				'org_domain'       => 'DNS domain name for e-mails',
				'org_signature_dir'=> 'Template for the signature<br/>of the Head of the Organization',
				'org_affil'        => 'Who is affiliated with the organization',
				'org_theme'        => 'Preferred theme (appearance)',
				),True);
    $this->defineTypes(array('isRO'        => array('org_signature_dir'=> signature_as_template),
			     'IDV'         => array('org_domain'       => "e-mail is &lt;user&gt;@domain"),
			     'expected_ext'=> array('_virt_favicon'    =>''),
			     ));
  }

  /*
   * Get the ea CV's
   */
  private $eaCV = Null;
  public function eaCV($org_id=Null,$av_id=Null){
    if ($this->eaCV === Null){
      locateAndInclude('bList_eaCV');
      if (empty($org_id)) $org_id = $this->ID;
      $this->eaCV = new bList_eaCV($org_id,$av_id);
    }
    return $this->eaCV;
  }
  
  /*
   * Get the EA unit
   */
  private $eaMembers = Null;
  public function eaMembers(){
    if (($this->eaMembers === Null) && ($r = b_reg_bUnit::get_description(RANK__ea_member))){
      $unit = $r['class'];
      locateAndInclude($r['class'],'fatal');
      $this->eaMembers = new $unit($this->ID,RANK__ea_member);
    }
    return $this->eaMembers;
  }

  /*
   *
   */
  public function org_bList($blist){
    $reply = ($blist == 'orgGroups'
	      ? $this->orgGroups()
	      : $this->orgTrades());
    return $reply;
  }


  /*
   * Get the organizational groups (departments)
   */
  private $orgGroups = Null;
  public function orgGroups(){
    if ($this->orgGroups === Null){
      locateAndInclude('bList_orgGroups');
      $this->orgGroups = new bList_orgGroups($this->ID);
    }
    return $this->orgGroups;
  }
  
  /*
   * Get the organizational trades
   */
  private $orgTrades = Null;
  public function orgTrades(){
    if ($this->orgTrades === Null){
      locateAndInclude('bList_orgTrades');
      $this->orgTrades = new bList_orgTrades($this->ID);
    }
    return $this->orgTrades;
  }
  
  /*
   *
   */
  function isReadable(){
    return True;
  }

  function isManager(){
    if ($reply = defined('myOrg_ID')){
      myPear_access(EA_MODULE);
      $reply = EA_manager_here;
      $this->dbg($reply);
    }
    return $reply;
  }

  function isWritable(){
    return b_cnf::check_conditions(array(True     =>array('mngr'=>$this->isManager() && ($this->getValue('org_code') == myOrg_code)),
					 'default'=>parent::isWritable()),
				   $this);
  }

  /*
   */
  function name($short_name=False) {
    if ((int)$this->ID){
      $reply = ($short_name
		? $this->getValue('org_name_short')
		: $this->getValue('org_name'));
    }else{
      $reply = 'the Organization';
    }
    $this->dbg($reply);
    return $reply;
  }

  /*
   *
   */
  function getValue($name,$asIs=True,$description_MBE=True) {

    if (($v=$this->getValue_fromCache($name,$asIs,$description_MBE)) !== Null) return $this->getValue_return('cache',$v);

    switch($name){
    case 'org_name_short':
      if ($reply=parent::getValue('org_name_short',$asIs,$description_MBE)) return $reply;
      $reply = parent::getValue('org_name',$asIs,$description_MBE);
      $hint = 'n';
      myPear::WARNING("Organization short name is not defined, using the full name '$reply'");
      break;
    case 'Z_org_signature_dir': 
      $hint = 's';
      $reply = $this->director_signature();
      break;
    case 'ZZZ _virt_favicon':   
      $hint = 'f';
      $reply = $this->getFavicon();
      break;
    default:
      return parent::getValue($name,$asIs,$description_MBE);
    }
    
    // We got the value. Now check how good is it
    return $this->getValue_return($hint,$this->getValue_toCache($name,$reply));
  }

  /*
   *
   */
  function getForm(){
    static $dejaVu = 0;
    if (!$dejaVu++){ // protection against infinite loop, happens if the form is auto-created
      if (!(int)$this->ID) $this->MODE = 'RW'; 

      myPear::H1('Organization description');

      $submit = array();
      if (0) if ($this->MODE != 'RW' && $this->isWritable()) {
	  $submit['update groups'] = b_url::same("?ed_bList=bList_orgGroups&arg1_once=$this->ID");
	}
      
      $org_code = $this->getValue('org_code',True);
      if (!empty($org_code) && (int)$this->ID){
	$_POST['org_code'] = $org_code;
	$this->isRO['org_code'] =  !superUser_here;
      }
      
      $this->getFormOpen();
      
      // General description, reset favicon cache first.
      b_os::file_match(array($this->uploadedFile_name('_virt_favicon',array(),False)),
		       $this->uploadsDir,
		       True);
      
      $dialog = array('_virt_favicon'  => 'uploadFile',
		      'org_name'       => array('textField'),
		      'org_name_short' => array('textField'),
		      'org_country'    => array('textFieldCountry'),
		      'org_code'       => array('textField'));
      if (!signature_as_template) $dialog['org_signature_dir'] = 'textArea';
      if (!$this->getValue('org_signature_dir')) $this->formDB['org_signature_dir'] = '------- Director';
      $this->formBlock('Description',
		       'Description',
		       $dialog,
		       array('i'=>'edit',
			     'l'=>b_url::same(array('form' => get_class($this),
						    'id'   => $this->ID))));
      
      
      // Signature template
      if (signature_as_template){
	$this->textField('org_signature_dir',
			 array(bIcons()->getButton(array('i'=>'i-edit',
							 'd'=>'edit template',
							 'l'=>b_url::same('?tab_mypear=tab__templates&id='.
									  $this->getValue('org_signature_dir',1))))),
			 $this->director_signature());
      }
      
      //
      // E-mails domain      
      $this->formBlock('InternetDomain',
		       'E-mails',
		       array('org_domain'=>'textField'),
		       array('i'=>'edit',
			     'l'=>b_url::same(array('form' => get_class($this),
						    'id'   => $this->ID))));
      
      //
      // Affiliated. Makes sense only if EA module presents
      $org_affil = array();
      locateAndInclude('b_registry');
      if (b_reg::get(EA_MODULE)){
	foreach($this->getAffiliation() as $role=>$s){
	  $f = "_virt_aff_$role";
	  $this->isMBE[$f] = True;
	  $this->isRO[$f]  =  (@$this->default_affiations[$role]['mode'] === 'RO');
	  $this->defineVariable($this->default_affiations[$role]['group'].' '.x('(',$this->default_affiations[$role]['title']),$f);
	  $dialog[$f] = array('checkBox');
	  // "old" value
	  $this->formDB[$f] = ($s ? 'on' : 'off');
	  // "new" value
	  $this->getValue($f);
	  $org_affil[$role] = ($this->checkBoxIsOn($f) ? 1 : 0);
	}
	$this->setAffiliation($org_affil);
	$this->formBlock('Affiliates',
			 "Affiliates to be shown on the \"Employees / Alumni\" page",
			 $dialog,
			 array('i'=>'bb-group',
			       'l'=>b_url::same(array('form' => get_class($this),
						      'id'   => $this->ID))));
      }
      
      //
      // Virtual fields - bList_orgTrades & bList_orgGroups
      if ((int)$this->ID){
	$this->tips['_virt_orgTrades']  = 'List of "Employee Types" used in the Organization';
	
	$this->defineVariable(array('_virt_orgTrades' => $this->orgTrades()->name(),
				    '_virt_orgGroups' => $this->orgGroups()->name(),
				    ),True);
	$this->defineTypes(array('isRO'        => array('_virt_orgGroups'   => True,
							'_virt_orgTrades'   => True),
				 'isUL'        => array('_virt_orgGroups'   => True,
							'_virt_orgTrades'   => True)));
	$this->formBlock_bList(array('bList_orgTrades'=>array('method'=>'get_trades',
							      'arg1'  => $this->ID,
							      'field' =>'_virt_orgTrades',
							      'icon'  =>'i_users'),
				     'bList_orgGroups' =>array('method'=>'get_groups',
							       'arg1'  => $this->ID,
							       'field' =>'_virt_orgGroups',
							       'icon'  =>'i_users')));      
      }
      // close the form
      $this->closeForm('submit',$submit);
    }
  }

  /*
   *
   */
  function uploadedFile_name($name,$argsP=array(),$ext=True){
    switch($name){
    case '_virt_favicon':
      $fn = sprintf("favicon-%s%s",$this->getValue('org_code',1),($ext ? '.ico' : ''));
      break;

    default:
      $fn = parent::uploadedFile_name($name,$argsP,$ext);
    }
    $this->dbg($fn);
    return $fn;
  }

  /*
   * Files for Organization class (currently favicon only) are uploaded to the myPear area
   */
  function uploadInit_path($module_path=array(),$module=Null){
    return parent::uploadInit_path(array(),myPear_MODULE);
  }

  /*
   *
   */
  function getCode(){
    return $this->getValue('org_code',True,True);
  }

  /*
   *
   */
  function getTheme(){
    return $this->getValue('org_theme',True,True);
  }


  /*
   *
   */
  protected function getValue_validate_value($k,&$v){
    $err_msg = '';
    if (!myPear::is_empty($v)){
      $err_msg_parent = $err_msg = parent::getValue_validate_value($k,$v);

      switch($k){
      case 'org_code':
	$q = myPear_db()->query("SELECT org_id,org_code FROM zzz_organizations WHERE org_code='$v' AND org_id !='".$this->ID."' LIMIT 1");
	if (myPear_db()->num_rows($q) >0 ) $err_msg = "Short name '$v' is already taken";
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
  function getFavicon(){
    // Try first the old convension (the icon might be in the "images directory")
    $reply = bIcons()->get(array('X'=> 'X', // don't go to "icons legend"
				 'i'=>'f-'.$this->getValue('org_code',1),
				 'd'=>$this->name()));
    // 
    if (empty($reply)) {
      /*
      $this->uploadInit();
      $reply = b_os::file_match(array($this->uploadedFile_name('_virt_favicon',array(),False)),
				$this->uploadsDir);
      if (!empty($reply)){
	$img   = $this->uploadsURL . $reply;
	$reply = "<img class='opacity_off' src='$img' title='".$this->name()."' alt='' />";
      }
      */
      list($reply,$error_message) = $this->saveFile('_virt_favicon');
      $this->dbg($error_message,cnf_dev);
    }
    $this->dbg(b_fmt::escape($reply));
    return $reply;
  }

  function director_signature(){
    if ((int)$this->ID){
      locateAndInclude('bTpl');
      bTpl::$owner = $this->ID;
      $tpl = new bTpl(array('title' => __METHOD__,
			     'render'=> Null,
			     'body'  => $this->name()." director\n-----------------",
			     'id'    => $this->getValue('org_signature_dir',True)));
      $POST['org_signature_dir'] = $tpl->id;
      $signature = $tpl->get();
    }else{
      $signature = '';
    }
    $this->dbg($signature);
    return $signature;
  }

  function domain() {
    $domain = $this->getValue('org_domain',True);
    if (empty($domain)) $domain = 'example.com';
    return $domain;
  }
}
