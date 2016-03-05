<?php
/*
 * Keep an editable list of available social events
 *
 * Record structure:
 *  lm_key   - textual description of the social event
 *  lm_value - price  
 *  lm_option - packed array
 *    p_model  - price model code (say, 'ppp' = Price Per Person, etc.)
 *    p_floor  - basic price for the 'pppf' price model
 *    p_isCD   - default compulsory (if there are many of those) service
 *    p_isCC   - is compulsory (say coffee in the morning), always included in the bill 
 *    p_isDD   - can be "daily"
 */
locateAndInclude('bList_vm');

class bList_vm_socialEventRates extends bList_vm{

  var $debug = False;
  protected $p_models=array('ppp'    => 'per person',
			    'ppd'    => 'per day',
			    'pppd'   => 'per person/day',
			    'ppe'    => 'per event',
			    'pppf'   => 'per person + "floor fee"',
			    );
  
  protected $packed_items = array('p_model','p_floor','p_isCD','p_isCC','p_isDD');

  var $isMBE = array('p_isCD' => 1,
		     'p_isCC' => 1,
		     'p_isDD' => 1);

  var $list_quering = array('lm_key'  => 'description',
			    'lm_value'=> 'price',
			    'p_model' => 'price model',
			    'p_floor' => 'floor fee',
//		            'p_isCD'  => 'default?',
			    'p_isCC'  => 'compulsory (i.e. always served)?',
			    'p_isDD'  => 'not compulsory, but can be every day?',
			    );
  
  var $list_showing = array('lm_key'  => '',
			    '_price'  => '', // lm_value
			    'p_model' => '',
			    'p_isCC'  => 'compulsory?',
			    'p_isDD'  => 'daily?',
			    '_used'   => 'popularity',
			    );
  
  /*
   * The social events might be defined for the whole organization and/or for any particular event
   */
  function __construct($parent_ID){
    parent::__construct($parent_ID);
    if (False)    $this->title = (is_object($this->e)
			 ? 'Social event rates for '.x('"',b_fmt::strcut($this->e->name(),33,'...'))
			 : myOrg_name.' social event rates');
    $this->dbg($this->title,cnf_dev);

    // Get the default set if the class is empty    
    $this->setDefaults();

    // Where does it come from???
    $this->deleteMember(array('lm_key'  => '',
			      'lm_value'=> ''),cnf_dev);
  }

  /*
   * 
   */
  protected function getFormHeader(){
    return '';
  }

  /*
   *
   */  
  protected function getTabsHeader(){
    return $this->title;
  }
  
  /*
   *
   */
  protected function setPageHeader() {
    myPear::H1($header=$this->getTabsHeader(),'reset');
  }

  /*
   * An example set of the social events
   */
  function setDefaults(){
    if ($this->getMembers()) return;

    if ($this->parent_ID == myOrg_ID){
      //
      // Initialize default Social Events for the Organization
      //
      foreach (VM::$SE as $lm_key=>$a){
	if (!$this->getMembers(array('lm_key'=>$lm_key))){
	  $lm_option = array('p_model'=>$a['m'],
			     'p_floor'=>$a['f'],
			     'p_isDD' =>$a['e'],
			     'p_isCD' =>$a['d'],
			     'p_isCC' =>$a['c']);
	  foreach($lm_option as $key=>$v) if (empty($v)) unset($lm_option[$key]);
	  $this->updateMember(array('lm_option'=>serialize($lm_option)),
			      $this->addMember(array('lm_key'=>$lm_key,'lm_value'=>$a['p'])));
	}      
      }
    }else{
      //
      // Initialize  Social Events for the "conference",
      // start from the defaults for the Organization
      //
      foreach(bList::getListInstance(myOrg_ID,get_class($this))->getMembers() as $lm_id=>$r){
	if (!$this->getMembers(array('lm_key'=>$r['lm_key']))){
	  $this->addMember(array('lm_key'   =>$r['lm_key'],
				 'lm_value' =>$r['lm_value'],
				 'lm_option'=>$r['lm_option']));
	}
      }
    }
  }
  
