<?php
class bToolBox{
/*
 * Fixing various problems in the database
 */
  
  public $tools  = array();
  public $action = '';

  public $myPear_tools = array('MO'=>array('t'=>'Search for duplicated Avatar objects',
					   'd'=>'<p> bla bla </p>',
					   ),
			       'NN'=>array('t'=>'Clean-up people first/last names',
					   'd'=>'<p>While filling the applications forms people write their names in an arbitrary upper/lower case, sometimes include salutation (like "Professor)" in the name, etc.</p><p>This tool makes the names tidy</p>',
					   ), 
			       );
  
  /*
   *
   */
  function __construct(){
    $this->action = @$_GET['tb_act_once'];
    $this->tools['myPear'] = $this->myPear_tools; 
  }
  
  /*
   *
   */
  function getForm(){
    
    if (empty($this->action) && !isset($_GET['continue_once'])){
      b_url::setReferrer('bToolBox','reset');
      myPear::H1('Tool Box');
      foreach ($this->tools as $module=>$tools){

	print x('h2',"$module toolbox");
	b_fmt::accordion('open',999);     

	foreach($tools as $action=>$data){
	  if (isset($tools['condition']) && !$tools['condition']) continue;
	  b_fmt::accordion($tools[$action]['t']);	  
	  print $data['d'];
	  print x("a class='page_load_progress' href='".b_url::same("?resetcache_once=1&tb_act_once=$action")."'",
		  x('em','Click to '.strToLower($tools[$action]['t'])));
	}
	b_fmt::accordion('close');
      }
    }else{
      b_url::setReferrer('bToolBox');
      foreach ($this->tools as $module=>$tools){
	if (isset($tools[$this->action])){
	  myPear::H1($tools[$this->action]['t']);
	  set_time_limit(0);
	  call_user_func(array($this, "execute_$this->action"));
	  break;
	}
      }
    }
  }
  
  
  /*
   *
   */
  protected function execute_MO(){
    static $referrer = array();
    if (empty($referrer))  $referrer = $_GET;
    bForm_Avatar::findDuplicates();
  }

  /*
   *
   */
  public function execute_NN(){
    $db = myPear_db();
    $something_done = False;
    foreach(array('jam_old_avatars','jam_avatars','zzz_avatars') as $table){
      if ($db->tableExists($table)){
	$q = myPear_db()->query("SELECT * FROM $table WHERE 1 OR av_firstname REGEXP '^(Prof|Dr[\. ])'");
	$n = 0;	
	while($r = myPear_db()->next_record($q)){
	  $changes = $updates = array();
	  $av_salutation = $r['av_salutation']; 
	  foreach(array('av_firstname','av_lastname') as $name){
	    $$name = b_fmt::ucwords(trim($r[$name]),$catchMac=($name=='av_lastname'));
	    foreach(array('Prof.',
			  'Prof ',
			  'Professor',
			  'Dr.',
			  'Dr ',
			  ) as $s) if (stripos($$name,$s) === 0){
	      if (stripos($s,'prof') === 0)                                             $av_salutation = 'Prof.';
	      if (stripos($s,'dr')   === 0 && stripos($av_salutation,'prof') === False) $av_salutation = 'Dr.';
	      $$name = b_fmt::ucwords(trim(str_ireplace($s,'',$$name)));
	    }
	    if ($$name !== $r[$name]) $updates[$name]  = myPear_db()->quote($$name, True);
	  }
	  if (($av_salutation !== $r['av_salutation']) && myPear_db()->columnExists('av_salutation',$table)){
	    $updates['av_salutation']= myPear_db()->quote($av_salutation,True);
	  }
	  if (!empty($updates)){
	    if ($n++ == 0)	      myPear::MESSAGE(x('h3',"Cleaning $table"));
	    foreach($updates as $key=>$value) $changes[$key] = myPear_db()->quote($r[$key],True);
	    myPear_db()->query("UPDATE $table SET ".b_fmt::joinX(',',$updates)." WHERE av_id = $r[av_id]");
	    myPear::MESSAGE(join(', ',$changes).' ==> '.join(', ',$updates));
	    $something_done = True;
	  }
	}
      }
    }
    if (!$something_done) myPear::MESSAGE("No update needed, the names are already correct");
  }
}
  
