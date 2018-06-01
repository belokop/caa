<?php
locateAndInclude('bToolBox');

class jamToolBox extends bToolBox{
  
  function __construct(){
    
    $this->tools['JAM']=array(
			      'FV'=>array('t'=>'Locate applicants for the current position in other JAM campaigns',
					  'd'=>(x('p','As stated...'))
					  ),
			      'CS'=>array('t'=>'Check the JAM database for corrupted data',
					  'd'=>(x('p','As stated...').
						x('p','Sometimes things get corrupt due to wrong input, bugs, etc...'))
					  ),
			      'DA'=>array('t'=>'Search for duplicated applications',
					  'd'=>(x('p','It happens that applicants submit duplicated applications'))
					  ),
			      'DR'=>array('t'=>'Fix the duplicated referees in recommendation letters '.(JAM_posID() ? ' for '.x('em',JAM_pos()->name()) : ''),
					  'd'=>(x('p','It happens that the applicants mistype the referee e-mails, '.
						  'and later "correct" them by requesting another letter from the same person with the proper E-mail.').
						x('p','When the JAM administrator merges the duplicated AV-records into one, the applicant gets the "proper" referee twice.'))
					  ),
			      'RD'=>array('t'=>'(Re)create dossiers for '.x('em',(JAM_posID() ? JAM_pos()->name() : 'the current position')),
					  'd'=>'<p>Refresh dossiers.</p><p>Note, this might take rather long time...</p>',
					  'condition' => !b_posix::is_empty(JAM_posID()),
					  ),
			      );
    parent::__construct();
  }
  
  /*   
   * Check sanity
   */
  protected function execute_CS(){
    locateAndInclude('bForm_jam_Applicant');
    locateAndInclude('bForm_jam_Recommendation');

    //
    //
    print x('h3','Check the non-completed applications');
    if (b_posix::is_empty(JAM::$bogusApplications)){
      print x('dl','No errors here');
    }else{

      if (False) foreach(JAM::$bogusApplications as $ap_id){
	  $ap = new bForm_jam_Applicant($ap_id);
	  $ap->check_formDB();
	  $ap->__clean_cache();
	}
      
      locateAndInclude('bHolder_jam_Applicant');
      $h = new bHolder_jam_Applicant(JAM::$bogusApplications);
      $h->listEntries();
    }
    
    //
    //
    print x('h3','Checking recommendation letters');
    bForm_jam_Recommendation_sanity();

    //
    //
    print x('h3','Removing dossiers for the revoked applications');
    print x('strong','TBD');
  }

  /*
   * Recreate dossiers
   */
  protected function execute_RD(){
    locateAndInclude('bForm_jam_Applicant');
    $q = myPear_db()->qquery("SELECT ap_id FROM jam_applicants WHERE ".
			       " ap_posid = '".JAM_posID()."' AND ".
			       " ap_status != '".JAM::status(JAM_ap_revoked)."'",1);
    while ($r=myPear_db()->next_record($q)){
      set_time_limit(0);
      $ap = new bForm_jam_Applicant($r['ap_id']);
      $ap->createDossier($silent=False,$forced=True);
      $ap->__clean_cache();
    }
  }
  
  /*
   * Locate "our" applicants in all the JAM positions
   */
  protected function execute_FV(){
    $q = myPear_db()->qquery("SELECT av_lastname,av_firstname FROM zzz_avatars ".
			     " LEFT JOIN jam_applicants ON ap_avid = av_id ".
			     " LEFT JOIN jam_positions ON ap_posid = pos_id ".
			     " WHERE pos_id = ".JAM_posID(),True);
    $applicants = array();
    while($r = myPear_db()->next_record($q)) $applicants[] = sprintf("(av_lastname = %s AND av_firstname = %s)",myPear_db()->quote($r['av_lastname']),myPear_db()->quote($r['av_firstname']));
    
    //    bForm_Avatar::findDuplicates(" WHERE av_lastname REGEXP ".myPear_db()->quote(join('|',$applicants)));
    bForm_Avatar::findDuplicates(" WHERE ".join(' OR ',$applicants));
  }
  
  /*
   * Catch duplicated applications
   */
  protected function execute_DA(){
    
    switch (b_posix::is_empty($id=@$_GET['continue_once'])){
    case False:
      print x('h3',"Dropping recommendation letter record '$id'");
      //      $q = myPear_db()->qquery("DELETE FROM jam_recommendations WHERE rec_id = $id",True);
      print b_btn::link('continue with the cleanup',b_url::same("?resetcache_once=1&tb_act_once=$this->action"));
      break;
      
    case True:

      // Collect the duplicated applications
      $where = array();
      if ($id=JAM_posID()) $where[] = "ap_posid = $id";
      bDB::$_debug=cnf_dev;
      $qq = myPear_db()->qquery("SELECT COUNT(*),ap_posid,ap_id,av_id,av_lastname  FROM jam_applicants ".
				" LEFT JOIN zzz_avatars ON ap_avid = av_id ".
				" LEFT JOIN jam_positions ON ap_posid = pos_id ".
				' WHERE '.join(' AND ',array_merge(array("pos_orgid = ".myOrg_ID),$where)).
				" GROUP BY av_id,ap_posid ".
				" HAVING COUNT(*)>1 ORDER BY ap_id DESC",cnf_dev);
      bDB::$_debug=False;

      // now create list of applications
      $list = array();
      while ($rr=myPear_db()->next_record($qq)){
	$w = $where;
	$w[] = "ap_avid = $rr[av_id]";
	$q = myPear_db()->qquery("SELECT ap_id FROM jam_applicants ".
				 ' WHERE '.join(' AND ',$w).' ORDER BY ap_avid ');
	while ($r=myPear_db()->next_record($q)){
	  $list[] = $r['ap_id'];
	}
      }

      locateAndInclude('bHolder_jam_Applicant');
      $h = new bHolder_jam_Applicant($list);
      $h->listEntries();
    }
  }

