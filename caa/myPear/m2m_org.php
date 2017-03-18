<?php
define('myPear_root_files','/Users/yb/Sites/drupal/sites/default/files/');
if (!isset($_SESSION)) session_start();
$_GET['q'] = 'ea';
$_GET['org'] = @$argv[1];
ini_set("memory_limit",-1);

print ('<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />'.
       '<pre>'.
       "<h1>".basename(__FILE__)."($_GET[org])</h1>\n");

/*
 * Convert maildb to myPear
 */
require_once  '../myPear/config.inc';
require_once  '../myPear/includes/myPear.inc';
myPear_db()->db = 'combodb';

require_once  '../myPear/includes/bForm.inc';
require_once  '../myPear/includes/bForm/Avatar/ea.inc';
require_once  '../myPear/includes/bList.inc';
require_once  '../myPear/includes/b_updates.inc';

require_once  '../ea/config.inc';
require_once  '../ea/includes/ea_updates.inc';

set_time_limit(0);

$GLOBALS['? employeeTitle'] = $GLOBALS['e_types'] = $GLOBALS['g_ids'] = array();
$orgs = array(
              'fysikum'=>array('org_name'  =>'SU/Fysikum',
                               'org_domain'=>'fysik.su.se',
                               'c'         =>'FKM'),
              'an'     =>array('org_name'  =>'AlbaNova',
                               'org_domain'=>'albanova.se',
                               'c'         =>'ALB'),
              'kth'    =>array('org_name'  =>'KTH',
                               'org_domain'=>'kth.se',
                               'c'         =>'KTH'),
              'vh'     =>array('org_name'  =>'Vetenskapenshus',
                               'org_domain'=>'vetenskapenshus.se',
                               'c'         =>'VH'),
              'nordita'=>array('org_name'  =>'Nordita',
                               'org_domain'=>'nordita.org',
                               'c'         =>'NOR'),
              'okc'    =>array('org_name'  =>'Oskar Klein Centre',
                               'org_domain'=>'fysik.su.se',
                               'c'         =>'?'),
              'gu'     =>array('org_name'  =>'Göteborgs universitet',
                               'org_domain'=>'gu.se',
                               'c'         =>'?'),
);

function create_orgs($nick){
    global $orgs;

    $org = bForm_Organization::get_myOrg($nick);

    $org_descr = $orgs[$nick];
    $org->updateDB(array('org_code'   => $nick,
                         'org_name'   => $org_descr['org_name'],
                         'org_domain' => $org_descr['org_domain'],
                         ));
    printf("ok %-10s %-20s\n",$nick, myOrg($nick)->name());
}

extract_maildb();

// b_debug::print_r(array_keys($staff));

foreach($orgs as $nn=>$org_descr){
    if (b_cnf::get('org') != $nn) continue;
    if ($nn == 'nordita')         continue;

    create_orgs($nn);
    
    print x('h1','------------------------------------------------------------ '.myOrg($nn)->name())."\n";
    
    if (empty($staff[$org_descr['c']])) $staff[$org_descr['c']] = array();
    foreach($staff[$org_descr['c']] as $user){
        
        $r = array();
        foreach($maildb[$user] as $k=>$v) if ($v=trim($v)) $r[$k] = $v;

        $avs = hook_avatar($r);
        switch(count($avs)){
        case 0:
            print "-- $user\n";
            $av = create_av($r);
            update_av($r, $av);
            break;
            
        case 1:
            $av = create_av($r,$avs[0]);
            $av->setEmail();
            update_av($r,$av);
            break;
            
        case 2:
            if ($nn == 'nordita'){
                $keep = False;
                if (stripos($avs[0]->getEmail(),'nordita') !== False) list($keep,$drop) = array(0,1);
                if (stripos($avs[1]->getEmail(),'nordita') !== False) list($keep,$drop) = array(1,0);
                if ($keep !== False){
                    system("echo 'keep $avs[$keep]' >&2");
                    bForm_Avatar::merge($avs[$drop]->ID,$avs[$keep]->ID);
                    $av = create_av($r,$avs[$keep]);
                    //$av->setEmail("$user@$org_descr[org_domain]"); 
                    $av->setEmail();
                    update_av($r,$av);
                }
            }
            break;
            
        default:
        }
        foreach($avs as $av) $av->__clean_cache();
    }
}

