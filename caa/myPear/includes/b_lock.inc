<?php
/**
 * @file
 * A database-mediated implementation of a locking mechanism.
 *
 * Crafted after drupal-7 lock.inc 
 */

/**
 * Initialize the locking system.
 */
function zzz_semaphore_initialise() {
  global $zzz_semaphore_locks;
  
  if (class_exists('myPear',False) && myPear_db()->isReady()){
    if (!isset($zzz_semaphore_locks))    $zzz_semaphore_locks = array();
    if (!myPear_db()->tableExists('zzz_semaphore')){
      myPear_db()->query("CREATE TABLE `zzz_semaphore` (
                               `s_name`   varchar(240) NOT NULL DEFAULT '',
                               `s_value`  varchar(240) NOT NULL DEFAULT '',
                               `s_expire` double NOT NULL,
                               `s_ts`     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                                PRIMARY KEY (`s_name`),
                                KEY `s_value` (`s_value`),
                                KEY `s_expire`(`s_expire`)
                             ) COMMENT='Table for holding semaphores, locks, flags, etc. that...';");
      myPear_db()->reset_cache();
    }
    return True;
  }else{
    return False;
  }
}

function b_lock_db_query(){
  if (zzz_semaphore_initialise()){
    $args = func_get_args();
    
    // Massage the arguments, I prefer to keep the initial code untouched.
    foreach(array('name','value','expire','ts') as $a) $args[0] = preg_replace('/\b'.$a.'\b/',"s_$a",$args[0]);
    $args[0] = str_replace('{semaphore}','zzz_semaphore',$args[0]);
    
    // Send the query
    $query = call_user_func_array('sprintf', $args);
    return myPear_db()->qquery($query,cnf_show_sem);
  }
}

function b_lock_db_fetch_array($result) {
  if ($result) $array = mysqli_fetch_array($result, MYSQLI_ASSOC);
  return isset($array) ? $array : False;
}

/**
 * @defgroup lock Functions to coordinate long-running operations across requests.
 * @{
 * In most environments, multiple Drupal page requests (a.k.a. threads or
 * processes) will execute in parallel. This leads to potential conflicts or
 * race conditions when two requests execute the same code at the same time. A
 * common example of this is a rebuild like menu_rebuild() where we invoke many
 * hook implementations to get and process data from all active modules, and
 * then delete the current data in the database to insert the new afterwards.
 *
 * This is a cooperative, advisory lock system. Any long-running operation
 * that could potentially be attempted in parallel by multiple requests should
 * try to acquire a lock before proceeding. By obtaining a lock, one request
 * notifies any other requests that a specific opertation is in progress which
 * must not be executed in parallel.
 *
 * To use this API, pick a unique name for the lock. A sensible choice is the
 * name of the function performing the operation. A very simple example use of
 * this API:
 * @code
 * function mymodule_long_operation() {
 *   if (b_lock_acquire('mymodule_long_operation')) {
 *     // Do the long operation here.
 *     // ...
 *     b_lock_release('mymodule_long_operation');
 *   }
 * }
 * @endcode
 *
 * If a function acquires a lock it should always release it when the
 * operation is complete by calling b_lock_release(), as in the example.
 *
 * A function that has acquired a lock may attempt to renew a lock (extend the
 * duration of the lock) by calling b_lock_acquire() again during the operation.
 * Failure to renew a lock is indicative that another request has acquired
 * the lock, and that the current operation may need to be aborted.
 *
 * If a function fails to acquire a lock it may either immediately return, or
 * it may call b_lock_wait() if the rest of the current page request requires
 * that the operation in question be complete.  After b_lock_wait() returns,
 * the function may again attempt to acquire the lock, or may simply allow the
 * page request to proceed on the  assumption that a parallel request completed
 * the operation.
 *
 * b_lock_acquire() and b_lock_wait() will automatically break (delete) a lock
 * whose duration has exceeded the timeout specified when it was acquired.
 *
 * Alternative implementations of this API (such as APC) may be substituted
 * by setting the 'lock_inc' variable to an alternate include filepath.  Since
 * this is an API intended to support alternative implementations, code using
 * this API should never rely upon specific implementation details (for example
 * no code should look for or directly modify a lock in the {semaphore} table).
 */

/**
 * Helper function to get this request's unique id.
 */
function _b_lock_id() {
  static $lock_id;

  if (!isset($lock_id)) {
    // Assign a unique id.
    $lock_id = uniqid(mt_rand(), TRUE);
    // We only register a shutdown function if a lock is used.
    // register_shutdown_function('b_lock_release_all', $lock_id);
  }
  return $lock_id;
}

/**
 * Acquire (or renew) a lock, but do not block if it fails.
 *
 * @param $name
 *   The name of the lock.
 * @param $timeout
 *   A number of seconds (float) before the lock expires.
 * @return
 *   TRUE if the lock was acquired, FALSE if it failed.
 */
