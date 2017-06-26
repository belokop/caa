<?php

class b_table{

  var $reallyPrint = True;
  var $tr = 0;
  var $td = 0;
  var $th = 0;
  var $headerPrinted = False;
  var $thead_opened = False;  
  var $thead_closed = True;  
  var $tbody_opened = False;  
  var $nRows = 0;
  var $nRowsForPendingObject = 20;
  var $lines  = array();
  var $line   = array();
  var $format = "%-s%33s %s"; 
  var $maxL   = 32;
  var $delimiter = array();
  var $CLI = False;
  var $pendingObject = array();
  var $extraTD       = array();  
  var $tbodyID = '';
  var $highlightRows = True;
  var $b_table_div= False;
  protected static $table_number = 0; 
  private $n_pendingObjects = 0;
  private $tableOpened = 0; 
  private $tableClosed = 0; 

  public $attr_table = '';

  /*
   *
   */
  function __construct($attr='',$prefix='') {
    $this->CLI = cnf_CLI;
    $this->tNumber = ++self::$table_number;
    $this->attr_table = $attr;
    if ($prefix && !cnf_CLI) print "\n$prefix\n"; 
  } 

  function __toString() {
    return get_class($this).' '.$this->tNumber;;
  }
  
  protected function open($attr=''){
    if (!(bool)($this->tableOpened++)){
      if (empty($attr)) $attr = $this->attr_table;
      $this->ID = "b_table".$this->tNumber;
      $this->line = array();
      $this->prt_tag(sprintf("%s\n<table id='b_table%d' %s> <!-- open table %d -->\n",
			     ($this->b_table_div ? "\n<div id='b_table_div".$this->tNumber."'>" : ""),
			     $this->tNumber,
			     $this->_parseAttr($attr,'b_table'),
			     $this->tNumber)); 
    }
  }
  
  function getHeader($forced=False,$txt='') {}

  /*
   *
   */
  protected function _parseAttr($attr,$imposedClass=array(),$droppedClass=array()){
    if (!is_string($attr)) $attr = '';

    // Parse the attributes, optinally impose a class
    $attr_array = preg_split('/\s/',
			     preg_replace('/\s?=\s?/','=',str_replace(array('"',"'"),'',trim($attr))));
    $reply_array = $items = array();
    if (!empty($imposedClass)){
      if (!is_array($imposedClass)) $imposedClass = array($imposedClass);
      $items['class'] = $imposedClass;
    }
    foreach ($attr_array as $i){
      if (!trim($i)) continue;
      if (strpos($i,'=') !== False){
	list($current_item,$v) = explode('=',$i,2); 
	$current_item = trim(strToLower($current_item));
	$items[$current_item][] = trim($v);  
      }else{
	$items[$current_item][] = trim($i);  
      }
    }

    // "b_wrap" has precidense over "b_nowrap"
    if ($class = @$items['class']){
      if (in_array('b_nowrap',$class)) $droppedClass[] = 'b_wrap';
      $items['class'] = array_diff(array_unique($items['class']),$droppedClass);
    }

    // Build the reply
    foreach($items as $item=>$values)  $reply_array[] = "$item='".join(' ',array_unique($values))."'";
    return ' '.join(' ',$reply_array);
  }

  function thead_o($txt=''){
    if (!$this->thead_opened){
      $this->prt_tag("\n<thead>");
      if (!empty($txt)) $this->prt_tag(x('tr',x('td colspan=9',$txt)))."\n";
      $this->thead_opened = True;
      $this->thead_closed = False;
    }
  }

  function thead_c(){
    if ($this->thead_opened && !$this->thead_closed){
      $this->prt_tag("\n</thead>");
      $this->thead_opened = False;
      $this->thead_closed = True;
    }
  }

  function tro($attr='')         { 
    $this->open();
    $this->thead_c();

    if ($this->thead_closed && !$this->tbody_opened){
      $this->prt_tag("\n<tbody $this->tbodyID>");
      $this->tbody_opened = True;
    }

    $this->prt_tag("\n <tr" .$this->_parseAttr($attr).">"); 
    $this->tr++; 
    $this->nRows++; 
  }
  function tdo($attr='')         { $this->prt_tag("\n  <td".$this->_parseAttr($attr).">"); $this->td++; }
  function tho($attr='')         { $this->prt_tag("\n  <th".$this->_parseAttr($attr).">"); $this->th++; }
  function tdc()                 { $this->prt_tag("</td>");      $this->td--; }
  function thc()                 { $this->prt_tag("</th>");      $this->th--; }

