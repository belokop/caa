<?php
/*
 * A simple one-pass covering algorith       YB 2017-06-25 (Midsommare 2017)
 */
define('bCover_2017_with_holes',False);
class bCover_2017 extends myPear_base{

  public  $solution = array();

  private $request;
  private $request_size;
  private $day1, $day2;
  private $holes = array();
  public  $debug = False;
  
  function __construct($request){
    $this->request_size = count($request);
    $this->request = $request;
    $this->sort($this->request);
    list($this->day1,$this->day9) = array(9999999999,-9999999999);
    foreach($this->request as $k=>$entry){
      $this->request[$k]['day1'] = b_time::check_in( $entry['day1']);
      $this->request[$k]['day9'] = b_time::check_in( $entry['day9']);
      $this->day1 = b_time::oa_start(min($this->day1,$this->request[$k]['day1']));
      $this->day9 = b_time::oa_end(  max($this->day9,$this->request[$k]['day9']));
    }
    $this->dbg(date("Y-m-d H:i",$this->day1).' --> '.date("Y-m-d H:i",$this->day9));
  }
  
  function solve(){
    bTiming()->cpu(__FUNCTION__);    
    for($pivot=0; $pivot<$this->request_size; $pivot++){
      if (!isset($this->request[$pivot]))      continue;
      
      $this->dbg(x('code',"Pivot $pivot --- ".b_time::period($this->request[$pivot]['day1'],
							     $this->request[$pivot]['day9'])));
      
      // Check does the entry fit into any hole
      $fits_a_hole = False;
      if (bCover_2017_with_holes) foreach($this->holes as $hole_pivot=>$holes){
	  foreach($holes as $hole){
	    $this->dbg("$hole_pivot - hole ".b_time::period($hole['day1'],$hole['day9']));
	    if ($this->request[$pivot]['day1']>$hole['day1'] &&
		$this->request[$pivot]['day9']  <$hole['day9']){
	      list($start_of_line,$end_of_line) = $this->addEntryToSolution($hole_pivot,$pivot);
	      $fits_a_hole = True;
	      myPear::WARNING_T("!!!!!!!! Fits a hole",$this);
	      if ($this->debug) $this->debug_pivot($hole_pivot);
	    }
	  }
	}
      
      // Try more if not...
      if (!$fits_a_hole){
	// Add the pivot itself
	list($start_of_line,$end_of_line) = $this->addEntryToSolution($pivot,$pivot);
	
	// Add candidates
	for($candidate=$pivot+1; $candidate<$this->request_size; $candidate++){
	  if (!isset($this->request[$candidate])) continue; 
	  if ($this->request[$candidate]['day1'] >= $end_of_line){ // or the candidate fits the hole
	    list($start_of_line,$end_of_line) = $this->addEntryToSolution($pivot,$candidate);
	    unset($this->request[$candidate]);
	  }
	}
      }
      //      $this->show_solution(array($pivot=>$this->solution[$pivot]));
      if ($this->debug) $this->debug_pivot($pivot);
      unset($this->request[$pivot]);
    }
    bTiming()->cpu();
    $this->show_solution();
    return $this->solution;
  }

  /*
   *
   */
  private function addEntryToSolution($pivot,$candidate){
    $this->solution[$pivot][] = $this->request[$candidate];
    $this->sort($this->solution[$pivot]);
    foreach($this->solution[$pivot] as $entry) list($start_of_line,$end_of_line) = array($entry['day1'],$entry['day9']);
    $this->get_holes($pivot);
    return array($start_of_line,$end_of_line);
  }

  /*
   *
   */
  private function debug_pivot($pivot){
    $this->dbg("--------------------------------------------solution $pivot");
    foreach($this->solution[$pivot] as $entry) $this->dbg(b_time::period($entry['day1'],
									 $entry['day9']));
    if (bCover_2017_with_holes){
      $this->dbg("-----holes $pivot");
      foreach($this->holes[$pivot] as $hole) $this->dbg(b_time::period($hole['day1'],$hole['day9']));
    }
  }

  /*
   *
   */
  private function get_holes($pivot){
    if (!bCover_2017_with_holes) return;
    $this->holes[$pivot] = array();
    $this->sort($this->solution[$pivot]);
    foreach($this->solution[$pivot] as $entry){
      if ($entry['day1'] > $this->day1) $this->holes[$pivot][] = array('day1'=>(empty($hole_starts) 
										  ? $this->day1
										  : $hole_starts),
									'day9'  =>$entry['day1']);
      $hole_starts = $entry['day9'];
    }
    $this->holes[$pivot][] = array('day1'=>(empty($hole_starts) 
					     ? $this->day1
					     : $hole_starts),
				   'day9'  =>$this->day9);
    foreach($this->holes[$pivot] as $k=>$hole){
      if (date('YMD',$hole['day1']) == date('YMD',$hole['day9'])) unset($this->holes[$pivot][$k]);
    }
  }

  /*
   *
   */  
  function show_solution($solution=Null){
    if (empty($solution)) $solution = $this->solution;
    $quality = count(array_values($solution));
    
    $hr_solution = array();
    $ml = 0;
    foreach(array_values($solution) as $line) foreach($line as $entry) $ml = max($ml,strlen(trim(@$entry['name'])));
    foreach(array_values($solution) as $line){
      $x = array();
      foreach($line as $entry) $x[] = (empty($entry['name'])
				       //? sprintf("<code>[%3d,%3d]</code>",$entry['day1'],$entry['day9'])
				       ? sprintf("<code>[%s]</code>",b_time::period($entry['day1'],$entry['day9'],'short'))
				       : sprintf("<code>%s[%s]</code>",str_repeat('&nbsp;',max(0,$ml-strlen($entry['name']))),trim($entry['name'])));
      $hr_solution[] = sprintf("%s %3d - %s\n",str_repeat('&nbsp;',5),(@++$k),join(', ',$x));
    }
    $this->dbg(sprintf("-------------- quality = %s/%s<br>\n",$quality,$this->request_size).
	       join('<br>',$hr_solution),cnf_dev);
  }

  /*
   * sort by 'day1'
   */
  function sort(&$array){
    if (!function_exists('cmp_2017')){
      function cmp_2017($a, $b){
       return ($a['day1'] == $b['day1']
	       ? 0
	       : ($a['day1'] < $b['day1']) ? -1 : 1);
      }
    }
    usort($array,'cmp_2017');
  }
}

