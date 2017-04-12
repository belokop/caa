<?php

/*
 * Collection of the "light-weighted javaScript" functions,
 * doing most of the modal tricks in CSS.
 *
 * Provides wrappers for the various CMS
 */

if ($movie = @$_GET['movie_once']) bJS()->fancybox_inline_movie(b_fmt::unEscape($movie,'url'));

class bJS{

  /**
   * Modal window parameters
   */
  public $closeBtn= 'true';
  public $height  = '80%';
  public $width   = '80%';

  var $w = 900;
  var $h = 600;
  var $debug = 0;
  private $modalID = '';
  private $fadeID  = '';
  private static $N  = 0;
  private static $id = '';
  
  /**
   * For the "our pages" (i.e. URLs pointing to myPear widgets)
   * add a flag "we are inside the fancybox". 
   * This flag triggers the widget to be shown without the theme header/footer.
   *
   * How to cut off the trigger/footer is a separate problem...
   * The best way to do it (thanks to HvZM) is to use css, see noHeader.css
   */
  public function modal_once($type=0){
    if    (cnf_inside_fb_ajax)  $modal_once = 0;
    elseif(cnf_inside_fb_iframe)$modal_once = 'iframe';
    else                        $modal_once = $type;
    return $modal_once;
  }

  /*
   * Return the "close" button for the modal window
   */
  public function close_modalWindow($text='Close this window',$style=''){
    static $n_requests = 0;
    if    ($n_requests++){
      return '';
    }else{
      if (0) bJS()->add_js_inline('
$(document).ready(function() {
    $("#submit_'.strToLower(str_replace(' ','_',$text)).'").click(function(){
	$.fancybox.close();
      });
  });
');
      $href = 'javascript:$.fancybox.close();';
      $href = 'javascript:jQuery.fancybox.close();';
      $href = (cnf_inside_fb_iframe
	       ? 'javascript:parent.jQuery.fancybox.close();'  // it works for iframe only
	       : 'javascript:jQuery.fancybox.close();'         // this DOES NOT work... TBD
	       );
      if ($text === 'href')   return $href;
      //    $reply = "<input type='submit' value='$text' onclick='document.location.href=\"$href\";return false;' />";
      $reply = b_btn::submit($text,
			     $href,		   
			     False, 
			     $style);
      return $reply;
    }
  }

  private static $cacheJS = array('code' => array(),
				  'file' => array(),
				  'link' => array(),
				  'tags' => array(),
				  'style'=> array(),
				  );
  function __construct() {
  
    // Load jQuery, first js, then css - js takes much longer to load
    $this->add_js_file( 'jquery.js'    ,myPear_js.'../jquery/');
    $this->add_js_file( 'jquery-ui.js' ,myPear_js.'../jquery/');
    $this->add_css_file('jquery-ui.css',myPear_js.'../jquery/');
    if (cnf_inside_fb)  $this->add_css_file('content-only.css',myPear_css,'content');
    
    $this->warning();
    $x=''; $this->modal_open($x); // passe by reference
    
    /*
     * provide "input default values for object with "class='input-def-value'"
     */
    $this->add_js_inline("
      $('.input-def-value').each(function() {
	  var default_value = this.value;
	  $(this).css('color', '#666'); 
	  $(this).css('font-style', 'italic');
	  $(this).focus(function() {
	      if(this.value == default_value) {
		this.value = '';
		$(this).css('color', '#000');
		$(this).css('font-style', 'normal');
	      }
	    });
	  $(this).blur(function() {
	      if(this.value == '') {
		$(this).css('color', '#999');
		$(this).css('font-style', 'italic');
		this.value = default_value;
	      }
	    });
	});");
    
      /*
       * Make the page centralized (under development)
       */
    if (cnf_inside_fb || b_cnf::get('center')) $this->add_js_inline("
  var maxWidth  = $('#container-fixed').width();
  var pageWidth = $('#region-contentwide').width();
  $('#container-scrolled').css('left',(maxWidth-pageWidth)/2);
/*  $('#block-colophon').css('width',maxWidth); */");
  }

  /*
   * Print warning "javascript should be enabled"
   */  
  private function warning(){
    static $dejaVu = 0;
    // should be done better...
    if(0) if (!$dejaVu++) myPear::MESSAGE(x('noscript',(x('strong','WARNING, this page uses javaScript.').'<blockquote>'.
							'Your browser either doesn\'t support javaScript or you have it turned off. <br/>'.
							'To see this page as it is meant to appear please use '.
							'a javaScript enabled browser</blockquote>')));
  }
  
  /*
   *
   */
  function add_css_inline($text) {
    static $dejaVu = array();
    if (!@$dejaVu[$text]++ && trim($text)){
      b_cache_file::CMS(__METHOD__,array($text));
      switch(HOSTED_BY_CMS){
      case 'drupal':
	switch(HOSTED_BY_CMS_vrsn){
	case 6:
	  drupal_set_html_head(x("style type='text/css' media='all'",$text));
	  break;
	case 7:
	  drupal_add_css($text,array('type'=>'inline','scope'=>'header'));
	  break;
	case 8:
	  global $myPear_page_attachments;
	  $myPear_page_attachments['style'][] = $text;
	  break;
	case 9:
	default:
	  myPear::ERROR(b_debug::__().' is not ready for drupal-'.HOSTED_BY_CMS_vrsn);
	}
	break;
	
      case 'joomla':
	$doc =& JFactory::getDocument();
	$doc->addStyleDeclaration($text=strip_tags($text));
	break;
	
      default:
	self::$cacheJS['style'][] = $text;
      }
    }
  }
  
  /*
   * This function makes sense in the non-CMS mode only,
   * its functionality is a part of any CMS. However, it is safer to have it like that.
   */
  function add_css_file($objectP, $root=myPear_css, $media='all'){
    static $dejaVu = array();
    $object = str_replace('//','/',"$root/$objectP");
    if (!@$dejaVu[$object]++){
      b_cache_file::CMS(__METHOD__,array($objectP,$root,$media));
      foreach(array('min','pack') as $p){
	if (file_exists($f = str_replace(".css",".$p.css",$object))){
	  $object = $f;
	  break;
	}
      }
      if (!file_exists($f=b_os::url2path($object))) myPear::ERROR(b_debug::__()." $f doesn't exist",$this);

      switch(HOSTED_BY_CMS){
      case 'drupal':
	switch(HOSTED_BY_CMS_vrsn){
	case 6: 
	  drupal_add_css($object,'theme');
	  break;

	case 7:
	  drupal_add_css($object,array('type'=>'file','scope'=>'header'));
	  break;

	case 8:
	  if ($media === 'content'){
	    $content = file_get_contents('.'.$object);
	    $this->add_css_inline(b_fmt::compact_html($content));
	  }else{
	    b_debug::xxx("not loaded by D8, must be in myPear.libraries.yml");
	  }
	  break;

	default:
	  myPear::ERROR($object);
	  myPear::ERROR(b_debug::__().' is not ready for drupal-'.HOSTED_BY_CMS_vrsn);
	}
	break;
	
      default:
	if ($media === 'content') $media = 'all';
	$link = "<link type='text/css' rel='stylesheet' media='$media' href='$object'>";
	self::$cacheJS['link'][] = $link;
      }
    }
  }
  
  /*
   *
   */
  function add_js_file($objectP, $root=myPear_js){
    static $dejaVu = array();
    $object = str_replace('//','/',"$root/$objectP");
    if (!@$dejaVu[$object]++){
      foreach(array('min','pack') as $p){
	if (file_exists($f = str_replace(".js",".$p.js",$object))){
	  $object = $f;
	  break;
	}
      }
      b_cache_file::CMS(__METHOD__,array($objectP,$root));
      if (!file_exists($f=b_os::url2path($object))) myPear::ERROR(b_debug::__()." $f doesn't exist",$this);
      switch(HOSTED_BY_CMS){
      case 'drupal':
	switch(HOSTED_BY_CMS_vrsn){
	case 6: 
	  drupal_add_js($object,'core');
	  break;
	case 7:
	  drupal_add_js($object,'file');
	  break;
	case 8:
	  // b_debug::xxx("not loaded, see myPear.libraries.yml");
	  break;
	case 9:
	default: 
	  myPear::ERROR($object);
	  myPear::ERROR(b_debug::__().' is not ready for drupal-'.HOSTED_BY_CMS_vrsn);
	}
	break;
	
      case 'joomla':
	$doc =& JFactory::getDocument();
	$doc->addScript($object);
	break;
	
      default:
	$object = str_replace(dirname(myPear_root),'../',$object);
	$s = "<script language='javaScript' src='$object'></script>";
	//if ($if) $s = '<!--['.$if.']>'.$s.'<![endif]-->';
	self::$cacheJS['file'][] = $s;
      }
    }
  }

  /*
   * Add JS code.
   * In case of jQuery the code is optionally encapsulated be able to use a custom jQuery 
   * (say, a newer version then a default one supplied with the CMS)
   */
  function add_js_inline($js_code,$custom_jQuery=True) {
    static $dejaVu = array(), $count=0;
    if (!@$dejaVu[$js_code . $custom_jQuery]++){
      $js_code = preg_replace(";on[cC]lick=\'[^\']*\';",'',b_fmt::compact_html($js_code,True));
      b_cache_file::CMS(__METHOD__,array($js_code,$custom_jQuery));
      
      if ($custom_jQuery){
	$j = '$' .get_class($this).(++$count);
	$prefix = "$j=jQuery.noConflict(); jQuery(document).ready(function($j)";
	if (stripos($js_code,'$(document).ready') === False){
	  $js_code = str_replace('$',$j,$js_code);
	  $js_code = $prefix . '{'. $js_code.'});';
	}else{
	  $js_code = str_replace('$(document).ready(function()','_ZZZZZZ_',$js_code);
	  $js_code = str_replace('$',$j,$js_code);
	  $js_code = str_replace('_ZZZZZZ_',$prefix,$js_code);
	}
      }

      switch(HOSTED_BY_CMS){
      case 'drupal':
	switch(HOSTED_BY_CMS_vrsn){
	case 6: 
	  drupal_add_js($js_code, 'inline', 'footer');
	  break;

	case 7:
	  drupal_add_js($js_code, array('type'=>'inline','scope'=>'footer')); 
	  break;

	case 8:
	  static $n_entry = 0;
	  // Write the script into a file
	  $fn = drupal_get_path('module','myPear')."/myPear.inline.js";
	  if (!$n_entry++){
	    $ok = (file_exists($fn) ? unlink($fn) : True);
	    if (!$ok) myPear::ERROR("Can't create file $fn");
	  }
	  $file = @fopen($fn,"a");
	  if (empty($file)){
	    myPear::ERROR("Can't write to file $fn");
	  }else{
	    fwrite($file,$js_code."\n");
	    fclose($file);
	  }
	  // global $myPear_page_attachments;
	  // $myPear_page_attachments['script'][] = $js_code;
	  break;
	case 9:
	default:
	  myPear::ERROR(b_debug::__().' is not ready for drupal-'.HOSTED_BY_CMS_vrsn);
	}
	break;
	
      default:
	self::$cacheJS['code'][] = x("script media='screen' type='text/javaScript'",$js_code);
      }
    }
  }

  /**
   * As far as i understand the ajax call works only for (or might be used for) local (i.e. the "same" URL as the referrer) pages, 
   * i.e. I can't show yuotube.com with ajax - just does not load the page, issues ERROR
   */
  function modal_ajax($url, $title='',$description=Null, $galery=False){
    $reply = $this->popup($title, $url, myPear::myPear_enabled_theme(), __FUNCTION__, $description, $galery);
    if ($this->debug) b_debug::xxx(b_fmt::escape($reply));
    return $reply;
  }
  
  /**
   * Modal iframe
   *
   * iframe can load any URL into the modal window, but...
   * If it is called from a secure connection the iframe must be also secure, otherwise the result is simply empty.
   *
   */
  function modal_iframe($url, $title='',$description=Null, $galery=False){
    // Check for the secure connection
    $mightBeModal = myPear::myPear_enabled_theme();
    if ($mightBeModal && ($_SERVER["SERVER_PORT"] == 443) && preg_match(';http:/;i',$url)){
      // Check that the server replies port 443, switch to 443 if it does
      $mightBeModal = myPear_hostReplies(parse_url($url,PHP_URL_HOST),443);
      //      b_debug::xxx("mightBeModal = ".var_export($mightBeModal,True));
      if ($mightBeModal){ 
	$url = str_ireplace('http:/','https:/',($x=$url));
	b_debug::xxx("!!! change url: <br>&nbsp;was: $x<br>&nbsp;now: $url");
      }
    }

    $reply = $this->popup($title, $url, $mightBeModal, __FUNCTION__, $description, $galery);

    if ($this->debug) b_debug::xxx(b_fmt::escape($reply));
    return $reply;
  }
  
  /*
   * 
   */
  function farbtastic(){
    $this->add_css_file('farbtastic.css',myPear_js.'../jquery/farbtastic/');
    $this->add_js_file ('farbtastic.js', myPear_js.'../jquery/farbtastic/');
    $this->add_js_inline('$(document).ready(function(){
     var f = $.farbtastic("#farbtastic-picker");
     var p = $("#farbtastic-picker").css("opacity", 0.25);
     var selected;
     $(".farbtastic-colorwell")
        .each(function () { f.linkTo(this); $(this).css("opacity", 0.75); })
        .focus(function() {
            if (selected) {
               $(selected).css("opacity", 0.75).removeClass("farbtastic-colorwell-selected");
            }
            f.linkTo(this);
            p.css("opacity", 1);
            $(selected = this).css("opacity", 1).addClass("farbtastic-colorwell-selected");
        });
});',True);
    print x('div id="farbtastic-picker" style="float: right;"','');
  }

/*
 *
 */
  function fancybox_inline_movie($image,$w=800,$h=400){
    $v = 111; // Volume
    $this->fancybox_inline("
    <OBJECT classid='clsid:6BF52A52-394A-11d3-B153-00C04F79FAA6' width='$w' height='$h'>
    <PARAM NAME='URL' VALUE='$image'>
    <PARAM NAME='volume' VALUE='$v'>
    <embed src='$image' width='$w' height='$h' volume='$v'></embed>
    </OBJECT>");
  }

  /**
   */
  function fancybox_inline($html){
    $html = b_fmt::unEscape(str_replace('"',"'",b_fmt::compact_html($html)));
    $html = preg_replace("#>'$#",">",$html);
    $x=''; $this->modal_open($x); // pass by reference...
    $this->add_css_inline('.fancybox-custom .fancybox-outer { box-shadow: 0 0 50px #222; }');
    $this->add_js_inline('$.fancybox("'.$html.'",{minHeight:"100", minWidth:"200", wrapCSS:"fancybox-custom", helpers:{overlay:{css:{"Xbackground-color":"#eee"}}}});',
			 True);
  }
  
  function modal_image_file($linkText,$url,$title='',$galery=True){
    $id = __FUNCTION__;
    
    $x=''; $this->modal_open($x); // pas by reference
    $this->add_js_inline("\$('.$id').fancybox({ padding:70, closeBtn:true, openEffect:'none', closeEffect:'none', nextEffect:'none', prevEffect:'none'});");
    if ($title == 'none') $title = '';
    return "<a ".$this->galery_id($galery)." class='$id' title='".no_wrap(b_fmt::escape($title))."' href='$url'>$linkText</a>";
  }

  private function galery_id($galery=True){
    return ($galery ? "rel='galery'" : "");
  }

  /*
   *
   */
  function popup($title, $url, $shouldBeModal=True,$engine='modal_iframe',$description=Null,$galery=False){
    if ((strpos($url,'http') !== False) && (strpos($url,'#content') === False)) $url = trim($url).'#content';
    if ($shouldBeModal) {
      $url = $this->modal_engine($url, $title, $description, $galery, $engine);
    }else{ 
      if (strpos('?',$url) !== False) $url = b_url::same(b_crypt::parse_query_string($url));
      $url = "<a href='$url'>$title</a>";
    }
    return $url; 
  }
  

  /*
   * Show the "inline" modal block, i.e. a <div class='black_overlay' id='xxx'>...</div> object which 
   * on click gets "display='block'" attribute.
   *
   * This works fine, the only problem is the performance - 
   * the should be not many such blocks on the page
   *
   * The preffered widget is fancybox since it is more efficient,
   * but its current implementation requires the 'widget-enabled' template.
   * Hence I still often use the modal_inline...
   */
  function modal_inline($title,$html_code,$showTouteSuite=False,$showCloseButton=True) {
    return $this->fancybox_inline($html_code);
  }
  
  /*
   * Flush the remaining buffers at the end of the page.
   * By the design this function should be called only once,
   * but by a strange reason it is called twice by drupal, so
   * it caches the information in order to return the same info every time.
   */
  function flush($type=''){
    static $cache = array();
    if (empty($cache)) $cache = self::$cacheJS;
    if (is_array($cache)){
      foreach ($cache as $typ=>$data){
	if (!empty($data) && (empty($type) || $typ == $type)){
	  print "\n<!-- flushing '$typ' -->\n".join("\n",array_unique($data))."\n";
	  self::$cacheJS[$typ] = array();
	}
      }
    }
  }
  
  /*
   * Tool Tip
   */
  function tt($target,$tip_text=''){
    if (!cnf_inside_fb){
      $x=''; $this->modal_open($x); // pas by reference
      $this->add_js_inline('$("['.b_fmt::unEscape($tip_text).']").tooltip();');
    }
    return $target;
    return "<span onmouseover=\"tooltip.show('$tip');\" onmouseout=\"tooltip.hide();\">".$target."</span>\n";
  }

  function tip($target,$tip_text=''){
    /*
     * Auto-translation problem...
     * The Tool-Tips might contain rather unexpected and/or long text, like a summary of a visit.
     * People would be upset by seeing that in the config file, hence do not translate them.
     * It would be nice to translate them on the fly, without saving in cache,
     * but the current translation service of  Google has rather strict license
     * and translating too often might result in banning the access.
     * To be continued...
     */
    if (cnf_inside_fb) return ''; // no tips for the modal mode
    else    $tip = b_fmt::escape(b_fmt::compact_html($tip_text,True));
    return "<span onmouseover=\"tooltip.show('$tip');\" onmouseout=\"tooltip.hide();\">".$target."</span>\n";
  }

  /*
   *
   */
  private function modal_open(&$url,$type=''){
    $this->add_css_file('jquery.fancybox.css',myPear_js.'../jquery/jquery.fancybox/');
    $this->add_js_file ('jquery.fancybox.js', myPear_js.'../jquery/jquery.fancybox/');

    $url = trim($url);
    if (!empty($url)){
      if (strpos($url,'http') !== 0){
	$url = b_url::repack($url,array(b_crypt_no          => 1,
					'modal_once'        => $this->modal_once($type),
					'doTranslation_once'=> (string)bText::$doTranslation));
      }else{
	$url = str_replace('#content','',$url) . '#content';
      }
      if     (preg_match('/^(.*)(#[\w]*)([\&\?].*)$/',$url,$m)) $url = $m[1].$m[3].$m[2];
      elseif (preg_match('/^(#[\w]*)(\?.*)$/',        $url,$m)) $url = $m[2].$m[1];
      return (preg_match(',^<,',$url) ? 'html' : 'iframe');
    }
  }
  
  /**
   * Use fancybox as as the engine
   */
  private function modal_engine($url, $title, $description, $galery, $id){
    $type = str_replace('modal_','',$id);
    if (cnf_inside_fb){
      $url = b_url::repack($url,array('modal_once'=>0));
      $reply = "<a href='$url'>$title</a>";
    }else{
      $this->modal_open($url,$type);
      $this->add_js_inline("\$('.$type').fancybox({ height:'$this->height', width:'$this->width', closeBtn:$this->closeBtn, autoSize:false, nextEffect:'none', prevEffect:'none' });");
      
      if ($description === Null) $description = $title;
      if (!empty($description)) $description = " title='".strip_tags($description)."' ";
      $reply = "<a class='fancybox.$type $type' ".$this->galery_id($galery)."$description href='$url'>$title</a>";
    }
    return $reply;
  }
  
  /*
   * add html line to the bottom of the page
   */
  private function addToFooter($tag,$body='',$cacheIt=True){
    if ($cacheIt) b_cache_file::CMS(__METHOD__,array($tag,$body,$cacheIt));
    if (!is_array(@self::$cacheJS['tags'])) self::$cacheJS['tags'] = array();
    self::$cacheJS['tags'][] = x($tag,$body);
  }
  
  private function _jsb($display_style='block',$title,$showTouteSuite=False){
    $x=''; $this->modal_open($x); // pas by reference
    $js = ("document.getElementById('$this->modalID').style.display='$display_style'; ".
	   "document.getElementById('$this->fadeID').style.display='$display_style';");
    if ($showTouteSuite)  $this->add_js_inline($js,False);
    return x("a href=# onclick=\"$js\"", $title);
  }

  /*
   *
   */
  function __toString() { 
    return 'bJS'; 
  }
}
