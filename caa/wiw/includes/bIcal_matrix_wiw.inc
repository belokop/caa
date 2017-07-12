<?php
locateAndInclude('bUnit');
locateAndInclude('bForm_wiw_trip');
locateAndInclude('bIcal_matrix');
class bIcal_matrix_wiw extends bIcal_matrix{
  
  var $see_day9 = True;

  public function __construct($date=NULL,$nDays=NULL){
    parent::__construct();
  }
  
  /*
   * 
   */
  function show($arg=0){    

    //
    // Select the travellers frm the current organization
    //
    $where = array("t_departure <= $this->day9",
		   "t_arrival   >= $this->day1",
		   "u_parent = ". myOrg_ID,
		   "u_rank   = ". RANK__ea_member,
		   "u_class  = 'bUnit_ea_members'",
		   );
  
    //
    // Apply the visibility
    //
    if (!bForm_wiw_trip::_trips_visible_to_everyone()) $where[] = sprintf("t_acl = '%s'",wiw_acl_public);

    $q = myPear_db()->qquery("SELECT av_id,av_firstname,av_lastname,t_id,t_orgid,t_avid,t_arrival,t_departure,t_destination,t_contact,t_comment,lm_key,lm_value ".
			     " FROM zzz_avatars ".
			     " LEFT JOIN wiw_trips        ON av_id = t_avid".
			     " LEFT JOIN zzz_unit_members ON av_id  = um_avid".
			     " LEFT JOIN zzz_units        ON u_id   = um_uid".
			     " LEFT JOIN zzz_list_members ON t_type = lm_id".
			     " WHERE ".join(' AND ',$where).
			     " GROUP BY t_id ".
			     myPear_db()->orderBy('av_lastname'),
			     cnf_dev);
    
    $trips = array();
    $n_trip_exists = myPear_db()->num_rows($q);
    while($r = myPear_db()->next_record($q)){

      // Leave just the first option of the trip types to limit the page width
      $lm_key = explode(',',@$r['lm_key']);
      $r['lm_key'] = $lm_key[0];
      if (empty($r['t_destination'])) $r['t_destination'] = @$r['lm_key'];
      
      $nDays= (int)((min($this->day9,$r['t_arrival'])-max($this->day1,$r['t_departure']))/86400);
      $dest = b_fmt::strcut($r['t_destination'],2*$nDays,'...',$nDays>3);
      
      if (WIW::has_rightToModifyTrips($r['av_id']))$url = "<a href='".b_url::same("?auth=no&form=wiw_trip&t_avid=$r[av_id]&id=$r[t_id]")."'>$dest</a>";
      else                                         $url = $dest;
      
      $trips[$r['av_id']][] = array('day1' => $r['t_departure'],
				    'day9' => $r['t_arrival'],
				    'text' => $url,
				    'clas' => $this->getCSS($r['lm_key'],$r['lm_value']),
				    'icon' => bForm_Avatar::_getThumbnail($r),
				    'tips' => nbsp(join('<br />',array(trim(x('strong',$r['t_destination'])),x('i',$r['t_contact']),$r['t_comment']))),
				    'name' => (1 ? mb_strToUpper($r['av_lastname']).' '.$r['av_firstname']
					       : $r['av_firstname'].' '.$r['av_lastname']),
				    );
    }

    // Show the selected entries
    if (empty($trips)){
      myPear::INFO(EMPTY_SELECTION,'Thank you for your interest in '.myOrg_name);
    // bAuth()->loginPrompt();
  }else{
      parent::show($trips);
    }
  }
  
  protected function IO_tip($time,$text1='in',$text2='out'){
    return nbsp(formatTime($time).': from '.strip_tags($text1).' / to '.strip_tags($text2));
  }
}
