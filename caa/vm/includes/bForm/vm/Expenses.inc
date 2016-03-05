<?php
/*                                  YB 2009 - 2012
 * This class is used both for 
 * - estimating the expenses 
 * - storing the amount paid
 *
 * There 2 sorts of estimation -
 * 1. The upper limit which the visit organizer is ready to pay
 * 2. The estimation based on the average prices for the region
 * 
 * The first estimation is more precise, hence it has precedence 
 */

locateAndInclude('bForm_vm');
locateAndInclude('bForm_vm_Visit');

class bForm_vm_Expenses extends bForm_vm{

  var $debug = False;

  static $pay_items = array('exp_living'  => array('d'=>'Accommodation', 
						   'p'=>VM_V_payAccommodation, 
						   'est'=>array('always','no_est')), 
			    'exp_travel'  => array('d'=>'Travel expenses', 
						   'p'=>VM_V_payTrip),
			    'exp_perdiem' => array('d'=>VM_wordPerdiem, 
						   'p'=>VM_V_payPerdiem),
			    'exp_other'   => array('d'=>'Other expenses', 
						   'p'=>VM_V_payOther,
						   ), 
			    '_exp_totalExpenses'=>array('d'=>'Total expenses',
							'p'=>0));
  
  /* 
   * Update the estimates (say, the rates are updated)
   */
  static public function _updateEstimates($parentID){
    if (cnf_dev) myPear::DEBUG(__METHOD__."($parentID)");
    foreach (($pay_items=array_diff(array_keys(self::$pay_items),array('exp_other'),array('_exp_totalExpenses'))) as $item) $set[] = "${item}_est = 0";
    if ($parentID == myOrg_ID){
      // Get all guest visits which are not yet reimbursed
      $v_start = b_time::txt2unix(date('2013-01-01')); 
      $where = array("(v_eid = 0 OR v_eid IS NULL)",
		     "v_start > $v_start");
      foreach($pay_items as $item) $where[] = "$item = 0"; 
      $q = myPear_db()->query("SELECT v_id FROM abs_visits LEFT JOIN abs_expenses ON exp_vid = v_id WHERE ".join(' AND ',$where));
    }else{
      // Get all visits for the event
      $q = myPear_db()->query("SELECT v_id FROM abs_visits WHERE v_eid = $parentID");
    }
    while ($r = myPear_db()->next_record($q)) { 
      // Reset all expenses estimates for the visit
      myPear_db()->query("UPDATE abs_expenses SET ".join(',',$set)." WHERE exp_vid = $r[v_id]");
      // Recreate estimates for the each visit
      $qq = myPear_db()->query("SELECT exp_id FROM abs_expenses WHERE exp_vid = $r[v_id]");
      while($rr = myPear_db()->next_record($qq)){
	$exp = new bForm_vm_Expenses($rr['exp_id']);
	$exp->createEstimates(); 
      }
    }
  }
  
  function __construct($id, $mode='RO') {
    bTiming()->cpu(__FUNCTION__);
    parent::__construct($id, $mode);
    if (@$_GET['blanks_once'] === 'yes') $this->getBlanketts('message');
    bTiming()->cpu();
  }

  function __clean_cache(){
    myPear::getInstance(get_class($this),$this->ID,'clean');
  }
  
  /*
   *
   */
  function createEstimates(){
    // Add currency to value
    //    $GLOBALS['b_fmt::money'] = $this->getValue('exp_currency');
    $this->hookVisit();

    $updates = array();
    if (is_object($this->visit)){
      $project_defined = $this->visit->getValue('v_projectid',True);
      foreach(array_keys(self::$pay_items) as $item){
	
	// silently accept the fact of the payment if it is already done
	$value = $this->getValue($item);
	if ($value > 0)  $this->visit->setPolicy(self::$pay_items[$item]['p'],True);
	
	$item_est = "${item}_est";
	$value_def = $this->defaultPay($item);
	$value_est = $this->getValue($item_est);
	$est = Null;
	switch($item){
	case 'ZZZ_exp_other':
	  $est = 0;
	  break;
	default:
	  // Cancel the estimate if the policy does not allow payment
	  $est = $value_est;
	  if (empty($est)) $est = $value_def;
	  if (!$this->visit->getPolicy(self::$pay_items[$item]['p']))  $est = 0;
	}
	if ($est !== Null) $updates[$item_est] = $est;
      }
      if (!empty($updates)){
	$updates['exp_vid'] = $this->visit->ID;
	$this->dbg('updates: '.b_fmt::joinX(',',$updates));
	$this->updateDB($updates);
      }
    }
  }
  
