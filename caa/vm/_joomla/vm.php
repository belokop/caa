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
$doc->addScript('components/com_vm/myPear/js/myPear.js');
$doc->addStyleSheet('components/com_vm/myPear/css/myPear.css');
$doc->addStyleSheet('components/com_vm/myPear/css/bIcal.css');
$doc->addStyleSheet('components/com_vm/myPear/css/b_table.css');

VM::_MENU()->process($arg);
