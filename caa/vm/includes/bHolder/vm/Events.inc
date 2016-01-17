<?php
locateAndInclude('bHolder_vm');
class bHolder_vm_Events extends bHolder_vm{

  var $sortBy_default = 'name';

  public function __construct(){
    parent::__construct();
  }
  
  function query(){
    return myPear_db()->query("SELECT *,e_start AS name  FROM abs_events WHERE ".
				($this->onlyAgenda?'e_code>0':'1').
				" ORDER BY $_GET[sBy] $_GET[sByDir]");
  }
  
  function header(){
    return array('name'          => 'title',
		 'e_start_nosort'=> 'start',
		 'e_end_nosort'  => 'end');
  }

  function format() {
    $e = myPear::getInstance('bForm_vm_Event',$this->rec['e_id']);
    if ($e->formDB['e_code'] > 0){
      $this->rec['e_start_nosort'] = date('d M - ', $this->rec['e_start']);
      $this->rec['e_end_nosort']   = date('d M Y',$this->rec['e_end']);
    }
    $this->rec['name']    = b_fmt::strcut($e->name(), 44, '...');
    if ($e->isWritable())      $this->t->extraTD[] = b_btn::link('',
								  $e->sameURL_but(),
								  bIcons()->get('i-edit')); 
    if ($this->rec['e_code']>0) $this->t->extraTD[] = b_btn::link('',"http://agenda.albanova.se/conferenceDisplay.py?confId=".$this->rec['e_code'],bIcons()->get('nordita')); 
    $this->t->repetValuesCol1only = False;
    axel($this->rec,$e->name());
  }
}

function axel($rec,$n){
  if ($rec['e_code'] <=0) return;
  $line = 
    "program $n / ".
    (int)((b_time::txt2unix($rec['e_end']) - b_time::txt2unix($rec['e_start']))/(24*3600)).' days';
  print "$line<br/>";
  system("echo $line>>/tmp/axel.txt");
  $q = myPear_db()->query("SELECT * FROM registrants WHERE v_eid = $rec[e_id]");
  while ($r = myPear_db()->next_record($q)){
    if (preg_match("/(will arrange|my way|my own|other)/",$r['v_comment']))
      $flag = 'local';
    else $flag = 'external';
    $line = 
      (int)(($r['v_end'] - $r['v_start'])/(24*3600)+1)." $flag";;
    print "$line<br/>";
    system("echo $line>>/tmp/axel.txt");
  }
}