  /*
   * Catch duplicated referees
   */
  protected function execute_DR(){
    
    switch (b_posix::is_empty($id=@$_GET['continue_once'])){
    case False:
      print x('h3',"Dropping recommendation letter record '$id'");
      $q = myPear_db()->qquery("DELETE FROM jam_recommendations WHERE rec_id = $id",True);
      print b_btn::link('continue with the cleanup',b_url::same("?resetcache_once=1&tb_act_once=$this->action"));
      break;
      
    case True:
      locateAndInclude('bForm_jam_Recommendation');
      bDB::$_debug=cnf_dev;
      $qq = myPear_db()->qquery("SELECT COUNT(*), rec_apid, rec_avid ".
				  " FROM jam_recommendations ".
				  (($id=JAM_posID()) ? " LEFT JOIN jam_applicants ON rec_apid = ap_id WHERE ap_posid=$id ":'').
				  " GROUP BY rec_avid,rec_apid ".
				  " HAVING COUNT(*)>1 ORDER BY rec_apid DESC",cnf_dev);
      bDB::$_debug=False;
      if (!myPear_db()->num_rows()) MSG::MESSAGE("There is no more duplicated request for the reference letter for this position");
      
      while ($rr=myPear_db()->next_record($qq)){
	$q = myPear_db()->qquery("SELECT * FROM jam_recommendations WHERE rec_apid = $rr[rec_apid] AND rec_avid = $rr[rec_avid] ORDER BY rec_submitdate DESC",False);
	$table = $matrix = array();
	$header = array('what'=>' ',
			'sp0' =>' ',
			);
	$nheader_init = count($header);
	$n = 0;
	while ($r=myPear_db()->next_record($q)){
	  $n++;
	  $rec = new bForm_jam_Recommendation($r['rec_id'],'RO');
	  if (count($header)==$nheader_init){
	    print x('h3',bJS()->modal_ajax("?id=".$rec->ap->ID."&form=jam_Applicant",$rec->name('ar')));
	  }	  
	  
	  foreach(array('rec_letter_file'=>'Letter',
			'rec_submitdate' =>'Date arrived',
			'rec_askdate'    =>'Date requested',
			) as $k=>$descr){
	    if (count($header)==$nheader_init){
	      $table[$k]['what'] = x('strong',$descr);
	      $table[$k]['sp0']  = ' ';
	    }
	    
	    if($k=='rec_letter_file'){
	      if  ($url = $rec->uploadedFile_url()) $url = x("a class='only_online' href='$url'",'[ '.b_t::_('see it').' ]');
	      else $url = b_fmt::redText('missing');
	      $table[$k][$rec->ID] = $url;
	    }elseif (!b_posix::is_empty($v = $rec->getValue($k))){
	      if (strpos($k,'date') === False) $table[$k][$rec->ID] = $v;
	      else                             $table[$k][$rec->ID] = x('tt',b_time::short_date($v));
	    }else{
	      $table[$k][$rec->ID] = b_fmt::redText('missing');
	    }    
	    $table[$k]["sp$n"] = ' ';
	  }    
	  $header[$rec->ID] = $rec->ID;
	  $header["sp$n"] = ' ';
	}
	
	// Add "delete it" to the header
	$ids = array();
	foreach(preg_grep('/^[0-9]*$/',array_values($header)) as $i) $ids[] = $i;
	
	foreach($ids as $n=>$k) if ($n > 0){
	  $header[$k] = b_btn::submit_icon('i-drop',
					   "Remove duplicated recommendation request for ".$rec->name('ar'), 
					   b_url::same("?continue_once=$k&tb_act_once=$this->action"),
					   $confirm=True);
	}
	
	// Show the duplicating(?) recommendations
	$t = new b_table_zebra($header);
	$t->noSort = True;
	$t->showLineCounter = False;
	foreach($header as $k=>$v){
	  if (strpos($k,'sp')===0) $t->class[$k]   = 'class=b_transparent';
	  else                     $t->th_attr[$k] = "style='text-align:right;margin-left:5px;'"; 
	}
	foreach($table as $k=>$prt) $t->prt($prt);
	$t->close();
      }    
    }
  }
}
