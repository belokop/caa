<?php

define('CONST_wic_lastMinutes', 10); // show users as active for the last "CONST_wic_lastMinutes" minutes

locateAndInclude("bHolder");

class bHolder_Logs extends bHolder{

  public static $defaults = array('login' => True,
				  'merge' => False);
  
  var $debug = False;
  var $maxLines = 150;
  var $sortBy_default = 'log_time';
  var $cache = Null;
  var $showLineCounter = False;

  public $doNotLog = False;

  private $av = Null;
  private $doing = 'syslog';

  protected $widget_align= 'align_left';

  
  /*
   * Who Is Connected
   */
  public function wic(){
    if (cnf_CLI || !myPear_db()->columnExists('log_org','zzz_logs')) return;

    $_GET['doing_once'] = CONST_wic;
    myPear::H1('Who is connected','reset');

    locateAndInclude('b_tabs');
    $b_tabs = new b_tabs_JQ(array('Now','Last hour','Since 24 hours'));
    foreach(array(0,60,1440) as self::$wic_since){
      $b_tabs->body();
      print myPear_logs()->getWidget();
    }
    $b_tabs->close();
  }
  
  /*
   * Write an entry to the server system log 
   */
  public function syslog($text){
    static $dejaVu = array(); 
    if (!cnf_CLI && class_exists('bAuth',False) && !@$dejaVu[$text]++){
      $user = (is_object(bAuth::$av) ? bAuth::$av->getEmail() : 'anonym');
      openlog(sprintf('webapp[%s,%s]',b_reg::$current_module,$user), LOG_PERROR, LOG_USER); 
      syslog (LOG_ALERT, str_replace(array("  ","\n",'\\\n','\\\N')," ", b_fmt::unEscape(strip_tags($text))));
      closelog();
    }
  }


  /*
   * Write a log record to the database
   */
  public function add($log_type='login',$log_form=0,$log_comment='',$log_avid=Null,$module=Null,$unique=False){
    
    if (cnf_CLI || $this->doNotLog) return;
    
    if (empty($module)) $module = b_reg::$current_module;
    if (!in_array($module,bMaster()->get_masterModules())){
      b_debug::xxx("Refuse module '$module'");
      return;
    }

    if ($log_avid===Null){
      if (is_object(bAuth::$av_authenticated))  $log_avid = bAuth::$av_authenticated->ID;
      else                             $log_avid = 0;
    }

    foreach (array('log_ip'    => @$_SERVER['REMOTE_ADDR'],
		   'log_avid'  => $log_avid,
		   'log_form'  => $log_form,
		   'log_time'  => time(),
		   'log_type'  => $log_type,
		   'log_api'   => $module,
		   'log_org'   => myOrg_ID,
		   'log_comment'=>$log_comment,
		   ) as $k=>$v){
      if (myPear::is_empty($v)) continue;
      $set[] = myPear_db()->quote($v,1,$k);
      if (in_array($k,array('log_type','log_form','log_comment'))) $where[] = myPear_db()->quote($v,1,$k);
    }
    
    // skip duplicated entries if required
    if ($unique){
      $q = myPear_db()->query("SELECT log_id FROM zzz_logs WHERE ".($m=b_fmt::joinX(' AND ',$where,"'")));
      if (myPear_db()->num_rows($q)){
	myPear::WARNING_T("SKIP LOG RECORD \"$m\"");
	return;
      }
    }

    // add the record    
    myPear_db()->query("INSERT INTO zzz_logs SET ".join(',',$set));
  }
  
