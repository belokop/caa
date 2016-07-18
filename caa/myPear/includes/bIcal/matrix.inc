<?php
locateAndInclude('bIcal');
locateAndInclude('bCover');

class bIcal_matrix extends bIcal {
  
  /*
   * The way to treat arrival/departure date is ambiguous,
   * sometimes the "arrival-date + departure-date = 1 day" is used, sometimes
   * it is 2 days
   * Flag saying "show the last-day-of-trip as a trip".
   */
  public    $see_day9 = False;

  public    $day1 = 0;
  public    $day9 = 0;
  public    $d1   = 'day1';
  public    $d9   = 'day9';
  protected $step = 1;

  public    $noNavig = False;
  public    $noNavig_arrows = False;
  public    $noNavig_months = False;

  public    $footer  = array();
  public    $colorsUsed = array();
  public    $randomColors = array('#2BDA39','#FDFA3D','#FDDAD9','orange',
				  '#26D4FE','#9EC633','#dd9988',);
  public    $showThumbnail = False;
  public    $showLegend    = True;
  public    $hideCheckBox  = False;

  public    $allowCollapse = False;
  public    $collapseDate  = 0;
  public    $collapseCounter = 0; // number of lines in the collapsed picture
  public    $t               = Null; // table class
  public    $tClass          = NULL;

  private $headerPrinted = False;
  private $dontClose     = False;
  
  function __construct(){
    $this->collapseDate = (int)@$_GET['collapse_once'];
    $this->nDaysToShow  = CONST_ical_cells_to_show * $this->step;
    $this->day1 = b_cnf::get(b_reg::$current_module.'_day1');
    $this->day9 = b_cnf::get(b_reg::$current_module.'_day9');
    if (empty($this->day1))    $this->day1 = $this->f_sunday(time());
    if (empty($this->day9))    $this->day9 = $this->day1 + $this->nDaysToShow * 86400;
    if ($this->day1 >= $this->day9){
      $this->day1 = $this->f_sunday(time());
      $this->day9 = $this->day1 + $this->nDaysToShow * 86400;
    }
    parent::__construct();
  }

  /*
   * Default function to render a simple calendar matrix
   */
  function show($matrix) {
    $this->showHeader();
    $this->showBody($matrix);
    if ($this->collapseDate) $this->showHeader($onlyDates=True);
    $this->showFooter();
  }

