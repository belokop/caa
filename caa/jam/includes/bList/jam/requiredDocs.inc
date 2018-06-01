<?php
/*
 * Documents required for the position
 */

/*
 * List of documents required by the Position
 */
define('JAM_compulsory_doc' ,'C');
locateAndInclude('bList_jam');
class bList_jam_requiredDocs extends bList_jam{

  var $list_quering = array('lm_value' => 'Filename',
			    'lm_key'   => 'Document description',
			    'lm_status'=> 'Compulsory?');
  
  protected $isReady = True;
  protected $orderBy ='lm_value';
  private   $pos_id  = Null;

  /**
   * @param $parent might be :
   * - numeric "position_id" 
   *   the class keeps names of documents required for the position
   * - string "module_name"
   *   the class keeps names of all the known documents 
   */
  function __construct($pos_id = Null){
    if (empty($pos_id)) $pos_id = JAM_posID();
    $this->pos_id = $pos_id;
    parent::__construct($pos_id,'required documents','document');
    if (!is_object(JAM_pos($this->pos_id)))      b_debug::traceBack("? empty position");

    // to be done only once
    fix_requiredDocs();
  }
  
  /*
   * The "required document" can't be removed if it was uploaded by somebody
   */ 
  protected function isMemberDeletable(){
    if ($reply = $this->isWritable()){
      locateAndInclude('jam_Document');
      $n = jam_Document::is_inUse($this->r['lm_id']);
      if ($n > 0){
	$this->dbg("$n documents \"".$this->r['lm_key']."\" were already uploaded",True);
	$reply = False;
      }
    }
    return $reply;
  }

  /*
   * Get the list of required documents
   */
  function get_requiredDocs($compulsory_only=False){
    $reply = array();
    foreach ($this->getMembers() as $id=>$r){ 
      if (!$compulsory_only || ($r['lm_status'] === JAM_compulsory_doc)) $reply[$r['lm_id']] = $r['lm_key'];
    }
    if (is_object(JAM_pos($this->pos_id))){
      if (empty($reply)) MSG::WARNING("Can't get the list of required documents for ".x('em',JAM_pos($this->pos_id)->name()));
    }else{
      b_debug::traceBack("? empty position");
    }
    $this->dbg(b_fmt::joinX(', ',$reply));
    return $reply;
  }

  /*
   *
   */
  protected function getFormAdd(){
    if (!cnf_inside_fb)  MSG::MESSAGE("documents required by the position");
    $this->bForm($this->list_quering,True);

    // Filename
    if (!$this->f->getValue('lm_value',True)){
      $n_max = 0;
      foreach($this->getMembers() as $m) $n_max = max($n_max,(int)$m['lm_value']);
      $this->f->getValue_reset('lm_value',sprintf('ap_doc_%02d',($n_max+1)));
    }
    $this->f->isRO['lm_value'] = True;
    $this->f->textField('lm_value');
    $this->f->textField('lm_key');
    $this->f->checkBox('lm_status');
    $this->f->closeForm();
  }

  protected function header($arg=Null){
    $header = $this->list_quering;
    return $header;
  }

  public function is_documentCompulsory($name){
    $r = $this->getMember(array('lm_value'=>$name));
    $reply = (bool)$this->_documentCompulsory($r);
    $this->dbg($reply);
    return $reply;
  }

  private function _documentCompulsory($rec){
    return (($rec['lm_status'] === 'no') ? '' : 'checked');
  }

  function set_documentCompulsory($id,$value=True){
    myPear_db()->query("UPDATE zzz_list_members SET lm_status = '".($value?'yes':'no')."' WHERE lm_id = $id");
  }

  protected function format(){
    $this->r['lm_status'] = "<input type='checkbox' name='b".$this->r['lm_id']."' disabled ".$this->_documentCompulsory($this->r).'/>'; 
    $this->r['lm_key']   = b_fmt::longString($this->r['lm_key'],30,False);
    $this->t->css['lm_value']['align'] = 'align_left';
  }
}

/*
 * jam 4 -> jam 5
 */
