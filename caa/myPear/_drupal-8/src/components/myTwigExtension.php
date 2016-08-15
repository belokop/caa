<?php

namespace Drupal\myPear\components;

use Drupal\myPear\access\AccessController;
use Drupal\Core\Access\AccessResult;
use Drupal\Core\Template\TwigExtension;
use Symfony\Component\Routing\Route;


if ($path = drupal_get_path('module','myPear')){
  require_once $path . '/includes/drupal8_compat.inc';
  require_once $path . '/config.inc';
}elseif (!defined('myPear_MODULE')){
  define('myPear_MODULE','myPear');
}

/*
 */
class myTwigExtension extends \Twig_Extension{

  private $serviceID = Null;
  
  public function __construct(SecurityService $serviceID= null){
    if (function_exists('locateAndInclude')) locateAndInclude('APImenu');
    $this->serviceID = $serviceID;
  }

  /*
   *
   */  
  public function mytwig_is_fp(){
    return \D8::is_fp();
  }

  public function mytwig_fp_content(){
    \D8::dbg();
    myPear_init();
    \b_reg::load_module();
    $tabh[] = x('h4','Select the desired Application:');
    $AC = new AccessController();
    foreach(\b_reg::$modules as $module=>$descr){
      if ($AC->access(new Route("/$module/")) == AccessResult::allowed()){
	\D8::dbg($module);
	$tabh[] = "<li ><a href='$module?q=$module'>$descr[d] </a></li>";
      }
    }
    return $this->signature(__FUNCTION__,x('ul',join("\n",$tabh)));
  }
  
  /*
   *
   */
  public function mytwig_fp_title(){
    return $this->signature(__FUNCTION__,
			    'Welcome to the '.(defined('myOrg_name') ? myOrg_name : '').' <br/>Computer Aided Administration');
  }
  
  /*
   * 
   */
  public function mytwig_login(){
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
      $reply = x('ul class="mytwig_login"',$reply);
    }
    \D8::dbg(\b_fmt::escape($reply));
    return $this->signature(__FUNCTION__,$reply);
  }

  /*
   *
   */
  public function mytwig_module(){
      return $this->signature(__FUNCTION__,
                              (empty($GLOBALS['myPear_current_module'])
                               ? ''
                               : $GLOBALS['myPear_current_module'].' '.constant(strToUpper($GLOBALS['myPear_current_module']).'_VERSION')));
  }

  /*
   *
   */
  public function mytwig_date(){
    if (class_exists('b_reg',False) && !empty(\b_reg::$modules[\b_reg::$current_module])){
      $reg = \b_reg::$modules[\b_reg::$current_module];
      $module_v = sprintf('%s&nbsp;%s',$reg['m'],$reg['v']);
      return $this->signature(__FUNCTION__,$reg['r']);
    }else{
      return $this->signature(__FUNCTION__);
    }
  }

  /*
   *
   */
  public function mytwig_showtime($args){

    $timing = '<!-- no timing information -->';

    ob_start();
    bTiming()->show();
    $timing1 = ob_get_contents();
    ob_end_clean();

    ob_start();
    bCount()->show();
    $timing2 = ob_get_contents();
    ob_end_clean();

    if (!empty($timing1) || !empty($timing2)){
      $timing = "<table><tbody>\n<tr>\n";
      if (!empty($timing1)) $timing .= "<td>$timing1</td>\n";
      if (!empty($timing2)) $timing .= "<td>$timing2</td>\n";
      $timing .= "</tr></tbody></table>\n";
    }

    return $this->signature(__FUNCTION__,$timing);
  }

  /*
   * NOT A GOOD IDEA. To be removed
   *
   * D8 features Workaround test... 
   * hide the non-active modules in the menu.
   * See also AccessController
   */
  public function mytwig_hidden_item(\Drupal\Core\Url $url){

    $to_be_hidden = False;
    return $to_be_hidden;
  }

  /*
   * Debugging...
   */
  private function signature($who,$body=''){
      return (defined('cnf_dev') && cnf_dev
              ? join("\n",["","<!-- start twig extension $who -->",$body,"<!-- end twig extension $who -->",""])
              : $body);
  }
  
  /*
   *
   */
  public function getFunctions(){
    $extensions = preg_grep('/mytwig_/',get_class_methods($this));
    foreach ($extensions as $ext){
      $functions[$ext] = new \Twig_Function_Method($this, $ext);
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
    $this->serviceID->bar($param);
    return __METHOD__;
  }
}
