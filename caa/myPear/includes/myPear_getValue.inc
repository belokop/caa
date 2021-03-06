<?php
/*
 * Define "getValue" mechanism
 */

define('b_rawData','raw');
locateAndInclude('myPear_base');

class myPear_getValue extends myPear_base{

  var $MODE    = 'RO';
  var $ID      = 0;
  var $block_ID= 0;

  public $TABLE           = Null;
  public $TABLEID         = Null;
  public $TABLE_member    = Null;
  public $TABLEID_member  = Null;
  public $TABLEID_member_p= Null;

  public $formDB          = array();
  public $formDB_member   = array();
  public $formDB_modified = array();

  public $r = array();  // current member while making a list of all members

  public $varsModified = array();
  public $vars_noTranslation = array();
  public $vars = array();

  public $show_gv = False;

  static    $getValue_cache_counter = array();
  protected $getValue_recursion = array();
  private   $getValue_cache = array();
  private   $getValue_cache_asIs = array();
  private static $tech_fields = array('_id','_select','timestamp'); // end by
  private static $virt_fields = array('btn','button_','_button','_spacer','_virt');        // start from 
  
  var $isUL         = array(); // the value to be interpreted as <UL>...</UL>
  var $isMBE        = array(); // "Might Be Empty" - list of variables which might be filled later
  var $isPRE        = array(); // pre-formatted fields, to be shown as <pre>field</pre> 
  var $isIcon       = array(); // icon to be shown in the text
  var $isHidden     = array();
  var $isVITAL      = array(); // if any of the isVITALfields are missing, the DB record is NOT created
  var $isVITAL_ifADMIN= array(); // isVITAL for admins, otherwise is "like MBE"  
  var $isMONEY      = array(); // shortcut for formatting money
  var $isFORMAT     = array(); // formatting function to apply
  var $isPRIVATE    = array(); // fields are shown only if the form isWritable 
  var $isINTERNAL   = array(); // fields might be shown only to authenticated users OR if accessed from the local network 
  var $submitOnChange=array(); // list of fields which auto-submit the form, errors to be ignored then
  var $isRW_ifMissing=array(); // fields to stay RW when the whole block is RO 
  var $isRW_forced  = array(); // exceptionally writable field, has precedence over the other flags
  var $isRO         = array('id'=>True); // list of RO fields
  var $isADMIN      = array('av_ssn'=>True);   // Fields shown to the administrator only

  public    $keepRW = array(); // the form does not become RO if this field changes
  protected $keepRW_changed = 0;

  /*
   * The constructor is dummy.
   */
  static $instance_count = array();
  protected $dejaVu_me = 0;
  function __construct($arg1=Null, $arg2=Null, $arg3=Null){

    if ($this->dejaVu_me++) MSG::WARNING("??? dejaVu_me = ".$this->dejaVu_me);
    if (cnf_show_gv) $this->show_gv = True;

    // Instance counter, used for debugging only
    if (empty(self::$instance_count[$id=get_class($this)])) self::$instance_count[$id] = 0;
    self::$instance_count[$id]++;

    /*
     * Set the debug level
     */
    $formdebug = strToLower(b_cnf::get('formDebug','?'));
    if(($formdebug == strToLower(get_class($this))) || 
       ((stripos(get_class($this),$formdebug) !== False) && (get_class($this)!='bForm'))){    
      static::$_debug = True;
    }
  }

  /*
   * A helper
   * [a,b] => [a=>getValue(a),b=>getValue(b)]
   */
  function args2args($fields){
    $reply = array();
    if (!is_array($fields)){
      foreach(func_get_args() as $arg) $f[] = $arg;
      $fields = $f;
    }
    foreach($fields as $f) $reply[$f] = ($f==$this->TABLEID
					 ? $this->ID
					 : $this->getValue($f));
    return $reply;
  }

  /*
   *
   */
  function __toString(){  
    $ids = array($this->ID,$this->MODE);
    if ($t=$this->block_ID) $ids[] = "block=$t";
    return get_class($this) . '('.implode(',',$ids).')';
  }

