<?php
define('HW_VERSION','1.6');
$releaseDate = '2016-01-19';

if (class_exists('myPear',0)) b_reg::add_module('hw',
						array('v'=>HW_VERSION,
						      'd'=>'Hello World',
						      'r'=>$releaseDate,
						      'i'=>'i-tips'));

