<?php
locateAndInclude('bHolder_vm');
class bHolder_vm_Events extends bHolder_vm{

  var $noSort = True;
  var $showLineCounter = False;

  public function __construct(){
    if (!empty($_REQUEST['vm_eventSelector'])){
      b_url::header(b_url::same(sprintf('?q=%s/%s&vm_eventSelector=%s',
					VM_MODULE,
					APImenu::tab_code(tab_vm_vis),
					$_REQUEST['vm_eventSelector'])));
    }
    parent::__construct();

    // Set "archived" bit for the old events
    if (!@$_SESSION['auto_archive']++){
      locateAndInclude('bForm_vm_Event');
      foreach(bForm_vm_Event::getEvents() as $e_id){
	$e = new bForm_vm_Event($e_id);
	if ((time() - AUTO_EXPIRATION*86400) > $e->getValue('e_end',1)){
	  $e->setPolicy(VM_E_archived,1,$verbose=True);
	}else{
	  if (cnf_dev) myPear::MESSAGE(sprintf("KEEP %s",$e->name()));
	}
      }
    }
  }
  
  function query(){
    $sBy = 'e_start';
    $sByDir = 'DESC';
    return myPear_db()->query("SELECT *,e_start AS name  FROM abs_events WHERE ".
			      ($this->onlyAgenda?'e_code>0':'1').
			      " ORDER BY $sBy $sByDir");
  }
  
  function header(){
    return array('year_nosort'   => ' ',
		 'name'   => ' ',
		 );
  }

  function format() {
    static $year_was = '?';

    $e = myPear::getInstance_new('bForm_vm_Event',$this->rec['e_id'],array('nocache','fatal'));
    if ($e->getValue('e_code')){
      $year = date('Y',$this->rec['e_start']);
      if ($year != $year_was) $this->t->prt(array('year_nosort'=>x('h3',$year)));      $year_was = $year;
      $this->rec['e_start'] = x('code',date('d M - ', $this->rec['e_start']));
      $this->rec['e_end']   = x('code',date('d M Y',$this->rec['e_end']));
    }
    $this->rec['name'] = x("a href=".b_url::same(sprintf("?vm_eventSelector=%s",$e->ID)),$e->name($includeDate=True,$max_length=66));
    if (False){
      if ($e->isWritable())       $this->t->extraTD[] = b_btn::link('',
								    $e->sameURL_but(),
								    bIcons()->get('i-edit')); 
      if ($this->rec['e_code']>0) $this->t->extraTD[] = b_btn::link('',"http://agenda.albanova.se/conferenceDisplay.py?confId=".$this->rec['e_code'],bIcons()->get('nordita')); 
    }
    //    axel($this->rec,$e->name());
  }
}

function axel($rec,$n){
  if (!myPear_db()->tableExists('registrants')) return;
  if ($rec['e_code'] <=0) return;
  $line = 
    "program $n / ".
    (int)((b_time::txt2unix($rec['e_end']) - b_time::txt2unix($rec['e_start']))/(24*3600)).' days';
  print "$line<br/>";
  system("echo $line>>/tmp/axel.txt");
  $q = myPear_db()->query("SELECT * FROM registrants WHERE v_eid = $rec[e_id]");
  while ($r = myPear_db()->next_record($q)){
    if (preg_match("/(will arrange|my way|my own|other)/",$r['v_acc_wish']))
      $flag = 'local';
    else $flag = 'external';
    $line = 
      (int)(($r['v_end'] - $r['v_start'])/(24*3600)+1)." $flag";;
    print "$line<br/>";
    system("echo $line>>/tmp/axel.txt");
  }
}
