<?php
defined( '_JEXEC' ) or die( 'Restricted access' );

foreach (array('',
	       'bForm',
	       'bIcal',
	       'bList',
	       'holder',
	       ) as $d) $GLOBALS['classes_PATH'][] = JPATH_COMPONENT.DS.'myPear'.DS.'includes'.DS.$d;
foreach (array('',
	       'holder',
	       'bForm',
	       'bIcal',
	       'myIndico',
	       'bMailer',
	       'bCover',
	       ) as $d) $GLOBALS['classes_PATH'][] = JPATH_COMPONENT.DS."/includes/$d";

foreach (array(
	       'config.inc', 
	       'includes/b_table.inc',
	       'includes/myPear.inc',
	       'includes/bAuth.inc',
	       ) as $m)   require_once JPATH_COMPONENT.DS.'myPear'.DS.$m;

require_once JPATH_COMPONENT.DS.'config.inc';

$doc =& JFactory::getDocument();
$doc->addScript('components/com_lic/myPear/js/myPear.js');
$doc->addStyleSheet('components/com_lic/myPear/css/myPear.css');
$doc->addStyleSheet('components/com_lic/myPear/css/bIcal.css');
$doc->addStyleSheet('components/com_lic/myPear/css/b_table.css');

locateAndInclude('lic_functions');
locateAndInclude('lBasic');
locateAndInclude('bAuth');
if (bAuth::authenticated()){
  lic_getCategories();
  lic_showDB($arg);
 }else{
  bAuth()->loginPrompt();
 }