print "<h3>===================================================================".myOrg()->name()." g_ids</h3>\n";
b_debug::print_r($GLOBALS['g_ids'],'g_ids');
foreach($GLOBALS['g_ids'] as $org=>$data){
    foreach($data as $group=>$v){
        if (myOrg_code != 'nordita'){    
            if($group == 'admin') $group = 'adm';
            myOrg()->orgGroups()->updateMember(array('lm_key' =>$group));
        }
    }
}
print "<h3>myOrg()->orgGroups()->get_groups()</h3>\n";
b_debug::print_r(myOrg()->orgGroups()->get_groups(),'myOrg()->orgGroups()->get_groups()');


print "<h3>=================================================================== ".myOrg()->name()." e_types</h3>\n";
b_debug::print_r($GLOBALS['e_types'],'e_types');
foreach($GLOBALS['e_types'] as $e_type){
    if (myOrg_code != 'nordita'){    
        if($e_type == 'admin') $e_type = 'adm';
        myOrg()->orgEmpTypes()->updateMember(array('lm_key' =>$e_type));
    }
}
print "<h3>myOrg()->orgEmpTypes()->get_empTypes()</h3>\n";
b_debug::print_r(myOrg()->orgEmpTypes()->get_empTypes(),'myOrg()->orgEmpTypes()->get_empTypes()');

b_debug::print_r($GLOBALS['? employeeTitle'],'? employeeTitle');


print "======== DONE ========================================= ".__FILE__."<br>\n";
exit;

function copy_values(&$r,$agrs,$inverse=False){
    $av_data = array();
    foreach($agrs as $t=>$f){
        if ($inverse) { $x=$f; $f=$t; $t=$x; }
        if (!myPear::is_empty(@$r[$f])) $av_data[$t] = trim($r[$f]);
        unset($r[$f]);
    }
    return $av_data;
}

function create_av(&$r,$av = Null){
    global $org_descr,$user,$maildb;
    // constract the e-mail address
    if (!empty($r['mailname'])) $av_mail[] = $r['mailname'].'@'.$org_descr['org_domain'];
    $av_mail[] = "$r[user]@".$org_descr['org_domain'];
    if (!empty($r['maildrop'])) $av_mail[] = $r['maildrop'];
    $r['mail'] = strToLower(join(',',$av_mail));
    
    // extract everything from maildb
    $av_data = copy_values($r, array('av_von'      =>'von',
                                     'av_firstname'=>'name',
                                     'av_lastname' =>'namefam',
                                     'av_email'    =>'mail')); 
    foreach (bForm_Avatar::$vitalFields as $f) if (empty($av_data[$f])){
        b_debug::print_r($av_data,'av_data');
        b_debug::print_r($maildb[$user],'maildb');
        b_debug::internalError("empty '$f'");
    }

    bForm_Avatar::$bForm_Avatar = 'bForm_Avatar_ea';
    if ($missing = !is_object($av))  $av = bForm_Avatar_ea::hook($av_data,$dontCreate=True);
    if ($missing = !is_object($av))  $av = bForm_Avatar_ea::hook($av_data,$dontCreate=False);
    bForm_Avatar::set_context($av,EA_MODULE);
    $av->setEmail();
    $av->setPWD();
    if ($missing) echo "-------------------- creating $av ".b_fmt::unEscape($av->name()).' '.$av->getValue('av_email')."\n";
    return $av;
}


function valid_postnummer($postnummer){
    if (empty($postnummer)) $postnummer = '';
    $postnummer = str_replace(' ','',trim($postnummer));
    if (strlen($postnummer) == 5){
        $postnummer = sprintf("%02d %03d",(int)($postnummer/1000), ($postnummer%1000));
    }
    return $postnummer;
}

