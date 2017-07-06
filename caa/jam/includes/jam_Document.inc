<?php
/*
 * This class keep documents, supplied by the applicant.
 * The documents are uploaded from the application form
 */

locateAndInclude('bForm_jam_Applicant');

class jam_Document{
  
  public  $debug  = False;
  public  $ID     = Null;
  public  $formDB = array();  
  private $ap     = null;

  private $member_ID = 0;

  /*
   * @params
   * $ap_id - Applicant class (either the class instance OR Applicant ID)
   * $lm_id - Documents list member, see class "bList_jam_requiredDocs"
   */  
  function __construct($ap_id, $lm_id){
    $this->dbg('-------------------------------------------------');

    // Applicant
    if (empty($ap_id)) b_debug::internalError('? empty "ap_id"');
    elseif($ap_id instanceof bForm_jam_Applicant)        $this->ap = $ap_id;
    elseif(@bForm_jam_Applicant::$current->ID == $ap_id) $this->ap = bForm_jam_Applicant::$current;
    else                                                 $this->ap = new bForm_jam_Applicant($ap_id);
    if (!(int)$this->ap->ID) b_debug::internalError("empty bForm_jam_Applicant");

    // Reference to the document in the list of documents for position
    $this->member_ID = $lm_id;
    $this->_init();
    if ($this->ap->isReady())    $this->check_sanity();
  }

  function __toString(){
    return $this->name();
  }

  function __clean_cache(){
    $this->ap->__clean_cache();
  }

  private function dbg($text='',$forced=False){
    b_debug::xxx($text,array(3,$this,'maroonText',(bool)$forced || (bool)self::$_debug));
  }
  
  /*
   * Debug / tuning helper
   */
  function check_sanity(){
    $formDB = '';
    switch(file_exists($this->uploadedFile_path())){
    case True:
      if (!$this->isUploaded()){
	$errors[] = 'File <i>"%s"</i> exists, but marked as NOT uploaded';
	$this->set_uploaded($this->ap->timestamp,False);
      }
      break;
      
    case False:
      if ($d=$this->isUploaded()){
	ob_start();
	print_r($this->formDB);
	$formDB .= ob_get_contents();
	ob_end_clean();
	$errors[] = 'File <i>"%s"</i> DOES NOT exists, but marked as uploaded '.b_time::sql_timestamp($d);
	$this->clean_uploadedFlag();
      }
      break;
    }

    if (!empty($errors)){
      foreach($errors as $error){
	$msg = sprintf($error,$this->name());
	myPear::WARNING($msg);
	b_debug::report_bug($msg . $formDB,__METHOD__);
      }
    }
  }

  /*
   * (Re)read the document data from the database,
   * create the documents folder if not yet done
   */
  private function _init(){
    $this->dbg();
    if (!empty($this->member_ID)){
      $w = array("doc_lmid =".$this->member_ID,
		 "doc_apid =".$this->ap->ID);
      
      $q=myPear_db()->qquery("SELECT *,lm_key AS doc_name,lm_value AS doc_file FROM jam_documents ".
			     " LEFT JOIN zzz_list_members ON lm_id=doc_lmid ".
			     " LEFT JOIN jam_applicants   ON ap_id=doc_apid ".
			     " LEFT JOIN zzz_avatars      ON av_id=ap_avid ".
			     " LEFT JOIN jam_positions    ON pos_id=ap_posid ".
			     " WHERE ".join(' AND ',$w),self::$_debug);
      while($r=myPear_db()->next_record($q))     $this->formDB = $r;
      
      if (empty($this->formDB)){
	if (cnf_dev) myPear::MESSAGE("Creating documents folder for ".$this->ap->name());
	$w[] = myPear_db()->quote(bMaster()->reserve_masterCounter(),True,'doc_id');
	myPear_db()->qquery($cmd="INSERT INTO jam_documents SET ".join(',',$w),self::$_debug);
	$this->_init();
      }else{
	$this->ID = $this->formDB['doc_id'];
      }
    }else{
      myPear::WARNING_T('??? empty $this->member_ID');
    }
  }

  /*
   * Clean the flag 'document is uploaded'
   */
  function clean_uploadedFlag(){
    myPear_db()->query("UPDATE jam_documents SET doc_date=0,timestamp='".b_time::sql_timestamp()."' WHERE doc_id = '$this->ID'");
    myPear::WARNING('CLEAN flag "uploaded" <i>"'.$this->name().'"</i> for '.$this->ap->name());
    $this->_init();
  }

