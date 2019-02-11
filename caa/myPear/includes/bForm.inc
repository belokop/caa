<?php
/*
 * Implementation of a persistent form object.
 * The back-end for this class might be either SQL database or a session
 */

locateAndInclude('myPear_getValue');

define('btn_update','update');

class bForm extends myPear_getValue{

  public static function loadWidget($href){
    $_GETsv = $_GET;
    $_GET = b_crypt::parse_query_string($href);
    $f = loader::getInstance_new(str_replace('bForm_bForm_','bForm_','bForm_'.$_GET['form']),$_GET['id'],array('nocache','fatal'));
    $widget = $f->getWidget();
    $_GET = $_GETsv;
    return $widget;
  }


  public $timestamp  = 0;  // the timestamp of the last file checked

  public $vars = array('_post_bForm_ID_once'   => 'ID of the form sent',
		       '_post_bForm_RW_once'   => 'MODE  of the form sent',
		       '_post_bForm_class_once'=> 'CLASS of the form sent',
		       'btnsubmit'       =>'',
		       'btncontinue'     =>'',
		       'MAX_FILE_SIZE'   =>'',
		       );
  
  var $SQL_backend = Null;
  var $mail    = Null;
  var $formName= 'form192';
  var $noNavigation  = False; // do not draw navigation links if true
  var $dontShowErrors= False;
  var $continueAfterErrors = False;
  var $printErrorAsIs  = array();
  var $aceptAsIs_DoNotSave = False;  // usually a form returned by 'search'
  var $leaveCommentUnformatted = False;
  var $everythingIsVital = False;
  var $parentForm   = False;   // flag saying that the form is "embedded" in the parentForm
  var $tips         = array();
  var $tips_byMode  = array();
  var $IDV          = array(); // Input Default Values, those shown in the form 

  var $expected_ext = array(); // expected extensions for the uploaded files (i.e. mime type)

  var $colorCode    = array();
  var $doNotShowChanges= Null;
  var $embedded  = False;
  var $t         = Null;
  var $embeddedRW_enabled = False;
  var $modalRW_enabled    = False;

  protected static $bForm_uploads = Null;
  protected $normalLine = 66;
  protected $bForm_isJustCreated = False;
  protected $arrayToCheck = array();
  public static $_dbg_color = 'dorchidText';

  // List of all RW fields for the current form. Used to decide about printing "update it" button
  private $all_RWfields = array(); 
  
  // Dynamic description. Provisional, the concept is not well defined yet
  protected $formDescription;

  public           $nodrupal       = False;
  public    static $inputToPDF     = False;
  protected static $t_formOpened   = Null;
  protected static $formOpened     = 0;

  public $buttons_external = array();

  static $upload_errors=array(UPLOAD_ERR_INI_SIZE  => array('p'=>'The uploaded file exceeds the upload_max_filesize directive in php.ini',
							    'm'=>'The uploaded file exceeds the maximum size'),
			      UPLOAD_ERR_FORM_SIZE => array('p'=>'The uploaded file exceeds the MAX_FILE_SIZE directive that was specified in the HTML form',
							    'm'=>'The uploaded file exceeds the maximum size'),
			      UPLOAD_ERR_PARTIAL   => 'The uploaded file was only partially uploaded',
			      UPLOAD_ERR_NO_FILE   => 'No file was uploaded',
			      UPLOAD_ERR_NO_TMP_DIR=> 'Missing a temporary folder',
			      UPLOAD_ERR_CANT_WRITE=> 'Failed to write file to disk',
			      UPLOAD_ERR_EXTENSION => 'A PHP extension stopped the file upload');

  /*
   * Basic constructor
   */  
  private static $class_globalCount = 0;
  function __construct($id, $mode='RO', $SQL_backend=True) {

    locateAndInclude('bText');
    static $dejavu = array(), $MAX_FILE_SIZE = 0;

    /*
     * Set the unique class identifier $this->ID
     */
    if ((strpos($id,'=') !== False) || (stripos($id,'REGEXP')!==False)){
      $q = myPear_db()->query($sql="SELECT ".$this->TABLEID." FROM $this->TABLE WHERE $id");
      if (($n=myPear_db()->num_rows($q)) > 1) $this->dbg("<strong>Found $n records</strong><br>$sql",array(True,'redText'));
      while ($r=myPear_db()->next_record($q)) $this->ID = $r[$this->TABLEID];
      if (empty($this->ID)) $this->ID = 'new';
    }else{
      $this->ID = $id;
    }

    
    /*
     * Set the class mode $this->MODE 
     */
    $this->set_MODE($mode); 

    // Maximum size for the class uploads 
    if (!$MAX_FILE_SIZE){
      // save the max file size
      $MAX_FILE_SIZE = b_os::max_upload_size();
      self::$upload_errors[UPLOAD_ERR_FORM_SIZE]['m'] .= " of ".(int)($MAX_FILE_SIZE/1024/1024)."MB"; 
      self::$upload_errors[UPLOAD_ERR_INI_SIZE]['m']  .= " of ".(int)($MAX_FILE_SIZE/1024/1024)."MB"; 
      foreach(self::$upload_errors as $k=>$v) if (!is_array($v)) {
	unset(self::$upload_errors[$k]);
	self::$upload_errors[$k]['p'] = self::$upload_errors[$k]['m'] = $v;
      }
    }

    // Build unique form name, mainly for debug
    $this->formName = join('',array(get_class($this),($this->globalCount = ++self::$class_globalCount)));
    $this->dbg($this->formName);

    if ($this->SQL_backend = $SQL_backend){
      $c = get_class($this);
      if (empty($this->TABLE))   $this->TABLE   = @loader::$sql_identities[$c]['t'];
      if (empty($this->TABLEID)) $this->TABLEID = @loader::$sql_identities[$c]['i'];
      if (empty($this->TABLE)){
	$this->SQL_backend = False;
	if (empty($id)) $this->ID = $id = 'empty';
      }
    }

    // Sanity
    if (!$this->SQL_backend) $this->dbg("_SESSION backend ID=".var_export($this->ID,True));
    
    // Sanity 2
    if (empty($this->ID) || !in_array($this->MODE,array('RW','RO'))) {
      b_debug::internalError(__METHOD__."(ID=$id,mode=$this->MODE) called with empty '$this->TABLEID'");
    }
			  
    // Read the form data from the database
    if ((int)$this->ID)      $this->readDefRecord();
    $this->dbg('done readDefRecord');
    
    // Cancel the "Input Default Values" in the _POST array.
    // How they happen to get there?? Browser problem? Modal problem?
    foreach ($_POST as $k=>$v){
      if ($v === @$this->IDV[$k]) 	unset($_POST[$k]);
    }

    if (@$_GET['mode_once'] == 'RO') $this->isLocked(True);

    // Define variables
    $this->defineVariables();

    // Done...
    $this->isReady(True);
  }

  /*
   * Set the class MODE 
   * Precedence (from top to low)
   *  - _post_bForm_RW_once
   *  - _GET
   *  - input argument
   */
  private function set_MODE($mode){ 

    // First set the mode as declared by the input arguments 
    if ($mode != 'RW') $mode = 'RO';
    $this->MODE = (is_numeric($this->ID) ? $mode : 'RW');
    $this->dbg("----------------------------------------------- ".($mode === $this->MODE ? '' : "'$mode' => '$this->MODE'"));
    
    // Check the _GET    
    if (($m=@$_GET['mode']) && 
	($f=@$_GET['form']) && 
	($i=@$_GET['id'])   && 
	(stripos(get_class($this),$f) !== False) &&
	((int)$i == (int)$this->ID)){
      if ($m != $this->MODE){
	$this->MODE = $m;
	$this->dbg("Set MODE = $this->MODE from _GET",cnf_dev);
      }
    }

    // the mode from the _POST takes precedence over _GET
    if ($this->myPost_bForm() && !empty($_POST['_post_bForm_RW_once'])){
      if ($this->MODE != 'RW')      $this->dbg("Force RW mode by '_post_bForm_RW_once'",cnfdev);
      $this->MODE = $_GET['mode'] = 'RW';
    }
    $this->dbg('MODE = '.$this->MODE);
  }


  /*
   * Clean the instances cache, see loader::getInstance
   */
  function __clean_cache(){}

  /*
   * Read the database record for the class and copy it to $this->formDB
   */
  function readDefRecord(){

    $this->formDB = array();
    if ($this->SQL_backend) {
      $q = myPear_db()->qquery("SELECT * FROM $this->TABLE WHERE $this->TABLEID = '$this->ID'",cnf_dev);
      while ($r=myPear_db()->next_record($q))    $this->formDB = $r; 
    }else{
      if ($s = @$_SESSION['DB_'.get_class($this)]) $this->formDB = $s;
    }
    $this->dbg('done');
  }
  
  /*
   * I do not know a better way to know that "function __construct" is completed
   */
  private  $constructor_completed = False; 
  public function isReady($setReady=Null){
    if(func_num_args() == 1)  $this->constructor_completed = True; 
    return $this->constructor_completed;
  }    

  
  /*
   * ******************************************************************************* Access block
   */
  
  /*
   * Basic access control functions
   */
  function isReadable() {
    $reply = True;
    return $reply;
  }
  
  function isOwner() {
    return b_cnf::check_conditions(array(True => array('creating' => ((int)$this->ID == 0)),
					 'default' => False),
				   $this);
  }
  
  function formBlock_isWritable($block_ID){ 
    $reply = $this->isWritable_default();
    $this->dbg($reply,cnf_show_w);
    return $reply;
  }
  
  private $isWritable_bForm = Null;
  function isWritable() { 
    if (!(int)$this->ID)  return True;
    if ($this->isWritable_bForm === Null) 
      $this->isWritable_bForm = (bool)b_cnf::check_conditions(array(True=>array('block'=>(!empty($this->block_ID) && $this->formBlock_isWritable($this->block_ID))),
								    'default' => $this->isWritable_default()),
							      $this);
    return $this->isWritable_bForm;
  }
  
  /*
   * 'isLocked' & 'modalRW_enabled' have precedence over superUser_here
   */
  private $isWritable_default = Null;
  protected function isWritable_default() { 
    if ($this->isWritable_default === Null) 
      $this->isWritable_default = (bool)b_cnf::check_conditions(array(False=>array('lock'      =>  $this->isLocked(),
										   'inModal'   => (cnf_inside_fb && !$this->modalRW_enabled),
										   'DBnotReady'=> !$this->isReady(),
										   'inPreview' => (@$_REQUEST['preview_once'] == $this->ID),
										 //'isEmbedded'=> (( $this->isEmbedded() && !$this->embeddedRW_enabled) && 
										 //		   (!$this->parentForm->errorless() || $this->vitalFieldMissing())),
										   ),
								      True => array('INSTALL'  => !defined('superUser_here'),
										    'superuser'=> (defined('superUser_here') && superUser_here),
										    'empty'    => ($this->ID == 'empty'),
										    'new'      => !(int)$this->ID,
										    'in memory'=> !$this->SQL_backend,
										    'owner'    =>  $this->isOwner(),
										    'creating.'=>  $this->bForm_isJustCreated,
										    'isManager'=> $this->isManager()),
								      'default' => False),
								$this);
    return $this->isWritable_default;
  }
  
  /*
   * keep the parent attributes for embedded forms
   */
  function isEmbedded($parentForm=Null){
    if ($parentForm instanceof bForm)  $this->parentForm = $parentForm;
    if (($this->parentForm instanceof bForm) && !$this->embedded){
      $this->embedded = True;
      $this->parentForm->dbg(b_fmt::escape($this->__toString()));
      if (is_object(self::$t_formOpened))  $this->t = self::$t_formOpened; // ZZZ
      foreach ($this->isMBE as $k=>$v) $this->parentForm->isMBE[$k] = $v;
      $this->parentForm->defineVariable($this->vars,True);
      $this->MODE = 'RO';
      $this->dbg("set MODE = ".$this->MODE,cnf_dev);
    }
    $this->dbg($this->embedded);
    return $this->embedded;
  }

  /*
   * Helper function, adds "onchange" and/or "onclick" attribute
   */
  private function submit_onchange($name,$type='change',$class=''){
    if (!empty($this->submitOnChange[$name])){
      if ($type != 'change') $r[] = "onclick='submit()'";
      $r[] = "onchange='submit()'";
      $r[] = "class='form-submit $class'";
      $reply = join(' ',$r);
    }else{
      $reply = (empty($class) ? "" : "class='$class'");
    }
    return $reply;
  }