  /*
   * It simply check who has visited a page in the last 5 minutes 
   */
  static $wic_since = 0;
  public function who_is_connected(){
    static $t_users_connected = array();

    if (empty(self::$wic_since)) self::$wic_since = CONST_wic_lastMinutes;
    if (empty($t_users_connected[self::$wic_since]))   $t_users_connected[self::$wic_since] = array();
    return $t_users_connected[self::$wic_since];  
    
    
    // The updates might not be applied yet, wait for them
    if ($ok = (!cnf_CLI && bAuth::authenticated())){
      if (myPear_db()->isReady()){
	$log_avid = (is_object(bAuth::$av_authenticated) ? bAuth::$av_authenticated->ID : 0);
	foreach(array('log_avid'=>$log_avid,
		      'log_type'=> CONST_wic,
		      'log_ip'  => @$_SERVER["REMOTE_ADDR"],
		      'log_org' => myOrg_ID,
		      'log_api' => b_reg::$current_module,
		      ) as $column=>$value){
	  if (!myPear_db()->columnExists($column,'zzz_logs')) $ok = False;
	  if (!empty($v)) $id[] = myPear_db()->quote($value,True,$column);
	}
      }
      
      // Check who is there
      if (empty($t_users_connected[self::$wic_since])){
	
	$verbose = False;
	
	// Check that the current user has ever logged in. It is enought to do it once per session
	if (empty($id)) $id = array(0);
	if (!@$_SESSION[__METHOD__.$log_avid]++){
	  $q = myPear_db()->qquery("SELECT * FROM zzz_logs WHERE ".join(' AND ',$id),$verbose);
	  if (!myPear_db()->num_rows($q)) myPear_db()->qquery("INSERT INTO zzz_logs SET ".join(',',$id),$verbose);
	}
	
	// Update the current activity
	myPear_db()->qquery("UPDATE zzz_logs SET ".join(',',array("log_time = ".time(),
								  myPear_db()->quote(date('Y-m-d H:i:s'),True,'timestamp'),
								  myPear_db()->quote(b_fmt::unEscape(is_object(bAuth::$av_authenticated) 
												     ? bAuth::$av_authenticated->fmtName('Lf') 
												     : 'Anonymous'),
										     True,
										     'log_comment'))) . " WHERE ".join(' AND ',$id),$verbose);
	// Now see who is there
	$q = myPear_db()->qquery("SELECT * FROM zzz_logs ".
				 " WHERE log_type = '".CONST_wic."' AND log_time > ".(time()-self::$wic_since*60).
				 " ORDER BY log_time DESC",$verbose);
	$dejaVu = array(); 
	while($r = myPear_db()->next_record($q)){  
	  if (!@$dejaVu[$r['log_ip']]++)  $t_users_connected[self::$wic_since][] = $r;        
	} 
      }
    }
    return @$t_users_connected[self::$wic_since];  
  }

  /*
   * Write a authenticated login as a simplified log record into a myPear custom server log
   */
  public function add_access() { 
    static $logfile = '/log/www/log';
    if (is_object(bAuth::$av_authenticated) && file_exists($logfile)){
      $user = bAuth::$av_authenticated->getEmail();
      
      list ($OS,$browser) = b_fmt::parse_HTTP_USER_AGENT();
      
      if (!@$_SESSION["add_access.$user".b_reg::$current_module]++){
	if ($handle = fopen ($logfile,'a')) {
	  $reg = b_reg::$modules[b_reg::$current_module];
	  fwrite($handle, sprintf ("%s %5s %-10s %-35s %-15s %-17s %-28s %s\n",
				   b_time::strftime("%Y-%m-%d %H:%M",time()),
				   $reg['m'],
				   $reg['v'],
				   $user,
				   $_SERVER['REMOTE_ADDR'],
				   "$OS",
				   "$browser",
				   $_SERVER['HTTP_HOST']));
	  fclose($handle);
	}
      }
    }
  }

  
  /*
   *
   */
  function listEntries(){
    $this->init_logs();
    $sv = bText::$acceptNewEntries;
    bText::$acceptNewEntries = False;
    return parent::listEntries();
    bText::$acceptNewEntries = $sv;
  }

