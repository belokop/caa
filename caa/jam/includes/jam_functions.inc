<?php

/*
 * bList_jam_requiredDocs wrapper
 */
function JAM_requiredDocs($pos_id=0){
  static $instances = array();
  if (empty($pos_id)) $pos_id = JAM_posID();
  if (empty($instances[$pos_id])){
    require_once dirname(__FILE__).'/bList/jam/requiredDocs.inc';
    $instances[$pos_id] = new bList_jam_requiredDocs($pos_id);
  }
  return $instances[$pos_id];
}

/*
 * bList_jam_subPositions wrapper
 */
function JAM_subPositions($pos_id=0){
  static $instances = array();
  if (empty($pos_id)) $pos_id = JAM_posID();
  if (empty($instances[$pos_id])){
    require_once dirname(__FILE__).'/bList/jam/subPositions.inc';
    $instances[$pos_id] = new bList_jam_subPositions($pos_id);
  }
  return $instances[$pos_id];
}

/**
 */
function jam_showBestJob(){
  print bIcons()->get('best-job-in-the-world');
}

/*
 *
 */
function jam_doSomething($id=Null,$action=Null){
  $id = @$_GET['id_once'];
  switch($action=@$_GET['jds_act_once']){
 
  case 'dossier':  // Create dossier
    locateAndInclude('bForm_jam_Applicant');
    $ap = new bForm_jam_Applicant($id);
    $ap->createDossier($silent=False,$forced=True,$debug=cnf_show_pdf);
    print join('&nbsp;',array(b_btn::link('see dossier',$ap->getValue('ap_dossier_url',True)),
			      b_btn::link(CONST_go_back,b_url::getReferrer())));
    break;

  default:
    MSG::WARNING("How to do '$action'?");
  }
}


/*
 * Invoke bForm_jam_Review with non-standard arguments
 */
function jam_giveRating(){
  locateAndInclude('bForm_jam_Review');
  $review = new bForm_jam_Review('r_apid = '.$_GET['r_apid'].' AND r_avid = '.$_GET['r_avid'],
				 'RO',
				 $_GET['r_apid'],
				 $_GET['r_avid']);
  $review->nodrupal = True;
  MSG::WARNING_T('Invoked as widget');
  print $review->getWidget('none');
}

/*
 * Send the reminding mail to the applicant
 */
function jam_mailErrorsInApplication(){
  JAM_mailer()->errorsInApplication($_GET['ap_id']);
}


/*
 * Invoke the Recommendations holder
 */
function jam_listRecommendations($av_id=0){
  if (empty($av_id)) $av_id = $_GET['id'];
  locateAndInclude('bHolder_jam_Recommendation');
  $holder = new bHolder_jam_Recommendation($av_id,@$_GET['sc']);
  $holder->listEntries();
}

/*
 * Send the reminding mail to the referee
 */
function jam_mailRemindRecommendation($rec_id=Null) {
  locateAndInclude('bForm_jam_Recommendation');

  if (empty($rec_id)) $rec_id = @$_GET['id'];
  $rec = new bForm_jam_Recommendation($rec_id);
  
  if (!cnf_inside_fb){
    MSG::H1("Re-sending recommendation letter request");
  }else{
    MSG::H1('empty');
  }
  
  if (@$_GET['sendmail_once'] === 'send'){
    JAM_mailer()->ask_for_recLetter($rec->av,$rec->ap,$no_preview=True);
    $rec->set_requested();
  }elseif ($f = $rec->isReceived()) {
    MSG::MESSAGE(join('<br/>',array(bIcons()->get('32-modify').
				       b_t::_('We have got already the Letter of Recommendation for you from').' '.bForm_Avatar::_fmtName('sfl',$rec->av->formDB).', '.
				       b_t::_('the letter timestamp is').' "'.b_time::medium_date_time($f).'"','',
				       b_t::_("There is no need to remind..."))));
    //  }elseif ((time()-($f=$rec->is_requested())) < 24*3600){
  }elseif (!$rec->is_reminderNeeded()){
    $f = $rec->is_requested();
    MSG::MESSAGE(join('<br/>',array(//bIcons()->get('32-modify').
				       b_t::_('The Letter of Recommendation from').' '.$rec->av->name().' '.b_t::_('has being recently requested,'),
				       b_t::_('the letter timestamp is').' "'.b_time::medium_date_time($f).'"','',
				       b_t::_("There is no need to remind now..."),'')));
  }else{
    JAM_mailer()->ask_for_recLetter($rec->av,$rec->ap,$no_preview=False);
  }
}
