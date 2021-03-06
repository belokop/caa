<?php

// namespace taskManager;

define('taskManager_ID','taskManager_task');
define('TM_clean_log_time',7); // in days

class myPear_taskManager extends myPear_base{

  public $exec_request = False;
  public $debug = True;
  public $removeAfterExecution = False;

  /*
   * 
   */
  public function __construct(){

    // Eventually add a new task, i.e. put the following command into the task execution queue:
    //  - method $_REQUEST[addTask_class_once]->$_REQUEST[addTask_function_once]
    //  - arguments $_REQUEST[addTask_args_once]
    $addTask_class_once    = (string)@$_REQUEST['addTask_class_once'];
    $addTask_function_once = (string)@$_REQUEST['addTask_function_once'];
    $addTask_args_once     = (string)@$_REQUEST['addTask_args_once'];
    if (!empty($addTask_function_once)){
      if(empty($addTask_args_once)) $addTask_args_once = array();
      $this->addTask($addTask_class_once,
		     $addTask_function_once,
		     $addTask_args_once);
    }

    // Perform "delete" action, mark task as completed
    if ($taskID = @$_REQUEST['tm_delete_once']){
      $task = b_vars::get($taskID,$module = $_REQUEST['tm_m_once']);
      if (empty($task['end'])){
	$this->set($taskID,$task,array('cmt'=>'Removed','end'=>time()));
      }else{
	b_vars::delete($taskID,$module);
      }
      myPear_logs()->syslog("Explicitly remove ".$this->doing($task));
      MSG::WARNING("Removing ".$this->doing($task));
    }
  }

  /*
   * Wrapper around b_vars::set 
   * Split the data between 2 fields - var_value (method arguments) & var_value2 (all the rest)
   */
  private function set($taskID,$value,$value2,$module=Null){
    static $to_var_value = array('class',
				 'function',
				 'args',
				 );
  
    $var_value = $var_value2 = array();
    foreach(array_merge($value,$value2) as $k=>$v){
      if (in_array($k,$to_var_value)) $var_value[$k] = $v;
      else                            $var_value2[$k]= $v;
    }
    b_vars::set($taskID,$var_value,$module,$var_value2);
  }

