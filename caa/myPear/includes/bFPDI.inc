<?php

require_once(dirname(__FILE__).'/_tcpdf/tcpdf.php');
require_once(dirname(__FILE__).'/_fpdi/fpdi.php');

class bFPDI extends FPDI{
/**
 * Merge a set of PDFs
 *
 * @param string $output_file File path
 * @param array  $pdffiles File paths
 * @param array  $titles   Optional bookmarks
 */
  public function merge($output_file, $pdffiles=array(), $titles = array()){

    // FPDI throws some notices that break the PDF
    $old_error = error_reporting(E_ERROR | E_WARNING | E_PARSE);
    
    foreach($pdffiles as $pdffile){
      if (file_exists($pdffile)){
	$count = $this->setSourceFile($pdffile);
	for($i=1; $i<=$count; $i++){
	  $template = $this->importPage($i);
	  $size = $this->getTemplateSize($template);
          
	  $this->AddPage('P', array($size['w'], $size['h']));
	  $this->useTemplate($template, null, null, null, null, true);
	  if ($i == 1){
	    if (isset($titles[$pdffile])) $bookmark = html_entity_decode($titles[$pdffile]);
	    else                          $bookmark = $pdffile;
	    $this->Bookmark($bookmark, 1, 0, '', '', array(0,0,0));
	  }
	}
      }
    }
    error_reporting($old_error);
    $this->Output($output_file, 'F');
  }

  public function concat($output_file, $pdffiles=array()){
    foreach($pdffiles as $file) { 
      $pagecount = $this->setSourceFile($file); 
      for ($i = 1; $i <= $pagecount; $i++) { 
	$tplidx = $this->ImportPage($i); 
	$s = $this->getTemplatesize($tplidx); 
	$this->AddPage($s['w'] > $s['h'] ? 'L' : 'P', array($s['w'], $s['h'])); 
	$this->useTemplate($tplidx); 
      } 
    } 
    $this->Output($output_file, 'F'); 
  } 
}
