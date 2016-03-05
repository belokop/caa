<?php
/*                2002 Iouri Belokopytov, 1-st version
 *                2010 Iouri Belokopytov, 2-nd version, update for MYSQLI
 *                2014 Iouri Belokopytov, 3-rd version, rewrite for AN 
 *                2015 Iouri Belokopytov, Adding transaction
 */

class bDB extends mysqli{

  private $isReady = False;

  public $Database= '?';
  public $Host    = '?';

  protected $result = Null;
  protected $charset= '?';

  public static $_debug   = False; 
  public        $debug   = False; 
  protected     $debug_changes = False;
  protected     $debug_qquery = False;
  
  /*
   *
   */
  function __construct($host=Null,$user=Null,$passwd=Null,$dbname=Null){

    $this->debug = self::$_debug || $this->debug || (class_exists('b_cnf',0) ? b_cnf::get('dbdebug') : False);

    foreach(array('host','user','passwd','dbname') as $arg){
      if (empty($$arg) && isset($GLOBALS[get_class($this)."_$arg"])){
	$$arg = $GLOBALS[$var=get_class($this)."_$arg"];
	$this->dbg("set $var='".$$arg."'");
      }
    }

    parent::__construct($host,$user,$passwd,$dbname);
    
    if (mysqli_connect_error()) {
      // Use mysqli_connect_error instead of $this->connect_error if you need to ensure
      // compatibility with PHP versions prior to 5.2.9 and 5.3.0.
      if (cnf_dev) b_debug::traceBack(mysqli_connect_error());
      die('Connect Error (' . mysqli_connect_errno() . ') ' . mysqli_connect_error());
    }else{
      $this->isReady = True;

      // Extract the true server name
      $this->Host = $host;
      $q = $this->query("SHOW VARIABLES LIKE 'hostname'");
      while($r = $this->next_record($q)){
	if ($r['Variable_name'] == 'hostname') $this->Host = $r['Value'];
      }

      // Extract the other useful variables
      $this->Database= $dbname;
      $this->charset = $this->character_set_name();
      $this->debug_changes = (defined('cnf_show_db') && cnf_show_db);
      $this->dbg(sprintf("user = %s,  database = %s@%s", $user, $this->Database, $this->Host),
		 $this->debug_changes || (defined('YBhere') && YBhere));
      $this->dbg("character_set_name = '$this->charset'",$this->debug_changes || (defined('YBhere') && YBhere));
    }
    $this->dbg("__construct done, isReady=".var_export($this->isReady,True));
  }

