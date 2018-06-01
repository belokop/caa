<?php
/*
 * Define the access control list
 *
 * Hopefully everything here is self-explained and intuitive enough
 */

locateAndInclude('APIaccess');
class APIaccess_jam  extends  APIaccess{

  private $rates_jam = array('JAM_manager_here'  => array('rank'=> 'RANK_jam_manager',
							  'def' => 'superUser_here'),
			     'JAM_posadmin_here' => array('rank'=> 'RANK_jam_posadmin',
							  'def' => 'JAM_manager_here'),
			     'JAM_SC1_here'      => array('rank'=> 'RANK_jam_SC1',
							  'def' => 'JAM_posadmin_here'),
			     'JAM_SC2_here'      => array('rank'=> 'RANK_jam_SC2',
							  'def' => 'JAM_SC1_here'),
			     'JAM_observer_here' => array('rank'=> 'RANK_jam_observer',
							  'def' => 'JAM_SC2_here'),
			     'JAM_secr_here'     => array('rank'=> 'RANK_jam_secr'),
			     'JAM_referee_here'  => array('rank'=> 'RANK_jam_referee'),
			     'JAM_applicant_here'=> array('rank'=> 'RANK_jam_applicant'),
			     );

  public function __construct(){
    bForm_Avatar::$bForm_Avatar = 'bForm_Avatar_jam';
    locateAndInclude('bForm_Avatar_jam');
    locateAndInclude('jam_updates.inc');
    parent::__construct();
    $this->who_is_here($this->rates_jam);
    // Referee & Applicants can't see other applicants & referees
    $this->who_is_here(array('JAM_member_here'=>array('IF'=>JAM_manager_here || JAM_posadmin_here || JAM_SC1_here || JAM_SC2_here || JAM_observer_here || JAM_secr_here)));
  }

  function isManager(){
    return JAM_manager_here;
  }
  
  /*
   * Do not authorize applicants&referees to scan the database
   */
  public function authorized(){
    if ($reply = parent::authorized()){
      $reply = (JAM_referee_here || JAM_secr_here || JAM_observer_here);
    }
    b_debug::xxx($reply);
    return $reply;
  }

  /*
   * Get the rank of the currently logged in user
   */
  function getRank($av_id=Null){
    if (empty($av_id)) $av_id = bAuth::$av->ID;

    $reply = parent::getRank($av_id);
    if ($reply == RANK__authenticated){
      foreach ($this->getRanks_matrix('ranks') as $pos_id=>$r){ 
	if    (!JAM_posID())            $reply = max($reply,$r);
	elseif( JAM_posID() == $pos_id) $reply = $r;
      }
    }
    return $reply;
  }

  /*
   * Everyone may read his own reviews
   */
  //  function may($av_id=Null,$rank){}
  public function may_seeApplicantInfo()     { return $this->may_seeReviews(); }
  
  public function may_seePrivateReviews()    { return (JAM_posadmin_here  or bForm::$inputToPDF); }
  
  public function may_seeReviews()           { return (JAM_observer_here  or JAM_secr_here or bForm::$inputToPDF); }

  public function may_sendMails($id='')      { return True; return (JAM_secr_here || JAM_posadmin_here); }
  
  public function mayRemindApplicants()      { return (JAM_secr_here || JAM_SC1_here); }
  
  public function may_writeReviews($id='')      { return (JAM_SC1_here || JAM_SC2_here); }
  
  public function may_seeRevokedApplications(){ return (cnf_show_revoked && JAM_manager_here); }
  
  /*
   * Show "the access matrix" (who may do what)
   */
  public function show(){
    return;
  }

