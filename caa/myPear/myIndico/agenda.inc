<?php
/* 
 * Get UTF-8 encoded conferens data.                            YB 2009-02-26
 */
class agenda {
  
  // Default indico server
  protected $agenda_url = 'http://agenda.albanova.se/';

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
    if (!empty($url)) $this->agenda_url = $url;
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
  function getCategoryInfo($category='270',$args='') {
    if ($this->debug) print "\n================================= \$agenda->getCategoryInfo('$category','$args')\n\n";
    $xml   = simplexml_load_string(`curl http://bgenda.albanova.se/tools/getConf.py?catinfo=270`);
				   //
				   $json  = json_encode($xml);
				   $array = json_decode($json,TRUE);
				   return $array;
  }

  /*
   *
   */
  function getCategory($category='270',$args='') {
    if ($this->debug) print "\n================================= \$agenda->getCategory('$category','$args')\n\n";
    return $this->parser("cat=$category&$args");
  }
  
  /*
   *
   */
  function getConference($conference='',$args='') {
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
   if (defined('cnf_CLI') && !cnf_CLI)  set_time_limit(0);
   $structues = 'registrant|contribution';
   $reply  = $bufer = array();
   $lastKey= 'No key';
   $url = $this->agenda_url.$this->tools_url.$args;
   foreach (preg_split("/\n/",$this->curl($url)) as $line) {
     if (strstr($line,'=')){
       list($key,$value) = explode('=',$line,2); $key = mb_strToLower($key);
       if ($value && $key) {
	 $key = preg_replace('/_date$/'  ,'',$key);
	 //$key = preg_replace("/_[0-9]*$/",'',$key);
	 if ($key == 'start'){
	   $ID = $value; 
	   $nChairs = 0;
	 }
	  if (preg_match("/^($structues)$/i",$key,$r)) {
	    if ($bufer)                 $reply[$ID][$lastKey][] = $bufer;
	    $bufer = array();
	    $lastKey = $r[1];
	  }elseif (preg_match("/^($structues)_/i",$key,$r)) {
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
    return $page;
  }

  function __toString() { return get_class($this); }

}