  /*
   * Get the default pay_values for the visit. 
   * It is defined by the policy settings
   * (to pay / not to pay scholarship), the visit length, etc.
   * 
   */ 
  private function defaultPay($pay,$returnFormula=False){
    list($value,$formula) = array(0,'');

    $this->hookVisit();
    $n = $this->visit->nDays(True);
    list($perdiem,$trip) = $this->visit->reimbursementRates()->get_rates($this->visit->formDB);
    
    switch($pay){
    case 'exp_travel':
      if ($this->visit->getPolicy(VM_V_payTrip)){
	$value   = (int)$trip;
	if($tripCostFromResidentshipCountry = $this->visit->reimbursementRates()->tripCostFromResidentshipCountry){
	  $formula = "$tripCostFromResidentshipCountry[country_name]-".b_fmt::money($tripCostFromResidentshipCountry['pay']);
	}else{
	  myPear::WARNING_T('empty  bList_vm_reimbursementRates::$tripCostFromResidentshipCountry');
	}
      }
      break;
      
    case 'exp_perdiem':
      if ($this->visit->getPolicy(VM_V_payPerdiem)){
	$value   = $perdiem * ($n= $this->visit->nDays(True));
	$formula = ($value>0
		    ? "$perdiem*$n=".b_fmt::money($perdiem*$n)
		    : ''); // "No per-diem"
      }
      break;
      
    case 'exp_living':
      $value = ($returnFormula
		? 999
		: $this->living($showFormula=False,$showDefaultOnly=True));
      if ($this->visit->getPolicy(VM_V_payAccommodation,($value>0))){
	$formula = $this->living($showFormula=True, $showDefaultOnly=False);
      }
      break;
      
    case 'exp_other':
    case '_exp_totalExpenses':
    }
    
    if ($returnFormula) $reply = $formula;
    else                $reply = $value;
    $this->dbg($reply);
    return $reply;
  }

  /*
   * MBE is always "True" here
   */
  public function getValue($name, $asIs=True, $d_MBE=True){
    return parent::getValue($name,True,True);
  }

  /*
   * The class might be invoked both from the parent class and by itself in the editing mode.
   */
  private $visit = Null;
  private $rates_ID = '???';
  function hookVisit(){
    if (empty($this->visit) && !empty($this->parentForm)){
      $this->visit = $this->parentForm;
    }
    if (!($this->visit instanceof bForm_vm_Visit)){
      if (!($this->visit = $this->parentForm)) $this->visit = myPear::getInstance('bForm_vm_Visit',$this->getValue('exp_vid'));
      if (! $this->visit) b_debug::internalError("no parent visit hooked for the bForm_vm_Expenses",$this);
      if (!(int)$this->ID){    
	$q = myPear_db()->query("SELECT * FROM $this->TABLE WHERE exp_vid = ".$this->visit->ID);
	while ($r = myPear_db()->next_record($q)) { 
	  $this->formDB = $r;
	  $this->ID = $r['exp_id'];
	}
      }
      $this->visit->setExp($this);
    }
    $this->rates_ID = (($v_eid=$this->visit->getValue('v_eid')) ? $v_eid : myOrg_ID);
    $this->dbg($this->visit->__toString()); // sic! this calls hookDependancies - i.e. do not kill this line
    $this->visit->mergeDefinitions($this);
  }
  
