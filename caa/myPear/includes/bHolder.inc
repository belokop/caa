<?php
/*
 * a holder class to keep & manage the forms
 */

class bHolder extends myPear_base{

  protected $css = array();
  var $strLength = 30; // number of characters in the item, cut if longer
  var $sortBy_default    = 'av_lastname';  
  var $noSort = False;
  var $showLineCounter = True;
  var $maxLines = 999999;
  var $title  = '';
  var $title2 = '';
  var $title3 = '';
  var $preHeaders= array();
  var $th_attr   = array();
  var $t         = Null; // table class
  var $styles = '';
  var $debug  = False;
  public $where = array();

  public $report_emptySelection = True;
  public $close_t_on_exit = True;

  protected $left_join = array();

  /*
   *
   */
  public function loadWidgets_array($href,$args=array()){
    static $saved_icons, $_GETsv, $counter=0;
    $this->dbg(++$counter);
    switch($href){
    case 'open': // ----------------------------------- open
      
      ob_start();
      
      print "<!-- open ".__METHOD__." -->\n";
      bIcons()->toggleExplainIcons(False);

      $saved_icons = bIcons()->reset();
      
      $this->showLineCounter = False;
      $this->close_t_on_exit = False;
      if (!is_object($this->t)){
	$this->t = new b_table_zebra($this->header());
	$this->t->th_attr = $this->th_attr;
      }

      foreach($args as $k=>$v){
	if ($k == 'h') $this->t->tr(x('h3',$v));
      }
      $widget = ob_get_contents();
      ob_end_clean();
      break;

    case 'close': // ----------------------------------- close
      ob_start();
      $this->listEntries_closing();
      print "<!-- close ".__METHOD__." -->\n";
      $widget = ob_get_contents();
      ob_end_clean();

      bIcons()->toggleExplainIcons();
      bIcons()->reset($saved_icons);
      break;
      
    default:     // ----------------------------------- populate
      $_GETsv= $_GET;
      $_GET  = b_crypt::parse_query_string($href);
      ob_start();
      if (empty($_GET['av_id'])) $_GET['av_id'] = @$_GET['id']; 
      $this->drawHeader_counter = 0;
      $this->listEntries();
      $widget = ob_get_contents();
      ob_end_clean();
      $_GET = $_GETsv;
    }
    if (0) b_debug::var_dump($widget,b_fmt::escape(sprintf("%s(%s,%s) exíting...",__FUNCTION__,$href,join(',',$args))));
    return $widget;
  }



  public static function loadWidget($href,$debug=Null){
    $_GETsv= $_GET;
    $_GET  = b_crypt::parse_query_string($href);
    if (empty($_GET['av_id'])) $_GET['av_id'] = @$_GET['id']; 

    $widget = '';
    if ($_GET['holder'] == 'Logs'){
      myPear_logs()->debug = (bool)$debug;
      myPear_logs()->report_emptySelection = empty($_GET['e_once']);
      $widget = myPear_logs()->getWidget();
    }else{
      if (locateAndInclude($h = 'bHolder_'.$_GET['holder'])){
	$h = new $h(($arg=@$_GET['id']) ? $arg : @$_GET['mode']);
	if ($debug !== Null) $h->debug = $debug;
	$h->report_emptySelection = empty($_GET['e_once']);
	$widget = $h->getWidget();
      }
    }
    $_GET = $_GETsv;
    return $widget;
  }

  protected $widget_align = 'centralized';
  function getWidget($header=''){
    ob_start();
    bIcons()->toggleExplainIcons(False);
    $saved_icons = bIcons()->reset();
    print "\n<div class='$this->widget_align'> <!-- open $this widget -->\n";
    print x('h3',($header?$header:$this->title));
    $this->listEntries();
    print "\n</div>  <!-- close $this widget -->\n";
    $widget = ob_get_contents();
    ob_end_clean();
    bIcons()->toggleExplainIcons();
    bIcons()->reset($saved_icons);
    return $widget;
  }


  function __construct(){
    locateAndInclude('b_table');
  }
  
  protected function new_t($setTitle=False){
    if (!(@$this->t instanceof b_table)) {
      // Copy the eventual b_table attributes to the proper class 
      if (!empty($this->t)) $this_t = $this->t;
      $header = $this->header();  // sets also title
      if ($setTitle){
	if (!empty($this->title)) myPear::H1($this->title,'noTranslate');
	print $this->title2;
      }
      $this->t = new b_table_zebra($header,$this->styles);
      $this->t->th_attr   = $this->th_attr;
      $this->t->preHeaders= $this->preHeaders;
      // ??? far too dangerous $this->t->delimiter[','] = '<br/>'; // replace ',' by '<br/>'
      $this->t->noSort    = $this->noSort;
      $this->t->showLineCounter = $this->showLineCounter;
      $this->t->repetValuesCol1only = @$_GET['sBy'];
      $this->t->css = $this->css;
    }
  }

  private $drawHeader_counter = 0;
  function drawHeader(){
    $this->new_t(True);
    if (!$this->drawHeader_counter++){
      // Copy holder styles to the table 
      $this->t->getHeader($forced=True,$this->title3);
      if (empty($this_t)) $this_t = array();
      foreach($this_t as $k=>$v) $this->t->$k = $v;
    }
    return $this->t;
  }
  
  public $custom_header=array();
  function header(){
    if (!empty($this->custom_header)) return $this->custom_header;
    else                              return array();
  }

  function listEntries(){
    bTiming()->cpu(__FUNCTION__);
    $this->_open();

    // Sanity, clean the "sort by" agruments
    $header = $this->header();  // sets also title
    foreach(array('sBy',) as $f){
      if (!isset($_GET[$f]) || empty($header[$_GET[$f]])){
	$_GET[$f] = $this->sortBy_default;
      }
    }

    $nLine = 0;
    if ($q = $this->query()){
      while ($this->rec = myPear_db()->next_record($q)){
	if (!$this->rejectRecord()){
	  if (!empty($this->rec)){	  
	    $this->drawHeader();
	    $this->format();
	    if (!empty($this->rec)){	  
	      $this->t->prt($this->rec);    
	      if (++$nLine > $this->maxLines){
		myPear::WARNING("$this->maxLines \"".str_replace('bHolder_','',get_class($this))."\" lines printed... Output stops");
		break;
	      }
	    }
	  }
	}
      }
    }

    if ($this->close_t_on_exit)   $this->listEntries_closing();
    
    bTiming()->cpu();
  }
  
  /*
   *
   */
  protected function listEntries_closing(){  
    if (@$this->t instanceof b_table){
      $this->dbg('b_table');
      $this->t->close();
      $this->_close();
      bIcons()->explain();
    }else{
      $this->dbg();
      $this->_emptySelection();
      $this->_close();
    }
  }

  function noSuccess(){ 
    if ($this->report_emptySelection) $this->_emptySelection(); 
  }
  
  function _emptySelection(){
    if ($this->report_emptySelection) myPear::INFO(EMPTY_SELECTION); 
  }

  protected function _open()       {}

  protected function _close()      {}

  function query()       { return False; }

  function rejectRecord(){ return False; }

  function format(){}
}
