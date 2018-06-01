<?php
/*
 * Setup the Variables
 */
locateAndInclude('bForm');
class myPear_Setup  extends bForm{

  public $debug = False;

  public $modalDialog = False;
  public $modalRW_enabled = True;

  public $doNotShowChanges= True;
  public $what = '?';  
  public $msg  = '';
  public $setup=array('db'  =>array('id'  =>      'bDB',
				    'title'=>     'Select application database',
				    'name'=>array('name'    =>'Database name',
						  'descr'   =>'Enter the MySQL database name for myPear applications.-BR-The database must exist',
						  'default' =>'database'),
				    'user'=>array('name'    =>'Database user name',
						  'descr'   =>'The user must have the read/write access to the database',
						  'default' =>'username'),
				    'pass'=>array('name'    =>'Database password',
						  'descr'   =>'Enter the myPear database password',
						  'default' =>'password'),
				    'host'=>array('name'    =>'Database server',
						  'default' =>'localhost',
						  'descr'   =>'Enter the myPear database host name'),
				    'port'=>array('name'    =>'Database port',
						  'descr'   =>'If your database server is listening to a non-standard port,-BR-please enter its number',
						  'default' => '3306')),
		      'smtp'=>array('id'   =>     'bMailer',
				    'title'=>     'Select SMTP server',
				    'host'=>array('name'    =>'SMTP server name',
						  'descr'   =>'Enter the SMTP server name',
						  'default' =>'smtp.google.com'),
				    'port'=>array('name'    =>'SMTP port',
						  'descr'   =>'If your server is listening to a non-standard port,-BR-please enter its number',
						  'default' => '587'),
				    'user'=>array('name'    =>'User Name',
						  'descr'   =>'If your server requires authentication (i.e. is listening to port 465 or 587),-BR-please enter the username',
						  'optional'=>'yes'),
				    'pass'=>array('name'    =>'Password',
						  'descr'   =>'If your server requires authentication,-BR-please enter the password',
						  'optional'=>'yes'),
				    ));
  private $dialog = array();
  private $show_final_form = False;
  
  function __construct() {
    parent::__construct('new','RW',False);
    foreach($this->setup as $block=>$vars){
      foreach(array_keys($vars) as $k) $this->defineVariable(array($k,$k),'only_if_not_defined');
    }
  }
  
  /*
   *
   */
  function show_dialog(){
    static $dejavu = 0;
    $setup = @$this->setup[$this->what];
    if ($setup !== Null){
      $this->defineVariables2($setup);
      $this->formOKspecific();
      if (!$this->errorless() || $this->show_final_form || @$_POST['_post_bForm_class_once'] === get_class($this)){
	if ($dejavu++)   return; // there might be many non-configured variables, lets do it one by one  
	$dialog = $this->getWidget('none');
	if ($this->modalDialog) bJS()->fancybox_inline($dialog);
	else                    print $dialog;
      }
    }
  }

  /*
   *
   */  
  function defineVariables2($setup=array()) {
    $vars = $isMBE = $IDV = array();
    foreach($setup as $k=>$value){
      if (is_array($value)){
	$key = $this->_key($k);
	$this->dialog[] = $key;
	$vars[$key]  = $value['name'].'-SKIP-:-BR-'.$value['descr'];
	$isMBE[$key] = (@$value['optional']==='yes');
	if ($d = @$value['default']) $IDV[$key] = $d;
      }
    }
    $this->defineVariable($vars,True);
    $this->defineTypes(array('isMBE'=>$isMBE,
			     'IDV'  =>$IDV));
  }
  
  /*
   *
   */
  protected function getValue_validate_empty($name,&$value=Null){
    $err_msg = '';
    if (b_posix::is_empty($value)){
      $err_msg = parent::getValue_validate_empty($name,$value);
    }
    $err_msg = '';
    return $err_msg;
  }

  protected function formBlock_isVisible($block_ID,$fields=array()){
    return True;
  }

  function isWritable(){
    return True;
  }

  function formBlock_isWritable($block_ID){
    return True;
  }

  protected function displayErrors_preflight(){
    // Check what the parent thinks
    $errors = parent::displayErrors_preflight();
    foreach($errors as $k=>$e) $this->dbg("$k: $e");
    $errors = array();
    return $errors;
  }

  protected function getFormBody(){
    $blockTitle = (empty($this->msg) 
		   ? $this->setup[$this->what]['title']
		   : b_fmt::redText($this->msg));
    $this->formBlock("",$blockTitle,$this->dialog);
  }
  
  
  private function _key($key){
    if (strpos($key,'_')!==False) $reply = $key;
    else                          $reply = $this->setup[$this->what]['id'] .'_'. $key;
    return $reply;
  }

  function getValue($key,$errorIfMissing=False,$MBE=True) {
    $name = $this->_key($key);
    if (($reply = @$_POST[$name])===Null){
      if (($reply = b_cnf::variable_get($name,Null))===Null){
	if ($errorIfMissing)  $this->errorsInTheForm[$name] = missing_value;
	$reply = parent::getValue($name,True,$MBE);
      }
    }
    $this->dbg($reply);
    return $reply;
  }
  
  function formOKspecific() {
    if (empty($this->setup[$this->what])) return;
    $errorKey = ' ';
    switch($this->what){
    case 'db':
      // Make sure that the database attrubutes are correct
      $host = $this->getValue('host',1) . (($p = $this->getValue('port')) ? ":$p" : "");
      $name = $this->getValue('name',1);
      $user = $this->getValue('user',1);
      $pass = $this->getValue('pass',1);
      if (class_exists('mysqli',0)){
	$this->dbg("new mysqli($host,$user,$pass,$name)");
	
	$l = error_reporting(0);
	$db = new mysqli($host,$user,$pass,$name);
	if (mysqli_connect_errno()) $err = mysqli_connect_error();
	else                        $err = $db->error;
	if (empty($err)) $this->msg = '';
	else             $errorKey = 'database connect error';   
	error_reporting($l);
	
      }else{
	$err = "add code for the other sql classes rather then mysqli...";
      }
      $this->msg = 'Database is configured as follows';
      break;
      
      // Make sure that the smtp settings are correct
    case 'smtp':
      $this->getValue('host',1);
      break;
    }      
    
    // In case of success store the values in the database
    foreach($this->setup[$this->what] as $item=>$setup){
      list($k,$v) = array($this->_key($item),$this->getValue($item));
      if (!empty($v) && (b_cnf::variable_get($k) !== $v)){
	MSG::MESSAGE("Saving $k = '$v'");
	b_cnf::variable_set($k, $v, 'db');
	$this->show_final_form = True;
      }
    }
    
    if (empty($err)){
      $this->MODE = 'RO';
    }else{
      $this->defineVariable($err,'error');
      $this->errorsInTheForm['error'] = $errorKey;
      $this->msg = $err;
    }
    if (!$this->errorless()) $this->MODE = 'RW';
  }
}
