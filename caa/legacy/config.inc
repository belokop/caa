<?php

$releaseDate = '2016-02-25';
define('LEGACY_MODULE'   , 'legacy');
define('LEGACY_VERSION','5.11');

// register the module in myPear registry
b_reg::add_module(LEGACY_MODULE,
		  array('v' =>LEGACY_VERSION,
			'r' =>$releaseDate,
			'c' =>__FILE__,
			'd' =>'', // Empty description, hense not to be explicitely shown
			'tm'=>'', // Empty description, hense not to be explicitely shown
			'i' =>'', // Empty icon, hense no url to the module is  explicitely shown
			));
