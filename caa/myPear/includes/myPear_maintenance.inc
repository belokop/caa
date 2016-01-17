<?php

function myPear_doSomething($action=Null,$id=Null){
  if (empty($id))     $id = @$_GET['id_once'];
  if (empty($action)) $id = @$_GET['action_once'];
  switch($action){
  case 'clean_names':
    
    $q = myPear_db()->qquery("SELECT * FROM zzz_avatars ".myPear_db()->orderBy('av_lastname'),1);
    while ($r = myPear_db()->next_record($q)){
      $r['av_firstname'] = trim($r['av_firstname']);
      $r['av_lastname']  = trim($r['av_lastname']);
      $fn = b_fmt::unEscape(bForm_Avatar::_fmtName('f',$r));
      $ln = b_fmt::unEscape(bForm_Avatar::_fmtName('l',$r));
      if ($fn !== $r['av_firstname'] || $fn !== $r['av_firstname']){
	myPear::DEBUG("$r[av_lastname], $r[av_firstname]<br/>$ln, $fn");
      }
    }

    break;
  default:
    myPear::WARNING("How to do '$action'?");
  }
}
