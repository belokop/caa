<?php
locateAndInclude('bList_vm');

class bList_vm_socialEvents extends bList_vm{

  protected $packed_items = array('s_price');
  var $list_quering = array('lm_key'   =>'what',  // link to the "social events setup" record
			    'lm_value' =>'when',  // social event date
			    );
  var $list_showing = array('_when'    =>'',
			    '_what'    =>'',
			    '_n'       =>'number of<br/>participants',
			    '_budget'  =>'budget<br/>estimation', // sick! redefined in function __construct
			    );
  
  var $orderBy = 'lm_value';
  var $total   = 0;

  function __construct($e){
    locateAndInclude('bForm_vm_Event');
    
    $this->list_showing['_budget']  ='budget<br/>estimation<br/>('.trim(CONST_currency).')';

    parent::__construct($e);

    // Sanity
    if (cnf_dev){
      if (!$this->budgetSource_exists())    $this->deleteMember();
      $members = $this->getMembers(array('lm_value'=>''));
      if (!$this->myPost_bList()) b_debug::print_r($members,'cnf_dev - bogus record with empty lm_value');
    }

    // Reset also the rates also the social events are wiped out
    if ((@$_GET['drop_list_once'] !== Null) && (@$_GET['drop_list_once'] == $this->ID)){
      bList::deleteList(VM_socialEventRates($this->e)->ID,True);
      if($this->budgetSource_exists()) VM_socialEventRates($this->e)->setDefaults();
    }
    
    // Initialize the list if requested,
    // by default the Social Events are not created until the explicit request.
    // This gives a chance to use the most recent rates for the catering services 
    // (assuming that the "global rates" are updated and correct).
    if (($l_id = @$_GET['drop_list_once']) == $this->ID){
      bList::deleteList($l_id,True);
    }elseif (@$_GET['init_list_once'] === 'yes'){
      $this->init();
    }

    // clean the database
    $this->deleteMember(array('lm_value'=>''),$verbose=False);
  }

  /*
   * Do not send a message when the empty bList_vm_socialEvents is created,
   * do it when it is explicitly initialized
   */
  protected function message_createList(){}

  /*
   *
   */
  protected function message_deleteMember($args){
    foreach($this->getMembers($args) as $lm_id=>$r){
      $ser = VM_socialEventRates($this->e)->getMember(array('lm_id'=>$r['lm_key']));
      $id  = (empty($r['lm_value']) && empty($ser['lm_key'])
	     ? 'empty record'
	     : "record ".x('em',"\"$r[lm_value] - $ser[lm_key]\""));
      MSG::MESSAGE("Delete $this->message_deleteMember_custom $id from the ".$this->name(True));
    }
  }
  
  /*
   *
   */
  private $budgetSource_exists = Null;
  private function budgetSource_exists(){
    if ($this->budgetSource_exists === Null) $this->budgetSource_exists = (bool)!b_posix::is_empty($this->e->budgetSource()->get_budgetSource());
    return $this->budgetSource_exists;
  }
  
  private function reset_cache(){
    $this->cache_bList_getMembers = array();
  }
  
  /*
   * Create the the "compulsory service entries", like coffee breaks,
   * if there is a budget source.
   */
  private function init($lm_id=0){
    if($this->budgetSource_exists() && $this->isWritable()){
      if (!empty($lm_id) || !$this->isReady()){
	$includeDay1 = True;
	$this->dbg($this->e->__toString());
	foreach(VM_socialEventRates($this->e)->getCompulsoryServices($lm_id) as $ser_array){
	  list($lm_id_comp,$ser_name,$ser_price,$x) = $ser_array;
	  for($date =$this->e->getValue('e_start')+($includeDay1 ? 0 : 24*3600); 
	      $date<=$this->e->getValue('e_end');
	      $date+=24*3600){
	    $ev_date = date('Y-m-d',$date);
	    if (!b_time::isRedDay($date)){
	      if (!($r=$this->getMember($args=array('lm_value'=>$ev_date,
						    'lm_key'  =>$lm_id_comp)))){
		$args['s_price'] = $ser_price;
		$this->dbg("$ev_date $ser_name");
		$this->addMember($args);
	      }
	    }
	  }
	}
      }
    }else{
      MSG::WARNING("Can't setup Social Events for ".x('em',x('"',$this->e->name())));
      bList::deleteList($this->ID,$membersOnly=True);
      $this->reset_cache();
      $this->MODE = 'RO';
    }
    $this->dbg('done');
  }

  /*
   *
   */  
  protected function addMember_check_args(&$args) {
    foreach(preg_grep('/^lm_/',array_keys($args)) as $arg) $vital_args[] = $arg;
    if (@$vital_args === array('lm_key')){
      $a['lm_value']=$args['lm_value'] = date('Y-m-d',(int)($this->e->getValue('e_end')/2 + $this->e->getValue('e_start')/2));
    }
    b_debug::xxx(@$vital_args);
    b_debug::xxx(@$a);
  }