  /*
   * Show the matrix header (months & dates)
   */
  static $iCal_conter = 0;
  function showHeader($onlyDates=False){

    if (!defined('DAY1')) define('DAY1' , $this->d1);
    if (!defined('DAY9')) define('DAY9' , $this->d9);
    if (!defined('NAME')) define('NAME' , 'name');

    if ($this->collapseDate < $this->day1) $this->collapseDate = 0;
    if ($this->collapseDate > $this->day9) $this->collapseDate = 0;

    if (!$this->headerPrinted){
      $this->collapseCounter = 0;
      if (!is_object($this->t)){
	printf("<div id='bIcal%d'>\n",(++self::$iCal_conter));
	$this->t = new b_table("class='bIcal'");
      }
      $this->t->highlightRows = False;
      $this->t->nCols = (int)(($this->day9 - $this->day1) / 86400 + 0.5);
      
      /*
       * Top line - months and left/right navigation buttons (left/right arrows)
       */
      $this->t->thead_o();  
      $this->t->tro();  
      $this->t->td('',"class='bIcal_name'"); 
      $this->t->td($this->arrow_left(),"class='bIcal_arrow_left'");
      $class = "class='bIcal_nav page_load_progress'";
      $day1  = $this->day1;

      // Show the month name (above the dates, which come after). Add also the year if there is enough space.
      // When the iCal mode is "cell/week" there is not much space to print the name of the each month,
      // so do it with gaps.
      while ($day1 < $this->day9) {
	// start from January in the cell/week mode...
	$step = ($this->step==1 || $day1==$this->day1 ? 1 : 3);
	$day1_nextMonth = min(b_time::noon(date('Y',$day1),date('m',$day1)+$step,1) , $this->day9);
	$label = array();
	$nCells_avaialable = (int)(($day1_nextMonth-$day1)/86400/$this->step+0.5);
	if ($nCells_avaialable){
	  if (($this->step > 1) && $day1 == $this->day1){
	  }else{
	    $m = b_time::strftime('%h',$day1);
	    if ($nCells_avaialable > strlen($m))        $label[] = $m;
	    if ($nCells_avaialable > strlen($m)+4)      $label[] = strftime('%Y',$day1);
	  }
	  $a = ($this->noNavig||$this->noNavig_months ? 'x' : 'a');
	  $this->t->th(x("$a $class href='".b_url::same('?'.b_reg::$current_module.'_day1='.$this->f_sunday($day1))."'",join('&nbsp;',$label)),
		       "colspan='$nCells_avaialable' $class"); 
	}
	$day1 = $day1_nextMonth;
      }
      $this->t->td($this->arrow_right(),"class='bIcal_arrow_right'");
      $this->t->trc();
      $this->t->thead_c();  
    }
    
    /*
     * Dates, normally the second line of the header.
     * Optionally might be printed anywhere
     */
  if (!$this->headerPrinted || $onlyDates){
      $this->t->tro("class='bIcal_date'");
      $this->t->td(); 
      $this->t->td();
      for ($day = $this->day1; $day <  $this->day9; $day+=86400*$this->step){
	$value = strftime('%e',$day);
	if ($this->allowCollapse) $value = x("a href='".b_url::same("?collapse_once=$day")."'",$value);
	if (b_time::_($day) == b_time::_($this->collapseDate) && $this->collapseCounter>0){
	  $value .= "<br/><b style='color:white;text-align:center;'>$this->collapseCounter</b>";
	}
	if ($this->collapseDate && (b_time::_($day) == b_time::_($this->collapseDate))) $style = bIcal::dateColor($day,array(),array('bIcal_bgred'));
	else	                                                                        $style = bIcal::dateColor($day);
	$this->t->td($value,$style);
      }
      $this->t->trc();
    }
    $this->headerPrinted = True;
  }

  protected function arrow_left(){
    if ($this->noNavig||$this->noNavig_arrows)  return '';
    else return bIcons()->getButton(array('class'=>'page_load_progress',
						  'i'=>'i-arrow_left',
						  'c'=>'bIcal_nav',
						  'l'=>b_url::same('?'.b_reg::$current_module.'_day1='.$this->f_sunday($this->day1,'month-'))));
  }

  protected function arrow_right(){
    if ($this->noNavig||$this->noNavig_arrows)  return '';
    else return bIcons()->getButton(array('class'=>'page_load_progress',
						  'i'=>'i-arrow_right',
						  'c'=>'bIcal_nav',
						  'l'=>b_url::same('?'.b_reg::$current_module.'_day1='.$this->f_sunday($this->day1,'month+'))));
  }

  /*
   * Show the body
   */
  public function showBody($matrixP,$skipIfCollapsed=False){
    bTiming()->cpu(__FUNCTION__);
    // rebuild the matrix, impose 'checkin/checkout time and resolve possible clashes 
    $matrix = array();

    foreach ($matrixP as $name=>$line){
      foreach($line as $k=>$v) {
	$line[$k][DAY1] = b_time::check_in($v[DAY1]);
	$line[$k][DAY9] = b_time::check_out($v[DAY9]);
      }
      $solver = new bCover($line,'n',DAY1,DAY9);
      foreach($solver->solve() as $n=>$solution) $matrix[bIcal_formatName($name,$n)] = $solution;
    }

    foreach ($matrix as $name=>$line){
      if ($name) $this->showLine($name, $line,$skipIfCollapsed);
    }
    bTiming()->cpu();
  }