  function trc($showPending=True){ 
    if ($showPending){
      if(($e=array_unique($this->pendingObject)) && !$this->n_pendingObjects){
	//	if (!bForm::$inputToPDF) 
	$this->td();
	$this->td(str_replace("'",'"',join("<br/>",$e)),  // tcpdf does not like "'"...
		  str_replace("'",'"'," rowspan='$this->nRowsForPendingObject' class='b_pending'"));
	$this->pendingObject = array();
	$this->n_pendingObjects++; 
      }
    }
    $this->w('TD',$this->td);
    $this->w('TH',$this->th); 
    $this->prt_data(); // to keep the empty cell narrow
    $this->prt_tag("\n </tr>"); $this->tr--; 
  }
  
  static $b_counter = 0;
  function close($postfix=''){
    if ((bool)$this->tableOpened && !(bool)($this->tableClosed++)){
      $this->w('TR',$this->tr);
      $this->w('TD',$this->td);
      $this->w('TH',$this->th); 
      if ($this->n_pendingObjects) for ($n=$this->nRows; $n < $this->nRowsForPendingObject; $n++) $this->tr();
      $this->prt_tag("\n".($this->tbody_opened?"</tbody>\n":'').
		     "</table>".($this->b_table_div ? "\n</div>" : "")." <!-- close table $this->tNumber -->\n",True); 
      $this->getRollOver();
      if ($postfix) $this->prt_tag("$postfix\n");
    }
  } 

  function getRollOver() {
    if (function_exists('bJS') && $this->tbody_opened && $this->nRows > 10 && $this->highlightRows){
      bJS()->add_js_file('highlightRow.js');
      bJS()->add_js_inline("addTableRolloverEffect('b_table$this->tNumber','hightlighRollOver','hightlighMarked');");
    }
  }
  
  function tr($body=' ',$atTD='',$atTR=''){
    if ($thisIsTH = is_array($atTD)) $atTD = @$atTD[0];
    $this->tro($atTR);
    $this->td($body,$atTD,$thisIsTH);
    $this->trc(); }
  
  function th($body=' ',$attr='') {
    $this->td($body, $attr, ($thisIsTH=True)); 
  }
  
  /*
   * the attributes might be packed into the array keys
   */
  function td($body=' ',$attr='',$thisIsTH=False){
    if (!is_array($body)) $body = array($body);
    foreach ($body as $a=>$td){
      if ($thisIsTH){
	$this->tho(is_numeric($a) ? $attr : $a);
	$this->prt_data($td);
	$this->thc(); 
      }else{
	$this->tdo(is_numeric($a) ? $attr : $a);
	$this->prt_data($td);
	$this->tdc(); 
      }
    }
  }
  
  function x($atr,$text=" ") {
    if ($this->CLI) return trim($text)."\n";
    if ($atr) return "<$atr>$text</".trim(preg_replace("/ .*/",'',$atr)).">";
    else      return trim($text); }

  function w($ID,$counter) { 
    if ($counter != 0){
      $this->prt_tag("<br/>\n".$this->x("font color='red'","$ID counter = $counter  table=$tableClassCounter")); 
    }
  }

  /*
   * Print the cell data
   */
  private function prt_data($text='') {
    if ($this->reallyPrint){
      // E.g. replace commas by '<br/>'. 
      // Not a bright idea, but sometimes usefull
      foreach($this->delimiter as $from=>$to){
	$text = str_replace($from,$to,$text);
      }
      if (cnf_CLI) $text = str_replace('&nbsp;', ' ', strip_tags($text))."\n";
      print $text;
    }
  }

  protected function prt_tag($text,$force=False) {
    $this->open();
    if ($this->reallyPrint && !cnf_CLI) print $text;
  }
  
  function utf8_encode($str){
    if ($str === mb_convert_encoding(mb_convert_encoding($str, "UTF-32", "UTF-8"), "UTF-8", "UTF-32")){ 
      return $str;
    }else {
      return utf8_encode($str);
    }
  }

  function prt($row,$highlight='') {} 
}

class box extends b_table {
  function box($cmds,$tt='code',$br='<br/>',$bgcolor='#E3EAF6') { 
    $this->open("class='b_box'");
    $this->tr($this->x($tt,nbsp(join("$br\n",$cmds))));
    $this->close();
  }
}

/*
 *
 */
