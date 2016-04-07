<?php
/*
 *  $cache = new b_cache($cache_id,'S');
 *  if (!$cache->wasSet($what)) $cache->set($value);
 *  $value = $cache->get();
 */
define('b_cache', 'b_cache');

class b_cache_S extends b_cache{
  function __construct($arg){
    $id = (is_object($arg) ? get_class($arg).'_'.@$arg->ID : $arg);
    parent::__construct($id, 'session');  
  }
}

class b_cache_G extends b_cache{
  function __construct($arg){
    $id = (is_object($arg) ? get_class($arg).'_'.$arg->ID : $arg);
    parent::__construct($id, 'globals');  
    if (self::$n> 280) b_debug::traceBack();
  }
}


class b_cache {
  static protected $n = 0;
  var $noCache = False;
  var $debug   = False;
  var $timeStamp = 0;
  var $stack   = array();
  var $tmpCache= array();
  private $holder  = 'G';
  
  function __construct($arg,$type='globals') {  
    if (b_cnf::get('nocache',False)) $this->noCache = True;
    if ($this->noCache){
      myPear::WARNING(b_cache."($type) IS OFF");
    }else{

      if (empty($arg)) b_debug::internalError("missing cache id");
      $this->holder= (substr(strToUpper($type),0,1)=='G') ? 'G' : 'S';
      $this->ID = $arg;

      if (empty( $GLOBALS[b_cache]))  $GLOBALS[b_cache] = array();
      if (empty($_SESSION[b_cache])) $_SESSION[b_cache] = array();
      
      if ((b_cnf::get('cachedebug') > 0) || (cnf_debug_level >= 100))      $this->debug = True;
      $this->dbg();
    }
  }
  
  /**
   * Check that the values is in the cache
   */
  function wasSet($name='', $id='', $key='') {
    // Say 'No' if no caching  
   if ($this->noCache)      return False;

    // Set the value_id stack     
    $this->_toStack($name, $id, $key);
    
    // watch the cache sise...
    $this->_exec('sanity');

    // get reply
    return $this->_exec('wasset');
  }

  /**
   * Save to the cache the value which was just enquired
   */
  function set(&$value=Null) {
    if (!$this->noCache)      $this->_exec('set',$value);
    return $value;
  }

  /**
   * Reset (empty) the cache
   */
  function reset($addCount=True) {
    $this->_exec('reset');
  }
  
  /**
   * Get a value from the cache
   */
  function get() {
    if ($this->noCache)      b_debug::internalError(b_debug::__());
    return $this->_exec('get');
  }
  
  /**
   * Remove a value from the cache
   */
  function remove($name='', $id='', $key='') {
    $this->_toStack($name, $id, $key);
    $this->_exec('unset');
  }

  /*
   * Executor
   */
  private function _exec($what, &$value=Null){
    static $cache_size_limit = 77;

    // If cache is off, just return. However, the 'get' request is fatal in case of "no cache"
    if ($this->noCache)  return $value;

    // Which cahe we are in?
    if ($this->holder == 'G')        $cache_array = &$GLOBALS;
    else                             $cache_array = &$_SESSION;
    if (empty($cache_array[b_cache]))$cache_array[b_cache] = array();
    
    // Get the unique ID
    $hash_id = $this->_hash();

    // What should we do?
    $reply = Null;
    switch($what){

    case 'sanity':
      if (count($cache_array[b_cache]) < $cache_size_limit) break;
      if(cnf_dev){
	$dbdID = b_fmt::strcut('cache_autoReset'.x('(',join(',',array_merge(array($this->holder,$this->ID),
									    $this->stack))),30);
	if ($deep_bug = False){
	  $debug_a = array();
	  foreach($cache_array[b_cache] as $hash_id=>$v){
	    if (is_array($v))                             $f_value = 'array('.count($v).' elements)';
	    elseif($f_value=myPear::getClassFromID($v))$f_value.= "($v)"; 
	    else          	                          $f_value = b_fmt::strcut(b_fmt::_($v),66,'...');
	    $debug_a[] = $f_value;
	  }
	  sort($debug_a);
	  b_debug::print_r($debug_a,$dbdID);
	}
      }
      static $restN = 0;
      myPear::WARNING_T("b_cache reset #".(++$restN),$this);
      
    case 'reset':
      $cache_array[b_cache] = array();
      $cache_array[b_cache.'_timestamp'] = array();
      bCount()->add('cache_reset',$this->holder);
      break;

   case 'unset':
      unset($cache_array[b_cache][$hash_id]);
      break;

    case 'set':
      $this->counter('set');
      $cache_array[b_cache][$hash_id] = $value;
      $cache_array[b_cache.'_timestamp'][$hash_id] = time();
      break;

    case 'wasset':
      //ZZZ      $v = @$cache_array[b_cache][$hash_id];
      //ZZZ      if ($reply = ($v !== Null))  	$this->tmpCache[] = $v;
      if ($reply = (@$cache_array[b_cache][$hash_id] !== Null))  	$this->tmpCache[] = $hash_id;
     break;

    case 'get':
      $this->counter('get');
      //ZZZ      $reply = array_pop($this->tmpCache);
      $hash_id = array_pop($this->tmpCache);
      $reply = $cache_array[b_cache][$hash_id];
      break;

    case 'getTimeStamp':
      $reply = strfTime("%e %h %Y %H:%M:%S",$cache_array[b_cache.'_timestamp'][$hash_id]);
      break;

   default:
      b_debug::internalError("? $what");
    }
    
    if ($reply !== Null){
      $this->dbg($reply,4);
      return $reply;
    }else{
      if ($what != 'sanity')  $this->dbg('',4);
    }
  }

  /*
   * Make a hash from the nae / key /id
   */
  private function _hash(){
    return b_crypt::hash(array_merge(array($this->ID),$this->stack),'md5');
  }

  /*
   */  
  function getTimeStamp(){
    return $this->_exec('getTimeStamp');
  }

  function getAll($name) {
    b_debug::xxx('TBD');
    return  array();
 }

  private function _toStack($name='', $id='', $key='') {
    if (empty($name))  $name = '.';
    if (empty($id))    $id   = '.';
    if (empty($key))   $key  = '.';
    $this->stack = array($name,$id,$key);
  }
  
  function counter($f){
    list($name, $id, $key) = $this->stack;
    bCount()->add(join('-',array(b_cache,$this->holder,strToUpper($f))),
			  $name,
			  $key);
  }
  
  function show(){
    b_debug::xxx('TBD');
  }
  
  private function dbg($text='',$level=3) {
    if ($this->debug)  b_debug::xxx($text,array($level,'greyText',$this,True));
  }

  function __toString() { 
    return "b_cache-".$this->ID; 
  }
}
