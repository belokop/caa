<?php
locateAndInclude('bUnit');

class bUnit_ea extends bUnit{

  function __construct($parent_ID=myOrg_ID,$rank){
    parent::__construct($parent_ID,$rank);
  }

  /*
   *
   */
  protected function edit_unitMember($av_id){
    locateAndInclude('b_tabs');
    b_tabs::$jQueryTabs_depth = 1;
    
    ea_myPage($av_id);
    return 'busy, wait please..';
  }
  
  protected function button_edit_unitMember(){
    $name = bForm_Avatar::_fmtName('Lf',$this->rec);
    $btn  = bIcons()->getButton(array('X'=>1,
				      'l'=>b_url::same(array('edi_m_once'=>$this->rec['av_id'],
							     'ed_bList'  => get_class($this),
							     'arg1_once' => $this->parent_ID,
							     'arg2_once' => $this->rank),
						       array()),
				      'd'=>"modify $name's membership"),
				'i-edit');

    $btn  = bIcons()->getButton(array('X'=>1,
				      'l'=>"?q=".EA_MODULE."&function=ea_myPage&clean_once=act_code,act_id&av_id=".$this->rec['av_id'],
				      'd'=>"modify $name's membership"),
				'i-edit');
    return ($this->isMemberEditable() ? $btn : '');
  }
}
