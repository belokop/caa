<?php
if (!isset($_SESSION)) session_start();
print ('<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />');
echo "<pre>\n";
define('myPear_root_files','/Users/yb/Sites/drupal/sites/default/files/');
$_GET['q'] = 'ea';
$_GET['org'] = 'fysikum';
require_once  '../myPear/config.inc';
$bub = array(
             "soh",
             "apapa",
             "bar",
             "bendtz",
             "cfinl",
             "clement",
             "danning",
             "gbert",
             "henjoh",
             "hkim",
             "hulth",
             "kargel",
             "kej",
             "ker",
             "klas",
             "lundberg",
             "marianjo",
             "mazo7883",
             "milstead",
             "moa",
             "mwolf",
             "nordkvist",
             "olbe4858",
             "ollu1939",
             "pklim",
             "safl7086",
             "seo",
             "simo6587",
             "sjolin",
             "sten",
             "strandberg",
             "tylmad",
             "walck",
             "wcrib",
             "yabul",
             "zhaoyu",
             );

$select = join(' ',$bub);
$cmd = "maildb $select -list name namefam -if cmsep | grep , | grep -v Name";
foreach(explode("\n",`$cmd`) as $line){
  $r = explode(',',$line);
  $user = array_shift($r);
  $maildb[$user] = $r;
} 
// system("maildb $select -set group=bub inst=FKM | grep '>>'");

function get_ldap($user,$what,$exclude="nordita|scfab|albanova|vetenskapenshus"){
    $cmd = "ldapsearch  -x -H ldap://ldap.su.se '(uid=$user)' $what|grep ^$what:";
    $reply = array();
    foreach(explode("\n",trim(`$cmd`)) as $line){
        $line = trim($line);
        if (empty($line))   continue;
        if (preg_match("/$exclude/",$line)) continue;
        list($x,$r) = explode(': ',$line,2);
        $reply[] = $r;
    }
    $reply = join(',',$reply);
    //    if ($reply) print "get_ldap($user,$what) = $reply \n";
    return $reply;
}


foreach($bub as $login){
    $av = Null;
    print "\n";
    foreach(explode(',',ldap_email($login)) as $e){
        if ($a = bForm_Avatar::hook(array('av_email'=>$e))){
            $av = $a;
            print "OK - LDAP ".b_fmt::unEscape($av->fmtName('fl'))." $e\n";
        }
    }
    
    if (!is_object($av)){
        print "??? missing $login \n";
        print "??? missing $login \n";
        print "??? missing $login \n";
        print "??? missing $login \n";
        print "??? missing $login \n";
    } 
}

function ldap_email($login){
    $ldap_email = array();
    foreach(array('mail',
                  'mailLocalAddress') as $mr){
        $ldap_email = array_merge($ldap_email,explode(',',get_ldap($login,$mr)));
    }
    $ldap_email = array_unique($ldap_email);
    sort($ldap_email);
    if (empty($ldap_email)) print "????????????????????????????????? no LDAP mail for $login \n";
    return strToLower(join(',',$ldap_email));
}
