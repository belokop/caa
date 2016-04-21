<?php

// namespace taskManager;

define('taskManager_ID','taskManager_task');
define('TM_clean_log_time',7); // in days

class myPear_taskManager{

  public $exec_request = False;
  public $debug = cnf_dev || cnf_CLI;
  public $removeAfterExecution = False;
  
  /*
   * 
   */
  public function __construct(){

    // Eventually add a new task, i.e. put the following command into the task execution queue:
    //  - method $_REQUEST{addTask_class_once}->$_REQUEST{addTask_function_once}
    //  - arguments $_REQUEST{addTask_args_once}
    $addTask_class_once    = (string)@$_REQUEST['addTask_class_once'];
    $addTask_function_once = (string)@$_REQUEST['addTask_function_once'];
    $addTask_args_once     = (string)@$_REQUEST['addTask_args_once'];
    if (!empty($addTask_function_once)){
      if(empty($addTask_args_once)) $addTask_args_once = array();
      $this->addTask($addTask_class_once,
		     $addTask_function_once,
		     $addTask_args_once);
    }

    // Perform actions
    if ($task = @$_REQUEST['delete_once']){
      $module = $_REQUEST['m_once'];
      b_vars::delete($task,$module);
      myPear_logs()->syslog("Explicitely remove ".$task);
      myPear::WARNING("Removing ".$task);
    }
  }

