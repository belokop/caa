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
  protected     $dbg_changes = False;
  protected     $dbg_qquery = False;
  
  /*
   *
   */
  function __construct($host=Null,$user=Null,$passwd=Null,$dbname=Null){

    if (class_exists('b_cnf',0) && b_cnf::get('dbdebug')) self::$_debug = True;

    foreach(array('host','user','passwd','dbname') as $arg){
      if (empty($$arg)){
	global $DBengine;
	$$arg = $GLOBALS[$DBengine][$arg];
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
      $q = $this->qquery("SHOW VARIABLES LIKE 'hostname'",cnf_dev||cnf_show_db);
      while($r = $this->next_record($q)){
	if ($r['Variable_name'] == 'hostname') $this->Host = $r['Value'];
      }

      // Extract the other useful variables
      $this->Database= $dbname;

      // $this->charset = $this->character_set_name();
      foreach(array('collation_',
		    'character_set_',
		    ) as $var){
	$q = $this->query("show variables like '$var%'");
	while($r = $this->next_record($q)){
	  MSG::DEBUG(sprintf("%s = '%s'",$r['Variable_name'],$r['Value']));
	}
      }
      
      $this->dbg_changes = (defined('cnf_show_db') && cnf_show_db);
      MSG::DEBUG(sprintf("user = %s,  database = %s@%s", $user, $this->Database, $this->Host),
		 $this->dbg_changes || (defined('YBhere') && YBhere));
    }
    MSG::DEBUG("__construct done, isReady=".var_export($this->isReady,True));
  }

  function isReady(){
    $reply = (bool)$this->isReady;
    $this->dbg($reply);
    return $reply;
  }

  /*
   * Important for InnoDB
   */
  public $in_autocommit = False;
  function noAutoCommit($mode='end'){

    if (!DB_autocommit){
      if(cnf_dev) MSG::WARNING('"SET AUTOCOMMIT" is ignored...');
      return;
    }

    $this->dbg('',array(cnf_dev,'greenText'));
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
  public $in_transaction = 0;
  function transaction($mode='start',$msg=''){
    $this->dbg('',array(cnf_dev,'greenText'));

    $reply = Null;
    switch($mode){
    case 'start':
      if ($this->in_transaction++ == 0){
	bTiming()->cpu(__function__);
	$reply = $this->query('START TRANSACTION');
      }else{
	$this->dbg(sprintf("Recursive transaction, depth %s, skip 'START TRANSACTION'",$this->in_transaction),cnf_dev);
	// b_debug::internalError("Recursive transactions");
      }
      break;
      
    case 'end':
      if ($this->in_transaction == 1){
	$reply = $this->query('COMMIT');
	bTiming()->cpu();
      }else{
	$this->dbg(sprintf("Recursive transaction, depth %s, skip 'COMMIT'",$this->in_transaction),cnf_dev);
      }
      $this->in_transaction--;
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
      // $this->dbg_qquery = cnf_dev;
    }

    // Timing...
    if (strpos($query,' ') === False) $queryType = $query;
    else list($queryType,$body) = explode(' ',$query,2);
    if ($do_bTiming=function_exists('bTiming')){
      if (strToLower($queryType)=='select'){
	bTiming()->cpu(strpos(trim($body),'*')===0 ? 'qSELECT*' : 'qSELECTx');
      }else{
	bTiming()->cpu('q'.strToUpper($queryType));
      }
    }

    // Debugging (RnD if you want...)
    if (cnf_dev || YBhere){
      $actions = array('ALTER','UPDATE','DELETE','INSERT','DROP','CREATE','START','COMMIT','ROLLBACK');
      if (empty($this->dbg_qquery)){
	$this->dbg_qquery = $looking_for_performance;
      }elseif(cnf_dev){
	if (in_array(strToUpper($queryType),$actions) 
	    || (stripos($query,' join ') !== False) 
	    || (stripos($query,'COMMIT') !== False)
	    )    $this->dbg_qquery = True;
      }
      if ($looking_for_performance && in_array(strToUpper($queryType),$actions)){
	static $tttuuuq = 0;
	if ($tttuuuq++ < 6){$this->dbg_qquery = True;  b_debug::traceBack($query);}
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
    $this->qquery_no_debug = ($debug === 'no');
    $this->dbg_qquery = (bool)$debug;
    $reply = $this->query($query);
    $this->qquery_no_debug = False;
    return $reply;
  }


  /*
   *
   */
  function next_record($res,$method=MYSQLI_ASSOC){
    if ($this->isReady){
      if (empty($res)){
	if (!is_bool($res)) if(cnf_dev) b_debug::internalError("Empty result object - ".$this->last_query);
	return array();
      }
      if (function_exists('bTiming')) bTiming()->cpu('qNEXT');
      $reply = (is_object($res)
		? $res->fetch_array($method)
		: (is_bool($res) 
		   ? array() 
		   : b_debug::internalError('result is not an object, but '.var_export($res,True))));
      $this->unquote($reply);
      if (function_exists('bTiming')) bTiming()->cpu();
    }
    return @$reply;
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
	    // MSG::WARNING_T(__FUNCTION__." converted REGEXP '$str_orig' -> '$str'");
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
			   '\\n'   =>"\n",
			   );
    if (is_array($text_array)){
      foreach(array_keys($text_array) as $k){
	if (($text_was=$text_array[$k]) === Null) continue;
	$text_array[$k] = str_replace(array_keys($quotes),
				      array_values($quotes),
				      b_crypt::encode_utf8($text_array[$k]));
	if (False) if ($d=b_fmt::diffHTML($text_was,$text_array[$k])) b_debug::_debug($d,array("lgreenText align-left",cnf_dev));
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
    $reply = in_array(trim($table),$this->getTables());
    if (!$reply) $this->dbg($reply);
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
    if (($cached = $this->cache(__FUNCTION__)) === Null){
      $reply = array();
      $q = $this->query("SHOW tables");
      while ($r = $this->next_record($q,MYSQLI_BOTH)) $reply[] = $r[0];
      $cached = $this->cache(__FUNCTION__,$reply);
      $this->dbg($cached);
    }
    return $cached;
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
    if (($cache = $this->cache($id=__FUNCTION__.".$table")) === Null){
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

    $id = __FUNCTION__.".$table";
    if (($cache = $this->cache($id)) === Null){
      $q = $this->query("SHOW COLUMNS FROM $table");
      while($r = $this->next_record($q)) $c[$r['Field']] = $r;
      $cache = $this->cache($id,$c);
    }
    return $cache[$column][$what];
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
    if ($id === 'reset'){
      $action = 'reset';
    }

    $id_cache = get_class($this);
    switch($action){
    case 'reset':      // reset the cache
      $_SESSION[$id_cache] = array();
      $this->dbg("Resetting cache",True);
      break;
    case 'get':        // asking for the cache info
      break;
    default:           // A new value provided, store it
      $_SESSION[$id_cache][$id] = $action;
      $this->dbg($action);
    }
    $reply = @$_SESSION[$id_cache][$id];
    //    $this->dbg($reply,True);
    return $reply;
  }

  /*
   *
   */
  protected function dbg_query($query){

    if ($this->result === False){
      $this->dbg(x('strong',' ???? query failed '.$this->error."<br>$query"),array(cnf_dev,'redText'));
      return;
    }

    if ($this->qquery_no_debug) return;
    if (!self::$_debug &&
	!$this->dbg_qquery && 
	!$this->dbg_changes) return;

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
      $cnf_show_db = $this->dbg_changes;
    }elseif(is_bool($this->result)){
      $n = 999;
      $cmt = '????????????????????????????????????????';
      $c = 'redText';
    }else{
      $cmt = "$n rows";
    }
    if ($n == 1) $cmt = preg_replace("/s$/","",$cmt);
    if (self::$_debug || (($n > 0) && ($this->dbg_qquery || $cnf_show_db))){
      b_debug::_debug(x("strong",' '.str_replace(' ','&nbsp;',$cmt)),array($this,True,$c,2));
    }
    $this->dbg_qquery = False;
  }

  /*
   *
   */
  private $qquery_no_debug = False;
  protected function dbg($text,$flags=array()){
    if(!$this->qquery_no_debug){
      if (!is_array($flags)) $flags = array($flags);
      foreach($flags as $f){
      if    (is_bool($f)) $debug_force = $f;
      elseif(is_numeric($f)) {}
      elseif(is_string($f)) $color = $f;
      }
      if (!isset($debug_force)) $debug_force = False;
    if (!isset($color)      ) $color = 'lgreenText';
    
    b_debug::_debug($text,array($color,2,$this,(static::$_debug || $this->dbg_qquery || $debug_force)));
    }
  }

  /*
   *
   */
  function __toString(){
    return get_class($this);
  }
}
      