  /*
   *
   */
  private function get_estimation($name,$estimate,$onlyEstimate,$formula,$showFormula){
    if     ($showFormula)       $value =  "<em>$formula</em>";
    elseif ($onlyEstimate){     $value = $estimate;            $this->colorCode[$name] = VM_color_budgetEstimate; }
    else   	                $value = $this->getValue($name);
    if (!(int)$this->ID) $this->colorCode[$name] = VM_color_budgetEstimate;
    
    $e = ((int)$estimate && ($estimate != $value) ? " &lt;- $estimate" : '');
    if ((int)$value) $this->dbg("$formula = $value $e");
    return $value;      
  }

  
  /**
   * Get perdiem value for the visit.
   * If the event policy is not to pay the perdiem, return zero
   */
  function scholarshipP($showFormula=False,$showDefaultOnly=False){
    $this->hookVisit();
    
    // silently accept the fact ...
    if ($payed=@$this->formDB['exp_perdiem']){
      $this->visit->setPolicy(VM_V_payPerdiem,True);
      return ($showFormula ? ' - payed' : $payed);
    }
    
    $payed = @$this->getValue('exp_perdiem');
    if ($this->visit->getPolicy(VM_V_payPerdiem)){
      list($perdiem,$trip) = $this->visit->reimbursementRates()->get_rates($this->visit->formDB);
      $perdiem_esmimation = $perdiem * ($n = $this->visit->nDays(True));
      if ($showDefaultOnly){
	return $perdiem_esmimation;
      }else{
	if ($payed)                          $formula = ' - payed';
	else                                 $formula = "($perdiem*$n=".$perdiem_esmimation.")";
	return $this->get_estimation('exp_perdiem',$perdiem_esmimation,$showDefaultOnly,$formula,$showFormula);
      }
    }
  }
  
  /**
   * Get travel expenses for the visit
   */
  function scholarshipT($showFormula=False,$showDefaultOnly=False){
    $this->hookVisit();
    
    // Was the reimbursement payed already?
    $payed = $this->getValue('exp_travel');

    if ($this->visit->getPolicy(VM_V_payTrip)){
      list($perdiem,$trip) = $this->visit->reimbursementRates()->get_rates($this->visit->formDB);
      if ($showDefaultOnly){
	return $trip;
      }else{
	$z = $this->visit->reimbursementRates()->tripCostFromResidentshipCountry;
	if ($payed)    $formula = "payed";
	elseif($z)     $formula = "$z[country_name]-".b_fmt::money($z['pay']);
	else           $formula = "???";
	return $this->get_estimation('exp_travel',@$z['pay'],$showDefaultOnly,$formula,$showFormula);
      }
    }
  }
  

  /**
   * Get living expenses for the visit
   */
  function living_ABC($accommodation_option,$method,$showFormula=False,$showDefaultOnly=True){
    $reply = $this->living($showFormula,$showDefaultOnly,$accommodation_option);

    if ((int)$this->ID)  $this->colorCode[$method] = $this->colorCode['living'];
    else                 $this->colorCode[$method] = VM_color_budgetEstimate;
    return $reply;
  }
  
  /*
   * Get the accommodation cost
   *
   * @param (bool) $showFormula 
   * @param (bool) $showDefaultOnly
   * @param (bool) $seeOnly - (optional) If set, the leases other then '$seeOnly' are ignored.
   *               Might be "lease_id" or "hut_code"
   * @return total cost for the selected accommodation option
   */
  function living($showFormula=False,$showDefaultOnly=False,$seeOnly=False){
    locateAndInclude('bForm_vm_Apartment');
    $this->hookVisit();
    $formula = array();
      $expenses = 0;
      foreach($this->visit->get_allLeases() as $l){
	if (!empty($seeOnly) && ($l->hut_code() != $seeOnly) && ($l->ID != $seeOnly)) continue;
	list($price,$booking_exists) = $l->accommodationCost();
	$formula[] = bForm_vm_Apartment::$receiptFormula;
	$expenses += $price;
	if (!$booking_exists) $this->colorCode['living'] = VM_color_budgetEstimate;
      }
      // if (!$this->getValue('exp_living'))  $_POST['exp_living'] = $expenses;
      $formulaTidy = str_replace(') (',' + ',join(' ',$formula));

      $debit_credit = @VM::$description[$this->visit->getValue('v_type')]['f'];
      if ($debit_credit === 'credit'){
	$formulaTidy = "CREDIT $formulaTidy";
	$expenses = -abs($expenses);
	$this->colorCode['living'] = VM_color_budgetCredit;
      }
      if (empty($this->colorCode['living'])) $this->colorCode['living'] = '';

      $reply = ($showFormula ? $formulaTidy : $expenses);
      $this->dbg($reply);
      return $reply;
  }

