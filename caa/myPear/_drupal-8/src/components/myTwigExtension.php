<?php

namespace Drupal\myPear\components;

use Drupal\Core\Template\TwigExtension;


/*
 */
class myTwigExtension extends \Twig_Extension{

  private $anotherService = Null;
  
  public function __construct(SecurityService $anotherService= null){
    $this->anotherService = $anotherService;
  }

  /*
   *
   */  
  public function mypear_is_fp(){
    return empty($GLOBALS['myPear_current_module']);
  }

  public function mypear_fp_content(){
    myPear_init();
    \b_reg::load_module();
     $tabh[] = x('h4','Select the desired Application:');
    foreach(\b_reg::$modules as $module=>$descr){
        if (!\APImenu::_used_by_myOrg($module) || !in_array($module,module_list())) continue;
      if ($module == myPear_MODULE){
	if (!superUser_here) continue;
	$descr['d'] = myPear_MODULE.' core';
      }
      // $tabh[] = "<!-- ".__FUNCTION__.": module='$module' descr='$descr[d]' -->";
      $tabh[] = "<li ><a href='$module?q=$module'>$descr[d] </a></li>";
    }
    return $this->signature(__FUNCTION__,x('ul',join("\n",$tabh)));
  }
  
  /*
   *
   */
  public function mypear_fp_title(){
      return $this->signature(__FUNCTION__,
                              'Welcome to the '.(defined('myOrg_name') ? myOrg_name : '').' <br/>Computer Aided Administration');
  }
  
  /*
   * 
   */
  public function mypear_login(){
    $reply = '';
    if (class_exists('bAuth',False)){
      $flavor = sprintf("%s=1&q=%s&flavor=%s&org=%s&resetcache_once=1",
			b_crypt_no,$GLOBALS['myPear_current_module'],\b_cnf::get('flavor'),myOrg_code);
      if (is_object(\bAuth::$av)){
	if (function_exists('myPear_access')){
	  $rank = myPear_access()->getRank();
	  $title = ($rank > RANK__authenticated ? myPear_access()->getTitle() : '') . ' ' . \bAuth::$av->fmtName('fl').' ';
	}else{
	  $title = 'Ghost';
	}
	$reply = x("li","$title<a href='".\b_url::same("?quit_once=yes&$flavor")."'>[logout]</a>");
      }elseif($GLOBALS['myPear_current_module']){
	$reply = x("li","<a href='".\b_url::same("?login=check&$flavor")."'><img alt='login' src=/".drupal_get_path('theme','nordita')."/images/login.png /></a>");
      }
      $reply = x('ul class="mypear_login"',$reply);
    }
    \D8::dbg(\b_fmt::escape($reply));
    return $this->signature(__FUNCTION__,$reply);
  }

  /*
   *
   */
  public function mypear_module(){
      return $this->signature(__FUNCTION__,
                              (empty($GLOBALS['myPear_current_module'])
                               ? ''
                               : $GLOBALS['myPear_current_module'].' '.constant(strToUpper($GLOBALS['myPear_current_module']).'_VERSION')));
  }

  /*
   *
   */
  public function mypear_date(){
    if (class_exists('b_reg',False) && !empty(\b_reg::$modules[\b_reg::$current_module])){
      $reg = \b_reg::$modules[\b_reg::$current_module];
      $module_v = sprintf('%s&nbsp;%s',$reg['m'],$reg['v']);
      return $this->signature(__FUNCTION__,$reg['r']);
    }else{
      return $this->signature(__FUNCTION__);
    }
  }

  /*
   * D8 features Workaround,
   * hide the non-active modules in the menu
   */
  public function mypear_hidden_item(\Drupal\Core\Url $url){

    if (D8MENU_CSS_HIDING_LINKS){
      \D8::current_tab();
      
      list($m,$p) = explode('.',$url->getRouteName());
      $to_be_hidden = (($m == $p) && ($m != $GLOBALS['myPear_current_module']));
      if ($to_be_hidden){
	if (class_exists('myPear',0)) \myPear::MESSAGE("$m to be hidden");
	\D8::dbg(['module'=>$m,'to hide'=>$to_be_hidden]);
      }
    }else{
      $to_be_hidden = False;
    }
    return $to_be_hidden;
  }

  private function signature($who,$body=''){
      return (defined('cnf_dev') && cnf_dev
              ? join("\n",["","<!-- start twig extension $who -->",$body,"<!-- end twig extension $who -->",""])
              : $body);
  }
  
  /*
   *
   */
  public function getFunctions(){
    foreach (array('hidden_item','module','date','login','is_fp','fp_title','fp_content') as $ext){
      $functions["mypear_$ext"] = new \Twig_Function_Method($this, "mypear_$ext");
    }
    return $functions;
  }

  /**
   * Returns the name of the extension.
   *
   * @return string The extension name
   */
  public function getName(){
    return 'twig_extension';
  }

  /*
   * another example
   */
  public function foo($param){
    $this->anotherService->bar($param);
    return __METHOD__;
  }
}
