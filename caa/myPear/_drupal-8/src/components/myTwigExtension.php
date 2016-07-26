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
  public function mypear_fp_content(){
    myPear_init();
    \b_reg::load_module();
    $api = new \APImenu();
    foreach(\b_reg::$modules as $module=>$descr){
      if (!$api->_used_by_myOrg($module) || !in_array($module,module_list())) continue;
      if ($module == myPear_MODULE && !superUser_here) continue;
      $tabh[] = "<li ><a href='$module?q=$module'>$descr[d] </a></li>";
    }
    return x('h4','Select the desired Application:') . x('ul',join("\n",(empty($tabh)?[]:$tabh)));
  }
  
  /*
   *
   */
  public function mypear_fp_title(){
    return 'Welcome to the '.(defined('myOrg_name') ? myOrg_name : '').' <br/>Computer Aided Administration';
  }

  /*
   * 
   */
  public function mypear_login(){
    $reply = '';
    if (class_exists('bAuth',False)){
      $flavor = sprintf("%s=1&q=%s&flavor=%s&org=%s&resetcache_once=1",
			b_crypt_no,$GLOBALS['myPear_current_module'],\b_cnf::get('flavor'),myOrg_code);
      if (is_object(\bAuth::$av) && function_exists('myPear_access')){
	$rank = myPear_access()->getRank();
	$title = ($rank > RANK__authenticated ? myPear_access()->getTitle() : '') . ' ' . \bAuth::$av->fmtName('fl').' ';
	$reply = x("li","$title<a href='".\b_url::same("?quit_once=yes&$flavor")."'>[logout]</a>");
      }elseif($GLOBALS['myPear_current_module']){
	$reply = x("li","<a href='".\b_url::same("?login=check&$flavor")."'><img alt='login' src=/".drupal_get_path('theme','nordita')."/images/login.png /></a>");
      }
      $reply = x('ul class="mypear_login"',$reply);
    }
    return $reply;
  }

  /*
   *
   */
  public function mypear_module(){
    return (empty($GLOBALS['myPear_current_module'])
	    ? ''
	    : $GLOBALS['myPear_current_module'].' '.constant(strToUpper($GLOBALS['myPear_current_module']).'_VERSION'));
  }

  /*
   *
   */
  public function mypear_date(){
    if (class_exists('b_reg',False) && !empty(\b_reg::$modules[\b_reg::$current_module])){
      $reg = \b_reg::$modules[\b_reg::$current_module];
      $module_v = sprintf('%s&nbsp;%s',$reg['m'],$reg['v']);
      return $reg['r'];
    }else{
      return '';
    }
  }

  /*
   *
   */
  public function getFunctions(){
    foreach (array('module','date','login','fp_title','fp_content') as $ext){
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
