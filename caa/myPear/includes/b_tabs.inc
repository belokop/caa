<?php
/*
 * Horizontal tabs.
 *
 * There are 2 versions of thabs 
 * - pure jQuery tabs
 * - myPear-style tabs.
 *
 * The reason to create the custom tabls is that the jQuery way is not perfect.
 * But some people like it...
 *
 * @synopsis:
 *   $b_tabs = new b_tabs(array('tab1','tab2'));
 *   foreach($tabs as $tab){
 *      // Start the next tab, if the tab is active
 *      // The jQuery tabs are always active.
 *      if ($b_tabs->body()){ 
 *          // fill it
 *          print "whatever goes to the tab"; 
 *      }
 *   }
 *   $b_tabs->close();
 */

// Default tabs are b_tabs_myPear
define('cnf_b_tabs_JQ',False);

if (cnf_b_tabs_JQ) {

  class b_tabs extends b_tabs_JQ{
    function __construct($tabs=array(),$selected=0){
      return parent::__construct($tabs,$selected);
    }
  }

}else{

  class b_tabs extends b_tabs_myPear{
    function __construct($tabs=array(),$selected=0){
      return parent::__construct($tabs,$selected);
    }
  }

}

/*
 * myPear tabs
 */
class b_tabs_myPear extends myPear_base{

  public $debug = False;
  public $current_tabTitle = '?';
  public static $jQueryTabs_depth = 0;
  public static $myPearTabs_depth = 0;
  public static $inside = False;

  public  $tabs         = array();
  private $n_symbols    = 0;
  private $n_symbols_ok = 30;
  public  $ID; // class counter, used for debugging
  public  $body = array();
  public  $body_prefix = '';

  private static $NN = 0;
  private $selected  = 0;
  private $_GET_b_tab = '';

  private function cash($args){
    if (is_array($args)) return trim(join(',',$args));
    else                 return explode(',',$args);
  }

  /*
   *
   */
  private $cnf_bTabs = 0;
  function __construct($tabs=array(),$selected=0){
   
    if (!function_exists('bJS')) require_once (dirname(__FILE__).'/myPear_functions.inc');
    print "\n<!-- ".__method__." start -->\n";

    // Load the styles
    bJS()->add_css_file('b_tabs.css');
    $this->ID = ++self::$NN;

    // Collect info about the "current tabs" (tabs on one page)
    // The idea is to prevent the session to use the "current tab" for the foreign instance of b_tabs
    if (0)    if (cnf_dev){
      $b_tab_stamp = $this->cash($tabs);
      
      // The tab level is set to 0 when the tab content is edited, take that into account.
      // It is decreased in order not to get a chance to change the tab which is edited
      $tab_depth = self::$myPearTabs_depth;
      if (($tab_depth == 0) && (count($tabs) == 1) && ($b_tab_stamp === @$_SESSION[__CLASS__][1])){
	if (0){
	  $tab_depth = 1;
        }else{
          // VERY UNRELIABLE !!!!
          $tabs = $this->cash($_SESSION[__CLASS__][0]);
          $b_tab_stamp = $this->cash($tabs);
        }
      }

      if (empty($_SESSION[__CLASS__][$tab_depth])) $_SESSION[__CLASS__][$tab_depth] = $b_tab_stamp;
      $fmt = __CLASS__."$tab_depth - %s '%s'";
      if ($updated=($_SESSION[__CLASS__][$tab_depth] != $b_tab_stamp)){
	b_debug::print_r($_SESSION[__CLASS__],
                         sprintf($fmt,'unknown',$b_tab_stamp));
        //      for($n=0; $n<4; $n++) b_cnf::_unset('cnf_btabs'.$n);
      }
      $_SESSION[__CLASS__][$tab_depth] = $b_tab_stamp;
      if ($updated) b_debug::print_r($_SESSION[__CLASS__],sprintf($fmt,'updated',''));
    }
    
    $this->dbg('');

    // Variable which keeps the current tab
    self::$myPearTabs_depth++;
    $this->cnf_bTabs ='cnf_btabs'.self::$myPearTabs_depth;

    // Clean the cache from tabs of lower level
    foreach($tabs as $k=>$tab){
      $tab = str_replace('<br>',' ',str_replace('<br/>',' ',$tab));
      if (class_exists('b_crypt',False)) $tab_hash = b_crypt::hash($tab);
      else                               $tab_hash = sprintf("%u",crc32($tab));
      $this->tabs[$tab_hash] = $_SESSION[__CLASS__.'debug'][$tab_hash] = $tab; 
      $this->n_symbols += (strlen($tab) + 4); // 2 spaces on the left & right sides
    }

    // Locate the current tab. Clean it from the cache if the latter is not valid any more
    if (isset($_GET[$this->cnf_bTabs])) b_cnf::set($_GET[$this->cnf_bTabs]);

    $t = array_keys($this->tabs);
    if (isset($this->tabs[$tab=b_cnf::get($this->cnf_bTabs)])){
      $this->_GET_b_tab = $tab;
    }else{
      b_cnf::set($this->cnf_bTabs,($_GET[$this->cnf_bTabs]=@$t[$selected])); 
      $this->_GET_b_tab  =  $_GET[$this->cnf_bTabs];
    }

    foreach($t as $k=>$v){
      if ($v == $this->_GET_b_tab) $this->selected = $k;
    }
    $this->current_tabTitle = @$this->tabs[$this->_GET_b_tab];
    $this->dbg($this->n_symbols);
    print "<!-- ".__method__." end -->\n";
  }

