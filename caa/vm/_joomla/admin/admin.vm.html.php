<?php
/**
* @package VMWorld
* @version 1.0
* @copyright Copyright (C) 2005 Open Source Matters. All rights reserved.
* @license http://www.gnu.org/copyleft/gpl.html GNU/GPL, see LICENSE.php
* Joomla! is free software and parts of it may contain or be derived from the
* GNU General Public License or other free or open source software licenses.
* See COPYRIGHT.php for copyright notices and details.
*/

/** ensure this file is being included by a parent file */
defined( '_JEXEC' ) or die( 'Restricted access' );
if (!class_exists('JLog',0)) require_once '/afs/physto.se/common/www/joomla_1.5.20/libraries/joomla/error/log.php';

/**
 * @package VMWorld
 */
class VMscreens {
/**  DEVNOTE: This class will take care of the actual display of pages for the component.
 *  It will be called by code in the admin.vm.php file.
 *  
 * Just a quick, 'global' note. Throughout these functions you will see calls to the static 'JText::_' function. 
 *  This function basically takes a parameter that is a 'description' of a string and will return the appropriate
 *  string for the user's current language. That is to say, by doing things this way, to localise a component you
 *   just have to create a language file for that language / component.

 * There is a language sub-directory in the administrator directory for the administrator components and a 
 * language sub-directory at the Joomla! root for 'front-end' components. 

 * Take for example the call:
 * JText::_( 'Weblink Manager' )

 * in the administrator/com_vm directory. Assuming the user's language is set as 'en-GB', this function will 
 * look in the administrator/language/en-GB directory for a file called 'en-GB.com_vm.ini'. Look in that
 * file and you will see rows of 'X=Y' types strings. In the example call above, 'HELLOWORLD' will be 'VM World' 
 * (the parameter is converted to uppercase first) and the 'Y' value is what will be returned. Hence the example
 * function call will return 'VM World'.

 * That may not seem so special as it stands, but this means that you can change strings fairly radically without
 * having to actually change your code AS WELL AS support different languages without having to change your code,
 * either. All you need do is create the appropriate language file. 
 */
   
	function vm()
	 {
	  //get text string based on languages
	  $ret = JText::_('vm'); 
	  
/** DEVNOTE: Joomla offers system log capabilities.
 *  Let's see what we're gonna see in logs/error.log
 *  This might be extremely usefull for debuging your code
 */ 
    //we will need joomla log system     
	  jimport( 'joomla.utilities.log' );
	  //give me log object
    $log = & JLog::getInstance();
    // write variable int log
    $log->addEntry(array("level" => 0,"status"=> 1, "comment" => "vm :".$ret));
	  //print string
	  echo $ret;
   }

	function helloagain()
	 {
	  //get text string based on languages
	  $ret = JText::_('helloagain'); 
	  
	  //log information
	  jimport( 'joomla.utilities.log' );
    $log = & JLog::getInstance();
    $log->addEntry(array("level" => 0,"status"=> 1, "comment" => "helloagain :".$ret));
	 
	  //print string
	  echo $ret;
   }   
   
	function hellotestfoo()
	 {
	  //get text string based on languages
	  $ret = JText::_('hellotestfoo'); 
	  
	  //log information
	  jimport( 'joomla.utilities.log' );
    $log = & JLog::getInstance();
    $log->addEntry(array("level" => 0,"status"=> 1, "comment" => "hellotestfoo :".$ret));
	 
	  //print string
	  echo $ret;
   }   

	function hellodefault()
	 {
	  //get text string based on languages
	  $ret = JText::_('hellodefault'); 
	  
	  //log information
	  jimport( 'joomla.utilities.log' );
    $log = & JLog::getInstance();
    $log->addEntry(array("level" => 0,"status"=> 1, "comment" => "hellodefault :".$ret));
	 
	  //print string
	  echo $ret;
   }  
       
}
?>
