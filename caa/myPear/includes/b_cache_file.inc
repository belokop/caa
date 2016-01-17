<?php
  /**
   * Aggressive html page caching, only for the Unix-type servers.
   *
   * Example of usage:
   *
   *    $file_cache = new b_cache_file();
   *    if ($file_cache->notYetReady()){
   *      <normal page processing>
   *    }
   *    print $file_cache->get();
   *
   require_once '../myPear/config.inc';
   $t = new b_cache_file();
   $t->get();
  */

// To cancel the cache uncomment this line
// $_GET['resetcache_once'] = 1;

class b_cache_file{
  
  public $debug = False;

  // Pages which are loaded faster then 2 seconds we will not cache...
  public static $alreadyFast= 2; 

  public static $cacheArea_anonym  = '/tmp/delete.me.caches_anonym/';
  public static $cacheArea_auth = '/tmp/delete.me.caches_auth/';
  public static $triggers   = array('q','x','z','option','form','id','av_id','privacy','flavor','org','bForm_Position','sBy','sByDir','cnf_btabs1','cnf_btabs2');

  // internal variables
  private static $cachePosible = True;
  private static $cacheEmpty   = True;

  var     $ID = 0;
  private $cacheArea;
  private $file  = Null;
  private $state = Null;
  private $startTime = Null;
  private static $cmsCache = array();

  function __construct($fileName=Null){
    static $cache_counter = 0;    
    if(cnf_show_cache) $this->debug = True;
    $this->ID = ++$cache_counter;

    $this->cacheArea = (is_object(bAuth::$av)
			? self::$cacheArea_auth
			: self::$cacheArea_anonym);

    if (!empty($fileName)) $this->file = $this->cacheArea . $fileName;
    if (b_os::type() == 'win')  self::$cachePosible = False;
    
    // Optionally wipe the cache
    if (self::$cachePosible){
      // create the cache directory
      foreach(array(self::$cacheArea_auth,self::$cacheArea_anonym) as $dir){
	if (!is_dir($dir) && !mkdir(($dir),0777,True)) b_debug::internalError(__METHOD__." can't create $dir");
      }
      // Remove old (those created >24 hours ago) caches 
      if (!isset($_SESSION['garbageCollection'])){
	$cmd = 'find '.$this->cacheArea.' -mount -type f -atime +1 -exec rm -rvf {} \; 2>&1';
	$out = trim(`$cmd 2>&1`);
	if (cnf_dev && !empty($out)) myPear::DEBUG(get_class($this).'->garbageCollection: '.x('pre',$out));
	$_SESSION['garbageCollection'] = $cmd;
      }
    }
    $this->dbg('cache is '.(self::$cachePosible ? '' : 'NOT').' possible');
  }

  /**
   * Initialize the page cache
   * Currently the cache is implemented for non-Windows servers only
   *
   * @return
   *   true  - the page is not in the cache, hence should be built
   *   false - the page is in the cache
   */
  function notYetReady($use_GET_as_trigger=True){

    if (!self::$cachePosible) return True;

    /*
     * Construct the cache file name
     */
    if (!empty($this->file)){
      self::$alreadyFast = -1;
      if (!is_dir($dir=dirname($this->file)))       mkdir($dir,0777,True);
    }else{
      // The cache is a complicated issue, good ideas here might bring you a fortune, see www.google.com ...
      // Here the cache is triggered by the query and authentication strings.
      // For the latter a 'privacy option' is set depending on the context.
      // However, some query items, like X/Y of the click, might be safely ignored for the trigger,
      // which makes life a bit easier... Still to be improved...
      if (function_exists($module_specific_triggers=b_reg::$current_module.'_b_cache_file')){
	$triggers = $module_specific_triggers();
	if (empty($triggers)) return True;
      }else{
	$triggers = array_merge($_POST,array('avid'=>(is_object(bAuth::$av) ? bAuth::$av->ID : 0)));
	foreach(array_merge(self::$triggers , b_cnf::_()) as $t) {
	  if ($r = @$_GET[$t])          $triggers[$t] = $r;
	  if ($r = b_cnf::get($t))      $triggers[$t] = $r;
	}
      }
      if ($use_GET_as_trigger){
	foreach(preg_grep('/_once/',array_keys($_GET)) as $k) $triggers[$k] = $_GET[$k];
      }
      ksort($triggers);
      if ($this->debug) b_debug::print_r($triggers,get_class($this).' triggers');
      
      $this->file = $this->cacheArea . b_crypt::hash(b_fmt::joinX(' ',$triggers));
    }

      /*
       * Remove the cache if requested
       */
    if (isset($_GET['resetcache_once'])){
      $this->dbg("removing ".$this->file);
      b_os::unlink($this->file);
      self::emptyCache();
    }
    
    /*
     * Is the page already in the cache?    
     */
    switch (file_exists($this->file)){
    case False:
      $this->state    = 'collect';
      $this->startTime= time();
      ob_start();
      $this->dbg('collect starts');
      return True;
      
    case True:
      $this->state = 'ready';
      $this->flush_js($this->file);
      $this->dbg('collect completed');
      return False;
    }
  }
  
