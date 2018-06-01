<?php
locateAndInclude('bUnit_jam');
locateAndInclude('bHolder_jam');
locateAndInclude('bForm_jam_Applicant');
locateAndInclude('APIaccess_jam');

class bHolder_jam_Reviews extends bHolder_jam {

  public $debug = False;

  public function __construct(){ // ($ap_id)
    if (empty($ap_id)) $ap_id = @$_GET['ap_id'];
    parent::__construct();
    $this->ap = loader::getInstance('bForm_jam_Applicant',$ap_id,array('strict','fatal'));
    $this->title = '<h2>'.b_t::_('Reviews for').' '.$this->ap->name().'</h2>';
    $this->noSort = True;
    bIcons()->legendTitle = 'Comment';
  }
  
  function header(){
    return array('name'     => 'Reviewer',
		 'r_rating' => 'Rate',
		 'r_comment'=> 'Comment',
		 'r_time'   => 'Date');
  }
  
  function query(){
    bDB::$_debug = self::$_debug;
    $query = myPear_db()->qquery("SELECT *,av_lastname AS name FROM jam_reviews "
				   ." LEFT JOIN zzz_unit_members ON r_avid = um_avid "
				   ." LEFT JOIN zzz_units        ON u_id   = um_uid "
				   ." LEFT JOIN zzz_avatars      ON r_avid = av_id "
				   ." WHERE r_apid =  '".$this->ap->ID."'"
				   ." AND u_parent = '".$this->ap->formDB['ap_posid']."'"
				   ." AND (u_rank = ".RANK_jam_SC1." OR u_rank = ".RANK_jam_SC2." ) "
				   ." GROUP BY r_avid "
				   .myPear_db()->orderBy(),
				   self::$_debug);
    bDB::$_debug = False;
    return $query;
  }
  
  private $i_wrote_a_review = False;
  function format() {
    if (JAM_access()->may_seePrivateReviews($this->rec['r_avid']) || (bUnit_jam::_getStatus($this->rec) == JAM::status(JAM_review_public))){
      $this->rec['r_comment'] = str_replace('-BR-','<br/>',b_fmt::longString($this->rec['r_comment'],60));
      if ((int)$this->rec['r_rating']){
	$this->rec['r_rating'] = x('spawn class="b_important_number"',$this->rec['r_rating']);
      }else{
	$this->rec['r_rating'] = x('spawn style="color:red"','abstein');
      }
    }else{
      $this->rec['r_rating'] = $this->rec['r_comment'] = nbsp("<i>&lt;secret&gt;</i>");
    } 
    $this->rec['name']  = mb_strToUpper($this->rec['av_lastname']).' '.$this->rec['av_firstname'];
    $this->rec['r_time']= b_time::strftime("%Y-%m-%d",$this->rec['r_time']);
    if ($this->rec['r_avid'] == bAuth::$av->ID) $this->i_wrote_a_review = True;
  }

  /*
   *
   */
  function _close(){

    //    if (!$this->ap->pos->isStatus(JAM_pos_archived) && JAM_access()->may_writeReviews()){
    if(1){
      if ($this->i_wrote_a_review) $text = 'update my review';
      else                         $text = 'write a review';      
      print b_btn::UL_buttons(array(b_btn::submit($text,
						  b_url::same(array('function'  =>'jam_giveRating',
								    'r_apid'    =>$this->ap->ID,
								    'r_avid'    =>bAuth::$av->ID,
								    'modal_once'=>'iframe',
								    )))));
    }
  }
}