  /*
   * Is manager here?
   * Rating (from high to low):
   * - superUser
   * - manager
   * - admin
   */
  function isManager(){
    $reply = (method_exists(myPear_access(),'isManager')
	      ? myPear_access()->isManager()
	      : False);
    return $reply;
  }

  /*
   *
   */
  function isAdmin(){
    $reply = $this->isManager();
    return $reply;
  }
  
  protected function myPost($arg=Null){
    b_debug::internalError(__METHOD__.' is not defined');
  }

  /*
   * Get the value of the variable
   *
   * The lookup list order:
   * - $_POST 
   * - $this->formDB_modified
   * - $this->formDB
   */
  private $getValue_MBND = False; // Might Be Not Defined
  private $list_memberID = array();
  protected $datePicker = array();
  public function getValue($name, $asIs=True, $d_MBE=False){ // BIG REVOLUTION ! //  public function getValue($name, $asIs=False, $d_MBE=False){
    
    //
    // ad-hock... Check $_REQUEST for the list/unit member ID
    //
    if(in_array($name,array('l_id','u_id','lm_id','um_id'))){
      $look_here = array('_REQUEST'=> @$_REQUEST[$name],
			 'cache'   => @$this->list_memberID[$name],
			 'cnf::get'=> b_cnf::get($name,'ask_only'));
      if(in_array($name,array('l_id','u_id'))) $look_here['act_id']   = @$_REQUEST['act_id'];
      if(in_array($name,array('lm_id')))       $look_here['memberID'] = @$_REQUEST['memberID'];
      foreach($look_here as $hint=>$value){
	if (empty($value)) continue;
	$this->dbg("get '$value' from '$hint', don't cache in getValue cache",cnf_dev);
	//??? why	$this->getValue_reset($name);
	$this->list_memberID[$name] = $value;
	return $this->getValue_return($hint,$value);
      }      
      
      // Not found...
      if (    ($name == 'lm_id') && (@$_GET['act_code'] == 'add')){
	$this->dbg("Adding new object, no '$name'",cnf_dev);
	$hint = 'adding object';
      }elseif(($name == 'lm_id') && empty($_POST)){
	$this->dbg("No \$_POST, hence no '$name'",cnf_dev);
	$hint = 'empty _POST';
      }elseif($d_MBE === 'isMBE'){
	static $isMBE_case = 0;
	$this->dbg("??? Case #".($isMBE_case)." - not found name '$name', but this was expected",cnf_dev);
      }else{
	$this->dbg($msg="???? not found name '$name'",cnf_dev);
	if (cnf_dev) b_debug::traceBack($msg);
	$hint = '???';
      }
      return $this->getValue_return($hint,Null);
    }      

    bTiming()->cpu(__FUNCTION__);
    
    locateAndInclude('b_checkBox');
    static $no_descr = '???';
    
    // SQL workaround. May be there is a better way to set 'the SQL null'?
    if (@$_POST[$name] === 'NULL') $_POST[$name] = Null;
    
    // Ignore calls from the function "is_visible()"
    if (!$this->valid_field_name($name)){
      bTiming()->cpu();
      return $this->getValue_return($no_descr,Null);
    }
      
    // Use the cache in an aggressive way 
    if (($v=$this->getValue_fromCache($name,$asIs,$d_MBE)) !== Null){
      bTiming()->cpu();
      return $this->getValue_return('cache',$v);
    }

    // well...
    if (@$this->getValue_recursion[$name]++){
      MSG::WARNING_T($msg="Recursion ".$this->getValue_recursion[$name]);
      b_debug::traceBack($msg);
      bTiming()->cpu();
      return Null;
    }

      // Read the database if not yet done 
    $this->getValue_readDefRecord(__FUNCTION__,$name);

    //
    // There is nothing in the cache, search for the value
    //
    $value = Null;
    $hint  = $no_descr;

    if ($this->myPost()){
      if ( isset($_POST[$name])){   
	//      
	// 1. Check $_POST
	//
	$hint = '_POST';
	$value = $_POST[$name];
	
	if (is_string($value) && preg_match('/^[12]\d\d\d-\d\d-\d\d$/',$value)){      
	  // Convert datePicker reply to unix time
	  $hint .= 'datePicker';
	  $value = b_time::txt2unix(min($value,CONST_eternity));
	  $this->datePicker[] = $name;
	}elseif (preg_match('/residentship|citizenship|country/',$name)){
	  // Convert country name to country code
	  $hint .= 'country';
	  if (strlen($value)>2)  $value = b_cc::getCC($value);
	}elseif(strpos($value,'checkBox') !== False){
	  $hint .= $value;
	  $value = join(',',b_checkBox::whatIsOn($name));
	  $this->getValue_reset($name,$value);
	}
	// Save value from the _POST to formDB_modified for the future update
	$this->formDB_modified[$name] = $value;

	//
	// 2. Check $_FILES
	//
      }elseif(isset($_FILES[$name])){
	list($value, $err) = $this->saveFile($name);
	if (!empty($err)){
	  $this->errorsInTheForm[$name] = $err;
	  $value = '';
	}elseif(!empty($value)){
	  // Unset the uploaded file info in the _FILES array,
	  // hence the next call to 'getValue' picks the uploaded file name from the formDB_modified array
	  $this->getValue_reset($name,$value);
	  unset($_FILES[$name]);
	}
      }
    }

    //
    // Nothing in the _POST / _FILES, check the database
    //
    if ($value === Null){
      //
      // 3. Check $formDB_modified
      //      
      if(isset($this->formDB_modified[$name])){ 
	$value = $this->formDB_modified[$name];
	$hint  = 'formDB_modified';
	
	//
	// 4. Check $formDB
	//
      }elseif(isset($this->formDB[$name])){ 
	$value = $this->formDB[$name];
	$hint  = 'formDB';
	
	//
	// 5. Check $formDB_member
	//
      }elseif(!empty($this->TABLE_member) && isset($this->formDB_member[$name])){ 
	// $value = $this->formDB_member[$name];
	$value = Null;
	$hint  = 'formDB_member';
	MSG::WARNING_T("Consider using getMemberValue(\$lmid,$name), and not getMember($name)");
	if (cnf_dev){ @$this->tttt++;	if ($this->tttt < 3){b_debug::traceBack("traceback $name");}}

	//
	// 6. Check the bUnit record being currently formatted
	//
	/*	
      }elseif(!empty($this->TABLE_member) && isset($this->rec[$name])){ 
	$value = $this->rec[$name];
	$hint  = 'rec';
	*/

	//
	// 7. Check the bList record being currently formatted
	//
	/*	
      }elseif(!empty($this->TABLE_member) && isset($this->r[$name])){ 
	$value = $this->r[$name];
	$hint  = 'rec';
	*/
	
	//
	// 8. No luck, the variable is not known
	//
      }else{
	$value = Null;
      }
    }

    //
    // We got the value. Now check how good is it and store in the cache
    $reply = $this->getValue_return($hint,$this->getValue_toCache($name,$value));
    $this->getValue_recursion[$name] = 0;
    bTiming()->cpu();

    return $reply;
  }

