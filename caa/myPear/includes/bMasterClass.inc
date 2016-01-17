<?php
/*
 * keep the unique counter
 *
 * Usage:
 * $c = new bMasterClass();
 * $reserveCounter: $c->reserve();
 */ 

define('bMasterClass_where'," WHERE master_object = 'masterCount'");

class bMasterClass{
  
  private $masterCounter = Null;

  // The myPear database can serve multiple clients with different CMS installations. 
  // '$cms_id' keeps "the client CMS ID"n order to get the correct modules list for the each client.
  private $cms_id = '';

  private static $__constructs = 0;
  public  $debug   = False;

  function __construct(){
    // Create the unique module ID
    if (!cnf_CLI && empty($_SESSION[__class__])){
      if (False){
	// Wrong
	$this->cms_id = join('_',array('',myPear_db()->Host));
      }else{
	// Correct
	$q = myPear_db()->query("SELECT REVERSE(SUBSTRING_INDEX(REVERSE(USER()),'@',1)) as ip");
	while($r = myPear_db()->next_record($q)){
	  $this->cms_id = join('_',array('',HOSTED_BY_CMS,HOSTED_BY_CMS_vrsn,$r['ip']));
	}
      }
      if (!empty($this->cms_id)) $_SESSION[__class__] = $this->cms_id;
    }
    if (!empty($_SESSION[__class__])) $this->cms_id = $_SESSION[__class__];
  }
  
  /*
   * Wait until the database become available,
   * create the master table if not yet done
   */
  private static $isReady = False;
  private function init(){
    if (!self::$isReady && myPear_db()->isReady()){

      // Create the master table if it does not exist
      if (!myPear_db()->tableExists('zzz_master')){
	myPear_db()->qquery('DROP TABLE IF EXISTS zzz_master',cnf_dev); // protection against cache problems
	myPear_db()->qquery(file_get_contents(dirname(__FILE__).'/../sql/zzz_master.sql'),cnf_dev);
	myPear_db()->reset_cache();
      }

      // Check that the database was not imported from another server
      if (!in_array(myPear_db()->Host,($cv=$this->_getValues('masterHost')))){
	$fr = (empty($cv) ? '' : "'".join(',',$cv)."' -> ");
	myPear::MESSAGE(sprintf("Truncate master table for %s'%s'",$fr,myPear_db()->Host));
	myPear_db()->truncateTable('zzz_master');
	$this->_addValue('masterHost',myPear_db()->Host);
      }
      self::$isReady = True;
    }
    return self::$isReady;
  }

  /*
   * Reset the master class. Used mainly to merge databases.
   */
  public function reset(){
      self::$isReady = False;
      myPear_db()->truncateTable('zzz_master');
      myPear_db()->reset_cache();
      $this->init();
      myPear_db()->reset_cache();
  }

  /*
   * Declare the "master counter" used, i.e. this value will not be given to anybody else
   */
  public function reserve_masterCounter(){
    
    try {
      if ($this->init()){
	// Adjust the master counter
	myPear_db()->transaction('start');
	$this->_adjust_masterCounter();
	$this->_update_masterCounter();
	myPear_db()->transaction('end');
      }
    }catch (Exception $e) {
      b_debug::internalError("Exception $e");
    }
    $this->dbg('reserve');
    return $this->masterCounter;
  }
  
  /*
   * get the current value of the "master counter" - the current record number
   */
  public function get_masterCounter(){
    if ($this->masterCounter === Null){
      myPear_db()->transaction('start');
      $this->_adjust_masterCounter(True);
      myPear_db()->transaction('end');
    }
    return $this->masterCounter;
  }

  /*
   * get list of users with the superuser privileges
   */  
  public function get_masterUsers(){
    return $this->getValue('masterUser');
  }