function valid_user($r,$as_array=False){
    static $pwd_file = array();
    static $dejaVu = array();
    if (empty($pwd_file)){
        $f = '/setup/maintenance/mail/2leif/passwd';
        if (!is_file($f)) die("gde $f???");
        foreach(explode("\n",file_get_contents ($f)) as $line){
            $a = explode(':',trim($line));
            if (!empty($a[0])) $pwd_file[] = strToLower($a[0]);
        }
    }
    foreach(array('user') as $login){
        if (!($login = strToLower(@$r[$login]))) continue;
        if (in_array($login,$pwd_file)) $reply[] = $login; 
    }
    foreach(array('acc_su','acc_kth') as $login){
        if (!($login = strToLower(@$r[$login]))) continue;
        $reply[] = $login; 
    }
    if (empty($reply)){
        $login = $r['user'];
        if (!@$dejaVu[$login]++) print "... Non valid user '$login'\n";
        return ($as_array ? array() : '');
    }else{
        return ($as_array ? $reply : $reply[0]);
    }
}

function update_av(&$r, $av){
    global $org_descr;

    $av->defineVariables();
    
    // extract AV data from maildb
    $r['valid_user']  = valid_user($r);
    $r['postnummer']  = valid_postnummer(@$r['postnummer']);
    
    // extract employment data from maildb
    $lm_option = copy_values($r, array('office'       =>'e_off',
                                       'groupid'      =>'e_gid',
                                       'employeeType' =>'e_rate',
                                       ),True);
    $lm_option['e_type'] = '20_mp';
    $lm_option['e_org']  = $e_org = myOrg_ID;
    $org_code = myOrg()->getValue('org_code',1);
    
    if ($group = @$lm_option['e_gid']){        
        if ($group == 'admin') $group = 'adm';
        if($org_code == 'nordita'){
            $ff = array();
            foreach(explode(',',$group) as $f){
                if ($f == 'BIO') $f = 'CM';
                if ($f == 'fop') $f = 'SA';
                if ($f == 'MA')  $f = 'SA';
                if ($f == 'kof') $f = 'CM';
                if ($f == 'nordita') $f = 'AP';
                if (in_array($f,array('adm','CM','AP','SA','MA'))) $ff[] = $f;
            }
            sort($ff);
            $lm_option['e_gid'] = join(',',array_unique($ff));
        }else{
            foreach(array('kmf'      => 'kof',
                          'fop'      => 'kof',
                          'workshop' => 'verk',
                          'workshope'=> 'verk',
                          'cad/cam'  => 'verk',
                          'qcl'      => 'mol',
                          ) as $f=>$t) if ($group == $f) $group = $t;
            $lm_option['e_gid'] = $group;
        }
        @$GLOBALS['g_ids'][$e_org][$group]++;
    }
    
    static $e_types = array(
                            'ekonom' =>          'adm',
                            'studexp' =>         'adm',
                            'diploma student' => 'students',
                            'phd student' =>     'phd students',
                            'fellow' =>          'post docs',
                            'post doc' =>        'post docs',
                            'guest' =>           'visitors',
                            'datoransvarig' =>   'staff',
                            'emeritus' =>        'staff',
                            'staff' =>           'staff',
                            'tap' =>             'staff',
                            'lector' =>          'staff',
                            'assistant professor'=>'staff',
                            'prefekt' =>         'head',
                            'stf prefekt' =>     'head',
                            'studierektor' =>    'head',
                            'vice director' =>   'head',
                            );
    if (empty($r['employeeTitle']) && ($position = @$r['position'])){
        $position = strToLower($position);
        if (empty($e_types[$position])) $e_types[$position] = '?';
        $r['employeeTitle'] = $e_types[$position];
    }
    if (!empty($r['employeeTitle']) && empty($position)){
        $r['position'] = $r['employeeTitle'];
        $r['employeeTitle'] = $e_types[$r['position']];
        @$GLOBALS['? employeeTitle'][$r['position']]++;
    }
    if (empty($r['employeeTitle'])) $r['employeeTitle'] = '?';
    @$GLOBALS['e_types'][$r['employeeTitle']]++;
    
    $lm_keys   = copy_values($r, array('position'     =>'lm_value',
                                       'employeeTitle'=>'lm_value'),True);
    
        // Period of stay
    if ($s = trim(str_replace(' ','',@$r['norPeriodOfStay']))){
        if (preg_match('/-$/',$s)){
            $s .= date('Y-m-d',b_time::txt2unix(preg_replace('/-$/','',$s))+364*24*3600);
            system("echo $s >&2");
        }
        if (preg_match('/^(\d*-\d*-\d*)-(\d*-\d*-\d*)$/',$s,$m)){
            $lm_keys['lm_key']    = date('Y-m-d',b_time::txt2unix($m[1]));
            $lm_keys['lm_status'] = date('Y-m-d',b_time::txt2unix($m[2]));
        }else{
            print "????????????????????????????? norPeriodOfStay=$s\n";
        }        
    } 

    $stay = 5 * 365 * 24 * 3600;
    if (empty($lm_keys['lm_key']) || empty($lm_keys['lm_status'])){
        if (empty($lm_keys['lm_key']) && ($edt=@$r['edt'])){
            $lm_keys['lm_key']    = date('Y-m-d',b_time::txt2unix($edt));
        }
        if (empty($lm_keys['lm_key']) && ($edt=@$r['edt']) && (stripos($r['status'],'active') !== False)){
            $lm_keys['lm_key']    = date('Y-m-d',($s=b_time::txt2unix($edt)));
            if (stripos($lm['lm_value'],'student')) $lm_keys['lm_status'] = date('Y-m-d',$s+$stay);
            else                                    $lm_keys['lm_status'] = CONST_eternity;
        }
        if (($end=@$r['expiration']) && !@$lm_keys['lm_status']){
            $lm_keys['lm_status'] = date('Y-m-d',b_time::txt2unix($end));
        }
        if (isset($lm_keys['lm_status']) && empty($lm_keys['lm_key'])){
            $lm_key = min(time(),b_time::txt2unix($lm_keys['lm_status'])) - $stay; 
            $lm_keys['lm_key'] = date('Y-m-d',$lm_key);
        }
    }
    if (isset($lm_keys['lm_key'])){
        if (empty($lm_keys['lm_status'])){
            if (stripos(@$lm_keys['lm_value'],'student')) $lm_keys['lm_status'] = date('Y-m-d',b_time::txt2unix($lm_keys['lm_key'])+$stay);
            else                                          $lm_keys['lm_status'] = date('Y-m-d',time());
        }
        unset($r['norPeriodOfStay']);
        unset($r['expiration']);
        unset($r['status']);
        unset($r['edt']);
        $ifEA = (b_time::txt2unix($lm_keys['lm_status']) > time() ? 'e' : 'ea');
        $ff = $lm_keys['lm_key'];
        $tt = $lm_keys['lm_status'];
        $lm_keys['lm_option'] = serialize($lm_option);
    }else{
        $ifEA = (stripos($r['status'],'active') === False ? 'ea' : 'e');
        unset($r['status']);
        $ff = $tt = '          ';
    }

    $compat = array(
                    'norGraduationCity'=>'av_ms_institute',
                    'norHomeInstitute' =>'av_institute',
                    'valid_user'       =>'av_identity',
                    'phone_home'       =>'av_phone',
                    'phone'            =>'av_phone',
                    'phone_mobile'     =>'av_phone',
                    'postnummer'       =>'av_zip',
                    'ortnamn'          =>'av_city',
                    'address'          =>'av_address',
                    'pers_number'      =>'av_ssn',
                    'country'          =>'av_residentship',
                    );
    foreach(array_merge(array('av_birthdate'),
                        array_values($compat)) as $k) $av->defineVariable(array($k=>$k),'only_if_not_defined');
    $av_data = copy_values($r, $compat,True);
    $av->updateDB($av_data);

    // extract the birtdate & retire
    $av_birthdate = $av_retire = Null;
    if ($av_birthdate = $av->getValue('av_birthdate',1,1)){
        $av_retire = b_time::txt2unix(sprintf("%04d-%02d-%02d",date('Y',$av_birthdate)+67,date('m',$av_birthdate),date('d',$av_birthdate)));
    }
    printf("===%3s ==%11s== %10s-%10s ======================================================================== %s\n",$ifEA,@$av_data['av_ssn'],$ff,$tt,b_fmt::unEscape($av->name()));

    // see the non-extracted data for debugging
    foreach(array('edt','su_code','altacct','institution','mailname','maildrop') as $f)    unset($r[$f]);
    
    foreach(array('m_myorg','m_avid','forward','aliases','acc_kth','acc_su') as $af) unset($r[$af]);
    if (!empty($r) && (array_keys($r) !== array('user')))  b_debug::print_r($r,'unused data from maildb');

    myOrg()->eaMembers()->add_unitMember($av,False,$ifEA);
    if(!method_exists($av,'hook_employment')){
        b_debug::print_r(myPear::$sql_identities);
        myPear::$isIdentified_debug = True;
        myPear::isIdentified($av->ID,$current_module_only=True);
        b_debug::internalError("$av has no hook_employment method");  
    }
    if (b_cnf::get('org') == 'fysikum'){
        $employment = $av->hook_employment();
        if ($employment->getMembers(array('e_org'=>myOrg('nordita')->ID))){
            print "SKIP EA records, there is already a Nordita record <br>\n";
        }else{
            $ok = False;
            if (1){
                foreach(array('10_mp','20_mp') as $e_type){
                    if (count($er = $employment->getMembers(array('lm_key'=>$ff,'lm_status'=>$tt,'e_type'=>$e_type)))==1){
                        print "$e_type OK\n";
                        foreach($er as $lm_id=>$rr)        $lm_keys['lm_id'] = $rr['lm_id'];
                        $employment->updateMember($lm_keys);
                        $ok = True;
                    }
                }
            }else{
                foreach($employment->get_currentEmploementRecords(array('e_org'=>myOrg_ID),999,$av->getValue('av_ddate',True)) as $rr){
                    print_r($rr);
                }
            }
            
            
            if (!$ok){
                if ($er = $employment->getMembers(array('e_org'=>myOrg()->ID))){
                    // b_debug::print_r($lm_keys,'already a member');
                    // $employment->dump(True);
                    if (isset($lm_keys['lm_key'])) $employment->updateMember($lm_keys);
                }elseif(!$employment->getMembers() && isset($lm_keys['lm_key'])){
                    $employment->updateMember($lm_keys);
                }else{
                    if (!empty($lm_keys))   b_debug::print_r($lm_keys,"can't match...");
                    $employment->dump(True);
                }
            }
        }
        $employment->debug = 0;
    }
}

