<?php
define('HW_VERSION','1.0.8');
$releaseDate = '2017-03-06';

if (class_exists('myPear',0)) b_reg::add_module('hw',
						array('v'=>HW_VERSION,
						      'd'=>'Hello World',
						      'r'=>$releaseDate,
						      'i'=>'i-tips'));

