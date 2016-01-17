<?php
//$_GET['showL'] = 1; $_GET['showl'] = 1;
require_once '../myPear/config.inc';
require_once '../vm/config.inc';
locateAndInclude('pdf_form');

#$v = myPear::getInstance('bForm_vm_Visit',45878,'fatal');
#$v = myPear::getInstance('bForm_vm_Visit',45654,'fatal');
$v = myPear::getInstance('bForm_vm_Visit',45422,'fatal');
$exp = $v->getExp();

switch(True){
case True:
  foreach(VM::$forms2send as $template){
    $exp->pdfReceipt($template,"/tmp/$template.pdf"); 
  }
  break;
  
default:
  foreach(    $v->formDB as $k=>$vv) $formDB[$k] = $vv;
  foreach($v->av->formDB as $k=>$vv) $formDB[$k] = $vv;
  $formDB['pdf_Lf'] = $v->av->fmtName('L,f');
  
  
  foreach(array('scholarship','nordea') as $tpl){
    locateAndInclude($tpl);
    
    $fdf = array();
    foreach($tpl() as $code=>$a){
      if ($def = @$a['d']){
	print "--$code = '$def'\n";
	$fdf[$a['t']] = $def;
      }elseif ($ref = @$a['c']){
	print "--$ref\n";
	$fdf[$a['t']] = b_fmt::unEscape(@$formDB[$ref]);
      }
    }
  b_debug::print_r($fdf);
  print pdf_form::fill($fdf,vm_root."templates/$tpl.pdf");//,"/tmp/$tpl.pdf");
  print "\n";
  }
  print "\n";
}