  /*
   * Get summary information about the tasks in the queue
   */
  function info($clean_duplicates=False){

    $taskManager = b_vars::get(taskManager_ID,Null,True);
    if (empty($taskManager)){
      MSG::MESSAGE("There is no tasks in the queue for now");
    }else{
      $header=array('n'         =>'#',
		    '_cmt'      => ' ',
		    '_module'   => 'mod',
		    '_function' => 'task',
		    '_av'       => ' ',
		    '_start'    => 'start',
		    '_end'      => 'end',
		    '_av_dev'   => '',
		    );
      if (!cnf_dev && !cnf_demo && !YBhere) unset($header['_av_dev'],
						  $header['_module']
						  );

      // Print message if the lock is active
      $this->lock_isActive($release_if_deadlock=(@$_REQUEST['tm_clean_lock_once']==='yes'));
      
      // Create the table
      $t = new b_table_zebra($header);

      $dejaVu = array();
      $n_ended = $n_waiting = 0;
      ksort($taskManager);
      foreach($taskManager as $module=>$tasks){
	$module_was = '';
	foreach($tasks as $task){

	  if (empty($task['class'])) $task['class'] = '';
	  if (empty($task['end'])){
	    $task['_cmt'] = (($task['class']==='bUnit') || @$dejaVu[$this->doing($task,False).$module]++
			     ? x('strong',b_fmt::redText('dejaVu'))
			     : '');
	    $t->class = array();
	  }else{
	    foreach(array_keys($header) as $k)  $t->class[$k] = 'overstrike';
	    $t->extraTD[] = '';
	    $t->extraTD[] = b_btn::submit_icon('i-drop',
					       'drop',
					       b_url::same("?resetcache_once=1&tm_m_once=$module&tm_delete_once=".$task['var_name']),
					       $confirm=True);
	  }

	  // Clean duplicates if needed
	  if ($clean_duplicates){
	    if (!empty($task['_cmt'])){
	      b_vars::delete($task['var_name'],$module);
	      myPear_logs()->syslog(sprintf("Clean duplicated %s %s",$task['var_name'],$this->doing($task,False)));
	    }
	    continue;
	  }

	  // Add the human readable name
	  if ($class = loader::getClassFromID(@$task['args'][0],'class')){
	    $task['_av'] = $class->name();
	  }

	  $task['n'] = x('h6',(int)str_replace(taskManager_ID,'',$task['var_name']));
	  $task['_function'] = $this->doing($task);
	  $task['_module'] = ($module == $module_was ? '' : x('strong',$module));
	  if (!empty($task['start'])) $task['_start']  = date('Y-m-d H:i',$task['start']);
	  if (!empty($task['cmt']))   $task['_cmt'] = x('strong',b_fmt::redText($task['cmt']));
	  if ( empty($task['end'])){
	    $n_waiting++;
	    $t->extraTD[] = b_btn::submit_icon('i-bullet_go',
					       'execute',
					       b_url::same("?resetcache_once=1&myPear_taskManager=y&tm_m_once=$module&tm_execute_once=".$task['var_name']),
					       $confirm=True);
	    $t->extraTD[] = b_btn::submit_icon('i-drop',
					       'drop',
					       b_url::same("?resetcache_once=1&tm_m_once=$module&tm_delete_once=".$task['var_name']),
					       $confirm=True);
	  }elseif(($task['end']+TM_clean_log_time*24*3600 < time()) || (@$_REQUEST['tm_clean_once']==='yes')){
	    myPear_logs()->syslog($msg="Clean completed $module::".$task['var_name']);
	    MSG::WARNING($msg);
	    b_vars::delete($task['var_name'],$module);
	    continue;
	  }elseif(@$_REQUEST['tm_clean_lock_once']==='yes'){
	    MSG::WARNING('tm_clean_lock_once');
	    continue;
	  }else{
	    $n_ended++;
	    $task['_end']  = (empty($task['start'])
			      ? x('em',date('Y-m-d H:i:s',$task['end']))
			      : x('em','in '.b_time::delta(b_time::txt2unix($task['start']),$task['end'])));
	  }
	  
	  if ($_av_dev = @$task['av_dev']){
	    $av = loader::getInstance_new('bForm_Avatar',$_av_dev,'fatal');
	    $task['_av_dev'] = $av->getEmail();
	  } 
	  $t->prt($task);
	  $module_was = $module;
	}
      }
      if ($n_waiting) 
	$btns[] = b_btn::submit("execute $n_waiting waiting tasks",
				b_url::same('?resetcache_once=1&tm_execute_once=yes&myPear_taskManager=yes'));
      if ($n_ended)   
	$btns[] = b_btn::submit("clean out $n_ended completed tasks",
				b_url::same('?tm_execute_once=1&resetcache_once=1&tm_clean_once=yes&myPear_taskManager=yes'));
      if ($this->lock_isActive(False)) 
	$btns[] = b_btn::submit("remove deadlock",
				b_url::same('?tm_execute_once=1&resetcache_once=1&tm_clean_lock_once=yes&myPear_taskManager=yes'));
      if (!empty($btns)) print x('tr',x('td colspan=9 style="text-align:center;"',b_btn::UL_buttons($btns)));
      $t->close();
    }
  }

  /*
   *
   */
  private function doing($task,$strong=True){
    $_function = array();
    if (!empty($task['class']))    $_function[] = $task['class'].'()->';
    if (!empty($task['function'])) $_function[] = $task['function'];
    if (is_array(@$task['args']))  $_function[] = x('(',join(', ',b_debug::parse_tb_arguments($task['args'])));
    $reply = implode('',$_function);
    return ($strong
	    ? x('strong',$reply)
	    :            $reply);
  }
  
  /*
   * Store the task for execution
   */
  public function addTask($class='',$function='',$args=array()){
    
    $this->dbg(starting);

    // Store the task
    if (!is_array($args)) $args = array($args);
    $taskManager_args = array('class'   =>$class,
			      'function'=>$function,
			      'args'    =>$args);
    
    if (b_vars::isValueKnown($taskManager_args)){
      $this->dbg("Deja Vu...");
    }else{
      // get and update the task counter
      myPear_db()->transaction('start');
      
      // Store the task description as a variable
      // Take care about "login as other user", used for debugging
      $extras = array('start'=>time());
      if ((cnf_dev || cnf_demo) && bAuth::authenticated()){
	$extras['av_dev'] = bAuth::$av_authenticated->ID;
	MSG::WARNING(sprintf('Set <em>%s</em>  as the task submitter',bAuth::$av_authenticated->name()));
      }
      
      // Get the new task name
      $this->set($this->get_var_name(),$extras,$taskManager_args);
      myPear_logs()->add('TM_add','',$this->doing($taskManager_args,False),myPear_cronjob()->ID,b_reg::$current_module,False);
      myPear_db()->transaction('end');
      // Syslog the request
      myPear_logs()->syslog(implode(' ',array(__METHOD__,
					      $this->get_var_name(False),
					      $this->doing($taskManager_args,False))));
      $this->dbg($this->doing($taskManager_args));
    }
    $this->dbg(exiting);
  }
  