  /*
   *
   */
  protected function message_deleteMember($args){
    $r = $this->getMember($args);
    myPear::MESSAGE("Delete social event \"$r[lm_key]\"");
  }

  /*
   * Get the list of all prices in the format usable by the "select menu"
   */
  function get_ser_names($name,$selected){
    $s = array();
    if (!empty($selected)) $c = $this->isCompulsory($selected);
    foreach ($this->getMembers() as $lm_id=>$r){
      if (!bForm::_checkBoxIsOn('p_isCC',$r)) $s[$lm_id] = $r['lm_key']; 
    }
    return $s;
  }

  /*
   *
   */
  function addMember($args=array()){
    // check for duplicated or incomplete records
    if (empty($args['lm_key'])){
      b_debug::print_r($args,'args');
      b_debug::traceBack("Missing social event name");
      myPear::ERROR("Missing social event name");
    }elseif($this->getMembers(array('lm_key'=>($n=$args['lm_key'])))){
      myPear::ERROR("Record '$n' already exists");
    }else{
      unset($args['lm_id']); // Just in case...
      return parent::addMember($args);
    }
  }

  /*
   *
   */
  protected function isMemberDeletable(){
    $reply = ($this->parent_ID == myOrg_ID
	      ? !self::_is_used($this->r['lm_id'])
	      : (count(array_keys($this->e->socialEvents()->getMembers(array('lm_key'=>$this->r['lm_id'])))) == 0));
    return $reply;
  }

  static function _is_used($lm_key){
    static $usage = array();
    if (@$usage[$lm_key] === Null){
      $q = myPear_db()->query("SELECT count(lm_id) FROM zzz_list_members ".
			      " LEFT JOIN zzz_lists ON l_id = lm_lid ".
			      " WHERE l_class='bList_vm_socialEvents' AND lm_key='$lm_key' GROUP BY lm_key"); 
      $usage[$lm_key] = 0;
      while($r = myPear_db()->next_record($q)) $usage[$lm_key] = $r['count(lm_id)'];
    }
    b_debug::xxx($usage[$lm_key].' ');
    return $usage[$lm_key];
  }

  protected function isMemberEditable(){
    $reply = $this->isWritable();
    if ((bForm::_checkBoxIsOn('p_isCC',$this->r) || 
	 bForm::_checkBoxIsOn('p_isCD',$this->r)) && !VM_manager_here) $reply = False;
    $this->dbg($reply);
    return $reply;
  }

  function isAddable(){
    $reply = $this->isWritable();
    $this->dbg($reply);
    return $reply;
  }
  
  function isWritable(){
    $reply = VM::hasRightTo('setup_SE',$this->parent_ID) || parent::isWritable();
    return $reply;
  }

  /*
   * Get the printable price for the event
   */
  function getPriceWithModel($lm_id){
    $p_model = $this->getMemberValue($lm_id,'p_model');
    $reply = sprintf("%s %s", 
		     b_fmt::money($this->getMemberValue($lm_id,'lm_value')),
		     $this->p_models[$p_model]);
    if ($p_model == 'pppf') $reply .= sprintf(" %s ",b_fmt::money($this->getMemberValue($lm_id,'p_floor')));
    $this->dbg($reply,cnf_dev);
    return $reply;
  }
  
  /*
   * Get estimation of a social event budget
   * @param - $N - number of persons
   */
  function get_ser_price($lm_id,$N=0){
    $m = $this->getMemberValue($lm_id,'p_model');
    switch($m){
    case 'pppd':
    case 'ppp':  $reply = $N * $this->getMemberValue($lm_id,'lm_value'); break;
    case 'ppd':
    case 'ppe':  $reply =      $this->getMemberValue($lm_id,'lm_value'); break;
    case 'pppf': $reply = $N * $this->getMemberValue($lm_id,'lm_value') + $this->getMemberValue($lm_id,'p_floor'); break;
    default:     $reply =  "unknown p_model '$m' ???"; 
    }
    $this->dbg($reply,!(int)$reply||$this->debug);
    return $reply;
  }

