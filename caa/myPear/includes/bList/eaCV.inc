<?php
/*
 * Helper for bForm_Avatar
 * Keep list of people's cvs
 * 
 * Member format:
 *  lm_key - av_id (Avatar ID) 
 *  cvs  - HTML formated CV summary in 1-2 lines
 *  cvf  - HTML formated CV text
 */
locateAndInclude('bList');
locateAndInclude('bForm_Avatar_ea');

class bList_eaCV extends bList{
  
  protected $packed_items = array('cvs','cvf');
  public    $list_quering = array('lm_key' => 'CV holder',
				  'cvs'    => 'Brief CV',
				  'cvf'    => 'Full CV');
  public    $list_showing = array('lm_key' => '',
				  'cvs'    => 'Brief CV',
				  'cvf'    => 'Full CV');
  private   $existing_CV = array();
  private   $missing_CV  = array();
  protected $isReady  = True;

  public    $av = Null;

  /*
   * @param $parent_ID 
   */
  function __construct($org_id=Null,$av_id=Null){

    // Hook the Avatar
    if (@$_POST['_post_bList_class'] == get_class($this)){
      if (empty($av_id)) $av_id = @$_POST['av_id'];
    }
    if (is_object($av_id)) $this->av = $av_id;
    elseif(!empty($av_id)) $this->av = new bForm_Avatar_ea($av_id);
    
    if (empty($org_id)) $org_id = myOrg_ID;
    parent::__construct($org_id);
    
    //
    // Prefill list of personell
    //
    if (is_object($this->av)){
      $this->dbg($this->av->fmtName());
      if (!$this->getMembers(array('lm_key'=>$this->av->ID))) $this->addMember(array('lm_key'=>$this->av->ID,
										     'cvs'   =>'',
										     'cvf'   =>''));
      
      $this->existing_CV = array($this->av->ID => $this->av->fmtname());
    }elseif (EA_administrator_here){
      // List of existing CV's
      $list = array(0);
      foreach($this->getMembers() as $lm_id=>$r) $list[] = "av_id  = $r[lm_key]";
      
      $q = myPear_db()->query("SELECT av_id,av_firstname,av_lastname FROM zzz_avatars WHERE ".join(' OR ',$list).myPear_db()->orderBy());
      while($r = myPear_db()->next_record($q)){
	$this->existing_CV[$r['av_id']] = bForm_Avatar::_fmtname('Lf',$r);
      }
      
      // List of non-existing CV's
      foreach(myOrg()->eaMembers()->getMembers_byStatus(array(EA_role_employee)) as $av_id=>$r){
	if (!in_array($av_id,array_keys($this->existing_CV))) $this->missing_CV[$av_id] = bForm_Avatar::_fmtname('Lf',$r);
      }
    }elseif(is_object(bAuth::$av)){
      $this->existing_CV[bAuth::$av->ID] = bAuth::$av->fmtname('Lf',$r);
      $this->missing_CV[bAuth::$av->ID]  = bAuth::$av->fmtname('Lf',$r);
    }
  }

  /*
   *
   */
  protected function hookJOIN(){
    $this->sortArgs = myPear_db()->orderBy();
    return join(' ',array(" LEFT JOIN zzz_avatars ON av_id = lm_key ",
                          parent::hookJOIN()));
  }

  /*
   *
   */
  protected function message_addMember_name($r){
    $lm_key = $this->getMemberValue($this->getValue('lm_id'),'lm_key');
    $reply = @$this->existing_CV[$lm_key];
    if (empty($reply)) $reply = @$this->missing_CV[$lm_key];
    return (string)$reply;
  }

  /*
   * Deletable if there is there is no parent avatar
   */
  protected function isMemberDeletable(){
    return empty($this->existing_CV[$this->r['lm_key']]);
  }
  
  /*
   * The owner can always edit his CV
   */
  protected function isMemberEditable(){
    $reply = $this->isWritable() || (is_object(bAuth::$av) && (bAuth::$av->ID == $this->rec['lm_key']));
    $this->dbg($reply);
    return $reply;
  }

  /*
   *
   */
  protected function getFormAdd(){

    $lm_id = $this->getValue('lm_id');
    if ($this->MODE == 'RW'){
      $this->list_quering['cvf'] .= '<br/><em>(HTML formatted)</em>';
      $this->list_quering['cvs'] .= '<br/><em>(HTML formatted, 1-2 lines)</em>';
      $names = $this->missing_CV;
    }else{
      $lm_key = $this->getMemberValue($lm_id,'lm_key');
      $names  = $this->existing_CV;
    }
    

    $this->bForm($this->list_quering,True);
    if ($lm_key = $this->getMemberValue($lm_id,'lm_key')){
      if (empty($this->existing_CV[$lm_key])){
	$this->av = new bForm_Avatar($lm_key);
	$this->existing_CV[$lm_key] = $this->av->fmtName();
      }
      $names = array($lm_key=>$this->existing_CV[$lm_key]);
      $this->f->isRO['lm_key'] = True;    
    }
    $this->f->tips = array();
    $this->f->selectMenu('lm_key',$names);
    $this->f->textArea('cvs');
    $this->f->textArea('cvf',10);
    if (is_object($this->av)) $this->f->textHidden('av_id',$this->av->ID);
    $this->f->closeForm();
  }

  /*
   *
   */
  protected function getMembers_forListing($arg=Null){
    if (is_object($this->av)) return $this->getMembers(array('lm_key'=>$this->av->ID));
    else                      return parent::getMembers_forListing($arg);
  }

  /*
   * Put the CV's into an accordion
   */
  private $show_open = 0;
  protected function show_open(){
    parent::show_open();
    //    if (!$this->show_open++) b_fmt::accordion('open');
    //    b_fmt::accordion($this->existing_CV[$this->r['lm_key']]);
  }

  protected function show_end(){
    //    b_fmt::accordion('close'); 
  }

  /*
   * Do not show control buttons for an individual
   */
  protected function show_close() {
    if (!is_object($this->av)) return parent::show_close();
    else                       return '';
  }

  /*
   * 
   */
  protected function format($secondCall=False){
    $av_name = @$this->existing_CV[@$this->r['lm_key']];
    if (empty($av_name)) $av_name = b_fmt::redText("Orphan CV for an unknown Avatar_".$this->r['lm_key']);
    $this->r['lm_key'] = $av_name;
  }
}
