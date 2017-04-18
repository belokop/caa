<?php
/*
 * Define the employment groups (aka "roles") for the organization.*
 * Say array('adm' => 'Administrators',
 *           'fac' => 'Faculty',...);
 */
locateAndInclude('bList');

class bList_orgEmpTypes extends bList{
  
  var $list_quering = array('lm_key'  =>'Code',
			    'lm_value'  =>'Employee Type');
  var $list_showing = array(//'lm_key'  =>'Code',
			    'lm_value'  =>'Employee Type',
			    'empTitles' =>'',
			    'n_members' =>'',
			    );
  var $MODE  = 'RO';

  public $empTitles = array();
  
  protected $isReady  = True;

  /*
   *
   */
  function __construct($org_id){

    // Initialise the EA module access class
    locateAndInclude('APIaccess_ea');
    if (function_exists('EA_access')) EA_access()->getRank();

    // Instantiate the bList
    parent::__construct(myOrg_ID);
  }

  /*
   * EA module is optional, hence we have to check the environment
   */
  function isManager(){
    myPear_access(EA_MODULE);
    return EA_ORG_manager_here;
  }
  
  function isWritable(){
    return b_cnf::check_conditions(array(False=>(array('not EA' => (b_reg::$current_module != EA_MODULE))),
                                         'default' => parent::isWritable()),
				   $this);
  }
  
  protected function isMemberEditable(){
    return $this->isWritable();
  }
  
  /*
   * The type is not deletable unless it is not used
   */
  protected function isMemberDeletable(){
    static $noname = '???';
    $nMembers = count($this->get_empByTypes($this->r['lm_key']));
    if (empty($this->r['lm_value'])) $this->r['lm_value'] = $noname;
    //? if ($nMembers && $this->isWritable()) myPear::MESSAGE("$nMembers members in \"".$this->r['lm_value']."\"");
    return b_cnf::check_conditions(array(True =>array('empty' => empty($this->r['lm_key'])),
					 False=>array('RO'    => !$this->isWritable(),
						      '!empty'=> ($nMembers > 0)),
					 'default' => True),
				   $this);
  }

  /*
   *
   */
  function get_defaultEmpType(){
    $reply = '?';
    foreach($this->get_empTypes() as $k=>$v){    
      if (stripos($k.$v,'researchers')!==False) $reply = $k;
    }
    $this->dbg($reply,True);
    return $reply;
  }