  /*
   * Register myPear module configuration file.
   * The reason for registering:
   *   CMS systems (drupal, etc.) load the modules in an arbitrary
   *   order, which causes problem for a module-defendant authentication. 
   *   The solution is to load the config_file when it is really needed, 
   *   hence we need to have the list of those.
   *
   * Obviously, the problem does not occur if only one myPear module is used, but that is rare :-)
   *
   * @param $module - myPear module name
   * @param $config_file - the module configuration file 
   */
  public function add_masterModule($module,$config_file){
    if (!empty($this->cms_id)){
      $module .= $this->cms_id;
      if ((strpos($module,myPear_MODULE)==False) && !@$_SESSION[myPear_cache][__METHOD__][$module]++){
	$this->addOption('masterModule',$module,$config_file);
      }
    }
  }

  /*
   *
   */
  public function get_masterModule($module){
    return $this->getOption('masterModule',$module);
  }
  
  /*
   * Get list of myPear modules
   */
  public function get_masterModules(){
    $reply = array();
    foreach($this->getValue('masterModule') as $m){
      if (!empty($this->cms_id) && strpos($m,$this->cms_id) === False) continue;
      $reply[] = (empty($this->cms_id)
		  ? preg_replace('/_.*/','',$m)
		  : str_replace($this->cms_id,'',$m));
    }
    return array_unique($reply); 
  }

  /*
   * Register a module update.
   *
   * @param return False if the update is already registered,
   *        i.e. there is no need to apply them again.
   */
  public function register_module_update_script($module,$version,$update_script){

    // Apply the updates (if any) only at the start of the session
    $id = __FUNCTION__.$update_script;
    if (isset($_SESSION[$id])){
      $_SESSION[$id] = False;
    }else{
      $reply = False;
      if ($mtime = filemtime($update_script)){
	$registered = $this->getOption('masterUpdate',$update_script);
	$reply = ((int)$mtime > (int)$registered);
	// In CLI mode the updates are applied, but not registered,
	// since the pdates might be CLI-mode-depandant
	if ($reply && !cnf_CLI){
	  $this->addOption('masterUpdate',$update_script,$mtime);
	  myPear::MESSAGE('Applying update script "'.basename($update_script).'".'.
			  ($registered > 333333 ? ' Previous update from '.b_time::short_date_time($registered).', i.e. '.b_time::delta($mtime,$registered).' ago.' : ''));
	  myPear_logs()->add('update','',"$module $version",myPear_cronjob()->ID,$module,True);
	}
      }
      $_SESSION[$id] = $reply;
    }
    return @$_SESSION[$id];
  }

  /*
   * Be sure that the "master counter" does not present in the database
   */
  private function _adjust_masterCounter(){

    try {
      
      // Walk thru the database and find the maximum value of the record ID
      $primary_keys = array();
      myPear_db()->tableNotEmpty('reset');
      foreach(myPear_db()->get_primaryKeys() as $table=>$keys){
	if (myPear_db()->tableNotEmpty($table)) $primary_keys[$table] = $keys;
      }
      
      $maxID = 1;
      if (!empty($primary_keys)){
	$q = $this->query("SELECT ".join(', ',array_values($primary_keys))." FROM ".join(', ',array_keys($primary_keys)));
	while($r = myPear_db()->next_record($q)){
	  $maxID = max(array_merge(array($maxID),array_values($r)));
	}
      }
      
      // Now read the "maximum record ID" as it is kept in the master record.
      $q = $this->query("SELECT master_value FROM zzz_master ".bMasterClass_where);
      if (!myPear_db()->num_rows($q)){
	$this->query("INSERT INTO zzz_master SET ".join(',',array("master_object='masterCount'",
								  "master_value = $maxID")));
	$q = $this->query("SELECT master_value FROM zzz_master " . bMasterClass_where);
      }
      $r = myPear_db()->next_record($q);
      $this->masterCounter = $r['master_value'];
      
      //
      // Check the database counter
      // This value must be less-then-OR-equal-to $maxID
      if ($maxID > $this->masterCounter){
	// The max record_ID is higher then the masterCounter saved in the database.
	// Presumably the records were updated by another process,
	// update masterCounter in the database
	$this->dbg('init',"maxID=$maxID");
	if (!empty($this->masterCounter)){
	  $msg = "non-zero this->masterCounter < maxID:  $this->masterCounter < ".($maxID+1);
	  myPear::WARNING_T($msg);
	  // b_debug::report_bug($msg);
	}                                                                                               
	myPear::WARNING_T("set $this->masterCounter -> ".($maxID+1),$this);
	$this->masterCounter = $maxID;
	$this->_update_masterCounter();
      }elseif($maxID == $this->masterCounter) {
	// The counter stays unchanged, good.
      }elseif(cnf_dev || cnf_CLI){
	// Something is wrong, the counter value in the database 
	// is smaller then the the previous value
	$msg = "??? maxID = $maxID, but \$this->masterCounter = $this->masterCounter";
	myPear::WARNING_T($msg,$this);
	myPear::DEBUG($msg);
      }
    }catch (Exception $e) {
      b_debug::internalError("Exception $e");
    }
  }
  
