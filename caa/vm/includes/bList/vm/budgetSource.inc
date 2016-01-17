<?php
/*
 * Customized for Nordita
 */

locateAndInclude('bList_vm');

define('VM_budgetL','L'); // "long event" code
define('VM_budgetS','S'); // "short event" code
define('VM_no_projectCode', 'not initialized');

class bList_vm_budgetSource extends bList_vm{

  // sick! redefined in function __construct
  var $list_quering = array('lm_key'  =>'budget source',
                            'lm_value'=>'amount');
  
  protected $isReady = True;
  protected $orderBy ='lm_key';
  protected $agenda  = Null;
  public    $total = 0;     // total budget
  private   $defaultEventBudget = 0;

  function __construct($e_id){

    $this->list_quering = array('lm_key'  =>'budget source',
				'lm_value'=>'amount ('.trim(CONST_currency).')');
    /*
    // First hook the event
    if (empty($e_id)) $e_id = VM::$e->ID;
    if ($e_id instanceof bForm_vm_Event) $this->e = $e_id;
    else      $this->e = myPear::getInstance('bForm_vm_Event',$e_id,'strict');
    */
    parent::__construct($e_id);
    
    // Do it explicitly to start with,
    // to be changed later
    $this->init();
  }

  /*
   *
   */
  protected function message_addMember_name($r){
    return $r['lm_key'];
  }

  /*
   * Do not a message when the empty bList_vm_budgetSource is created, 
   * do it when the budget is initialised
   */
  protected function message_createList(){}

  /*
   *
   */
  private $project_code = VM_no_projectCode;
  function init(){
    if ($this->project_code === VM_no_projectCode){
      $this->project_code = Null;

      // Extract the budget project code from the projects list,
      // match by the event code
      if (is_object($this->e) && ($lm_id = VM_projects()->getProjectID($this->e->ID))){
	$pr = VM_projects()->getMember($lm_id);
	$this->project_code = $pr['lm_value'];

	$defaultEventBudget = $this->e->get_defaultEventBudget();
	$budget_message = ($defaultEventBudget > 0 
			   ? sprintf("%s project %s %s (%s%s per week, %s weeks)",
				     myOrg_name,
				     $this->project_code, 
				     b_fmt::money($defaultEventBudget),
				     VM_conferenceBudget_week,
				     CONST_currency,
				     $this->e->nWeeks())
			   : sprintf("%s project %s Budget to be defined",
				     myOrg_name,
				     $this->project_code)); 
	
	// Check that the already existing budget source records comply with the CURRENT rules,
	// remove the default sourceif it is not complient
	foreach($this->getMembers() as $id=>$r){
	  if (!$this->is_budget_default($r)) continue;
	  $ok = True;
	  switch($this->e->get_defaultEventBudget() > 1){
	  case True:
	    // Wrong, if the "long event" has the 'S' code OR the message is not correct
	    if ($r['lm_status'] != VM_budgetL)      $ok = False;
	    if ($r['lm_key']    != $budget_message) $ok = False;
	    break;
	    
	  case False:
	    // Wrong, if the "short event" has the 'L' code OR the message is not correct
	    if ($r['lm_status'] != VM_budgetS) $ok = False;
	    if ($r['lm_key']    != $budget_message && empty($r['lm_value'])) $ok = False;
	  }
	  if (!$ok)	$this->deleteMember($r['lm_id']);
	}
	
	// (Re)create the default budget source 
	// if it is not yet done OR it was removed by the upper checkup
	if (!$this->get_budgetSource($default_only=True,$verbose=False)){
	  myPear::MESSAGE('Creating budget source record for "'.$this->e->name().'"');
	  $this->addMember(array('lm_key'   => $budget_message,
				 'lm_value' => $defaultEventBudget,
				 'lm_status'=>($defaultEventBudget > 0 ? VM_budgetL : VM_budgetS)));
	}
      }
    }
  }

  /*
   * Return the project code as assigned by the the finance department (sometimes called "budget code")
   */
  function get_project_code(){
    $this->init();
    if (empty($this->project_code)){
      if (VM_administrator_here){
	$name = (is_object($this->e) ? x('"',$this->e->name()) : 'The event');
	myPear::WARNING(x('em',"$name does not have an associated ".myOrg_name." project code, reimbursement not possible"));
      }
    } 
    return $this->project_code;
  }

  function isWritable(){
    return VM_access()->mayDoAccounting() || parent::isWritable();
  }
  
  protected function isMemberEditable(){
    $reply = ($this->r['lm_status'] != VM_budgetL) && parent::isMemberEditable();
    $this->dbg($reply);
    return $reply;
  }

  protected function isMemberDeletable(){
    return $this->isMemberEditable();
  }

  private function is_budget_default($r){
    return in_array($r['lm_status'],array(VM_budgetL,VM_budgetS));
  }

  /*
   * Get the budgetSource list
   */
  function get_budgetSource($default_only=False,$verbose=True){
    $reply = array();
    $this->total = 0;
    foreach ($this->getMembers() as $id=>$r){ 
      if (!$default_only || $this->is_budget_default($r)){
	$reply[$r['lm_id']] = $r['lm_key'];
	$this->total       += $r['lm_value'];
      }
    }
    return $reply;
  }

  protected function getFormAdd(){
    $this->bForm(array($this->memberTitle()=>'lm_key',
		       'amount ('.trim(CONST_currency).')' =>'lm_value'));
    $this->f->textField('lm_key');
    $this->f->textField('lm_value');
    $this->f->closeForm();
  }

  function getFormAdd_OK(){
    $reply = (parent::getFormAdd_OK() && 
	      (int)$_POST['lm_value'] &&
	      ($_POST['lm_value'] > 0));
    $this->dbg($reply);
    return $reply;
  }

  protected function header(){
    return array('lm_key'   =>'',
		 'lm_value' =>trim(CONST_currency),
		 //'lm_status'=>'',
		 //'lm_option'=>'',
		 );
  }

  function is_projectActive($rec){
    return (($rec['lm_option'] === 'no') ? '' : 'checked');
  }
  
  function set_projectActivity($id,$value=True){
    myPear_db()->query("UPDATE zzz_list_members SET lm_option = '".($value?'yes':'no')."' WHERE lm_id = $id");
  }

  protected function format(){
    $this->t->class['lm_key'] = 'b_nowrap';
    $this->t->showLineCounter = False;
    $this->r['lm_value'] = x('strong',b_fmt::money($this->r['lm_value']));
    $this->r['lm_status'] = x('em',$this->r['lm_status']);
  }

  function draw_summary() {
    if (count($this->get_budgetSource() > 1)){
      $this->t->prt(array('lm_value'=>'<hr/>'));
      $this->t->prt(array('lm_value'=>x('strong',b_fmt::escape(b_fmt::money($this->total)))));
    }
  }
}