function b_lock_acquire($name, $timeout = 30.0) {
  global $zzz_semaphore_locks;
  
  if (zzz_semaphore_initialise()){
    // Insure that the timeout is at least 1 ms.
    $timeout = max($timeout, 0.001);
    list($usec, $sec) = explode(' ', microtime());
    $expire = (float)$usec + (float)$sec + $timeout;
    if (isset($zzz_semaphore_locks[$name])) {
      // Try to extend the expiration of a lock we already acquired.
      b_lock_db_query("UPDATE {semaphore} SET expire = %f WHERE name = '%s' AND value = '%s'", $expire, $name, _b_lock_id());
      if (!myPear_db()->affected_rows) {
	// The lock was broken.
	unset($zzz_semaphore_locks[$name]);
      }
    }
    else {
      // Optimistically try to acquire the lock, then retry once if it fails.
      // The first time through the loop cannot be a retry.
      $retry = FALSE;
      // We always want to do this code at least once.
      do {
	if (@b_lock_db_query("INSERT INTO {semaphore} (name, value, expire) VALUES ('%s', '%s', %f)", $name, _b_lock_id(), $expire)) {
	  // We track all acquired locks in the global variable.
	  $zzz_semaphore_locks[$name] = TRUE;
	  // We never need to try again.
	  $retry = FALSE;
	}
	else {
	  // Suppress the error. If this is our first pass through the loop,
	  // then $retry is FALSE. In this case, the insert must have failed
	  // meaning some other request acquired the lock but did not release it.
	  // We decide whether to retry by checking b_lock_may_be_available()
	  // Since this will break the lock in case it is expired.
	  $retry = $retry ? FALSE : b_lock_may_be_available($name);
	}
	// We only retry in case the first attempt failed, but we then broke
	// an expired lock.
      } while ($retry);
    }
    return isset($zzz_semaphore_locks[$name]);
  }else{
    return True;
  }
}

/**
 * Check if lock acquired by a different process may be available.
 *
 * If an existing lock has expired, it is removed.
 *
 * @param $name
 *   The name of the lock.
 * @return
 *   TRUE if there is no lock or it was removed, FALSE otherwise.
 */
function b_lock_may_be_available($name) {
  $lock = b_lock_db_fetch_array(b_lock_db_query("SELECT expire, value FROM {semaphore} WHERE name = '%s'", $name));
  if (!$lock) {
    return TRUE;
  }
  $expire = (float) $lock['s_expire'];
  list($usec, $sec) = explode(' ', microtime());
  $now = (float)$usec + (float)$sec;
  if ($now > $lock['s_expire']) {
    // We check two conditions to prevent a race condition where another
    // request acquired the lock and set a new expire time.  We add a small
    // number to $expire to avoid errors with float to string conversion.
    b_lock_db_query("DELETE FROM {semaphore} WHERE name = '%s' AND value = '%s' AND expire <= %f", $name, $lock['s_value'], 0.0001 + $expire);
    return (bool)myPear_db()->affected_rows;
  }
  return FALSE;
}

/**
 * Wait for a lock to be available.
 *
 * This function may be called in a request that fails to acquire a desired
 * lock. This will block further execution until the lock is available or the
 * specified delay in seconds is reached.  This should not be used with locks
 * that are acquired very frequently, since the lock is likely to be acquired
 * again by a different request during the sleep().
 *
 * @param $name
 *   The name of the lock.
 * @param $delay
 *   The maximum number of seconds to wait, as an integer.
 * @return
 *   TRUE if the lock holds, FALSE if it is available.
 */
function b_lock_wait($name, $delay = 30) {

  while ($delay--) {
    // This function should only be called by a request that failed to get a
    // lock, so we sleep first to give the parallel request a chance to finish
    // and release the lock.
    sleep(1);
    if (b_lock_may_be_available($name)) {
      // No longer need to wait.
      return FALSE;
    }
  }
  // The caller must still wait longer to get the lock.
  return TRUE;
}

/**
 * Release a lock previously acquired by b_lock_acquire().
 *
 * This will release the named lock if it is still held by the current request.
 *
 * @param $name
 *   The name of the lock.
 */
function b_lock_release($name) {
  global $zzz_semaphore_locks;
  if (zzz_semaphore_initialise()){
    unset($zzz_semaphore_locks[$name]);
    b_lock_db_query("DELETE FROM {semaphore} WHERE name = '%s' AND value = '%s'", $name, _b_lock_id());
  }
}

/**
 * Release all previously acquired locks.
 */
function b_lock_release_all($lock_id = NULL) {
  global $zzz_semaphore_locks;
  $zzz_semaphore_locks = array();

  if (empty($lock_id)) {
    $lock_id = _b_lock_id();
  }
  b_lock_db_query("DELETE FROM {semaphore} WHERE value = '%s'", _b_lock_id());
}

/**
 * @} End of "defgroup locks".
 */