  /*
   * return an array of arbitrary objects from the master table
   */
  private function getValue($object,$value=Null){
    if ($this->init()) return $this->_getValues($object,$value);
  }
  
  private function _getValues($object,$value=Null){
    $reply = array();
    $where = array("master_object = '$object'");
    if (!empty($value)) $where[] = "master_value='$value'";
    
    $q = $this->query("SELECT * FROM zzz_master WHERE ".join(' AND ',$where));
    
    while ($r = myPear_db()->next_record($q)) $reply[] = (empty($this->cms_id) && ($object == 'masterModule')
							  ? preg_replace('/_.*/','',$r['master_value'])
							  : $r['master_value']);
    return $reply;
  }

  private function deleteValue($object,$value=Null){
    $where[] = "master_object = '$object'";
    if ($value) $where[] = "master_value = '$value'";
    $this->query("DELETE FROM zzz_master WHERE ".join(' AND ',$where));
  }

  /*
   * Add new data
   */
  public function addValue($object,$value){
    if ($this->init()){
      return $this->_addValue($object,$value);
    }
  }
  private function _addValue($object,$value){
    if (!empty($value) && !in_array($value,$this->_getValues($object))){
      $this->query("INSERT INTO zzz_master SET master_object='$object',master_value='$value'");
      $reply = True;
    }else{
      $reply = False;
    }
    return $reply;
  }

  /*
   *
   */
  private function getOption($object,$value){
    $reply = Null;
    if ($object=='masterModule'){
      $value .= $this->cms_id;
      $regexp = empty($this->cms_id);
    }
    if ($this->init()){
      if (!empty($value) && in_array($value,$this->_getValues($object))){
	$q = $this->query($x="SELECT master_option FROM  zzz_master WHERE master_object='$object' AND ".(@$regexp
													 ? myPear_db()->quote("${value}_",'RR','master_value')
													 : "master_value = '$value'"));
	while($r = myPear_db()->next_record($q)) $reply = $r['master_option'];
      }
    }
    return $reply;
  }
  
  /*
   *
   */
  private function addOption($object,$value,$option){
    if ($this->init()){
      $this->addValue($object,$value);
      if (!empty($value)){
	$where = "master_object='$object' AND master_value='$value'";
	$q = $this->query("SELECT master_option FROM zzz_master WHERE $where");
	while($r = myPear_db()->next_record($q)){
	  if (myPear::is_equal($option,$r['master_option'])) $is_equal = True;
	}
	if (!@$is_equal) $this->query("UPDATE zzz_master SET master_option='$option' WHERE $where");
      }
    }
  }
  
  /*
   * Update the "free counter" 
   */
  private function _update_masterCounter() {
    $this->masterCounter++;
    $this->dbg('set');
    $this->query("UPDATE zzz_master SET master_value=".$this->masterCounter.bMasterClass_where);
  }
  
  private function query($query,$debug=0){
    $q = myPear_db()->query($query);
    return $q;
  }

  private function dbg($method,$txt='',$force=False){
    b_debug::_(get_class($this),$method,"$this->masterCounter  $txt",(bool)($this->debug || $force));
  }
  
  function __toString() { 
    return __CLASS__;
  }
}