class b_table_simple extends b_table {
  function __construct($items=array(),$id='',$attr=''){
    if (!is_array($items)) $items = array($items);
    //    print "<div $id>";
    $this->tNumber = ++self::$table_number;
    $this->open($attr);
    $this->tr($items);
    $this->close();
    //    print "</div>";
  }
}

/*
 *
 */
class b_table_dummy extends b_table {
  var $reallyPrint = False;

  protected function open($attr='') {} 

  function close($postfix=''){}
}

/*
 *
 */
class b_table_zebra extends b_table {

  var $header          = array();
  var $css             = array();
  var $class           = array();
  var $colspan         = array();
  var $keepRepetValues = True;
  var $repetValuesCol1only= False;
  var $skipDuplicatedLines= False;
  var $showLineCounter    = True;
  var $autoBreakLine      = False;
  var $afirst          = False;
  var $hiddenGET       = '';
  var $BGcode          = 0;
  var $counter         = 0;
  var $th_attr    = array();
  var $preHeaders = array();
  private $justPrintedTableRow = '';

  /*
   * Constructor
   * @param - (array) $header - associated array with the rows description 
   * @param - $atT - optional table attributes
   * @param - $noSort - flag shpuld the table be sortable or not
   */
  function __construct($header=array(),$atT='',$noSort=False) {
    $this->tNumber = ++self::$table_number;
    $this->header = $header;
    $this->noSort = $noSort;
    // Optionally impose minimal table width
    if (is_numeric($atT) && $atT > 0) $atT = " style='min-width:".(count($header) * $atT)."px;'";
    $this->attr_table = "class='b_table-zebra' $atT";
    if (empty($_GET['sByDir'])) $_GET['sByDir'] = 'ASC';
  }
  
  function tro($attr=''){
    $this->open();
    $this->prt_tag("\n <tr".$this->_parseAttr($attr,'b_table-zebra').">");
    $this->tr++; 
    $this->nRows++; 
  }

  function tdo($attr=''){ 
    if ($this->autoBreakLine) $attr .= ' b_wrap';
    $this->prt_tag("\n  <td ".$this->_parseAttr($attr).">"); 
    $this->td++; 
  }
  
  /*
   *
   */
  function getHeader($forced=False,$txt='') {  
    if (!$this->headerPrinted || $forced){
      $this->headerPrinted = True;
      if (is_array($this->header)){
	$this->thead_o($txt);

	// optional "pre-header" ... rather dark place...
	foreach ($this->preHeaders as $h2){
	  print "<tr><th></th>\n";
	  foreach($h2 as $item) { 
	    list($span,$text) = explode(':',$item,2);
	    $style = ($text ? 'border-bottom:1px solid;':'');
	    $space = ($item === '1:' ? ' highlightShadow' : '');
	    printf("<th colspan='%d' style='%s' class='align_right %s'>%s</th>\n",$span,$style,$space,$text);
	    //print "<th colspan='$span' ".($text?"style='border-bottom:1px solid;'":'')." class='align_right'>$text</th>\n";
	  }
	  print "</tr>\n";
	}
	
	// Optionally reorder columns 
	if (!$this->noSort && ($c = $this->repetValuesCol1only)) {
	  if ($h[$c] = @$this->header[$c]){ // be sure that the field is in the header
	    foreach ($this->header as $k=>$v) if ($k != $c) $h[$k] = $v; 
	    $this->header = $h;
	  }
	}
	
	// process "always first column"
	foreach ($this->header as $k=>$v) if (stripos($k,'_afirst')!==False) { $h = array($k => $v); $this->afirst = $k; }
	if ($this->afirst) {
	  if ($this->showLineCounter) {
	    myPear::WARNING_T("showLineCounter is set to 'False'");
	    $this->showLineCounter = False;
	  }
	  foreach ($this->header as $k=>$v) if ($k != $this->afirst) $h[$k] = $v;
	  $this->header = $h;
	}
	
	print "<tr><th></th>\n";	 
	foreach ($this->header as $k=>$v) printf("<th %s>%s</th>",@$this->th_attr[$k],$this->sortlink($v,$k));
      //foreach ($this->header as $k=>$v) print "<th ".@$this->th_attr[$k].">".$this->sortlink($v,$k)."</th>";
	print "</tr>\n"; 
	
	$this->thead_c();
	if (!$this->tbody_opened) $this->prt_tag("<tbody $this->tbodyID>");
	$this->tbody_opened = True;
      }else{
	if (class_exists('b_debug',False)) b_debug::internalError("header is not an array",$this);
      }
    }
  }

