<?php
/*
 * "Set Cover" 
 * Find the best distribution of 1-d arrays
 * $this->set = Array([<key>] => Array([start] => <startTime>
 *                                     [end]   => <endTime>))
 */ 
class bCover{

  var $id    = 'id';//NULL;
  var $start = 'start';
  var $end   = 'end';
  var $debug = False; 
  var $timeMn= Null;
  var $timeMx= Null;
  var $set        = array();
  var $constraints= array();
  var $solution   = array();
  
  function __construct($set=array(),$id=NULL,$start=NULL,$end=NULL){
    $this->dbg('-----------------------------------');
    if (!empty($set)) $this->_init($set,$id,$start,$end);
  }

  function __toString(){
    return get_class($this);
  }
  
  function _init($set=array(),$id=NULL,$start=NULL,$end=NULL){
    if ($id)    $this->id    = $id;
    if ($start) $this->start = $start;
    if ($end)   $this->end   = $end;

    $setCounter = 0;
    foreach ($set as $k=>$r){
      $this->set[$k] = $r;
      if (!isset($this->set[$k][$this->id])) $this->set[$k][$this->id] = ++$setCounter;
      $this->setMinMaxTime($this->set[$k]);
      
      // interface with the bIcal_matrix class
      if ($this->start!='day1' && empty($this->set[$k]['day1'])) $this->set[$k]['day1'] = $this->set[$k][$this->start];
      if ($this->end  !='day9' && empty($this->set[$k]['day9'])) $this->set[$k]['day9'] = $this->set[$k][$this->end];
      //      if (!$k) b_debug::var_dump($this->set[$k],"k=$k");
    }
    if ($this->debug) myPear::DEBUG(sprintf("%s() length of the set = %s, %s",
					    __METHOD__,count($this->set),b_time::period($this->timeMn,$this->timeMx)));
  }
  
  public $current_solution = array();
  function solve(){
    bTiming()->cpu(__FUNCTION__);
    $this->dbg(count(array_keys($this->constraints)).' lines of constraints');
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

  /*
   *
   */
  protected function quality(){
    static $n = 0;
    $quality = 0;
    $n_unused = $n_used = 0;
    foreach($this->current_solution as $id=>$visits){
      $n_u = 0;
      $quality++;
      foreach($visits as $k=>$visit){
	if (empty($visit['day1'])) $this->current_solution[$id][$k]['day1'] = $visit['day1'] = $visit[$this->start];
	if (empty($visit['day9'])) $this->current_solution[$id][$k]['day9'] = $visit['day9'] = $visit[$this->end];
 	if (count($visits) > 1) $n_u += ($visit[$this->end] - $visit[$this->start]);
	static $hghjhjhj=0; if (!$hghjhjhj++) b_debug::print_r($visit,"this->current_solution-$id-$k");
      }
      $n_unused += (int)(($this->timeMx - $this->timeMn - $n_u) / 24 / 3600);
      $n_used   += $n_u;
    }
    $this->dbg((++$n)." - quality=$quality  n_unused=$n_unused n_used=".(int)($n_used/24/3600));
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

    foreach ($reply as $n=>$a){
      if (($n1=count($reply[$n])) != ($n2=count(@$this->constraints[$n]))){
	//	$this->dump($a,'solution '.$n);
	// myPear::DEBUG(sprintf('n=%s ??? count(reply)=%s != count(constraints)=%s',$n,$n1,$n2));
      }
    }
    return $reply;
  }
  
  protected function same($ar1, $ar2) {
    return $ar1[$this->id]==$ar2[$this->id];
  }

  protected function clashing($ar1, $ar2) {
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
    if ($this->timeMn === Null) $this->timeMn = +999999999999;
    if ($this->timeMx === Null) $this->timeMx = -999999999999;
    if (is_array($r)) {
      $d1 = $r[$this->start];
      $d9 = $r[$this->end];
    }else{
      $d1 = $d9 = $r;
    }
    $this->timeMn = min($this->timeMn, $d1, $d9);
    $this->timeMx = max($this->timeMx, $d1, $d9);
    //    if ($this->debug) myPear::DEBUG(__METHOD__.'() '.b_time::period($this->timeMn,$this->timeMx));
  }

  function dbg($text=' ',$forced=False){
    b_debug::xxx($text,array((bool)$this->debug||$forced,3,'orangeText'));
  }
}