  /*
   * Helper
   */
  protected function valid_field_name($name){
    $virtual = (is_numeric($name) ||
		empty($name) || 
		(strpos($name,'_select')!==False) ||
		(strpos($name,'_spacer')===0) || 
		(strpos($name,'_debug') ===0));
    return !$virtual;
  }

  /*
   * Helper function for getValue.
   * Returns the input value, optionally printing it first
   */
  protected function getValue_return($hint,$value,$debug=False,$level=3){
    if ($this->show_gv || $debug){
      $hint = (empty($hint) ? "" : " ($hint)");
      $this->dbg(($value===Null
		  ? '&lt;NOT SET&gt;' 
		  : b_fmt::escape(b_fmt::_($value)) . $hint),
		 array(3,True));
    }
    return $value;
  }

  /*
   * Helper - Read the database if not yet done 
   */
  private function getValue_readDefRecord($trigger1,$trigger2){
    if (($this instanceof bForm) &&  (int)$this->ID && empty($this->formDB[$this->TABLEID])){  
      $this->dbg("?? readDefRecord was not called yet",$this->show_gv);
      $this->readDefRecord();
    }
  }
  
  /*
   * Helper function for "$this->getValue()". 
   */
  public function getValue_reset($name,$value=Null){

    $this->dbg();
    $this->getValue_readDefRecord(__FUNCTION__,$name);
    
    if ($value !== Null){
      $this->getValue_cache[$name] = $value;
      if ($this->myPost()){
	$_REQUEST[$name] = $_POST[$name] = $value;
	if (!($this instanceof bList)) $this->formDB_modified[$name] = $value;
      }
    }else{
      unset($this->getValue_cache[$name]);
      if ($this->myPost()){
	unset($_REQUEST[$name],$_POST[$name]);
	if (!($this instanceof bList)) unset($this->formDB_modified[$name]);
      }
      $this->formDB[$name] = Null;
    }
    $this->dbg($value,$this->show_gv);
  }

