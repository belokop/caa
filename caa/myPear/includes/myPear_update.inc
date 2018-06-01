<?php

function myPear_update(){
  MSG::H1('Checking for updates');

  if ($path = @$_GET['update_once']){
    MSG::MESSAGE(x('pre',`svn up $path 2>&1`));
  }


  foreach(array_keys(b_reg::$modules) as $module){

    list($path,$version,$sversion) = myPear::module_info('version',$module);

    $table[$path] = array('m' =>x('strong',$module),
			  'p' =>$path,
			  'v' =>$version,
			  's' =>$sversion,
			  );
  }
  
  if (!empty($table)){
    $t = new b_table_zebra(array('m'=>'module',
				 'p'=>'real path',
				 'v'=>'local<br/>version',
				 's'=>'server<br/>version'));
    $t->css['v']['align'] = 'align_right';
    $t->css['s']['align'] = 'align_right';
    
    krsort($table);
    foreach($table as $path=>$data){
      if  (empty($data['s']))                      $button = bIcons()->get('ok');
      elseif(($s=$data['s']) == ($v=$data['v']))   $button = bIcons()->get('ok');
      elseif(is_numeric($s)&&is_numeric($v)&&$v>$s)$button = bIcons()->get('ok');
      else         $button = bIcons()->getButton(array('l'=>b_url::same("?update_once=$path&resetcache_once=yes"),
							       'i'=>'update'));
      $t->extraTD[] = $button;
      $t->prt($data);
    }
    $t->close();
  }
}