    /*
     * Show the footer
     */
  function showFooter(){

    if ($this->collapseDate)  $this->showHeader($onlyDates=True);
    $this->t->close(); 
    print "</div>\n";
    unset($this->t); 
    
    if (!empty($this->footer)){
      $msg = join('&nbsp;',$this->footer);
      $t = new b_table("class='bIcal'");
      $t->tr($msg, "class='bIcal_name' colspan='".(($this->day9-$this->day1)/86400)."'");
      $t->close();
    }
    bIcons()->explain();
    $this->colorLegend();
    
    /*
     * Debug - error report
     */
    if (is_array(@$GLOBALS['clashICAL'])){
      foreach($GLOBALS['clashICAL'] as $Nline=>$line) {
	print x('strong',"<br/>clash in line $Nline<br/>");
	foreach ($line as $entry){
	  print $entry['name'].date('Y-m-d',$entry['v_start']).'/'.date('Y-m-d',$entry['v_end']).'&nbsp; ; &nbsp; ';
	}
      }
    }
  }
  
  /*
   *
   */
  private function showLine($name, &$line, $skipIfCollapsed=False){
    static $namePprev = '???';
    bTiming()->cpu(__FUNCTION__);
    
    $day1 = $this->day1;
    $entries = $this->sort($line);
    foreach (array_keys($entries) as $k){
      if ($entries[$k][DAY1] > $this->day9) continue;
      if ($entries[$k][DAY9] < $this->day1) continue;
      if ($cd = $this->collapseDate){
	if (b_time::_($entries[$k][DAY1]) > b_time::_($cd)) continue;
	if (b_time::_($entries[$k][DAY9]) < b_time::_($cd)) continue;
	if (!$skipIfCollapsed)  $this->collapseCounter++;
      }
      
      // Start the calendar row with the name. 
      // The "explicit" name takes precedence over the function argument
      if (!$this->t->tr) { 
	if (!($nameP = @$entries[$k][NAME])) $nameP = $name; 
	// set the name to blank if the object is expanded to a next row
	$nameP = preg_replace('/&nbsp;*/','&nbsp;',
			      str_replace('&nbsp','&nbsp;',
					  preg_replace('/;*$/','',$nameP)));
	if ($url = @$entries[$k]['url']) $nameP = x($url,$nameP); 
	$this->t->tro();
	$this->t->td(array(($this->showThumbnail ? $entries[$k]['icon'] : ''),
			   ($nameP==$namePprev ? '' : nbsp($nameP))),  "class='bIcal_name'");
	$namePprev = $nameP;
      } 
      
      // add 1 day if the departure date should be shown on the graph
      // Insert (see a few lines below) a "yellow day - In/Out" if there is no gap between 2 sequential visits
      if ($this->see_day9){	 
	$gap = (is_array(@$entries[$k+1]) ? b_time::diff($entries[$k+1][DAY1], $entries[$k][DAY9]) : 999);
	if ($gap)    {if(!@$entries[$k]['adjusted'])       $entries[$k][DAY9]  += 86400; }
	else         {     $entries[$k+1]['adjusted']=True;$entries[$k+1][DAY9]-= 86400;  }
      }else{
	$gap = 999;
      }

      // make sure the entries are within the matrix limits
      $entries[$k][DAY1] = max($entries[$k][DAY1], $this->day1);
      $entries[$k][DAY9] = min($entries[$k][DAY9], $this->day9);

      // show the "before stay" days
      $skipDays = b_time::diff($entries[$k][DAY1], $day1);
      if ($skipDays < 0) { $GLOBALS['clashICAL'][$nameP] = $entries; continue; }// clashing entry, should not happen
      if ($skipDays > 0){
	for ($n=1; $n<=$skipDays; $n=$n+$this->step) $this->t->td('',bIcal::dateColor($day1+($n-1)*86400)); 
      }
      
      // show the stay period as a colored line
      $staingDays = (int)(b_time::diff($entries[$k][DAY9], $entries[$k][DAY1])/$this->step);
      if ($staingDays > 0){
	$text_saved = @$entries[$k]['text'];
	$entries[$k]['text'] = b_fmt::strcut($text_saved,(int)(1.5*$staingDays),'...',$staingDays>3);
	$this->t->td(bJS()->tip(x(@$entries[$k]['styl'],$entries[$k]['text']), @$entries[$k]['tips']),
		     "class='".@$entries[$k]['clas']."' colspan='$staingDays'");
	if (!$gap){
	  $this->t->td(bJS()->tip('<i>IO</i>',$this->IO_tip($entries[$k][DAY9],$text_saved,$entries[$k+1]['text'])),
		       "class='bIcal_inoutday'");
	}
      }
      $day1 = $entries[$k][DAY9];
      if (@$entries[$k]['adjusted']) $day1 += 86400;
    }
    if ($this->t->tr) { 
      for ($n=1; $n <= b_time::diff($this->day9,$day1); $n=$n+$this->step){
	$this->t->td('',bIcal::dateColor($day1+($n-1)*86400));
      }
      $this->t->trc();
    }
    bTiming()->cpu();
  }
    
