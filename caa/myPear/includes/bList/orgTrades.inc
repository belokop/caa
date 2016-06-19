<?php
/*
 * Define the employment groups (aka "roles") for the organization.*
 * Say array('adm' => 'Administrators',
 *           'fac' => 'Faculty',...);
 */
locateAndInclude('bList');

class bList_orgTrades extends bList{
  
  public    $debug = False;
  protected $packed_items = array('e_type');
  var $list_quering = array('lm_key'  =>'Code',
			    'e_type'  =>'Employee Type');
  var $list_showing = array('lm_key'  =>'Code',
			    'e_type'  =>'Employee Type',
			    'n_members'=>'');
  var $MODE  = 'RO';
  
  protected $isReady  = True;

  /*
   *
   */
  function __construct($org_id){

    // Initialise the EA module access class
    locateAndInclude('APIaccess_ea');
    if (function_exists('EA_access')) EA_access()->getRank();

    // Instantiate the bList
    parent::__construct(myOrg_ID);
  }

  /*
   * EA module is optional, hence we have to check the environmemnt
   */
  function isManager(){
    myPear_access(EA_MODULE);
    return EA_manager_here;
  }
  
  function isWritable(){
    return b_cnf::check_conditions(array(False=>(array('not EA' => (b_reg::$current_module != EA_MODULE))),
                                         'default' => parent::isWritable()),
				   $this);
  }
  
  protected function isMemberEditable(){
    return $this->isWritable();
  }
  
  /*
   * The trade is not deletable unless it is not used
   */
  protected function isMemberDeletable(){
    static $noname = '???';
    $nMembers = count($this->get_tradeMembers($this->r['lm_key']));
    if (empty($this->r['e_type'])) $this->r['e_type'] = $noname;
    //? if ($nMembers && $this->isWritable()) myPear::MESSAGE("$nMembers members in \"".$this->r['e_type']."\"");
    return b_cnf::check_conditions(array(True =>array('empty' => empty($this->r['lm_key'])),
					 False=>array('RO'    => !$this->isWritable(),
						      '!empty'=> ($nMembers > 0)),
					 'default' => True),
				   $this);
  }
  
  /*
   * Input form
   */
  function getFormAdd(){
    myPear::H1('Updating '.$this->getValue('l_name'));
    
    // Create the record if it is empty
    if (empty($this->member_ID))      $this->updateMember();
    
    // Show the dialog    
    $this->bForm($this->list_quering,$inverted=True);
    if ($this->f->getValue('lm_key',True)) $this->f->isRO['lm_key'] = True;
    $this->f->isRO['lm_value'] = True;
    foreach($this->list_quering as $k=>$v){
      $this->f->textField($k);
    }
    $this->f->closeForm();
  }

  /*
   * Check the sanity
   */
  function getFormAdd_OK(){
    $trades = $this->get_trades();
    $lm_id  = $this->getValue('lm_id');
    $code   = $this->getMemberValue($lm_id,'lm_key');

    $reply = parent::getFormAdd_OK();
    if (!empty($trades[$code]) && empty($lm_id)) {
      $msg = "Code \"$code\" is already in use. Please chouse another one.";
      if (is_object($this->f)) $this->f->errorsInTheForm['lm_key'] = $msg;
      $reply = False;
    }
    return $reply;
  }

  /*
   * Get the array of roles array(code=>title)
   */
  private $_trades = Null;
  function get_trades(){
    if ($this->_trades === Null){
      $this->_trades = array();
      foreach($this->getMembers() as $lm_id=>$r){
	$this->_trades[$r['lm_key']] = @$r['e_type'];
      }
    }
    return $this->_trades;
  }
  
  /*
   * Header for the employmentType records listing
   */
  protected function header(){
    return $this->list_showing;
  }


  /*
   * Formatting for the employmentType record listing
   */
  protected function format(){
    parent::format();
    $nMembers = count($this->get_tradeMembers($this->r['lm_key']));
    if (empty($nMembers)) $this->r['n_members'] = b_fmt::redText(x('em','No members'));
    else                  $this->r['n_members'] = "$nMembers members";
  }
  
  
  /*
   * Get emploeers by trades
   */
  private $tradeMembers = Null;
  function get_tradeMembers($trade=Null){
    static $unk = '??';
    
    locateAndInclude('bList_eaEmpRecords');

    // How to see the errornious avatar
    $modal = True;
    
    // Save in cache the membership
    if($this->tradeMembers === Null){
      
      $this->tradeMembers = array();
      $this->get_trades();
      
      // Scan all the emploeers records to check for the departments
      foreach(myOrg($this->parent_ID)->eaMembers()->getMembers_byStatus() as $av_id=>$r){
      	$name = $r['sfl'];
	$url  = b_url::same(array('form' => 'bForm_Avatar_ea',
				  'id'   => $av_id,
				  'mode' =>'RW',
				  'block_once'=>'office_contract'),
			    b_url::get_GET('^(act_|ed_bList)',True));
	
	$EmpRec_class = new bList_eaEmpRecords($av_id);
	$not_defined = True;
	foreach($EmpRec_class->getMembers() as $er_id=>$er){
	  //if (!isset($er['e_rate']) && isset($er['_e_rate'])) $er['e_rate'] = $er['_e_rate'];
	  if (!isset($er['e_rate']))  continue;
	  if (in_array($er['e_rate'],array_keys($this->_trades))){
	    $this->tradeMembers[$er['e_rate']][] = $av_id;
	  }else{
	    $this->tradeMembers[$unk][] = $av_id;
	    $msg = "Unknown group \"".$er['e_rate']."\" for ";
	    // Modal or not? Some people hate modal windows...
	    if($modal)myPear::WARNING($msg . bJS()->modal_iframe($url," [ $name ]"));
	    else      myPear::WARNING($msg . b_btn::link($name,b_url::same($url)));
	  }
	  $not_defined = False;
	}
	if ($not_defined){
	  $this->tradeMembers[$unk][] = $av_id;
	  $msg = "Employee Type is not set for ";
	  if($modal)myPear::WARNING($msg . bJS()->modal_iframe($url," [ $name ]"));
	  else      myPear::WARNING($msg . b_btn::link($name,b_url::same($url)));
	}
      }
      ksort($this->tradeMembers);
    }
    if (empty($trade)) $trade = $unk;
    if (!@$this->tradeMembers[$trade]) myPear::WARNING_T("No members in \"$trade\"");
    return @$this->tradeMembers[$trade];
  }
}
