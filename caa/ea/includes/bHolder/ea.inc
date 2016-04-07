<?php
locateAndInclude('bHolder');
class bHolder_ea extends bHolder{

  var $sortBy_default = 'e_start';
  var $onlyAgenda   = False;
  var $highlight    = '';
  var $autoOpenHeader = True;

  function __construct(){
    parent::__construct();
  }

  function getEntries(){
    return $this->getAllEvents();
  }
  
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
    
    $this->listEntries_summary();
    if (isset($this->t) && ($this->t instanceof b_table)){
      $this->t->close();
      $this->_close();
      bIcons()->explain();
    }else{
      if (myPear_db()->num_rows($q)) $this->noSuccess();
      $this->_close();
    }
  }

  function isWritable(){
    $reply = EA_administrator_here;
    $this->dbg($reply,$this->debug || cnf_show_w);
    return $reply;
  }

  function listEntries_summary() {}

  function getAllEvents(){
    $q = myPear_db()->query("SELECT * FROM abs_events WHERE ".
			      ($this->onlyAgenda?'e_code>0':'1').
			      " ORDER BY e_start");
    $list = array();
    while ($r=myPear_db()->next_record($q)) $list[]=$r['e_id'];
    return $list;
  }

  private $nLabelsPrinted = 0;
  protected function doorLabels($whatToDo,$o_id=Null,$condition=array('o_status','visitors'),$items=array('av_institute')){
    static $dejaVu = 0;
    if (!$dejaVu++ && function_exists('drupal_add_css')) drupal_add_css('./'.drupal_get_path('module',EA_MODULE).'/css/doorlabel.css');
    
    switch($whatToDo){
    case 'collect':
      if ($condition && ($this->rec[$condition[0]] != $condition[1])) return;
      if (empty($o_id))   return;

      $av = myPear::getInstance_new(bForm_Avatar::$bForm_Avatar,$this->rec['av_id'],array('nocache','fatal'));
      $img = $av->getPhoto(True);
      if (empty($img)) $img = myPear_images."avatar.png";

      if (empty($this->labels[$o_id])){
	$oc = myPear::getInstance_new(Null,$o_id,array('nocache','fatal'));
	$this->labels[$o_id][] = x('h2',$oc->getValue('o_status').' - '.$oc->name(False));
      }
      $info = array(x('strong',$av->fmtName('fl',False)));
      foreach($items as $item) if ($i=$av->getValue($item,1)) $info[] = x('em',str_replace(',',', ',$i));
      $this->labels[$o_id][] = array($img,$info);
      $this->rec = array();
      break;
      
    case 'print':
      if ($this->nLabelsPrinted == 0) print(x('style','@media print{ footer {page-break-after:always;}').
					    x('footer',' '));
      $css_class = 'doorlabels';
      foreach($this->labels as $o_id=>$people){
        $o_name = array_shift($people);
        print sprintf("<div id='maildb_doorlabel' class='label labelclass%s'>",min(4,count($people))).
          x('div',"<img src='".drupal_get_path('module',EA_MODULE)."/css/doorlabel-logo.png' alt=''>");
        print "<table><tbody><tr>\n";
	$n_people = count($people);
        foreach($people as $user){
	  list($img,$info) = $user;
	  $scale = ($n_people == 1 ? 1 : 1+$n_people/10);
	  $i = getimagesize($img);
	  $w = (int)($i[0]/$scale);
	  $h = (int)($i[1]/$scale);
	  $img = sprintf('<img width="%d" height="%d" src="%s" alt="">',$w,$h,$img);

          if ($n_people == 1){
	    print x('td',$img);
	    print x('td',join('<br>',$info));
	  }elseif ($n_people<5){
	    print x('td',join('<br>',array_merge(array($img),$info)));
	  }else{
	    print x('td',join(', ',$info)).'</tr><tr>';
	  }
	}
        print "</tr></tbody></table>\n"."</div>\n".x('div',$o_name).str_repeat("<p>&nbsp;</p>\n",1);
	$this->nLabelsPrinted++;
	if ($this->nLabelsPrinted%3 == 0) print(x('footer',' '));
      }
    }
  }
  
  function _emptySelection() {}
}