  /*
   * Define variables and types
   */
  private $dejaVu_defineVariables = 0;
  function defineVariables() {
    if ($this->dejaVu_defineVariables++) return;

    // Build the variable description
    $vars = array('exp_currency'   => 'Currency',
		  'exp_owner_avid' => 'Updated by',
		  'exp_closed'     => 'Is the budget final?');
    foreach(self::$pay_items as $column=>$d) $vars[$column] = $d['d'];
    $this->defineVariable($vars,True);
    
    $this->defineTypes(array('isRO' => array('exp_currency'  => True,
					     'exp_owner_avid'=> True,
					     'exp_closed'    => True // !VM_access()->mayDoAccounting()
					     ),
			     'isMBE'=> array_merge(array_keys(self::$pay_items),
						   array('exp_closed',
							 'exp_owner_avid',
							 'exp_currency'))));
  }
  
  /*
   * Define "budget estimate" block description.
   * Needed during the interactive dialog only
   */
  private function defineVariables_est(){

    $this->hookVisit();
    if ($this->visit->getValue('v_type',1) == VISIT_TYPE_RENT){
      self::$pay_items['exp_living']['d'] = 'To be invoiced ';
    }
    
    foreach (self::$pay_items as $i=>$d){
      $descr     = array($d['d'].' '.x('em','- paid'));
      $descr_est = array($d['d'].' '.x('em','- estimation'));
      $formula   = $this->defaultPay($i,$returnFormula=True);
			 
      $isRO = array($i.'_est');
      switch($i){
      case 'exp_travel':
      case 'exp_perdiem':
	if (!empty($formula)) $descr_est[] = x('em',$formula);
	break;
      case 'exp_living':
	if (strlen($formula) > 25) $formula = str_replace(') (',')<br/>(',$formula);
	$descr = array($d['d'].' '.x('em',$formula));
	break;
      case '_exp_totalExpenses':
	$isRO[] = $i;
	$this->colorCode[$i]        = 'boldText';
	$this->colorCode["${i}_est"]= 'boldText';
	break;
      default:
      }
      $this->defineVariable(array($i       =>join('<br/>',$descr),
				  $i.'_est'=>join('<br/>',$descr_est)),True);
      $this->defineTypes(array('isVITAL_ifADMIN' =>array($i),
			       'isRO'   => $isRO,
			       'isMBE'  => array($i,
						 $i.'_est'),
			       'isMONEY'=> array($i,
						 $i.'_est'),			       
			       ));
    }
  }
  
  
  /*
   * Get the pre-filled PDF blanks for the payments and show the download button
   */ 
  private $blanks = array();
  function getBlanketts($what='message'){
    $reply = array();

    switch($what){
    case 'message':
      if (!cnf_inside_fb){
	if (empty($this->blanks)){
	  $this->hookVisit();
	  $tr = $this->visit->getPolicy(VM_V_payTrip);
	  $pd = $this->visit->getPolicy(VM_V_payPerdiem);
	  if ($this->visit->getValue('v_projectid') && ((bool)$tr || (bool)$pd)){
	    foreach(VM::$forms2admin as $pdf_tpl){
	      $this->blanks[$pdf_tpl] = $this->pdfReceipt($pdf_tpl,
							  myPear_tmp . "/${pdf_tpl}_" . $this->visit->ID . '.pdf',
							  $verbose=True);
	    }
	  }
	}
    
	foreach($this->blanks as $tpl=>$file){
	  if (is_file($file = str_replace('//','/',$file)))
	    myPear::MESSAGE("Click <a href='$file'>".bIcons()->get('i-m_pdf')."</a> to download '$tpl' blank for ".$this->visit->av->name());
	}	
      }	
      break;
      
    case 'button':
      if (($this->MODE == 'RW' && VM_reimberser_here) || // The accounter need blanks
	  ($this->visit->av->ID == bAuth::$av->ID)){       // The visitor needs blanks
	$reply['print payment form'] = b_url::same("?blanks_once=yes&".b_fmt::joinX('&',$_GET));
      }
      break;
	
    case 'blanks':
      $reply = $this->blanks;
      break;
    }
    return $reply;
  }

