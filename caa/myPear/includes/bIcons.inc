<?php
  /**
   * provide html "includes" to for icons
   *
   * Icon components:
   *   i - (compulsory) icon name, treated as an regexp to find the file with the images pool
   *   d - description (argument for 'action_once' as well)
   *   l  - url, i.e. the icon is a submit button 
   *   la - same as 'l', but the page to be shown as an modal overlay thru bJS()->modal_ajax 
   *   li - same as 'l', but the page to be shown as an modal overlay thru bJS()->modal_iframe
   *   u  - the parsed l-url
   *   c  - css class to be added to the 'img' tag: <img class="<c>" .../>
   *   X  - the button description should not go to the summary (i.e. to the icons legend)
   *   r  - sub-folder where the icon is kept
   *   m == 'forced'  the button must be shown (to be checked???)
   */
locateAndInclude('b_cache');

class bIcons{
  
  public  $hide_explainIcons  = False;
  private $explainIconButtons = True;
  var $defaultIcons = array('ICON_edit'   => array('d'=>'edit', 'i'=>'i-edit'),
			    'ICON_drop'   => array('d'=>'drop', 'i'=>'i-drop'));
  var $showToolTips= True;
  var $legendTitle = 'Symbols legend';
  var $explainColumns = 3;
  private $toExplain = array();	
  private static $iconsPool = array();
  private static $cache     = Null;
  
  function __construct(){
    //    if (class_exists('bAuth',False) && is_object(bAuth::$av) && bAuth::$av->isOwner()){
    //      $this->defaultIcons['ICON_edit']['i'] = 'i-edit';
    //    }
    $this->setDescription($this->defaultIcons);
    $this->toggleExplainIcons();
  }

  function getFlag($country_code,$as_input_field=True){
    if (is_array($country_code)) $flag = $country_code;
    else                         $flag = array('i' => $country_code);
    $flag['r'] = 'flag'.($as_input_field ? 's' : '');
    $flag['d'] = b_t::getLangName($country_code);
    $flag['i'] = preg_replace('/.*[_-]/','',mb_strToLower($flag['i']));
    return $this->getButton($flag);
  }

  public function toggleExplainIcons($value='restore'){
    static $savedValue = True;
    if ($value === 'restore'){
      $this->explainIconButtons = $savedValue;
    }else{
      $savedValue = $this->explainIconButtons;
      $this->explainIconButtons = $value;
    }
  }

  /**
   * Get an icon from the pool and return it as a submit button
   */
  function getButton($button=array(),$knownAs=False){
    static $counter = array();

    bTiming()->cpu(__FUNCTION__);

    // Alias to another icon
    if ($knownAs){
      if (!is_array(@self::$iconsPool[$knownAs])) self::$iconsPool[$knownAs] = array();
      foreach (@self::$iconsPool[$knownAs] as $k=>$v) {
	if (empty($button[$k])) $button[$k] = $v;
      }
      if (empty($button['i'])) $button['i'] = $knownAs;
      $icon = b_crypt::hash($button['d']);
    }else{
      $icon = @$button['i'];
    }

    $reply = '';
    if (!empty($button['i'])){

      $this->setDescription(array($icon => $button));
      
      if ( $this->hide_explainIcons == 'forced') unset(self::$iconsPool[$icon]['m']);
      if (!$this->hide_explainIcons || (@self::$iconsPool[$icon]['m']==='forced')){
	if      ($l = @self::$iconsPool[$icon]['li']){
	  $reply = bJS()->modal_iframe($l,$this->get($button['i']));
	}elseif ($l = @self::$iconsPool[$icon]['la']){
	  $reply = bJS()->modal_ajax($l,$this->get($button['i']));
	}elseif ($l = @self::$iconsPool[$icon]['l']){
	  if ($d=@self::$iconsPool[$icon]['d']) $l = b_url::repack($l,array('action_once'=>b_fmt::escape($d)));
	  $c = (isset($button['class']) ? $button['class'] : '');
	  $a = "a class='mypear_a blackText only_online $c' href='$l'";
	  
	  // Repeat the submit button in the description (in the 'explain pane'), 
	  // but guarantee that it is unique.
	  // Drop the tooltip for the symbols legend 
	  if (!isset($counter[$icon])) $counter[$icon] = 0;
	  if  (! $counter[$icon]++)         self::$iconsPool[$icon]['u'] = $a;
	  elseif($counter[$icon] > 1) unset(self::$iconsPool[$icon]['u']);
	  $reply = x($a,$this->get($icon,@self::$iconsPool[$icon]['r']));
	}else{
	  $reply = $this->get($icon,@self::$iconsPool[$icon]['r']);
	}
      }
    }
    bTiming()->cpu();
    return $reply;
  }
  