  function getCompulsoryServices($lm_id=0){
    $reply = array();
    if ($this->isWritable()){
      $args = (empty($lm_id) ? array() : array('lm_id' => $lm_id));
      foreach($this->getMembers($args) as $id=>$r){
	if (empty($lm_id) && !bForm::_checkBoxIsOn('p_isCC',$r)) continue;
	$reply[] = array($r['lm_id'],$r['lm_key'],$r['lm_value'],$r['p_model']);
      }
      $this->dbg($reply,cnf_dev);
    }
    return $reply;
  }

  private function reset_cache(){
    $this->cache_bList_getMembers = array();
  }

  /*
   * Strip out the checkbox "no"
   */
  protected function unpack($buffer=array(), $updates=array()){
    locateAndInclude('b_checkBox');

    // Remove "checkbox off"
    $buffer_clean = parent::unpack($buffer,$updates);
    foreach(preg_grep('/^p_is/',$this->packed_items) as $k){
      if (bForm::_checkBoxIsOn($k,$buffer_clean)) $buffer_clean[$k] = b_checkBox::$ON[0];
      else                                  unset($buffer_clean[$k]);
    }

    // Repack the packables
    $lm_option = array();
    foreach(array_intersect(array_keys($buffer_clean),$this->packed_items) as $p) $lm_option[$p] = $buffer_clean[$p];
    $buffer_clean[$this->serialized] = serialize($lm_option);

    return $buffer_clean;
  }

  /*
   * Check is the given "social item" compulsory or not
   */
  function isCompulsory($lm_id){
    return $this->is($lm_id,'p_isCC');
  }

  /*
   * Check may the given "social item" be run every day
   */
  function isDailyPossible($lm_id){
    return $this->is($lm_id,'p_isDD');
  }

  /*
   * Helper to isCompulsory / isDailyPossible
   */
  private function is($lm_id,$what='p_isCC'){
    if (empty($lm_id)) $lm_id = $this->getValue('lm_id');
    if (!(int)$lm_id)  return False;
    $reply = bForm::_checkBoxIsOn($what,$this->getMember($lm_id));
    return $reply;
  }

  private static function __is($r,$what='p_isCC'){
    $reply = !empty($r[$what]);
    return $reply;
  }


  /*
   *
   */
  function getFormAdd_OK(){
    $lm_id = $this->getValue('lm_id');
    if ($this->getMemberValue($lm_id,'p_model') != 'pppf') unset($this->list_quering['p_floor']);
    $v = @$_POST['lm_value'];
    $reply = parent::getFormAdd_OK() && ctype_digit($v);
    if (!empty($v) && !ctype_digit($v)){
      $errors[] = x('li',"The price can't be '$v', it must contain digits only");
      $reply = False;
    }
    if (bForm::_checkBoxIsOn('p_isCC',$_POST) && bForm::_checkBoxIsOn('p_isDD',$_POST)){
      $errors[] = x('li',str_replace('?','',"You can't select both "
				     .x("'",$this->list_quering['p_isCC']).' & '
				     .x("'",$this->list_quering['p_isDD'])));
      $reply = False;
    }
    if (!empty($errors)) myPear::ERROR(x('ul',join("\n",$errors)));
    $this->dbg($reply);
    return $reply;
  }

  protected function getFormAdd(){
    $this->bForm($this->list_quering,$inversed=True);
    if (empty($this->f->formDB['p_model'])) $this->f->formDB['p_model'] = 'ppp';
    $this->f->isMONEY['lm_value']= True;
    $this->f->isMONEY['p_floor'] = True;
    $lm_id = $this->getValue('lm_id');

    if ($this->isCompulsory($lm_id))  $this->f->isRO['p_model'] = True;

    foreach(array_keys($this->list_quering) as $item){
      switch($item){
      case 'p_isCD':
	if (!$this->isCompulsory($lm_id)){
	  $this->f->textHidden($item,0);
	  break;
	}
      case 'p_isCC':
      case 'p_isDD':
	if ($this->isCompulsory($lm_id) && ($item==='p_isDD')){
	  $this->f->textHidden($item,0);
	}else{
	  $this->f->checkBox($item);
	}
	break;
      case 'p_model': 
	$this->f->selectMenu($item,array_merge($this->p_models,array('default'=>'ppp')));
	break;
      default:
        $this->f->textField($item);
      }
    }
    $this->f->textHidden('lm_id',$this->member_ID);
    $this->f->closeForm();
  }
  