  function create_textKey($text,$parent=Null){
    if (empty($parent)){
      $counter = count($this->getMembers());
    }else{
      static $counts = array();
      if (empty($counts[$parent])) $counts[$parent] = 0;
      $counter = $counts[$parent]++;
    }
    if (!function_exists('title_compactor')){
      function title_compactor($text){
	foreach(array(myOrg_name   => '',
		      myOrg_nameS  => '',
		      ' and '      => '',
		      ' of '       => '',
		      'scientific' => '',
		      'researcher '=> '',
		      'researchers'=> '',
		      'professor'  => 'Prof',
		      'associate'  => 'Assoc',
		      'assistant'  => 'Ass',
		      'affiliate'  => 'Aff',
		      'visiting'   => 'V.',
		      'junior'     => 'J.',
		      'masters'    => 'M.',
		      'building'   => 'B.',
		      'board '     => 'B.',
		      'corresponding'  => 'C.',
		      'administrative' => 'Adm',
		      'administration' => 'Admins',
		      'coordinator'    => 'Coord',
		      ) as $f=>$t) $text = str_ireplace($f,$t,$text);
	$text = trim(str_replace(' ','',$text));
	b_debug::xxx($text);
	return $text;
      }
    }
    //    $reply = sprintf("%03d_%d",$counter,b_crypt::hash(strToLower($text)));
    $reply = sprintf("%03d_%s",$counter*10,title_compactor($text));
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  /*
   * Input form
   */
  protected function getFormAdd(){
    myPear::H1('Updating '.$this->getValue('l_name'));
    
    // Create the record if it is empty
    if (empty($this->member_ID))      $this->updateMember();
    
    // Show the dialog    
    $this->bForm($this->list_quering,$inverted=True);
    $this->f->isRO['lm_value'] = True;
    if ($lm_key=$this->f->getValue('lm_key',True)) $this->f->isRO['lm_key'] = True;
    foreach($this->list_quering as $k=>$v){
      if($k == 'lm_key'){
	$this->f->textHidden($k,(empty($lm_key) ? $this->create_textKey() : $lm_key));
      }else{
	$this->f->textField($k);
      }
    }
    $this->f->closeForm();
  }

  /*
   * Check the sanity
   */
  function getFormAdd_OK(){
    $types = $this->get_empTypes();
    $lm_id  = $this->getValue('lm_id');
    $code   = $this->getMemberValue($lm_id,'lm_key');

    $reply = parent::getFormAdd_OK();
    if (!empty($types[$code]) && empty($lm_id)) {
      $msg = "Code \"$code\" is already in use. Please choose another one.";
      if (is_object($this->f)) $this->f->errorsInTheForm['lm_key'] = $msg;
      $reply = False;
    }
    return $reply;
  }

  /*
   * Titles & title codes
   */
  private $_code2text = Null;
  private $_text2code = Null;
  private $_text2titl = Null;
  private $_code2tits = Null;
  private $title2emp_type = Null;
  private function set_code2text(){
    if($this->_code2text === Null){
      static $counter = 0;
      foreach($this->getMembers() as $lm_id=>$r){
	if (empty($r['lm_value'])) $r['lm_value'] = str_repeat('?',++$counter).' Unknown';
	if (!empty($this->_code2text[strToLower($r['lm_key'])]))   b_debug::internalError(sprintf("??? clashing key '%s'",strToLower($r['lm_key'])));
	if (!empty($this->_code2text[strToLower($r['lm_value'])])) b_debug::internalError(sprintf("??? clashing key '%s'",strToLower($r['lm_value'])));
	$this->_code2text[strToLower($r['lm_key'])]   = $r['lm_value'];
	$this->_code2tits[strToLower($r['lm_key'])]   = $r['lm_option'];
	$this->_text2code[strToLower($r['lm_value'])] = $r['lm_key'];

	foreach(unserialize($r['lm_option']) as $k=>$v){
	  if (!empty($this->_code2text[strToLower($k)])) b_debug::internalError(sprintf("??? clashing key '%s'",strToLower($k)));
	  if (!empty($this->_code2text[strToLower($v)])) b_debug::internalError(sprintf("??? clashing key '%s'",strToLower($v)));
	  $this->_code2text[strToLower($k)] = $v;
	  $this->_text2titl[strToLower($v)] = $r['lm_key'];
	  $this->_text2code[strToLower($v)] = $k;
	  $this->title2emp_type[strToLower($v)] = array('t'=>$r['lm_option'],
							'k'=>$r['lm_key']);
	}
      }
    }
  }

  public function title2emp_type($title){
    $this->set_code2text();
    /*
      $this->title2emp_type[strToLower($v)] = array('t'=>$r['lm_option'],
      'k'=>$r['lm_key']);
    */
    b_debug::print_r($this->title2emp_type,__FUNCTION__);
    var_dump($this->title2emp_type,__FUNCTION__);
  }

  public function code2text($code,$MBE = False){
    $this->set_code2text();
    $reply = @$this->_code2text[strToLower($code)];
    if (empty($reply)){
      if ($MBE) $reply = $code; // Might Be Empty
      else      myPear::ERROR("Unknown employee code \"$code\"",$this);
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  public function text2code($text,$createIfMissing=False){
    $this->set_code2text();
    $reply = @$this->_text2code[strToLower($text)];
    if (empty($reply)){
      if ($createIfMissing){
	$reply = create_textKey($text);
	$msg = ", assign new key \"$reply\"";
      }else{
	$msg = "";
      }
      myPear::WARNING("Unknown employee type/title \"$text\"$msg");
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }
  
  function code2titles($code){
    $this->set_code2text();
    $reply = (empty($this->_code2tits[strToLower($code)])
	      ? array()
	      : unserialize($this->_code2tits[strToLower($code)]));
    if (empty($reply)){
      myPear::ERROR("Unknown employee code \"$code\"");
      //b_debug::var_dump($this->getMembers(),"Unknown code \"$code\"");
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  /*
   *
   */
  function title2empType($text,$id='unknown'){
    $this->set_code2text();
    $reply = @$this->_text2titl[strToLower($text)];
    if (empty($reply)){
      $reply = $this->get_defaultEmpType();
      if ($this->isWritable()) myPear::WARNING("Unknown employee title \"$text\", set empType to \"".$this->code2text($reply)."\" for $id");
    }
    $this->dbg(@$reply);
    return $reply;
  }

  /*
   * Get the array of roles array(code=>title)
   */
  private $_types  = Null;
  private $_titles = Null;

  function get_empTypes(){
    $this->set_code2text();
    if ($this->_types === Null){
      $this->_types = array();
      foreach($this->getMembers() as $lm_id=>$r){
	$this->_types[$r['lm_key']] = @$r['lm_value'];
	$this->_titles[$r['lm_key']] = unserialize($r['lm_option']);
      }
    }
    return $this->_types;
  }

  /*
   *
   */
  function get_empTitles($emp_type='000_Directors'){
    
    $reply = array();
    $this->get_empTypes();

    if (!($reply = @$this->_titles[$emp_type])){
      foreach($this->_types as $k=>$v){
	if (($k == $emp_type) || (strToLower($emp_type) == strToLower($v))) $key = $k;
	else continue;
      }
      $reply = (empty($key)
		? array()
		: $this->_titles[$key]); 
    }
    b_debug::xxx($reply);
    return $reply;
  }
  
  /*
   * Header for the employmentType records listing
   */
  protected function header($arg=Null){
    return $this->list_showing;
  }


  /*
   * Formatting for the employmentType record listing
   */
  protected function format(){
    parent::format();
    $nMembers = count($this->get_empByTypes($this->r['lm_key']));
    if (empty($nMembers)) $this->r['n_members'] = b_fmt::redText(x('em','No members'));
    else                  $this->r['n_members'] = "$nMembers members";
    // Fetch the list of emp. titles
    $this->empTitles = unserialize($this->r['lm_option']);
    $this->r['empTitles'] = ' ';
  }
  
  /*
   * Print the emp. titles 
   */
  protected function prt(){
    parent::prt();
    foreach($this->empTitles as $title) $this->t->prt(array('empTitles'=>$title));
  }
  
  /*
   * Get employers by types
   */
  private $empTypes = Null;
  function get_empByTypes($emp_type=Null){
    static $unk = '??';
    
    locateAndInclude('bList_eaEmpRecords');

    // How to see an erroneous avatar record, modal or not modal?
    $modal = True;
    
    // Save in cache the membership
    if($this->empTypes === Null){
      
      $this->empTypes = array();
      $this->get_empTypes();
      
      // Scan all the employers records to check for the departments
      foreach(myOrg($this->parent_ID)->eaMembers()->getMembers_byStatus() as $av_id=>$r){
      	$name = $r['sfl'];
	$url  = b_url::same(array('form' => 'bForm_Avatar_ea',
				  'id'   => $av_id,
				  'mode' =>'RW',
				  'block_once'=>'office_contract'),
			    b_url::get_GET('^(act_|ed_bList)',True));
	
	$EmpRec_class = new bList_eaEmpRecords($av_id);
	$not_defined = True;
	foreach($EmpRec_class->getMembers() as $er_id=>$er){
	  if (!isset($er['e_rate']))  continue;
	  if (in_array($er['e_rate'],array_keys($this->_types))){
	    $this->empTypes[$er['e_rate']][] = $av_id;
	  }else{
	    $this->empTypes[$unk][] = $av_id;
	    $msg = "Unknown emp. type \"".$er['e_rate']."\" for ";
	    b_debug::print_r($er,$msg.$name);
	    b_debug::traceBack($msg.$name);
	    // Modal or not? Some people hate modal windows...
	    if($modal)myPear::WARNING($msg . bJS()->modal_iframe($url," [ $name ]"));
	    else      myPear::WARNING($msg . b_btn::link($name,b_url::same($url)));
	  }
	  $not_defined = False;
	}
	if ($not_defined){
	  $this->empTypes[$unk][] = $av_id;
	  $msg = "Employee Type is not set for ";
	  if($modal)myPear::WARNING($msg . bJS()->modal_iframe($url," [ $name ]"));
	  else      myPear::WARNING($msg . b_btn::link($name,b_url::same($url)));
	}
      }
      ksort($this->empTypes);
    }
    if (empty($emp_type)) $emp_type = $unk;
    if (!@$this->empTypes[$emp_type]) myPear::WARNING_T("No members in \"$emp_type\"");
    return @$this->empTypes[$emp_type];
  }
}