  /*
   * To write or not the anchor
   */
  private function anchor(){
    $reply = (empty($this->extraTD['anchor'])
	      ? ''
	      : "\n<tr><td><a name='".$this->extraTD['anchor']."'></a></td></tr>  <!-- anchor -->\n");
    unset($this->extraTD['anchor']);
    return $reply;
  }
  
  /*
   * produce a "gray zebra" listing
   */
  private $CLI_buffer = array();
  function prt($row,$highlight='') { 
    if (empty($row)) return;
    bTiming()->cpu('b_table');

    if (cnf_CLI){
      foreach ($this->header as $k=>$v)  if (!empty($v)) $this->CLI_buffer[$k] = $v;
    }else{    
      if ($this->repetValuesCol1only) $this->keepRepetValues = True;
      $this->getHeader();
      
      // do not print the same line twice
      $test = '';
      if ($this->skipDuplicatedLines) {
	foreach ($this->header as $k=>$v)  $test .= preg_replace("/[\W]/",'',strip_tags($row[$k]));
	$test .= serialize($this->extraTD);
	if ($test == $this->justPrintedTableRow) {
	  bTiming()->cpu();
	  return;
	}
	$this->justPrintedTableRow = $test;
      }
      
      print $this->anchor(); 

      if ($this->BGcode == 1) $this->BGcode=2;
      else                    $this->BGcode=1;
      $this->tro(); 
      $counterPrinted = 0;
      $colspan = 1;
      foreach ($this->header as $k=>$v) {
	if (--$colspan > 0)              continue;
	if (stripos($k,'hidden') === 0)  continue;
	if (substr(@$row[$k],0,1)==='<') $data = $row[$k];
	else	                         $data = str_replace("~","<br/>",@$row[$k]);
	if (empty($data)) $data = '';

	if     (@$this->afirst == $k)                        { $cell = $this->getIfChanged($k,$data); $cellF=$cell; }
	elseif ( $this->repetValuesCol1only == $k)             $cell = $this->getIfChanged($k,$data);
	elseif (!$this->keepRepetValues && !preg_match('/value/i',$k)) $cell = $this->getIfChanged($k,$data);
	else                                                   $cell = $data;
	
	if ($this->repetValuesCol1only == $k && $this->afirst && $cellF)  $cell = $data;
	if (!$counterPrinted++){
	  if ($cell && $this->showLineCounter) $c = (++$this->counter).'&nbsp;-&nbsp;';
	  else                                 $c = '&nbsp;';
	  $this->td($c,"class='align_right'");
	}
	if (!($text_align = @$this->css[$k]['align'])){
	  $text_align = (!preg_match(';\..*\.;',$data) &&  // strip out ip-addresses
			 preg_match("/^[0-9\/\-\.\,\%]+$/",b_fmt::unEscape(strip_tags($data)))
			 ? "align_right"
			 : "align_left");
	}
	//	if (is_array($this->class[$k])) 	var_dump($this->class[$k]);
	if (is_array(@$this->class[$k])) $this->class[$k] = join(' ',$this->class[$k]);
	$css_style = ($css=@$this->css[$k]['style']) ? " style='$css'" : '';
	$css_class = ($cls=@$this->class[$k])        ? " $cls" : '';

	// colspan
	if(($cs = @$this->colspan[$k]) > 0){
	  $colspan = $cs;
	  $cs = "colspan=$colspan";
	  unset($this->colspan[$k]);
	}else{
	  $cs = '';
	}
	$this->td($cell, "$cs class=b_table-zebra$this->BGcode $highlight ".$text_align.$css_class.$css_style);
      }

      if (empty($this->extraTD))   $this->extraTD = array();
      foreach($this->extraTD as $attr=>$td){
	$a = (is_numeric($attr) ? '' : $attr); 
	$this->td($td,'class="width0" '.$a); 
      }
      $this->trc(); 
    }
    $this->extraTD = array();
    bTiming()->cpu();
  }

  /*
   *
   */
  public function reset_counter($new_value=0){
    $this->counter = $new_value;
  }

  /*
   *
   */
  public $ignoreColor_for_changes = False;
  private static  $getIfChanged_cache = array();
  private function getIfChanged($item,$value) {
    if (!empty($this->class[$item]) && is_array($this->class[$item])) $this->class[$item] = join(' ',$this->class[$item]);
    if ($this->ignoreColor_for_changes) $hash = strip_tags($value);
    else                                $hash = $value . @$this->class[$item]; 
    //    if (empty($hash)) $hash = $value;

    $reply = ((($hash=b_crypt::hash($hash)) === @self::$getIfChanged_cache[$item]) 
	      ? '' 
	      : $value);
    self::$getIfChanged_cache[$item] = $hash;
    return $reply; 
  }

