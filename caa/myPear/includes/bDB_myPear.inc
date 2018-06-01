<?php
locateAndInclude('bDB');

class bDB_myPear extends bDB{
  
  /*
   * Use proper collation for sorting people by name
   */
  function orderBy($columns='av_lastname',$table='zzz_avatars',$direction='ASC'){
    if (empty($_SESSION['collation_database'])){
      $q = $this->query('show variables like "collation_database"');
      while ($r = $this->next_record($q)){
	if (!empty($r['Value'])){
	  $_SESSION['collation_database'] = $r['Value'];
	  $this->dbg("collation ".$_SESSION['collation_database'],cnf_dev);
	}
      }
    }
    $order = array();
    foreach(explode(',',$columns) as $c){
      $order[] = (!cnf_CLI && preg_match("/(text|char)/i",$this->getColumnType($table,$c,True))
		  ? "UPPER($c) COLLATE ".$_SESSION['collation_database']
		  : $c);
    }
    $reply = " ORDER BY ".join(',',$order)." $direction ";
    return $reply;
  }
  
  /*
   *
   */
  function reset_cache(){
    parent::reset_cache();
    $this->get_primaryKeys('reset');
  }


  /*
   *
   */
  function get_primaryKeys($mode='max'){
    
    if ($mode === 'reset'){
      $_SESSION[myPear_cache][$this->Database] = array();
    }else{
      $primary_keys = @$_SESSION[myPear_cache][$this->Database][$mode];
      if (empty($primary_keys)){
        // This is a "brute force" method. Not elegant, but works
        $p_keys = array();
        $n_column = 0;
        foreach($this->getTables() as $tb){
	  // Skip the tables which do not use a unique ID
          if (in_array($tb,array('zzz_master','zzz_semaphore','zzz_logs','zzz_templates','zzz_bugs','zzz_tt'))) continue;
	  if (strpos($tb,'lic_')  === 0)            continue;
	  
	  $q = myPear_db()->query("SHOW KEYS FROM $tb WHERE Key_name = 'PRIMARY'");
	  while($r = myPear_db()->next_record($q)){
	    if (strpos($r['Column_name'],'_id') == (strlen($r['Column_name'])-3)){
	      $p_keys[$tb][] = ($mode === 'max' 
				? "MAX(`$tb`.`$r[Column_name]`) AS m".(++$n_column)
				: $r['Column_name']);
	    }
	  }
        }
        $primary_keys = array();
        foreach($p_keys as $key=>$values){
	  $values = array_unique($values);
	  if (count($values) != 1) b_debug::internalError("table $key, IDs=".join(',',$values));
	  $primary_keys[$key] = join(', ',$values);
	}
        $_SESSION[myPear_cache][$this->Database][$mode] = $primary_keys;
      }
      return $primary_keys;
    }
  }
}
