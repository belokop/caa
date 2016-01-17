<?php
/*
 * plugin to present table as an Excel spreadsheet
 */

require_once "Spreadsheet/Excel/Writer.php";

class b_table_excel extends b_table{
  
  var $_filename  = 'excel_file.xls';
  var $titleText  = '';
  var $_f         = False;
  var $currentRow = 2;
  var $currentCol = 0;
  var $nCols      = 0;
  var $c2c = array('bIcal_weekend'  => 'red',
		   'bIcal_inoutday' => 'yellow',
		   'ical_[0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z][0-9a-z]\b'=> 'fix');
  
  function __construct($titleText='Excel spreadsheet',$_filename=False){
    if ($_filename) $this->_filename = $_filename;
    $this->titleText = $titleText;
  }
  
  function open(){
    if ($this->sheetOpen++) return;
    $this->xls = new Spreadsheet_Excel_Writer();
    $this->ws  = $this->xls->addWorksheet(get_class($this)); // Create worksheet
    $f = $this->xls->addFormat();
    $f->setFontFamily('Helvetica');
    $f->setBold();
    $f->setSize('13');
    $f->setColor('navy');
    $f->setBottom(2);            // the bottom border width is "thick"
    $f->setBottomColor('yellow');// the color of the bottom border
    $f->setAlign('center');
    $this->ws->setMerge(0,0,1,$this->nCols+3);
    $this->ws->write(0,0,$this->titleText,$f); 
    
    /* The cell group to freeze
     *   1st Argument - vertical split position
     *   2st Argument - horizontal split position (0 = no horizontal split)
     *   3st Argument - topmost visible row below the vertical split
     *   4th Argument - leftmost visible column after the horizontal split
     */
    //$this->ws->freezePanes(array(3,0,4,0));

    $this->ws->setRow(0,30);               // row height
    $this->ws->setColumn(0,0,1); 
    $this->ws->setColumn(1,1,10); 
    $this->ws->setColumn(2,$this->nCols+3,2);   
    $this->td = $this->tr = 0;
  }
  
  function tro($attr='') { 
    $this->open();
    $this->tr++;
    $this->currentRow++; 
    $this->currentCol = 0;
    $this->tdCount = 0;
  }

  function trc() { 
    $this->tr--;
  }

  function tdo($attr='') { 
    $this->td++;
    $this->tdCount++;
    $fg=NULL; foreach ($this->c2c as $html=>$excel)
      if (preg_match("/$html/",$attr)) $fg = $excel;
    if ($fg=='fix'){
      if (($this->tdCount)%2 == 0) $fg='lime';
      else                         $fg='aqua';
    } 
    $this->_f = $this->xls->addFormat();
    if (!empty($fg)) $this->_f->setFgColor($fg);
    $this->_f->setFontFamily('Helvetica');
    $this->_f->setSize(8);
    $this->_f->setColor('navy');
    $this->_f->setBorderColor('silver');
    $this->_f->setBorder(1);
    
    if (preg_match('/colspan=([0-9]*)\b/',$attr,$m)) {
      $this->colspan = (int)$m[1] - 1;
      $this->ws->setMerge($this->currentRow,$this->currentCol,
			  $this->currentRow,$this->currentCol+$this->colspan);
    }else
      $this->colspan = 0;

    if (False && $fg) { ob_start(); print_r($this); $txt = ob_get_contents(); ob_end_clean();  file_put_contents("/tmp/x",$txt); }
  }

  function td($attr='',$body='',$th=False) { 
    $this->tdo($attr);
    $this->ws->write($this->currentRow,
		     $this->currentCol,
		     $this->strip_tags($body),
		     $this->_f); 
    $this->tdc();
  }

  function tdc() { 
    $this->td--;
    $this->currentCol++; 
    $this->currentCol+= $this->colspan;
}
  
  function tho($a='') { $this->tdo($a); }
  function thc()      { $this->tdc(); }
  function th ($attr,$body) { $this->td($attr,$body,True); } 

  function close(){
    $this->xls->send($this->_filename);
    $this->xls->close();
  }

  function strip_tags($text){
    return preg_replace('/&nbsp;/', ' ', strip_tags($text));
  }
}
