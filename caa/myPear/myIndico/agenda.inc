<?php
/* 
 * Parse the Indico_v0.9 export output, get UTF-8 encoded data.                         YB 2009-02-26
 *
 */
class agenda {

  // URL of the agenda tools on the server
  public $tools_url  = '/tools/getConf.py?';

  public $debug = False;
  private static $doc = '
Example:
    $agenda = new agenda();
    foreach ($agenda->getConference("355","show_contrib=390,411") as $id=>$entry) 
      print_r($entry);

Usage:
    $a = new agenda();
    (array)$reply = $a->getCategory($categoryId,$arguments);    - all conferences within the category
    (array)$reply = $a->getConference($conferenceId,$arguments);- one conference
 
Supported arguments:
    show_reg=id1,id2 - include registrants data for conferences id1 & id2
    show_reg=all     - include all registrants 
    show_desc=yes    - include the program description
 
    show_contrib=c1,c2 - include contributions c1 & c2
    show_contrib=all   - include all contributions 
 
';

  var $noCache  = False;
  var $nRecords = 0;
  var $url = '';

  function __construct($url=Null){
    if (!defined('AGENDA_URL')) define('AGENDA_URL',$url);
  }

  /*
   *
   */
  function help(){
    return self::$doc;
  }
  
  /*
   *
   */
  function getCategory($category='270',$args='') {
    if ($this->debug) print "\n================================= \$agenda->getCategory('$category','$args')\n\n";
    $reply = $this->parser("cat=$category&$args");
    return $reply;
  }
  
  /*
   *
   */
  function getConference($conference,$args='') {
    if ($this->debug) print "\n================================== \$agenda->getConference('$conference','$args')\n\n";
    return $this->parser("conf=$conference&$args");
  }

  /*
   * Get list of conferences for the Category 
   */
  public function getConferenceIDs($cat){
    $reply = array();
    foreach($this->getCategory($cat) as $conf) $reply[] = $conf['id'];
    return $reply;
  }

  function parser($args){
    $this->dbg();
    if (defined('cnf_CLI') && !cnf_CLI)  set_time_limit(0);
    $structures = 'registrant|contribution';
    $reply  = $bufer = array();
    $lastKey= 'No key';
    $url = AGENDA_URL.$this->tools_url.$args;

    if (class_exists('b_debug',0)) b_debug::_debug($url,cnf_dev);
    foreach (preg_split("/\n/",$this->curl($url)) as $line) {
      if (strstr($line,'=') !== False){
	list($key,$value) = explode('=',$line,2); $key = mb_strToLower($key);
	if ($value && $key) {
	  $key = preg_replace('/_date$/'  ,'',$key);
	  //$key = preg_replace("/_[0-9]*$/",'',$key);
	  if ($key == 'start'){
	    $ID = $value; 
	    $nChairs = 0;
	  }
	  if (preg_match("/^($structures)$/i",$key,$r)) {
	    if ($bufer)                 $reply[$ID][$lastKey][] = $bufer;
	    $bufer = array();
	    $lastKey = $r[1];
	  }elseif (preg_match("/^($structures)_/i",$key,$r)) {
	    if ($key==$r[1].'_structure' && $value=='end'){
	      $reply[$ID][$lastKey][] = $bufer;
	      $bufer = array();
	    }elseif(strstr(mb_strToLower($key),'contribution_speaker_')){
	      $bufer['speaker'] = $value; 
	      $bufer['speakers'][] = $value; 
	    }else{
	      $bufer[str_replace($r[1].'_','',$key)] = $value; 
	    }
	  }elseif ($key == 'chair') { $reply[$ID][$key][] = $value;
	  }elseif (strstr($key,'chair_')){
	    switch($k=str_replace('chair_','',$key)){
	    case "start": 
	      break;
	      
	    case "end":   
	      $nChairs++;
	      break;
	      
	    default:
	      $reply[$ID]['chairs'][$nChairs][$k] = $value;
	    }
	  }else{                      $reply[$ID][$key]   = $value; }
	}
      }
    }
    if ($bufer) $reply[$ID][$lastKey][] = $bufer;
    krsort($reply);
    $this->nRecords = count($reply);
    return $reply;
  }
  
  function curl($url){
    if (function_exists('bTiming')) bTiming()->cpu(__FUNCTION__);
    if (defined('starting'))    $this->dbg(starting);
    if (function_exists ("curl_setopt")) {
      $ch = curl_init();
      curl_setopt($ch, CURLOPT_URL, $url);
      curl_setopt($ch, CURLOPT_HEADER, 0);
      curl_setopt($ch, CURLOPT_AUTOREFERER, True);
      curl_setopt($ch, CURLOPT_FOLLOWLOCATION, True);
      curl_setopt($ch, CURLOPT_AUTOREFERER, True);
      curl_setopt($ch, CURLOPT_FOLLOWLOCATION, True); 
      curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, False);
      curl_setopt($ch, CURLOPT_RETURNTRANSFER, True);
      $page = curl_exec($ch);
      curl_close($ch);
    }else{
      $curl = '/usr/bin/curl';
      $page = "$curl $url";
      $page = `$page 2>/dev/null`;
    }
    $this->dbg(count(explode("\n",$page)).' lines on output');
    if (defined('exiting'))    $this->dbg(exiting);
    if (function_exists('bTiming')) bTiming()->cpu();
    return $page;
  }

  function __toString() { return get_class($this); }

  protected function dbg($o='',$t1='',$t2=''){
    if (class_exists('b_debug',0)){
      if (is_string($o)){
	b_debug::_debug($o,array(3,'fuchsiaText',cnf_dev,$this));
      }else{
	b_debug::_debug($o->name().' '.b_time::period($o->getValue($t1,1),$o->getValue($t2,1)),
			array('T',3, cnf_dev));
      }
    }else{
      print_r($o);
    }
  }
}