  private function flush_js($cache){
    if (file_exists($js_cache = "$cache.cms")){
      $this->dbg("creating $js_cache");
      //
      // Here we are in an "interesting situation" - the html code is cached in a file,
      // but the inner cache of the CMS which was collected during the html build is not in this file.
      // I resolve it by creating an extra cache (see class bJS.inc).
      // Now is time to restore those settings and just repeat the calls to CMS which were there during
      // the page build.
      //
      foreach(parse_ini_file($js_cache,True) as $n=>$item){
	$args = array();
	$this->dbg($n);
	foreach ($item as $key=>$value){
	  $this->dbg("$n - $key = $value");
	  if ($key == 'callback') $function = $value; 
	  else                    $args[] = str_replace('-QUOTE-','"',$value);
	}
	list($class,$method) = explode('::',$function);
	switch($class){
	case 'myPear':
	  switch(count($args)){
	  case 1:  myPear::$method($args[0]);                          break;
	  default: b_debug::internalError(count($args)." $function arguments, please complete the switch");
	  }
	  break;
	case 'bJS':
	  switch(count($args)){
	  case 0:  bJS()->$method();                           break;
	  case 1:  bJS()->$method($args[0]);                   break;
	  case 2:  bJS()->$method($args[0],$args[1]);          break;
	  case 3:  bJS()->$method($args[0],$args[1],$args[2]); break;
	  default: b_debug::internalError(count($args)." $function arguments, please complete the switch");
	  }
	  break;
	default: b_debug::internalError("unexpected $function, please complete the switch");
	}
      }
    }
  }
  
  static function CMS($callback, $args){
    static $counter = 0;
    if (self::$cachePosible){
      self::$cmsCache[] = '[ function'.(++$counter).' ]';
      self::$cmsCache[] = "callback = \"$callback\"";
      foreach($args as $n=>$arg) self::$cmsCache[] = "arg$n = \"".str_replace('"','-QUOTE-',$arg)."\"";
    }
  }

  /**
   * There are cases when the page must not be cached to prevent chaos, say:
   * - a form being filled
   * - ?
   */
  private function keepCache(){
    $reply = (bool)((@$_REQUEST['mode'] != 'RW') &&
		    (@$_REQUEST['id']   != 'new') &&
		    ((time() - $this->startTime) > self::$alreadyFast));
    $this->dbg(($reply?'YES':'NO').', build takes '.b_time::delta(time(),$this->startTime));
    return $reply;
  }
  
  function dbg($text){
    if ($this->debug) b_debug::xxx($text,array(3,'greenText',$this,True));
  }

  function get(){
    if (self::$cachePosible){
      switch ($this->state){
	
      case 'ready':  
	$this->dbg($this->state.' - '.$this->file);
	self::$cacheEmpty = False;
	if (is_file($this->file)){ 
	  $this->flush_js($this->file);
	  return file_get_contents($this->file);
	}else{
	  return '';
	}

      case 'collect':  
	$this->dbg($this->state);
	$content = ob_get_contents();
	ob_end_clean();
	// there are cases when the cache is not welcome, say filling the form
	if ($this->keepCache()){

	  // cache the html page
	  system("mkdir -p ".dirname($this->file));
	  if ($fp = fopen($this->file, 'w')){
	    fwrite($fp, $content);
	    fclose($fp);
	  }else{
	    myPear::WARNING_T("can't write ".$this->file);
	  }
	  
	  // cache the inner css & js definitions
	  if (!empty(self::$cmsCache)){
	    system("mkdir -p ".dirname($this->file));
	    if ($fp = fopen($this->file.'.cms', 'w')){
	      fwrite($fp, join("\n",self::$cmsCache)."\n");
	      fclose($fp);
	    }else{
	      myPear::WARNING_T("can't write ".$this->file.'.cms');
	    }
	  }
	}else{
	  b_os::unlink($this->file);
	}
	$this->dbg($this->state.' - '.$this->file);
	return $content;

      default:
	$this->dbg('cache is off');
	return '';
      }
    }
  }
  
  /*
   * Propose to refresh the cache, if such is active
   */
  static function refreshButton(){
    if (!self::$cacheEmpty) return bIcons()->getButton(array('i'=>'i-refresh',
							     'd'=>'refresh the page',
							     'l'=>b_url::same('?resetcache_once=1'),
							     'class'=>'page_load_progress'));
    else return '';
  }

  public static function emptyCache($force=False){
    if (self::$cachePosible && ($force || bAuth::authenticated())){
      $cmd = 'rm -rfv '.self::$cacheArea_auth.'/* 2>&1; chmod -R a+rw '.self::$cacheArea_auth.' 2>&1';
      $out = trim(`$cmd`);
      if (cnf_dev && !empty($out)) myPear::DEBUG(__METHOD__."($cmd)<br/>".x('pre',$out));
    }
  }

}