  /*
   *
   */
  private function init_logs(){
        
    $this->cache = new b_cache_S($this);
    if (@$_GET['doing_once'] === CONST_wic)  $this->doing = CONST_wic;
    else                                     $this->doing = 'syslog';
    
    switch($this->doing){
    case CONST_wic:
      $this->noSort = True;
      $this->where[] = 1;
      $this->maxLines = 1;
      break;
      
    case 'syslog':
      if (!empty($_GET['av_id'])){
	$this->av = myPear::getInstance_new(bForm_Avatar::$bForm_Avatar, $_GET['av_id'],array('nocache','strict'));
      }else{
	locateAndInclude('b_checkBox');
	
	$l24 = '24_hours';
	$somethingSelected = b_checkBox::whatIsOn();
	if (!$somethingSelected && empty($this->av))  b_checkBox::set($l24,'on');
	
	//
	// Collect the types of the log records prepare the type selection pane (line of checkboxes).
	//
	$where[] = "log_api = '".b_reg::$current_module."'";
	foreach(array_keys(self::$defaults) as $w) $where[] = "log_type = '$w'";
	foreach(array(CONST_wic,'mail') as $no)    $where_no[] = "log_type != '$no'";

	$cb = array();
        $l  = myPear_db()->qquery("SELECT log_type FROM zzz_logs WHERE ".join(' AND ',$where_no)." AND (".join(' OR ',$where).") GROUP BY log_type ORDER BY log_type",cnf_dev);
	while ($r = myPear_db()->next_record($l)) {
	  $cb[$r['log_type']] = $r['log_type'];
	  $r['log_type'] = preg_replace('/\W/','',$r['log_type']);
	  if (!b_checkBox::isOn($r['log_type'])) b_checkBox::set($r['log_type'],'off');
	  if (!$somethingSelected && @self::$defaults[$r['log_type']])  b_checkBox::set($r['log_type'],'on');
	}
	$cb[$l24] = $l24;
	$this->title = 'Access log';
	$this->title2 = b_checkBox::create($cb,'&nbsp;');
      }
    }
  } 
  
  function header(){
   $header = array('log_time'  => 'when',
		   'log_ip'    => 'host',
		   'log_avid'  => 'who',
		   'log_type'  => 'what',
		   'log_form'  => '',
		   'log_comment'=>'',
		   );
   if ($this->doing == CONST_wic){
     unset($header['log_form']);
     unset($header['log_type']);
     unset($header['log_comment']);
   }

   if ($this->av)  $this->title3 = '<br/><br/>'.x('h2',$this->av->name('Lf').' access log');

   return $header;
  }
  
  /*
   *
   */
  function query() {
    locateAndInclude('b_checkBox');

    $left_join = array();
    $left_join[] = " LEFT JOIN zzz_avatars ON log_avid = av_id ";

    if (!bAuth::authenticated()){
      $where[] = '0';
    }elseif (is_object($this->av) || !myPear::is_empty($av_id = @$_GET['av_id'])){
      if (empty($av_id)) $av_id = $this->av->ID;
      $where[] = " ( log_avid = '$av_id' OR log_form = '$av_id' ) ";
    }elseif (!empty($this->where)){
      $where = $this->where;
    }else{
      $where = array(1);
      $types = array();
      foreach (b_checkBox::whatIsOn() as $type) $types[] = myPear_db()->quote($type,1,'log_type');
      if ($types)                         $where[] = ' ( '.join(' OR ',$types).' ) ';
      if (b_checkBox::isOn('24_hours'))   $where[] =  'log_time > UNIX_TIMESTAMP()-24*3600';
    }
    if (!superUser_here) $where[] = 'log_org = '.myOrg_ID;

    if ($_GET['sBy'] != 'log_time')  $_GET['sBy'] .= ',log_time';
    $_GET['sByDir'] = 'DESC';

    $q = myPear_db()->qquery("SELECT av_lastname,av_firstname,log_api,log_avid,log_comment,log_form,log_id,log_ip,log_org,log_time,log_type FROM zzz_logs ".
			     join(' ',$left_join).
			     " WHERE ".join(' AND ',$where).
			     " ORDER BY $_GET[sBy] $_GET[sByDir] LIMIT ".$this->maxLines,$this->debug);
    return $q;
  }
  
  /*
   * Restrict the log to the current module
   */
  private $nRecords = 0;
  function rejectRecord(){ 
    return 
      ($this->nRecords++ > $this->maxLines*2) ||
      (b_reg::$current_module != $this->rec['log_api'] && !superUser_here);
  }

  private $n_printed = 0;
  function format() {
    switch($this->doing){
    case CONST_wic:
      if (!$this->n_printed++){
	$recs = self::who_is_connected();
	foreach($recs as $rec) {
	  $this->rec = array_merge($rec,myPear_db()->fetchAssoc("SELECT * FROM zzz_avatars WHERE av_id = '".$rec['log_avid']."'"));
	  $this->format_syslog();
	  if (empty($rec['log_avid']))  $this->rec['log_avid'] = $rec['log_comment'];
	  $this->t->prt($this->rec);
	}
      }
      $this->rec = array();
      break;
      
    case 'syslog':
      $this->format_syslog();
    }    
  }    
  
