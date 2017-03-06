<?php
/*
 * JAM units                      YB 2008-2012
 *
 * JAM units are build around the "access level"
 *
 */
locateAndInclude('bUnit');
locateAndInclude('bForm_jam_Review');
class bUnit_jam extends bUnit{

  var $pos   = Null;
  var $debug = False;

  /* 
   * Get all JAM units for the given position
   */
  public static function get_units($ranks,$title){
    // Build list of sub-committees                                                                                                                                                          
    $sc = array();
    foreach($ranks as $rank)         $names[] =  JAM_unit($rank)->name();
    
    // Show tabs with all the sub-committees                                                                                                                                                 
    print x('h2',$title);
    locateAndInclude('b_tabs');
    $tabs = new b_tabs($names);
    foreach($ranks as $rank){
      if ($tabs->body()) JAM_unit($rank)->getForm();
    }
    $tabs->close();
  }

  /*
   * Constructor
   */  
  function __construct($pos_id,$rank){

    // First hook the position
    if (empty($pos_id)) $pos_id = JAM_posID();
    $this->pos = myPear::getInstance('bForm_jam_Position',$pos_id,array('strict','fatal'));
    if (empty($this->pos)) b_debug::internalError("empty pos_id. Can't be");

    // Then appeal to the parents
    parent::__construct(@$this->pos->ID,$rank);
    
    $this->subtitle = x('blockquote',x('strong',$this->pos->name()));
    
    // sanity
    if (($rank == RANK_jam_SC1) || ($rank == RANK_jam_SC2)){
      myPear_db()->qquery("UPDATE zzz_unit_members SET um_status = '".JAM::status(JAM_review_public)."' WHERE ".
			  "(um_status IS NULL OR um_status != '".JAM::status(JAM_review_private)."') AND  um_uid = $this->ID ",cnf_show_db);
    }
    // b_debug::xxx($this->pos.'='.$this->pos->name().' bUnit_jam='.$this->name());
  }
  
  /*
   * Add specific items 
   */
  protected function membersInfo_toSelect($extended=False){
    return array_merge(parent::membersInfo_toSelect($extended),
                       array('av_institute',
                             'av_position',
			     ));
  }

  /*
   *
   */
  function getForm(){
    if ($av_id = myPear::_GET_and_unset('toggle')){
      $status = self::_getStatus($this->getUnitMember($av_id));
      $toggle = ($status==JAM::status(JAM_review_private) 
		 ?        JAM::status(JAM_review_public) 
		 :        JAM::status(JAM_review_private));
      $av = myPear::getInstance('bForm_Avatar_jam',$av_id);
      $this->add_unitMember($av,$verbose=False,$um_status=$toggle);
      $_GET['action'] = 'show';
      myPear::MESSAGE($av->name()."'s ".JAM::status($toggle,'d'));
    }
    parent::getForm();
 }
  
  /*
   * The member is not deletable if the is a review written by him
   */
  protected function isUnitMemberDeletable(){
    if ($reply = parent::isUnitMemberDeletable()){
      $reply = (bForm_jam_Review::getReviews('count',$this->rec['av_id'])==0);
    }
    $this->dbg($reply);
    return $reply;
  }

  function isWritable(){
    //    if ($this->MODE == 'RO') return False;
    $reply = JAM_posadmin_here || parent::isWritable();
    $this->dbg($reply);
    return $reply; 
  }

  /*
   *
   */
  protected function header($arg=Null){
    $header = parent::header();
    if ($this->rank==RANK_jam_SC1 || 
	$this->rank==RANK_jam_SC2){
      if (bForm_jam_Review::getReviews('count') > 0){
	$header['n_reviews'] = 'reviews written';
      }      
    }
    return $header;
  }
  protected function show_set_css() {
    $this->t->th_attr['n_reviews'] = "class='align_right'";
  }

  /*
   *
   */
  public static function _getStatus($rec){
    $reply = @$rec['um_status'];
    return $reply; 
  }

  /*
   *
   */
  protected function showStatus($rec=array()){
    if ($this->rank==RANK_jam_SC1 || 
	$this->rank==RANK_jam_SC2){
      $status = self::_getStatus($rec);
      $av = myPear::getInstance('bForm_Avatar_jam',$rec['av_id'],'strict');
      $button = ($av->isWritable()
		 ? array('X'=>1,
			 'l'=>b_url::same("?toggle=$rec[av_id]"),
			 'i'=>JAM::status($status,'i'),
			 'd'=>JAM::status($status,'d').' - TOGGLE')
		 : array('d'=>JAM::status($status,'d'),
			 'i'=>JAM::status($status,'i'),
			 ));
      $reply = bIcons()->getButton($button,$status);
      return $reply;
    }else{
      return parent::showStatus();
    }
  }

  /*
   *
   */
  protected function format(){
    $this->rec['n_reviews'] = x('strong',bForm_jam_Review::getReviews('count',$this->rec['av_id']));
  }
  
  function __toString(){
    $str[] = $this->pos->nickname();
    $str[] = $this->getValue('l_name');
    return get_class($this).' ('.join(',',$str).')';
  }
}