  /*
   *
   */
  function getForm(){
    
    if (!is_object(bAuth::$av)) return;

    $this->hookVisit();
    if (!$this->visit->isApproved() && !$this->visit->isWritable()) return;

    // Define variables which are needed for the interactive session only
    $this->defineVariables_est();
    
    $this->getFormOpen();    
    myPear::H1($this->name());
    
    $this->createEstimates();

    $dialog = array();
    if (VM_external_registrant_here || VM_visitor_here){
      //
      // Visitor or registrant logged in.
      // Let him see his reimbursement
      $dialog = array('exp_travel'=>'textField');
      
    }elseif($this->visit->getValue('v_type',1) == VISIT_TYPE_RENT){
      
      //
      // The visit is just an apartment renting
      $dialog = array('exp_living' =>array('textField'),
		      'exp_closed' =>array('radioButton',array('yes','no')));
      
    }else{

      //
      // Normal visit to be reimbursed
      //
      foreach(array_keys(self::$pay_items) as $i){
	if (empty(self::$pay_items[$i]['est'])) self::$pay_items[$i]['est'] = array();
	if (!$this->visit->getPolicy(self::$pay_items[$i]['p']) && !in_array('always',self::$pay_items[$i]['est'])) continue;
	
	$i_est = $i . '_est';
	$v     = $this->getValue($i);     
	$v_est = $this->getValue($i_est);
	
	switch($i){
	case 'ZZZ_exp_other':
	  if (!$this->getValue($i) && isset($dialog[$i])) unset($dialog[$i]);
          break;
	  
	case 'exp_living':
	  if (empty($v)) $this->formDB[$i] = $v = $this->defaultPay($i);
	  $v_est = $v;
          break;
	  
	default:
	  break;
	}

	if (empty($this->formDB['_exp_totalExpenses']))     $this->formDB['_exp_totalExpenses']     = 0;
	if (empty($this->formDB['_exp_totalExpenses_est'])) $this->formDB['_exp_totalExpenses_est'] = 0;
	$this->formDB['_exp_totalExpenses']     += $v;
	$this->formDB['_exp_totalExpenses_est'] += $v_est;

	$this->isRO[$i]     = !VM_access()->mayDoAccounting();
	$this->isRO[$i_est] =  VM_access()->mayDoAccounting();
	
	if (empty(self::$pay_items[$i]['est'])) self::$pay_items[$i]['est'] = array();
	if (!in_array('no_est',self::$pay_items[$i]['est'])) $dialog[$i_est] = array('textField');
	if ($this->MODE == 'RW' || !empty($v))               $dialog[$i]     = array('textField');
      }
      
      $this->isRO['exp_living'] = ($this->visit->getValue('v_type',1) != VISIT_TYPE_RENT);
      foreach(array('_exp_totalExpenses','_exp_totalExpenses_est') as $item){
	if ($this->getValue($item) == $this->getValue('exp_living')) unset($dialog[$item]);
	$this->isRO[$item] = True;
      }
    }

    if (!empty($dialog)){
      // Currency
      //if ($this->MODE == 'RW') $dialog['exp_currency']=array('selectMenu',array('SEK'=>'SEK','DKK'=>'DKK','USD'=>'USD','default'=>'SEK'));
      
      // Who did the updates?
      if (!$this->getValue('exp_owner_avid'))  $this->formDB['exp_owner_avid'] = bAuth::$av->ID;
      $dialog['exp_owner_avid'] = array('textHidden');
    }
    
    if ($this->getBlanketts('button'))  $this->getBlanketts('message');
    $this->textHidden('exp_vid',$this->visit->ID);

    // Show the dialog
    $this->formBlock(VM_visit_expenses, 
		     ($this->visit->getValue('v_type',1) == VISIT_TYPE_RENT ? 'Invoice ' : 'Expenses reimbursement'),
		     $dialog,
		     array('d'=> 'update budget',
			   //'i'=> ($this->exp_arePayed()?'i-lock':'i-dollar'),
			   'i'=> 'i-dollar',
			   'l'=> b_url::same("?id=$this->ID&mode=RW&form=vm_Expenses&exp_vid=".$this->visit->ID)));
    $this->buttons_external[] = b_btn::submit('back to the visit', b_url::same(array('id'  => $this->visit->ID,
										     'form'=> 'vm_Visit',
										     )));
    $this->closeForm(); 
  }
  
  