  /*
   *
   */  
  private function format_syslog(){
    
    if (is_object($this->t)) $this->t->skipDuplicatedLines = True;
    foreach(array_keys($this->header()) as $k) if (empty($this->rec[$k])) $this->rec[$k] = '';
    
    $this->rec['log_ip'] = $this->ip2name($this->rec['log_ip']);
    
    $this->rec['log_avid'] = x('em',bForm_Avatar::_fmtName('Lf',$this->rec));

    $this->rec['log_comment'] = x('em',strip_tags($this->rec['log_comment']));
    
    if ($this->doing == CONST_wic) $this->rec['log_time'] = b_time::short_date_time($this->rec['log_time']);
    else                           $this->rec['log_time'] = b_checkBox::isOn('24_hours')
				     ? x('i',date("Y-m-d H:00",$this->rec['log_time']))
				     : x('i',date("Y-m-d",     $this->rec['log_time']));
    
    /*
    // carefull with the modules which are not initialised
    if (b_reg::$current_module == $this->rec['log_api'] && !empty($this->rec['log_form'])){
      $form = myPear::getClassFromID(($id=$this->rec['log_form']),False,True);
      if (stripos($form,'bForm') !== False){
	$class = myPear::getInstance_new($form,$id,'fatal');
	$name = x('em',str_replace('bForm_','',$class->name('Lf')));
	$this->rec['log_form'] = ($form == 'bForm_vm_Lease'
				  ? $name
				  : $this->rec['log_form'] = bJS()->popup($name,"?".b_crypt_no."=1&info=yes&mode=RO&form=$form&id=$id"));
      }else{
	if (empty($form)) $form = 'ID';
	if (is_numeric($f=$this->rec['log_form']) && ($f>0)) $this->rec['log_form'] = x('em',"${form}_${f}");
	else $this->rec['log_form'] = x('em',preg_replace('; from=.*;',' ',str_replace('to=','',$this->rec['log_comment'])));
      }    
    }
*/

    b_reg::load_module($this->rec['log_api']);
    $form = myPear::getClassFromID(($id=$this->rec['log_form']),False,True);
    if (stripos($form,'bForm') !== False){
      $class = myPear::getInstance_new($form,$id,'fatal');
      $name = x('em',str_replace('bForm_','',$class->name('Lf')));
      $this->rec['log_form'] = ($form == 'bForm_vm_Lease'
				? $name
				: bJS()->popup($name,"?".b_crypt_no."=1&info=yes&mode=RO&form=$form&id=$id"));
    }elseif(myPear::is_int($f=$this->rec['log_form'])){
      $this->rec['log_form'] = x('em',"${form}_${f}");
    }else{
      $this->rec['log_form'] = x('em',preg_replace('/ from=.*/',' ',str_replace('to=','',$this->rec['log_comment'])));
      $this->rec['log_comment'] = '';
    }    

    
    // Show the module&organization icons 
    if (is_object($this->t)){
      $this->t->extraTD[] = myOrg($this->rec['log_org'])->getFavicon();
      if (in_array($this->rec['log_api'],bMaster()->get_masterModules())){
	$this->t->extraTD[] = bIcons()->get(@b_reg::$modules[$this->rec['log_api']]);
      }
      $this->t->extraTD = array_unique($this->t->extraTD);
    }
  }
  
  private function ip2name($ip) {
    if (empty($ip)) return '&nbsp;';

    if ($this->cache->wasSet($ip)){
      return $this->cache->get();
    }else{
      $a = preg_split('/\./',gethostbyaddr($ip));
      while (count($a) > 4) array_shift($a);
      if (strlen(join('.',$a))>16 && count($a)>2) array_shift($a);
      if (strlen(join('.',$a))>16 && count($a)>2) array_shift($a);
      $reply = strToLower(join('.',$a));
      return $this->cache->set($reply);
    }
  }
}

