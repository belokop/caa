<?php
b_os::needPhpVersion('5.1',__FILE__);
$dblue= '#003366';
$blue = "#E3EAF6";

locateAndInclude('bIcal');
class bIcal_mini extends bIcal {
  
  var $ID = '';
  var $disableBefore= False;
  var $testing = False;
  var $opacBefore = 0;
  var $see_day9 = False;
  
  function __construct($mode=''){
    $this->mode = $mode;
    $this->ID   = mb_strToLower('i_'.preg_replace('/ |&nbsp;/','_',$mode));
    $this->opacBefore = b_time::_();
    unset($this->colors); // for debugging
    b_cnf::get("ical_".$this->ID."_start",b_time::noon(0,0,1));
    parent::__construct();
  }
  

  function reset() {
    if (!$this->testing) $_SESSION['ical'] = array();
  }

  function set($selectedDate=False,
	       $opacBefore=False,
	       $disableBefore=False,
	       $start=False) { 
    // disaster...if ($this->mode)    if (!$start)  $start = $selectedDate;
    if ($disableBefore) $this->disableBefore = $disableBefore;
    if ($opacBefore)    $this->opacBefore    = $opacBefore;
    if ($start)         $this->start         = b_cnf::get("ical_".$this->ID."_start",b_time::noon(0,0,1,$start));
    if ($selectedDate)  $this->selected      = $_SESSION['ical'][$this->ID."_selec"] = $selectedDate;
  }

  function setPeriod($start,$end){
    $this->start = b_time::_($start);
    $this->end   = b_time::_($end);
  }  

  function get() {
    if (!($date = (int)b_cnf::getValue("ical_".$this->ID."_start")))  $date = b_time::noon(0,0,1);
    if ($act = @$_GET[$this->ID."_start"]) {
      if ($act == 'today')     $date = b_time::noon(0,0,1);
      if ($act == 'nexty')     $date = b_time::noon(date('Y',$date)+1,              0,  1,$date);
      if ($act == 'prevy')     $date = b_time::noon(date('Y',$date)-1,              0,  1,$date);
      if ($act == 'nextm')     $date = b_time::noon(                0,date('n',$date)+1,1,$date);
      //if($act== 'prevm')     $date = b_time::noon(                0,date('n',$date)-1,1,$date);
      if ($act == 'prevm')     $date = b_time::noon(                0, 0, 1,$date - 2*86400); // '0' in b_time::noon means 'now'...
      $this->date = $date;
      if($_SESSION['ical'][$this->ID."_selec"]) 
	if (($this->disableBefore && $date > $this->disableBefore) ||
	    !$this->disableBefore)   
	  $_SESSION['ical'][$this->ID."_selec"] = $date;
      unset($_GET[$this->ID."_start"]);
    }
    b_cnf::get("ical_".$this->ID."_start",$date);
    if ($this->opacBefore < $date) $this->opacBefore = 0;
    
    if ($s=@$_GET[$this->ID."_selec"]) $_SESSION['ical'][$this->ID."_selec"] = $s;
    if ($this->disableBefore &&   $_SESSION['ical'][$this->ID."_selec"] < $this->disableBefore)
      unset ($_SESSION['ical'][$this->ID."_selec"]);
    
    b_cnf::_unset(array($this->ID."_start",
			$this->ID."_selec",
			'updatedb'));
		  
    return @$_SESSION['ical'][$this->ID."_selec"];
  }
  