function fix_requiredDocs(){
  static $already_running = 0;

  // This should be executed only once...
  foreach(array('already running'    => $already_running++,
		'cnf_CLI'            => cnf_CLI,
		'no "jam_documents"' => !myPear_db()->tableExists('jam_documents'),
		'no "pos_req_rp"'    => !myPear_db()->columnExists('pos_req_rp','jam_positions')) as $txt=>$c){
    if ($c){
      if (cnf_dev)  MSG::DEBUG(__FUNCTION__."() exiting... ".x('em',$txt));
      return;
    }
  }
  
  $debug = True;
  MSG::DEBUG(x('h1',__FUNCTION__ .' start'));
  
  locateAndInclude('bForm_jam_Applicant');
  
  if (myPear_db()->tableExists('jam_old_documents')){
    myPear_db()->qquery("DROP TABLE `jam_old_documents`",True);
    myPear_db()->reset_cache();
  }
  
  foreach(array('doc_lmid','doc_date') as $f){
    if (!myPear_db()->columnExists($f,'jam_documents')){
      myPear_db()->truncateTable('jam_documents');
      myPear_db()->qquery("ALTER TABLE `jam_documents` ADD `$f` INT NOT NULL AFTER `doc_apid`",$debug);
      myPear_db()->reset_cache();
    }
  }
  
  // update the indexes
  myPear_update_addIndex('jam_documents',array('doc_apid','doc_lmid'));
  
  locateAndInclude('bForm_jam_Position');
  locateAndInclude('jam_functions.inc');
  foreach(array_keys(bForm_jam_Position::getPositions()) as $pos_id){
    $pos = loader::getInstance('bForm_jam_Position',$pos_id,array('strict'));
    MSG::DEBUG("============================== ".$pos->name());
    if (preg_match('/PhD in Solar Activity/i',$pos->name())){
      $list = array('pos_req_rp' =>array('ap_rp_file','Motivation statement: why to pursue studies at Nordita'),
		    'pos_req_cv' =>array('ap_cv_file','Curriculum vitae with List of publications'),
		    'pos_req_pl' =>array('ap_pl_file','Verified list of courses with grades'),
		    'pos_req_er' =>array('ap_er_file','Employment record'),
		    'pos_req_mt' =>array('ap_mt_file','Master Thesis'));
    }else{
      $list = array('pos_req_rp' =>array('ap_rp_file','Research plan'),
		    'pos_req_er' =>array('ap_er_file','Employment record'),
		    'pos_req_pl' =>array('ap_pl_file','List of Publications'),
		    'pos_req_cv' =>array('ap_cv_file','Curriculum vitae'),
		    'pos_req_mt' =>array('ap_mt_file','Master Thesis'));
    }
    
    // Walk thru the applicants and move the "document field" to the "jam_Document class"
    $qa = myPear_db()->qquery("SELECT * FROM jam_applicants ".
			      " LEFT JOIN jam_positions ON pos_id = ap_posid ".
			      " WHERE pos_id=".$pos->ID,$debug);
    while($r = myPear_db()->next_record($qa)){
      $ap  = new bForm_jam_Applicant($r['ap_id']);
      foreach($list as $pos_ident=>$a){
	if ($pos->getValue($pos_ident,1,1) ==='yes'){ // i.e. the document required by the position
	  
	  $l = JAM_requiredDocs($pos->ID);
	  
	  // get/create the record for the is document
	  $args = array('lm_key'    => $a[1],
			'lm_value'  => $a[0],
			'lm_status' => 'yes');
	  $l_member_ID = 0;
	  if ($rr = $l->getMembers($args)){
	    foreach($rr as $id=>$rec)  $l_member_ID = $rec['lm_id']; 
	  }else{
	    $l->updateMember($args);
	    $l_member_ID = $l->member_ID;
	  }
	  if (empty($l_member_ID)) b_debug::print_r($l);
	  if (empty($l_member_ID)) b_debug::internalError('????????????????',$l);
	  locateAndInclude('jam_Document');
	  $doc = new jam_Document($ap, $l_member_ID);
	  if (!empty($r[$a[0]])){
	    $time = 0;
	    foreach(array('ap_submitdate',
			  'pos_deadline') as $t) if (!empty($r[$t])) {
	      $time = b_time::txt2unix($r[$t]);
	      break;
	    }
	    $doc->set_uploaded($time);
	  }
	}
      }
      $ap->__clean_cache();
    }
  } 
    
  // remove the outdated fields
  foreach(array('jam_applicants'=>array('ap_tmp',
					'ap_mt_url',
					'ap_mt_file',
					'ap_cv_file',
					'ap_er_file',
					'ap_pl_file',
					'ap_rp_file',
					),
		'jam_positions' =>array('pos_posfid',
					'pos_scid',
					'pos_sc_pwd',
					'pos_sc_username',
					'pos_sc_per_field',					 
					'pos_req_cv',
					'pos_req_mt',
					'pos_req_pl',
					'pos_req_rp',
					'pos_req_er',
					'pos_may_see_list',
					'pos_may_send_pdfs',
					'pos_exclusive_fields',
					'pos_cm_admins',
					'pos_cm_secretary',
					'pos_cm_observers',
					'pos_ap_modify_observers',
					'pos_ap_modify_admins',
					'pos_docs_dir',),
		'jam_documents' =>array('doc_tmp',
					'doc_url',
					'doc_name',
					'doc_file',),
		) as $table=>$list){
    foreach($list as $f){
      if (myPear_db()->columnExists($f,$table)){
	myPear_db()->qquery("ALTER TABLE `$table` DROP `$f`",True);
	myPear_db()->reset_cache();
      }
    }
  }
  MSG::DEBUG(x('h1',__FUNCTION__ .' end'));
}
