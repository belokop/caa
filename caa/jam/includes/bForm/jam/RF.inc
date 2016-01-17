<?php
/*
 * An auxiliary form which keeps the links to the research fields
 * for the position.
 *
 * This form does not have the 'getForm()' method, i.e. it is a 'virtual' form.
 */

locateAndInclude('bForm_jam');
locateAndInclude('APIaccess_jam');

class bForm_jam_RF extends bForm_jam{

  var $TABLE   = 'jam_rf';
  var $TABLEID = 'rf_id';
  var $pos     = Null;

  /*
   *
   */
  function defineVariables() {
    if ($this->defineVariable_counter) return;
    $this->defineVariable(array('rf_name' => 'Reseach fiield',
				'rf_posid'=> 'rf_posid',
				'rf_lmid' => 'rf_lmid',
				),True);
  }

  /*
   *
   */
  function __construct($id, $mode='RO',$pos=False){

    parent::__construct($id, $mode);

    if ($pos instanceof bForm_jam_Position){
      $this->pos = $pos;
    }else{
      $this->pos = myPear::getInstance('bForm_jam_Position',
				       $this->getValue('rf_posid',1),
				       array('strict','fatal'));
    }
  }
  
  /*
   *
   */
  function name() {
    $this->defineVariables();
    if (!($name=$this->getValue('rf_name'))){
      if (is_object($this->pos)) 	$name = $this->pos->name();
    }
    return $name;
  }
  
  function isReadable() {
    if (is_object($this->pos)) return $this->pos->isReadable();
  }
  
  function isWritable() {
    if (is_object($this->pos)) return $this->pos->isWritable();
  }
    
  function applicants(){
    if (!isset($this->appList)){
      $this->appList = array();
      $q = myPear_db()->query("SELECT ap_id FROM jam_applicants WHERE ap_rfid = $this->ID ".
				 (JAM_access()->may_seeRevokedApplications()
				  ? ''
				  : " AND ap_status != ".JAM::status(JAM_ap_revoked)));
      while ($r = myPear_db()->next_record($q)) $this->appList[] = $r['ap_id'];
    }
    return $this->appList;
  }

  /*
   * Load the RF data to formDB
   */
  function readDefRecord(){
    $this->formDB = array();
    $q = myPear_db()->query("SELECT *,lm_key AS rf_name FROM jam_rf ".
			    " LEFT JOIN zzz_list_members ON rf_lmid = lm_id ".
			    " LEFT JOIN jam_positions    ON rf_posid = pos_id ".
			    " WHERE $this->TABLEID = '$this->ID'");
    while ($r = myPear_db()->next_record($q))      $this->formDB = $r;
  }

  function getValue($name,$arg1=True,$arg2=True){
    return parent::getValue($name,True,True);
  }

  function formOKspecific() { }

  /*
   * List all RF names for the position, excluding the position itself,
   * i.e. the answer is empty if the position has no research fields. 
   */
  static function all_RFs($pos_id) {
    $q = myPear_db()->qquery("SELECT *,lm_key AS rf_name FROM jam_rf ".
			       " LEFT JOIN zzz_list_members ON rf_lmid = lm_id ".
			       " LEFT JOIN jam_positions ON rf_posid = pos_id ".
			       " WHERE pos_id=$pos_id");
    $reply = array();
    while ($r = myPear_db()->next_record($q)){
      if ($v = trim($r['rf_name']))  $reply[$r['rf_id']] = $v;
    }
    if (count($reply) > 1) return $reply;
    else                   return array();
  }

}