  /*
   * Get the bit-mask value
   */
  protected function getBitMask($bitName,$field){
    $reply = b_mask::get($this->getValue($field,True,True),$bitName);
    return $reply;
  }

  /*
   * Set the bit value
   * @return 
   * - True  if the value is changed
   * - False if not
   */
  protected function setBitMask($bitName,$field,$value,$checkOnly=False){
    if ((int)$this->ID){
      $isChanged = ((bool)$this->getBitMask($bitName,$field) !== (bool)$value);
      if ($isChanged && !$checkOnly){
	$old =  $this->getValue($field,True,True);
	$new =  b_mask::set($old, $bitName, $value);
	$this->updateDB(array($field=>$new),$forced=True);
      }
      $this->dbg("isChanged ? ".($isChanged ? 'YES' : 'NO'),(bool)cnf_show_pv);
    }else{
      $isChanged = False;
    }
    return $isChanged;
  }
  
  /*
   *
   */
  private function style($class,$class2='',$extra=''){
    if (self::$inputToPDF) return '';
    if (!empty($class2)) $class2 = ' '.$class2;
    switch ($class){
    case 'data':   return "class='b_form_data$class2' $extra";
    case 'desc':   return "class='b_form_desc$class2' $extra";
    case 'space':  return "class='b_form_space$class2' $extra";
    case 'transp': return "class='b_form_transp$class2' $extra";
    }
  }

  /*
   * ************************************************************************************ bForm invocation block
   */

  protected function myPost($arg=Null){
    return $this->myPost_bForm($arg);
  }
  
  protected $closeForm_continue = 'continue with changes';
  /*
   * Check that the destination of the $_POST array is the current bForm class
   */
  private $reply_myP = Null;
  function myPost_bForm($id=Null){


    // Reset the cache if requested
    // This is done from "updateDB()" to set the record ID to the ID of the newly created database record
    if ($id !== Null){
      $this->reply_myP == Null;
      $_POST['_post_bForm_ID_once'] = $id;
      $this->dbg("Reset ID --> $id",True);
    }
    
    // Get the cached reply
    if  ($this->reply_myP === Null){
      $this->reply_myP = ((isset($_POST['_post_bForm_ID_once']) && (@$_POST['_post_bForm_class_once']===get_class($this))) &&
			  (( (int)$this->ID && ($_POST['_post_bForm_ID_once'] == $this->ID)) ||
			   (!(int)$this->ID && ($_POST['_post_bForm_ID_once']==='new' || 
						$_POST['_post_bForm_ID_once']==='empty'))));
      $color = (cnf_dev || (static::$_dbg_color == 'blackText') ? 'oliveText' : static::$_dbg_color);
      $this->dbg($this->reply_myP,array(3,$this,$color,(bool)(cnf_dev && $this->reply_myP)));
    }
    return $this->reply_myP;
  }

  /*
   * The "canonical" form invocation
   */
  function getForm(){
    $this->getFormHeader();
    $this->getFormOpen();    
    $this->getFormBody();
    $this->getFormFooter();
  }

  /*
   * Open the form
   */
  public function getFormOpen(){
    locateAndInclude('b_table');
    bTiming()->cpu(get_class($this).'.doForm');

    // The serialized fields are never writable during the bForm dialog,
    // those are updated with the bList editor
    foreach ($this->isUL as $k=>$v){
      if ($v) $this->isRO[$k] = True;
    }
    
    // Check the _POST & _FILES arrays, update the database if there is new data 
    $this->new_vs_empty();
    $this->formOK();

    if (!$this->isWritable() && ($this->MODE == 'RW')){    // Sanity
      $this->dbg('set mode=RO, the form is not writable',cnf_dev);
      $this->MODE = 'RO';
    }

    // Reset the name counter 
    if (!@$this->isEmbedded())      self::nameCounter('resetCounter');

    if (self::$inputToPDF) {
      $this->dbg('inputToPDF');
      if (!($this->t instanceof b_table))         $this->t = self::$t_formOpened;
    }else{
      $this->getFormOpen_execute();
    }
  }
    
  /* 
   * Default form header is empty
   */
  protected function getFormHeader(){}

  /*
   * Empty default form body
   */
  protected function getFormBody(){}

  /*
   * Default footer is the button "update"
   */
  function getFormFooter(){
    $this->closeForm(btn_update); 
  }

  /*
   * Close the form
   */
  private $buttons_line = array();
  public $buttons_line_embedded_in_table = True;
  function closeForm($submit_button=btn_update,$extraButtons=array()){
    
    bTiming()->cpu();

    //
    // Close the form here for PDF, the "submit buttons" are not shown
    //
    if (self::$inputToPDF){
      $this->closeForm_execute();
      return;
    }
    
    if (empty(bForm::$formOpened)){ 
      $this->dbg('ZZZ return since not formOpened');
      $this->closeForm_execute(False);
      return;
    }

    $this->_debug_showID(True);

    if ($this->embedded && !$this->embeddedRW_enabled) {
      $this->closeForm_execute(False);
      return;
    }
    
    //
    // Print the "submit buttons" as a row at the bottom of the form
    //
    if (empty($extraButtons)) $extraButtons = array();
    if (is_object($this->t)){
      
      // Print the buttons only if it makes sense
      $only_one_RW_field_here = False;
      $this->all_RWfields = array_unique($this->all_RWfields);
      $this->dbg("all_RWfields = ".join(', ',$this->all_RWfields),cnf_dev);
      if (count($this->all_RWfields)==1){
	foreach($this->all_RWfields as $item){
	  $only_one_RW_field_here = !empty($this->submitOnChange[$item]);
	  $toPrint = !$only_one_RW_field_here;
	}
      }else{
	$toPrint = True;
      }
      $this->dbg("toPrint=".var_export($toPrint,True));
      
      // Build the row of "submit" buttons
      $buttons = array();
      if ($this->isWritable()){
	if     (empty($submit_button))  $submit_button = array();
	if (!is_array($submit_button))  $submit_button = array($submit_button);
	foreach(array_values($submit_button) as $button_text){
	  if (empty(bForm::$formOpened)) b_debug::internalError('form is not opened');
	  if ($this->MODE == 'RO'){
	    $button_text = preg_replace('/submit|update|continue/',btn_update,$button_text);
	    $this->textHidden('_post_bForm_RW_once','RW');
	  }else{
	    $button_text = preg_replace('/submit|update|continue/',$this->closeForm_continue,$button_text);
	  }
	  //	 	 MSG::DEBUG(b_fmt::escape($button_text,'url'),cnf_dev);
	  $buttons[] = ($this->buttons_line_embedded_in_table
			? b_btn::submit($button_text)
			: b_btn::big_button_grey($button_text));
	}
      }
      
      //
      // Print button "cancel" as:
      //  - "close this window" for FancyBox
      //  - "cancel" for the normal view
      //
      if (cnf_inside_fb) {
	if (!$this->modalRW_enabled) $buttons = $extraButtons = array();
	unset($extraButtons['cancel']);
	unset($extraButtons['continue']);
	
	// fancybox_ajax does not have the "javascript:parent.jQuery.fancybox" object
	if (cnf_inside_fb_iframe) $buttons[] = bJS()->close_modalWindow();

      }elseif($only_one_RW_field_here){
	$toPrint = True;	
	$extraButtons = $buttons = array();
	$this->cancelButton($extraButtons);
      }elseif (!empty($buttons) && ($this->MODE == 'RW')){
	$this->cancelButton($extraButtons);
      }
      
      // Merge the remaining buttons
      foreach ($extraButtons as $name=>$href) {
	if (!empty($href)) $buttons[] = ($this->buttons_line_embedded_in_table
					 ? b_btn::submit($name,$href)
					 : b_btn::big_button_grey($name,$href));
      }
      
      if ($toPrint){
	// Remove an useless "update" button if there are "edit block" icons
	$ok_to_drop = (self::$editableBlocks_counter && (count($buttons) == 1) && preg_match('/value.*update/i',$buttons[0]));
	if ($ok_to_drop && 
	    (self::$editableBlocks_counter == 1) &&
	    (self::$ROskipedBlocks_counter > 0)) $ok_to_drop = False;
	if ($ok_to_drop) 	  $buttons = array();

	$this->buttons_line = $this->closeForm_buttons($buttons);
	if ($this->buttons_line_embedded_in_table){
	  $this->t->highlightRows = False;
	  $this->t->tro();
	  $this->t->td(array(''));
	  $this->t->td(x("div style='margin:".(cnf_inside_fb?30:2)."px;text-align:center'",
			 b_btn::UL_buttons($this->buttons_line)),
		       'colspan=5');
	  $this->t->trc();
	  $this->buttons_line = array();
	}
      }
    }

    // print closing attributes    
    $this->closeForm_execute($closeTable=True,$closeForm=True);

    // Show the eventual user errors in the form
    $this->displayErrors();
  }

  /*
   * Helper - "cancel" button
   */
  protected function cancelButton(&$extraButtons,$text='cancel'){

    // Pre-compute arguments for the later use
    $_once = '';
    foreach(array('arg1_once',
		  'act_parent',
		  ) as $args)      if ($_once=@$_GET[$args]) break;
    if ($_once) $_once = "arg1_once=${_once}&arg2_once=".@$_GET['arg2_once'];
    
    // Those arguments must NOT present on the page invoked by "cancel"
    $drop = b_url::get_GET('^(act_.*|lm_.*)$',True);

    // Create the "cancel" according to the context
    if (b_cnf::get('form') && (int)$this->ID){
      $reply=$this->sameURL_but("?id=$this->ID&mode=RO&form=".get_class($this));  
    }elseif($l=b_cnf::get('ed_bList')){
      $reply = $this->sameURL_but("?$_once&resetcache_once=1&ed_bList=$l",array('id','form','act_code'));
    }else{
      $reply = $this->sameURL_but("?$_once",$drop);
    }
    if (!empty($reply)) $extraButtons[$text] = $reply;
  }

  /*
   * Helper function
   */
  protected function closeForm_buttons($buttons=array()){
    return array_merge($buttons,$this->buttons_external);
  }

  /*
   *
   */
  public $bForm_action_custom = '';
  protected function bForm_action($myGet=array(),$drop=array()){
 
    if (empty($this->bForm_action_custom)){
      $reply = b_url::same($myGet);
    }else{
      $reply = $this->bForm_action_custom;
    }

    $repack = (($_after_create=b_cnf::get('_after_create'))
	       ? array('_after_create'=>$_after_create)
	       : array());
    if (cnf_inside_fb) $repack['modal_once'] = bJS()->modal_once();
    $reply = b_url::repack($reply,$repack,$drop);
    return $reply;
  }

  /*
   * Helper to getFormOpen.
   * Print "<form>", also print _post_bForm_ID_once if needed
   */
  private function getFormOpen_execute($comment='',$width='width100'){
    static $case = 0;    $case++;

    $hidden = (($this->MODE == 'RW')
	       ? array('_post_bForm_ID_once'   => $this->ID,
		       '_post_bForm_class_once'=> get_class($this))
	       : array());
    
    $this->dbg(bForm::$formOpened. " $case");
    switch (empty(bForm::$formOpened)){

    case False:
      /*
       * <form> was already printed
       */
      if ($this->MODE == 'RW'){
	switch((int)bForm::$formOpened){
	case 0:
	  //
	  // The form was already "pre-open", set the real form data
	  bForm::$formOpened = $this->ID;
	  static $dejaVu = 0;
	  $this->dbg("ZZZ form was pre-opened $case, set ID=".$this->ID);
	  print "<!-- ------------------------- (Re)open form(".bForm::$formOpened.") as RW $comment ------------------------ -->\n"; 
	  foreach($hidden as $k=>$v) $this->textHidden($k,$v);
	  break;

	default:
	  $this->forceROmode();
	}
      }
      break;

    case True:
      /*
       * <form> was not yet printed, print it now
       */
      switch($this->MODE){
      case 'RO':
	bForm::$formOpened    = 'preliminary';
	b_t::$dropAfterSKIP = True;
	break;

      case 'RW':
	bForm::$formOpened    = $this->ID;
	b_t::$dropAfterSKIP = False;
	break;
      }

      
      //
      // Open <form> for editing
      $this->dbg('ZZZ &lt;form ...&gt; $case, '.bForm::$formOpened);
      
      $myGet = array('id'   =>( $this->embedded 
				? $this->parentForm->ID  
				: $this->ID),
		     'form' =>( $this->embedded 
				? get_class($this->parentForm)
				: get_class($this)),
		     'mode' =>(($this->embedded && !$this->embeddedRW_enabled) 
			       ? 'RO' 
			       : 'RW'),
		     'resetcache_once'=>1);
      foreach(array('modal_once') as $keep){
	if (!b_posix::is_empty($g=@$_GET[$keep])) $myGet[$keep] = $g;
      }

      printf("\n<form action='%s' method='post' enctype='multipart/form-data' name='%s'>\n  <!-- form(%s) %s -->\n",
	     $this->bForm_action($myGet, array('updatedb')),$this->formName,bForm::$formOpened,$this->formName);
      foreach($hidden as $k=>$v) $this->textHidden($k,$v);

      //
      // start b_table if not yet done (should not be !!!)
      if (self::$t_formOpened instanceof b_table){
	$this->t = self::$t_formOpened;
      }else{
	$this->dbg('ZZZ open table '.$case);
	$this->t = self::$t_formOpened = new b_table ("class='b_form $width'","<!-- $comment form table $this->formName -->");
      }
      $this->dbg('ZZZ $this->t = '.$this->t);
      $this->t->highlightRows = False;
    }
  }