  /*
   * Get back the encoded tab name, used mainly for debugging
   */
  public static function code2tab($tab_hash){
    if (empty($_SESSION[__CLASS__.'debug'][$tab_hash])) $_SESSION[__CLASS__.'debug'][$tab_hash] = '???';
    return    $_SESSION[__CLASS__.'debug'][$tab_hash];
  }

  /*
   *
   */
  private $tab_counter=0;
  private $body_content = '';
  function body(){      
    $this->dbg("tab ".$this->tab_counter,cnf_dev);
    self::$inside = $this->isActive(0);
    $this->body_close();
    return self::$inside;
  }

  /*
   *
   */
  private function body_close($closing=False){
    $this->dbg(starting);
    print "<!-- ".__method__." start -->\n";
    if ($this->tab_counter > 0){
      $body_content = ob_get_contents();
      ob_end_clean();
      $this->dbg('ob_end_clean',cnf_dev);
      if ($this->isActive(-1))	$this->body_content = $this->body_prefix . $body_content;
    }
    if (!$closing){
      $this->dbg('ob_start',cnf_dev);
      ob_start();
      $this->tab_counter++;
    }
    print "<!-- ".__method__." end -->\n";
    $this->dbg(exiting);
  }

  /*
   * Check is the tab active
   */
  private function isActive($tab){
    $tabs = array_keys($this->tabs);
    if (count($this->tabs) == 1){
      $reply = True;
      $tab = $tabs[0];      
    }else{
      if     ($tab == 0)     $tab = $tabs[$this->tab_counter];
      elseif ($tab ==-1)     $tab = $tabs[$this->tab_counter-1];
      elseif (($t=array_search($tab,$this->tabs)) !== False) $tab = $t;
      $reply = ($tab == $this->_GET_b_tab);
    }
    $this->dbg($this->tabs[$tab].' - '.($reply?'YES':'NO'),cnf_dev);
    return $reply; 
  }

  /*
   * Return the index of the active tab
   */
  public function active_tab(){
    $tab = @$this->tabs[@$this->_GET_b_tab];
    $tab_index = array_search($tab,array_values($this->tabs));
    $this->dbg("$tab_index - $tab");
    return $tab_index;
  }

  public function active_tab_name(){
    $tab = @$this->tabs[@$this->_GET_b_tab];
    b_debug::xxx($tab);
    return $tab;
  }