  function isReady(){
    $reply = (bool)$this->isReady;
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Important for InnoDB
   */
  protected $in_autocommit = False;
  function noAutoCommit($mode='end'){
    $this->dbg('',cnf_dev,'greenText');
    switch($mode){
    case 'start':
      $this->in_autocommit = True;
      $this->query('SET autocommit=0');
      break;
      
    case 'end':
      $this->query('COMMIT');
      $this->query('SET autocommit=1');
      $this->in_autocommit = False;
    }
  }
  
  /*
   * Naive transaction handler
   */
  protected $in_transaction = 0;
  function transaction($mode='start',$msg=''){
    $this->dbg('',cnf_dev,'greenText');
    switch($mode){
    case 'start':
      if ($this->in_transaction++) b_debug::internalError("Recursive transactions");
      bTiming()->cpu(__function__);
      $reply = $this->query('START TRANSACTION');
      break;

    case 'end':
      $reply = $this->query('COMMIT');
      $this->in_transaction--;
      bTiming()->cpu();
      break;

    default:
      if (!$this->in_transaction) b_debug::internalError("Rollback request, but not in transaction");
      b_debug::traceBack($msg);
      $reply = $this->query('ROLLBACK');
      break;
    }
    return $reply;
  }
  
  /*
   *
   */
  private $last_query = '';
  function query($query,$stop_if_fails=True){

    $looking_for_performance = False;

    if (!$this->isReady)   return Null;

    // Lock during the transaction
    if ($this->in_transaction && stripos($query,'select ') !== False){
      $query .= ' FOR UPDATE';
      // $this->debug_qquery = cnf_dev;
    }

    // Timing...
    if (strpos($query,' ') === False) $queryType = $query;
    else list($queryType,$body) = explode(' ',$query,2);
    if ($do_bTiming=function_exists('bTiming')) bTiming()->cpu('q'.strToUpper($queryType));

    // Debugging (RnD if you want...)
    if (cnf_dev || YBhere){
      $actions = array('ALTER','UPDATE','DELETE','INSERT','DROP','CREATE','START','COMMIT','ROLLBACK');
      if (empty($this->debug_qquery)){
	$this->debug_qquery = $looking_for_performance;
      }elseif(cnf_dev){
	if (in_array(strToUpper($queryType),$actions) 
	    || (stripos($query,' join ') !== False) 
	    || (stripos($query,'COMMIT') !== False)
	    )    $this->debug_qquery = True;
      }
      if ($looking_for_performance && in_array(strToUpper($queryType),$actions)){
	static $tttuuuq = 0;
	if ($tttuuuq++ < 6){$this->debug_qquery = True;  b_debug::traceBack($query);}
      }
    }
    
    $this->last_query = $query;
    $this->result = parent::query($query);
    $this->dbg_query($query);
    if ($this->result === False){
      if ($this->in_transaction) return $this->transaction('rollback',$this->error);
      if ($stop_if_fails) b_debug::internalError($this->error);
    }else{
      
      // Reset the keys cache if the query changes number of rows in the table                                                                                                                                 
      if (class_exists('myPear',False) && in_array(strToUpper($queryType),array('DROP','INSERT','DELETE','TRUNCATE','CREATE','ALTER'))){
	$res_saved = $this->result;
	$this->reset_cache();
	$this->result = $res_saved;
      }
    } 
    if ($do_bTiming) bTiming()->cpu();
    return $this->result;
  }

  /* 
   * Same as 'query', but with a flexible debug level
   */
  function qquery($query,$debug=False){
    if (!$this->isReady)   return Null;
    $this->debug_qquery = (bool)$debug;
    return $this->query($query);
  }


  /*
   *
   */
  function next_record($res,$method=MYSQLI_ASSOC){
    if (!$this->isReady)   return Null;
    if (empty($res)){
      if (!is_bool($res)) if(cnf_dev) b_debug::internalError("Empty result object - ".$this->last_query);
      return array();
    }
    if (function_exists('bTiming')) bTiming()->cpu('qNEXT');
    if (is_object($res)){
      $reply = $res->fetch_array($method);
    }elseif (is_bool($res)){
      if (cnf_dev) b_debug::traceBack("res is boolean ".($res ? 'TRUE' : 'FALSE'));
      $reply = array();
    }else{
      b_debug::internalError('result is not an object, but '.var_export($res,True));
    }
    $this->unquote($reply);
    if (function_exists('bTiming')) bTiming()->cpu();
    return $reply;
  }

  /*
   * Escape the 'value' and construct a query element "<key> <operator> <escaped value>" 
   */
  function quote($str,$toEscape=False,$key='',$already_tidy=False) {
    
    if (is_string($str))       $str = $str_orig = trim($str);
    else                              $str_orig = $str;
    $key = trim($key);

    $explicit     = (is_string($toEscape) && in_array($toEscape,array('!=','<','>','<=','>=')));
    $negate       = ($toEscape === 'N');
    $regexp_strict= ($toEscape === 'R');
    $regexp       = (strpos($toEscape,'R') === 0);

    // Prepare a search string for REGEXP if requested.
    // Escape non-ascii characters by '.*' sequence
    if ($str !== Null){
      if(($regexp_strict || $regexp) && !$already_tidy){
	static $regexp_escapes = array('_DSTR_'   => '.*', '_UPPSE_'  => '^',
				       '_APOS_'   => "'",  '_SPACE_'  => ' ',
				       '_DOT_'    => '.',  '_DASH_'   => '-',
				       '_SNABELA_'=> '@',  '_VERTI_'  => '|',
				       '_SBRO_'   => '[',  '_SBRC_'   => ']',
				       );
	$str = mb_strToLower($str,'UTF-8');
	foreach ($regexp_escapes as $t=>$f) $str = str_replace($f,$t,$str);
	$str = preg_replace('/\W+/u','.*',$str);
	foreach ($regexp_escapes as $f=>$t) $str = str_replace($f,$t,$str);
	if ($regexp) foreach(array('?' => '\\?',
				   '\\\\'=>'\\') as $f=>$t) $str = str_replace($f,$t,$str); 
      }
      
      if (!empty($toEscape))    $str = $this->real_escape_string($str);
      $str = trim(str_replace("'", "\\'", str_replace("\\", "\\\\", $str)));
      if    ($str === 'NULL')        $str = Null;
      elseif($regexp_strict)         $str = '[[:<:]]'.$str.'[[:>:]]';
      elseif(empty($key))            $str = "'$str'";
    }
  
    // Add the column name if requested
    if (!empty($key)){
      if ($str === Null){
	$str = "$key = NULL";
      }else{
	$inv_synonyms = $sub_str = $tokens = array();
	if($regexp && preg_match('/av_firstname|av_lastname/',$key)){
	  // LIKE
	  if (!$regexp_strict) $sub_str[] .= "CONVERT(`$key` USING utf8) LIKE '%$str%'";
	  // REGEXP
	  // First replace "ambiguous" symbols by tokens 
	  $synonyms = array('ä' => 'a',
			    'å' => 'a',
			    'á' => 'a',
			    'ö' => 'o',
			    'ø' => 'o',
			    'ü' => 'u',
			    'é' => 'e',
			    'ð' => 'd');
	  // Build the tokens table
	  foreach($synonyms as $a=>$b){
	    if (empty($inv_synonyms[$b])) $inv_synonyms[$b] = array($b);
	    $inv_synonyms[$b][] = $a;
	    $inv_synonyms[$a] = array($a,$b);
	  }
	  foreach($inv_synonyms as $f=>$t){
	    $tokens[$f] = b_crypt::hash(serialize($t));
	    $str = str_ireplace($f,$tokens[$f],$str);
	  }
	  // Second - replace tokens by the 'OR' list
	  foreach($tokens as $f=>$h){
	    $str = str_ireplace($h,'('.join('|',$inv_synonyms[$f]).')',$str);
	  }
	  if (strToLower($str) !== strToLower($str_orig)){
	    // myPear::WARNING_T(__FUNCTION__." converted REGEXP '$str_orig' -> '$str'");
	  }
	  $sub_str[] = "CONVERT(`$key` USING utf8) REGEXP '$str'";
	  
	  // Complete the query string
	  $str = join(' OR ',$sub_str);
	  if (count($sub_str) > 1) $str = " ( $str ) ";
	}else{
	  if ($negate)        $operator = '!=';
	  elseif ($explicit)  $operator = $toEscape;
	  elseif ($regexp)    $operator = 'REGEXP';
	  else                $operator = '=';
	  $str = "`$key` $operator '$str'";
	}
      }
    }else{
      if(($str === Null) || strToLower($str)==='null') $str = 'NULL';        
      elseif((strpos($str,"'") === False) && 
	     !((int)$str && is_numeric($str)))         $str = "'$str'";
    }
    return $str;
  }

  /*
   *
   */
  private function unquote(&$text_array) {
    static $quotes = array("\\\\"  => "\\", 
			   "\\'"   => "'",
			   '\\"'   => '"',
			   '\\r\\n'=>"\n",
			   '\\n'=>"\n",
			   );
    if (is_array($text_array)){
      foreach(array_keys($text_array) as $k){
	if (($text_was=$text_array[$k]) === Null) continue;
	$text_array[$k] = b_crypt::encode_utf8($text_array[$k]);
	if (False) if ($d=b_fmt::diffHTML($text_was,$text_array[$k])) myPear::DEBUG(x("span class='lgreenText align-left'",__METHOD__.'(encode_utf8): '.x('em',$d)));
	foreach ($quotes as $f=>$t){
	  if (strpos($text_array[$k],$f) !== False)  $text_array[$k] = str_replace($f,$t,$text_array[$k]);
	}
      }
    }
  }

  /*
   * Check if the value fits into the column
   */
  public $dataFits = '';
  function dataDoesNotFit($table,$column,$value){
    $type = $this->getColumnType($table,$column,True);
    if(stripos($type,'varchar') !== False){
      $size = preg_replace('/\D/','',$type);
    }elseif($type == 'tinytext'){
      $size = 256;
    }elseif($type == 'text'){
      $size = 65535;
    }else{
      $size = 10;
    }
    $need = strlen($value);
    $have = (int)(0.95 * $size); // some encodings cut the words
    if ($reply = ($need > $have)){
      $reply = "$column: need $need, but got only $have bytes";
    }
    return $reply;
  }

  /*
   *
   */
  function num_rows($res=Null){
    if (!$this->isReady)                  $reply = Null;
    elseif($res instanceof mysqli_result) $reply = $res->num_rows;
    else                                  $reply = $this->affected_rows;
    return $reply;
  }

  /*
   *
   */
  function tableExists($table){
    $reply = in_array($table,$this->getTables());
    $this->dbg($reply);
    return $reply;
  }

  function tableNotEmpty($table) {
    static $status = array();
    if ($table === 'reset') $status = array();
    if (empty($status)){
      $q = $this->query("SHOW TABLE STATUS");
      while ($r=$this->next_record($q)) $status[$r['Name']] = $r['Rows'];
    }
    $reply = !empty($status[$table]);
    return $reply; 
  }

  function getTables(){
    if (!$this->isReady)   return array();;
    if (($cache = $this->cache($id=__FUNCTION__)) === Null){
      $reply = array();
      $q = $this->query("SHOW tables");
      while ($r = $this->next_record($q,MYSQLI_BOTH)) $reply[] = $r[0];
      $cache = $this->cache($id,$reply);
    }
    $this->dbg($cache);
    return $cache;
  }

  /*
   *
   */
  function truncateTable($table){
    if ($this->tableExists($table)){
      $this->qquery("TRUNCATE TABLE `$table`",cnf_dev);
      $this->reset_cache();
    }
  }

  /*
   *
   */
  private $showIndex = array();
  function showIndex($table){
    if (empty($this->showIndex[$table])) $this->showIndex[$table] = array();
    if ($this->tableExists($table) && empty($this->showIndex[$table])){
      $q = $this->query("SHOW INDEX FROM $table");
      while($r = $this->next_record($q)) if (!empty($r['Key_name']))$this->showIndex[$table][] = $r['Key_name'];
    }
    return array_unique($this->showIndex[$table]);
  }

  /*
   *
   */
  function columnExists($column,$table=Null){
    if (empty($table))     $table = $this->getTables();
    if (!is_array($table)) $table = array($table);
    foreach($table as $t){
      $reply = in_array($column,$this->getColumns($t));
      if ($reply) break;
    }
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   *
   */
  function getColumns($table) {
    if (!$this->isReady)   return array();
    if (($cache = $this->cache($id=__FUNCTION__.$table)) === Null){
      $reply = array();
      if ($this->tableExists($table)){
	$q = $this->query("SHOW COLUMNS FROM $table");
	while($r = $this->next_record($q)) $reply[] = $r['Field'];
      }
      $cache = $this->cache($id,$reply);
    }
    $this->dbg($cache);
    return $cache;
  }

  private $showColumns = array();
  function showColumns($table){
    if(empty($this->showColumns[$table])){
      $q = $this->query("SHOW COLUMNS FROM $table");
      while($r = $this->next_record($q)) $this->showColumns[$table][$r['Field']] = $r;
    }
    return $this->showColumns[$table];
  }

  /*
   *
   */
  function getColumnType($table,$column,$keep_all=False) {
    if (!$this->isReady)   return Null;
    if (is_string($keep_all)){
      $what = $keep_all;
      $keep_all = False;
    }else{
      $what = 'Type';
    }

    $id = __FUNCTION__.$table;
    if (($cache = $this->cache($id)) === Null){
      $q = $this->query("SHOW COLUMNS FROM $table");
      while($r = $this->next_record($q)) $c[$r['Field']] = $r;
      $cache = $this->cache($id,$c);
    }
    return $cache[$column][$what];
  }

  function getColumnType_OBSOLETE($table,$column,$keep_all=False) {
    if (!$this->isReady)   return Null;

    $id = __FUNCTION__.$table.$keep_all;
    if (($cache = $this->cache($id)) === Null){
      $c = array();
      $q = $this->query("SHOW COLUMNS FROM $table");
      while($r = $this->next_record($q)){
	$c[$id][$r['Field']] = $keep_all ? $r['Type'] : preg_replace('/\(.*/','',$r['Type']);
      }
      $cache = $this->cache($id,$c[$id]);
    }
    $this->dbg($cache[$column]);
    return $cache[$column];
  }
  
  /*
   *
   */
  function reset_cache(){
    $this->cache('reset');
  }

  /*
   *
   */
  protected function cache($id,$action='get'){
    if ($id === 'reset') $action = 'reset';
    switch($action){
    case 'reset':
      // reset the cache
      $_SESSION[get_class($this)] = array();
    case 'get':
      // asking for the cache info
      break;
    default:
      // A new value provided, store it
      $_SESSION[get_class($this)][$id] = $action;
    }
    return @$_SESSION[get_class($this)][$id];
  }

  /*
   *
   */
  protected function dbg_query($query){

    if ($this->result === False){
      $this->dbg(x('strong',' ??? query failed '.$this->error),cnf_dev,'redText',3);
      return;
   }

    // Drop the locks info
    //    if (!$this->debug && (stripos($query,'zzz_semaphore') !== False)) return;

    if (!$this->debug_qquery && 
	!$this->debug &&
	!$this->debug_changes) return;

    $cnf_show_db = False;
    $n = $this->num_rows();
    $c = 'lgreenText';
    $cmt = '';
    if(preg_match('/^(transaction|commit|rollback)/i',$query)){
      $n = 999;
      $c = 'dgreenText';
    }elseif(!preg_match('/^(select|show)/i',$query)){
      if (stripos($query,'INSERT')!==False){
	$n = max($n,1);
	$cmt = (($id=$this->insert_id) ? "id=$id, " : "")."$n rows";
      }else{
	$cmt = $this->num_rows()." rows";
      }
      $c = 'dgreenText';
      $cnf_show_db = $this->debug_changes;
    }elseif(is_bool($this->result)){
      $n = 999;
      $cmt = '????????????????????????????????????????';
      $c = 'redText';
    }else{
      $cmt = "$n rows";
    }
    if ($n == 1) $cmt = preg_replace("/s$/","",$cmt);
    if ($this->debug || (($n > 0) && ($this->debug_qquery || $cnf_show_db))){
      $this->dbg(x("strong",' '.str_replace(' ','&nbsp;',$cmt)),True,$c,3);
    }
    $this->debug_qquery = False;
  }

  /*
   *
   */
  protected function dbg($text,$debug_force=False,$color='lgreenText',$level=2){
    if ($this->debug ||
	$this->debug_qquery ||
	$debug_force){

      if (is_bool($text))     $text = ($text ? 'YES' : 'NO');
      elseif ($text === Null) $text = 'NULL';
      elseif(is_array($text)) $text = join(', ',$text);
      myPear::DEBUG(x("span class='$color align-left'",
		      str_replace('bDB_myPear','db',
				  str_replace('bDB',get_class($this),b_debug::__($level)).x("em",$text))));
    }
  }
}