  /*
   * Helper to "closeForm"
   */
  function forceROmode(){
    if (bForm::$formOpened != $this->ID){
      // This should never happen (two or more bForms have MODE == 'RW'), 
      // may be the MODE of the "secondary bForm's" must then be set to RO ??
      $this->MODE = 'RO'; 	  //  to be tested, but look OK 
      MSG::WARNING_T($msg="??? form is already open with ID=".bForm::$formOpened.", set MODE=RO",$this);
    }else{
      $this->dbg("MODE = ".$this->MODE);
    }
  }
  
  /*
   * Helper to "closeForm"
   */
  protected function closeForm_execute($closeTable=True,$closeForm=False){

    if ($closeForm || !$this->isEmbedded()){
      if ($closeTable){
        $this->dbg('ZZZ closeTable');
        if (is_object($this->t)) $this->t->close();
        $this->t = self::$t_formOpened = Null;

	bIcons()->hide_explainIcons = True;
	bIcons()->explain();
      }

      if (!empty(bForm::$formOpened)){
        $this->dbg('ZZZ closeForm '.bForm::$formOpened);
	// Send current "lm_id" if available
	if (!empty($this->member_ID)) $this->textHidden('memberID',$this->member_ID);

	// Send the hiden fields, close the form
	foreach(array_keys(self::$textHidden_fields) as $field) print $field; 

	// Print the "buttons line" AFTER closing the table and BEFORE closing the form...
	if ($this->buttons_line) b_btn::buttons_line($this->buttons_line);
	$this->buttons_line = array();

        print "</form> <!-- end $this->formName -->\n";

	// Clean the buffers
	// b_debug::print_r(self::$fData_fields,'fData_fields');
	self::$textHidden_fields = array();
	self::$fData_fields  = array();
	bForm::$formOpened   = Null;
      }
    }
  }


  /*
   * "Receive" the POST array during the dialog, 
   * check it for the completeness, 
   * write results to the database and optionally print the errors
   *
   * This function is called right BEFORE processing the form, hence ONLY in the interactive context.
   */
  protected $formOK = False;
  protected function formOK(){
    $this->dbg(starting);
    if ($this->myPost_bForm()){
      
      // Perform the specific quality checks
      $this->formOKspecific();
      
      // Save the data collected so far to the database, UNLESS a vital field is missing
      if ($reply = !$this->vitalFieldMissing()){
	
	$this->updateDB($this->formDB_modified);    
	
	//
	// Finally set RO mode if possible
	$r1 = !empty($this->keepRW_changed); 
	$r2 = (@$_POST['_post_bForm_RW_once'] === 'RW');
	$r3 = (!$this->errorless() && $this->isOwner());
	$stay_RW = $r1 || $r2 || $r3;
	if (cnf_dev) $this->dbg(sprintf("stay_RW = %s (keepRW_changed='%s', _post_bForm_RW_once='%s', errors+owner='%s')",
					var_export($stay_RW,True),var_export($r1,True),var_export($r2,True),var_export($r3,True)),True);
	if ($r3 && ($this->MODE !== 'RW')){
	  $this->dbg("reinforce RW mode for the owner");
	  $this->MODE = 'RW';
	}

	if(!$stay_RW){
	  $_GET['mode'] = $this->MODE = 'RO';
	  $this->dbg('set MODE = RO',cnf_dev);
	  
	  //
	  // Optionally go to the "Search results" to complete the dialog
	  // (the search proposes to "create" a new avatar if it can't be found in the database)
	  //
	  if ($_after_create=b_cnf::get('_after_create')){
	    b_cnf::_unset('_after_create');
	    $this->dbg(exiting);
	    b_url::header(b_url::same(array('function'         =>'bForm_Avatar_Search::show_selection',
					    'firstname_search_once' => $this->getValue('av_firstname',True),
					    'lastname_search_once'  => $this->getValue('av_lastname',True),
					    'email_search'     => $this->getValue('av_email',True),
					    'av_id_once'       => $this->ID,
					    'form_once'        => get_class($this),
					    '_button'          => @$_GET['_button'],
					    '_after_search'    => str_replace('AV_ID',$this->ID,$_after_create)),
				      'drop_all'));
	  }	  
	}
      }
    }else{
      $reply = !$this->vitalFieldMissing();
    }
    $this->formOK = $reply;
    $this->dbg($reply,cnf_dev);
    $this->dbg(exiting);
    return $reply;
  }
  
  /*
   * Helper to formOK
   */
  function formOKspecific() {}

  /*
   * Show the errors in the form
   */  
  protected $displayErrors_heading = 'The form is not complete';
  private function displayErrors($no_filter=False){
    
    if (!b_btn::$confirm_waiting){

      // optionally clean and/or reformat the error messages
      $this->dbg(starting);
      $this->dbg((($n=count($this->errorsInTheForm))==0 ? 'No' : $n).' errors BEFORE displayErrors_preflight'.($n?":<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;":'').
		 b_fmt::joinX('<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;',$this->errorsInTheForm));
      $errors = $this->displayErrors_preflight();
      $this->dbg((($n=count($errors))==0 ? 'No' : $n).' errors AFTER displayErrors_preflight');
      $this->dbg(exiting);

      // Sanity
      $this->clean_outOfScopeMissingValues($errors);
      
      // Ouu, shit... This happens for the "embedded" forms.
      // getvalue checks only for myPost arrays, so it does not see other forms...  
      // workaround...
      foreach ($errors as $key=>$err){
	$faulty_message = '';
	if (($err == missing_value) && !empty($this->isMBE[$key])) 
	  $faulty_message = "'$key' isMBE, but declared 'error, missing value'";
	elseif (($err == missing_value) && ($v=$this->getValue($key,True,True)))
	  $faulty_message = "$key='$v', but declared 'error, missing value'";
	if (!empty($faulty_message)){	
	  unset($errors[$key]);
	  $this->dbg($faulty_message,cnf_dev);
	  //	  if (cnf_dev) b_debug::traceBack($faulty_message);
	}
      }

      // Display them as an error message
      $display = array();
      foreach ($errors as $key=>$err){
	if (is_int($key)){
	  $display[] = $err;
	}else{
	  $var = x('em',x('"',(b_posix::is_empty($v=$this->vars[$key]) 
			       ? x('strong',$key) 
			       : $v)));
	  $display[] = str_replace('-BR-','<br/>',preg_replace('/-SKIP-.*/','"',"$err for $var"));
	}
      }
      if (!empty($display)){
	MSG::MESSAGE(x('ul',b_fmt::joinMap('li',$display)),  // same as MSG::ERROR, almost...
			(cnf_dev ? $this : Null),
			'error',
			$this->displayErrors_heading);
	if (cnf_dev) b_debug::print_r($this->errorsInTheForm,$this.'->errorsInTheForm');
      }
    }
  }

  /*
   * Helper to 'displayErrors'.
   *
   * Removes "technical" error messages in order not to confuse the user
   */
  protected function displayErrors_preflight(){

    if (!(int)$this->ID && $this->SQL_backend)$errors = array();
    elseif (!$this->isWritable())             $errors = array();
    elseif (self::$inputToPDF)                $errors = array();
    else                                      $errors = $this->errorsInTheForm;

    foreach($errors as $k=>$v){
      if (empty($v) || (!is_int($k) && !isset($this->vars[$k]))){
	$this->dbg("Cancel errors message '$v' for non-defined variable '$k'");
	unset($errors[$k]);
      }
    }
    return (empty($errors) ? array() : $errors);
  }

  /**
   * Get a summary (normally public information only)
   * from the form
   */ 
  function getInfo(){
    $this->getForm(); 
  }

  /*
   * bForm widget is the $this->getForm() output, which is usually used in modal window(s)
   *
   * @param return HTML widget code 
   */
  function getWidget($header=''){
    static $saved_icons;

    if ($header === 'none')$header = '';
    elseif(empty($header)) $header = x('h3',$this->name());
    else                   $header = x('h3',$header);

    bIcons()->toggleExplainIcons(False);
    $saved_icons = bIcons()->reset();

    ob_start();         // -----------Start catching the output
    print "<div class='centralized'> <!-- ".$this->__toString()." start widget -->\n$header";
    $this->getFormOpen_execute('opened by '.__METHOD__,'width100');
    $this->getFormWidget();
    if (is_object($this->t)) $this->t->close();
    print "</div> <!-- ".$this->__toString()." end widget -->\n";
    $widget = ob_get_contents();
    ob_end_clean();     // -----------End catching the output

    bIcons()->toggleExplainIcons();
    bIcons()->reset($saved_icons);

    unset($this->t);
    return $widget;
  }
  
  protected function getFormWidget(){
    $this->getForm();
  }

  
  private $_islocked = False;   // Forced lock 
  function isLocked($lock=Null)  {
    if ($lock !== Null) $this->_islocked = $lock;
    return $this->_islocked;
  }