    /*
     *
     */
  function formOKspecific(){
    foreach (array("exp_travel",
		   "exp_living",
		   "exp_other",
		   "exp_perdiem") as $item) 
      if (($v=$this->getValue($item,1)) && !(int)$v) $this->errorsInTheForm[$item] = "'".$this->getDescr($item)."' must be a number";
  }
  
  protected function _recordUpdated() {
    if ($this->myPost_bForm() && $this->getValue('exp_closed')==='yes') myPear_logs()->add('expenses',$this->ID);
  }
  
  protected function _recordCreated() {
    if ($this->myPost_bForm())    myPear::MESSAGE("Create \"Expenses\" record");
    $this->_recordUpdated();
  }
  
  function name(){
    $this->hookVisit();
    return $this->visit->name();
  }

  function isVisible(){
    return True; // the visibility is checked in the visit class
  }

  /*
   *
   */
  protected function formBlock_isVisible($block_ID,$fields=array()){
    if ($reply = VM_administrator_here){
      $reply = False;
      foreach(array_keys(self::$pay_items) as $i){
        if ($this->visit->getPolicy(self::$pay_items[$i]['p']) || @self::$pay_items[$i]['est']==='always'){
	  $reply = True;
	  break;
	}
      }
    }
    $this->dbg($reply,$this->debug||cnf_show_v);
    return $reply;
  }

  /*
   *
   */
  function isReadable(){
    $reply=($this->isOwner() || VM_observer_here);
    return $reply;
  }

  public function unlock(){
    if ((int)$this->ID){
      $this->updateDB(array('exp_closed'=>'no'));
      myPear_logs()->add('expenses_unlock',$this->ID);
    }
  }

  public static function _exp_arePayed($formDB){
    $reply = (@$formDB['exp_closed'] === 'yes') || 
      ((@$formDB['exp_travel'] + @$formDB['exp_perdiem'] + @$formDB['exp_other']) > 0);
    //    if (cnf_dev) myPear::DEBUG(@$formDB['id'].__FUNCTION__.'() '.x('em',var_export($reply,True)));
    return $reply;
  }
  
  function exp_arePayed($lock=Null){
    $this->hookVisit();
    $reply = self::_exp_arePayed(array('id'        => $this->visit->__toString().'->',
				       'exp_closed'=> $this->getValue('exp_closed'),
				       'exp_travel'=> $this->getValue('exp_travel'),
				       'exp_perdiem'=>$this->getValue('exp_perdiem'),
				       'exp_other'=>  $this->getValue('exp_other')));
    $this->dbg($reply);
    return $reply;
  }
  
  /*
   *
   */
  private $isWritable = Null;
  function isWritable(){
    bTiming()->cpu(__FUNCTION__);
    $this->hookVisit();
    $this->visit->hookEvent();
    if ($this->isWritable === Null) $this->isWritable = (bool)b_cnf::check_conditions(array('default' => $this->visit->isWritable()),
										      $this,
										      cnf_dev);
    bTiming()->cpu();
    return $this->isWritable;
  }
  
  function isVisitor(){
    $this->hookVisit();
    return !VM_manager_here && bAuth::authenticated() && ($this->visit->getValue('v_avid') == bAuth::$av->ID);
  }

  function isOwner(){
    $this->hookVisit();
    $reply = bAuth::authenticated() && (bAuth::$av->ID == $this->visit->getValue('v_avid'));
    return @$reply;
  }

  function getGroup() {
    return VM_projects()->get();
  }