  /*
   * Same, but with the enforced DB update
   */
  public function getValue_reset_forced($name,$value=Null){
    if ((int)$this->ID && method_exists($this,'updateDB')){
      $isVITAL = $this->isVITAL;
      $this->isVITAL = array();
      $this->updateDB(array($name=>$value));
      $this->isVITAL = $isVITAL;
    }
    $this->getValue_reset($name,$value);
  }

  
  /*
   * Helper function for "$this->getValue()". 
   * Get value from cache
   */
  protected $defineVariable_counter = 0;
  protected function getValue_fromCache($name,$asIsP,$description_MBE=False){

    // Save the flags
    if(func_num_args() != 3) $description_MBE = $this->getValue_MBND;

    if ($asIsP === 'restore'){
      $asIs = $this->getValue_cache_asIs[$name];
      $description_MBE = True;
    }else{
      $asIs = $this->getValue_cache_asIs[$name] = $asIsP;
    }
    
    // Skip checks for the vaoues which are explicitely declare "might be not defined yet"
    if ($this->getValue_MBND = $description_MBE){
      $this->defineVariable(array($name=>$name),'only_if_not_defined');
    }

    if (isset($this->getValue_cache[$name])){ // The value is in the cache
      @self::$getValue_cache_counter[$this->ID][$name]++;
      switch(b_posix::is_empty($value = $this->getValue_cache[$name])){
      case False: 
	$reply = $value; 
	break;
      case True: 
	$reply = ((bool)$asIs ? $value : b_fmt::redText(missing_value));
      }    
      
    }else{       // The value is not yet in the cache. Sanity check
      
      //      if (False)  // Not(?) needed any more...
      if (!$this->getValue_cancel_error($name,'x','x',array('_')) && !$description_MBE && !isset($this->vars[$name]) && cnf_dev){
	$this->dbg("Undefined variable '$name'",array((bool)($this->show_gv||cnf_dev),5,'maroonText'));
	static $fff = 0;   if (++$fff < 3){ // replace 0 by a counter to see the debug printout
	  b_debug::traceBack();
	  b_debug::print_r($this->vars,$this.'->vars');
	}
      }
      $reply = Null;
    }
    //    if (($asIsP !== 'restore') && ($reply!==Null))  $this->dbg($reply,$this->show_gv);
    return $reply;
  }

  /*
   * Helper function for "$this->getValue()". 
   */
  protected function getValue_toCache($name,$value){
    
    //  Function "getValue_validate_value" is called first, since the value might be set to Null
    //  if it is not valid
    if ($err1 = $this->getValue_validate_value($name,$value)) $this->errorsInTheForm[$name] = $err1;
    
    // Now check the empty value. 
    if ($err2 = $this->getValue_validate_empty($name,$value)) $this->errorsInTheForm[$name] = $err2;
    
    // Clean the error message if everything is ok
    if ($this->getValue_cancel_error($name,$err1,$err2))   unset($this->errorsInTheForm[$name]);
    
    if ($value === Null){
      return Null;
    }else{
      $this->getValue_cache[$name] = $value;
      return $this->getValue_fromCache($name,'restore');
    }
  }

  /*
   * Do not show error for the "technical" & "virtual" fields
   */
  private function getValue_cancel_error($name,$err1='x',$err2='x',$extra_virt=array()){
    $cancel_error = (empty($name) || (empty($err1) && empty($err2)));
    if (!$cancel_error) foreach(self::$tech_fields as $i){ if (strpos($name,$i) == (strlen($name)-strlen($i))){$cancel_error = True; break;}}
    if (!$cancel_error) foreach(self::$virt_fields as $i){ if (strpos($name,$i) === 0)                        {$cancel_error = True; break;}}
    if (!$cancel_error) foreach($extra_virt        as $i){ if (strpos($name,$i) === 0)                        {$cancel_error = True; break;}}
    // $this->dbg($cancel_error,$this->show_gv);
    return $cancel_error;
  }

