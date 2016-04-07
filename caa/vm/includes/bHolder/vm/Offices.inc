<?php
locateAndInclude('bHolder_ea_Offices');
class bHolder_vm_Offices extends bHolder_ea_Offices{

  function __construct(){
    parent::__construct('summary',VM::$e->name(False));
  }

  function query($where=1){
    return myPear_db()->qquery("SELECT * FROM abs_offices ORDER BY o_name ASC");
  }

  function init() {}

  private $_o_name_was = '?';
  function format() {
    $q = myPear_db()->qquery("SELECT v_id,av_id,av_lastname,av_firstname FROM abs_offices ".
			     " LEFT JOIN abs_visits ON o_id = v_oid ".
			     " LEFT JOIN zzz_avatars ON av_id = v_avid ". 
			     " WHERE ".join(' AND ',array("v_eid = ".VM::$e->ID,
							  "v_status = '".STATUS_YES."'",
							  "o_name  = '".$this->rec['o_name']."'")).
			     " ORDER BY av_lastname",cnf_dev);

    while($r = myPear_db()->next_record($q)) $tenants[] = array($r['v_id'],bForm_Avatar::_fmtName('Lf',$r));
    if (empty($tenants)) $tenants = array('');

    foreach($tenants as $k=>$tenant){
      if (!empty($tenant))$this->rec['_tenants'] = x(sprintf('a href="%s"',
							     b_url::same(sprintf('?mode_once=RO&id=%s&form=vm_Visit',$tenant[0]))),$tenant[1]);
      if ($k == 0) list($o,$o_name) = $this->format_summary();
      
      // Remove the repeted columns
      if ($o_name == $this->_o_name_was) foreach($this->header() as $k=>$v) if ($k != '_tenants') unset($this->rec[$k]);
      $this->_o_name_was = $o_name;
      $this->t->prt($this->rec);
    }
    $this->rec = array();
  }
  

  /*
   *
   */
  function listEntries_summary(){
      bIcons()->reset();

      $edit_buttons = array();
      if ($this->isWritable()){
        $edit_buttons[] = b_btn::submit('add new office',
                                        b_url::same('?id=new&form=vm_Office&clean_once=act_code,act_id'));
      }
      $this->t->tr(b_btn::UL_buttons($edit_buttons),'colspan=6');
  }
}