  /*
   * Show the collected tab. 
   */
  public function close(){
    $this->dbg('',cnf_dev);
    $this->body_close(True);

    self::$inside = False;
    self::$myPearTabs_depth--;
    
    // Flash the user buffer, if any
    if (isset($this->body[$this->selected])) $this->body_content .= $this->body[$this->selected];
    $this->body = array();

    // Print only the tab content, without the "tabs frame-work" if there is only 1 tab
    if (count($this->tabs) > 1){
      // Set the left margin depending on the total size of the text
      $margin = 233;
      $margin = ($this->n_symbols > $this->n_symbols_ok
		 ? (int)($margin * $this->n_symbols_ok / $this->n_symbols)
		 : $margin);
      $style = " style='margin-left:${margin}px;'";
      $style = 'b_tabs_margin';
      // Show tabs as <li> tags
      foreach ($this->tabs as $hash=>$tab){
	$drop = array();
	$href = b_url::same("?".$this->cnf_bTabs."=$hash&clean_once=function",$drop,'keepAllBut');
	$li[] = x("li class='$style".($this->isActive($hash)?" b_tabs_current'":"'"),x("a href='$href'",$tab));
	$style = '';
      }
      $this->dbg('Output multytab text',cnf_dev);
      print implode("\n",array('',
			       '<!-- '.__method__.' ----------------------------- -->',
			       x("div style='clear:both'",''),
			       x("div id='b_tabs".(self::$myPearTabs_depth+1)."'",
				 join("\n",array(x("div class='b_tabs_menu'",join("\n",array(x('ul class="form-submit" style="margin-bottom:0;"',join("\n",$li))))),
						 x("div style='clear:both'",''),
						 x("div class='b_tabs_body'",$this->body_content)))),
			       x("div style='clear:both'",''),
			       '<!-- '.__method__.' ----------------------------- -->',
			       ''));
    }else{
      $this->dbg('Output single tab text',cnf_dev);
      print implode("\n",array('',
			       x("div style='clear:both'",''),
			       x("div id='b_tabs'",
				 x("div class='b_tabs_body'",$this->body_content)),
			       x("div style='clear:both'",''),
			       ''));
    }
    // Finally set back the current tab
    b_cnf::set($this->cnf_bTabs,($_GET[$this->cnf_bTabs]=$this->_GET_b_tab)); 
  }
}

/*
 * jQuery "tabs div"
 *
 */
class b_tabs_JQ extends myPear_base{
  public $ID = 0;
  public $current_tabTitle = '?';
  public  static $inside = False;
  public  static $jQueryTabs_depth = 0;
  private static $div_counter=0;
  private $tab_counter=0;
  private $div_id;
  private $tab_names=array();
 
  private $selected = 0;
  private $depth = 0;
  public  $body = array();
  public  $debug = False;

  function __construct($tabs=array(),$selected=0){
    $this->ID = ++self::$div_counter * 100 + ($this->depth = ++self::$jQueryTabs_depth);
    $this->tab_names = $tabs;
    $this->selected = $selected;
    $this->div_id   = __CLASS__.$this->ID;
    $this->dbg($this->div_id);
  }
  
  /*
   * Return the name of the active tab
   */
  public function active_tab(){
    // In jQuery all the tabs are "active" 
    $tab = 'b_tabs_JQ';
    return $tab;
  }

  public function isActive($tab=0){
    return True;
  }

  /*
   * tab body starts, auto-close the previous one, open new one
   */
  function body(){      
    if ($this->tab_counter < count($this->tab_names)){
      // Expose the current tab name
      $this->current_tabTitle = $this->tab_names[$this->tab_counter];
      $this->open_tabs();
      if ($this->depth == 1){
	$this->close_div();
	$id  = $this->div_id . $this->tab_counter;
	$tab = $this->tab_names[$this->tab_counter];
	print "\n<div id='$id'>  <!-- open tab '$tab' -->\n";
	$this->dbg(strip_tags($tab));
      }
      $this->tab_counter++;
    }
    return True;
  }
  
  public function close(){
    foreach($this->body as $body) $this->body($body);
    $this->close_tabs();
    $this->body = array();
  }

  private $deja_open = 0;
  private function open_tabs(){
    if (!$this->deja_open++ && ($this->depth == 1)){
      $this->dbg();
      bJS()->add_js_inline('$(function() { $( "#'.$this->div_id.'" ).tabs({ heightStyle:\'auto\',selected:'.$this->selected.'});});',True);
      print "\n<div id='$this->div_id'>  <!-- open tabs div $this->div_id -->\n";
      
      $li[] = '';
      foreach($this->tab_names as $k=>$tab_text){
	$id = $this->div_id . $k;
	$li[] = x('li',x("a href='#$id'",$tab_text));
      }
      $li[] = '';
      print x('ul',join("\n",$li))."\n";
    }
  }
  
  /*
   * all tabs are finished, closing tabs division
   */
  private function close_tabs(){    
    if ($this->depth == 1){
      $this->close_div();
      print "</div>  <!-- close tabs div $this->div_id -->\n";
      $this->dbg();
    }
    self::$jQueryTabs_depth--;
  }

  private function close_div(){
    if ($this->tab_counter > 0) {
      bIcons()->explain();
      bIcons()->reset();
      $tab = $this->tab_names[$this->tab_counter - 1]; 
      print "\n</div> <!-- close tab '$tab' -->\n";
      $this->dbg($tab);
    }
  }
}
