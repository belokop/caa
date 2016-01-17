#! /usr/bin/php
<?php
    ///////////////////////////////////////////////////////////////////////////////////
    //                                                                               //
    // Usage: /path/collector.php                                                    //
    //                                                                               //
    // The scrint must be invoked with the full path, since the latter is used       //
    // to locate the current working directory.                                      //
    //                                                                               //
    ///////////////////////////////////////////////////////////////////////////////////

    // To be sure...
define('cnf_CLI',True);

if (!isset($_SESSION)) session_start();
error_reporting(0);  

$scriptSrc = dirname(array_shift($argv));

// Important !
$_GET['noFS'] = 1;

// load myPear & lic module
foreach (array('../myPear/config.priv',
	       '../myPear/includes/bDB',
	       '../myPear/config',
	       '../myPear/includes/myPear',
	       '../myPear/includes/b_cache',
	       'config',
	       'includes/lic_functions',
	       'includes/lBasic') as $inc){
  $ff = "$scriptSrc/../$inc.inc";
  require_once $ff;
}

// Collect the licences usage
b_cnf::set('monitorExec',"$scriptSrc/seeLicenseUsage");
lic_fillDB();
myPear_db()->query('COMMIT');
