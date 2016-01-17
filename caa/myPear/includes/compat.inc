<?php
$_GET['q'] = 'vm';
if (!isset($_SESSION)) session_start();
echo '<pre>';
ini_set("memory_limit",-1);
print "memory is set<br/>\n";
require_once '../myPear/config.inc';

renameColumnsToNewNames();

print __FILE__. "done<br/>\n";

function defineNewColumnNames(){
  $mods = array('jam_applicants'  =>array('ap_posid_fixed'=>'ap_posid',
                                          'ap_country_optional'=>'ap_residentship',
                                          'ap_office_phone'=>'ap_phone',
                                          'ap_rating_RO'=>'ap_rating',
                                          'ap_submitdate_RO'=>'ap_submitdate',
                                          'ap_research_plan_file'    =>'ap_rp_file',
                                          'ap_publication_list_file' =>'ap_pl_file',
                                          'ap_employment_record_file'=>'ap_er_file',
                                          'ap_comment',),
                'jam_positions'   =>array('pos_refnumber',
                                          'pos_req_emp_record'=>'pos_req_er',
                                          'pos_suspended'=>'pos_status',
                                          'pos_bcc',
                                          'pos_fields_cm',
                                          'pos_desc',
                                          'pos_url',
                                          'pos_n_referees_max',
                                          ),
                'jam_recommendations' =>array('rec_comment',
                                              'rec_askdate_RO'=>'rec_askdate',
                                              'rec_submitdate_RO'=>'rec_submitdate',),
                'jam_reviews'         =>array('r_optional_comment'=>'r_comment',
                                              'r_optional_file'   =>'r_file',),
                'abs_visits'          =>array('v_actual_expenses',
                                              'v_owner' =>'v_owner_avid',
                                              'v_host'  =>'v_host_avid',
                                              'v_office_optional'=>'v_oid',
                                              'v_chance',
                                              'v_suggested_by',
                                              'v_lecture_title',
                                              'v_comment',
                                              'v_who_is_interested',),
                'zzz_avatars'        => array('av_salutation',
                                              'av_phone',
                                              'av_fax',
                                              'av_address',
                                              'av_country_optional'=> 'av_residentship',
                                              'av_http',
                                              'av_photo'    =>'av_photo',
                                              'av_identity_RO' =>'av_identity',
                                              'av_lastlogin_RO'=>'av_lastlogin',),
                'abs_events'         =>array('e_pwd2' =>'e_pay_PD',
                                             'e_admin'=>'e_admin_avid'),
                'zzz_bugs'           =>array('bug_email'),
                'zzz_flavors'        =>array('f_headerclass'),
                'jam_sc'             =>array('sc2_id'=>'sc_id',
                                             'sc2_posid'=>'sc_posid',
                                             'sc2_timestamp'=>'sc_timestamp'));
  
  $mods['old_flavors'] = $mods['zzz_flavors'];
  $mods['old_visits']  = $mods['abs_visits'];
  $mods['old_avatars'] = $mods['jam_old_avatars'] = $mods['zzz_avatars'];
  foreach ($mods as $tab=>$a){
    if (preg_match('/jam_/',$tab))      $mods[str_replace('jam_','jam_old_',$tab)] = $mods[$tab];
  }
  return $mods;
}

function renameColumnsToNewNames(){
    $mods = defineNewColumnNames();
    foreach ($mods as $table=>$data) compat_table($table,$data);
    
    modifyTimestamFields();
}

function modifyTimestamFields(){
    myPear_db()->reset_cache();
    $db = myPear_db();
    foreach ($db->getTables() as $table){
        foreach ($db->getColumns($table) as $c){
            $type = myPear_db()->getColumnType($table,$c,True);
            if ($type=='timestamp') $db->query("ALTER TABLE $table CHANGE $c $c timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP");
        }
    }
}

function compat_table($table,$fields){
    myPear_db()->reset_cache();
    foreach ($fields as $fFrom=>$f){
        if (preg_match('/^([0-9]*)$/',$fFrom)) $fFrom = $f.'_optional';
        if (myPear_db()->columnExists($fFrom,$table)){
            $type = myPear_db()->getColumnType($table,$fFrom,True);
            $cmd  = "ALTER TABLE $table CHANGE  $fFrom $f $type";
            //myPear::WARNING_T($cmd);
            myPear_db()->query($cmd);
            myPear_db()->reset_cache();
        }
    }
}