  /**
   *
   */
  function get($icon,$helper='',$attrs=''){
    bTiming()->cpu(__FUNCTION__);
    $reply = '';
    if (!empty($icon)){
      if (is_array($icon) && ($description=@$icon['d']) && ($i=@$icon['i'])){
	$iconName = b_crypt::hash($description);
	$this->setDescription(array($iconName => $icon));
	if (!empty($icon['class'])) $class = explode(' ',$icon['class']);
	$icon = $iconName;
      }
      
      if (!($f = @$this->_fromRepository(self::$iconsPool[$icon]['i'],$helper))){
	$f = $this->_fromRepository($icon,$helper);
      }

      if (!empty($f)){
 	$description = (string)@self::$iconsPool[$icon]['d'];

	switch($helper){

	case 'submit':
	  $name = ((stripos($attrs,'name=') === False) ? "name='_button'" : ""); 
	  $reply = "<input src='$f' $attrs type='image' $name style='border:0;'/>";
	  break;
	  
	  // do not create tool-tips for flags - they are shown mostly in the corner where there is no space for tt
	case 'flag':
	  $reply = "<img class='opacity_off' src='$f' title='$description' alt='' />";
	  break;
	case 'flags':
	  $reply = "<span class='opacity_on'><input src='$f' type='image' name='_button' title='$description' alt='' /></span>";
	  break;
	  
	case '100':	  
	  $class[] = 'min100';
	default:
	  if ($description){
	    if (isset(self::$iconsPool[$icon]['X'])) unset(self::$iconsPool[$icon]['u']);
	    elseif  (!$this->hide_explainIcons)               $this->toExplain[] = $icon;
	  }
	  if ($c = @self::$iconsPool[$icon]['c']) $class[] = $c;
	  $c = (empty($class)
		? ''
		: sprintf("class='%s'",join(' ',$class)));
	  $img = "<img src='$f' alt='".b_fmt::escape($description)."' $c />"; 
	  if(!cnf_inside_fb && $this->showToolTips && $description) $reply = bJS()->tip($img,$description);
	  else                                                      $reply = $img;
	}
      }
      if (cnf_show_ic) {
	if (empty($reply)) MSG::WARNING_T("missing icon ".var_export($icon,True),$this);
	else               b_debug::xxx(b_fmt::escape($reply),array(2,True,'blueText'));
      }
    }
    bTiming()->cpu();
    return str_replace(array('///','//'),'/',$reply);
  }
  
  function getFavicon($org=Null) { 
    return myOrg($org)->getFavicon();
  }
  
  /*
   *
   */
  function getThumbnail($av_emailP,$srcOnly=False){
    if (is_string($av_emailP)){
      locateAndInclude('bForm_Avatar');
      return bForm_Avatar::_getPhoto(array('av_email'=>$av_emailP),'','',8);
    }else{
      return '';
    }
  }
  
  /*
   * Look for the file in the repository and return its URL if found
   */
  private function _fromRepository($icon,$helper=''){
    static $use_cache = False;
    if (!empty($icon) && is_string($icon)){
      if ($use_cache) if (empty(self::$cache)) self::$cache = new b_cache_S($this);
      if ($use_cache) if (self::$cache->wasSet($icon) && ($img=self::$cache->get()))	return $img;
      
      // first try the thumbnail
      if ($thumb = $this->getThumbnail($icon,True)){
	if ($use_cache) self::$cache->set($thumb);
	return $thumb;
      }
      
      if (!($img = @self::$iconsPool[$icon]['i']))  $img = $icon;
      foreach(array('png','gif','jpg')  as $ext){
	$f = '';
	switch($helper){
	case 'submit':
	  $f = myPear_images."/$img.$ext";
	  break;
	case 'flag':
	  $helper = 'flags';
	default:
	  $f = (is_dir(myPear_images."/".$helper)
		? myPear_images."/$helper/$img.$ext"
		: myPear_images."/$img.$ext");
	}
	// file_exists($f) will fail in the 'clean url mode', hence check the full path
	if (file_exists(b_os::url2path($f))){
	  if ($use_cache) self::$cache->set($f);
	  return $f;
	}
      }
      //MSG::WARNING_T("no icon '$icon' in ".myPear_images,$this);
    }
    return Null;
  }
  
  function known($icon){
    return !empty(self::$iconsPool[$icon]);
  }

  /*
   * Save the icon description in the pool
   */
  function setDescription($icons=array()){
    if ($icons === Null){
      self::$iconsPool = array();
    }else{
      foreach ($icons as $icon=>$a){
	if (is_array($a)){
	  foreach($a as $k=>$v){
	    if (!empty($v)) self::$iconsPool[$icon][$k] = $v;
	  }
	}
      }
    }
  }

  function getDescription($icon){
    $reply = @self::$iconsPool[$icon];
    if (empty($reply)) $reply = array();
    return $reply;
  }

  function reset($newValue=array()){
    $current = $this->toExplain;
    $this->toExplain = $newValue;
    if (empty($this->toExplain)) $this->toExplain = array();
    return $current;
  }

  function explain(){
    if (empty($this->toExplain)) $this->toExplain = array();
    $nColumns = (($n=count(array_unique($this->toExplain)))>$this->explainColumns-1 ? $this->explainColumns : 1);
    if ($n < 6) $nColumns = 3;
    $save = $this->showToolTips;   $this->showToolTips = False;
    if ($this->hide_explainIcons || !$this->explainIconButtons) $this->toExplain = array();
    $legend = x('strong',$this->legendTitle.':');
    $lPrinted = 0;
    foreach (array_unique($this->toExplain) as $icon){
      if ($descr = @self::$iconsPool[$icon]['d']){
	$a = @self::$iconsPool[$icon]['u'];
	if (empty($t))  $t = new b_table('','<span class="only_online align_left">');
	if (!$t->tr)  $t->tro(); 
	$t->td(array($legend,
		     x($a,$this->get($icon)),
		     x($a,b_t::_($descr)))); 
	$legend = ''; 
	if (++$lPrinted == $nColumns) { $lPrinted = 0; $t->trc(); } 
      }
    }
    if (!empty($t)) { if ($t->tr) $t->trc(); $t->close('</span>'); }
    $this->toExplain = array();
    $this->showToolTips = $save;
  }

  function __toString() { return 'bIcons'; }

}