  function set_uploaded($date,$verbose=True){
    if (empty($date)) $date = time();
    if ($date != $this->isUploaded()){
      myPear_db()->qquery("UPDATE jam_documents SET doc_date=$date,timestamp='".b_time::sql_timestamp($date)."' WHERE doc_id = '$this->ID'",cnf_show_db||self::$_debug);
      $this->_init();
      $msg = b_time::short_date($date).' upload '.$this->name().' for '.$this->ap->name();
      if ($date+3600 < time()) $verbose = False; // Do not report recovered files
      if ($verbose) myPear::MESSAGE($msg);
      else          myPear::WARNING_T($msg);
    }
  }
  
  function isUploaded(){
    $reply = @$this->formDB['doc_date'];
    $this->dbg($this->name().' '.($reply ? b_time::short_date($reply) : ' NO... '));
    return $reply;
  }

  function name() {
    $reply = $this->getValue('doc_name'); // same as 'lm_key'
    return $reply; 
  }

  function uploadedFile_path(){
    if ($reply = $this->ap->isReady()){
      if ($reply = $this->ap->uploadedFile_path($this->filename(),True)){
	$this->set_uploaded($this->ap->timestamp);
      }
    }
    $this->dbg($reply);
    return $reply;
  }

  function filename($include_extension=False){
    $reply = $this->getValue('doc_file'); // same as 'lm_value'      
    if ($include_extension && !preg_match('/\.pdf$/i',$reply)) $reply .= '.pdf';
    $this->dbg($reply);
    return $reply; 
  }

  function getValue($name){
    return @$this->formDB[$name];
  }

  function date(){
    if ($reply = $this->ap->isReady()){
      if (!($reply = $this->isUploaded())){
	$reply = b_time::txt2unix($this->ap->getValue('ap_submitdate',1));
      }
    }
    return $reply;
  }

  /*
   * Check is there any document of this type has being uploaded.
   */
  static function is_inUse($lm_id){
    $q=myPear_db()->qquery("SELECT doc_apid FROM jam_documents WHERE doc_lmid = $lm_id",cnf_dev);
    return myPear_db()->num_rows($q);
  }

  /* 
   * Get list of all documents ever supplied by the avatar 
   * (both as an applicant and as a referee)
   */
  public static function avatar_docs($av_id){
    $debug = False;
    if (is_object($av_id)){
      $av    = $av_id;
      $av_id = $av->ID;
    }

    /*
     * First as an applicant
     */
    $q=myPear_db()->qquery("SELECT ap_id,lm_id  FROM jam_documents ".
			   " LEFT JOIN zzz_list_members ON lm_id=doc_lmid ".
			   " LEFT JOIN jam_applicants   ON ap_id=doc_apid ".
			   " LEFT JOIN zzz_avatars      ON av_id=ap_avid ".
			   " WHERE av_id = '$av_id' ORDER BY ap_submitdate DESC",$debug);
    while($r = myPear_db()->next_record($q)){
      $doc = new jam_Document($r['ap_id'], $r['lm_id']);
      $docs[$doc->name()][$doc->ap->pos->name()][$doc->date()] = $doc->uploadedFile_path();
    }
    
    /*
     * Second as a referee
     */
    $q=myPear_db()->qquery("SELECT rec_id  FROM jam_recommendations ".
			   " LEFT JOIN zzz_avatars ON av_id=rec_avid ".
			   " WHERE av_id = '$av_id' ORDER BY rec_askdate DESC",$debug);
    
    locateAndInclude('bForm_jam_Recommendation');
    while($r = myPear_db()->next_record($q)){
      $rec = new bForm_jam_Recommendation($r['rec_id']);
      $docs['Recommendations'][$rec->name()][$rec->date()][] = $rec->uploadedFile_path();
      $rec->__clean_cache();
    }

    if (empty($docs)){
      if (cnf_dev) myPear::DEBUG("No documents supplied by av_id='$av'");
    }else{
      ksort($docs);
      print x('h3','Uploaded files');
      locateAndInclude('b_tabs');
      $b_tabs = new b_tabs(array_keys($docs));
      foreach ($docs as $tab=>$data){      
	if (!$b_tabs->body())  continue;
	$t = new b_table_zebra(array('pos'=>'Position','date'=>'Date','f'=>' '));
	$t->showLineCounter = False;
	$t->noSort = True;
	foreach($data as $pos=>$a){
	  foreach ($a as $date=>$file){
	    $t->prt(array('pos'=>$pos,'date'=>b_time::short_date($date),'f'=>$file[0]));
	  }
	}
	$t->close();
      }
      $b_tabs->close();
    }
  }
}