  protected function IO_tip($time,$text1='in',$text2='out'){
    return nbsp(formatTime($time) .': '. strip_tags($text1).' / '.strip_tags($text2));
  }

  function sort(&$line){
    $reply = $i = array();
    foreach ($line as $k=>$r) $i[$r[DAY1]] = $k;  
    ksort($i);  
    foreach ($i as $k=>$key)  $reply[] = $line[$key];
    return $reply;
  }

  private function _session($sid,$value=Null){
    if (empty($value)){
      $reply = @$_SESSION[__CLASS__][$sid];
    }else{
      $_SESSION[__CLASS__][$sid] = $value;
      $reply = Null;
    }
    return $reply;
  }

  /*
   * Get a random color from the pool.
   * Keep the color choice thru the session
   */
  function getRandomColor($legend){
      if (!($color = self::_session($sid=__FUNCTION__.$legend))){
	static $counter = 0;
	$color = $this->randomColors[(++$counter)%count($this->randomColors)];	
	self::_session($sid,$color);
      }
      return $this->getCSS($legend,$color);
  }

  function getCSS($legend,$color){
    static $color_was = Null;
    static $counter = 0;
    static $classes = array();

    if (!@$classes[$legend]){
      ++$counter;
      $classes[$legend] = "bIcal_css$counter";
      $this->colorsUsed[$legend] = $classes[$legend];
      if (empty($color)){
	if (empty($color_was)) $color_was = $this->randomColors[0];
	$color = $color_was;
	// if (cnf_dev) b_debug::traceBack("empty color");
      }else{
	$color_was = $color;
	bJS()->add_css_inline("td.".$classes[$legend]." { background-color:$color !important; }");
      }
    }
    return $classes[$legend];
  }

  private function f_sunday($date=Null,$mode='month',$direction='left'){
    $date = b_time::f_sunday($date,$mode,$direction);
    switch($this->step){
    case 1:   return $date;
    default:  return $date + 86400;
    }
  }

  private function define($c,$v,$d){
    if (defined($c) && $d != $v) b_debug_internalError("$c != $v");
    define($c,$v);
  }

  function colorLegend() {
    if ($this->showLegend)
      foreach ($this->colorsUsed as $legend=>$class) {
	if (!isset($t)) {
	  $t = new b_table("class='bIcal'");
	  $t->tro(); $t->th('<div class="align_left">Colors legend:</div>',"colspan=4"); $t->trc();
	  $t->tro(); $t->td('&nbsp;&nbsp;&nbsp;&nbsp;');
	}
	$t->td(b_fmt::strcut($legend,20),"class='$class'");
      }
    if (isset($t)) { $t->trc();    $t->close(); }
  }
}

function formatTime($v,$showTime=False) {
  return b_time::strftime("%Y-%m-%d".($showTime?' %H:%M':''),$v);
}

function bIcal_formatName($name,$nn){
  $reply = $name;
  for ($n=1; $n<$nn; $n++) $reply .= ';';
  return $reply;
}