  /*
   * Return an error message if the value is not valid (i.e. broken mail address, etc.).
   * Set the value to NULL if the data is not valid
   */
  protected function getValue_validate_value($k, &$value){

    $msg = '';
    if (!b_posix::is_empty($value)){
      
      // The vital fields must contain some readable information
      if (@$this->isVITAL[$k]){
	if (!preg_match('/\w/',$value)) $msg = "'$value' is not a valid ".$this->vars[$k];
      }
      
      // Check some obvious objects
      foreach(array('email','phone') as $match){
	if (stripos($k,$match) !== False) switch($match){
	    
	  case 'email':      // The e-mail address should comply with the RFC 2822
	    if ($emails = b_fmt::RFC_2822($value)){
	      if (($vnew = join(',',$emails)) != $value){
		if (strToLower($vnew) != strToLower($value)) MSG::WARNING("E-mail '$value' is not valid, replacing by '$vnew'");
		$value = $vnew;
	      }
	    }else{
	      MSG::WARNING($msg = "'$value' is not a proper email address, ignored.");
	    }
	    break;
	    
	    
	  case 'phone':      // The phone should contain numbers
	    if (!preg_match('/[0-9]/',$value)) $msg = "'$value' is not valid. Value ignored";
	    break;
	  }
      }
    }
    // Drop the variable if it is not valid
    if (!empty($msg)){
      $value = Null;
      $this->dbg($msg,$this->show_gv);
    }
    return $msg;
  }

  /*
   * Check the validity of the empty variable
   */
  private static $vital_types = array('isRO'  => False,
				      'isMBE' => False);
  protected function getValue_validate_empty($k, &$value){
    
    $msg='';
    if (b_posix::is_empty($value)){

      // Seem to be the most reasonable place for this check
      if (@$this->isVITAL[$k])        $this->isMBE[$k] = False;
      if (in_array($k,$this->keepRW)) $this->isMBE[$k] = True;
      
      // Sanity...
      // Set the default value "isRO" & "isMBE" for the technical fields
      static $case = 0;
      foreach(self::$vital_types as $type=>$default_value){
	if (!isset($this->{$type}[$k])){
	  $err_msg = $type.'['.$k.'] is not set for '.$this->__toString();
	  if ($this->getValue_cancel_error($k,'x','x',array('_')) || !myPear_db()->columnExists($k,$this->TABLE))  $this->{$type}[$k] = $default_value;
	  elseif(0 && cnf_dev) b_debug::internalError($err_msg);
	  elseif(!$case++)     b_debug::traceBack($err_msg);
	  else                 MSG::WARNING_T($err_msg);
	}
      }
      
      // All the variables which are NOT declared as "might be empty" should be set
      if (!@$this->isRO[$k] && !@$this->isMBE[$k] && (strpos($k,'_select') === False)){
	if (isset($_POST["${k}_select"])){
	  // "Select menu" answer missing
	  $msg = missing_value;                                 
	}elseif (b_posix::is_empty($value)){ // string '0' is not empty, while PHP insists it is
	  // Text field data missing
	  $msg = missing_value;    
	}
      }
    }
    if (!empty($msg))  $this->dbg($msg,$this->show_gv);
    return $msg;
  }

  /*
   * ******************************************************************************** definitions block
   */

  /*
   *
   */
  protected $mergeDefinitions = array();
  protected function mergeDefinitions($class){
    if (@$this->mergeDefinitions[$class->ID]++) return;
    foreach(array('vars','isMBE','isRO','isVITAL',) as $var){
      $this->dbg($class->__toString()."->$var");
      foreach($class->{$var} as $k=>$v) $this->{$var}[$k] = $v;
    }
  }

  /*
   * Form definitions, to be supplied by the implementation
   */
  function defineVariables() {}