  public function reset_changes_cache(){
    self::$getIfChanged_cache = array();
  }

  private function sortlink($value,$code) { 
    if ($this->noSort || preg_match('/_nosort$/i',$code)){
      return $value;
      $href = '#';
    }else{
      if (@$_GET['sByDir'] == 'DESC'){
	$order = 'ASC';
      }else{
	$order = 'DESC';
	$_GET['sByDir'] == 'ASC';
      }
      $arrow = bIcons()->get('i-arrow_'.$_GET['sByDir']);
      if (@$_GET['sBy'] == $code) $value = "$arrow$value";
      $href = b_url::same(array('sByDir'=>$order, 'sBy'=>$code));
      return "<a href='$href'>$value</a>";
    }
  }
}

/*
 * plugin to print table as a text file
 */
class b_table_text extends b_table{

  var $_filename = 'table.txt';
  var $maxLength = 45;
  var $lines     = array();   

  function __construct($_filename){
    $this->tNumber = ++self::$table_number;
    $this->_filename = $_filename;
  }
  
  protected function open($attr='') {
    $this->tableOpened++; 
  }
  
  function tro($attr='') { 
    $this->open();
    $this->tr++;
    $this->line = array();
  }

  function trc($attr='') { 
    if (trim(@$this->line[1])) $this->lines[] = array('','','');
    $N = 0;
    foreach (explode('-BR-',(string)@$this->line[4]) as $l) {
      if (@$this->line[2] === 'Comment') $this->line[2] = '';
      if ($N++) $this->lines[] = array('','',$l);
      else      $this->lines[] = array((string)@$this->line[1], (string)@$this->line[2], $l);
    }
    $this->tr--;
  }
  
  function td($body='',$attr='',$x=0){
    if (!is_array($body)) $body = array($body);
    foreach ($body as $td) $this->line[] = $this->strip_tags($td);
  }
  
  function close($postfix=''){
    if ((bool)$this->tableOpened && !(bool)($this->tableClosed++)){
      if (cnf_CLI){
	print_r($this->CLI_buffer);
	$this->CLI_buffer = array();
      }else{
	// find the string length, create the format
	$lenght = array(); 
	foreach($this->lines as $line){
	  foreach($line as $n=>$td) $lenght[$n] = max(@$lenght[$n],strlen($this->strip_tags($td))); 
	}
	$lenght[0] = 3; foreach ($lenght as $l) { if (!$l) $l=1; $format[] = "%-${l}s"; }
	
	// build the formatted text array
	$txt = array();
	foreach($this->lines as $n=>$line){
	  $txt[] = rtrim(sprintf(join(' ',$format), $line[0], $line[1], $this->strcut($line[2])));
	}
	
	if (!empty($txt)){
	  $txt[] = '';
	  $fp = fopen($this->_filename, 'w');
	  fwrite($fp, join("\n",$txt));
	  fclose($fp);
	}
      }
    }
  }
  
  function tdo($attr='')  { $this->td++; }
  function tdc()          { $this->td--; }
  function tho($attr='')  { $this->tdo(); }
  function thc()          { $this->tdc(); }
  function th($b='',$a=''){ $this->td(); } 
 
  private function strip_tags($text){
    static $escapes = array('&#146;' =>"'", 
			    '&#148;' =>'"',
			    '&#44;'  =>',',
			    '&#46;'  =>'.',
			    '&#062;' =>'>',
			    '&#060;' =>'<',
			    '&lt;'   =>'<',
			    '&gt;'   =>'>',
			    '\['     =>' ',
			    '\]'     =>' ',
			    '&nbsp;' =>' ');
    $text = $this->utf8_encode($text);
    foreach ($escapes as $f=>$t) $text = preg_replace("/$f/i",$t,$text);
    $text = strip_tags(preg_replace('/<br[\s\/]?>/','-BR-',$text));
    
    return trim($text ? $text : '');
  }

  private function strcut($text){
    return $text;
    if (($lng=strlen($text)) > $this->maxLength){
      if ($pos = strpos($text, ' ', $this->maxLength)){
	$text  = substr($text,0,$pos);
	if (strlen($text) < $lng) $text .= '...';
      }
    }
  }
}