  /*
   * Get summary information about the tasks in the queue
   */
  function info($clean_duplicates=False){

    $taskManager = b_vars::get(taskManager_ID,Null,True);
    if (empty($taskManager)){
      myPear::MESSAGE("There is no tasks in the queue for now");
    }else{
      $t = new b_table_zebra($header=array('n'         =>'#',
					   '_cmt'      => ' ',
					   '_module'   => 'module',
					   '_function' => 'task',
					   '_av'       => ' ',
					   '_start'    => 'start',
					   '_end'      => 'end',
					   ));
      $dejaVu = array();
      $n_ended = $n_waiting = 0;
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
					       b_url::same("?resetcache_once=1&m_once=$module&delete_once=".$task['var_name']),
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
	  if ($class = myPear::getClassFromID(@$task['args'][0],'class')){
	    $task['_av'] = $class->name();
	  }
	  
	  $task['_function'] = $this->doing($task);
	  $task['n'] = x('tt',(int)str_replace(taskManager_ID,'',$task['var_name']));
	  $task['_module'] = ($module == $module_was ? '' : x('strong',$module));
	  $task['_start']  = date('Y-m-d H:i',$task['start']);
	  if (empty($task['end'])){
	    $n_waiting++;
	    $t->extraTD[] = b_btn::submit_icon('i-bullet_go',
					       'execute',
					       b_url::same("?resetcache_once=1&myPear_taskManager=y&m_once=$module&execute_once=".$task['var_name']),
					       $confirm=True);
	    $t->extraTD[] = b_btn::submit_icon('i-drop',
					       'drop',
					       b_url::same("?resetcache_once=1&m_once=$module&delete_once=".$task['var_name']),
					       $confirm=True);
	  }elseif(($task['end']+TM_clean_log_time*24*3600 < time()) || (@$_GET['clean_once']==='yes')){
	    myPear_logs()->syslog("Clean completed $module::".$task['var_name']);
	    b_vars::delete($task['var_name'],$module);
	    continue;
	  }else{
	    $n_ended++;
	    $task['_end']  = x('em','in '.b_time::delta(b_time::txt2unix($task['start']),$task['end']));
	  }
	  $t->prt($task);
	  $module_was = $module;
	}
      }
      if ($n_waiting) $btns[] = b_btn::submit("execute $n_waiting waiting tasks",
					      b_url::same('?resetcache_once=1&execute_once=yes&myPear_taskManager=yes'));
      if ($n_ended)   $btns[] = b_btn::submit("clean out $n_ended completed tasks",
					      b_url::same('?resetcache_once=1&clean_once=yes&myPear_taskManager=yes'));
      if (!empty($btns)) print x('tr',x('td colspan=9 style="text-align:center;"',b_btn::UL_buttons($btns)));
      $t->close();
    }
  }

  private function doing($task,$strong=True){
    $_function = array();
    if (!empty($task['class']))    $_function[] = $task['class'].'()->';
    if (!empty($task['function'])) $_function[] = $task['function'];
    $_function[] = x('(',implode(',',$task['args']));
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
      $task_counter = b_vars::get('taskManager_counter',myPear_MODULE);
      if (!myPear::is_int($task_counter)) $task_counter = 0;
      b_vars::set('taskManager_counter',++$task_counter,myPear_MODULE);
      
      // Store the task description as a variable
      // Take care about "login as other user", used for debugging
      $extras = array('start'=>time());
      if (is_object(bAuth::$av_authenticated)) $extras['avid'] = bAuth::$av_authenticated->ID;

      b_vars::set($this->task_id($task_counter),array_merge($extras,$taskManager_args));
      myPear_logs()->add('TM_add','',$this->doing($taskManager_args,False),myPear_cronjob()->ID,b_reg::$current_module,False);
      myPear_db()->transaction('end');
      // Syslog the request
      myPear_logs()->syslog(implode(' ',array(__METHOD__,
					      $this->task_id($task_counter,True),
					      $this->doing($taskManager_args,False))));
      $this->dbg($this->doing($taskManager_args));
    }
    $this->dbg(exiting);
  }
  
  /*
   *
   */
  private function task_id($task_counter,$short=False){
    $task_id = (is_numeric($task_counter)
		? sprintf("%s%04d",($short ? 'task' :  taskManager_ID),$task_counter)
		: sprintf("%s%04d",'task',preg_replace('/[a-zA-Z_-]*/','',$task_counter)));
    return $task_id;
  }

  /*
   * Execute tasks from the queue. 
   * Small protection - by default the tasks are NOT executed, unless $_REQUEST['execute_once'] is set.
   */  
  function doTasks($module2exec='all',$task2exec='all'){
    static $class_instance = array();
    static $sleep_time = 13; // seconds
    if (empty($_REQUEST['execute_once'])) continue;

    $this->dbg(starting);
    set_time_limit(0);
    
    // Clean the queue
    $this->info(True);
    
    // Execute the task(s)

    if (($task = @$_REQUEST['execute_once']) && ($module = @$_REQUEST['m_once'])){
      $module2exec = $module;
      $task2exec   = $task;
      $this->dbg("module2exec=$module2exec, task2exec=$task2exec");
    }

    // Execute the queue
    $info = array();
    foreach(b_vars::get(taskManager_ID,Null,True) as $module=>$tasks){
      if (($module != $module2exec) && ($module2exec != 'all')) continue;
      
      // Be sure that the module is loaded
      b_reg::load_module($module);
      
      // Set flag "call from taskManager" 
      $this->exec_request = True; 
      
      // Walk thru the tasks
      foreach($tasks as $task){
	if (($task['var_name'] != $task2exec) && ($task2exec != 'all')) continue;
	
	if (!empty($task['end'])){
	  $this->dbg(sprintf("%s ended %s",$this->task_id($task['var_name']),b_time::delta($task['end'],time())));
	  continue;
	}
	$args = (empty($task['args'])
		 ? array()
		 : b_fmt::safe_decode($task['args']));
	
	// Transmit the debugger email, the tasks started by him should be sent back
	if ($av_id = @$task['avid']){
	  $av_authenticated = myPear::getInstance_new('bForm_Avatar',$av_id,'fatal');
	  $GLOBALS['av_send_to'] = $av_authenticated->getEmail();
	}else{
	  unset($GLOBALS['av_send_to']);
	}

	$this->dbg($this->doing($task));
	if (empty($task['class'])){
	  //	  
	  // Calling (static method) function
	  //
	  list($class,$function) = explode('::',$task['function']);
	  if (!empty($function)) locateAndInclude($class,'fatal');
	  $OK = call_user_func_array($task['function'],$args);
	}else{
	  //
	  // Calling class method
	  // Instantiate the class
	  //
	  locateAndInclude(($class=$task['class']),'fatal');
	  if (empty($class_instance[$task['class']])) $class_instance[$task['class']] = new $class();
	  $class_instance[$task['class']]->debug = $this->debug;
	  
	  // Execute the task
	  $OK = call_user_func_array(array($class_instance[$task['class']],$task['function']),$args);
	}

	if ($OK){
	  // Log the task execution	
	  myPear_logs()->add('TM_exec','',$this->doing($task,False),myPear_cronjob()->ID,b_reg::$current_module,False);
	  $log_info=implode(' ',array(__METHOD__,
				      $this->task_id($task['var_name']),
				      $this->doing($task)));
	  myPear_logs()->syslog(strip_tags($log_info));
	  $info[] = x('p',$log_info);
	  
	  // Remove (or mark as executed) the task
	  $task['end'] = time();
	  if ($this->removeAfterExecution){
	    // Optionally remove the task after completion
	    b_vars::delete($task['var_name'],$module);	
	  }else{
	    // Or leave it, but save the execution time
	    $this->dbg(sprintf('%s - set "ended"',$task['var_name']));
	    b_vars::set($task['var_name'],$task);
	  }
	  // Sleep for a few seconds to make the paranoiac SMTP servers happy
	  sleep(++$sleep_time);
	}else{
	  myPear::ERROR($this->doing($task).' FAILED');
	}
      }
    }
    if (!empty($info)){
      myPear::INFO($info,__CLASS__.' - Executing collected tasks'); 
      // Print comments to the html file, to be extracted by a cron job
      printf("<!--\n%s - executing collected tasks\n",__CLASS__);
      foreach($info as $i) print b_fmt::unEscape(strip_tags($i))."\n";
      print "-->\n";
    }


    // Show the remaing tasks
    $this->info();

    $this->dbg(exiting);
  }

  function dbg($text=' ',$forced=False){
    if (True || $this->debug || $forced) b_debug::xxx($text,array(3,True,'redText',$this));
  }
}