  /*
   * Print the slall calendar
   *   $this->start - the first day of the period
   *   $this->end   - the  last day of the period
   */
  function prt($extra='') {
    $this_start = $this->start;
    $this_end   = $this->end;
    // work-around    
    $this->end += 24*3600;
    if ($this->see_day9) $this->end += 24*3600;
    if (cnf_dev) b_debug::xxx(b_time::period($this->start,$this->end,'full'));
    $this->get();
    if ($this->mode){
      $tt = new b_table("class='bIcal_border1'",$extra);
      $tt->tro();
      $tt->th($this->mode,"class='align_left'");
      $tt->trc();

      $tt->tro();$tt->tdo();
      $t = new b_table ("class='bIcal cal_border0'");
      $day1 = b_time::noon(0,0,1,b_cnf::getValue("ical_".$this->ID."_start"));
      $day9 = b_time::noon(0,(int)date('m',$day1)+1,1,$day1);
      $t->tro(); $t->th($this->anker('prevm').
			$this->anker('today',b_time::strftime("%B",$day1).' '.date("Y",$day1)).
			$this->anker('nextm'),"colspan=7 align=center");
      $t->trc();
    }else{
      $t = new b_table ("class='bIcal bIcal_border1'");
      //print date('Y-m-d',$this->start);
      //print date('Y-m-d',$this->end);
      $day1 = $this->start - 7*86400; while ((int)date("w",$day1)!=1) $day1 -= 86400;
      $day9 = $this->end   + 7*86400; while ((int)date("w",$day9)!=1) $day9 += 86400;
      $wToShow = 3;
      if (($this->end - $this->start) > 3*$wToShow*7*86400){
	$startBreak = $this->start + $wToShow*7*86400; 
	while ((int)date("w",$startBreak)!=1) $startBreak -= 86400;
	$endBreak   = $this->end   - $wToShow*7*86400; 
	while ((int)date("w",$endBreak)  !=1) $endBreak   += 86400;
      }
      $this->opacBefore = 0;
    }
    
    $linesPrinted = 0;
    for ($date = $day1; $date < $day9; $date += 86400) { 
      $weekDay = date("N",$date);
      if (!$linesPrinted++ || $weekDay == 1) {
	$t->tro(); 
	if (!$this->mode){ 
	  $id = b_time::strftime("%h %Y",$date); 
	  $t->td(@$monthP==$id ? '' : nbsp($id),"class='bIcal_dath'");
	  $monthP=$id;
	}
      }
      if ($linesPrinted == 1) 
	for ($s=1; $s < $weekDay; $s++) $t->td('',"class='bIcal_date'"); 
      if ($date >= @$startBreak && $date < @$endBreak-2*84600) {
	//print b_time::strftime("startBreak=%Y-%m-%d<br/>",$startBreak); print b_time::strftime("endBreak=%Y-%m-%d<br/>",$endBreak);
	for ($s=1; $s < 8; $s++) $t->td('.',"class='bIcal_visit'");
	$date += 7*86400;
	if (b_time::noon(0,date('m',$date)+1,1,$date) < @$endBreak) {
	  $date = b_time::noon(0,date('m',$date)+1,1,$date)+86400;
	  while ((int)date("w",$date)!=1) $date += 86400; $date -= 86400;  $t->trc(); 	  
	}else
	  $date = @$endBreak - 86400;
      }else{
	$this->drawDate($t,$date);
	if ($weekDay==7 && date("w",$date+86400)==1) $t->trc(); 
      }
    }
    for ($s=$weekDay; $s < 7; $s++) $t->td('',"class='bIcal_date'");
    if ($t->tr) $t->trc(); $t->close();
    if (!empty($tt)) { $tt->tdc(); $tt->trc(); $tt->close(); }
    if (!$this->mode) $this->reset();
    $this->start = $this_start;
    $this->end   = $this_end;
  }
  
  function drawDate ($t,$date){
    if ($this->opacBefore && b_time::_($date) < b_time::_($this->opacBefore)) $font = "span class='bIcal_opac'";
    else $font = '';

    $csel = 'bIcal_date';
    if (b_time::isRedDay($date))   $csel = 'bIcal_weekend';
    
    $s = @$_SESSION['ical'][$this->ID."_selec"];
    if ($s && b_time::_($date)==b_time::_($s))                               $csel = 'bIcal_selected';
    elseif(b_time::_($date) >= $this->start && b_time::_($date) < $this->end)$csel = 'bIcal_visit';
    $t->td($this->anker($date,x($font,date('j',$date)),$this->ID."_selec"),
	   "class='$csel'");
  }
  
  function anker($txt,$anker=False,$id=False) {
    if (!$this->mode) return $anker;
    if ($id == $this->ID."_selec" && $this->disableBefore) 
      if ($txt < $this->disableBefore) 
	return $anker;

    if (!$id) $id = $this->ID."_start";
    if (!$anker) $anker = eregi('next',$txt) 
      ? bIcons()->get('arrow_right')
      : bIcons()->get('arrow_left');
    return 
      x("a class='bIcal_nav' href=".x("'",b_url::same(array($id=>$txt, 
							     'updatedb'=>'net'))), 
	$anker);
  }
}