  /*
   * Create OR return a short form of the task name
   */
  private $taskID = Null;
  private function get_var_name($new_ID=True,$task_name=''){
    if ($new_ID){
      $n = 0;
      while(b_vars::isVariableKnown($this->taskID=sprintf("%s%04d",taskManager_ID,$n))) { $n++; }
      $reply = $this->taskID;
    }else{
      if (empty($task_name)) $task_name = $this->taskID;
      if (empty($task_name)) b_debug::internalError('Missing "$task_name" argument');
      $reply = str_replace(taskManager_ID,'task_',$task_name);
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  /*
   * Lock the taskManager
   */
  private $lock_start = Null; 
  private function lock_acquire(){

    $this->dbg();

    // Wait until the lock expires
    $this->lock_isActive($release_if_deadlock=True);

    // Write the lock record
    b_vars::set('taskManager_lock',array('time'=>time()),myPear_MODULE);
    $this->lock_start = time();
    if ($this->lock_start != time()) MSG::MESSAGE("Lock acquired in ".b_time::delta(time(),$this->lock_start));
  }
  
  /*
   * Unlock the taskManager
   */
  private function lock_release(){
    b_vars::delete('taskManager_lock',myPear_MODULE);
    $this->dbg("Locked since ".b_time::delta(time(),$this->lock_start));
  }

  /*
   * Check is the lock active.
   * Optionally release the the lock the it is expired.
   */
  private function lock_isActive($release_if_deadlock=True, $deadlock=60){ // dead lock in minutes
    static $wait_for = 30; // Seconds

    $reply = False;
    $this->dbg();
    while($lock = b_vars::get('taskManager_lock',myPear_MODULE)){
      $locked_at = $lock['time'];
      $reply = True;
      if ($release_if_deadlock){
	// Wait for the lock to disapier...
	if ((time() - $locked_at) > 60*$deadlock){
	  MSG::WARNING(sprintf("taskManager is locked for %s, remove the deadlock",b_time::delta(time(),$locked_at)));
	  $this->lock_release();
	  $reply = False;
	}else{
	  sleep($wait_for);
	}
      }else{
	// if ($this->lock_start != time()) MSG::MESSAGE(date('Y-m-d H:i:s ',$this->lock_start)." TaskManager is last locked in ".b_time::delta(time(),$this->lock_start));
	if ($locked_at != time()) MSG::MESSAGE("TaskManager is locked since ".b_time::delta(time(),$locked_at).date(' (Y-m-d H:i:s) ',$locked_at));
	break; // Lock is there, we no not want to know more...
      }
    }
    return $reply;
  }

  /*
   * Execute tasks from the queue. 
   * The function/method executed by the taskManager must return boolean status ('true' for success, 'false' for failure)
   */  
  function doTasks($module2exec='all',$task2exec='all'){
    static $class_instance = array();
    static $sleep_time = 13; // seconds
    if (empty($_REQUEST['tm_execute_once'])){
      $this->dbg("Spurious invocation, abort...");
      return;
    }
    
    //    $this->dbg(starting);
    //    $this->lock_acquire();
    set_time_limit(0);
    
    // Clean out the duplicated entries
    $this->info(True);
    
    // Execute the task(s)

    if (($task = @$_REQUEST['tm_execute_once']) && ($module = @$_REQUEST['tm_m_once'])){
      $module2exec = $module;
      $task2exec   = $task;
      $this->dbg("module2exec=$module2exec, task2exec=$task2exec");
    }
 
    // Execute the queue
    $info = array();
    foreach(b_vars::get(taskManager_ID,Null,True) as $module=>$tasks){
      if (($module != $module2exec) && ($module2exec != 'all')) continue;

      $this->dbg($tasks);
     
      // Be sure that the module is loaded
      b_reg::load_module($module);
      
      // Set flag "call from taskManager" 
      $this->exec_request = True; 
      
      // Walk thru the tasks
      foreach($tasks as $task_p){
	if (($task_p['var_name'] != $task2exec) && ($task2exec != 'all')) continue;
	
	$this->dbg(starting);
	$this->lock_acquire();
	
	// Reread the task, it might have been changed by
	// another process while we were waiting for the lock
	$task = b_vars::get($task_p['var_name'],$module);
	//	if (!empty($task)) $task['var_name'] = $task_p['var_name']; // shit... should be better
	
	if (($task_already_completed = @$task['end']) || empty($task)){
	  if (empty($task_already_completed)) $task_already_completed = time();
	  $this->dbg(sprintf("%s ended %s",$this->get_var_name(False,$task['var_name']),b_time::delta($task_already_completed,time())));
	}else{
	  $args = (empty($task['args'])
		   ? array()
		   : b_fmt::safe_decode($task['args']));
	  
	  // Transmit the developer email, the mails from tasks started by the developer should be sent back to him
	  if ($av_dev = @$task['av_dev']){
	    $av = loader::getInstance_new('bForm_Avatar',$av_dev,'fatal');
	    $GLOBALS['task_report_to'] = $av->getEmail();
	    MSG::WARNING(sprintf("<em>%s</em> id declared as the task submitter (debug mode)",$av->getEmail()));
	  }else{
	    unset($GLOBALS['task_report_to']);
	  }
	  
	  $this->dbg($this->doing($task));
	  if (empty($task['class'])){
	    //	  
	    // Calling (static method) function
	    //
	    list($class,$function) = explode('::',$task['function']);
	    if (!empty($function)) locateAndInclude($class,'fatal');
	    // Execute the task
	    $OK = call_user_func_array($task['function'],$args);
	  }else{
	    //
	    // Calling class method
	    // Instantiate the class
	    //
	    locateAndInclude(($class=$task['class']),'fatal');
	    if (empty($class_instance[$task['class']])) $class_instance[$task['class']] = new $class();
	    if (method_exists($class_instance[$task['class']],$task['function'])){
	      // Execute the task
	      $class_instance[$task['class']]::$_debug = self::$_debug;
	      $OK = call_user_func_array(array($class_instance[$task['class']],$task['function']),$args);
	    }else{
	      // Method does not exist...
	      $msg = sprintf("Method %s()->%s does not exist can't execute the task.<br/>Task ignored and declared to be completed...",$class,$task['function']);
	      MSG::ERROR($msg);
	      b_debug::xxx($msg);
	      $OK = True;
	    }
	  }
	  
	  if ($OK){
	    // Log the task execution	
	    myPear_logs()->add('TM_exec','',$this->doing($task,False),myPear_cronjob()->ID,b_reg::$current_module,False);
	    $log_info=implode(' ',array(__METHOD__,
					$this->get_var_name(False,$task['var_name']),
					$this->doing($task)));
	    myPear_logs()->syslog(strip_tags($log_info));
	    $info[] = x('p',$log_info);
	    
	    // Remove (or mark as executed) the task
	    if ($this->removeAfterExecution){
	      // Optionally remove the task after completion
	      b_vars::delete($task['var_name'],$module);	
	    }else{
	      // Or leave it, but save the execution time
	      $this->dbg(sprintf('%s - set "ended"',$task['var_name']));
	      $this->set($task['var_name'],$task,array('end'=>time()),$module);
	    }
	  }else{
	    MSG::ERROR($this->doing($task).' FAILED');
	  }
	}
	$this->lock_release();
	$this->dbg(exiting);
	// Sleep for a few seconds to make the paranoiac SMTP servers happy
	if (!$task_already_completed) sleep(++$sleep_time);
      }
    }

    // Print a summary information
    if (!empty($info)){
      MSG::INFO($info,__CLASS__.' - Executing collected tasks'); 
      // Print comments to the html file, to be extracted by a cron job
      printf("<!--\n%s - executing collected tasks\n",__CLASS__);
      foreach($info as $i) print b_fmt::unEscape(strip_tags($i))."\n";
      print "-->\n";
    }
    
    
    // Show the remaining tasks
    $this->info();
    //    $this->lock_release();
    //    $this->dbg(exiting);
  }
}
  
