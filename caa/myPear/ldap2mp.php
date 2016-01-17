<?php
$fysikum = array(
                 'Administrativa avdelningen' => 'adm',
                 'AlbaNova Universitetscentrum' => 'an',
                 'Atomfysik' => 'atom',
                 'Avdelningen för fysikalisk kemi' => 'mol',
                 'Avdelningen för oorganisk kemi och strukturkemi' => 'kemio',
                 'Centralverkstaden' => 'verk',
                 'Elementarpartikelfysik' => 'bub',
                 'Fysikalisk systemteknik' => 'sip',
                 'Fysikum' => 'adm',
                 'Fysikum Atomfysik' => 'atom', 
                 'Fysikum Instrumenteringsfysik' => 'sip',
                 'Fysikum Kosmologi Partikelastrofysik och Strängteori' => 'cops',
                 'Institutionen för astronomi' => 'astro',
                 'Institutionen för biokemi och biofysik' => 'mol',
                 'Institutionen för geologiska vetenskaper' =>  'mol',
                 'Institutionen för matematikämnets och naturvetenskap' =>  'mol',
                 'Institutionen för organisk kemi' =>  'kemio',
                 'Instrumenteringsfysik' => 'sip',
                 'Kemisk Fysik' => 'mol',
                 'Kondenserad materia och kvantoptik' => 'kmk',
                 'Kondenserade materiens fysik' => 'kmk',
                 'Kosmologi Partikelastrofysik och Strängteori' => 'cops',
                 'Kosmologi astropartikelfysik  och strängteori' => 'cops',
                 'Kvant- och fältteori' => 'kof',
                 'Kvantkemi' => 'mol',
                 'Kärnfysik' => 'nuc',
                 'Ledningskansliet' => 'adm',
                 'Manne Siegbahnlaboratoriet' => 'msl', 
                 'Medicinsk strålningsfysik' => 'mol',
                 'Meteorologiska institutionen (MISU)' => 'mol', 
                 'Molekylfysik' => 'mol',
                 'Sektionen för arkiv och registratur' => 'adm',
                 'Student' => 'student',
                 'Tekniska avdelningen' => 'tap',
                 );

$nordita = array('aaz',
                 'es547',
                 'wilczek',
                 'abala',
                 'ablac',
                 'ahe',
                 'anne',
                 'anssi',
                 'apva1567',
                 'atba5227',
                 'avane',
                 'awijns',
                 'bama9041',
                 'bgout',
                 'brandenb',
                 'chialva',
                 'dintrans',
                 'divecchi',
                 'eichhorn',
                 'emro1282',
                 'eniko',
                 'fabio',
                 'fpinh',
                 'giesel',
                 'guerrero',
                 'harmark',
                 'helle',
                 'hubbard',
                 'hvzm',
                 'iomsn',
                 'jhertz',
                 'jjy',
                 'joern',
                 'jpjmarti',
                 'jsuor',
                 'koen',
                 'kolausen',
                 'larsam',
                 'larus',
                 'luther',
                 'mama9887',
                 'matsho',
                 'mmode',
                 'molsson',
                 'mottelso',
                 'mreinhardt',
                 'nazh5756',
                 'nbabkovs',
                 'niccolo',
                 'paata',
                 'pethick',
                 'piyalic',
                 'pkapyla',
                 'rni',
                 'rplasson',
                 'sabineh',
                 'sgane',
                 'shofm',
                 'sjabb',
                 'slade',
                 'solveig',
                 'spowe',
                 'sunhede',
                 'ul',
                 'valentina',
                 'viefers',
                 'wikfeldt',
                 'yasser',
                 'yb',
                 'zingg',
                 );
if (!isset($_SESSION)) session_start();
print ('<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />');
echo "<pre>\n";
define('myPear_root_files','/Users/yb/Sites/drupal/sites/default/files/');
$_GET['q'] = 'ea';
$_GET['org'] = 'fysikum';
require_once  '../myPear/config.inc';
$passwd = '/afs/physto.se/common/uadmin/passwd/passwd';
if (!file_exists($passwd)) die("Gde $passwd ???");

