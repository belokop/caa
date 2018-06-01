<?php
require_once '../myPear/config.inc';
require_once '../vm/config.inc';
require_once '../myPear/includes/bUnit.inc';


foreach(array(VISIT_TYPE_PROGRAM,
	      VISIT_TYPE_COLLABORATION) as $t) $where[] = "v_type = '$t'";
$q = myPear_db()->query("SELECT av_identity,v_start,v_end,av_lastname,av_email,av_firstname,e_name,e_start  FROM abs_visits ".
			  " LEFT JOIN zzz_avatars ON av_id=v_avid ".
			  " LEFT JOIN abs_events ON e_id=v_eid ".
			  " WHERE ".join(' OR ',$where).
			  " GROUP BY av_id".
			  " ORDER BY av_lastname ASC "
			  );
$table = array();
while ($r = myPear_db()->next_record($q)){
  if (b_posix::is_empty($m = b_fmt::RFC_2822($r['av_email']))){
    //    print $r['av_email'] . " ??? \n";
  }else{
    $r['av_email'] = $m[0];
    if (stripos($r['av_email'],'nordita') !== False) continue;
    if (empty($r['e_name'])) $r['e_name'] = date('Y',$r['v_start'])." visit";
    else                     $r['e_name'] = date('Y',$r['e_start'])." - $r[e_name]";
    foreach(array('av_email',
		  'av_firstname',
		  'av_lastname',
		  'e_name',
		  //'av_identity',
		  ) as $f){
      print str_replace('  ',' ',str_replace(',',' ',$r[$f])).',';
    }
    print "\n";
  }
}
