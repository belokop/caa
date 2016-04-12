<?php
/*
 * Keep list of persons to be contacted in case of emergency
 */
locateAndInclude('bUnit');
locateAndInclude('bForm_Avatar_ea');

class bUnit_ea_contacts extends bUnit{

  protected $isReady  = True;
  protected $packed_items= array('relation');

  function __construct($parent_ID,$rank=RANK__ea_contactPerson){
    parent::__construct($parent_ID,RANK__ea_contactPerson);

    if (($av_ddate=$this->getValue('av_ddate')) && ($av_ddate>time())){
      foreach($this->getUnitMemberIDs() as $av_id){
	// ?	$this->remove_unitMember($av_id,$toConfirm=False);
      }
    }
  }
  
  protected function readDefRecord_joinTables(){
    return "LEFT JOIN zzz_avatars ON av_id = $this->parent_ID";
  }


  function unitMemberTitle($av_id=Null){
    return 'contact person';
  }

  protected function membersInfo_toSelect($extended=False){
    return array_merge(parent::membersInfo_toSelect($extended),
                       array('relation','av_phone'));
  }

  function isOwner(){
    return ($this->parent_ID == bAuth::$av->ID);
  }

  protected function isMemberEditable(){
    return $this->isWritable();
  }

  /*
   * Edit dialog
   */
  protected function edit_unitMember($av_id){
    $rec = $this->getUnitMember($av_id);
    $cp = bForm_Avatar::_fmtName('fl',$rec,False);
    myPear::setPageHeader("Contact person $cp");
    
    $this->bForm(array());
    
    $this->f->defineVariable(" ",'who');
    $this->f->isRO['who'] = True;
    $this->f->formDB['who'] = $cp;
    $this->f->textField('who');
    
    $this->f->defineVariable("Relation",'relation');
    $this->f->formDB['relation'] = @$rec['relation'];
    $this->f->textField('relation');

    $this->f->textHidden('um_id',$rec['um_id']);
    $this->f->closeForm();
    return 'busy...';
  }

  protected function header(){
    return array('name'=>'', 'relation'=>'relation','av_email'=>'e-mail','av_phone'=>'phone');
  }
  
  function format(){
    $this->t->css['av_phone']['align'] = 'align_left';
  }
}
