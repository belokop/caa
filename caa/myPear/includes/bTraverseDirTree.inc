<?php
class bTraverseDirTree{
  
  /*
   * Traverse directory tree
   */
  static public function go($base,$dirFunc=null,$fileFunc=null,$afterDirFunc=null,$errorFunc=Null){
    $er = error_reporting(0);
    if (($subdirectories=opendir($base)) === False){
      if ($errorFunc!==null) $errorFunc($base);
    }else{
      while (($subdirectory=readdir($subdirectories))!==false){
	$path=$base.$subdirectory;
	if (!file_exists($path)){
	  if ($errorFunc!==null) $errorFunc($path);
	}elseif (is_dir($path)){
	  if (($subdirectory!='.') && ($subdirectory!='..')){
	    if     (($dirFunc!==null) && function_exists($dirFunc))              $dirFunc($path);
	    elseif (($dirFunc!==null) &&   method_exists('bTraverseDirTree',$dirFunc)) self::$dirFunc($path);
	    self::go($path.'/',$dirFunc,$fileFunc,$afterDirFunc,$errorFunc);
	    if (($afterDirFunc!==null)&& function_exists($afterDirFunc)) $afterDirFunc($path);
	  }
	}elseif (is_file($path)){
	  if     (($fileFunc!==null) && function_exists($fileFunc))              $fileFunc($path);
	  elseif (($fileFunc!==null) &&   method_exists('bTraverseDirTree',$fileFunc)) self::$fileFunc($path);
	}else{
	  print '?????????????????????????????????????????';
	}
      }
    }
    error_reporting($er);
  }

  /*
   * Get the depth of the directory tree
   */
  private static $depth = 0;
  static public function depth($base){
    self::$depth = 0;
    self::go($base,'depth_callback');
    return (self::$depth - count(explode('/',$base)) + 1);
  }

  /*
   * Get the directory structure in the form suitable for a menu
   */
  private static $tree = array();
  private static $base = Null;
  static public function tree($base){
    self::$tree = array();
    self::$base = $base;
    self::go($base,'tree_callback');
    return self::$tree;
  }

  private static function tree_callback($path){
    if (!is_link($path)){
      $stack = explode('/',str_replace(self::$base,'',$path));
      foreach($stack as $level=>$dir) if (!is_numeric($dir))  return;

      foreach($stack as $level=>$dir){
	switch($level){
	case 0:
	  if (empty(self::$tree[$stack[0]])) self::$tree[$stack[0]] = array();
	  break;
	case 1:
	  if (empty(self::$tree[$stack[0]][$stack[1]])) self::$tree[$stack[0]][$stack[1]] = array();
	  break;
	case 2:
	  if (empty(self::$tree[$stack[0]][$stack[1]][$stack[2]])) self::$tree[$stack[0]][$stack[1]][$stack[2]] = array();
	  break;
	default:
	}
      }
      MSG::DEBUG(str_replace(self::$base,'',$path));
    }
  }

  /*
   */
  private static function depth_callback($path){
    self::$depth = max(self::$depth , count(explode('/',$path)));
  }
  
}
