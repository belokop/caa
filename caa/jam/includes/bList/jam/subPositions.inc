<?php
/*
 * Define the position "fine structure", i.e. research fields within the position
 */

locateAndInclude('bList_jam');
class bList_jam_subPositions extends bList_jam{

  var $list_quering = array('lm_key' => 'Research Field');

  protected $isReady = True;
  protected $orderBy ='lm_key';
  private   $pos     = Null;
  
  /*
   *
   */
  function __construct($pos_id=Null){

    // Hook the position
    if (empty($pos_id))  $pos_id = JAM_posID();
    $this->pos = myPear::getInstance('bForm_jam_Position',$pos_id,array('fatal','strict'));

    // Execute the bList constructor
    parent::__construct($this->pos->ID);

    // Create the first entry as the Position itself, i.e. initialise the
    // class with no reseach fields
    if (!$this->getMembers()) $this->addMember(array('lm_key'=>CONST_noname));
  }

  /*
   *
   */
  function add($lm_key, $lm_value=Null,$lm_id=Null,$lm_status=Null){
    if (empty($lm_value)) $lm_value = CONST_noname;
    return parent::add($lm_key, $lm_value, $lm_id, $lm_status);
  }

  /*
   *
   */
  function isWritable(){
    return $this->pos->isWritable();
  }

  /*
   * Check that the "sub-position is not already "populated" by applicants
   * @return (boolean)
   *   - false if the RF can'be deleted (there are applicants subscribed to it)
   *   - true  if no applicants yet, hence the RF might be deleted
   */
  protected function isMemberDeletable(){
    $q = myPear_db()->query("SELECT *,COUNT(ap_id) AS n FROM jam_rf ".
			      " LEFT JOIN jam_applicants ON ap_rfid = rf_id ".
			      " LEFT JOIN zzz_list_members ON rf_lmid = lm_id ".
			      " WHERE rf_lmid = ".$this->r['lm_id'].
			      " GROUP BY rf_id");
    $reply = True;
    while($r = myPear_db()->next_record($q)){
      if ($r['n'] > 0)	$reply = False;
    }
    return $reply;
  }

  /*
   * Get the list of fields
   */
  function get_subPositions(){
    $reply = array();
    foreach ($this->getMembers() as $id=>$r){
      if ($r['lm_key'] !== CONST_noname)      $reply[$r['lm_id']] = $r['lm_key'];
    }
    if (empty($reply)) return array('--- no reaseach fields defined ---');
    return $reply;
  }

  protected function getFormAdd(){
    myPear::MESSAGE("Research fields within the position");
    $this->bForm($this->list_quering,True);
    $this->f->textField('lm_key');
    $this->f->closeForm();
  }

  protected function header($arg=Null){
    unset($this->list_quering['lm_value']);
    return $this->list_quering;
  }
  
  protected function format(){
    if ($this->r['lm_key'] == CONST_noname){
      $this->r = array();
    }else{
      $this->r['lm_key'] = b_fmt::longString($this->r['lm_key'],30,False);
    }
  }
}

fix_subPositions();

// Convert JAM-4 RF classes => JAM-5 subPositions
function fix_subPositions(){
  if (!cnf_CLI && myPear_db()->columnExists('rf_name','jam_rf')){
    set_time_limit(0);
    locateAndInclude('jam_functions.inc');
    myPear::DEBUG(x('h1',__FUNCTION__.' started ---------------------------------'));
    fix_subPositions_1();
    fix_subPositions_2();
    fix_subPositions_3();
    myPear::DEBUG(x('h1',__FUNCTION__.' completed ---------------------------------'));
  }
}

// Remove unused columns from the database & add the new ones
function fix_subPositions_1(){
  myPear::DEBUG(x('h2',__FUNCTION__));
  myPear_db()->reset_cache();
  if (myPear_db()->tableExists('jam_old_posfields')){
    myPear_db()->qquery('DROP TABLE `jam_old_posfields`',1);
  }
  if (myPear_db()->columnExists('pos_fields_cm','jam_positions')){
    myPear_db()->qquery('ALTER TABLE `jam_positions` DROP `pos_fields_cm`',1);
  }
  if (myPear_db()->columnExists('rf_nickname','jam_rf')){
    myPear_db()->qquery('ALTER TABLE `jam_rf` DROP `rf_nickname`',1);
  }
  if (!myPear_db()->columnExists('rf_lmid','jam_rf')){
    myPear_db()->qquery('ALTER TABLE `jam_rf` ADD `rf_lmid` INT NOT NULL AFTER `rf_posid`',1); 
  }
  myPear_db()->reset_cache();
}

