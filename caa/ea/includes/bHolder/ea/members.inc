<?php
locateAndInclude('bHolder_ea');
class bHolder_ea_members extends bHolder_ea{

  function listEntries(){
    $nLine = 0;
    $header = $this->header();  // sets also title                                                                                                                                                  
    $this->_open();
    if (empty($_GET['sBy']) || empty($header[$_GET['sBy']]))  $_GET['sBy'] = $this->sortBy_default;
    if ($q = $this->query()){
      while ($this->rec = myPear_db()->next_record($q)){
        if (!$this->rejectRecord()){
          if ($this->autoOpenHeader) $this->drawHeader();
          $this->format();
          if (isset($this->t) && ($this->t instanceof b_table)) $this->t->prt($this->rec,$this->highlight);
          if (++$nLine > $this->maxLines){
            print b_fmt::redText($this->maxLines." lines printed... Output stops");
            break;
          }
        }
      }
    }

  function format(){
    $this->doorLabels('collect',array());
  }

}