function hook_avatar($r){
    global $orgs;
    static $domains = array();
    if (empty($domains)){
        $domains = array('physto.se','mbox.su.se','scfab.se','kth.se');
        foreach($orgs as $nn=>$org_descr)            $domains[] = $org_descr['org_domain'];
        $domains = array_unique($domains);
    }
    
    $reply = $emails = $reply_h = array();
    $av = Null;
    foreach(valid_user($r,True) as $login){
        if ($av = bForm_Avatar::hook(array('av_identity'=>$login),'array')) $reply = array_merge($reply,$av);
        foreach($domains as $d){
            if ($av = bForm_Avatar::hook(array('av_email'=>"$login@d"))) array_merge($reply,$av);
        }
    }
    
    $reply = array_unique($reply);
    if (count($reply) > 1){
        foreach ($reply as $av) print __FUNCTION__."??????????????????????????????????????????????????????? duplicated $av      <br/>\n";
        $av = array_shift($reply);
        $av->defineVariables();
        while(count($reply)){
            bForm_Avatar::merge($reply[0]->ID,$av->ID);
            array_shift($reply);
        }
        $reply = array($av);
    }
    return $reply;

    $av = array_pop($reply);
    if (is_object($av)){
        $toAdd = True;
        foreach($reply as $x) if ($x->ID == $av->ID) $toAdd = False;
        if ($toAdd){
            if ($e=$av->getValue('av_email', 1)) $emails[] = $e; 
            if ($e=$av->getValue('av_email2',1)) $emails[] = $e; 
            $reply[]   = $av;
            $reply_h[] = b_fmt::joinX(',',$hook);
        }
    }

    if (!empty($r['maildrop']))  $emails[] = $r['maildrop'];
    foreach(b_fmt::RFC_2822(join(',',$emails)) as $email){
        if ($av = bForm_Avatar::hook(array('av_email'=>$email), $doNotCreate=True)){
            $toAdd = True;
            foreach($reply as $x) if ($x->ID == $av->ID) $toAdd = False;
            if ($toAdd) {
               $reply[] = $av;
               $reply_h[] = "av_email=$email";
            }
        }
    }

    if (count($reply) > 1)
        foreach($reply as $k=>$av){
            printf ("%s %-10s %7d %-30s %s\n",($k==0?'--':'++'), $r['user'], $av->ID, $reply_h[$k],$av->getValue('av_email2'));
            $r['user'] = '';
        }
    return array_unique($reply);
}