// Convert from JAM-4 RF classes to JAM-5 subPositions
function fix_subPositions_2(){

  $debug = False;
  myPear::DEBUG(x('h2',__FUNCTION__));
  $t = new b_table_zebra(array('pos'  =>' ',
			       'rf_id'=>' ',
			       'rf'   =>' ',
			       'n'    =>' ',
			       't'    =>' ',
			       ));
  $t->noSort = True;
  $t->repetValuesCol1only = 'pos';

  $qq = myPear_db()->query("SELECT * FROM jam_positions");

  $rf_to_drop = array();
  while ($rr = myPear_db()->next_record($qq)){
    myPear::DEBUG(x('h3',"==============================================$rr[pos_name]"));
    if (!preg_match('/(yes|on)/i',$rr['pos_type'])) $rr['pos_type'] = bIcons()->get('ok');
    else                                            $rr['pos_type'] = '';   
    $q = myPear_db()->qquery("SELECT *,COUNT(ap_id) AS n  FROM jam_rf ".
			     " LEFT JOIN jam_applicants ON rf_id = ap_rfid ".
			     " WHERE rf_posid = $rr[pos_id] ".
			     " GROUP BY rf_id ORDER BY rf_name",$debug);
    if (myPear_db()->num_rows($q)){
      while ($r = myPear_db()->next_record($q)){
	$drop = ($rr['pos_type'] != 'no') && empty($r['n']) && !empty($r['rf_id']);
	if ($drop){
	  $rf_to_drop[] = $r['rf_id'];
	  $t->extraTD[] = bIcons()->get('drop');
	}
	
	if (!empty($r['rf_name'])) myPear::DEBUG(x('h4',"-----------------------------$r[rf_name]"));
	$t->prt(array('pos'  =>$rr['pos_name'],
		      't'    =>$rr['pos_type'],
		      'rf'   =>$r['rf_name'],
		      'rf_id'=>$r['rf_id'],
		      'n'    =>$r['n']));
	
	/*
	 * Start processing
	 */
	if (!$drop && (int)$r['rf_id']){
	  if (myPear::is_empty($name = trim($r['rf_name'])))  $name = CONST_noname;
	  $rf = myPear::getInstance('bForm_jam_RF',$r['rf_id'],array('fatal','strict'));
	  $jpf= JAM_subPositions($rr['pos_id']);
	  $rf->updateDB(array('rf_lmid'=>$jpf->addMember(array('lm_key'=>$name))));
	}
      }
    }else{
      $t->prt(array('pos'  =>$rr['pos_name'],
		    't'    =>$rr['pos_type'],
		    ));
    }
  }
  $t->close();

  // drop spurious records
  myPear::DEBUG("------------ drop spurious records");
  foreach($rf_to_drop as $rf_id){
    myPear_db()->qquery("DELETE FROM jam_rf WHERE rf_id = $rf_id",$debug);
  }

  // drop field 'rf_name'
  myPear::DEBUG("------------ drop field 'rf_name'");
  if (myPear_db()->columnExists('rf_name','jam_rf')){
    myPear_db()->qquery('ALTER TABLE `jam_rf` DROP `rf_name`',$debug);
    myPear_db()->reset_cache();
  }
}

// Fine tuning, set RF name to 'CONST_noname' for the Positions without explicit RF
function fix_subPositions_3(){

  $debug = True;
  myPear::DEBUG(x('h2',__FUNCTION__));
  myPear_db()->qquery("UPDATE zzz_list_members SET lm_key   = '".CONST_noname."' WHERE lm_key = 'dummy'",$debug);
  myPear_db()->qquery("UPDATE zzz_list_members SET lm_value = '".CONST_noname."' WHERE lm_value = 'dummy'",$debug);
  
  $qq = myPear_db()->query("SELECT * FROM zzz_lists ".
			     " LEFT JOIN jam_positions ON l_parent = pos_id ".
			     " WHERE l_class = 'bList_jam_subPositions'");
  while ($rr = myPear_db()->next_record($qq)){
    $q = myPear_db()->qquery("SELECT * FROM zzz_list_members ".
			       " WHERE lm_lid = $rr[l_id] AND lm_key = '$rr[pos_name]'",$debug);
    while ($r = myPear_db()->next_record($q)){
      myPear_db()->qquery("UPDATE zzz_list_members SET lm_key = '".CONST_noname."' WHERE lm_lid = $r[lm_lid]",$debug);
    }
  }
}