  /*
   *  Call method if it is defined
   */
  public function _call_bForm_method($method,$args=array(),$fatal=False){
    if (empty($method)){
      $reply = True;
    }else{
      // Check that this not a negating request
      if ($negate = (strpos($method,'!') !== False)){
	$method = trim(str_replace('!','',$method));
      }

      // Execute
      if(method_exists($this, $method)) {
	if (!is_array($args)) $args = array($args);
	$reply = call_user_func_array(array($this, $method), $args);
	if ($negate) $reply = !$reply;
      }else{
	$msg = sprintf('The required method "%s->%s" does not exist', get_class($this), $method);
	if ($fatal) throw new Exception($msg);
	else        MSG::ERROR($msg);
	$reply = False;
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  function declareRO($condition=True,$exceptions=array()){ // optionally set the whole record RO
    if ($condition) {
      foreach ($this->vars as $k=>$v)
	if (!@$exceptions[$k]) if (!preg_match("/^[0-9]*$/",$k)) $this->isRO[$k] = $this->isMBE[$k] = True; 
    }
  }
  
  /*
   * Return array of years starting "<now> - $from", ending "<now> + $to"
   */
  private function list_of_years($from,$to,$include_notConsidered=False){
    $reply = array();
    if ($include_notConsidered) $reply[CONST_year_not_considered] = 'Not considered';
    //    for ($y=date('Y',time())-$from; $y<=date('Y',time())+$to; $y++) $reply[$y] = $y.($y>date('Y',time()) ? ' (expected)' : ''); 
    for ($y=$from; $y<=$to; $y++) $reply[$y] = $y.($y>date('Y',time()) ? ' (expected)' : ''); 
    return $reply;
  }

  /*
   * Check the formDB and set the status field
   */
  public function check_formDB($setStatus=True){
    if (!$this->myPost_bForm()){
      foreach($this->formDB as $k=>$v){
	if (myPear_db()->columnExists($k,$this->TABLE)) $this->getValue($k,True,True);
      }
    }
    $this->vitalFieldMissing();
    if ($setStatus)  $this->setStatus();
  }

  /*
   * Set the status after checking the formDB.
   * Usually, is  the form os ok or not.
   * The default method is empty
   */
  public function setStatus($what=Null){}

  /*
   *
   */
  protected function selectMenuFromStatus($name,$validStatuses){
    $select = array();
    foreach ($validStatuses as $k=>$descr){
      if ($this->MODE == 'RO' && $this->getValue($name)==$k) @$this->isIcon[$name] = $k;
      $select[$k] = b_t::_(array('key'=>$name.'_'.$k,'item'=>$descr['d']));
    }
    return $select;
  }

  function selectMenu_bList($name,$bList,$getMembers_where=array(),$column='lm_key'){
    $options = array();
    $bList->getMembers_where = $getMembers_where;
    foreach ($bList->getMembers() as $id=>$r){
      $options[$r['lm_id']] = $r[$column]; // b_t::_($r[$column],True,$r['lm_locale']);
    }
    asort($options);
    $this->getFormRow($name, $this->selectMenu_exec($name,$options));
  }

  /*
   */
  function selectMenu($name, $values, $extras='', $onlyRW=False) {
    $this->getFormRow($name, $this->selectMenu_exec($name,$values,$extras,$onlyRW));
  }

  /*
   */
  public function selectMenu_exec($name,$values=array(),$extras='',$onlyRW=False) {
    if (empty($values)) $values = array();
    $selected = $this->getValue($name,True);
    if (!$selected) $selected = @$values['default'];
    //
    // Cast to avoid problem with indeces like '333,444'                                        ********************
    // Apparently var_dump(333=='333,444') IS TRUE !!!!                                         ********************
    // i.e. cast before comparison is done in a strange way - (int)'333,444', and not (string)111 ******************
    //
    if (!b_posix::is_empty($selected) && is_numeric($selected)) $selected = (int)$selected;

    $select_what = (empty($values['*select_name*'])
		    ? ''
		    : $values['*select_name*'].' ');
    unset($values['*select_name*'],$values['*default*']);
    if (!$this->isRO($name,$onlyRW)){
      $s = array();
      if (!empty($values)){
	if (!$selected)                     $s[] = x("option value='NULL' selected","<-- select ${select_what}-->");
	elseif (empty($values[$selected]))  $s[] = x("option value='$selected' selected",$selected); // strictly speaking this is not correct...
	foreach ($values as $k=>$v){ 
	  $v = strip_tags($v);
	  if(strpos($k,'separator')!==False)$s[] = x("option value='' disabled='disabled' class='select_separator'",'');
	  elseif($k != 'default')           $s[] = x("option value='$k'".($selected===$k?" selected='selected'":''), $v);
	}
      }
      if (preg_match("/class='(.*)'/",$extras,$m)){
	$class = trim($m[1]);
	$extras = trim(str_replace("class='$class'","",$extras));
      }else{
	$class = 'shadow-box form-submit'; // Firefox gets crasy...
      }
      // The "spinning wait page" is very annoying in the pop-up selection. 
      // Let's try to invent 30 seconds delay before the spiing starts.
      // May be better to cancel it it all...
      $reply = preg_replace('/(form-submit|page-load-progress)/','\\1'.'-30',x("select name='$name' $extras ".$this->submit_onchange($name,'change',$class),join("\n",$s)));
      //      if (cnf_dev && strpos($reply,'-30')) MSG::WARNING_T("'$name' is (form-submit|page-load-progress)",$this);
    }elseif($selected){
      $reply = @$values[$selected] . $this->textHidden($name,$selected);
    }else{ 
      $reply = $this->getValue($name);
    }
    return $reply;
  }
  
  /*
   * ******************************************************************* uploads block
   *
   * Every myPear module uploads files (if it is required) into the directory <myPear_root_files>/<module_name>
   */

  /**
   * Initialize the uploading area for the bForm class.
   */
  public $uploadsURL = Null;
  public $uploadsDir = Null;
  function uploadInit(){
    if (empty($this->uploadsURL)){
      $this->dbg(starting);
      $this->uploadsURL = $this->uploadInit_path();
      $this->dbg("uploadsURL=".$this->uploadsURL);
      
      $this->uploadsDir = b_os::realpath($this->uploadsURL);
      $this->dbg("uploadsDir=".$this->uploadsDir);
      $this->dbg(exiting);
    }
  }
  
  /*
   * The uploads area for the module is <root> / <module> / <org> / ...
   */
  function uploadInit_path($module_path=array(),$module=Null){
    // Set the module, default is the current module
    if ($module === Null){
      b_reg::_();
      $module = b_reg::$current_module;      
    }
    
    $path = '';
    foreach(array_merge(array(myPear_root_files,
			      $module,
			      sprintf("%06d",abs(myOrg_ID))),
			$module_path) as $element){
      $path .= $element . DIRECTORY_SEPARATOR;
      if (!is_dir($path) && !is_dir(getcwd().$path)){
	// Create the uploads area and protect it with an empty index.html file
	$this->dbg("creating $path",cnf_dev);
	b_os::mkdir($path);
	$this->uploadsDir_protect($path);
      }
    }
    $this->dbg($path,cnf_dev);
    return $path;
  }

  /*
   * do not let browsing of the documents directory
   */
  protected function uploadsDir_protect($dir=array()){
    if (is_string($dir)) $dir = array($dir);
    if (empty($dir))     $dir = array(bForm::$bForm_uploads, $this->uploadsDir);
    foreach($dir as $d) {
      foreach(array('html') as $ext){
	if (file_exists($if="$d/index.$ext")) $s = file_get_contents($if); else $s = '';
	if ($s != b_emptyHTML) {
	  $this->dbg("s='$s' replacing $if");
	  file_put_contents($if, b_emptyHTML);
	}
      }
    }
  }


  /*
   * *********************************************************************************** updateDB block
   */


  /*
   * Export data to the database
   */
  private $updateDB_recursion = 0;
  function updateDB($data=array()){
    static $ignoreFields = array('button_x','button_y','MAX_FILE_SIZE'), $call_counter = 0;

    $this->dbg();
    if ($this->updateDB_recursion++){
      static $caseN = 0;
      $this->dbg($m=sprintf("Recursion depth %s, case %s",$this->updateDB_recursion,(++$caseN)),cnf_dev);
      if (cnf_dev) b_debug::traceBack($m);
      return $this->ID;
    }
    
    if (empty($data)) $data = $this->formDB_modified;
    if (empty($data) ||
	$this->aceptAsIs_DoNotSave ||    /* search mode - the form should not be saved */
       !$this->updateDB_preflight($data)){
      $this->updateDB_recursion--;
      return; 
    }
    
    //    $this->dbg(starting);
    bTiming()->cpu(get_class($this).'.'.__FUNCTION__);
    if (static::$_debug) b_debug::print_r($data,$this.'[updateDB] input:');

    $pars = array();
    foreach ($data as $k=>$v) {
      if ($k == $this->TABLEID ||
	  in_array($k,$ignoreFields) ||
	  preg_match("/(^[0-9]*|_select)$/", $k)){
	$this->dbg("IGNORE input value $k='$v'");
	continue;
      }
      if (is_array($v)) b_debug::internalError("array not expected: ".var_export($v,True));
      if (is_string($v)) $v = trim($v); 
      if (stripos($k,'mail')!==False) $v = mb_strToLower($v);
      if ($this->SQL_backend){
	$oldValue = @$this->formDB[$k];
	if (!b_posix::is_equal($v,$oldValue)){
	  if (myPear_db()->columnExists($k,$this->TABLE)) {
	    if (empty($v) && (strToUpper(myPear_db()->getColumnType($this->TABLE,$k,'Null')) === 'YES')) $v = Null;
	    $this->getValue_reset($k,$v); // reset the cache for this variable
	    $pars[] = myPear_db()->quote($v,True,$k);
	    if (in_array($k,$this->keepRW)) $this->keepRW_changed++;
	  }else{
	    if (stripos($k,'_post_')===False){
	      $this->dbg("Unknown database column '$this->TABLE.$k' = '$v'");
	    }
	  }
	}else{
	  $this->dbg("x IGNORE input value $k='$v' - same as in DB");
	}
      }
      $this->formDB[$k] = $this->formDB_modified[$k] = $v; 
    }
    
    if ($this->SQL_backend){
      if (!empty($pars)) {
	if ((int)$this->ID){
	  myPear_db()->qquery("UPDATE $this->TABLE SET ".join(', ',$pars)." WHERE $this->TABLEID='$this->ID'",cnf_show_db);
	  $this->readDefRecord();
	  $this->_recordUpdated();
	}else{
	  $this->updateDB_createRecord($pars);
	  $this->readDefRecord();
	  $this->_recordCreated();
	}
      }else{
	$this->dbg('the record is up-to-date');
      }
    }else{ // session mode, no SQL backend
      $this->formDB_modified[$this->TABLEID] = $this->ID = 1;
      $_SESSION['DB_'.get_class($this)]     = $this->formDB_modified;
    }
    $this->updateDB_recursion = 0;
    bTiming()->cpu();
    //    $this->dbg(exiting);
    return $this->ID;
  }

  /*
   * Create a new bForm record
   */
  function updateDB_createRecord($pars=array()){
    $this->dbg("Creating ".$this->__toString(),cnf_show_db);
    $this->bForm_isJustCreated = True;
    $this->formDB[$this->TABLEID] = $this->formDB_modified[$this->TABLEID] = $this->ID = bMaster()->reserve_masterCounter();
    // ???? Not sure that this is correct and/or needed...
    if (!isset($_GET['id'])) $_GET['id'] = $this->ID;

    $pars[] = myPear_db()->quote($this->ID,True,$this->TABLEID);
    
    // Reset the "my POST" if it was not this a new record
    if ($this->myPost_bForm()) $this->myPost_bForm($this->ID);
    
    myPear_db()->qquery("INSERT INTO $this->TABLE SET ".join(', ',$pars),cnf_dev);
    $this->dbg('New record is created',True);
  }
  
  protected function _recordUpdated() {}
  protected function _recordCreated() {}

  /*
   * final check for the missing vital data
   */
  function updateDB_preflight(&$updates=array()){

    // $this->defineVariables();

    // Sanity
    foreach($updates as $name=>$value){
      $msg = $this->getValue_validate_value($name,$value);
      if (!empty($msg)){
	$this->dbg("Cancel non-valid \"$name\" = ".$updates[$name],True);
	MSG::ERROR('"'.$updates[$name].'" is not a valid value for the '.x('em',$this->vars[$name]));
	unset($updates[$name]);
      }
    }

    $vf = $this->vitalFieldMissing($updates);
    foreach($vf as $k=>$v){
      if (!myPear_db()->columnExists($k,$this->TABLE)) unset($vf[$k]);
    }

    if (b_posix::is_empty($f=join(', ',array_values($vf)))){
      return True;
    }else{
      MSG::WARNING_T("'$f' vital fields missing, abandon ".__FUNCTION__,$this);
      return False;
    }
  }

  /*
   * Return True if the form is ok
   */
  private $inside_errorless = 0;
  function errorless(){
    if ($this->inside_errorless++){
      if (cnf_dev) b_debug::traceBack("recursion $this->inside_errorless");
      $reply = 'recursion in '.__method__;
    }else{
      $this->check_formDB(False);
      $this->clean_outOfScopeMissingValues();
      $reply = empty($this->errorsInTheForm);
    }
    $this->inside_errorless--;
    $this->dbg(var_export($reply,True),cnf_dev);
    return (bool)$reply;
  }

  /*
   * Cancel errors for the values which are NOT in the dialog
   */
  protected function clean_outOfScopeMissingValues(&$errors_list=Null){
    b_debug::xxx();
    // Check that the missing values were asked, skip if not
    if (!empty($_POST) && !$this->myPost_bForm()) return;
    
    if (!empty($errors_list)) $args['errors_list'] = $errors_list;
    $args['$this->errorsInTheForm'] = $this->errorsInTheForm;
    
    foreach($args as $type=>$arg){
      foreach($arg as $field=>$msg){
	//	if ($msg != missing_value) continue;
	if (!in_array($field,array_keys($_POST))){
	  if (cnf_dev) MSG::DEBUG(sprintf(" - Cancel error '%s' for %s[%s]",$msg,$type,$field),$this);
	  if ($type == 'errorsInTheForm') unset($this->errorsInTheForm[$field]);
	  else                            unset($errors_list[$field]);
	}
      }
    }
  }

  /*
   * Loop thru the vital field, check that there no one missing.
   * @params return (array) missing fields 
   */
  protected function vitalFieldMissing($updates=array()){

    // In principal the values should be in the cache already, but check again - just in case (superstition)...
    if ($this->myPost_bForm()){
      foreach($_POST  as $name=>$x)            $v=$this->getValue($name,True,True);
      foreach($_FILES as $name=>$x)            $v=$this->getValue($name,True,True);
    }
    foreach(array_keys($this->isVITAL) as $vf) $v=$this->getValue($vf,True,9);
    // Take into account the coming updates. Cancel errors if the updates are coming
    foreach($updates as $name=>$value){
      $err1 = $this->getValue_validate_value($name,$value);
      $err2 = $this->getValue_validate_empty($name,$value);
      if (empty($err1) && empty($err2))  unset($this->errorsInTheForm[$name]);
      //      b_debug::var_dump($this->errorsInTheForm,"$name--$value err1/err2 $err1/$err2");
    }
    
    // Check if the form was auto-submitted, cancel errors if yes  - the form will anyway stay in the RW mode
    foreach($this->submitOnChange as $field=>$status){
      if (@$this->formDB[$field] !== $this->getValue($field,True)) $this->errorsInTheForm = array();
    }

    // Sanity
    $this->clean_outOfScopeMissingValues();

    $reply = array();
    foreach ($this->isVITAL as $vf=>$isVITAL){
      if ((bool)$isVITAL && !b_posix::is_empty(@$this->errorsInTheForm[$vf])){
	$reply[] = $vf;
      }
    }
    
    // Sanity
    if (!empty($reply))        $this->dbg(($reply ? join(', ',$reply) : False),cnf_dev);
    if($this->errorsInTheForm) $this->dbg("errorsInTheForm: ".b_fmt::joinX(',',$this->errorsInTheForm),cnf_dev);
    //    if($this->errorsInTheForm) {
    //      b_debug::print_r($this->isMBE);
    //      b_debug::traceBack("errorsInTheForm: ".b_fmt::joinX(',',$this->errorsInTheForm));
    //    }
    $this->dbg((bool)!empty($reply));
    return $reply;
  }
  
  /*
   * Checkbox group
   */
  function checkBox($name,$show_disabled_checkbox=False,$img_on='i-ok-small',$img_off='i-no-small') {
    $this->t->tro();
    $this->fSpacer();
    $this->fDescr($name);
    $this->fStar($name); 

    $c = ($checkBoxIsOn = $this->checkBoxIsOn($name) ? "checked='checked'" : '');

    if ($show_disabled_checkbox){
      // in RO mode show the value as disabled checkbox, while 
      // in RW mode show it as a normal click-able checkbox
      $r = ($this->isRO($name) ? 'disabled': '');
      if (empty($r)) $this->all_RWfields[] = $name;
      $this->fData("<input type='checkbox' name='${name}' $c $r border=0 ".$this->submit_onchange($name,'')."/>".
		   "<input type='hidden'   name='${name}_cb_expected' value='off'/>\n",
		   $name);
    }else{
      // in the RO mode show the value as YES/NO icon (in RW as a normal checkbox)
      if ($this->isRO($name)){
	$this->fData(bIcons()->get($checkBoxIsOn ? $img_on : $img_off),$name);
      }else{
	$this->all_RWfields[] = $name;
	$this->fData("<input type='checkbox' name='${name}' $c border=0 ".$this->submit_onchange($name,'')."/>".
		     "<input type='hidden'   name='${name}_cb_expected' value='off'/>\n",
		     $name);
      }
    }
    $this->t->trc();
  }

  public function checkBoxIsOn($name){
    $reply = self::_checkBoxIsOn($name,array($name =>$this->getValue($name,True,True)));
    return $reply;
  }

  public static function _checkBoxIsOn($name,$formDB=array()){
    $reply = !((bool)b_checkBox::isOff(@$formDB[$name]));
    return $reply;
  }

  /*
   * Array of check-boxes
   */
  function checkBoxes($name,$boxes=array(),$separator='<br/>'){
    locateAndInclude('b_checkBox');

    $values = explode(',',$this->getValue($name,True));
    
    $this->t->tro();
    $this->fSpacer();
    $this->fDescr($name);
    $this->fStar($name); 
    if ($this->MODE == 'RW'){
      b_checkBox::set_prefix($name);
      
      foreach($boxes as $k=>$v){
	$cb[$k] = $v;
	b_checkBox::set($k,in_array($k,$values) ? b_checkBox::$ON[0] : b_checkBox::$OFF[0]);
      }

      if (@$this->submitOnChange[$name]) $this->fData(b_checkBox::create($cb,$separator),$name);
      else                               $this->fData(b_checkBox::create($cb,"\n",''),$name);
    }else{
      $items = array();
      foreach($values as $k) if (!empty($boxes[$k])) $items[] = $boxes[$k];
      $this->fData(join($separator,$items),$name);
    }
    $this->t->trc();
  }

  /*
   * Radio buttons
   */
  function radioButton($name,$values,$defaultValue=Null,$translate=True){

    $separ = "";
    $radio = array();
    if (!$this->isRO($name)){
      $currentValue = (string)$this->getValue($name,True);
      if ($currentValue == '' && !is_null($defaultValue)) $currentValue = $defaultValue;
      $associative = b_fmt::isArrayAssociative($values);
      foreach ($values as $k=>$v) {
	if (!$associative) $k = $v;
	$c = (($currentValue==$v) || ($currentValue==$k) ? "checked='checked'" : ''); 
	if ($translate){
	  $textKey = (preg_match('/^(yes|no|bonus)$/i',trim($v))) ? '_RADIO_'.$v : $name.'_'.$v;
	  $vv = b_t::_(array('key'=>$textKey,'item'=>$v));
	}else{
	  $vv = $v;
	}
	$onChange = $this->submit_onchange($name,'');
	$radio[] = "<input type='radio' name='$name' $onChange $c value='$k'/>$vv&nbsp;";
	if (!preg_match("/^(F|M|yes|no|bonus|[0-9])$/i",$v)) $separ = "<br/>\n";
      }
    }else{
      $radio[] = $this->getValue($name);
    }
    $this->getFormRow($name, join($separ,$radio));
  }
  
  function textArea($name,$rows=3,$cols=40) {
    $value = $this->getValue($name,True);
    $rows = max(3,$rows,(int)(strlen($value) / $cols));
    $this->getFormRow($name, ($this->isRO($name)
			      ? str_replace("\n","<br/>",preg_replace('/([\.\,])/',"\\1 ", $value))
			      //: "<textarea name='$name' rows='$rows' cols='$cols'>$value</textarea>"));
			      : "<textarea name='$name'>$value</textarea>"));
  }
  
  function commentField($text){
    if ($this->MODE == 'RW'){
      $this->t->tro();
      $this->fSpacer();
      $this->t->td($text,$this->style("data",'','colspan=3'));
      $this->t->trc();
    }
  }

  /*
   * Let the form to printed many times, we need it for the modal objects
   *
   * Apparently if "static $fieldCounter" is inside the function, then the scope of variables is not that I would expect....
   * Looks like the multiple instances COPY the in-function static variables. Strange...
   */
  private static $calllCounter = 0;
  private static $fieldCounter = array();
  public static function nameCounter($name='resetCounter',$nameToReset=''){
    if (cnf_inside_fb || (strpos($name,'_spacer') === 0)) return 0;

    self::$calllCounter++;
    if ($name === 'resetCounter'){
      if (empty($nameToReset)) foreach(self::$fieldCounter as $k=>$v) self::$fieldCounter[$k] = 0;
      else                     self::$fieldCounter[$nameToReset] = 0;
      $reply = 0;
    }elseif ($nameToReset == 'getCounter'){
      $reply = (int)@self::$fieldCounter[$name];
    }else{
      $reply = @self::$fieldCounter[$name]++;
    }
    return $reply;
  }
  
  protected function formSignature($text){
    switch ((bool)self::$inputToPDF){
    case True:
      return ''; // ZZZ
      $spaces = array('','');
      $text = x("div style='text-align:right'",x('strong',x('em','<br/><br/>'.nbsp($text))));
      break;
    case False:
      $spaces = array('','');
    }
    $this->t->tro();
    $this->fSpacer();
    $this->t->td(array('',''),$this->style('transp'));
    $this->t->td($text,"class='bold-text italic-text align_right'");
    $this->t->trc();
  }

  /*
   * Print the dialog line
   * @param $name - field name
   * @param $data - field value
   * @param (array)$extraTD - optional values to be printed on the right hand side of the line 
   */
  protected function getFormRow($name, $data, $extraTD=array()) {
    static $n_spacers = 0;
    
    if (!$this->isRO($name)) $this->all_RWfields[] = $name;

    // Sanity check
    if (!($this->t instanceof b_table)) {
      static $dejaVu = 0;
      if (cnf_dev && !$dejaVu++){
	b_debug::var_dump($this->t,"'t' is not b_table as it is expected to be");
	b_debug::traceBack("'t' is not b_table as it is expected to be");
      }
      return;
    }

    if ($this->_visible($name,$data) && !self::nameCounter($name)){
      if ($this->isRO($name)){
	if (preg_match("/^https?:/",$data))$data = bJS()->modal_iframe($data,$data); // $data = "<a href='$data'>$data</a>";
	elseif(  @$this->isPRE[$name])   $data = "<pre>$data</pre>";
	elseif(! @$this->isUL[$name])    $data = b_fmt::longString($data,$this->normalLine);
      }
      if (strpos($name,'_spacer')===0){
	if (!$n_spacers++) $this->t->tr(' '); // no 2 sequential spacers 
      }else{
	$n_spacers = 0;
	$this->t->tro();
	$this->fSpacer();
	$this->fDescr($name);
	$this->fStar($name); 
	$this->fData($data,$name);
	$this->t->td($extraTD);
	$this->t->trc();
      }
    }
  }
  
  /*
   * Color-picker based on html5
   */
  function colorPickerField($name,$extraTD=array()){
    $color_v = $this->getValue($name,True);
    $this->getFormRow($name, 
		      x("div id='html5DIV'",
			"<input name=$name id='html5colorpicker' type='color' style='width:85%;' value='$color_v' onchange='clickColor(0, -1, -1, 5)'>"),
		      $extraTD);
  }
  
  /*
   * Color-picker based on Farbtastic from Steven Wittens.
   */
  private $farbtastic_open = 0;
  function colorPickerField_old($name,$extraTD=array()){
    if (!$this->farbtastic_open++)      bJS()->farbtastic();
    $color_v = $this->getValue($name,True);
    $color_n = $name . '_color';
    $this->getFormRow($name, 
		      //		      x('div class="form-item"',
		      "<input type='text' id='$color_n' name='$color_n' class='farbtastic-colorwell' value='$color_v' />",
		      $extraTD);
  }
  
  /*
   *
   */
  function datePickerField($name,$min_max=array()){
    $value = $this->getValue($name,True);
    if ($value != CONST_year_not_considered){
      locateAndInclude('b_datePicker.inc');
      $fmt='medium';
      if ($this->isRO($name)){
	$this->getFormRow($name, $this->year2date($name,$fmt));
      }else{
	$this->all_RWfields[] = $name;
	$this->t->tro();
	$this->fSpacer();
	$this->fDescr($name);
	$this->fStar($name); 
	$this->t->tdo($this->style("data"));
	bDatePicker()->get($name,$value,$min_max);
	$this->t->tdc();
	$this->t->trc();
      }
    }
  }

  /*
   * Enquire the date range, "start" & "end" of an event.
   * The method guarantees that the "start" <= "end"
   */
  function datePickerField2($name='date_range',$from,$to,$icon_from=Null,$icon_to=Null){
    static $fmt = 'true_medium';

    $vfrom = $this->getValue($from,1);
    $vto   = $this->getValue($to  ,1);
    if (empty($vfrom)) $this->formDB[$from] = $vfrom = time();

    // Set the minimal period to 1 day
    $this->formDB[$to] = $vto = date('Y-m-d',max(b_time::txt2unix($vto) , b_time::txt2unix($vfrom)+24*3600));
    locateAndInclude('b_datePicker.inc');
    
    if ($this->isRO($from)){
      $this->formDB[$name] = ($this->year2date($from,$fmt).' -> ' .
			      $this->year2date($to,  $fmt));
      $this->getFormRow($name,$this->formDB[$name]);
    }else{
      $this->formDB[$name] = b_time::period($vfrom,$vto);
      $this->textHidden($name,$name);
      $this->t->tro();
      $this->fSpacer();
      $this->fDescr($name);
      $this->fStar($name); 
      $this->t->tdo($this->style('data'));
      if (!empty($icon_from)){
	bDatePicker()->icon1 = bIcons()->get($icon_from);
	bDatePicker()->icon2 = bIcons()->get($icon_to);
      }
      new b_table_simple(bDatePicker()->get2($from,$vfrom,$to,$vto,True),'','style="margin:0;"');
      $this->t->tdc();
      $this->t->trc();
    }
    $this->dbg("$vfrom -> $vto");
  }

  /*
   * Add calendar to be shown on the right hand side
   */
  function showCalendar($start,$end,$see_day9=False){
    locateAndInclude('bIcal_mini');
    $this->getFormOpen_execute('opened by '.__METHOD__);

    $cal = new bIcal_mini(); 
    $cal->setPeriod($this->getValue($start,0), $this->getValue($end,0));
    $cal->see_day9 = $see_day9;
    
    ob_start();
    $cal->prt();
    $this->t->pendingObject[] = b_fmt::compact_html(ob_get_contents());
    ob_end_clean();
  }
  
  /*
   * Prompt for the date / year
   */
  function year2date($column_name,$mode='unix_time',$date='ask'){
    static $to      = +5;  // in 5 years from now...

    if ($date === 'ask') $date = $this->getValue($column_name,True,True);

    switch($mode){    
    case 'unix_time':  // return the true unix time (or 'not considered')  -----------------------------------------------------
      $reply = b_time::txt2unix($date);
      break;

    case 'dialog':     // return the method to be used in the dialog -----------------------------------------------------
    case 'dialog2':
      
      // See the age of the avatar
      foreach(array('av_ddate','av_birthdate') as $d){
	$v = $this->getValue($d,True,True);
	$$d = ($valid[$d] = b_time::is_valid($v))
	  ? b_time::txt2unix($v)
	  : False;
      }
      if (!function_exists('max_date')){      
	function max_date($av_birthdate,$av_ddate,$n){
	  return (empty($av_ddate)
		  ? time() + $n*365*24*3600
		  : $av_ddate);
	}
      }

      // set description and the year limits
      $include_notConsidered = False;
      // switch($column_name){
      if (stripos($column_name,'ms_year') !== False){
	// case 'av_ms_year':
	if ($this->getValue($column_name,True) == CONST_year_not_considered) unset($date);
	$descr = ($this->MODE == 'RW' ? 'Date of award' : ' ');  
	$include_notConsidered = True;
	list($from,$to) = ($av_birthdate // MS at the age of 15 years 
			   ? array($av_birthdate  + 15*365*24*3600, max_date($av_birthdate,$av_ddate,+5))
			   : array(time() - 15*365*24*3600,         max_date($av_birthdate,$av_ddate,+5)));
      }elseif (stripos($column_name,'phd_year') !== False){
	// case 'av_phd_year':
	if ($this->getValue($column_name,True) == CONST_year_not_considered) unset($date);
 	$descr = ($this->MODE == 'RW' ? 'Date of award' : ' ');  
	$include_notConsidered = True;
	list($from,$to) = ($av_birthdate // PhD at the age of 20 years 
			   ? array($av_birthdate + 20*365*24*3600, max_date($av_birthdate,$av_ddate,+5))
			   : array(time() - 20*365*24*3600,        max_date($av_birthdate,$av_ddate,+5)));
      }elseif(preg_match('/ddate|birth/i',$column_name)){
      	$descr = (stripos($column_name,'birth') !== False
		  ? 'Birth date'
		  : 'Death date');
	list($from,$to) = array(time()-100*365*24*3600,time());  // from 0 to <max_age> years old....
      }
      if (!isset($from)) \D8::traceBack();
      list($from,$to) = array(date('Y',$from),date('Y',$to));
      
      // should we ask for the year or for the date? 
      $dialog = array();
      switch(b_time::is_valid(@$date)){
      case -2:
	// ask for the year
	$dialog = array('selectMenu',$this->list_of_years($from,$to,$include_notConsidered));
	$descr = trim(str_replace(array('YYYY-MM-DD','date','Date'),array('','year','Year'),$descr));
      case -1;
	break;
      default:
        $dialog = array('datePickerField');
      }
      if (!empty($descr)) $this->defineVariable($descr,$column_name);
      
      // if (cnf_dev) b_debug::xxx("av_birthdate='$av_birthdate' from='$from' to='$to' descr='$descr' add_NC='".var_export($include_notConsidered,True)."'");
      $reply = ($mode == 'dialog2'
		? array($column_name => $dialog)
		: $dialog);
      break;

    default:     // return formatted date  ----------------------------------------------------------------------------------------------
      switch(b_time::is_valid($date)){
      case -2: // empty
	$reply = '';
	break;
      case -1: // not considered 
	$reply = (stripos($mode,'true')===False
		  ? 'Not considered'
		  : CONST_year_not_considered);
	break;
      case  0: // not a valid time (presumably YYYY)
	$reply = $date;
	break;
      case  2:
	$date = b_time::txt2unix($date);
      case  1: // presumably unix time
	if (in_array($column_name,array('av_ms_year','av_phd_year')) && ($date > CONST_near_future) && (stripos($mode,'true')===False)){
	  // format the unix time (say "expected" for phd/bs)
	  $reply = date('Y',$date).' (expected)';
	}elseif (False && in_array($column_name,array('av_birthdate')) && (b_time::is_valid($dday=$this->getValue('av_ddate',True,True)) > 0)){
	  $this->isRO[$column_name] = True;
	  $reply = b_time::_date('medium',$date) .' - '.b_time::_date('medium',$dday);
	  $this->vars[$column_name] = '&nbsp;';
	}else{
	  $mode = str_replace('true_','',$mode);
	  switch($mode){
	  case 'short':
	  case 'medium':
	  case 'long':
	  case 'full':
	    $reply = b_time::_date($mode, $date);
	    break;
	    
	  default:
	    $reply = date($mode,$date);
	  }
	}
	break;
      }
    }

    $this->dbg($reply);
    return $reply;
  }


  /*
   * As textField, but a country named is assumed. 
   * The country names are auto-completed in order to avios miss-spelling
   */
  function textFieldCountry($name){
    $country = b_cc::getName($this->getValue($name,True,True));
    if ($this->MODE == 'RW'){
      b_cc::autocomplete($name);
      $country = "<input id='$name' name='$name' size='40' value='$country' maxlength='99' type='text'/> <!-- textFieldCountry -->";
    }
    $this->getFormRow($name, $country);
  }

  /*
   * A text field, i.e.
   * <input name='$name' ... type='text'/>";
   */
  function textField($name, $extraTD=array(),$text=Null,$colspan=0) { // sick! $text=NULL
    $this->getFormRow($name, $this->textFieldExec($name,$text), $extraTD);
    if (@$this->isRO[$name] && ($this->MODE == 'RW')) $this->textHidden($name);
  }

  private function textFieldExec($name, $text=Null) { // sick! NULL
    if ($this->_visible($name,$text)){
      $value     = $this->getValue($name,False,True);
      $valueAsIS = $origin = $this->getValue($name,True,True);

      // Replace av_id by the name/ID
      if ((int)$valueAsIS && preg_match('/_avid$/',$name) && ($av=new bForm_Avatar($valueAsIS))){
	if ($this->isRO($name))  $value = $valueAsIS = $av->fmtName('Lf');
	else                     $value = $valueAsIS;
      }
      $size      =  min(100,max(40,strlen($valueAsIS)));;
      $maxlength = 25000;
      if ($this->isRO($name)){
	$reply = (empty($text) ? $value : $text);
	return $reply;
      }else{
	if (empty($valueAsIS) && !empty($this->IDV[$name])) {
	  $valueAsIS = $this->IDV[$name];
	  $c = "class='input-def-value'";
	}else{
	  $c = '';
	}
	$quote = (strpos($valueAsIS,"'") === False) ? "'" : '"';
	$reply = "<input $c name='$name' size='$size' value=${quote}${valueAsIS}${quote} maxlength='$maxlength' type='text'/>";
	return $reply;
      }
    }
  }

  /*
   * Show a hidden value
   */  
  private static $textHidden_fields = array();
  function textHidden($name,$value=Null,$printNow=True) {
    if (bForm::$formOpened === $this->ID){
      $show_it = ($value === 'show_it');
      if ($show_it || is_null($value))  $value = $this->getValue($name,True,True);
      self::$fData_fields[$name][] = $value;
      if ($show_it && cnf_dev){
	$this->isRO[$name] = True;
	$this->vars[$name] = $name;
	$this->getFormRow($name, $value);
      }
      $reply = "<input type='hidden' name='$name' value='".strip_tags($value)."'/>\n";
      if ($printNow){
	// The array must be static, otherwise fields from other classes will be lost :-/
	@self::$textHidden_fields[$reply]++;
      }else{
	return $reply;
      }
    }
  }
  
  /*
   * Return the filename (without the path) if the file is uploaded and readable
   */
  function isUploaded($name,$htmlFormatted=False) {
    $reply = $this->uploadedFile_path($name,True);
    if ($this->timestamp <= 0){
      if ($htmlFormatted) $reply = $this->getValue($name);
      else                $reply = False;
    } 
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Check that the file type is valid.
   * For the pdf file the check is direct (by looking into the file content),
   * the other types are checks only by the extension name.
   */
  public  $valid_fileType_error = '';
  function valid_fileType($name,$extensions,$display_name=''){

    if (is_file($name)){
      $file_path = $name;
      $file_name = basename($file_path);
    }else{
      $file_path = @$_FILES[$name]['tmp_name'];
      $file_name = @$_FILES[$name]['name'];
    }
    $display_name = "<em>$display_name \"$file_name\"</em>";

    $reply = False;
    if (empty($extensions))  $extensions = '.pdf';
    if (!is_array($extensions)) $extensions  = array($extensions);
    $this->valid_fileType_error = "The file $display_name is not of a proper type (".join(',',$extensions).')';

    foreach($extensions as $ext){
      switch(strToLower($ext)){
      case  'pdf':
      case '.pdf':
	if (b_pdf::isPDF($file_path,$display_name)){
	  $this->valid_fileType_error = '';
	  $reply = True;
	}else{
	  $this->valid_fileType_error = b_pdf::$isPDF_error;
	}
	break;
	
      case 'image':
	$reply = $this->isImage($file_path);
	break;

      default:
	if (empty($ext) || (stripos($file_name,$ext) !== False)){
	  $this->valid_fileType_error = '';
	  $reply = True;
	}
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Check that the file is uploaded and get the file timestamp
   *
   * @return file absolute path
   * $this->timestamp is set to:
   *      = 'file modification time' if the file exists
   *      =  -1 if the file exists, but not of the required MIME type
   *      =   0  if the file does not exist
   */
  function uploadedFile_path($name,$onlyIfExists=False){

    $this->uploadInit();
    
    $file_path  = b_os::join(array($this->uploadsDir,$this->uploadedFile_name($name)));

    $error_reporting = error_reporting(0); 
    $this->timestamp = filemtime($file_path);
    error_reporting($error_reporting);
    
    if (empty($this->timestamp))      $this->timestamp = 0;
    if ($onlyIfExists && ($this->timestamp <= 0))      $file_path = False;
    $this->dbg($file_path,cnf_dev);
    return $file_path;
  }
  
  /*
   * same as uploadedFile_path, but instead of the absolute file path it returns the URL
   */
  function uploadedFile_url($name,$onlyIfExists=False){
    $path = $this->uploadedFile_path($name);
    $url  = $this->uploadsURL . DIRECTORY_SEPARATOR . basename($path);
    if ($onlyIfExists && !(file_exists($url) && is_file($url))) $url = False;
    $this->dbg($url);
    return $url;
  }
  
  /*
   * Construct a file name to be uploaded
   * The default file name is always lower case
   */
  function uploadedFile_name($name,$args=array(),$ext=True){
    if (b_posix::is_empty($fn = $this->getValue($name))){
      $this->dbg($this->expected_ext,cnf_dev);
      $fn = strToLower(join('_',array_merge(array($name),$args)) . ($ext ? $this->expected_ext[$name] : ''));
    }
    $this->dbg($fn,cnf_dev);
    return $fn;
  }

  /*
   * Uploading dialog
   */
  function uploadFile($name){
    
    $this->dbg(starting);
    $this->uploadInit();

    // Upload the file and acknowledge the upload
    $file = $this->getValue($name,True);
    if ((int)$this->ID && !empty($file) && (@$this->formDB[$name] !== $file)){
      $this->updateDB(array($name=>$file));
      $this->uploadFile_acknowledge($name);
    }

    // show "input default value" if defined
    $this->IDV[$name] = "Select PDF file";  
    $c = (empty($file) && !empty($this->IDV[$name]) 
	  ? "class='input-def-value' value='".$this->IDV[$name]."'"
	  : '');
    $data = $input = 
      "<input $c name='$name' type='file' />";
    
    if ($url = $this->uploadedFile_url($name,True)){
      // The file was already uploaded
      if (self::$inputToPDF){ // text file
	$data = x('em','('.b_t::_('included').')');
      }else{  // web dialog
	$data = $seeIt = x("a class='only_online' href='$url'",'[ Click to see it ]');
	if ($this->isRO($name) || !$this->isWritable()){
	  $data = $seeIt;
	}elseif ($this->MODE == 'RW'){
	  $this->varsModified[$name] = $this->vars[$name].'<br/>'.$seeIt;
	  $data = x('em',b_t::_('You can replace it by uploading a new '.$this->expected_ext[$name].' file')) .'<br/>'. $input;
	  $data = $input;
	}
      }
    }elseif(self::$inputToPDF){
      // Not uploaded, just report it if in PDF mode
      $data = x('em',x('(',b_t::_('missing')));
    }elseif($this->MODE == 'RW'){
      // Not yet uploaded, prompt for it
      $data = $input;
      //$this->varsModified[$name] = $this->vars[$name].'<br/><em>Please select '.@$this->expected_ext[$name].' file</em>';
      $this->varsModified[$name] = $this->vars[$name];
    }else{
      $data = $this->getValue($name);
    }

    // Print the form row
    $this->dbg("url=".b_fmt::escape($url));
    if(is_object($this->t) && !(($this->MODE == 'RO') && @$this->isMBE[$name] && empty($file) && empty($url))) { 
      $this->getFormRow($name, $data);
    }
    $this->dbg(exiting);
  }
  
  protected $_FILES_uploaded = array();
  /*
   * @return (array)($value,$msg)
   *   - $value file name if successfully uploaded
   *   - $msg   Error message (if any)
   */
  protected function saveFile($name){

    $this->uploadInit();
    $reply = Null;
    $error_message = '';

    $ue_code = @$_FILES[$name]['error'];
    switch ($ue_code){
    case UPLOAD_ERR_OK:

      $f = @$_FILES[$name]['tmp_name'];
      if (empty($f) || !file_exists($f)){
	// Sanity
	$error_message = "_FILES[$name][tmp_name]=$f does not exist";
	// b_debug::internalError($error_message);
      }elseif ($this->valid_fileType($name,	// Save the valid file OR return the error
				     @$this->expected_ext[$name],
				     $this->getDescr_file($name))){
	if (!($reply = $this->saveFile_destination($name))) $error_message = "Can't copy ".$_FILES[$name]['tmp_name']." -> destination";
      }else{
	MSG::ERROR($error_message = $this->valid_fileType_error);
      }
      break;
      
    case Null:
    case UPLOAD_ERR_NO_FILE:
      $error_message = (@$this->isMBE[$name] || $this->isUploaded($name) 
			? ''
			: "please upload ".$this->expected_ext[$name]." file");
      break;
      
    case UPLOAD_ERR_INI_SIZE:
    case UPLOAD_ERR_FORM_SIZE:
    case UPLOAD_ERR_PARTIAL:
    default:
      if (!($error_message = @self::$upload_errors[$ue_code]['m'])) $error_message = "An unknown error occurred during file upload";
    }
    $this->dbg($reply);
    if ($error_message) $this->dbg(b_fmt::redText($error_message));
    return array($reply,$error_message);
  }

  /*
   * Executor
   */
  protected function saveFile_destination($name){
    locateAndInclude('bFile.inc');
    $original_file = $_FILES[$name]['name'];
    $fileName = $this->uploadedFile_name($name,array(),False);
    $fileExtension = bFile_file_name_extension($original_file);
    $destination = str_ireplace(".$fileExtension.$fileExtension",
				".$fileExtension",
				b_os::join(array($this->uploadsDir,$fileName.$fileExtension)));
    if (copy($_FILES[$name]['tmp_name'], $destination)){            
      $this->dbg("OK $original_file -> $destination",cnf_dev);
      $reply = $this->_FILES_uploaded[$name] = $fileName.$fileExtension;
    }else{
      $reply = False;
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  function uploadFile_acknowledge($name) {}
  
  /*
   * ***************************************************************************************************************
   */
  private function fStar($name=Null) {
    if (self::$inputToPDF){
      $this->t->td('&nbsp;&nbsp;'," width='2%'");
    }else{
      if ($name) $symbol = (($this->MODE == 'RW') && (@$this->isVITAL[$name] ||
						      (!@$this->isMBE[$name] &&
						       !@$this->isRO[$name] &&
						       stripos(get_class($this),'search')===False))) ? '*' : '';
      else       $symbol = '';
      $this->t->td($symbol,$this->style("space"));
    }
  }
  
  /*
   *
   */
  private function fSpacer($txt='&nbsp;&nbsp;&nbsp;&nbsp;'){
    if (!self::$inputToPDF){
      $this->t->td(array($txt,$txt),$this->style('transp'));
    }
  }

  /*
   *
   */
  private function fDescr($name,$descr=Null){

    $tip = @$this->tips_byMode[$this->MODE][$name];
    if(empty($tip)) $tip = @$this->tips[$name];
    if(empty($tip)) $tip = '';
    else            $tip = bJS()->tip(bIcons()->get('i-info'),$tip);

    if (empty($descr))    $descr = $this->getDescr($name);
    //    $descr = b_t::_(array('key'=>$name,'item'=>$descr));
    if (self::$inputToPDF) $this->t->td($descr,
					" width='25%' style='text-align:right'");
    else                   $this->t->td($this->fDescrEdit($name).
					$this->colorized($name,$descr).$tip,
					$this->style("desc"));
  }
  
  /*
   * I forgot what this is for... YB
   */
  private function fDescrEdit($name){
    if (is_object($this->formDescription) && $this->isManager()){
      return $this->formDescription->widget(bIcons()->get('i-edit_red'),
					    $name);
    }else{
      return '';
    }
  }

  /*
   * Check is the field (sql column) is ReadOnly
   */
  protected function isRO($name,$onlyRW=False){
    $reply = ((($this->MODE == 'RO') || @$this->isRO[$name]) &&
	      !@$this->isRW_forced[$name] &&
	      !$onlyRW);
    return $reply;
  }

  /*
   *
   */  
  private static $fData_fields = array();
  protected function fData($data,$name){
    $data = trim($data);
    if (empty($data)) $data = $this->getValue($name,($this->MODE == 'RW'));
    if (@$this->isMONEY[$name]){
      if ($this->isRO($name)) $data = b_fmt::money($data);
    }elseif(@$this->isUL[$name]){
      $list = unserialize($data);
      if (is_array($list))       $data = x('ul',b_fmt::joinMap('li',$list));
      elseif(cnf_dev){
	b_debug::var_dump($data,"'$name' expected to be serialized");
	b_debug::traceBack();
      }
    }

    $i = (@$this->isIcon[$name]
	  ? bIcons()->get($this->isIcon[$name])
	  :'');
    // if ($name=='d_comment')    var_dump(explode('_;;_',$data));
    $data = $this->colorized($name,$data);
    if (($this->MODE == 'RO') && (stripos($name,'_debug_')===False)) $data = b_t::_(array($this,$name,$data)); // YB ZZZ new
    if (self::$inputToPDF) $this->t->td(x('em',$data)," 'width=36%'");
    else                   $this->t->td($i . $data,
					$this->style('data','',@$this->cellStyle[$name]));
    self::$fData_fields[$name][] = $data;
  }

  /*
   * Show the missing fields in red color
   */
  private function colorized($name,$value){
    if (b_posix::is_empty($value)) $value = $this->getValue($name,True);
    //    $value = b_t::_(array($this,$name,$value));
    foreach(array('-BR-'=>'<br/>',
		  '-BS-'=>"<strong>",
		  '-BE-'=>"</strong>") as $f=>$t)      $value = str_replace($f,$t,$value);
    if ((int)$this->ID){
      if (!empty($this->isVITAL[$name]) && b_posix::is_empty($value)) $this->colorCode[$name] = 'redText';
      if (!empty($this->errorsInTheForm[$name]))                      $this->colorCode[$name] = 'redText';
      if ($c=@$this->colorCode[$name]) $value = "<span class='$c'>$value</span>";
    }
    return $value;
  }
  
  /*
   * Check is the group of fields visible
   */
  private function visible_group($fields){
    $reply = False;
    if (!is_array($fields)) $fields = array($fields);
    foreach($fields as $k=>$f){
      if (is_array($f))                $field = $k; 
      elseif (is_numeric($k) || $k===0)$field = $f;
      else                             $field = $k; 

      if (is_array($f) && in_array('textHidden',$f))continue;
      if (strpos($field,'_debug') !== False)        continue;

      if ($this->_visible($field)){
	$reply = True;
	break;
      }
    }
    $this->dbg($reply,cnf_show_v);
    return $reply;
  }

  /*
   * Check is the field visible
   */
  protected function _visible($field,$value=Null){
    // Hidden fields
    if (in_array($field,$this->isHidden) || !empty($this->isHidden[$field])){
      $this->dbg("false - isHidden",cnf_dev);
      return False;
    }
    // Admin fields
    if ($reply  = ($this->MODE == 'RW' && ($this->isAdmin() || $this->isOwner()))){
      b_debug::xxx("true - Admin/Owner",cnf_show_v);
    }else{
      if ($value === Null) $value = $this->getValue($field,True);
      $spacer = (stripos($field,'spacer') !== False);
      $hide_admin_field= @$this->isADMIN[$field] && !$this->isAdmin() && !$this->isOwner();
      if (@$this->isADMIN[$field]) b_debug::_debug('hide_admin_field='.var_export($hide_admin_field,True),array($this,True));
      $reply = b_cnf::check_conditions(array(True =>array('debug'  => !$hide_admin_field && (stripos($field,'_debug') !== False),
							  'notMBE' => !$hide_admin_field && (($this->isWritable() || !empty($value)) && !@$this->isMBE[$field] && !$spacer),
							  'vitalA' => (@$this->isVITAL_ifADMIN[$field] && $this->isAdmin()),
							  'vital'  =>  @$this->isVITAL[$field],
							  ),
					     False=>array('spacer' =>  $spacer,
							  'empty'  => (($field === cnf_emptyString) || (empty($value) && @$this->isMBE[$field] && $this->isRO($field))),
							  'Xempt'  => (!$this->isWritable() && empty($value)),
							  //'ROempt' => ($this->isRO($field) && @$this->isMBE[$field] && empty($value)),
							  'local'  => (@$this->isINTERNAL[$field] && function_exists('internalCall') && !internalCall()),
							  'privat' => (@$this->isPRIVATE[$field] && !$this->isWritable())),
					     'default' => !$hide_admin_field),
				       $this,
				       cnf_show_v);
    }
    return $reply;
  }
  
  /*
   * Show the form ID, this helps a lot in debugging
   */
  private static $_debug_counter = 0;
  private function _debug_showID($only_if_never=False){
    if ((cnf_show_st || cnf_dev) && is_object($this->t) &&
	!self::$inputToPDF && (stripos(get_class($this),'_search') === False)){
      if (self::$_debug_counter++ && $only_if_never) return;
      $f = '_debug_'.__FUNCTION__.self::$_debug_counter;
      $this->isRO[$f] = True;
      $this->getFormRow($f,b_fmt::redText($this->__toString()));
    }
  }
  
  /*
   * A group of form-Block's.
   * Invoke bList/bUnit method(s) define by the arguments and display them as a form block
   *
   * @synopsis
   *  $this->formBlock_bList(array('bUnit_jam'=>array('method'=>'get',
   *						      'arg1'  => RANK_jam_secr,
   *						      'arg2'  => $this->ID,
   *						      'field' =>'_virt_pos_secretary',
   *						      'btit'  =>'Secretary', 
   *						      'icon'  =>'i-oxford')));
   */
  function formBlock_bList($args){
    
    foreach($args as $list_name=>$a){
      
      // Instantiate the list/unit class
      locateAndInclude($list_name);
      if (!empty($a['arg3'])){
	$list_class = new $list_name($a['arg1'],$a['arg2'],$a['arg3']);
	$args_once = array('arg1_once' => $a['arg1'],
			   'arg2_once' => $a['arg2'],
			   'arg3_once' => $a['arg3']);
      }elseif (!empty($a['arg2'])){
	$list_class = new $list_name($a['arg1'],$a['arg2']);
	$args_once = array('arg1_once' => $a['arg1'],
			   'arg2_once' => $a['arg2']);
      }else{
	$list_class = new $list_name($a['arg1']);
	$args_once = array('arg1_once' => $a['arg1']);
      }      
      
      // Get the list content
      $this->formDB[$a['field']] = serialize(array_values(call_user_func_array(array($list_class,$a['method']),array())));
      if (b_posix::is_empty(unserialize($this->formDB[$a['field']]))) $this->formDB[$a['field']] = serialize(array(b_fmt::redText('&lt;empty&gt;')));
      
      // Field description
      $descr = $this->getDescr($a['field']);
      // Block title
      $btit  = (empty($a['btit'])   
		? preg_replace('/\(.*/','',$descr)
		: $a['btit']);   
      // URL to see/update the list
      $url = b_url::same("?ed_bList=$list_name&".b_fmt::joinX('&',$args_once), array('form','id'));
      
      // Show the block
      $this->isRO[$a['field']] = True;
      $this->isUL[$a['field']] = True;
      if (empty($this->listCounter[$list_name])) $this->listCounter[$list_name] = 0;
      $this->formBlock($list_name.(++$this->listCounter[$list_name]),
		       $btit,
		       array($a['field']),
		       array('d'=>$btit,
			     'i'=>$a['icon'],
			     'l'=>$url)
		       );
    }
  }
	private $listCounter = array();
    
  /**
   * Display bForm block - group of fields with a title
   *
   * @param $block_ID - the block name
   * @param $blockTitle - the block title
   * @param $fields - array of the fields to display
   * @param $button (optional array) - icon, shown in front of the title
   *        'i' - icon name
   *        'f' - (optional) if present, the icon must be shown 
   *                         (normally the repeated icons are shown only ones) 
   *        'd' - (optional) icon description
   *        'l' - (optional) URL, icon then becomes a submit button
   */
  protected $visibility = False;
  private $dejaVu_formBlock = array();

  /*
   * In most of the cases the selected block is supposed to be edited, hence force RW
   */ 
  protected function formBlock_setModeRW(){
    if (@$_GET['block_once'] == $this->block_ID){
      $this->MODE = 'RW';
      $this->dbg("'block_once', set MODE=RW",True);
    }
  }

  function formBlock($block_ID,$blockTitle, $fields=array(), $button=array(),$extraTD=''){
    $this->dbg();
    if (!empty($block_ID) && @$this->dejaVu_formBlock[$block_ID]++) return;
    
    // 
    // Check the block visibility, exit if it is not visible
    $this->visibility = $this->formBlock_isVisible($block_ID,$fields);
    if(!$this->visibility){
      self::$ROskipedBlocks_counter++;
      return False;
    }

    // Apparently impose RW mode
    $this->block_ID = $block_ID;
    $this->formBlock_setModeRW();

    // The visibility is 'RO', don't let edit the data - mark all fields as RO 
    if  ($this->visibility === 'RO'){
      foreach($fields as $k=>$data){
        if (@$this->isRW_ifMissing[$k] && $this->MODE == 'RW'){
	  $v = $this->getValue($k,True);
          $RO = (!b_posix::is_empty($v) && b_posix::is_empty($this->getValue_validate_value($k,$v)));
        }else{
          $RO = True;
        }
        $this->isRO[$k] = $RO;
        if (is_array($data) && in_array('datePickerField2',$data)){
          $this->isRO[$data[1]] = $RO;
          $this->isRO[$data[2]] = $RO;
        }
      }
    }
    
    //
    // Check if there is any visible field in the block,
    // exit if not
    $toPrint = array();
    $associative = b_fmt::isArrayAssociative($fields);
    foreach ($fields  as $field=>$method){
      if (!$associative) $field= $method; 
      if ($this->_visible($field) && !self::nameCounter($field,'getCounter'))    $toPrint[] = $field;
    }

    // Do not print an empty header if there is only a photo to show
    if ($toPrint === array('av_photo') && ($block_ID == 'av_birth_block')) $toPrint = array();

    $this->dbg('toPrint: '.join(', ',$toPrint),cnf_show_st);

    //
    // Show the block title row
    $this->blockTitle($blockTitle,$button);

    //
    // Show the data rows
    $something_shown = 0;
    foreach ($fields as $field=>$method) {
      if (!is_array($method)) {
        if (preg_match("/^[0-9]*$/",$field)) { $field = $method; $method = 'textField'; }
      }
      //print "visible=".$this->_visible($field)." field=$field method=$method value=".$this->getValue($field)."<br/>";    
      if ($field == cnf_emptyString){
        if (is_object($this->t)) $this->t->tr('&nbsp;');
      }elseif (strpos($field,b_rawData)===0){
	$this->dbg('b_rawData '.var_export($method,True),cnf_dev);
        $this->defineVariable($method[0],$field);
        $this->getFormRow($field,$method[1]);
      }else{
        if ($this->_visible($field)) {
          // if (!$something_shown++ && !empty($extraTD)) $this->t->extraTD[] = $extraTD;
          if (is_array($method)){
            switch($n=count($method)){
	    case 1: call_user_func(array($this, $method[0]), $field);      break;
            case 2: call_user_func(array($this, $method[0]), $field, $method[1]);      break;
            case 3: call_user_func(array($this, $method[0]), $field, $method[1], $method[2]);      break;
            case 4: call_user_func(array($this, $method[0]), $field, $method[1], $method[2], $method[3]);     break;
            case 5: call_user_func(array($this, $method[0]), $field, $method[1], $method[2], $method[3],$method[4]); break;
            case 6: call_user_func(array($this, $method[0]), $field, $method[1], $method[2], $method[3],$method[4],$method[5]); break;
            default: b_debug::internalError("add another case with n_args=$n");
            }
            $method=$method[0]; 
          }elseif(!empty($method) && method_exists($this,$method)){
            call_user_func(array($this, $method), $field);
          }
          $this->dbg("method='$method' field='$field'");
        }
      }
    }
    // Show the form ID, this helps a lot in the debugging
    $this->_debug_showID();
    $this->block_ID = Null;
    return True;
  }

  /*
   * Check that the given block (or group of blockes) has to be shown
   * @return
   * - (bool)   True  - visible and writable
   * - (bool)   False - not visible
   * - (string) 'RO'  - visible, but not writable
   */  
  protected function formBlock_isVisible($block_ID,$fields=array()) {

    // Accept all blocks with names matching $_GET['block_once']
    $block_only = @$_GET['block_once'];

    // Check if the block was not yet filled, let it then to be filled
    $allFields_empty = ($this->MODE == 'RW');

    //    foreach(array_keys($fields) as $k){
    foreach((b_fmt::isArrayAssociative($fields) ? array_keys($fields) : $fields) as $k){
      if ((bool)$this->getValue($k,True))  $allFields_empty = False; 
    }
    
    $isVisible = b_cnf::check_conditions(array(False=>array('empty'   => ( empty($fields) || !$this->isReadable()), 
							    'not_vsbl'=> (!$this->visible_group($fields) && !$this->isWritable()),  
							    'b_only'  => (!empty($block_only) && stripos($block_ID,$block_only) === False)),
					       True =>array('b_only'  => (!empty($block_only) && stripos($block_ID,$block_only) !== False),
							    'noname'  =>   empty($block_ID),
							    'nyfiled' =>  $allFields_empty),
					       'default' => True),
					 $this,cnf_show_v);
    if ($isVisible && $block_only) $this->MODE = 'RW';
    $this->dbg($isVisible,cnf_show_v);
    return $isVisible;
  }
  
  /*
   * Show the block title
   */
  private static $dejaVuButton = array();
  private static $isSet_goBack = False;
  private static $previous_icon = '?';
  private static $editableBlocks_counter = 0;
  private static $ROskipedBlocks_counter = 0;
  protected function blockTitle($textP,$button=array()){

    if (is_array($textP) || (is_string($textP) && preg_match('/[a-zA-Z]/',$textP))) {

      $this->getFormOpen_execute(__METHOD__); // ZZZ

      if ($swap_MODE = ($this->formBlock_isWritable($this->block_ID) && (@$_GET['mode']=='RW') && ($this->MODE=='RO'))){
	$this->dbg('swap mode to RW',cnf_dev);
	//	$this->MODE = 'RW';
      }
      
      // sanity check
      if (!($this->t instanceof b_table) && $this->isEmbedded())                      $this->t = $this->parentForm->t;
      if (!($this->t instanceof b_table) && (self::$t_formOpened instanceof b_table)) $this->t = self::$t_formOpened;
      if (!($this->t instanceof b_table)){
	if(cnf_dev) {
	  $this->dbg($msg=b_fmt::redText('see traceback, '.($this->isEmbedded()?'embedded ':'').$this."->t is not 'b_table' but '".$this->t."'"),True);
	  b_debug::var_dump($this->t,$msg);
	  b_debug::traceBack("'\$this->t' is NOT 'b_table'");
	}
	$this->t = new b_table ("class='b_form'");
	$this->t->highlightRows = ($this->MODE == 'RO');
	$this->t->b_table_div = True;
      }

      // Optionally skip the text title
      if ($this->blockTitle_skip($this->block_ID))	return;
      
      // split the input argument into text + buttons 
      if (is_array($textP)){
	$text = array_shift($textP);
	$action_buttons = $textP;
	bIcons()->toggleExplainIcons(False);
      }else{
	$text = $textP;
	$action_buttons = array();
      }
      
      if (self::$inputToPDF) $action_buttons = $button = array();

      //
      // In the first versions of bForm the block icon was also a  "block submit button".
      // This is not intuitive enough, so now we ignore the "block icon" and replace it by "edit icon" (a "pencil"),
      // and the button description is not used - it is auto-generated
      if (@$_GET['mode_once'] == 'RO')	unset($button['l']);
      if (!cnf_inside_fb && !$this->isWritable() && isset($button['l'])){
	if (cnf_dev) MSG::DEBUG("$this: not writable form, cancel button '".@$button['d']."'");
	unset($button['l']);
      }
      
      if ($url = @$button['l']){
	$buttonImg = '';
	if ($this->isWritable()){	
	  // The 'action buttons' are converted to "submit" in the RW mode
	  $button['d'] = preg_replace('/\(.*/','',b_t::_(array('key'=>"bt;".($x="update $text"),'item'=>$x)));
	  if(stripos($url,'ed_bList')===False)
	    $url_updateBlock=b_url::repack($url,array('block_once'=>$this->block_ID,'action_once'=>@$button['d']));
	  else
	    $url_updateBlock=$url;
	  
	  if($this->MODE == 'RO'){
	    self::$editableBlocks_counter++;
	    $buttonImg = x("a href='$url_updateBlock'",bIcons()->get(array('i'=>'i-edit',
									   'd'=>$button['d'],
									   'X'=>1)));
	  }elseif (bForm::$formOpened == $this->ID){
	    $buttonImg = bIcons()->get($button,'submit');
	  }
	}elseif(isset($button['i'])){
	  $button['d'] = b_t::_(array('key'=>"bt;$text",'item'=>$text));
	  $buttonImg = bIcons()->getButton($button);  
	  if (@self::$dejaVuButton[md5($text)]++)     $text = '';
	  if (@self::$previous_icon === $button['i'])$buttonImg = '';
	  self::$previous_icon = $button['i'];
	}
      }else{
	$buttonImg = bIcons()->getButton($button);
      }
      
      $textP = trim($text);
      
      if ($this->t->highlightRows) $this->t->thead_o();
      $this->t->tro();
     
      if (!self::$inputToPDF) $this->t->td((cnf_show_st ? "$this " : "").$buttonImg,$this->style('transp','align_right'));
      if ( self::$inputToPDF && !empty($textP)) array_unshift($action_buttons,'<br/>');

      $this->t->th(join('',$action_buttons) . x('strong',b_t::_(array('key'=>"bt;$textP",'item'=>$textP))),"colspan=4");
      $this->t->trc(False);
      if ($this->t->highlightRows) $this->t->thead_c();
    }
  }

  /*
   * Helper for Show the block title
   */
  protected function blockTitle_skip($block_ID){ 
    return False;
  }
  
  /*
   * Build the url based on the current one.
   * Used for "update" & "cancel" buttons
   */
  public function sameURL_but($args=array(),$asArray=False,$returnArgs=False){

    if($args === 'RW')       $args = array('mode'=>$args);
    elseif(is_string($args)) $args = b_crypt::parse_query_string($args);

    if (is_array($asArray)){
      $drop = $asArray;
      $args['clean_once'] = join(',',$drop);
      $asArray = False;
    }else{
      if (empty($args['id']))        $args['id']  = $this->ID;
      if (empty($args['mode']))      $args['mode']= 'RO';
      if (empty($args['form']))      $args['form']= get_class($this);
      $drop = array();
    }
    $reply = ($returnArgs
	      ? $args
	      : b_url::same($args,$drop,$asArray));
    return $reply;
  }

  /*
   *
   */
  function log($type='create', $text='', $class=False) {
    if (empty($class)) $class = $this;
    myPear_logs()->add($type,$class->ID,$text);
  }
}
