<?php

print "================================= skip ".__FILE__."\n";
exit;

define('myPear_root_files','/Users/yb/Sites/drupal/sites/default/files/');
$GLOBALS['orgs'] = array(
                         'an'     =>array('org_name'    =>'AlbaNova',
                                          'org_domain'  =>'albanova.se',
                                          'c'         =>'ALB'),
                         'fysikum'=>array('org_name'    =>'SU/Fysikum',
                                          'org_domain'  =>'fysik.su.se',
                                          'c'         =>'FKM'),
                         'kth'    =>array('org_name'    =>'KTH',
                                          'org_domain'  =>'kth.se',
                                          'c'         =>'KTH'),
                         'vh'     =>array('org_name'    =>'Vetenskapenshus',
                                          'org_domain'  =>'vetenskapenshus.se',
                                          'c'         =>'VH'),
                         'nordita'=>array('org_name'    =>'Nordita',
                                          'org_domain'  =>'nordita.org',
                                          'c'         =>'NOR'),
                         'okc'    =>array('org_name'    =>'Oskar Klein Centre',
                                          'org_domain'  =>'fysik.su.se',
                                          'c'         =>'?'),
                         'gu'     =>array('org_name'    =>'Göteborgs universitet',
                                          'org_domain'  =>'gu.se',
                                          'c'         =>'?'),
              );
if (empty($argv[1])){
    system("php ldap2mp.php  > ../jam/tmp/ldap2mp.html");
    system("php bub2bub.php  > ../jam/tmp/bub2bub.html");
}

foreach(array_keys($GLOBALS['orgs']) as $org){
    system("php m2m_org.php $org > ../jam/tmp/m2m_$org.html");
}