  /*
   * Create a pdf file containing the pre-filled payment form
   * The templates are expected in the 'templates' directory,
   * each template is defined by a pair of files:
   *  - <template>.inc with the data pointer
   *  - <template>.pdf the "empty" pdf form to be filled
   */
  function pdfReceipt($template,$filename='',$verbose=False){
    if (!$this->isWritable())   $verbose = False;
    if (locateAndInclude($template) && function_exists($template)){

      // Merge the formDB's      
      $this->hookVisit();
      foreach($this->visit->formDB     as $k=>$v) if(!empty($v)) $this->formDB[$k] = $v;
      foreach($this->visit->av->formDB as $k=>$v) if(!empty($v)) $this->formDB[$k] = $v;
      foreach(array('pdf_ddmmyy'   => 'av_birthdate',
		    'pdf_birthdate'=> 'av_birthdate',) as $pdf=>$av) $this->formDB[$pdf] = @$this->formDB[$av];
      if ($d=@$this->formDB['pdf_birthdate']){
	$this->formDB['pdf_birthdate']= date('Y-m-d',$d); 
	$this->formDB['pdf_ddmmyy']   = date('d/m/Y',$d); 
      }


      $this->defineVariable(array('pdf_c'        => 'Country',
				  'pdf_zc'       => 'Postal code and city',
				  'pdf_ddmmyy'   => 'Birth date',
				  'pdf_birthdate'=> 'Birth date',
				  'pdf_K'        => 'Sex',
				  'pdf_M'        => 'Sex',
				  ),True);
      foreach($this->visit->av->vars as $k=>$v){
	if(empty($this->vars[$k])) $this->defineVariable($this->visit->av->vars[$k],$k);
      }
      
      // Get some specific data
      $this->formDB['pdf_Lf'] = bForm_Avatar::_fmtName('L,f',$this->formDB);
      $this->formDB['pdf_f']  = bForm_Avatar::_fmtName('f',  $this->formDB);
      $this->formDB['pdf_l']  = bForm_Avatar::_fmtName('l',  $this->formDB);
      $this->formDB['av_cc']  = b_cc::getCC(@$this->formDB['av_residentship']);
      $this->formDB['pdf_c']  = b_cc::getName(@$this->formDB['av_residentship']);

      foreach(array('pdf_K','pdf_M') as $f)     $this->formDB[$f] = 'Nej';
      if     (@$this->formDB['av_sex'] === 'F') $this->formDB['pdf_K'] = 'Ja';
      elseif (@$this->formDB['av_sex'] === 'M') $this->formDB['pdf_M'] = 'Ja';

      $f = $this->formDB['pdf_c'];
      $t = myOrg_name;
      $pdf_trip = array(date('Y-m-d ',(int)$this->visit->getValue('v_start',1))."$f -> $t",
			date('Y-m-d ',(int)$this->visit->getValue('v_end',  1))."$t -> $f");
      if ($this->visit->hookEvent()) array_unshift($pdf_trip,$this->visit->e->name());
      $this->formDB['pdf_trip'] = join("\n\n",$pdf_trip);
      
      if ($id = @$this->formDB['v_projectid']){
	$r = VM_projects()->getMember($id);
	if (!empty($r['lm_value'])) $this->formDB['v_project'] = $r['lm_value']; 
      }
      $this->formDB['pdf_year'] = date('Y',time()); 

      // Fill the pdf form
      $fdf = array();
      foreach($template() as $code=>$a){
	if (!myPear::is_empty($default = @$a['d']))    $fdf[$a['t']] = $default;
	if ($ref = @$a['c']){
	  if (!myPear::is_empty($v=b_fmt::unEscape(@$this->formDB[$ref]))){
	    $fdf[$a['t']] = $v;
	  }else{
	    if($verbose)  myPear::WARNING("Reimbursement form - missing ".$this->getDescr($ref));
	    $this->dbg("Missing '$ref' ".$this->getDescr($ref),cnf_dev);
	  }
	}
      }
      return pdf_form::fill($fdf,vm_root."templates/$template.pdf",$filename);
    }else{
      myPear::ERROR("Can't find the pdf template for \"$template\"");
    }
  }
}