  /*
   * The class is ready if there are social event already created
   */
  function isReady($setReady=False){
    return (count($this->getMembers()) > 0);
    if (func_num_args() != 0){
      $reply = parent::isReady($setReady);
    }else{
      $reply = parent::isReady();
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }
  
  /*
   * This list might be administrated by the event organizers
   */
  function isAdmin(){
    return VM_prg_coordinator_here || VM_organizer_here;
  }

  /*
   * Called by the function format
   */
  protected function isMemberDeletable(){
    $reply = VM_administrator_here || (($this->e->isOrganizer() || VM::hasRightTo('endorse_event')) && 
				       !VM_socialEventRates($this->e)->isCompulsory($this->r['lm_key']));
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   *
   */
  protected function isMemberEditable(){
    $reply = VM_administrator_here || (parent::isMemberEditable() && 
				       !VM_socialEventRates($this->e)->isCompulsory($this->r['lm_key']));
    $this->dbg($reply);
    return $reply;
  }

  //  return " LEFT JOIN zzz_lists ON $this->TABLEID = $this->TABLEID_member_p" ;
  protected function hookJOIN_ZZZ(){
    return join(' ',array(" LEFT JOIN abs_events ON e_id = lm_status ",
			  parent::hookJOIN()));
}

  /*
   * Check the input data validity
   */
  public function getFormAdd_OK(){
    $lm_id = $this->getValue('lm_id');
    if (@$_GET['add_continue_once'] === 'yes'){
      $reply = True;
    }elseif ($reply = parent::getFormAdd_OK()){
      $date = $this->getMemberValue($lm_id,'lm_value');
      if (empty($date)){
	$reply = False;
      }elseif ($date < date('Y-m-d',$this->e->getValue('e_start')) ||
	       $date > date('Y-m-d',$this->e->getValue('e_end'))){
	MSG::ERROR("Can't setup social event for $date".
				 ", which is is outside the conference period ".b_time::period($this->e->getValue('e_start'),$this->e->getValue('e_end')));
	$reply = False;
      }
    }
    $this->dbg($reply,cnf_dev);
    return $reply;
  }

  /*
   * Catch the "social event every day" request
   */  
  function addMember($args=array()){
    // Catch the "every day" case
    if (empty($_POST['p_isDD'])){
      return parent::addMember($args);
    }else{
      $this->init($_POST['lm_key']);
    }
  }

  function isCompulsory($lm_id){
    return False;
    return VM_socialEventRates($this->e)->isCompulsory($lm_id);
  }
  
  
  /*
   * "Add social event" dialog
   */
  protected function getFormAdd(){

    $this->show_gv=cnf_dev;

    $lm_id = $this->getValue('lm_id');
    if (empty($lm_id))  $lm_id = $this->createMember();

    $lm_key = $this->getMemberValue($lm_id,'lm_key'); 

    // Get the event special price (or the default one) 
    $s_price= $this->getMemberValue($lm_id,'s_price'); 
    
    // Build the dialog
    $this->bForm($this->list_quering,$inversed=True);
    $this->f->show_gv = $this->show_gv;
    $this->f->formDB['lm_id']  = $lm_id; 
    $this->f->formDB['lm_key'] = $lm_key;


    $this->f->isRO['_what'] = True;
    $this->f->formDB['_what']= VM_socialEventRates($this->e)->getMemberValue($lm_key,'lm_key');
    $this->f->defineVariable(array('_what'=>$this->list_quering['lm_key']),True);
    $this->f->textField('_what');

    $this->f->textHidden('lm_key',$lm_key);
    $this->f->textHidden('lm_id', $lm_id);
    
    // Set the price && dates if the SE is chowsen
    if (!empty($lm_key)){
      // Date
      if (VM_socialEventRates($this->e)->isCompulsory($lm_key)) $this->f->isRO['lm_value'] = True;
      // Price
      list($s_price,$p_model) = VM_socialEventRates($this->e)->getPriceWithModel($lm_key,$s_price,True);
      $this->f->formDB['s_price'] = $s_price;
      $this->f->isRO['s_price']   = !$this->isWritable();
      if ($this->f->isWritable()){
	$this->list_quering['s_price'] = 'price';
	$this->f->defineVariable(array('s_price'=>'price '.$p_model),True);
	$this->f->textField('s_price');  
      }else{
	$this->list_quering['_s_price'] = 'price';
	$this->f->formDB['_s_price'] = VM_socialEventRates($this->e)->getPriceWithModel($lm_key,$s_price);
	$this->f->textField('_s_price');  
	$this->f->textHidden('s_price');
      }

      // Set the date to the "half way" if it is not defined yet
      if (empty($this->f->formDB['lm_value']) || (b_time::txt2unix($this->f->formDB['lm_value']) < b_time::noon('1997'))){
	$this->f->formDB['lm_value'] = date('Y-m-d',(int)($this->e->getValue('e_end')/2 + $this->e->getValue('e_start')/2));
	if (VM_socialEventRates($this->e)->isDailyPossible($lm_id)){
	  $what = VM_socialEventRates($this->e)->getMemberValue($lm_key,'lm_key');
	  $this->f->submitOnChange['p_isDD'] = True;
	  $this->f->isMBE['p_isDD']= True;
	  $this->f->vars['p_isDD'] = "Check if $what should be every day";
	  $this->f->checkBox('p_isDD');
	}
      }
      $this->f->datePickerField('lm_value',array($this->e->getValue('e_start'),$this->e->getValue('e_end')));
    }
    $this->f->closeForm();
  }

  /*
   * Page header
   */
  protected function setPageHeader() {
    MSG::H1($this->e->name(),'reset');
  }


  /*
   * Header for the list of events
   */ 
  protected function header($arg=null){
    $header = $this->list_showing;
    return $header;
  }

  /*
   * Formatting for the members list
   */
  private $previous_day = '?';
  protected function format(){
    if (empty($this->r['lm_key']) || empty($this->r['lm_value'])) {
      b_debug::print_r($this->r,'empty lm_key OR lm_value');
    }else{
      $n_attenders = count($this->e->attendersToday(b_time::txt2unix($this->r['lm_value'])));
      $price = VM_socialEventRates($this->e)->get_ser_price($this->r['lm_key'],@$this->r['s_price'],$n_attenders);
      $what  = VM_socialEventRates($this->e)->getMemberValue($this->r['lm_key'],'lm_key');
      if ($n_attenders > 0 && (empty($price) || empty($what))){
	MSG::WARNING_T("??? price='$price' what=''$what'");
	if (cnf_dev) b_debug::print_r($this->r,"??? price='$price' what=''$what'");
	$this->r = array();
      }else{       
	$this->t->showLineCounter = False;
	unset($GLOBALS['b_fmt::money']);
	
	$this->r['_budget'] = b_fmt::escape(b_fmt::money($price));
	$this->r['_what']   = $what;
	$this->r['_when'] = ($this->r['lm_value'] == $this->previous_day ? '' :x('i',$this->r['lm_value']));
	$this->r['_n']    = $n_attenders;
	
	// get the total spending
	$this->get_budget($what,$price);
	
	// highlight the non-compulsory spending
	if (!VM_socialEventRates($this->e)->isCompulsory($this->r['lm_key'])) $this->r['_what'] = x('strong',$this->r['_what']);
	
	array_unshift($this->t->extraTD,
		      bIcons()->getButton(array('d'=>'Get list of participants',
						'i'=>'bb-group',
						'l'=> b_url::same('?w_once='.strip_tags($this->r['_what']).'&doing_once=reception_attenders&d_once='.
								  $this->r['lm_value']))).'&nbsp;');
      }
      $this->previous_day = $this->r['lm_value'];
    }
  }
  
  /*
   *
   */
  public function get_budget($what='',$budget=0){
    if (!empty($what) && !empty($budget)){
      if (empty($this->summary[$what])) $this->summary[$what] = 0;
      $this->summary[$what] += $budget;
      $this->total += $budget;
    }elseif (is_object($this->e)){ 
      $this->summary = array();
      $this->total = 0;
      foreach($this->getMembers() as $lm_id=>$r){
	// $ser_record=VM_socialEventRates($this->e)->getMember(array('lm_id'=>$r['lm_key']),True);
	$n_attenders = count($this->e->attendersToday(b_time::txt2unix($r['lm_value'])));
	$this->get_budget(//  $ser_record['lm_key'],    
			  VM_socialEventRates($this->e)->getMemberValue($r['lm_key'],'lm_key'),
			  VM_socialEventRates($this->e)->get_ser_price($r['lm_key'],@$r['s_price'],$n_attenders));
      }
      b_debug::print_r($this->summary[$what],"total=".$this->total);
    }
  }


  /*
   *
   */
  protected function draw_buttons(){
    $reply = array();
    if ($this->isWritable()){
      if ($this->e->attenders(True)){
	if ($this->budgetSource_exists()){
	  $reply[] = ($this->isReady()
		      ? b_btn::submit('add new social event',b_url::same("?function_form=VM_socialEventRates&arg1_once=".$this->parent_ID))
		      : b_btn::submit('initialize social events',b_url::same("?init_list_once=yes&resetcache_once=1")));
	  if (VM_administrator_here && $this->isReady()) $reply[] = b_btn::submit('wipe all social events',
										  b_url::same("?drop_list_once=".$this->ID),
										  $confirm=True);
	}else{
	  print x('i',"There is no budget for social events");
	}
      }else{
	MSG::WARNING("There is no approved attenders for this event, can't initialize the Social events");
      }
    }
    return $reply;
  }

  /*
   *
   */
  protected function draw_summary() {
    if ($this->isReady()){
      $this->t->prt(array('_budget'=>'<hr/>'));
      $this->t->prt(array('_budget'=>x('strong',b_fmt::escape(b_fmt::money($this->total)))));
    }else{
      MSG::WARNING("'Social events' are not yet initialized");
    }
  }
}
