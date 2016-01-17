<?php
/*
 * "Set Cover" 
 * Find the best distribution of 1-d arrays
 * $this->set = Array([<key>] => Array([start] => <startTime>
 *                                     [end]   => <endTime>))
 */ 
class bCover{

  var $id    = NULL;
  var $start = 'start';
  var $end   = 'end';
  var $debug = 0; 
  var $timeMn= 0;
  var $timeMx= 0;
  var $set        = array();
  var $constraints= array();
  var $solution   = array();
  
  function __construct($set=array(),$id=NULL,$start=NULL,$end=NULL){
    $this->dbg('-----------------------------------');
    if (!empty($set)) $this->_init($set,$id,$start,$end);
  }
  
  function _init($set=array(),$id=NULL,$start=NULL,$end=NULL){
    if ($id)    $this->id    = $id;
    if ($start) $this->start = $start;
    if ($end)   $this->end   = $end;
    $this->constraints = $this->solution = $this->set = array();
    $this->timeMn = $this->timeMx = $setCounter = 0;
    foreach ($set as $k=>$r){
      $this->set[$k] = $r;
      if (!isset($this->set[$k][$this->id])) $this->set[$k][$this->id] = ++$setCounter;
      $this->setMinMaxTime($this->set[$k]);
      
      if ($this->start!='day1' && empty($this->set[$k]['day1'])) $this->set[$k]['day1'] = @$this->set[$k][$start];// interface with the bIcal_matrix class
      if ($this->end  !='day9' && empty($this->set[$k]['day9'])) $this->set[$k]['day9'] = @$this->set[$k][$end];
    }
    $this->dbg("length of the set = ".count($this->set));
  }
  
  public $current_solution = array();
  function solve(){
    bTiming()->cpu(__FUNCTION__);
    $this->dbg(count($this->constraints).' lines of constraints');
    foreach ($this->constraints as $n=>$c) $coun["constraint $n"] = count($c);

    if (empty($this->set)){
      $this->solution = $this->constraints;
    }else{
      foreach  ($this->set as $n=>$pivot)  {
	if ((int)$pivot[$this->start] && (int)$pivot[$this->end]){ // sanity
	  $this->current_solution = $this->solvePivot($pivot);
	  list($quality,$n_unused)= $this->quality();
	  if (!isset($bestN)){
	    list($bestN,$bestQuality,$bestOcc,$this->solution) = array($n,$quality,$n_unused,$this->current_solution);
	  }
	  if (($quality > $bestQuality) || ($quality == $bestQuality && $n_unused < $bestOcc)){
	    list($bestN,$bestQuality,$bestOcc,$this->solution) = array($n,$quality,$n_unused,$this->current_solution);
	  }
	}
      }
    }
    bTiming()->cpu();
    return $this->solution;
  }

  protected function quality(){
    static $n = 0;
    $quality = $n_unused = 0;
    foreach($this->current_solution as $id=>$list){
      $quality++;
      foreach(array_values($list) as $item){
	if (count($list) > 1) 	$n_unused += ($item['day9'] - $item['day1']);
	if (!isset($day1)) $day1 = $item['day1'];
	if (!isset($day9)) $day9 = $item['day9'];
	$day1 = min($day1,$item['day1']);
	$day9 = max($day9,$item['day9']);
      }
    }
    $n_unused = (int)(($day9 - $day1 - $n_unused) / 24 / 3600);
    $this->dbg((++$n)." - quality=$quality  n_unused=$n_unused ");
    return array(-$quality,$n_unused);
  }

  function pivotIndex(){
    return array(bMaster()->get_masterCounter() + 1, '');
  }

  function solvePivot($pivot){
    $reply = array();
    $pivotNotSet = True;
    // insert constraints in the future solution
    foreach ($this->constraints as $n=>$constr) {
      $reply[$n] = array();
      $nClaches  = 0; 
      if (is_array($constr)) foreach ($constr as $nc=>$c){
	$reply[$n][] = $c;
	$nClaches += $this->clashing($pivot, $c);
      }
      if (!$nClaches && $pivotNotSet) {
	$pivotNotSet = False;
	$reply[$n][] = $pivot;
      }
    }
    if ($pivotNotSet) { 
      list($index,$name) = $this->pivotIndex();
      $reply[$index][] = $pivot;
    }

    foreach ($this->set as $k=>$cand){
      if (!(int)$cand[$this->start]) continue; //sanity
      if (!(int)$cand[$this->end])   continue; //sanity
      if ($this->same($cand,$pivot)) continue;
      $Nvacant = $Nmax = 0;
      foreach ($reply as $n=>$tenants){
	$clashes=0; foreach ($tenants as $nt=>$tenant) $clashes += $this->clashing($cand,$tenant);
	if (!$clashes) { $Nvacant = $n; break; }
	$Nmax = max($Nmax, $n);
      }
      if (!$Nvacant) $Nvacant = ++$Nmax;
      $reply[$Nvacant][] = $cand;
    }

    foreach ($reply as $n=>$a) if (count($reply[$n]) != count(@$this->constraints[$n])) $this->dump($a,'solution '.$n);
    return $reply;
  }
  
  protected function same($ar1, $ar2) {
    return $ar1[$this->id]==$ar2[$this->id];
  }

  function clashing($ar1, $ar2) {
    return b_time::inInterval($ar1[$this->start],$ar1[$this->end],
			      $ar2[$this->start],$ar2[$this->end]);
  }

  function dump($ar,$text){
    $l = strlen($text);
    ob_start();
    print "<div class='align_left'><pre>";
    $this->dumpAxi($ar,$text,$l);
    print "</pre></div>";
    $reply = ob_get_contents();
    ob_end_clean();
    if ($this->debug) myPear::DEBUG($reply);
  }
  
  function dumpAxi($ar,$text,$l=15){
    if (@$ar[$this->start]) printf("%-${l}s %11d - %s %s %s\n",
				   $text,$ar[$this->id],
				   b_time::short_date($ar[$this->start]),
				   b_time::short_date($ar[$this->end]),
				   @$ar['name']); 
    else foreach ($ar as $n=>$a){$this->dumpAxi($a, $text); $text=''; }
  }
  
  function setMinMaxTime($r){
    if (is_array($r)) {
      $d1 = $r[$this->start];
      $d9 = $r[$this->end];
    }else $d1 = $d9 = $r;
    if (!$this->timeMn) $this->timeMn = $d1;
    if (!$this->timeMx) $this->timeMx = $d9;
    $this->timeMn = min($this->timeMn, $d1, $d9);
    $this->timeMx = max($this->timeMx, $d1, $d9);
  }

  function dbg($text=' '){
    b_debug::xxx($text,array((bool)$this->debug,3,'orangeText'));
  }
}