function extract_maildb(){
    global $maildb, $staff,$orgs;

    $db = myPear_db();
    
    $maildb = array();
    $q=myPear_db()->qquery("SELECT * FROM legacy_maildb WHERE LOWER(user) REGEXP '^[a-z\.]*$' AND institution REGEXP 'ALB|VH|FKM|NOR'  ORDER BY user",1);
    //$q=myPear_db()->qquery("SELECT * FROM legacy_maildb WHERE LOWER(user) REGEXP 'borysov' AND institution REGEXP 'ALB|VH|FKM|NOR'  ORDER BY user",1);
    //$q=myPear_db()->qquery("SELECT * FROM legacy_maildb WHERE LOWER(user) REGEXP '^[a-z]*[\.][a-z]*$' AND institution REGEXP 'ALB|VH|FKM|NOR'  ORDER BY user",1);
    while($r=myPear_db()->next_record($q)){
        $login = $r['user'];
        if ($login == 'belokop')   continue;
        if ($login == 'nannazh')   continue;
        if ($login == 'dfitger')   continue;
        if ($login == 'bernade')   continue;
        if ($login == 'hertz')     $login = 'jhertz';
        if ($login == 'mottelson') $login = 'mottelso';
        $r['user'] = $login;
        
        if ($login == 'yb')   unset($r['su_code']);
        
        foreach($r as $k=>$v){
            if     (!trim($v))                unset($r[$k]);
            elseif ( trim($v)== '0000-00-00') unset($r[$k]);
            elseif ( trim($v)== '000000000')  unset($r[$k]);
            else{
                switch($k){
                case 'country': if ($v === 'other')    unset($r[$k]); break;
                case 'altacct': if ($v === $r['user']) unset($r[$k]); break;
                }
            }
        }
        
        foreach (array('name',
                       ) as $k) if (empty($r[$k])) $r = array();
        
        foreach(array(
                      'm_id',
                      'cmt',
                      'last_login',
                      'changeslog',
                      'sukatdate',
                      'changesdate',
                      'created_by',
                      'altname',
                      'timestamp',
                      'rating',
                      'norLocalContactPerson',
                      ) as $k)                unset($r[$k]);
        
        foreach(array('pers_number',
                      'su_code',
                      'maildrop',
                      ) as $k){
            $v = trim(@$r[$k]);
            if ($v === '000000-0000')        $r = array();
            if ($v === 'rm')                 $r = array();
            if ($v === 'F:rm')               $r = array();
            if ($v === ':S')                 $r = array();
            if ($v === 'VMS')                $r = array();
            if (stripos($v,'@vms')!==False)  $r = array();
            if (stripos($v,'nobody')!==False)$r = array();
        }
        
        if (empty($r['status']) && !empty($r['pers_number'])) $r['status'] = 'Passive';
        if (empty($r['status'])) $r = array();
        
        if (!empty($r)) {
            $maildb[$login] = $r;
            foreach($r as $k=>$v) @$stat[$k]++;
            
            foreach($orgs as $nn=>$org_descr){
                if (strpos($r['institution'],$org_descr['c']) !== False){
                    $staff[$org_descr['c']][] = $login;
                }elseif(empty($r['institution'])){
                    $staff['???'][] = $login;
                    b_debug::print_r($r);
                    b_debug::internalError("unknown inst");
                }
            }
        }
    }
    
    arsort($stat,SORT_NUMERIC);
    print x('h2',count($maildb)." users");
    print x('h2',count($stat)." attributes");
    //        print_r($stat);
}

