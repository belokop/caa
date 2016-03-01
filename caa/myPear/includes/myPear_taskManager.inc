<?php

// namespace taskManager;

define('taskManager_ID','taskManager_task');

class myPear_taskManager{

  public $request = False;
  public $debug = True;
  
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
  }

  /*
   * Get summary information about the tasks in the queue
   */
  function info($clean_duplicates=False){
    $taskManager = b_vars::get(taskManager_ID,Null,True);
    if (empty($taskManager)){
      myPear::MESSAGE("There is no tasks in the queue for now");
    }else{
      $t = new b_table_zebra(array('n'       =>'#',
				   'cmt'     =>' ',
				   'module'  => 'M',
				   'class'   => 'Class',
				   'function'=> 'Function',
				   '_args'   => 'Args',
				   'var_timestamp' => 'Started',
				   'ended'   => 'Ended',
				   'wait'    => 'Waiting',
				   ));
      $dejaVu = array();
      $n_ended = 0;
      foreach($taskManager as $module=>$tasks){
	$module_was = '';
	$class_was = '';
	foreach($tasks as $task){
	  if (empty($task['class'])) $task['class'] = '';
	  $task['cmt'] = (($task['class']==='bUnit') || @$dejaVu[serialize(array_merge($task['args'],array($task['class'],$task['function'])))]++
			  ? b_fmt::redText('DejaVu')
			  : '');
	  
	  if (($now=@$task['class']) == $class_was) $task['class'] = '';
	  $task['n'] = x('tt',(int)str_replace(taskManager_ID,'',$task['var_name']));
	  $task['_args'] = (empty($task['args']) ? '' : str_replace(array(';',','),array('; ',', '),x('(',join(',',$task['args']))));
	  $task['module'] = ($module == $module_was ? '' : $module);
	  if (!empty($task['ended'])){
	    $task['wait']  = x('i',b_time::delta(b_time::txt2unix($task['var_timestamp']),$task['ended']));
	    $task['ended'] = date('Y-m-d H:i',$task['ended']);
	    $n_ended++;
	  }
	  $task['var_timestamp'] = date('Y-m-d H:i',b_time::txt2unix($task['var_timestamp']));
	  $t->prt($task);
	  $module_was = $module;
	  $class_was  = $now;
	  if ($clean_duplicates && !empty($task['cmt'])){
	    b_vars::delete($task['var_name'],$module);
	  }
	}
      }
      $t->close();
    }
  }

  /*
   * Store the task for execution
   */
  public function addTask($class='',$function='',$args=array()){

    // Store the task
    if (!is_array($args)) $args = array($args);
    $taskManager_args = array('class'   => $class,
			      'function'=>$function,
			      'args'    =>$args);

    if (b_vars::isValueKnown($taskManager_args)){
      b_debug::xxx("Deja Vu...");
    }else{
      // get and update the task counter
      myPear_db()->transaction('start');
      $task_counter = b_vars::get('taskManager_counter',myPear_MODULE);
      if (!(int)$task_counter) $task_counter = 0;
      b_vars::set('taskManager_counter',++$task_counter,myPear_MODULE);
      
      // Store the task description as a variable
      $task_id = sprintf("%s%04d",taskManager_ID,$task_counter);
      b_vars::set($task_id,$taskManager_args);
      myPear_db()->transaction('end');
      b_debug::xxx($task_id);
      myPear_logs()->add('TM_add','',$task_id,myPear_cronjob()->ID,b_reg::$current_module,True);
      myPear_logs()->syslog(__METHOD__." $task_id");
    }
  }

  /*
   * Execute tasks from the queue. 
   * By default the tasks are NOT executed, unless $_REQUEST['execute'] is set.
   */  
  function executeTasks($nmax=10,$removeAfterExecution=False){
    static $class_instance = array();

    // Show and eventually clean the queue
    $toExecute = !empty($_REQUEST['execute']);
    $this->info($toExecute);
    if (!$toExecute)  return;


    // Execute the queue
    set_time_limit(0);
    $info = array();
    foreach(b_vars::get(taskManager_ID,Null,True) as $module=>$tasks){

      // Be sure that the module is loaded
      b_reg::load_module($module);

      // Set flag "call from taskManager" 
      $this->request = True; 
      
      // Walk thru the tasks
      foreach($tasks as $task){
	if (!empty($task['ended'])) continue;
	$args = (empty($task['args'])
		 ? array()
		 : b_fmt::safe_decode($task['args']));
	
	if (empty($task['class'])){
	  //	  
	  // Calling (static method) function
	  //
	  list($class,$function) = explode('::',$task['function']);
	  if (!empty($function)) locateAndInclude($class,'fatal');
	  call_user_func_array($task['function'],$args);
	  $log_info = sprintf("%s(%s)",$task['function'],join(',',$args));
	}else{
	  //
	  // Calling class method
	  // Instantiate the class
	  //
	  locateAndInclude(($class=$task['class']),'fatal');
	  if (empty($class_instance[$task['class']])) $class_instance[$task['class']] = new $class();
	  $class_instance[$task['class']]->debug = True;
	  
	  // Execute the task
	  $reply = call_user_func_array(array($class_instance[$task['class']],$task['function']),$args);
	  $log_info = sprintf("%s()->%s(%s)",$task['class'],$task['function'],join(',',$args));
	}
	if (!empty($reply)) b_debug::xxx($reply);

	// Log the task execution	
	$log_info = sprintf("%s - %s %s",__METHOD__,$task['var_timestamp'],$log_info);
	myPear_logs()->syslog($log_info);
	$info[] = x('p',$log_info);
	
	// Remove (or mark as executed) the task
	if ($removeAfterExecution){
	  // Optionally remove the task after completion
	  b_vars::delete($task['var_name'],$module);	
	}else{
	  // Or leave it, but mark the execution time
	  $task['ended'] = time();
	  b_vars::set($task['var_name'],$task);
	}

	// Sleep for 30 seconds to make the SMTP server happy
	sleep(30);
	// Don't go too far...
	if (False){
	  if (empty($n_tasks_executed)) $n_tasks_executed = 0;
	  ++$n_tasks_executed;
	  if (isset($class_instance[$task['class']]->sent_mails_counter)) $n_tasks_executed = $class_instance[$task['class']]->sent_mails_counter;
	  if ($n_tasks_executed > $nmax) return;
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
  }
}