  /*
   *
   */
  protected function draw_buttons(){
    $reply = array();
    if ($this->isWritable() && $this->isAddable()) {
      $reply[] = b_btn::submit('Propose new social event',
                               $this->get_action('add'),
                               $confirm=True);
    }
    //    if ($this->isWritable())
    if (is_object($this->e)) $reply[] = b_btn::submit('back to the budget estimation',b_url::same("?tab_force=tab_vm_ogz_budget"));
    return array_merge($reply,$this->extra_buttons);
  }


  /*
   *
   */
  protected function header(){
   $header = $this->list_showing;
   if ($this->parent_ID != myOrg_ID) unset($header['_used']);
   return $header;
  }

  /*
   *
   */
  protected function format(){
    if (is_object($this->e))  return $this->format_radio();
    else                      return $this->format_usual();
  }
  
  /*
   *
   */
  protected function format_radio(){
    myPear::INFO_once(array(x('p','To select an event from the list click on "[ select ]"'),
			    x('p','Before the selection you might want to '.
			      '<br/>&nbsp;- update the rates by clicking '.bIcons()->get('i-edit').
			      '<br/>&nbsp;- remove an uninteresting event by clicking '.bIcons()->get('i-drop')),
			    x('p','You can also propose a new social event, see button '.x('em',"'Propose new social event'").' below.')),
		      'Tips for selecting social events');

    $skip = (empty($this->r['lm_value']) && empty($this->r['lm_key']));
    
    if ($skip){
      $this->r = array();
    }else{
      //      $GLOBALS['b_fmt::money'] = CONST_currency;
      $this->t->css['lm_value']['align'] = 'align_right';
      $this->r['_price']  = x('i',(empty($this->r['lm_value']) ? '?unknown' : b_fmt::money($this->r['lm_value'])));
      $this->r['p_model'] = x('i',$this->p_models[$this->r['p_model']].' '.b_fmt::money(@$this->r['p_floor']));
      foreach(array('p_isCC','p_isDD','p_isCD') as $item){
	$this->r[$item] = (bForm::_checkBoxIsOn($item,$this->r) ? bIcons()->get('i-ok') : '');
      }
      $action = array_merge($this->e->socialEvents()->get_action_array('add'),
			    array('_POST__post_bList_class'  => 'bList_vm_socialEvents',
				  '_POST__post_bList_classID'=> $this->e->socialEvents()->ID,
				  '_POST_lm_key' => $this->r['lm_id'],
				  '_POST_l_id'   => $this->e->socialEvents()->ID,
				  'add_continue_once'  => 'yes',
				  ));
      $url = b_url::same($action);
      array_unshift($this->t->extraTD,
		    (empty($this->r['lm_value']) || empty($this->r['lm_key']) || 
		     bForm::_checkBoxIsOn('p_isCC',$this->r) || 
		     bForm::_checkBoxIsOn('p_isCD',$this->r)
		     ? ''
		     : b_btn::link('select',$url)));
    }
  }

  protected function format_usual(){
    //    $GLOBALS['b_fmt::money'] = CONST_currency;
    $this->t->css['lm_value']['align'] = 'align_right';
    $this->r['_price']  = x('i',(empty($this->r['lm_value']) ? b_fmt::redText('?unknown') : b_fmt::money($this->r['lm_value'])));
    $this->r['_used']   = ($n=self::_is_used($this->r['lm_id'])) ? 'Used '.($n>1?"$n times":"once") : "";

    $this->r['p_model'] = x('i',$this->p_models[$this->r['p_model']].' '.b_fmt::money(@$this->r['p_floor']));
    foreach(array('p_isCC','p_isDD','p_isCD') as $item){
      $this->r[$item] = (bForm::_checkBoxIsOn($item,$this->r) ? bIcons()->get('i-ok') : '');
    }
  }
}