  /*
   * Select the position to manage.
   * JAM always works within a defined environment:
   * - organization (derives from the position)
   * - position (derives from the research field), if known
   * - research field (if known)
   */
  function selectPosition() {

    // Get list of relevant positions
    $names = $this->getRanks_matrix('names');

    if (count($names)==1){
      foreach($names as $pos_id=>$pos_name){
	JAM_posID($pos_id);
	JAM_pos();
      }
    }elseif (count($names)){ 
      ob_start();
      JAM_posID();
      if ($id = @$_POST['JAM_posID'])  JAM_pos($id);
      
      $f = new bForm('new','RW',False);
      $f->defineVariable(' ','JAM_posID');
      $f->formDB['JAM_posID'] = (empty($_POST['JAM_posID']) ? JAM_posID() : $_POST['JAM_posID']);
      $f->action = '';
      $f->submitOnChange['JAM_posID'] = True;
      $f->getFormOpen();
      $f->formBlock('',
		    'You manage multiple positions, please choose the one you want to work with:',
		    array('JAM_posID'=>array('radioButton',$names,Null,False)));
      $f->closeForm('confirm');   print "<br/><br/>";
      $formDB = ob_get_contents();
      ob_end_clean();
      MSG::MESSAGE($formDB);
    }
  }

  /*
   * The same person might have different access level for various positions, i.e. 
   * being an applicant for one position and selection commettee member for another, etc.
   *
   * Get the "access matrix" for the currently logged-in person 
   * and keep it in cache
   */
  private function getRanks_matrix($what){
    
    bForm_Avatar::set_context(bAuth::$av,JAM_MODULE);
    $av_id = bAuth::$av->ID;
    
    if (1||empty($_SESSION[myPear_cache][__METHOD__])){
      $_SESSION[myPear_cache][__METHOD__] = array();

      // Get all the position for the Organization
      locateAndInclude('bForm_jam_Position','fatal');
      $org_positions = bForm_jam_Position::getPositions(myOrg_ID);
      
      // Check for the managers to be logged in
      $organizational_duty = myPear::org_duty($av_id,array(RANK__superuser,
							   RANK_jam_manager));

      // Walk thru the positions and see which ones might be shown       
      locateAndInclude('bForm_jam_Applicant','fatal');
      locateAndInclude('bForm_jam_Recommendation','fatal');
      foreach($org_positions as $pos_id=>$formDB){
	// Skip the placeholders
	if (! bForm_jam_Position::_managedByJAM($formDB)) continue;

	if ($organizational_duty){
	  $rank = $organizational_duty;
	}else{
	  // Check that the user is referee and/applicant
	  if (!isset($_isReferee)){
	    $_isReferee = bForm_jam_Recommendation::_isReferee($av_id);
	    $_isApplicant = bForm_jam_Applicant::_isApplicant($av_id);
	  }
	  
	  if ($rank = myPear::org_duty($av_id,
				       array(RANK_jam_posadmin,
					     RANK_jam_SC1,
					     RANK_jam_SC2,
					     RANK_jam_observer,
					     RANK_jam_secr),
				       $pos_id)){
	    // Skip the archived positions, but not always...
	    if (bForm_jam::__isStatus(JAM_pos_archived,$formDB,'pos_status')) continue;
	  }elseif (in_array($pos_id,$_isReferee)){
	    $rank = RANK_jam_referee;
	  }elseif (in_array($pos_id,$_isApplicant)){
	    $rank = RANK_jam_applicant;
	  }else{
	    continue;
	  }
	  // The archived positions might be intersing for the applicants & referees
	  //?????	if (bForm_jam::__isStatus(JAM_pos_archived,$formDB,'pos_status')) continue;
	}
	$_SESSION[myPear_cache][__METHOD__]['names'][$pos_id] = $formDB['pos_name'];
	$_SESSION[myPear_cache][__METHOD__]['ranks'][$pos_id] = $rank;
      }
      if (empty($_SESSION[myPear_cache][__METHOD__]))$_SESSION[myPear_cache][__METHOD__] = array('names' => array(),
												 'ranks' => array());
    }
    return $_SESSION[myPear_cache][__METHOD__][$what];
  }
}