$cmd = "grep /afs/su.se/home/ $passwd | awk -F : '{print $1}'";
foreach(explode("\n",trim(`$cmd`)) as $login){
    $av = Null;
    if (empty($login)) continue;
    
    $emals = explode(',',ldap_email($login,False));
    foreach($emals as $e){
        // walk thru all the emails, eventually merge
        if ($a = bForm_Avatar::hook(array('av_email'=>$e))){
            if (empty($av)) print "OK - LDAP $e ".b_fmt::unEscape($a->fmtName('fl'))."\n";
            $av = $a;
        }
    }
    
    if (empty($av))
        if ($av = bForm_Avatar::hook(array('av_identity'=>$login))){
            print "OK - av_identity ".b_fmt::unEscape($av->fmtName('fl'))."\n";
        }
    
    if (empty($av)){
        print "?? $login\n";
        $updates = array('av_identity'=>$login);
        $updates['av_email']  = $emals[0];
        $updates['av_email2'] = join(',',$emals);
        foreach(array(
                      'givenName' => 'av_firstname',
                      'sn'        => 'av_lastname',
                      ) as $i=>$a){
            if ($v = get_ldap($login,$i)){
                if ($i == 'socialSecurityNumber') $v = SSN::fix($v); 
                $updates[$a] = $v;
            }        
        }        
        $av = bForm_Avatar::hook($updates, $doNotCreate=False);
    }        

    // Complete the things
    if (!is_object($av)) continue;

    $clashed = array();    
    foreach(explode(',',($emails=ldap_email($login))) as $e){
        $a = bForm_Avatar::hook(array('av_email'=>$e));
        if (is_object($a) && ($a->ID != $av->ID)) $clashed[] = $e;
    }
    if (empty($clashed)) $av->setEmail($emails);
    else print "?????????????????????????? clashes = ".join(', ',$clashed)."\n";

    $av->setPWD();
    $updates = array();
    foreach(array('title'     => 'av_position',
                  'telephoneNumber'=>'av_phone',
                  'socialSecurityNumber'=>'av_ssn',
                  ) as $i=>$a){
        if ($v = get_ldap($login,$i)){
            if ($i == 'socialSecurityNumber') $v = SSN::fix($v); 
            $updates[$a] = $v;
        }        
    }        
    
    // Summary of the departments
    if (in_array($login,$nordita)){
        $o = 'Nordita';
        $ou = '';
        unset($group);
    }else{
        //$o  = get_ldap($login,'o');
        $o = 'Stockholms universitet';
        $ou = get_ldap($login,'ou');
        $group = @$fysikum[$ou];
        if (empty($group)){
            print "??? no group for $login - ".$av->fmtName('Lf',False)."\n";
            $group = 'fysikum';        
        }
        $groups[$group][] = $login;
        $av->updateDB($updates);
    }
    if    ($o && $ou) $ous[$o][$ou][] = $login;
    elseif($o)        $ous[$o][]  = $login;
    elseif($ou)       $ous[$ou][] = $login;
    //if (((++$ggjhgjhg)%100) == 0) b_debug::print_r($ous,'map');
}

foreach ($groups as $g=>$members){
    $cmd = "maildb ".join(' ',$members)." -set group=$g ";
    print "$cmd\n";
}
foreach(array_keys($ous) as $k) ksort($ous[$k]);
b_debug::print_r($ous,'map');

exit;

function get_ldap($user,$what,$filter=True){
    global $nordita;
    static $exclude = array('scfab','albanova','vetenskapenshus');
    
    $excludes = $exclude;
    if (!in_array($user,$nordita)) $excludes[] = 'nordita';
    if (!$filter) $excludes = array();

    foreach(array('scfab','vetenskapenshus','exams','kurs','physto.su.se',
                  'abuse','backup','kallek','soh1','moa1','master','info','amanda','agenda','setup','support',
                  'prefekt','dire.t.r','postmast','manager','studie','ledare') as $e) $excludes[] = $e;
    $toDrop = join('|',$excludes);

    $cmd = 
        "ldapsearch  -x -H ldap://ldap.su.se '(uid=$user)' $what ".
        " | grep ^$what: ".
        " | perl -MMIME::Base64 -MEncode=decode -n -00 -e 's/\\n+//g;s/(?<=:: )(\S+)/decode(\"UTF-8\",decode_base64(\$1))/eg;print'"
        ;
    $cmd = 
        "ldapsearch  -x -H ldap://ldap.su.se '(uid=$user)' $what ".
        " | grep ^$what:";
    $reply = array();
    foreach(explode("\n",trim(`$cmd`)) as $line){
      if (preg_match("/$toDrop/",$line)) continue;
        $line = trim($line);
        if (empty($line))   continue;
        list($x,$r) = explode(': ',$line,2);
        if (!preg_match('/^.[0-9]*$/',$r) && !preg_match('/^[a-zA-Z]*$/',$r) && preg_match('%^[a-zA-Z0-9/+]*={0,2}$%', $r)){
            $r = base64_decode($r);
        }
        $reply[] = $r;
    }
    $reply = join(',',$reply);
    // if ($reply) print "get_ldap($user,$what) = $reply \n";
    return $reply;
}

function ldap_email($login,$filter=True){
    $ldap_email = array();
    foreach(array('mail',
                  'mailLocalAddress') as $mr){
        $e = get_ldap($login,$mr,$filter);
        if (!empty($e)) $ldap_email = array_merge($ldap_email,explode(',',$e));
    }
    $ldap_email = array_unique($ldap_email);
    sort($ldap_email);
    if (empty($ldap_email) && !$filter) print "????????????????????????????????? no LDAP mail for $login \n";
    if (empty($ldap_email)) $ldap_email[] = "$login@fysik.su.se";
    return strToLower(join(',',$ldap_email));
}