  /*
   *
   */
  function defineVariable($pdescr,$pname='') {

    if (($bForm=$pdescr) instanceof myPear_getValue){
      // Add definitions from another class (e.g. an embedded one)
      foreach($bForm->vars as $name=>$value){
	$this->vars[$name] = $value;
	foreach(self::$vital_types as $type=>$default_value){
	  if (isset($bForm->{$type}[$name])) $this->{$type}[$name] = $bForm->{$type}[$name];
	}
      }
    }else{
      $this->defineVariable_counter++;
      
      if (!empty($pdescr)){
	// compatibility check & rebuild the array - workaround a wrong indexing...
	if (is_array($pdescr)){
	  $description_array = array();
	  $inverted = (is_bool($pname) && $pname) || ($pname === 'only_if_not_defined');
	  foreach ($pdescr as $_descr=>$_name){
	    list($name,$descr) = ($inverted
				  ? array($_descr,$_name)
				  : array($_name,$_descr));
	    if (($pname === 'only_if_not_defined') && isset($this->vars[$name])) continue; 
	    $description_array[$name]  = $descr;
	  }
	}else{
	  $description_array = array($pname=>$pdescr);
	}
	
	// build the column description table
	if (empty($description_array)) $description_array = array();
	foreach ($description_array as $name=>$descr){
	  if (!trim($name) && !trim($descr)) continue;
	  // does not work for bList, where the definition of fields is
	  // dynamic... Need a better idea...
	  // This is just a workaround...
	  $this->vars_noTranslation[$name] = $descr;
	  $this->vars[$name] = (!trim($descr) || (get_class($this)=='bForm')
				? $descr
				: $this->vars[$name] = b_t::_(array('key'=>$name,'item'=>$descr)));
	  // Fill the default values for the vital types
	  foreach(self::$vital_types as $type=>$default_value){
	    if (!isset($this->{$type}[$name])) $this->{$type}[$name] = $default_value;
	  }
	}
      }
      $this->new_vs_empty();
    }
  }

  /*
   * Create on the fly the record requested as 'empty'
   */
  protected function new_vs_empty(){
    if (($this->ID === 'empty') && $this->myPost_bForm()){
      $this->dbg("ID == empty",True);

      // Create a new empty record, set the new ID
      $this->VITAL = array();
      $this->updateDB_createRecord();
      //      $this->_recordCreated();

      // Fill ad-hoc data
      $args = array();
      foreach(array_keys($_POST) as $column){
	if (myPear_db()->columnExists($column,$this->TABLE)) $args[$column] = $this->getValue($column,1,1);
      }
      $this->updateDB($args);
    }
  }

  /*
   * Helper to define the type of variables (isRO, isVITAL, etc.)
   */
  function defineTypes($variables=array()){
    
    foreach($variables as $type => $items){
      foreach($items as $key=>$item){
	list($column,$value) = (is_int($key) 
				? array($item,True)
				: array($key,$item));
	$this->{$type}[$column]  = $value;
	// Sanity...
	if (cnf_dev && !isset($this->vars[$column])){
          if (!$this->getValue_cancel_error($column,'x','x',array('_'))){
	    b_debug::internalError("variable '$column' is not defined yet, but '$type\[$column\]=$value' is assigned"); 
	  }
	}
      }
    }
    $this->new_vs_empty();
  }

  /* 
   * Fetch the variable description
   */
  public function getDescr($name){
    $descr = '';
    if ($this->valid_field_name($name)){
      if (!($descr = @$this->varsModified[$name]))    $descr = @$this->vars[$name];
      if (! $descr)                                   $descr = ((strpos('_id',$name)==strlen($name)-3) ? $name : "unDescribed_$name");
      
      if ($this->MODE == 'RO' || @$this->isRO[$name]) $descr = preg_replace('/-SKIP-.*/',' ',$descr);
      else                                            $descr = str_replace(  '-SKIP-',   ' ',$descr);
      
      if (preg_match('/(_cm_|_cm$)/', $name))         $this->IDV[$name] = 'comma separated list';
    }
    return $descr;
  }

  /*
   * File-specific description 
   */
  public function getDescr_file($name){
    return $this->getDescr($name);
  }

  function name(){ 
    return get_class($this);  
  }

}

