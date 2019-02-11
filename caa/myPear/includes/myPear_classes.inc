<?php

if (!defined('ROOT_DEBUG')){
  define('ROOT_DEBUG',dirname(dirname(dirname(dirname(__file__)))));
}

define('b_t_long_text',55);
define('b_t_reference','reference');
define('tt_SQL_based',True);

  /*
   * Simple manually maintained translation module
   */
class b_t{
  
  static $_debug = False;
  
  // By default translate to Russian
  static $known_lang_default = 'ru_RU';

  // Backwards compat...
  static $acceptNewEntries, $doTranslation, $dropAfterSKIP;

  //
  static $may_inviteEditTranslation = True;

  /*
   * A better version... Translation might be called before the myPear startup is complited
   */
  static function current_module($filter=".*"){
    // Custom list of valid modules
    if (function_exists('current_module_custom')) $filter = call_user_func('current_module_custom');
    if (!preg_match("/^($filter)/",@$_REQUEST['q'],$match)) $match=array('myPear','myPear');
    $module = $match[1];
    b_debug::_debug($module,self::$_debug);
    return $module;
  }

  /*
   *
   */
  static function TT_locales($not_tt_SQL_based=False){
    static $TT_locales = Null;
    if ($TT_locales === Null){
      if (!$not_tt_SQL_based){
	$q = myPear_db()->qquery("SELECT * FROM zzz_tt_locales",cnf_dev);
	while($r=myPear_db()->next_record($q)) $TT_locales[$r['l_locale']] = array('l_flag'    =>$r['l_flag'],
										   'l_currency'=>$r['l_currency'],
										   'l_name'    =>$r['l_name'],
										   );
	
      }else{
	$TT_locales = array(CONST_localeBase=>array('l_flag'    =>'us.png',
						    'l_currency'=>'USD',
						    'l_name'    =>'English'),
			    'ru_RU'         =>array('l_flag'    =>'ru.png',
						    'l_currency'=>'RUB',
						    'l_name'    =>'Russian'),
			    'sv_SE'         =>array('l_flag'    =>'se.png',
						    'l_currency'=>'SEK',
						    'l_name'    =>'Swedish'),
			    'fr_CH'         =>array('l_flag'    =>'ch.png',
						    'l_currency'=>'CHF',
						    'l_name'    =>'French'),
			    );
      }
      b_debug::_debug($TT_locales,cnf_dev);
    }
    return $TT_locales;
  }

  /*
   *
   */
  static function TT_guess_locale($text){
    $lang = (preg_match( '/([\p{Cyrillic}]+)/u', $text,$match)
           ? 'ru'
	     : (preg_match('/([éèçëòôùàâ]+)/',$text,$match)
              ? 'fr'
		: (preg_match('/([å]+)/i',$text,$match)
                 ? 'sv'
		   : 'en')));
    foreach(preg_grep("/^$lang/",array_keys(self::TT_locales())) as $l) $lang = $l;
    if (!in_array($lang,array_keys(self::TT_locales()))) $lang = CONST_localeBase;
    b_debug::_debug($lang,cnf_dev);
    return $lang;
  }
  

  /*
   * Create the Translation Table if does not exist yet
   */
  private static function TT_create($path=Null,$reset=False){
    if (tt_SQL_based) b_debug::internalError('Should not happen...');
    if (empty($path)) $path = self::TT_path();
    if ($reset) b_os::unlink($path);
    if (!is_file($path)){
      if (!is_dir(dirname($path))) mkdir(dirname($path));
      file_put_contents($path,"<?php\n");
      if (strpos($path,'myPear') === False){
	file_put_contents($path,      
			  "require (dirname(dirname(dirname(__FILE__))).'/myPear/translate/translation.php');\n",
			  FILE_APPEND);
      }
    }
  }

  /*
   * Return path to the Translation Table
   */
  private static function TT_path($module=Null){
    if (tt_SQL_based) b_debug::internalError('Should not happen...');
    static $TT_path = array();
    if (empty($module)) $module = self::current_module();
    if (empty($TT_path[$module])) $TT_path[$module] = sprintf("%s/data/%s/translate/translation.php",
							      myPear_root_files,
							      $module);
    b_debug::_debug($TT_path[$module],self::$_debug);
    return $TT_path[$module];
  }

  /*
   * Set the list of modules to be translated
   */
  static $modules  = array();
  static $TT_module = array();
  static function init($module=Null){
    if (empty($module)) $module = self::current_module();
    if (!in_array(myPear_MODULE,self::$modules)) self::$modules[] = myPear_MODULE;
    if ( in_array($module,self::$modules)) return;

    // Add module to the list
    self::$modules[] = $module;
    if (class_exists('b_cnf',False) && !b_cnf::get('b_t_lang')) b_cnf::set('b_t_lang',self::$known_lang_default);
    // if (defined('cnf_dev'))
    b_debug::_debug(sprintf("Translation to %s initialized for modules %s",
			    b_cnf::get('b_t_lang'),
			    join(', ',self::$modules)),
		    self::$_debug);
  }

  /*
   * Load the translation table
   */
  private static function TT_load(){
    
    if (!empty(self::$TT)) return;
    foreach(array(myPear_MODULE,self::current_module()) as $module){
      $count = 0;
      $q = myPear_db()->qquery("SELECT * FROM zzz_tt WHERE tt_module = '$module' ORDER BY tt_key",cnf_dev);
      while($r=myPear_db()->next_record($q)){
	self::$TT[$r['tt_key']]['_']            = $r['tt_default']; 
	  self::$TT[$r['tt_key']][$r['tt_locale']]= $r['tt_translated']; 
      }
      b_debug::_debug("$module - ".($count=count(array_keys(self::$TT))-$count)." keys",self::$_debug);
    }
  }

  /*
   *
   */
  static $keys2skip = array('_timestamp','_id2?','_pwd2?'); 
  static function set_keys2skip($keys){
    self::$keys2skip = array_unique(array_merge(self::$keys2skip,$keys));
  }

  /*
   *
   */
  static function keys2skip($key){
    static $cache = Null;
    static $keys2skip = Null;
    if ($cache !== self::$keys2skip){
      $cache = self::$keys2skip;
      foreach (self::$keys2skip as $k=>$v) if (strpos($v,'_') === 0) self::$keys2skip[$k] = "[a-z]*$v";
      $keys2skip = join('|',self::$keys2skip);
    }
    return ($keys2skip && preg_match("/^($keys2skip)$/",$key,$match)
	    ? $match[1]
	    : False);
  }

  private static function b_t_short_txt($text,$max=b_t_long_text){
    return self::escape(strlen($text) > $max
			? mb_strcut($text, 0, $max, "UTF-8") 
			: $text);
  }
  
  /*
   *
   */
  private static function TT_return_result($reply,$case){
    b_debug::_debug("'$reply'".(in_array($case,array('OK')) ? "" : " ($case)"),array(2,'greenText',self::$_debug));
    return str_replace('&apos;',"'",$reply);
  }

  /*
   *
   */
  public static function noPointToTranslate($key,$value='Not set',$class=Null,$option=''){
    
    // Catch the "obvious cases"
    $reply = False;
    if (!$reply) $reply = (($R['outOfScope'] = (is_object($class) && !myPear_db()->columnExists($key,@loader::$sql_identities[get_class($class)]['t']))) ||
			   ($R['isHidden']   = (is_object($class) && in_array($key,$class->isHidden))) ||
			   ($R['OK']         = self::$no_need_to_translate) ||
			   ($R['outOfMod']   = !in_array(self::current_module(),self::$modules)) 
			   //	 ||     ($R['&lt;&gt;'] = self::same_as_reference(@self::$TT[$key][$to]))
			   );
    
    // Skip already translated items (menu selections)
    if (!$reply && is_object($class)){
      if ($reply = self::keys2skip($key)) $R["keys2skip '$reply'"] = True;
    }
    
    // Check the text to be translated    
    if (!$reply && ($value !== 'Not set')){
      $s = join('|',array('gif','png','jpg','jpeg'));
      $b = join('|',array('bForm','bList','bUnit','bHolder'));
      $reply = (($R['empty']   =empty($value))                       ||
		($R['numeric'] =is_numeric($value))                  ||
		($R['test']    =(strToLower($value) === 'test'))     ||
		($R['html']    =($value != strip_tags($value)))      ||
		($R['http']    =(strpos($value,'http:/' ) === 0))    ||
		($R['https']   =(strpos($value,'https:/') === 0))    ||
		($R['notA_Z']  =!preg_match('/[\sa-z\p{Cyrillic}]+/ui',$value))||
		($R['bForm']   = preg_match("/($b)/", $value))        ||       
		($R['image']   = preg_match("/($s)$/i",$value))       ||       
		($R['date']    = preg_match("/^[x\-\,\d\s\/]*$/ui",$value))  ||      // 200 x 300, 1900-2000
		($R['date1']   = preg_match("/^\d\d\s.*\s\d\d\d\d$/",$value)) ||
		($R['date2']   = preg_match("/^\d\d\s.*\s\d\d\d\d.*\d\d .* \d\d\d\d$/",$value)) ||
		($R['dbField'] =myPear_db()->columnExists($value))    ||
		($R['money']   =preg_match("/^((USD|CHF)[\d ]*|[\d ]*(RUB|SEK))$/",$value)));
    }
    
    $reason = '???';
    foreach($R as $text=>$cond) if ($cond) $reason = $text;
    //    b_debug::_debug(var_export($reply,True).($reply?" - $reason":""), self::$_debug);

    return ($reply
	    ? $reason
	    : False);
  }
    
  /*
   *
   */
  private static function TT_substituteIfEmpty($key,$to,$substituteIfEmpty){
    $reply = ($substituteIfEmpty
	      ? Null
	      : (self::$no_need_to_translate===b_t_reference 
		 ? (string)@self::$TT[$key]['_']
		 : (string)@self::$TT[$key][$to]
		 ));
    //    if ($reply)      b_debug::_debug(var_export($reply,True),cnf_dev);
    return $reply;
  }
  
  /*
   * Translator
   */
  private static $TT;
  private static $no_need_to_translate = False;
  static function _($text,$no_need_to_translate=False,$substituteIfEmpty=True){

    // Not to be translated
    if (!in_array(($m=self::current_module()),self::$modules)){
      //      b_debug::_debug("'$m' is not to be translated",cnf_dev);
      if (is_string($text)){
	$reply = $text;
      }elseif (is_array($text)){
	if (!empty($text['item'])){
	  $reply = $text['item'];
	}else{
	  $class = array_shift($text);
	  $field = array_shift($text);
	  $reply = (!empty($text)
		    ? array_shift($text)
		    : $class->getValue($field));
	}	
      }
      if (!isset($reply)){
	b_debug::var_dump(is_object($text) ? "----------object ".get_class($text) : $text);
	$reply = $text;
      }
      b_debug::_debug($reply,array('greenText',self::$_debug));
      return $reply;
    }

    // Load the trabslation table if not yet done
    self::TT_load();

    // Sanity
    if (is_object($text)) b_debug::internalError("Object '$text' as argument");

    // First of all get arguments
    if (is_string($no_need_to_translate) && in_array($no_need_to_translate,array_keys(self::TT_locales()))){
      $to = $no_need_to_translate;
      self::$no_need_to_translate = False;
    }else{
      $to = b_cnf::get('b_t_lang');
      self::$no_need_to_translate = $no_need_to_translate; 
    }

    setlocale(LC_TIME, $to);

    //
    // The text which might be changed by the user should be indexed
    //
    $object_field = False;
    if ($indexed = is_array($text)){
      if (!empty($text['key'])){
	$key  = $text['key'];
	if (($reply=self::TT_substituteIfEmpty($key,$to,$substituteIfEmpty)) !== Null) return self::TT_return_result($reply,'no_substituteIfEmpty');

	$text = (empty(self::$TT[$key])
		 ? @$text['item']
		 : self::$TT[$key]['_']);
      }else{
	$object_field = True;
	$class = array_shift($text);
	$field = array_shift($text);
	if (!($class instanceof bForm)) b_debug::internalError("??? '$class' is not a bForm instance");
	
	// Get the key
	$key = self::build_key($class,$field);
	$text  = (empty($text)
		  ? $class->getValue($field)
		  : array_shift($text));
	if(empty($key))             return self::TT_return_result($text,'empty key');     
	if(!is_numeric($class->ID)) return self::TT_return_result($text,"ID=".$class->ID);
	if (($reply=self::TT_substituteIfEmpty($key,$to,$substituteIfEmpty)) !== Null) return self::TT_return_result($reply,'no_substituteIfEmpty');
	
	// Is the translation nesessary?
	if ($r=self::noPointToTranslate($field,$text,$class)) return self::TT_return_result($text,$r);

	// For the newly created entry resolve the TWIK problem
	if (!self::TT_block_exists($key)){
	  // The "default value" is essential for names only (insider info :-)
	  $tt_default = (strpos($field,'name') === False
			 ? $field.' '.$class->ID
			 : $text);
	  self::TT_add_block($key,$text,'',$tt_default);
	}
      }
    }
    
    if (is_string($text)) $text = trim(str_replace('  ',' ',$text));
    $text_original = $reply = $text;

    // Catch a simple html string
    if (preg_match('#(<[^>]*>)([^<]*)(</[^>]*>)$#',$text,$match) && (count($match)==4) && trim($match[2])){
      array_shift($match);
      list($ob,$text,$cb) = $match;
      $text_original = $text;
    }else{
      $ob = $cb = '';
    }

    // Non-indexed translation, get key from the text if posible
    // (the non-indexed entries come usually from bHolder instances)
    if (empty($key)){
      if ($r=self::noPointToTranslate(0,$text,@$class)) return self::TT_return_result($text,$r);
      if (empty($text)){
	$key = '???';
      }else{
	static $text2key = array();
	if (empty($text2key[$text])){
	  foreach(self::$TT as $k=>$d){
	    foreach($d as $k2=>$t2){
	      if (str_replace(array("'",'&apos;'),'',$t2) == str_replace(array("'",'&apos;'),'',$text)){
		$text2key[$text] = $k;
		// b_debug::_debug("$text <== <strong>$k</strong>/$k2/$t2",self::$_debug);
		break 2;
	      }
	    }
	  }
	}
	if (!($key = @$text2key[$text])){
	  b_debug::internalError("Nonindexed text '$text'");
	  MSG::INFO("Nonindexed text '$text'");
	  $key = 'text;' . b_fmt::translit(self::b_t_short_txt($text,12),True);
	}
      }
    }
    // if (($key == $text) && !myPear_db()->columnExists($key)) b_debug::traceBack("key==text '$key'");

    // The string to be translated might be very long, massage it a bit
    $short_txt = self::b_t_short_txt($text);

    //
    // No need to translate?
    //
    if ($r=self::noPointToTranslate($key,$text,Null,$to)) return self::TT_return_result($reply,$r);
    
    // Workaround(?) hook the known non-indexed expressions
    // Would be nice to create a reversed translation table...
    if (!self::TT_block_exists($key) && ((strpos($key,'text;')!==False) || is_numeric($key))){
      foreach(self::$TT as $k=>$d){
	if (empty($d['_'])){
	  var_dump("??? empty reference text='$k'",$d);
	}elseif (strToLower(self::escape($text_original)) == strToLower($d['_'])){
	  self::$TT[$key] = self::$TT[$k];
	  self::$TT[$key]['hooked'] = 'x';
	  break;
	}
      }
    }
    
    // Do it...
    if (self::TT_block_exists($key)){      

      // Shortcut
      if (strpos(self::$TT[$key][$to],cnf_emptyString)!==False) return self::TT_return_result('','empty'); 
    
      // Try to translate
      if (!empty(self::$TT[$key][$to])){  // The translation is known 
	// if (self::same_as_reference(self::$TT[$key][$to])) self::$TT[$key][$to] = self::$TT[$key]['_'];
	$reply = $ob . self::$TT[$key][$to] . $cb;
	$case = 'OK';
	//	b_debug::_debug($reply,array('greenText',self::$_debug));
      }
    }else{
      // Update the current dictionary, add request for the manual update
      self::TT_add_block($key,$text_original);
      $case = 'add to trans. table';
    }
    
    // Print invitation to fix the translation
    if (self::$may_inviteEditTranslation && empty(self::$TT[$key][$to])){
      b_debug::_debug("empty TT[$key][$to]",cnf_dev);
      MSG::MESSAGE($object_field
		   ? $class->invite2editObjectTT($field,$to) 
		   : self::invite2editCommonTT($key,$to,$text));
      $case = 'to be provided';
    }
    if (in_array((string)$reply,self::$same_as_reference)) $reply = self::$TT[$key]['_'];
    if (in_array((string)$reply,self::$same_as_reference)) b_debug::internalError("reply = '$reply'");
    return self::TT_return_result($reply,'OK');
  }

  private static function invite2editCommonTT($key,$to,$text){
    return (function_exists($f=strToUpper(self::current_module()).'_invite2editCommonTT')
	    ? call_user_func($f,$key,$to,$text)
	    : sprintf("Please update %s/%s dictionary for '".self::b_t_short_txt($text)."'",self::current_module(),$to));
  }
  
  /*
   *
   */
  static function build_key($class,$field){  
    if (!is_object($class)) b_debug::internalError("class=\"".var_export($class,True)."\" is not an object");

    if (!is_numeric($class->ID)){
      if ($class->ID !== 'new'){
	MSG::ERROR($msg="ID=\"".var_export($class->ID,True)."\" is not numeric");
	b_debug::traceBack($msg);
      }
      return Null;
    }

    // No need to translate?    
    //???    if (self::noPointToTranslate($field,'Not set',$class)) return Null;
    
    if (!myPear_db()->columnExists($field,loader::$sql_identities[get_class($class)]['t'])){
      if (self::$_debug){
	MSG::ERROR($msg="Field \"$field\" is not in ".$class->__toString());
	b_debug::traceBack($msg);
      }
      return Null;
    }

    // OK, build the key
    $key = sprintf("%s___%s",$field,$class->ID);
    // b_debug::_debug($key,True);
    return $key;
  }

  /*
   *
   */
  static $same_as_reference = array('_','<>');
  private static function same_as_reference($trigger=Null){
    $reply = in_array((string)$trigger,self::$same_as_reference);
    if ($reply) b_debug::internalError('????????????????????????? this function is OBSOLETE');
    return $reply;
  }

  /*
   *
   */
  static function TT_block_exists($key){
    if (empty($key)) b_debug::internalError("Empty argument \"key\"");
    self::TT_load();
    $reply = isset(self::$TT[$key]);
    if (!$reply) b_debug::_debug($reply,self::$_debug);
    return $reply;
  }
  
  /*
   *
   */
  static function TT_update_block($locale,$key,$translated_text){
    
    $translated_text = self::escape($translated_text);
    if (@self::$TT[$key][$locale] === $translated_text){
      MSG::MESSAGE("No changes in the translation table :-)");
      return;
    }
    
    $module = self::current_module();
    if (($g=self::TT_guess_locale($translated_text)) != $locale){
      MSG::MESSAGE("??? Imposed $locale, but guessed $g");
    }
    
    if (!self::TT_block_exists($key)){
      //
      // Simple case, new block arrives
      //
      b_debug::_debug("add block",self::$_debug);
      self::TT_add_block($key,$translated_text,$locale);
    }else{
      //
      // An existing block is updated
      //
      b_debug::_debug("update block",self::$_debug);
      
      $locale_text = self::TT_guess_locale($translated_text);
      $translated_text = self::escape($translated_text);
      
      // Update the translation table
      self::$TT[$key][$locale] = $translated_text;
      while(empty($tt_id)){
	$q = myPear_db()->query("SELECT tt_id FROM zzz_tt WHERE tt_key = '$key' AND tt_locale = '$locale' AND tt_module = '$module'");
	while($r = myPear_db()->next_record($q)) $tt_id = $r['tt_id'];
	if (!empty($tt_id)) break;
	// Fix on the fly...
	MSG::ERROR("??? tt_key is not found for '$locale', adding the record");
	myPear_db()->query("INSERT INTO zzz_tt (tt_key,tt_locale,tt_module) VALUES('$key','$locale','$module')",True);
      }

      myPear_db()->qquery("UPDATE zzz_tt SET tt_translated = '$translated_text' WHERE tt_id = $tt_id",cnf_dev);

      // Update the translit table, just one record
      call_user_func(array(search(),'TT_update_translit_line'),
		     'zzz_tt',
		     array('tt_id'        => $tt_id,
			   'tt_translated'=> $translated_text),
		     'UPDATE');
    }
    MSG::MESSAGE(__method__."($locale,$key,$translated_text) => $module",True);
  }
  
  /*
   * Add one block to the dictionary, save it on disk
   */
  static function TT_add_block($key,$tt_translated,$tt_locale='',$tt_default=''){
    
    if ( empty($key))            b_debug::traceBack('??? empty key');
    if ( empty($key) || !empty(self::$TT[$key]['hooked'])) return;
    if (!empty(self::$TT[$key])) b_debug::internalError("TT[$key] already exists");
    if ( empty($tt_translated) && empty($tt_default))  b_debug::internalError('??? empty text_original');

    // Guess the module
    self::init();
    $module = (myPear_db()->columnExists($key)
	       ? myPear_MODULE
	       : b_t::current_module());
    
    // Guess the language
    if (empty($tt_locale))     $tt_locale = self::TT_guess_locale($tt_translated);

    // Fill the block
    $tt_translated = self::escape($tt_translated);
    $tt_default = (empty($tt_default)
		   ? $tt_translated
		   : self::escape($tt_default));
    self::$TT[$key]['_'] = $tt_default;
    
    if(!function_exists('TT_join_escaped')){
      function TT_join_escaped($arg){
	foreach($arg as $a) $a_escaped[] = "'$a'";
	return join(',',$a_escaped);
      }
    }
    $insert = array('tt_key'    => $key,
		    'tt_module' => $module,
		    'tt_default'=> $tt_default);
    foreach(array_keys(self::TT_locales()) as $locale){
      self::$TT[$key][$locale] = ($locale == $tt_locale ? $tt_translated : '');
      $insert['tt_locale']     = $locale;
      $insert['tt_translated'] = self::$TT[$key][$locale];
      $sql = sprintf("INSERT INTO zzz_tt (%s) VALUES (%s)",join(',',array_keys($insert)),TT_join_escaped(array_values($insert)));
      myPear_db()->qquery($sql,True);
      // Update the search tables
      call_user_func(array(search(),'TT_update_translit_line'),'zzz_tt',$insert);
    }
    b_debug::_debug(sprintf("Add block TT[$module][$key][$tt_locale] = $tt_translated"),array(cnf_dev,'redText'));
  }
  
  /*
   *
   */
  public static $escapes = array("\n"   => "_CR_",
				 "\r"   => "_CR_",
				 "<br>" => "_;_",
				 "<em>" => "_IO_",
				 "</em>"=> "_IC_",
				 "<li>" => "_LIO_",
				 "</li>"=> "_LIC_",
				 "<ul>" => "_ULO_",
				 "</ul>"=> "_ULC_",
				 "</p><p class='subtitle'>" => "_H2O_",
				 "</p><p>" => "_H2C_",
				 "</p><div class='b-incut__small i-left'>"  => "_ILO_",
				 "</p><div class='b-incut__small i-right'>" => "_IRO_",
				 "</div><p>" => "_ILC_",
				 "</div> <p>"=> "_IRC_",
				 "'"    => "&apos;",
				 '©'    => "_COPY_",
				 );
 /*
  * The unEscapes are array_merge($escapes, $unEscapes)
  */
 public static $unEscapes = array('_EMO_' => '<em style=font-size:smaller>', 
				  '_EMC_' => '</em>',
				);


 /*
  * Beutify a bit text for being edited by a human
  */
 static function tidyLine($text,$newLine="\n"){
   static $ba = array('_ILO_',
		      '_ILC_',
		      '_IRO_',
		      '_IRC_',
		      '_ULO_',
		      '_ULC_');
   static $a  = array('_H2C_',
		      '_LIC_',
		      '_IMGC_');
   static $b  = array('_;_',
		      '_;;_',
		      '_H2O_',
		      '_LIO_',
		      '_IMGO_');
   $reply= str_replace(array("\n\n\n","\n\n"),"\n",
		       preg_replace("/(".implode('|',array_merge($b,$ba)).")/",$newLine.'\\1',
				    preg_replace("/(".implode('|',array_merge($a,$ba)).")/",'\\1'.$newLine,
						 str_replace(array("\n",
								   "\r",
								   "_CR_"),'',$text))));
   if ($reply !== $text) b_debug::_debug($reply,True);
   return $reply;
 }

 /*
  *
  */
 static function escape($text){
   // Remove all "new lines" 
   $escaped = b_fmt::escape(str_replace(array("\n","\r","_CR_","   ","  ")," ",$text));

   // Get back some escapes which are not TT specific
   foreach(array(',','.') as $i) $escaped = str_replace(b_fmt::escape($i),$i,$escaped);

   // Translation specific escapes
   $escaped = str_replace(array_keys(self::$escapes),array_values(self::$escapes),$escaped);

   if (cnf_dev && ($escaped !== strip_tags($escaped))) var_dump(__method__,$escaped,strip_tags($escaped)); 
   return $escaped;
 }

 /*
  *
  */
  static function unEscape($text){

    //
    // Init
    static $search, $id__form;
    if (empty($id__form)){
      $id__form = array('img_id'=>'ART_xImage',
			'av_id' =>'person',
			'c_id'  =>'collection');
      
      $search = join('|',array_keys($id__form));
    }
    
    //
    // unescape http: & https:
    if (strpos($text,"http")===0){
      $reply = array();
      // Make the non-secure links "modal"
      foreach(explode(';',$text) as $h){
	$line[] = (strpos($text,'https') !== False
		   ? x("a href='$h'",$h)
		   : bJS()->modal_iframe($h,$h)); // $dd = "<a href='$dd'>$dd</a>";
      }
      $text = join('<br>',$line);
    }

    //
    // Unescape images
    //    "_IMGO_ <img src>;<any text>  _IMGC_"
    if (strpos($text,'_IMGO_') !== False){    
      $reply = '';
      foreach(explode('_IMGO_',$text) as $line){
	if (strpos($line,'_IMGC_') === False){
	  $reply .= $line;
	}else{
	  $d = explode('_IMGC_',$line,2);
	  $dd = explode(';',$d[0]);
	  $reply .= "<img data-fancybox='images' src='$dd[0]' alt='No image' />\n".
	    (empty($dd[1])?'':x("p",$dd[1])).
	    (string)$d[1];
	}
      }
      $text = $reply;
    }
    
    //
    // Unescape anchor, it might be as follows: (Note only "<any text>" might be missing
    //    "_AO_ (av_id|c_id|img_id)=<id>;<any text> _AC_"
    //    "_AO_ <href>;<href img>;<any text> _AC"
    //    "_AO_ <href>;<any text> _AC_"
    if (strpos($text,'_AO_') !== False){    
      $reply = '';
      $fb = "data-fancybox data-type=iframe data-options={'smallBtn':true,'toolbar':false} href=javascript:;";
      foreach(explode('_AO_',$text) as $line){
	$match = Null;
	if (strpos($line,'_AC_') === False){	  // Syntax error...
	  $reply .= $line;
	}elseif(preg_match("/(^[^;]*);([^;]*);(.*)_AC_(.*)?/",$line,$match)){
	  // Clickable Image
	  // ----------- "_AO_ <href>;<img src>;<any text> _AC"
	  foreach($match as $k=>$v) $match[$k] = trim($v);
	  $reply .= ($x=x("a $fb data-src='$match[1]'","<img src='$match[2]' alt='No image'/>\n".
			  x("p",$match[3]))) . $match[4];
	}elseif(preg_match("/($search)[\s+]?=[\s+]?([^;]*)[\s+]?;[\s+]?(.*)_AC_(.*)?/",$line,$match)){
	  // A page within the site, use fancybox iframe
	  // --- "_AO_ (c_id|av_id|img_id)=<id>;<any text> _AC_"
	  $url = ($match[1] == 'img_id'
		  ? ART_xImage($match[2])
		  : b_url::same("?id=$match[2]&form=" . ART::_bForm($id__form[$match[1]])));
	  $reply .= x("a $fb data-src='$url'",$match[3]) . $match[4];
	}elseif(preg_match("/^([^;]*);(.*)_AC_(.*)?/",$line,$match)){ // ------------------- "_AO_ <href>;<any text> _AC_"
	  $reply .= x("a $fb data-src='$match[1]'",$match[2]) . $match[3];
	}else{
	  MSG::ERROR("??? Syntax error in '$line'");
	}
      }
      $text = $reply;
    }

    //
    // Unescape TT specific <p>...</p>
    if (strpos($text,'_;;_') !== False) $text = x('p',str_replace("_;;_","</p>\n<p>",$text));

    //
    // Unescape the remaining TT specific tags
    $unEscaped = str_replace(array_merge(array_values(self::$escapes),array_keys  (self::$unEscapes)),
			     array_merge(array_keys  (self::$escapes),array_values(self::$unEscapes)),
			     b_fmt::unEscape($text));

    // Sanity...
    if ((strpos($text,'_AO_') !== False) || (strpos($text,'_AC_') !== False)) MSG::ERROR("_AO_/_AC_ mismatch<br>$text");

    return trim($unEscaped);
  }
  
  /*
   *
   */
  static function lang_selector($extras=''){
    static $selector = Null;
    if (empty($selector)){
      foreach(self::TT_locales() as $lang=>$flag){
	if ($lang == b_cnf::get('b_t_lang')) continue;
	$s[] = x("a href='?".$_SERVER['QUERY_STRING']."&b_t_lang=$lang'","<img src='".myPear_images."flags/$flag[l_flag]"."'/>");
      }
      if ($extras){
	if (!is_array($extras)) $extras = array($extras);
	$s = array_merge($s,$extras);
      }
      $selector = x('ul style="font-size:0.75em;list-style:none;color:#000"',
		    b_fmt::joinMap('li style="float:left;padding-left:10px;"',$s));
    }
    return $selector;
  }

  // Backwork compats
  static function getForm(){}
  static function getLangName($locale=Null){ return $locale; }
}


/*
 * Currency converter
 */
// b_currency::demo();
class b_currency{
  
  // European Central Bank URL
  public static $ECBurl = "http://www.ecb.int/stats/eurofxref/eurofxref-daily.xml";

  // Save time...  
  public static $cache_refresh_interval = 36000;
    
  // Popular currencies
  public static $currency_names = array ('EUR' => "European Euro",
					 'USD' => "US Dollar",
					 'GBP' => "Pound Sterling",
					 'CHF' => "Swiss Franc",
					 'CNY' => 'Chinese Yuan',
					 'RUB' => "Russian Ruble",
					 'JPY' => "Japanese Yen",
					 'SEK' => "Swedish Krona",
					 'DKK' => "Danish Krone",
					 'NOK' => "Norwegian Krone",
					 'ISK' => "Icelandic Krona",
					 'BGN' => "Bulgarian Lev",
					 'CYP' => "Cyprus Pound",
					 'CZK' => "Czech Koruna",
					 'HUF' => "Hungarian Forint",
					 'EEK' => "Estonian Kroon",
					 'LTL' => "Lithuanian Litas",
					 'LVL' => "Latvian Lats",
					 'MTL' => "Maltese Lira",
					 'PLN' => "Polish Zloty",
					 'ROL' => "Romanian Leu",
					 'SIT' => "Slovenian Tolar",
					 'SKK' => "Slovakian Koruna",
					 'TRL' => "Old Turkish Lira (to 2004)",
					 'TRY' => "New Turkish Lira (2005)",
					 'AUD' => "Australian Dollar",
					 'CAD' => "Canadian Dollar",
					 'HKD' => "Hong Kong Dollar",
					 'NZD' => "New Zealand Dollar",
					 'SGD' => "Singapore Dollar",
					 'KRW' => "South Korean Won",
					 'ZAR' => "South African Rand",
					 'RON' => 'Romanian Leu',
					 'HRK' => 'Croatian Kuna',
					 'BRL' => 'Brazilian Real',
					 'ILS' => 'Israeli Shekel',
					 'IDR' => 'Indonesian Rupiah',
					 'INR' => 'Indian Rupee',
					 'MXN' => 'Mexican peso',
					 'MYR' => 'Malaysian Ringgit',
					 'PHP' => 'Philippine peso',
					 'THB' => 'Thai Baht',
					 );
  
  /*
   * The converter itself
   */
  static function convert($amount,$from,$to){
    
    // Grab current exchange rates from European Central Bank
    // Check whether we have a recent copy of the data locally
    $cache = sys_get_temp_dir()."/erates.xml";
    if (!file_exists($cache) || ((time() - filemtime($cache)+1) > self::$cache_refresh_interval)){
      // Protect againt "no network"
      if (!$stuff = file(self::$ECBurl))	$stuff = array();
      $fh = fopen ($cache,"w");
      foreach ($stuff as $line) fputs($fh,$line);
    } else {
      $stuff = file($cache);
    }
    
    // Extract data from page - conversion rates between each currency and the Euro
    $exchrate['EUR'] = 1.0;
    foreach ($stuff as $line) {
      preg_match("/currency='([[:alpha:]]+)'/",$line,$gota);
      if (preg_match("/rate='([[:graph:]]+)'/",$line,$gotb)) $exchrate[$gota[1]] = $gotb[1];
    }

    $ok = True;
    foreach(array($from,$to) as $i){
      if (!empty($exchrate[$i])) continue;
      $ok = False; 
      printf("Unknown currency '%s' %s<br>\n",$i,@self::$currency_names[$i]); 
    }

    if (!$ok){
      return $amount;
    }else{
      $amount_converted = floor($amount * $exchrate[$to] / $exchrate[$from]);
      // printf("exchrate(%s->%f) = %.2f <br>\n",$from,$to,$exchrate[$from]);
      return $amount_converted;
    }
  }

  /*
   * Simple demo page
   */
  static function demo(){

    if (function_exists("date_default_timezone_set") and 
	function_exists("date_default_timezone_get"))  @date_default_timezone_set(@date_default_timezone_get());

    print "<h2>Exchange rate demo</h2>\n";
    
    print "<pre>\n";
    foreach (array('SEK',
		   'EUR',
		   'RUB',
		   ) as $from){
      foreach (array('SEK',
		     'EUR',
		     'RUB',
		     'CHF',
		     'DKK',
		     ) as $to){
	$amount = 1000;
	$got = self::convert($amount,$from,$to);
	printf("%d %s == %8.2f %s\n",$amount,$from,$got,$to);
      }
    }
    print "</pre>\n";
    
    
    // Sample output - a table showing an amount converted to Euros, Pounds and Dollars
    print ("<h3>Conversion to Euros, Pounds, US Dollars from other currencies</h3><br>\n");
    print ("<center>\n<table cellpadding=5>\n<tr><td>&nbsp;</td><th>Euros</th><th>Pounds</th><th>Dollars</th></tr>\n");
    $amount = 1000;
    foreach (self::$currency_names as $from=>$name) {
      printf("<tr>\n<td>%s %s</td>\n",$amount,$name);
      foreach (array("EUR","GBP","USD") as $to) {
	$converted_amount = self::convert($amount,$from,$to);
	if ($converted_amount === Null) print("<td align=right>?</td>\n");
	else                            printf("<td align=right>%.2f</td>\n",$converted_amount);
      }
      print ("<tr>\n");
    }
    print ("</table></center>\n");
    
    print "<hr>
Want to <a href=http://www.wellho.net/course/ph.html>learn
how to write a page like this?</a><br>
Copyright <a href=http://www.wellho.net>Well House Consultants</a>, ".date("Y")."\n";
  }
}

/*
 *
 */
class loader{

  public  static $sql_identities = array();
  public  static $sql_identities_extra = array();

  /**
   * Get the object for class(id) from the cache.
   * Instantiate a new one if the cache is empty (see '$mode' below).
   *
   * @param $class
   *   Class name to instantiate 
   * @param $args
   *   Class arguments
   * @param $mode
   *   Additional optional array of attributes. Recognized attributes:
   *     'RO' or 'RW'- access mode for the class instantiation  
   *     'strict'    - do not create an instance if the ID is unknown to the database 
   *                   (otherwise a 'new' class is instantiated)
   *     'fatal'     - like 'strict', but an internal error is generated if the instance is unknown
   *     'nocache'   - do not save the object in the cache
   *     'array'     - the output is expected as an array of classes, i.e. the instance might be not unique
   *     'clean'     - remove instance from the cache and return NULL
   *     'delete'    - remove instance from the cache AND from the database
   *     ''(default) 
   * @param $helper  - optional array($table,$id), needed when module is not yet ready...
   * @return
   *   Reference to the class object
   */
  public static function getInstance_new($class,$args,$mode=array(),$helper=array()){

    // Escape the bogus invocation    
    if (!myPear_db()->isReady()) return Null;

    // Sanity...
    if (!is_array($mode))      $mode = array($mode);
    if (empty($args)){
      if (in_array('fatal', $mode)) b_debug::internalError("??? Empty args, can't find $class($args)");
      return Null;
    }

    if (b_posix::is_int($args)) $args = (int)$args;
    if(!in_array('nocache',$mode)) bCount()->add(__FUNCTION__,$class);

    // Optionally get the class name from class_ID
    if (empty($class)){
      $class = self::getClassFromID($args,False,True);
      b_debug::_debug("getClassFromID($args) - $class",'I');
      bCount()->add(__FUNCTION__,'getClassFromID');
    }

    // Workaround...
    //    $class = str_replace('bForm_Avatar_myPear','bForm_Avatar_ea',$class);

    // Keep the instances in a local cache
    static $reply = array();
    $hash = b_crypt::hash($class.serialize($args));
    
    // Save memory, remove instance from the cache 
    if (in_array('clean',$mode)){ 
      unset($reply[$hash]);
      if ($class === 'all') $reply = array();
      return Null;
    }elseif(in_array('delete',$mode)){
      unset($reply[$hash]);
    }
    
    // Get the instance if it is not cached yet
    if (empty($reply[$hash])){
      locateAndInclude($class);
      list($table,$id) = (empty($helper)
                          ? array_values(b_reg::sql_ids($class,b_reg::$current_module))
                          : $helper);

      // Parse class arguments, it might be class_ID, av_email or query
      if (is_numeric($args))            $args = array($id=>$args);
      elseif ($m=b_fmt::RFC_2822($args))$args = array('av_email'=>x("'",array_shift($m)));
      if (is_array($args)){
        foreach($args as $k=>$v) $where[] = myPear_db()->quote($v,True,$k);
      }else{
        $where = array($args);
      }

      // Query the database
      $where_str = join(' AND ',$where);
      if (empty($table) || empty($id)){ // The module is not yet initialized
        $qquery = sprintf("can't locate table for %s::%s",b_reg::$current_module,$class);
        $num_rows = 0;
      }else{
        $q = myPear_db()->qquery($qquery="SELECT $id FROM $table WHERE $where_str",$verbose=cnf_show_i);
        $num_rows = myPear_db()->num_rows($q);
      }

      // Analasise the query result
      switch($num_rows){
      case 0:
        if ( in_array('fatal', $mode)) b_debug::internalError("??? Can't find $class($where_str) q=$qquery");
        if ( in_array('delete',$mode)) return;
        if (!in_array('strict',$mode)){
          $instance = new $class('empty');
	  b_debug::xxx('creating '.$instance,'I');
          $isVITAL = $instance->isVITAL;
          $instance->isVITAL = array();
          $instance->updateDB($args);
          $instance->isVITAL = $isVITAL;
          if (in_array('nocache',$mode)){
	    b_debug::xxx($instance,'I');
            return $instance;
          }
          $reply[$hash] = $instance;
        }
        break;

      case 1:
        if (in_array('delete',$mode)){
          myPear_db()->qquery("DELETE FROM $table WHERE ".($id=join(' AND ',$where)),cnf_dev||cnf_show_i);
	  MSG::MESSAGE("Removing class $class($id)");
        }else{
          while ($r = myPear_db()->next_record($q)){
            $instance = new $class($r[$id]); 
            if (in_array('nocache',$mode)){
	      b_debug::xxx($instance,'I');
              return $instance;
            }
            $reply[$hash] = $instance;
          }
        }
        break;

      default:
	b_debug::internalError("$num_rows answers for $qquery");
      }
    }else{
      bCount()->add(__FUNCTION__,"$class <span class='greenText'>inCache</span>");
    }
    b_debug::xxx(@$reply[$hash],'I');
    return @$reply[$hash];
  }

  public static function getInstance($class,$id,$mode=array()){

    // Escape the bogus invocation
    if (!myPear_db()->isReady() || empty($id)) return Null;
    
    // Convert argument to array
    if (!is_array($mode))         $mode = array($mode);
    if ( in_array('array',$mode)) $mode[] = 'nocache';
    if (!in_array('RW',   $mode)) $mode[] = 'RO';
    $mode = array_unique($mode);
    
    // Keep the Avatar class as the current module wants
    bForm_Avatar::set_context($class,in_array('basic_avatar',$mode));
    
    // Get the class name
    if ($class_toBeCreated=in_array($id,array('new','empty'))){
      // The instance does not exists yet
      $class_name = $class;
      $class_IDs  = array($id);
      $mode[] = 'RW';
      $mode = array_diff($mode,array('RO'));

    }else{

      // The instance might already exist, check it
      $class_name = self::isIdentified($id,(bool)(stripos($class,'Avatar') !== False),$class);
      $class_IDs  = self::$isIdentified_ID;
      if ($class === 'x') $class = $class_name;
      if (empty($class_name))        return Null;

      // This is not normal
      if (($n=count($class_IDs)) > 1)   b_debug::traceBack("$n records found");
      
      // Sanity, check that the class name is what is is expected to be
      if (!empty($class_name) && (stripos($class_name,$class)===False) && (stripos($class,$class_name)===False)){
        if (cnf_dev){
          $msg = __METHOD__."($class,$id) getClassFromID($id) == '$class_name'";
          if (in_array('fatal',$mode)) b_debug::internalError($msg);
          else                         b_debug::traceBack($msg);
        }
      }
      
      // Remove the instance from the cache (save memory) if needed
      if (in_array('clean',$mode)){ 
        $cache = new b_cache(__METHOD__);
        if ($cache->wasSet($class_name,$id)){
          $o = $cache->get();
	  b_debug::xxx((method_exists($o,'name')?$o->name():''),array(3,'greenText',(bool)(cnf_show_i||cnf_debug),$o));
          bCount()->add(__FUNCTION__,"$class_name <span class='greenText'>removed</span>");
          $cache->remove($class_name,$id);
        }
        return Null;
      }
    }
    
    //
    // Need the form instance, find or create it
    //
    bTiming()->cpu(__FUNCTION__); // ."($class_name)");
    if(!in_array('nocache',$mode)) bCount()->add(__FUNCTION__,$class_name);
    
    // Check the cache and get the instance from the cache if avalaible,
    // otherwise search the database
    locateAndInclude('b_cache');
    $cache = new b_cache(__METHOD__);
    $cacheReady = ($class_toBeCreated
                   ? False
                   : $cache->wasSet($class_name,$id));
    if ($cacheReady){  
        
      bCount()->add(__FUNCTION__,"$class_name <span class='greenText'>inCache</span>");
      $reply = $cache->get();
      if (!($reply instanceof $class_name)) b_debug::internalError(__METHOD__." got wrong class from the cache: $reply instead of $class_name($id)");
      
    }else{
      
      // Fix the e-mail on the fly
      if ($m=b_fmt::RFC_2822($id)){
        $id = array("av_email = '".array_shift($m)."'");
        foreach($m as $email) $id[] = "av_email2 REGEXP '$email'";
        $id = join(' OR ',$id);
      }
      
      // Get new instances
      $reply = array();
      locateAndInclude($class_name,in_array('fatal',$mode));
      foreach($class_IDs as $r_id){ 
        $c = new $class_name($r_id,(in_array('RW',$mode) ? 'RW' : 'RO'));
        // Initialise the cache for the newly created class
        if ($class_toBeCreated) $cache->wasSet($class_name,$c->ID);
        $reply[] = $c;
      }   
    }
    
    // Re-pack the output according to the request "expected as an array"
    // (see avatar_merge)
    $str = array();
    if (empty($reply))       $str[] = '?not found';
    if (in_array('array',$mode)){
      if (empty($reply))     $reply = array();
      if (is_array($reply))  $reply = array_unique($reply);
      else                   $reply = array($reply);
      foreach($reply as $o)  $str[] = $o->__toString(); 
    }else{
      if (is_array($reply))  $reply = @$reply[0];
      if ($o=$reply)         $str[] = $o->__toString(); 
    }
    
    if(empty($reply) && in_array('fatal',$mode)){
      b_debug::internalError(__METHOD__." can't get instance $class_name(id='$id')");
    }
    if (is_object($reply) && !in_array('nocache',$mode))      $cache->set($reply);
    bTiming()->cpu();
    
    b_debug::xxx(join(' ',$str),'I');
    return $reply;
  }
  

  /*
   * Get the bForm class name from the record_ID.
   * All the records in the database(s) of bForm/myPear classes have unique ID (see 'bMasterClass' class),
   * so it is feasible.
   */ 
  public static $_getClassFromID = array();
  public static function getClassFromID($id,$noFormPrefix=False,$current_module_only=False) {
    $reply = Null;
    if ($id instanceof bForm){
      if    ($noFormPrefix==='class') $reply = $id;
      elseif($noFormPrefix)           $reply = str_replace(array('form','bForm_'),'',get_class($id));
      else                            $reply = get_class($id);
    }elseif(b_posix::is_int($id)){
      if ($reply = self::isIdentified($id,$current_module_only)){
	self::$_getClassFromID = loader::$sql_identities[$reply];
        if    ($noFormPrefix==='class') $reply = self::getInstance_new($reply,$id,'strict');
        elseif($noFormPrefix)           $reply = str_replace(array('form','bForm_'),'',$reply);
      }
    }
    return $reply;
  }
  
  /*
   *
   */  
  public static function replaceInstance($class){
    $cache = new b_cache('myPear::instance');
    if ($cache->wasSet(get_class($class),$class->ID)){
      $cache->remove(get_class($class),$class->ID);
    }
    if ($cache->wasSet(get_class($class),$class->ID)){
      b_debug::internalError('something wrong');
    }else{
      $cache->set($class);
    }
  }
  
  /*
   * Get back the widget HTML code
   *
   * @param - the query string
   */
  public static function getWidget(){
    if ($g=$_GET['holder']){
      
      locateAndInclude($h = "bHolder_$g");
      $h = new $h(($arg=@$_GET['id']) ? $arg : @$_GET['mode']);
      $w = $h->getWidget();

    }elseif ($class=$_GET['form']){
      
      if (strpos($class,'bForm') === False) $class = "bForm_$class";
      $f = loader::getInstance_new($class,$_GET['id'],'fatal');
      $w = $f->getWidget();
      if (cnf_dev) b_debug::xxx("expect class '$class', got ".$f->__toString());
    }elseif ($f=$_GET['file_once']){
      
      ob_start();
      include $f;
      $reply = ob_get_contents();
      ob_end_clean();
      $w = x('center',b_fmt::compact_html($reply));

    }else{
      $w = $_SERVER['REQUEST_URI'];
    }
    return $w;
    return "<div id='myPear_widget'>$w</div>";
  }

  public static function setIdentity($id) {
    foreach($id as $class=>$i) loader::$sql_identities[$class] = $i;
  }

  public static $isIdentified_ID = 0;
  public static $isIdentified_debug = False;
  static function isIdentified($id,$current_module_only=False,$class_name=''){
    static $specific_classes = array('l_class','u_class');
    static $cache = array();
    
    // Since this function is called very often, the answer is cached 
    if (is_object($id)){
      // Exotic call with the argument as an object
      $cache[$id->ID] = array('ID'       => array($id->ID),
                              'generic'  => get_class($id),
                              b_cms::_(0)=> get_class($id));
      $id = $id->ID;
    }elseif(is_array($id)){
      // Exotic call with the argument as an array
      if (cnf_dev) b_debug::var_dump($id,b_debug::__());
      return Null;
    }

    if (empty($id))      return False;
    if ($id === 'new')   return True;
    if ($id === 'empty') return True;
    if (stristr($id,'missing')!==False){
      if (cnf_dev) b_debug::traceBack('Broken invocation '.b_debug::__());
      return False;
    }

    // Normal call
    bCount()->add(__FUNCTION__,($id_count = (b_posix::is_int($id) ? 'this->ID' : preg_replace('/( OR ).*/','',$id))));
    if (isset($cache[$id])){
      bCount()->add(__FUNCTION__,"$id_count <span class='greenText'>inCache</span>");
    }else{
      bTiming()->cpu(__METHOD__);
      // Scan the database for this record
      foreach(loader::$sql_identities as $_class=>$_identities){
        // b_debug::xxx("$_class - ".b_fmt::joinX(',',$_identities));
        if (empty($_identities['i'])){
          continue;
        }elseif (!empty($class_name)){
          if ((strpos($_class,$class_name) === False) && (strpos($class_name,$_class) === False)) continue;
          // if (cnf_dev) b_debug::xxx("waiting for '$class_name', got '$_class'");
        }else{
          static $cont = 0;
          // if ($cont++ < 5) b_debug::traceBack();
        }
        
        // Read the record ID and (if presents) the class name which uses this record
        $selects = array();
        foreach(array_merge($specific_classes,array($_identities['i'])) as $field){
          if (myPear_db()->columnExists($field,$_identities['t'])) $selects[] = $field;
        }
        if (empty($selects))  continue;

        $query = "SELECT ".implode(',',$selects)." FROM ".$_identities['t']." WHERE ".(b_posix::is_int($id) 
                                                                                       ? $_identities['i']."='$id'" 
                                                                                       : $id);
        // MSG::DEBUG($query);
        $q = myPear_db()->query($query,$stopIfFails=cnf_dev);
        if (empty($q))  continue;
        
        unset($identified_class); 
        while($r = myPear_db()->next_record($q)){
          $IDs[] = $r[$_identities['i']];
          $identified_class = $_class;
          // Eventually replace the generic class name (like bList / bUnit) by the actual class name (bList_eaEmpRecords / bUnit_ea_members)
          foreach($specific_classes as $c){
            if (isset($r[$c])){
              if (cnf_dev || self::$isIdentified_debug) MSG::DEBUG(__METHOD__."($id): $identified_class --> ".$r[$c]);
              $identified_class = $r[$c];
            }
          }
        }
        if (!isset($identified_class)) continue;

        // Save result in the cache
        // Count all the modules/classes where the found SQL table is used
        $cache[$id]['ID'] = $IDs;
        $cache[$id]['generic'] = $identified_class;
        foreach(loader::$sql_identities as $cls=>$ids){
          if ($ids['t'] == $_identities['t'])  $cache[$id][$ids['m']] = $cls;
        }
        break;
      }
      if (!isset($cache[$id])) $cache[$id] = False;
      bTiming()->cpu();
    }

    if (empty($cache[$id])){
      self::$isIdentified_ID = array();
      $reply = Null;
    }else{
      // Make the ID's available for the calling function
      self::$isIdentified_ID = $cache[$id]['ID'];
      
      // The caller asks for the class_name, optionally filtered by the current myPear module 
      if($current_module_only){
        // During the authentication the proper Avatar class might not yet being loaded,
        // hence give back the generic one instead.
        $reply = @$cache[$id][b_cms::_(0)];
        if (empty($reply) && @$cache[$id]['myPear']) $reply = $cache[$id]['myPear'];
        if (empty($reply) && (stripos(@$cache[$id]['generic'],'Avatar') !== False)){
          $reply = 'bForm_Avatar';
        }
      }else{
        $reply = @$cache[$id]['generic'];
      }
    }
    
    // Sanity && debugging
    if (cnf_dev || self::$isIdentified_debug){
      if(($n=count(self::$isIdentified_ID))>1)b_debug::xxx("Search gives $n replies");
    }
    if (self::$isIdentified_debug || cnf_show_ii || (cnf_dev && empty($reply)))     b_debug::xxx($reply,'I');
    return $reply;
  }

  /*
   * Get the DB class from cache
   */
  public static    $__construct_notComplited = array();
  protected static $_cacheF = array();
  static function _fromCache($c,$arg0=Null,$arg1=Null,$arg2=Null){
    
    foreach(self::$__construct_notComplited as $reset_me){
      foreach(array_merge(array($reset_me),
                          preg_grep("/^$reset_me\./",array_keys(self::$_cacheF))) as $i){
        if (cnf_dev)  b_debug::xxx("Reset cache '$i'");
        unset(self::$_cacheF[$i]);
      }
    }
    self::$__construct_notComplited = array();
    
    locateAndInclude($c,'fatal');
    switch($n=func_num_args()){
    case 4: if (!isset(self::$_cacheF["$c.$arg0.$arg1.$arg2"])) self::$_cacheF["$c.$arg0.$arg1.$arg2"] = new $c($arg0,$arg1,$arg2);
      return self::$_cacheF["$c.$arg0.$arg1.$arg2"];
    case 3: if (!isset(self::$_cacheF["$c.$arg0.$arg1"]))       self::$_cacheF["$c.$arg0.$arg1"]       = new $c($arg0,$arg1);  
      return self::$_cacheF["$c.$arg0.$arg1"];
    case 2: if (!isset(self::$_cacheF["$c.$arg0"]))             self::$_cacheF["$c.$arg0"]             = new $c($arg0);           
      return self::$_cacheF["$c.$arg0"];
    case 1: if (!isset(self::$_cacheF["$c"]))                   self::$_cacheF["$c"]                   = new $c();               
      return self::$_cacheF["$c"];
    default:  b_debug::internalError("strange call with $n arguments");
    }
  }
}

/*
 * Social Security Number treatment
 */
class SSN{
  
  /* Year */
  //static private $yymmdd   =       '(\d\d)([01]\d)([0123]\d)'; // y m d
  static private $yymmdd   =       '(\d\d)(0[1-9]|1[012])([012]\d|3[01])'; // y m d
  static private $yyyymmdd = '([12]\d\d\d)([01]\d)([0123]\d)'; // Y m d

  /* Number */
  static private $n = '\w\d\d\d';
  
  /*
   * We know about the Swedish SSN only.
   */
  public static function known(){
    $reply = array('SE' => array('/^'.self::$yymmdd         .self::$n.'$/', 
				 '/^'.self::$yymmdd  .'[-+]'.self::$n.'$/', 
				 '/^'.self::$yyyymmdd       .self::$n.'$/',  
				 '/^'.self::$yyyymmdd. '-'  .self::$n.'$/',  
				 ));
    return $reply;
  }

  /*
   * Fix known mistakes when filling SSN
   */ 
  public static function fix($ssn){
    $pattern = '/^('.self::$yymmdd.')('.self::$n.')$/';
    if (preg_match($pattern,$ssn,$m)){
      $ssn = $m[1] .'-'. strToUpper($m[5]);
    }

    $pattern = '/^('.self::$yymmdd.')-('.self::$n.')\d*$/';
    if (preg_match($pattern,$ssn,$m)){
      $reply = $m[1] .'-'. strToUpper($m[5]);
    }else{
      $reply = $ssn;
    }
    if ($ssn !== $reply) MSG::WARNING_T("SSN '$ssn' ==> '$reply'");
    return $reply;
  }
  
  /*
   * SE checksum, taken "from Google" as the official algorithm
   * Modified however...
   */
  private static function checkPnr10($pnr) {
    if (strlen($pnr) == 12){
      $pnr = substr($pnr, 2);
    }elseif(strlen($pnr) != 11){
      return False;
    }else{
      $pnr = str_replace(array('-','+'), '', (string)$pnr);
      if(strlen($pnr) != 10) return False;
    }
    if (!preg_match('/^'.self::$yymmdd.'/',$pnr,self::$matches)) return False;

    $n = 2;
    $sum = 0;
    for ($i=0; $i<9; $i++) {
      $tmp = $pnr[$i] * $n;
      ($tmp > 9) ? $sum += 1 + ($tmp % 10) : $sum += $tmp; ($n == 2) ? $n = 1 : $n = 2;
    }
    $reply = (bool)(!(($sum + $pnr[9]) % 10));
    return (bool)$reply;
  }

  /*
   * SSN can't contain string "0000"
   */
  public static  $matches = array();
  public static  $ssn_country = '?';
  public  static function is_valid($ssn,$verbose=False){

    self::$ssn_country = 'SE';
    $ssn = preg_replace('/^(......)-[pP0]000$/','\\1'.'-P123',(string)$ssn);
    $reply = !empty($ssn) && (strpos((string)$ssn,'000') === False) && (self::checkPnr10($ssn) || preg_match('/^(\d\d)?\d\d\d\d\d\d-[pP]\d\d\d$/',$ssn));
  
    /*  
    $reply = False;
    if (strpos((string)$ssn,'000') === False){
      foreach(self::known() as $country=>$patterns){
      foreach($patterns as $pattern){
        if (preg_match($pattern, (string)$ssn,self::$matches)){
	    
	    // Careful if the age is > 100 years
	        if (strpos($ssn,'+') == 6)   self::$matches[1] -= 100;
		    
		    // Guess the century...
		        if(self::$matches[1] < 1900) self::$matches[1] += 1900;
			    
			    // static $ccc=0; if (++$ccc < 8) b_debug::print_r(self::$matches);
			        self::$ssn_country = $country;
				    $reply = True;
				        break 2;
					    }
					      }
					      }
      } 
    */

    // Print a warning if the SSN is not valid & the record is editable
    if ($verbose === 'check_only') $verbose = False;
    if (!$reply && !empty($ssn)){
      $msg = "\"$ssn\" is not a valid Social Security Number";
      if (is_object($av=$verbose)){
	$msg .= ' for '.$av->fmtName('fl');
	if (!$av->isWritable()) unset($verbose);
      }
      if (!empty($verbose) && !empty($msg)) MSG::ERROR($msg);
      // if (cnf_dev) b_debug::xxx($reply ? "Yes, ".self::$ssn_country : "No");
    }
    return $reply;
  }
  
  /*
   * The birthdate is packed to the SSN
   * @param return 
   *   - Null if SSN is not valid
   *   - unix time if SSN is ok
   */
  public static function ssn2birthDate($ssn,$av=False){
    $reply = (self::is_valid($ssn,$av)
	      ? b_time::noon(self::$matches[1],self::$matches[2],self::$matches[3])
	      : False);
    return $reply;
  }
}


/*
 * Messages handling
 */
class MSG{
  
  static public function TBD($text='.',$class=Null)  { self::MESSAGE($text, $class, 'TBD'); }
  
  static public function ERROR($text,$class=Null){
    if (stripos($text,'missing value')!==False)  self::mail_diagnostics(Null,strip_tags($text));
    b_debug::_debug($text,array(2,True,'bold-text'),'error');
    b_debug::_debug($text,array(2,cnf_dev,'redText'));
  }
  
  static public function WARNING  ($text,$class=Null){
    b_debug::_debug($text,array(2,True,'bold-text'),'warning');
    b_debug::_debug($text,array(2,cnf_dev,'redText',$class));
  }

  static public function WARNING_T($text,$class=Null){
    // WARNING
    b_debug::_debug($text,array(2,True,'redText',$class),'SICK');
    // debug line
    if (is_object($class) && is_callable(array($class,'dbg')))  $class->dbg($text,array(3,True,'redText'));
    else      b_debug::_debug($text,array(2,True,'redText'));
  }

  static public function MESSAGE2 ($text,$class=Null){ self::MESSAGE($text, $class, 'message_bottom'); }
  
  static public function MESSAGE_once($text,$class=Null,$id='status',$heading=''){ self::MESSAGE($text,$class,'status',$heading,True); }
  
  static public function INFO_once($text,$heading=''){ self::INFO($text,$heading,True); }
  
  static public function INFO($text,$heading='',$only_once=False){ 
    if (empty($text))      $text = array();
    if (!is_array($text))  $text = array($text);
    if (is_array($heading))$heading = $heading[0];
    if (!empty($heading))  $heading = x('span style="font-weight:bold"',$heading); 
    
    self::$show_this_message_in_NOUSER_context = True;
    self::MESSAGE($heading . (empty($text) ? '' : x('ul',b_fmt::joinMap('li',$text))),
		  Null,
		  'info',
		  '',
		  $only_once);
    self::$show_this_message_in_NOUSER_context = False;
  }
  
  static public function DEBUG($text='',$class=Null){ 
    $forced = ($class === 'forced');
    if (($class === False) || (!YBhere && !cnf_dev && !$forced))  return;
    if (!is_object($class)) $class = Null;
    if ($forced) self::MESSAGE_forced($text?$text:b_debug::__(2), $class, 'debug'); 
    else                self::MESSAGE($text?$text:b_debug::__(2), $class, 'debug'); 
  }
  
  
  private static $show_this_message_in_NOUSER_context = False;
  static public function MESSAGE_forced($text,$class=Null,$id='warning',$heading=''){
    self::$show_this_message_in_NOUSER_context = True;
    self::MESSAGE($text,$class,$id,$heading);
    self::$show_this_message_in_NOUSER_context = False;
  }
  
  /*
   * Display the unified message.
   */
  static public function MESSAGE($text,$class=Null,$id='status',$heading='',$only_once=False){
    
    // By default we don't show any messages in no-user context, unless it is explicitly requested 
    $drop = (!self::$show_this_message_in_NOUSER_context &&
	     !cnf_dev && (!class_exists('bAuth',0) ||
			  (class_exists('bAuth',0) && !bAuth::$isReady) || 
			  (class_exists('bAuth',0) && !is_object(bAuth::$av))));
    if ($drop)  return;
    if ($only_once && @$_SESSION[__METHOD__][b_crypt::hash($text.$heading)]++) return;
    
    // Add the caller in debug mode
    //    if (cnf_dev && in_array($id,array('error'))) $text = x('strong',x('em',(b_debug::__(array(3),$class).": "))) . $text;
    
    // Go on
    $caller = '';
    if (trim(strip_tags(trim($text)))){
      $css_class = $id;
      switch($css_class){
      case 'TBD':
	$caller = b_debug::__(3,$class);
	$text   = b_fmt::redText("<em>$id: $caller</em> $text");
      case 'status':
	if (!cnf_dev && !cnf_CLI && class_exists('bAuth',0) && !is_object(bAuth::$av)) return;  
	break;
      case 'error':  
	myPear_logs()->syslog(strip_tags($text));
	break;
      case 'SICK': 
	$css_class = 'debug';
      case 'debug':
	if (is_object($class)){
	  $caller = b_debug::__(3,$class);
	  $text = $caller.' '.x('strong',x('em',$text));
	}
	$text = b_debug::s_e_depth() . $text;
	break;
      default:
      }
      
      static $MESSAGE_dejaVu = array();
      $msg_id = b_crypt::hash($x=trim(str_replace('&nbsp;','',$text)));
      
      if (empty($MESSAGE_dejaVu[$msg_id])) $MESSAGE_dejaVu[$msg_id] = 0;
      if (b_cnf::get('noCompactMessages') || b_debug::$starting_exiting || !$MESSAGE_dejaVu[$msg_id]++){
	if ($id=='message_bottom')  $_SESSION['message_bottom'][] = $text; 
	elseif (function_exists('drupal_set_message') && !b_cnf::get('nodrupal')) drupal_set_message($text,$css_class);
	elseif (cnf_CLI) b_fmt::printt($text);
	else   print "<div class='messages $css_class'>$text</div>\n";
      }
    }
  }
  
  static public $messageBuffer_active = False;
  static public $MESSAGE_add_counter = 0;
  static public function MESSAGE_open($title,$h3='h3') {
    if (!empty($_SESSION['messages.cache'])) self::MESSAGE_close();
    $_SESSION['messages.cache'] = array("<$h3>$title</$h3>");
    self::$messageBuffer_active = True;
  }
  
  static public function MESSAGE_close($id='status'){
    if (is_array($c = @$_SESSION['messages.cache'])){
      if ($id == 'status') MSG::MESSAGE(join('<br/>',$c).'<br/>&nbsp;');
      else                 MSG::WARNING(join('<br/>',$c).'<br/>&nbsp;');
    }
    unset($_SESSION['messages.cache']);
    self::$messageBuffer_active = False;
    self::$MESSAGE_add_counter = 0;
  }
  
  static public function MESSAGE_add($text,$add_counter=False,$noCompactMessages=True){
    $noCompactMessages_sv = b_cnf::get('noCompactMessages');
    b_cnf::set('noCompactMessages',(int)$noCompactMessages);
    if ($add_counter) $text = (++self::$MESSAGE_add_counter) . " - $text";
    if (empty($_SESSION['messages.cache']))  MSG::MESSAGE($text);
    elseif(!in_array($text,$_SESSION['messages.cache']) || b_cnf::get('noCompactMessages')) $_SESSION['messages.cache'][] = $text;
    b_cnf::set('noCompactMessages',(int)$noCompactMessages_sv);
  }
  
  /*
   *
   */
  public static function H1($text, $mode='noTranslate'){
    static $dejaVu = array();
    static $title = Null;

    if ($text == 'get')   return $title;
    
    if (!empty($text) && ($text != b_reg::$current_module)){
      if (!is_array($mode)) $mode = array($mode);
      if ($text=='empty')   $mode[] = 'noTranslate';
      
      if (!empty($text)){
	$text = ($text=='empty'
		 ? ''
		 : b_t::_($text,in_array('noTranslate',$mode)));
	
	if (is_null($title) || in_array('reset', $mode)){
	  self::setPageHeader($title = trim(b_fmt::unEscape($text)));
	}
      }
    }
    return '';
  }
  
  static function H2($text,$noTranslate=True)  { 
    static $dejaVu = array();
    $reply = (empty($text) || @$dejaVu[mb_strToLower($text)]++ || (mb_strToLower($text) == mb_strToLower(self::H1('get')))
	      ? ''
	      : '<h2>'.b_fmt::escape(b_t::_($text,$noTranslate)).'</h2>'); 
    return $reply;
  }
  
  static function H3($text,$noTranslate=False)  { 
    return '<h3>'.b_fmt::escape(b_t::_($text,$noTranslate)).'</h3>'; 
  }

  /*
   *
   */
  static function set_main_selector($input,$noPrint=False){
    static $n = 0;
    
    $li = array('');
    $enctype = (1 ? 'application/x-www-form-urlencoded' : 'multipart/form-data');
    foreach($input as $label=>$selection){
      if (!is_numeric($label)) $li[] = x("li role='label'",$label);
      $li[] = x("li",
		(preg_match('/<(select|input)/i',$selection)
		 ? x(sprintf("form action='%s' method='POST' enctype='$enctype' class='only_online' name='%s%d' accept-charset='UTF-8'",
			     b_url::same(),'selector',++$n),
		     $selection)
		 : $selection));
    }
    $reply = (x("div id='mypear_ul_selector'",x('ul',join("\n",$li))).
	      x("div style='clear:both'",''));
    if ($noPrint)   return $reply;
    else            print  $reply;
  }
      
  /*
   *
   */
  static function setPageHeader($header,$strip_tags=True){
    global $myPear_custom_title_striped,
           $myPear_custom_title;

    b_cache_file::CMS(__METHOD__,array($header));
    $myPear_custom_title_striped = $header;

    if (function_exists('drupal_set_title')) { 
      // Drupal does not let html tags to be in the header
      if (($header_striped = strip_tags($header)) != $header){
	$myPear_custom_title = $header;
	$myPear_custom_title_striped = $header_striped;
	drupal_set_title($header_striped);
      }else{
	drupal_set_title($header);
      }
    }else{
      $header = "<h1>$header</h1>\n"; 
    }
    b_debug::_debug($header,cnf_dev);
    return $header;
  }  

}

/*
 * Variables
 */
class b_vars{

  public static $debug = False;

  /*
   * Delete variable
   */
  static function delete($name,$module){
    myPear_db()->query("DELETE FROM zzz_variables WHERE var_module='$module' AND var_name='$name'");
  }
  
  /*
   * Check is the variable name known 
   */
  static function isVariableKnown($var_name){
    $q = myPear_db()->qquery("SELECT var_name FROM zzz_variables WHERE var_name = '$var_name'",True);
    $reply = myPear_db()->num_rows($q);
    b_debug::xxx($reply,array(2,cnf_dev));
    return $reply;
  }

  /*
   * Check is the value known 
   */
  static function isValueKnown($value,$module=Null){
    if(empty($module))    $module = b_reg::$current_module;

    if (is_array($value)){
      ksort($value);
      $value = serialize($value);
    }

    $q = myPear_db()->qquery("SELECT var_name FROM zzz_variables WHERE ".
			     "var_module = '$module' AND ".myPear_db()->quote($value,True,'var_value')." LIMIT 1",True);
    $reply = (bool)myPear_db()->num_rows($q);
    b_debug::xxx($reply,array(2,cnf_dev));
    return $reply;
  }
  
  /*
   * Get the variable(s)
   */
  static function get($name,$module=Null,$regexp=False){

    $reply = array();
    
    if ($regexp){
      // Get the variable names which match the regular expression
      $q = myPear_db()->qquery("SELECT * FROM zzz_variables WHERE ".
			       myPear_db()->quote($name,'RR','var_name').
			       " ORDER BY var_name",self::$debug||cnf_dev);
      
      // Get recursively all the values & modules
      while($r = myPear_db()->next_record($q)){
	if (!is_array(b_fmt::safe_decode($r['var_value']))){
	  b_debug::traceBack('var_value is not array');
	  b_debug::print_r($r);
	  continue;
	}
	// add the variable name to the output (sic, recursion!)
	$var = self::get($r['var_name'],$r['var_module']);
	foreach(array('var_name','var_timestamp') as $i){
	  if (@$r[$i]) $var[$i] = $r[$i];
	}
	$reply[$r['var_module']][] = $var;
      }
    }else{
      $q = myPear_db()->qquery("SELECT * FROM zzz_variables WHERE ".join(' AND ',self::args($name,$module)),self::$debug);
      while($r = myPear_db()->next_record($q)){
	if (is_array($reply = b_fmt::safe_decode($r['var_value']))){
	  if (is_array($var_value2=b_fmt::safe_decode($r['var_value2']))) $reply += $var_value2; 
	  if (empty($reply['var_name'])) $reply['var_name'] = $name;
	}
      }
    }
    if (self::$debug) b_debug::xxx($reply);
    return $reply;
  }

  /*
   *
   */
  private static function args($name,$module=Null){
    if (empty($module)) $module = b_reg::$current_module;
    $args = array(myPear_db()->quote($module,True,'var_module'),
		  myPear_db()->quote($name,True,'var_name'));
    return $args;
  }

  /*
   *
   */
  static function set($name,$value,$module=Null,$value2=Null){
    if (is_array($value))  ksort($value);
    if (is_array($value2)) ksort($value2);

    // Save the values
    $var_values[] = myPear_db()->quote(b_fmt::safe_encode($value),True,'var_value');
    if (!empty($value2)) $var_values[] = myPear_db()->quote(b_fmt::safe_encode($value2),True,'var_value2');

    // Create or update the record
    myPear_db()->qquery((b_posix::is_empty(self::get($name,$module))
			 ? "INSERT INTO zzz_variables SET ".join(',',array_merge($var_values,self::args($name,$module)))
			 : "UPDATE zzz_variables SET ".join(',',$var_values)." WHERE ".join(' AND ',self::args($name,$module))),
			True);
  }
}

/*
 * Bit-mask get/set
 */
class b_mask{

  static $debug = False;

  /*
   * Set the bit
   *
   * @param $packed_value - the packed data
   * @param $mask - the bit-mask to set
   * @param $value - empty / !empty
   */
  public static function set($packed_value, $mask, $value){
    $o = (int)$packed_value;
    $m = (int)$mask;
    if ($value) $reply = ($o |  $m);
    else        $reply = ($o & ~$m);
    b_debug::xxx($reply,array(2,self::$debug));
    return $reply;
  }

  /*
   * Check that the bit is set
   *
   * @param $packed_value - the packed data
   * @param $mask - the bit (bit-mask, might be not a single bit) to check
   */
  public static function get($packed_value,$mask){
    $reply = (bool)((int)$packed_value & (int)$mask);
    return $reply;
  }

}



/*
 * Buttons & Links
 * - button: used normally to submit a form
 * - link: click-able text string (like "[ next view ]") or 16x16 icon   
 */
class b_btn{

  /*
   * Confirm an action without javascript (pop-up the yes/no question)
   *
   *  switch(b_btn::confirm()){
   *  case 'no':
   *    return "NO, will not do it";
   *    
   *  case 'yes':
   *    return "YES, sure";
   *   
   *  case Null:
   *  default:      // ask for confirmation
   *    $hidden_args = array('key1'=>'value1', 'key2'=>'value2');
   *    $confirm = b_btn::confirm("Are you sure you want to do this?", False, $hidden_args);
   *    print bJS()->modal_inline($text, $confirm, $showTouteSuite=True, $showCloseButton=False);
   *  }
   */
  static public  $confirm_waiting = False;
  static function confirm($text=Null,$printQuestion=True,$hidden_args=array()){
    static $act_confirmation = 'act_confirmation';
    if (empty($text)){
      self::$confirm_waiting = False;      
      return b_cnf::getValue($act_confirmation);
    }else{

      self::$confirm_waiting = True;      
      $hd = array();
      foreach ($hidden_args as $n=>$v) $hd[] = "<input type='hidden' name='$n' value='$v' />";
      
      $txt = x("div class='error align_center messages'",
	       bIcons()->get('32-phone2').' '.$text.
	       x("form method='post' enctype='multipart/form-data' name='confirmator'",
		 (join("\n",array(join("\n",$hd),
				  "<input type='submit' class='form-submit' name='$act_confirmation' value='yes'>",
				  "<input type='submit' class='form-submit' name='$act_confirmation' value='no'>")))));
      if ($printQuestion) print  $txt;
      else                return $txt;
    }
  }

  /*
   * Really Big Button
   */
  static function big_button_grey($text,$url='',$bgColor=Null,$smaller=Null,$confirm=Null){
    return self::big_button($text,
			    $url,
			    ($background=(stripos($text,'delete') === False ? 'grey' : 'red')),
			    ($smaller_font=(is_bool($smaller) ? $smaller : True)),
			    ($confirm=(stripos($text,'delete') !== False)));
  }

  static function big_button($text,$url='',$bgColor='green',$smaller=False,$confirm=True){
    if (is_array($url)) $url = '?'.b_fmt::joinX('&',$url);
    if (!empty($url) && (strpos($url,'?') !== 0)) $url = b_url::same($url);
    return self::submit($text,
			$url,
			$confirm,
			$class=($smaller ? "mdl" : "big" )."_button bg_$bgColor");
  }
  
  /*
   * Button type="submit", uses Java-Script for the submittion, hence works without the <form> as well.
   * Raison d'être - to have multiple submit buttons in the form, not all of them "parts" of the <form>
   */
  static function submit($text, $url='', $confirm=False, $class='', $img=''){
    // Optionally translate ....
    $maxL = 20;
    $text = b_t::_(array('key'=>'button;'.(strlen($text)>$maxL?substr($text,0,$maxL):$text),'item'=>$text));

    $style = "style='padding:1px; font-size:small;'";
    $style = '';
    list($img,$type)= (!empty($img)
		       ? array("src='$img'", 'image')
		       : array('',           'submit'));
    
    // Check for the explicit confirmation request encoded in the URL
    if (stripos($url,                    'confirm_yes')  !== False) $confirm = True;
    if (stripos($url,b_crypt::hex_encode('confirm_yes')) !== False) $confirm = True;
    
    if (empty($url)){ 
      $reply = "<input type='$type' value='$text' $style class='$class form-submit' title='$text' name='_${url}' />\n";
    }else{
      if (strpos($url,'?')===0) $url = b_url::repack($url);
      $url = str_replace('&amp;','&',$url);
      $reply = sprintf("<input type='$type' value='$text' $style class='$class form-submit' $img onClick='%s' title='$text' name='%s' />\n",
		       ($confirm 
			? "return submitConfirm(this);"
			: "document.location.href=\"$url\";return false;"),
		       $url
		       );
    }
    return $reply;
  }

  /*
   * Icon as <a href='$url'>$text</a>  button 
   */
  static function submit_icon($icon, $text_action, $url, $confirm=True,$min100='min100'){
    $text_action = strip_tags($text_action);
    return x("span class='only_online'",
             x("a href='$url'".($confirm ? " onClick='return confirm(\"Really $text_action?\");'" : ""),
	       bIcons()->get(array('i'=>$icon,'d'=>$text_action,'class'=>"form-submit $min100"))));
  }

  /*
   * return "<a href='$url'>$text</a>"
   */
  static function a($text, $url){
    if (strpos($url,'?') === 0) $url = b_url::repack($url); 
    return x("a href='$url' class='mypear_a'",$text);
  }

  /*
   * Click-able link like "[ next view ]" OR 16x16 icon
   */
  static function link($text, $url, $img=Null){

    // First try $text as an 16x16 icon name
    foreach(array(bIcons()->get($text,'',True),
		  bIcons()->getThumbnail($text)) as $i){
      if (!empty($i)){
	$button = $i;
	$text   = '';
	if(is_string($img) && stristr($img,'onclick') !== False) $onclick = $img;
	break;
      }
    }

    // Then check that the text is not a button already
    if (empty($button) && (stripos($text,'<img') !== False)){
      $button = $text;
      $text   = '';
    }
    
    // If the text in not an icon name, then continue
    if (empty($button)){
      if (!empty($text)){
	$text = trim(b_t::_($text));
	if (!preg_match('/^<.*>/',$text)) $text = str_replace(' ','&nbsp;',$text);
      }
      $button = (is_string($img) ? "<img src=$img ".(strpos($img,'alt=')?'':" alt='$text' title='$text'")."/>" : '').$text;
      if (!empty($text)) $button = "&nbsp;[&nbsp;$button&nbsp;]&nbsp;";
    }

    // Optionally confirm the request
    if (is_bool($img) && ($confirm = $img)) $onclick = "class='form-submit' title='$text' onclick='return submitConfirm(this);'"; 
    
    if (strpos($url,'?') === 0) $url = b_url::repack($url); 
    return x("span class='only_online'","<a class='mypear_a' ".@$onclick." href='$url'>$button</a>");
  }

  /*
   *
   */
  static function UL_buttons($submits){
    if (is_string($submits)) $submits = array($submits);
    $li = "li style='float:left;margin:2px;list-style:none;background:none'";
    $dv = "div class='centralized'";
    return x($dv,x('ul',b_fmt::joinMap($li,$submits)));
  }

  /*
   *
   */
  static function buttons_line($submits,$print=True){
    if (empty($submits)){
      $reply = "<!-- empty 'buttons_line' -->\n";
    }else{
      if (!is_array($submits)) $submits = array($submits);
      $reply =
	x("div style='clear:both'",'').
	x('div class="buttons_line only_online"',
	  x('ul',
	    b_fmt::joinMap('li',$submits))).
	x("div style='clear:both'",'');
    }
    if ($print) print "$reply\n";
    else        return $reply;
  }
  
}

/*
 * Various formatting utilities
 */
class b_fmt{

  /*
   * convert 'Öle Åkaön','è','å','Å','ö','Ö','ä','Ä'
   *      to 'Ole Akaon','e','a','A','o','O','a','A'
   */
  static function translit($text,$killSpaces=False){
    $text = preg_replace("/&([a-z])[a-z]+;/i", "$1", 
			 htmlentities(str_replace(array('&lt;','&gt;','/','?','"',"'",'(',')','[',']','”','«','»','’','_'),
						  '',
						  $text)));
    if ($killSpaces) $text = str_replace(' ','_',$text);
    return trim($text);
  }

  /*
   * Split HTML into array:
   * "<p>text</p> becomes array(<p>,text,</p>)
   */
  static function html2array($text){
    $parced = array();
    // $text = trim(str_replace("\n","",$text));
    $text = trim($text);
    while(!empty($text)){
      if (($n=strpos($text,'<')) === False){
	$substr = $text;
      }elseif($n === 0){
	$n = strpos($text,'>');
	$substr = ($n===False
		   ? b_debug::internalError("Wrong syntax ".var_export($text,True))
		   : substr($text,0,$n+1));
      }else{
	$n = strpos($text,'<');
	$substr = ($n===False
		    ? $text
		   : substr($text,0,$n));
      }
      $pos = strpos($text, $substr);
      $text = trim(substr_replace($text, '', $pos, strlen($substr)));
      $parced[] = trim($substr);
    }
    return $parced;
  }

  /*
   *
   */
  static function safe_explode($delimiter,$value){
    if (is_array($value)){
      $reply = $value;
    }else{
      $reply = (strpos($value,$delimiter) !== False
		? explode($delimiter,$value)
		: $value);
    }
    b_debug::xxx($reply);
    return $reply;
  }

  /*
   * Safe version of unserialize
   */
  public static function safe_decode($encoded,$always_return_array=False){
    if (is_array($encoded)){
      $reply = $encoded;
    }elseif($encoded === self::$safe_null){
      $reply = Null;
    }else{
      $er = error_reporting(0);
      $reply = unserialize($encoded);
      $er = error_reporting($er);
      if (!is_array($reply) && !is_object($reply)){
	$reply = $encoded;
	//	if (cnf_dev) b_debug::xxx(var_export($reply,True));
      }
    }
    if ($always_return_array && !is_array($reply)) $reply = (empty($reply) ? array() : array($reply));
    return $reply;
  }

  /*
   * Safe version of serialize
   */
  private static $safe_null = '<Null>';
  public static function safe_encode($array){
    if ($array === Null){
      $reply = self::$safe_null;
    }elseif (is_array($array) || is_object($array)){
      $reply = serialize($array);
    }else{
      $reply = $array;
    }
    return $reply;
  }

  /*
   *
   */
  public static $uml = array('&aring;' => 'å',
                             '&Aring;' => 'Å',
                             '&auml;'  => 'ä',
			     '&uuml;'  => 'ü',
                             '&Auml;'  => 'Ä',
                             '&ouml;'  => 'ö',
                             '&Ouml;'  => 'Ö',
                             '&Eacute;'=> 'É',
                             '&eacute;'=> 'é',
                             '%26aring%3B' => 'å',
                             '%26auml%3B'  => 'ä',
                             '%26ouml%3B'  => 'ö',
			     '%26uuml%3B'  => 'ü',
                             );         
  public static function unEscape_uml(&$text,$extra=array()){
    if (True){
      $text = html_entity_decode(str_ireplace('&nbsp;',' ',str_replace('%26','&',str_replace('%3B',';',$text))));
    }else{
      foreach(self::$uml as $f=>$t) $text = str_replace($f,$t,$text);
    }
    foreach($extra as $f=>$t) $text = str_replace($f,$t,$text);
  }

  public static function printt($line){
    if (cnf_CLI) print self::unEscape(strip_tags($line))."\n";
    else         print $line;
  }

  /*
   * format a value as "human readable", mainly for the debugging
   */
  public static function _($v) {
    if ($v === Null)                                   return 'Null';
    if (is_string($v) || is_numeric($v))               return $v;
    if (is_bool($v))                                   return ($v ? 'True' : 'False');
    if (is_array($v))                                  return sprintf('array-%d(%s)',count($v),b_fmt::join($v));
    if (empty($v))                                     return b_fmt::escape(cnf_emptyString);
    if ($v instanceof bForm)                           return $v->__toString();
    if ($v instanceof myPear_getValue)                 return $v->__toString();
    return $v;
  }

  /*
   * Proper ucwords for the first/last name.
   * Extend the definition of a whitespace by adding 'dot' &'dash
   */
  static function ucwords($text,$catchMc=False){
    static 
      $dejaVu = 0,
      $stay_lower = array('at','in','by','the','for', 'für', 'and', 'of','och',// Connecting words
			  //			  'van','van de','van der'
			  'v.','van','von','di','de','del','der','den','la','los',"'t", // Dutch, French, German nobility
			  'degli'),   // Common names
      
      // specific words like PhD
      $specific   = array('PhD', 'I', 'II', 'III','ETH','CERN','MIT','IHEP','CNRS','EPFL','MIPT','HR','CERN','NBI'),
      // d'Artagnan
      $sustr      = array("/\\bD'/i"=>"d'", "/\\bDe'/i"=>"de'");
    
    // Small optimization
    if (!$dejaVu++){
      foreach (array_merge($stay_lower,$specific) as $l) $sustr["/\\b$l\\b/i"] = $l;
    }
    
    // Convert everything to the lower case, then selectively apply 'mb_convert_case'
    $text = mb_convert_case(mb_convert_case($text,MB_CASE_LOWER,'utf-8'),MB_CASE_TITLE,'utf-8');
    foreach(array(',','.','-','/',"'") as $s){
      if (strpos($text,$s) !== False){
	$words = array();
	foreach(explode($s,$text) as $word){
	  /*
	  if ($w = @$specific[$word]){
	    print "$word ==> $w\n";
	    $word = $w;
	  }elseif(!in_array($word,$stay_lower)){ 
	  */
	  $word = mb_convert_case($word, MB_CASE_TITLE, 'utf-8');
	  // Catch 'van Nieuwkoop-McCall' 
	  if ($catchMc) self::ucwords_Mc($word);
	  $words[] = $word;
	}
	$text = join($s,$words);
      }
    }
    
    if (!empty($sustr)){
      $text = trim(preg_replace(array_keys($sustr), array_values($sustr),$text));
      // 't Hooft ...
      if (strpos($text,"'T ") === 0) $text = preg_replace("/^.T /","'t ",$text);
    }
    
    // Catch 'McInne' && 'MacDenees', skip 'Macej'
    if ($catchMc) self::ucwords_Mc($text);
    return $text;
  }
  
  protected static function ucwords_Mc(&$word){
    static $mc  = 'Mc';
    static $mac = 'Mac';
    foreach(array($mc,$mac) as $mm){ 
      $az = ($mm === $mac
	     ? '[bcdfhjlmnpqrstvwxyz]' // consonants (but not g,k) http://genforum.genealogy.com/macarthur/messages/595.html
	     : '.');                   // any character
      if (preg_match("/^($mm)($az)(.*)$/",$word,$m)){
	$word = $m[1] . mb_convert_case($m[2],MB_CASE_UPPER,'utf-8') . $m[3];
	return;
      }
    }
  }

  /*
   *
   */
  static function redText($text,$color='redText') { 
    if (empty($text))  return '';
    if  (!is_string($text)) $text = debug::parse_value($text,10,90);
    $reply = '<span class="'.$color.'">'.trim($text).'</span>'; 
    //b_debug::_debug(b_fmt::escape($reply),cnf_dev);
    return $reply;
  }
  

  /*
   * Populate the jQuery accordion:
   * 
   * @synopsis:
   *   b_fmt::accordion('open');                // start the accordion
   *   while($is_enough){
   *      b_fmt::accordion("header $name");    // start the next accordion section
   *      print "whatever goes to the section"; // fill the section
   *   }
   *   b_fmt::accordion('close');               // close the accordion
   */ 
  public static function accordion($legend,$active=0){

    static $header = 'h4', $counter= 0, $opened = False;
    switch($legend){

    case 'open': 
      bJS()->add_js_inline('$("#accordion'.(++$counter)."\").accordion({ header:'$header', autoHeight:false, collapsible:true, active:$active })");
      print  "\n<div id='accordion$counter'> <!-- open accordion$counter -->\n";
      return;

    case 'close':  
      if ($opened) print "</div>\n</div> <!-- close accordion$counter -->\n";
      print "</div>\n" ;
      if (!empty($active)) print $active; 
      print "<div class='clear'></div>";
      $opened = False;
      return;

    default:
      if ($opened) print "</div>\n</div>\n"; 
      $opened = True;
      $css = is_string($active) ? "class='$active'" : "";
      print "<div>\n<$header $css><a href='#'>$legend</a></$header>\n<div>\n";
    }
  }

  
  
  /*
   * Open/Close the collapsible fieldset
   */
  public static function collapsible($collapsed=True,$legend=''){

    static $opened = False;
    bJS()->add_js_file('collapse.js','misc');
    switch($opened){
      
    case False: // ignore the "closing call" if the fieldset is not yet open
      if (func_num_args()){ 
	print 
	  "<fieldset class='collapsible".($collapsed ? ' collapsed' : '')."'>\n".
	  "<legend>$legend</legend>\n";
	$opened = True;
      }
      break;
      
    case True: // auto-close the fieldset if needed
      print "</fieldset>\n";
      $opened = False;
      if (func_num_args()==2) self::collapsible($collapsed,$legend);
    }
  }

  /* Paul's Simple Diff Algorithm v 0.1
   * (C) Paul Butler 2007 <http://www.paulbutler.org/>
   *
   * May be used and distributed under the zlib/libpng license.
   * This code is intended for learning purposes; it was written with short code taking priority over performance. 
   * It could be used in a practical application, but there are a few ways it could be optimized.
   * Given two arrays, the function diff will return an array of the changes.
   * 
   * diffHTML is a wrapper for the diff command, it takes two strings and returns the differences in HTML. 
   */
  public static function diff($old,$new,$dropEmpty=False){
    $maxlen = 0;
    foreach($old as $oindex => $ovalue){
      $nkeys = array_keys($new, $ovalue);
      foreach($nkeys as $nindex){
	$matrix[$oindex][$nindex] = isset($matrix[$oindex - 1][$nindex - 1]) ? $matrix[$oindex - 1][$nindex - 1] + 1 : 1;
	if($matrix[$oindex][$nindex] > $maxlen){
	  $maxlen = $matrix[$oindex][$nindex];
	  $omax = $oindex + 1 - $maxlen;
	  $nmax = $nindex + 1 - $maxlen;
	}
      }
    }

    if ($maxlen == 0) $reply = array(array('d'=>$old, 'i'=>$new));
    else $reply = array_merge(self::diff(array_slice($old, 0, $omax), array_slice($new, 0, $nmax)),
			      array_slice($new, $nmax, $maxlen),
			      self::diff(array_slice($old, $omax + $maxlen), array_slice($new, $nmax + $maxlen)));
    if ($dropEmpty){
      $r = array();
      foreach($reply as $l=>$lines){
	foreach($lines as $c=>$old_new){
	  if (($c !== 'd') && ($c !== 'i')) continue;
	  foreach($old_new as $line) if ($v = trim($line)) $r[$l][$c] .= "$v ";
	}
      }
      return $r;
    }else{
      return $reply;
    }
  }
  
  public static function diffHTML($old, $new,$compare_attributes=False){

    static $synonyms = array("'"=>'&#146;');

    $ret = '';
    $n_diffs = 0;
    foreach(self::diff(self::diffHTML_prepare($old,$compare_attributes), 
		       self::diffHTML_prepare($new,$compare_attributes)) as $k){
      if(is_array($k)){
	$del = implode(' ',@$k['d']);
	$ins = implode(' ',@$k['i']);

	// consider the elements equal if they are synonyms
	list($xd,$xi) = array("del class='highlight-text'", "ins class='highlight-text'");
	foreach($synonyms as $l=>$r){
	  if (strstr($del,$l) && strstr($ins,$r)) list($xd,$xi) = array('','');
	  if (strstr($del,$r) && strstr($ins,$l)) list($xd,$xi) = array('','');
	}
	if ((trim($del) || trim($ins)) && !empty($xd)) $n_diffs++;
	$ret .= ((!empty($del)?x($xd,$del):'').
		 (!empty($ins)?x($xi,$ins):''));
      }else{
	$ret .= $k . ' ';
      }
    }
    if ($n_diffs > 0) return $ret;
    else              return False;
  }
  
  static function diffHTML_prepare($text,$compare_attributes=False){
    if ($compare_attributes){
      $html = self::unEscape(trim($text));
      $html = str_replace("\n"," ",$html);
      $html = preg_replace('/>( *)?</','>-SPLIT-<',$html);
      $html = str_replace('>','>-SPLIT-',$html);
      $html = str_replace('<','-SPLIT-<',$html);
      $html = preg_replace('/-SPLIT-( *)?-SPLIT-/','-SPLIT-',$html);
      $html = preg_replace('/-SPLIT-( *)?-SPLIT-/','-SPLIT-',$html);
      $html = preg_replace('/-SPLIT-( *)?-SPLIT-/','-SPLIT-',$html);
      $reply = array();
      foreach(explode('-SPLIT-',$html) as $l){
	$l = trim($l);
	if (!empty($l)) $reply[] = $l;
      }
     return $reply;
    }else{
      $text = self::escape(trim($text));
      return preg_split("/[\n\s]/",$text);
    }
  }
  

  /*
   * Parse the e-mail address to be RFC_2822 compatible.
   * The order of the e-mails is kept, the duplications are skipped
   */
  public static function RFC_2822($string){
    static $re = "[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*@(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?";
    $reply = array();
    if (is_string($string)){
      foreach(preg_split('/[;,\/]+/',trim(b_fmt::unEscape(strToLower(str_replace('@?.?','@somewhere.earh',$string))))) as $email){
	if (preg_match(";$re;",trim($email),$m)) $reply[] = $m[0];
      }
    }
    //    b_debug::xxx(join(',',array_unique($reply)));
    return array_unique($reply);
  }

  static function money($value){
    if (empty($value)) return '';
    $sign  = ($value < 0 ? '-' : '');
    $value = abs($value);
    if (True){
      $value = (int)$value;
      $m0 = (int) ($value      /1000000000);
      $m1 = (int)(($value - $m0*1000000000)     /1000000);
      $m2 = (int)(($value - $m0*1000000000 - $m1*1000000)/1000);
      $m3 =        $value   %1000;
      if     ($m0) $reply = sprintf("%d,%03d,%03d,%03d",$m0,$m1,$m2,$m3);
      elseif ($m1) $reply = sprintf(     "%d,%03d,%03d",    $m1,$m2,$m3);
      elseif ($m2) $reply = sprintf(          "%d,%03d",        $m2,$m3);
      else         $reply = $value;
      if (empty($reply)) $reply = '0';
      if ($currency = @$GLOBALS['b_fmt::money']) $reply .=  $currency;
      return $sign . $reply;
    }else{
      $format = '%i'.(($currency = $GLOBALS['b_fmt::money'])
		      ? $currency
		      : CONST_currency);
      return $sign . money_format($format, $value);
    }
  }
  
  /*
   * Wrap the long string
   */
  static function longString($string,$normalLine=60,$brEncoded=True) {  
    
    if (strpos($string,'-BR-') !== False)   return $string;
    if (preg_match(';^<[^<^>]*>$;',$string))return $string; 
    
    $string_parsed = array('o'=>'','b'=>'','c'=>'');
    
    $string = trim(b_fmt::unEscape($string));
    if (preg_match(';^(<.*>)(.*)(</.*>)$;',$string,$m)){
      if (strip_tags($m[2]) == strip_tags($string)){
	$string_parsed = array('o'=>$m[1],
			       'b'=>$m[2],
			       'c'=>$m[3]);
      }else{
	// don't try to parse the complicated string, may be later...
	return $string;
      }
    }else{
      $string_parsed['b'] = strip_tags($string);
    }
    
    $formattedstring = array();
    foreach (preg_split("/-ZZZZZZ-\s?/",
			preg_replace("/ (..?.?.?).-ZZZZZZ-/",' \1. ',
				     preg_replace("/\.\s+/",".-ZZZZZZ-",
						  preg_replace("/\b(\w\w?\w?\w?)\./",'\1&#46;',
							       str_replace("\n"," ",$string_parsed['b']))))) as $line) {
      if (strlen($line) > $normalLine){
	while (strlen($line) > (int)(1.5 * $normalLine)) {
	  $n = strpos($line,' ',$normalLine);
	  $formattedstring[] = b_fmt::escape(substr($line, 0,$n));
	  $line= substr($line,$n+1);
	}
      }
      if ($line=trim($line)) $formattedstring[] = $line;
    //if ($line=trim($line)) $formattedstring[] = nbsp($line);
    }    
    return
      $string_parsed['o'].
      join(($brEncoded?'-BR-':'<br/>'),$formattedstring).
      $string_parsed['c'];
  }
  
  /*
   * compact the HTML source as much as i can
   */
  static function compact_html($html,$keepQuotes=False){
    //    $input = b_fmt::diffHTML_prepare(str_replace('"',"'",$html),True);
    $escapes = array("/[\n\r]+/"  =>' ',
		     '/&#44;/'    => ',',
		     '/> *</'     => '><',
		     '/\s(=|==|!=)\s/'=> '\\1',
		     '/\s?(\+|\(|\)|{|}|===|!==|,|;)\s?/'=> '\\1',
		     '/<!--[^>]*-->/' => '',
		     ';/[\*][^/]*[\*]/;' => '',
		     "/\s+/"  =>' ',
		     );
    
    // 
    // Remove urls, forms, etc. which is not needed in the pdf file
    // Replace single quotes by double ones - tcpdf restriction
    if ($keepQuotes === 'pdf'){ 
      foreach(array('a','form','font','pre','span',) as $i){
	$escapes = array_merge($escapes,
			       array(";<$i>;"        => '',
				     ";</$i>;"       => '',
				     ";<-SLASH-$i>;" => '',
				     ";<$i [^>]*>;"  => ''));
      }
      $html = str_replace("'",'"',$html);
    }elseif (!$keepQuotes){
      $html = str_replace("'",'"',$html);
    }

    $safe = array('-BR-'    =>'<br>',
		  '-SLASH-' =>'/',
		  '-QUOTE-' =>"'",
		  '-QUOTD-' =>'"');
    foreach($safe as $t=>$f) $html = str_replace($f,$t,$html);
    $reply = preg_replace(array_keys($escapes),
			  array_values($escapes),
			  $html);
    foreach($safe as $f=>$t) $reply = str_replace($f,$t,$reply);
    
    foreach(array("'"=>'"',
		  '"'=>"'",
		  ) as $f=>$t){
      if (stripos($reply,"${f}document.location.href=${f}") !== False){
	$reply = preg_replace("/document.location.href=.([^']*)./",
			       "document.location.href=${t}\\1${t}",$reply);
      }
    }
    //    b_debug::print_r(b_fmt::diff($input,b_fmt::diffHTML_prepare(str_replace('"',"'",$reply)),True),'diff');
    return trim($reply);
  }
  
  /*
   *
   */
  private static function escape_init(&$mode=''){
    $escape_url = array(' ' => '%20',			'<' => '%3C',
			'>' => '%3E',			'#' => '%23',
			'%' => '%25',			'{' => '%7B',
			'}' => '%7D',			'|' => '%7C',
			'\\'=> '%5C',			'^' => '%5E',
			'~' => '%7E',			'[' => '%5B',
			']' => '%5D',			'`' => '%60',
			';' => '%3B',			'/' => '%2F',
			'?' => '%3F',			':' => '%3A',
			'@' => '%40',			'=' => '%3D',
			'&' => '%26',			'$' => '%24');
    
    $escapes = array(' ' =>'&nbsp;',
		     '"' =>array('&quot;', '&#34;', '&#034;'),
		     "'" =>array('&apos;', '&#39;', '&#039;'),
		     '[' =>array(          '&#91;', '&#091;'),  
		     ']' =>array(          '&#93;', '&#093;'),  
		     ',' =>array(          '&#44;', '&#044;'),  
		     '.' =>array(          '&#46;', '&#046;'),
		     '<' =>array('&lt;',   '&#60;', '&#060;'),
		     '>' =>array('&gt;',   '&#62;', '&#062;'),
		     );
    
    if($mode == 'url'){
      return $escape_url;
    }else{
      if (!empty($mode)){
	$mode = '';
	foreach(array('<','>') as $i) unset($escapes[$i]);
      }
      return $escapes;
    }
  }

  /*
   * Escape the symbols. Never escape "space"
   */
  static function escape($line,$mode='') {
    $line = self::unEscape($line,$mode);
    foreach (self::escape_init($mode) as $f=>$t){
      if (empty($mode) && ($f == ' '))   continue;
      if (!is_array($t)) $t = array($t);
      $line = str_replace($f,$t[0],$line);
    }
    return $line;
  }
  
  /*
   * Unescape symbols
   */
  static function unEscape($line,$mode=''){
    foreach (self::escape_init($mode) as $symbol=>$codes){
      if (!is_array($codes)) $codes = array($codes);
      //      foreach($codes as $f)  $line = str_replace($f,$symbol,$line);
      $line = str_ireplace($codes,$symbol,$line);
    }
    return $line;
  }
  
  static function join($a){
    if (b_fmt::isArrayAssociative($a)){
      return b_fmt::joinX(',',$a,'','=>');
    }elseif(is_array($a)){
      foreach($a as $k=>$v) if (is_array($v)) $a[$k] = 'array()';
      return join(',',array_values($a));
    }elseif(cnf_dev){
      b_debug::traceBack();
    }
  }
  
  /**
   * Convert an associative array into a URL-like string,
   * i.e. array(a=>aaa,b=>bbb) -> "a='aaa'&b='bbb'"
   */
  static function joinX($glue,$a,$glue2='',$glue3='='){
    if (empty($a)) $a = array(); 
    $isArrayAssociative = b_fmt::isArrayAssociative($a);
    $aa = array();
    foreach ($a as $k=>$v) {
      if (is_array($v))    $v = b_fmt::join($v);
      elseif(is_object($v))$v = get_class($v);
      else                 $v = trim($v);
      if($isArrayAssociative){
	if (in_array($k,array('password','pwd'))) $v = 'x';
	if    ($glue2==':')   $item = "$k: $v";
	elseif($glue2==' ')   $item = "$k  $v";
	elseif($k=='')        $item = "$glue2$v$glue2";
	else                  $item = "$k$glue3$glue2$v$glue2";
	$aa[] = $item;
      }else{
	$aa[] = $v;
      }
    }
    return join($glue,$aa);
  }
  
  /*
   * @param return "<$glue><$glue2>$key</$glue2><i>$value</i></$glue>";
   */
  static function joinMap($glue,$items=array(),$glue2=''){
    if (!is_array($items)) if(cnf_dev) b_debug::traceBack('input is NOT array');

    // build closing attribute
    if (strpos($glue,' ') === False) $glue_close = $glue;
    else                        list($glue_close,$x) = explode(' ',$glue,2);

    $r = array();
    if (empty($glue2)){
      foreach($items as $item) $r = array_merge($r,array("<$glue>",$item,"</$glue_close>",''));
    }else{
      foreach($items as $k=>$v) $r[] = "<$glue><$glue2>&lt;".self::escape($k)."/&gt;</$glue2>&nbsp;<i>$v</i></$glue_close>";
    }
    return join("\n",$r);
  }

  public static function strcut($text, $maxL, $textOverflow='', $doNotCutWord=True) {
    if (strlen($text) > $maxL) $text = str_replace('&nbsp;',' ',$text);
    return self::htmlwrap($text,$maxL,$textOverflow,$cutAfterFirstWrap=True,$exact_cut=!$doNotCutWord);
  }
  /*
   * 'wordwrap', but ignoring html tags
   */
  public static function htmlwrap(&$str, $maxLengthP, $char='<br/>', $cutAfterFirstWrap=True, $exact_cut=False){
    $newStr = '';
    $lastspace = $count = 0;
    $ignore_rest = $openTag = false;
    $lenstr = strlen($str);
    $maxLength = $maxLengthP;
    for($i=0; $i<$lenstr; $i++){
      if($str{$i} == '<'){
	  $openTag = true;
	  $newStr .= $str{$i};
	  continue;
      }
      if (!$ignore_rest || $openTag) $newStr .= $str{$i};
      if(($openTag) && ($str{$i} == '>')){
	$openTag = false;
	continue;
      }
      if(!$openTag){
	if($str{$i} == ' '){
	  if ($count == 0) {
	    if (!$ignore_rest) $newStr = substr($newStr,0, -1);
	    continue;
	  } else {
	    $lastspace = $count + 1;
	  }
	}
	$count++;
	if(($count==$maxLength) && !$exact_cut){
	  if (@$str{$count} != ' ') $maxLength++;
	}else{
	  $maxLength = $maxLengthP;
	}
	if($count==$maxLength){
	  if (@$str{$i+1} != ' ' && $lastspace && ($lastspace < $count)) {
	    $tmp = ($count - $lastspace)* -1;
	    if (!$ignore_rest || $openTag) $newStr = substr($newStr,0, $tmp) . $char;
	    $ignore_rest = $cutAfterFirstWrap;
	    if (!$ignore_rest || $openTag) $newStr .= substr($newStr,$tmp);
	    $count = $tmp * -1;
	  } else {
	    if (!$ignore_rest) $newStr .= $char;
	    $ignore_rest = $cutAfterFirstWrap;
	    $count = 0;
	  }
	  $lastspace = 0;
	}
      } 
    }
    return $newStr;
  }

  // Escape the REGEXP characters
  private static $escape = array('\\'=> '-BKS-',
				 ';' => '-QQQ-',
				 '$' => '-BUKS-',
				 '+' => '-PLUS-',
				 '*' => '-STAR-',
				 "'" => '-QUOT-',
				 '"' => '-QUOD-',
				 '?' => '-QUES-',
				 '(' => '-OPNB-',
				 '[' => '-OPNS-',
				 '{' => '-OPNQ-',
				 ')' => '-CLSB-',
				 '}' => '-CLSQ-',
				 ']' => '-CLSS-',
				 // '/' => '-SLSH-',
				 );
  

  static function strcut_OBSOLETE($textM, $maxL, $postfix='', $doNotCutWord=True) {
    static $recursion = False;

    if (is_numeric($textM))                  return $textM; 
    if (!is_string($textM) || empty($textM)) return $textM; 
    if (stristr($textM,'http:') !== False)   return $textM; 
    
    if ($recursion) return $textM;    // sanity
    $recursion = True;
    
    // Extract "visible" text from the input string
    foreach(array("\n","\r",",ZZZ") as $esc) $textM = str_replace($esc,' ',$textM);

    // Detect html text, preserve the html tags
    $ao = $ac = '';
    if ((strpos($textM,'<') !== False) && (strpos($textM,'>') !== False)){
      if (True){
	$text = strip_tags($textM); 
	if (empty($text)) $text = $textM;
	elseif(count($a=explode($text,$textM)) == 2) list($ao,$ac) = $a;
	else  return $text;
      }else{
	foreach (self::$escape as $f=>$t)        $textM = str_replace($f,$t,trim($textM));
	if (strlen($textM)<999 && preg_match(";^(<[^>]*>)(.*)(</[^>]*>)$;",$textM,$mm)){
	  list($ao,$text,$ac) = array($mm[1],$mm[2],$mm[3]);
	  foreach (self::$escape as $t=>$f){
	    $ao   = str_replace($f,$t,$ao);
	    $ac   = str_replace($f,$t,$ac);
	    $text = str_replace($f,$t,$text);
	  }
	  // remove html formatting from the inner text
	  $text = strip_tags($text); 
	}
      }
    }else{
      $text = $textM;
    }

    if ($doNotCutWord) {    // Optionally do not cut the complete words
      if (strpos($text,'<') === False){
	$reply = '';
	$text = str_replace('  ',' ',str_replace('  ',' ',$text));
	foreach(explode(' ',$text) as $l){
	  if (strlen($reply) < $maxL) 	  $reply = trim("$reply $l");
	}
	$text = $reply . (($reply == $text) ? '' : $postfix);
      }else{
	if (strlen($text) > $maxL + 2*strlen($postfix)){
	  $pos = 999;
	  foreach(array('&nbsp;',' ','/','-',',') as $splitter){
	    if (($p=strpos($text, $splitter, $maxL))>0) $pos = min($pos,$p);
	  }
	  if ($pos < 900) $text = substr($text,0,$pos).$postfix;
	}
      }
    }else{
      if (strlen($text) > $maxL) $text = substr($text,0,$maxL).$postfix;
    }
    $reply = $ao.$text.$ac;

    $recursion = False;
    return $reply;
  }

  static function cyr_detect_encoding($str) {
    $charsets = Array('KOI8-R'     => 0,
		      'WIN-1251'   => 0,
		      'CP866'      => 0,
		      'ISO-8859-5' => 0,
		      'MAC'        => 0
		      );
    for ( $i = 0, $length = strlen($str); $i < $length; $i++ ) {
      $char = ord($str[$i]);
      //non-Russian characters
      if ($char < 128 || $char > 256) continue;
      
      if (($char > 159 && $char < 176) || ($char > 223 && $char < 242)) $charsets['CP866']+=3;
      if ( $char > 127 && $char < 160) $charsets['CP866']+=1;
      
      if ($char > 191 && $char < 223) $charsets['KOI8-R']+=3;
      if ($char > 222 && $char < 256) $charsets['KOI8-R']+=1;
      
      if ($char > 223 && $char < 256) $charsets['WIN-1251']+=3;
      if ($char > 191 && $char < 224) $charsets['WIN-1251']+=1;
      
      if ($char > 221 && $char < 255) $charsets['MAC']+=3;
      if ($char > 127 && $char < 160) $charsets['MAC']+=1;
      
      if ($char > 207 && $char < 240) $charsets['ISO-8859-5']+=3;
      if ($char > 175 && $char < 208) $charsets['ISO-8859-5']+=1;
      
    }
    arsort ($charsets);
    if (key($charsets) == 'MAC') return '';  // What is MAC ?
    else                         return key($charsets);
  }

  /*
   *
   */
  static function isArrayAssociative(&$array){
    $reply = False;
    if (is_object($array)){
      $reply = True;
    }elseif(is_array($array)){
      $reply = !empty($array) && ($array !== array_values($array));
    }else{
      b_debug::internalError("$array in not an array",$array);
    }
    return $reply;
  }

  /*
   *
   * $a[0] and $a[1] are the width and height of the image.
   * $a[2] has the image type.
   *
   * Other image formats include :
   *
   * [IMAGETYPE_UNKNOWN] => 0
   * [IMAGETYPE_GIF]     => 1
   * [IMAGETYPE_JPEG]    => 2
   * [IMAGETYPE_PNG]     => 3
   * [IMAGETYPE_SWF]     => 4
   * [IMAGETYPE_PSD]     => 5
   * [IMAGETYPE_BMP]     => 6
   * [IMAGETYPE_TIFF_II] => 7
   * [IMAGETYPE_TIFF_MM] => 8
   * [IMAGETYPE_JPC]     => 9
   * [IMAGETYPE_JPEG2000]=> 9
   * [IMAGETYPE_JP2]     => 10
   * [IMAGETYPE_JPX]     => 11
   * [IMAGETYPE_JB2]     => 12
   * [IMAGETYPE_SWC]     => 13
   * [IMAGETYPE_IFF]     => 14
   * [IMAGETYPE_WBMP]    => 15
   * [IMAGETYPE_XBM]     => 16
   * [IMAGETYPE_ICO]     => 17 // mpeg videos are detected as IMAGETYPE_ICO !!
   * [IMAGETYPE_COUNT]   => 18
   */
  public static $image_size = array();
  public static $image_size_string = '';
  static function isImage($path,$get_size=False) {
    if ($reply = ((file_exists($path) && !is_dir($path)) &&
		  in_array(($exif=exif_imagetype($path)),array(IMAGETYPE_GIF, IMAGETYPE_JPEG,IMAGETYPE_PNG, IMAGETYPE_BMP)))){
      // Optionally get the image size
      static::$image_size = array();
      static::$image_size_string = '';
      if ($get_size){
	$a = getimagesize($path);
	static::$image_size = array($a[0],$a[1]);
	if((int)$get_size > 1){
	  $max_size = $get_size;
	  if (  ($a[1] <= $max_size) && ($a[0] <= $max_size)) $c = 1;
	  elseif($a[1] <= $a[0])                              $c = $max_size / $a[0];
	  else                                                $c = $max_size / $a[1];
	  $w = min($a[0],(int)round($a[0]*$c));
	  static::$image_size_string = sprintf("max-width='%s' max-height='%s'",
					       (int)round($a[0]*$c),
					       (int)round($a[1]*$c));
	  static::$image_size_string = sprintf("width='%s'",
					       min($a[0],(int)round($a[0]*$c)));
	  b_debug::_debug("size=".var_export(static::$image_size,True).
			  " string=".var_export(static::$image_size_string,True),cnf_dev);
	}else{
	  b_debug::_debug("size=".var_export(static::$image_size,True),cnf_dev);
	}
      }
    }
    b_debug::_debug(var_export($reply,true)." - $path",cnf_dev);
    return $reply;
  }

  
  /*
   Sorry for my English
   
   100% worked function for converting string to utf-8.
   In this implementation support main Cyrillic encoding (cp1251, koi8-r, iso8859-5, cp866, mac)
   For supporting another code pages - just add needed code page in $recode array
   (codes in UCS-4. Add just second part of code-table).
   Second argument for this function for Cyrillic code pages -
   like in convert_cyr_string function ('k','w', 'i', 'a','d','m')
   Write in accordance with rfc2279
   
   Created by Andrey A Sidorenko aka sidor
  */
  static function cyr_utf8($str,$from) {
    if (strToUpper($from) == 'ISO-8859-5') $from = 'i';
    if (strToUpper($from) == 'KOI8-R')     $from = 'k';
    $outstr='';
    $recode=array();
    $recode['k']=array(0x2500,0x2502,0x250c,0x2510,0x2514,0x2518,0x251c,0x2524,
		       0x252c,0x2534,0x253c,0x2580,0x2584,0x2588,0x258c,0x2590,
		       0x2591,0x2592,0x2593,0x2320,0x25a0,0x2219,0x221a,0x2248,
		       0x2264,0x2265,0x00a0,0x2321,0x00b0,0x00b2,0x00b7,0x00f7,
		       0x2550,0x2551,0x2552,0x0451,0x2553,0x2554,0x2555,0x2556,
		       0x2557,0x2558,0x2559,0x255a,0x255b,0x255c,0x255d,0x255e,
		       0x255f,0x2560,0x2561,0x0401,0x2562,0x2563,0x2564,0x2565,
		       0x2566,0x2567,0x2568,0x2569,0x256a,0x256b,0x256c,0x00a9,
		       0x044e,0x0430,0x0431,0x0446,0x0434,0x0435,0x0444,0x0433,
		       0x0445,0x0438,0x0439,0x043a,0x043b,0x043c,0x043d,0x043e,
		       0x043f,0x044f,0x0440,0x0441,0x0442,0x0443,0x0436,0x0432,
		       0x044c,0x044b,0x0437,0x0448,0x044d,0x0449,0x0447,0x044a,
		       0x042e,0x0410,0x0411,0x0426,0x0414,0x0415,0x0424,0x0413,
		       0x0425,0x0418,0x0419,0x041a,0x041b,0x041c,0x041d,0x041e,
		       0x041f,0x042f,0x0420,0x0421,0x0422,0x0423,0x0416,0x0412,
		       0x042c,0x042b,0x0417,0x0428,0x042d,0x0429,0x0427,0x042a
		       );
    $recode['w']=array(
		       0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,
		       0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
		       0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
		       0x0000,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
		       0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,
		       0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
		       0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,
		       0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
		       0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
		       0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
		       0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
		       0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
		       0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
		       0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
		       0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
		       0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F
		       );
    $recode['i']=array(
		       0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,
		       0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
		       0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,
		       0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
		       0x00A0,0x0401,0x0402,0x0403,0x0404,0x0405,0x0406,0x0407,
		       0x0408,0x0409,0x040A,0x040B,0x040C,0x00AD,0x040E,0x040F,
		       0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
		       0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
		       0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
		       0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
		       0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
		       0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
		       0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
		       0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F,
		       0x2116,0x0451,0x0452,0x0453,0x0454,0x0455,0x0456,0x0457,
		       0x0458,0x0459,0x045A,0x045B,0x045C,0x00A7,0x045E,0x045F
		       );
    $recode['a']=array(
		       0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
		       0x0418,0x0419,0x041a,0x041b,0x041c,0x041d,0x041e,0x041f,
		       0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
		       0x0428,0x0429,0x042a,0x042b,0x042c,0x042d,0x042e,0x042f,
		       0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
		       0x0438,0x0439,0x043a,0x043b,0x043c,0x043d,0x043e,0x043f,
		       0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,
		       0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
		       0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,
		       0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
		       0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,
		       0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
		       0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
		       0x0448,0x0449,0x044a,0x044b,0x044c,0x044d,0x044e,0x044f,
		       0x0401,0x0451,0x0404,0x0454,0x0407,0x0457,0x040e,0x045e,
		       0x00b0,0x2219,0x00b7,0x221a,0x2116,0x00a4,0x25a0,0x00a0
		       );
    $recode['d']=$recode['a'];
    $recode['m']=array(
		       0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
		       0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
		       0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
		       0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
		       0x2020,0x00B0,0x00A2,0x00A3,0x00A7,0x2022,0x00B6,0x0406,
		       0x00AE,0x00A9,0x2122,0x0402,0x0452,0x2260,0x0403,0x0453,
		       0x221E,0x00B1,0x2264,0x2265,0x0456,0x00B5,0x2202,0x0408,
		       0x0404,0x0454,0x0407,0x0457,0x0409,0x0459,0x040A,0x045A,
		       0x0458,0x0405,0x00AC,0x221A,0x0192,0x2248,0x2206,0x00AB,
		       0x00BB,0x2026,0x00A0,0x040B,0x045B,0x040C,0x045C,0x0455,
		       0x2013,0x2014,0x201C,0x201D,0x2018,0x2019,0x00F7,0x201E,
		       0x040E,0x045E,0x040F,0x045F,0x2116,0x0401,0x0451,0x044F,
		       0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
		       0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
		       0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
		       0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x00A4
		       );
    $and=0x3F;
    for ($i=0;$i<strlen($str);$i++) {
      $octet=array();
      if (ord($str[$i])<0x80) {
	$strhex=ord($str[$i]);
      } else {
	$strhex=$recode[$from][ord($str[$i])-128];
      }
      if ($strhex<0x0080) {
	$octet[0]=0x0;
      } elseif ($strhex<0x0800) {
	$octet[0]=0xC0;
	$octet[1]=0x80;
    } elseif ($strhex<0x10000) {
	$octet[0]=0xE0;
	$octet[1]=0x80;
	$octet[2]=0x80;
    } elseif ($strhex<0x200000) {
	$octet[0]=0xF0;
	$octet[1]=0x80;
	$octet[2]=0x80;
	$octet[3]=0x80;
      } elseif ($strhex<0x4000000) {
	$octet[0]=0xF8;
	$octet[1]=0x80;
	$octet[2]=0x80;
	$octet[3]=0x80;
	$octet[4]=0x80;
      } else {
	$octet[0]=0xFC;
	$octet[1]=0x80;
	$octet[2]=0x80;
	$octet[3]=0x80;
	$octet[4]=0x80;
	$octet[5]=0x80;
      }
      for ($j=(count($octet)-1);$j>=1;$j--) {
	$octet[$j]=$octet[$j] + ($strhex & $and);
	$strhex=$strhex>>6;
      }
      $octet[0]=$octet[0] + $strhex;
      for ($j=0;$j<count($octet);$j++) {
	$outstr.=chr($octet[$j]);
      }
    }
    return($outstr);
  }
  
  /*
   * curl/7.21.4 (x86_64-apple-darwin12.2.0) libcurl/7.21.4 OpenSSL/0.9.8y zlib/1.2.5 libidn/1.20
   */
  static function parse_HTTP_USER_AGENT() {
    static $windowsCodes = array('NT 4.0' => 'NT',
				 'NT 5.0' => '2000',
				 'NT 5.1' => 'XP',
				 'NT 5.2' => 'XP x64',
				 'NT 6.0' => 'Vista',
				 'NT 6.1' => '7');
    preg_match ("/(curl|Lynx|Mozilla\S+|Opera\S+).*((Linux|Win|Mac_|Mac |libwww|libcurl)[\s\w\.\-\/]+)/", 
		$_SERVER["HTTP_USER_AGENT"],
		$s);
    $OS = (in_array(@$s[1],array('curl','Lynx'))
	   ? 'Linux' 
	   : preg_replace('/Linux.*/','Linux',@$s[2]));
    
    if (preg_match ("/(Shiira|Chrome\S+|Safari|MSIE \S+|Firefox\S+|Camino\S+|SeaMonkey\S+|Opera\S+|curl)/",
		    str_replace(';',' ',@$_SERVER["HTTP_USER_AGENT"]),
		    $b)) $browser = $b[1];
    else                 $browser = $s[1]; 
    if ($browser == 'Safari') 
      $browser .= ' '.preg_replace('/ .*/','',preg_replace("/.*Version\//",'',@$_SERVER["HTTP_USER_AGENT"]));
    if (preg_match('/mobile/i',@$_SERVER["HTTP_USER_AGENT"])) 
      $browser .= '/mobile';
    
    // replace Windows "internal" codes by "external" ones
    foreach ($windowsCodes as $cint=>$cext) $OS = str_replace($cint,$cext,$OS);
    if (preg_match ("/(iPad)/",@$_SERVER["HTTP_USER_AGENT"],$ss))               $OS = 'iPad';
    if (preg_match ("/(iPhone OS \S+)\s/",@$_SERVER["HTTP_USER_AGENT"],$ss))    $OS = str_replace(' OS ',' ',$ss[1]);
    if (preg_match ("/(Android \S+);?\s/",@$_SERVER["HTTP_USER_AGENT"],$ss))    $OS = $ss[1];
    
    $core_HTTPoscode = 'linux';
    if (preg_match('/windows/i',$OS))       $core_HTTPoscode = 'windows';
    if (preg_match('/(iphone|os x)/i',$OS)) $core_HTTPoscode = 'macosx';
    $core2hostdb = array('linux'   => 'lx',
			 'macosx'  => 'osx',
			 'windows' => 'win',
			 '?'       => 'pda',
			 '??'      => 'winlx',
			 '???'     => '_unknown');
    $GLOBALS['core_guessOS'] = $core2hostdb[$core_HTTPoscode]; 
    
    if (preg_match("/browser=(\S*) /",@$_SERVER["HTTP_USER_AGENT"],$m)) $engine = $m[1];
    $reply = array (str_replace('_','.',$OS),$browser);
    return $reply;
  }
}


/*
 * PDF stuff
 */
class b_pdf{

  /*
   * Check that the file is really in the PDF format and readable.
   */
  private static $cache_pdf = Null; 
  static $isPDF_error = '';
  static function isPDF($file,$display_name=''){

    if (empty($display_name)) $display_name = basename($file);

    // Keep the answer for the whole session
    if (!is_object(self::$cache_pdf)){
      locateAndInclude('b_cache');
      self::$cache_pdf = new b_cache_S(__FUNCTION__);
    }

    // Get the answer and save it to the session
    if (self::$cache_pdf->wasSet($file)){
      list($isPDF,self::$isPDF_error) = self::$cache_pdf->get();
    }else{
      bTiming()->cpu(__METHOD__);
      self::$isPDF_error = "\"$display_name\" is not a pdf file";
      // PDF files have standard 1-st line
      $isPDF = trim(`grep --text ^%PDF $file`);
      if ($isPDF){
	// The file must be "CONCAT-enable", i.e. not signed, etc.
	$tmp_file = tempnam("/tmp", __FUNCTION__);
	$cmd = b_os::path() . "pdftk $file cat output /dev/null > $tmp_file 2>&1";
	b_debug::xxx($cmd);
	system($cmd,$exit);
	self::$isPDF_error = (($isPDF = ($exit == 0))
			      ? ""
			      : $display_name.' is not an usable pdf file.<br/>Please check the file validity, remove the signature, etc...');
	unlink($tmp_file);
      }
      // Argument by reference, hence this:
      $save = array($isPDF,self::$isPDF_error);
      self::$cache_pdf->set($save);
      bTiming()->cpu();
    }
    if (!$isPDF) MSG::WARNING_T(self::$isPDF_error);
    return !empty($isPDF);
  }
  
  /**
   * Create a pdf file from a text file
   *
   * @param $txt2pdf
   * @param $file the reference file
   * @param location of the python conversion script
   */
  static function txt2pdf($txtFile,$file,$txt2pdfExec='../../bin/txt2pdf.py') {
    $txt2pdfExec = realpath(dirname($file).DIRECTORY_SEPARATOR.$txt2pdfExec);
    if ($txt2pdfExec && is_file($txt2pdfExec)){
      $pdfFile = b_os::getFilename_noExt($txtFile) . ".pdf";
      $errFile = "$txtFile.err";
      b_os::exec($cmd=b_os::path().join(';',array("rm -rf $txtFile.latin1",
						  "iconv -f utf-8 -t latin1 < $txtFile > $txtFile.latin1",
						  "python $txt2pdfExec -I -A4 -o $pdfFile $txtFile.latin1 > /dev/null 2>$errFile")));
      return $pdfFile;
    }else{
      return Null;
    }
  }
  
  /*
   * Wrapper around bTCPDF
   */
  static function html2pdf($output_file,$html,$subHeader=''){
    
    locateAndInclude('bTCPDF');
    $pdf = new bTCPDF();
    $pdf->subHeader = $subHeader;
    $pdf->html2pdf($output_file, $html);
    return file_exists($output_file);
  }
  
  static function join($files=array(),$output){
    b_os::exec(b_os::path()."pdftk ".join(' ',$files)." cat output $output 2>&1");
  }
}


/*
 * Set of debug helpers
 */
if (!class_exists('myPear_base',False)) require_once "../myPear/includes/myPear_base.inc";
class b_debug extends myPear_base{

  public static $backtraceData = '';

  /**
   * Give the name and arguments of the function/method calling sequence
   */
  public static function __($flags=array(),$class=Null){
    static $inside = 0;    if ($inside++) die("recursion in b_cnf::__($flags)");
    $includeClass = True;
    $includeLine  = False;

    // Parse input arguments
    if (!is_array($flags)) $flags = array($flags);
    foreach($flags as $item){
      if (is_bool($item))      $includeClass = $item;
      elseif ($item==='l')     $includeLine  = True;
      elseif (is_object($item))$clas = get_class($item);
      else                     $level        = (int)$item;
    }
    if (empty($level)) $level = 1;

    // Get the class identifiers (name, ID, mode)
    if (is_object($class)){
      $clas = self::parse_value($class);
    }elseif (is_string($class) && !empty($class)){
      $clas = $class;
    }

    $calls = debug_backtrace();
    while (True){
      if (isset($calls[$level])){
	$call = $calls[$level];
	break;
      }else{
	$level--;
      }
    }

    if (@$call['type']=='::') $includeClass = True;
    if (@$call['type']=='->' && !empty($clas) && $clas!='b_debug')   $call['class'] = $clas;

    if (!is_array($call['args'])) $call['args'] = array();
    $args = array(); 
    foreach($call['args'] as $a) {
    /*
      $classArgs = array();
      if (is_object($a) && ($id=@$a->ID))  $classArgs[] = $id;
      if (is_object($a) && ($id=@$a->MODE))$classArgs[] = $id;
      if     ($classArgs)      $args[] = get_class($a)."(".join(',',$classArgs).")";      
      elseif (is_object($a))   $args[] = get_class($a);      
      elseif (is_array($a))    $args[] = 'array('.b_fmt::join($a).')';
      else                     $args[] = $a;
    */
      $args[] = self::parse_value($a);
    }
    $reply = str_replace('[','',
			 str_replace(']','',
				     str_replace('bForm_','',join('',array((empty($call['class']) || !$includeClass ? '' : @$call['class'].@$call['type']),
									   @$call['function'], 
									   "(".join(',',$args).")",
									   ($includeLine ? ' line '.@$call['line'] : ''))))));

    $inside = 0;  
    return $reply;
  }

  /*
   *
   */
  public static function report_bug($text=''){
    self::internalError(__METHOD__.": $text");
  }
  
  /*
   *
   */
  private static $was = array();
  public static function watchDog($now,$class='all',$condition=True,$verbose=False){
    if ($condition){
      $id=(is_object($class) ? str_replace('bForm_','',get_class($class).'('.@$class->ID.')') : $class);
      if (!is_object($class)) $class = True;
      
      if (!isset(self::$was[$id])){
	self::$was[$id] = $now;
	if ($verbose) self::xxx("init($id) with ---------> '".var_export($now,True)."'",array(2,$class,True,'redText'));
      }
      if (self::$was[$id] !== $now){
	static $case = array(); if (empty($case[$id])) $case[$id] = 0;
	self::traceBack(sprintf($id."-%s: \"%s\" --> \"%s\"",(++$case[$id]),var_export(self::$was[$id],True),var_export($now,True)));
      }
      self::$was[$id] = $now;
    }
  }

  /*
   *
   */
  static function get_callStack($drop_args=False){
    if (defined('DEBUG_BACKTRACE_IGNORE_ARGS') && $drop_args){
      $stack = debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);
    }else{
      $stack = debug_backtrace();
    }

    $reply = array();
    foreach ($stack as $r){
      if (in_array($r['function'],array(__FUNCTION__,'locateAndInclude','_fromCache','bDB_timing',
					'require_once','cpu','call_user_func_array',))) continue;
      if (in_array($r['function'],array('call_user_func_array','module_invoke_all',
					'_menu_check_access','menu_execute_active_handler')))                break;
      if (empty($r['function'])) continue;
      $reply[] = 
	($drop_args||empty($r['class']) ? '' : $r['class'].$r['type']) .
	$r['function'].
	($drop_args ? '' : '('.join(',',$r['args']).')');
    }
    return $reply;
    return array_reverse($reply);
  }

  static function _($txt,$t2='',$t3='',$forced=False,$color='blackText') {
    if (is_string($t3) && ($t3 == 'M'))   {$txt = "----------- $t2"; $t2=$t3=''; $forced = cnf_show_m; $color='fuchsiaText'; }
    if (is_bool($t2) && count(func_get_args()==2)) { $t2=''; $forced=True; }
    if (cnf_debug || $forced){
      if (is_null($t2))  $t2 = 'NULL';
      if (is_null($t3))  $t3 = 'NULL';
      if (is_bool($t2))  $t2 = ($t2 ? 'True' : 'False');
      if (is_bool($t3))  $t3 = ($t3 ? 'True' : 'False');
      if (is_array($t2)) $t2 = 'array('.join(',',$t2).')';
      if (is_array($t3)) $t3 = 'array('.join(',',$t3).')';
      if (trim($t2))     $t2 = '['.trim($t2).'] ';
      if ($txt = $txt.($t2.$t3 ? $t2.b_fmt::_($t3):'')){
	if ($color) $txt = "<span class='$color'>$txt</span>";
	MSG::DEBUG($txt);
      }
    }
  }

  /*
   * Generic debug printout function
   */
  private static $xxx_cache = array();
  private static $s_e_depth = 0;
  public  static $starting_exiting = False;
  static function xxx($text=' ',$flags=array()){

    if (!is_array($flags)) $flags = array($flags);
    foreach($flags as $k=>$v) if (is_numeric($v)){ $levelOK = True; $flags[$k]=$v-1;}elseif(is_bool($v)){$debugOK=True;}
    if (empty($levelOK)) $flags[] = 2;
    if (empty($debugOK)) $flags[] = True;

    return self::_debug($text,$flags);

    static $default_flags = array('fuchsiaText',2);
    
    if (!defined('cnf_dev') || !defined('YBhere') || (!cnf_dev && !YBhere)) return;
    $toPrint = Null;
    
    // Start / End a block
    $s_e_text = $text;
    if (self::$starting_exiting = is_string($s_e_text)){
      $s_e_s = (strpos($s_e_text,starting) !== False);
      $s_e_e = (strpos($s_e_text,exiting)  !== False);
      if (self::$starting_exiting = ($s_e_s || $s_e_e)){
	if ($s_e_e) self::$s_e_depth--;
	$toPrint = True;
	$color = 'orangeText';
      }
    }
    
    if (!self::$starting_exiting){
      // Object
      if (is_object($text)) $text = (method_exists($text,'__toString')
				     ? $text->__toString()
				     : "Object ".get_class($text));
      
      switch(func_num_args()){
      case 0:
	$flags = $default_flags;      // no arguments, mark a function entry point
	$text = ' ';
	break;
	
      case 1:
	$flags = array_merge($default_flags,array(True)); // always print 
	break;
	
      case 2:
      default:
	if(empty($flags))      $flags = $default_flags;  
	if(!is_array($flags))  $flags = array_merge($default_flags,array($flags)); 
      }
      
      if (!is_array($flags)) self::internalError('xxx');
      
      $text = b_fmt::_($text);
      foreach($flags as $k => $item){
	if ($k === 'name'){
	  $class  = $item;
	}elseif(is_bool($item)){     
	  $toPrint = (bool)$item; 
	}elseif(is_numeric($item)){
	  $level  = (int)$item;
	}elseif(is_object($item)){             
	  $class  = $item;
	}elseif(is_string($item)){
	  switch(strToUpper($item)){
	  case 'DB':
	    $stringFormat = 'DB';
	    break;
	  case 'I': // Instance
	    $stringFormat = 'I';
	    if ($toPrint===Null) $toPrint = cnf_show_i;
	    $level = 2;
	    break; 
	  case 'C': // configuration - options, variables
	    $stringFormat = 'C';
	    if ($toPrint===Null) $toPrint = (bool)@$_GET['showc'] || (bool)@$_SESSION[CONST_cnfOptions]['showc'];
	    $level = 2;
	    $color='maroonText';
	    break;
	  case 'L': // Load & include
	    $stringFormat = 'L';
	    if ($toPrint===Null) $toPrint = cnf_show_l;
	    break;
	  case 'T': // Just the text, no calling sequence
	    $stringFormat = 'Text';
	    break;
	  case 'AA': // APIaccess debug
	    $stringFormat = 'AA';
	    if ($toPrint===Null) $toPrint = cnf_show_a;
	    if (!isset($level))    $level = 2;
	    break;
	  case 'N': // nordita module debug
	    if ($toPrint===Null) $toPrint = (bool)b_cnf::get('nordebug');
	  case 'M': // menu debug
	    $stringFormat = 'M';
	  case 'A': // menu access debug
	    if (!isset($stringFormat)) $stringFormat = 'A';
	    if ($toPrint===Null) $toPrint = cnf_show_m;
	    if (!isset($level))    $level = 2;
	    break;
	  default: // color or level
	    if (($n=(int)$item)>0 && (string)$n===$item)  $level = $n; 
	    elseif (!empty($item)) $color  = $item;
	  }
	}
      }
    }

    if ($toPrint || cnf_debug){    
      $caller =  self::__((isset($level)?$level:3), @$class);
      if (($text === True) || ($text === Null) || ($text === False))  $text = var_export($text,True);
      $text   = preg_replace(';/[a-zA-Z/]*/trunk/;','&lt;trunk&gt;/',str_replace(getcwd(),'&lt;cwd&gt;',$text));
      $text   = x('em',b_fmt::strcut($text,222,'...'));

      if (empty($color)) $color = 'blackText';
      switch(@$stringFormat){
      case 'Text':$dbgString = $text;                                               $color='pinkText';    break;
      case 'I':   $dbgString = sprintf("... %s %s",$caller,$text);                  $color='maroonText';  break;
      case 'L':   $dbgString = sprintf("... %s",$text);                             $color='blueText';    break;
      case 'M':   $dbgString = sprintf("--- %s %s", $caller, $text);                $color='fuchsiaText'; break;
      case 'A':   $dbgString = sprintf("--- <tt>%-3s</tt> %s",$text,$caller);   $color='redText';     break;
      case 'AA':  $dbgString = sprintf("%s %s", $caller, $text);                    $color='redText';     break;
      case 'DB':  $dbgString = sprintf("%s %s",str_replace('bDB','DB',$caller), $text); $color='greenText';   break;
      default:    $dbgString = sprintf("%s %s", $caller, $text);
      }

      $dbgString = "<span class='$color'>$dbgString</span>";
      if (self::$starting_exiting || !@self::$xxx_cache[b_crypt::hash($dbgString)]++){
	if (isset($class) && is_object($class) && (bool)@$class->nodrupal){
	  printf("<br/>\n%s%s\n",self::s_e_depth(),$dbgString);
	}else{
	  MSG::DEBUG($dbgString);
	}
      }
    }
    if (self::$starting_exiting && $s_e_s) self::$s_e_depth++;
  }

  public static function s_e_depth($extra_shift=0){
    return (self::$s_e_depth > 0 ? str_repeat('&nbsp;',5*(self::$s_e_depth+$extra_shift)) : ''); 
  } 

  /*
   * Another debug helper
   *   $max_a_m = 3; // number of array elements to parse
   *   $max_s_l = 30;// maximal string length 
   */
  private static $current_class = '';
  public static function parse_value($arg,$max_a_m=3,$max_s_l=30){
    if (strpos(self::$current_class,'bDB')!== False) $max_s_l = 999; // show the full DB query string
    if (is_object($arg)){
      // $arg = $arg->__toString(); // __toString() is not always up-to-date
      $a = array();
      if($arg instanceof bUnit) {
	$a[] = $arg->parent_ID; 
	$a[] = $arg->rank; 
      }elseif($arg instanceof bList) {
	$a[] = var_export($arg->parent_ID,True);
	if(isset($arg->l_name)) $a[] = var_export($arg->l_name,True);
      }else{
	if(($id=@$arg->ID)||($id=@$arg->id)||($id=@$arg->parent_ID)) $a[] = $id;
	if($arg instanceof myPear_getValue) $a[] = $arg->MODE;
      }
      $arg = str_replace("'","",get_class($arg) . "(".join(',',$a).")");      
    }elseif(is_array($arg)){
      if (empty($arg)) $arg = '[]';
      else{
	$a=array();
	$ids = preg_grep('/(_id|_aid)$/',array_keys($arg)); 
	foreach($ids as $i) if (count($a)<$max_a_m) $a[$i]=self::parse_value($arg[$i]);
	if (empty($a)) foreach($arg as $k=>$v){if(count($a)<$max_a_m){
	      if (is_array($v)) $a[$k] = self::parse_value($v);
	      elseif(b_fmt::isArrayAssociative($arg)){$a[$k]=$v;}else{$a[]=$v;}}
	  }
	if (count($a) < count($arg)) $a[''] = '...';
	$arg = sprintf('&#91;%s&#93;',b_fmt::joinX(',',$a));
      }
    }elseif(is_numeric($arg)){
      // keep as is
    }elseif(is_bool($arg) || ($arg === Null)){
      $arg = var_export($arg,True);
    }elseif(is_string($arg)){
      $arg = str_replace(ROOT_DEBUG.'/','',$arg);
      $arg = (strlen($arg)>$max_s_l?substr($arg,0,$max_s_l).' ...' : $arg);
      if (strip_tags(trim($arg)) !== trim($arg)) $arg = b_fmt::escape($arg);
      $arg = "'$arg'"; // as var_export does...
    }else{
      $arg = var_export($arg,True);
    }
    return $arg;
  }

  /*
   *
   */
  public static function parse_tb_arguments($callStack_args){
    $args = array();
    foreach($callStack_args as $arg){
      $p = self::parse_value($arg);
      if (preg_match("/^(&#91;)(.*)(&#93;)$/",$p,$match)){
	$repacked = array();
	foreach(explode(',',$match[2]) as $i) $repacked[] = self::parse_value($i);
	$p = '&#91;'.join(',',$repacked).'&#93;';
      }
      $args[] = $p;
    }
    return $args;
  }

  /*
   * Parse "debug_backtrace()" output
   */
  public static function parse_tb_entry($call,$class=Null){
    
    foreach(array('class','type') as $f) if (empty($call[$f])) $call[$f] = '';
    $class = (is_object($class) 
	      ? self::parse_value($class)
	      : $call['class'].($call['type']=='::'?'':'()'));
    $reply = sprintf("%s%s%s(%s)",
		     $class,
		     $call['type'],
		     $call['function'],
		     join(', ',self::parse_tb_arguments($call['args'])));
    if (strpos($reply,'()')===0) $reply = substr($reply,2);
    return $reply;
  }

  /*
   * The main messager
   */
  public static function _debug($txt='',$flags=array(1),$message_class='debug'){

    if (!in_array($message_class,array('status','error','debug','warning','SICK')) && cnf_dev) b_debug::internalError("??? unknown message class '$message_class' ".__method__);
    if ( in_array($message_class,array('debug','SICK')) && !cnf_dev) return;
    
    static $color_codes = array('I' => array('c'=>'maroonText', // instance
					     'p'=>cnf_show_i),
				'L' => array('c'=>'blueText',   // load & require
					     'p'=>cnf_show_l),
				'C' => array('c'=>'maroonText', // configuration option
					     'p'=>cnf_show_c),
				'M' => array('c'=>'fuchsiaText',// menu
					     'p'=>cnf_show_m),
				'A' => array('c'=>'redText',    // menu access
					     'p'=>cnf_show_m),
				'AA'=> array('c'=>'redText',    // APIaccess access
					     'p'=>cnf_show_m),
				'DB'=> array('c'=>'greenText',  // DB
					     'p'=>cnf_show_db));
    
    // Defaults
    list($is_string,$level,$class,$toPrint) = array(False,1,Null,False);
    
    // Extract the arguments
    if (!is_array($flags)) $flags = array($flags);
    foreach($flags as $m){
      if    (is_numeric($m)) {$is_numeric= 1; $level = $m;}
      elseif(is_object($m))  {$is_object = 1; $class = $m;}
      elseif(is_string($m))  {$is_string = 1; $color = $m;}
      elseif(is_bool($m))    {$is_bool   = 1; $toPrint = $m;}
    }
    self::$current_class = is_object($class) ? get_class($class) : '';

    // Special formats... May be not important any more...
    if ($is_string && ($c = @$color_codes[$color])){
      $stringFormat = $color;
      $color   = $c['c'];
      $toPrint = $c['p'];
      $is_numeric = 1;
    }
	
    // Start & End of a logical block
    if ((self::$starting_exiting = is_string($txt)) && ($message_class=='debug')){
      $s_e_s = (strpos($txt,starting) !== False);
      $s_e_e = (strpos($txt,exiting)  !== False);
      if (self::$starting_exiting = ($s_e_s || $s_e_e)){
	if ($s_e_e) self::$s_e_depth--;
	list($toPrint,$color) = array(True,'orangeText');
      }
    }else{
      self::$starting_exiting = $s_e_s = $s_e_e = False;
    }
    
    if ($toPrint){    
      if (cnf_dev || in_array($message_class,array('debug','SICK'))){
	if (empty($color)) $color = array();
	if (!is_array($color)) $color = array($color);
	if ($message_class === 'SICK') { $message_class = 'warning'; $color[] = 'bold-text smaller-text'; }
	$callStack = debug_backtrace(False,$level+1);
	// Get the color from a static class
	if ((@$callStack[$level]['type'] === '::') && empty($is_string) && isset($callStack[$level]['class']::$_dbg_color)) $color[] = $callStack[$level]['class']::$_dbg_color;
	if (!preg_grep('/Text$/',$color)) $color[] = self::$_dbg_color;
	// Get the message
	$msg = sprintf("<span class='%s'>%s%s:%s <em>%s</em></span>\n",
		       join(' ',$color),
		       '__DEPTH__', // self::s_e_depth(),
		       self::parse_tb_entry($callStack[$level],$class),
		       (cnf_dev && ($message_class !== 'debug') ? '<br>' : ''),
		       (is_string($txt) ? $txt : self::parse_value($txt,10)));
      }else{
	$msg = (is_string($txt) ? $txt : self::parse_value($txt,10));
      }
      if (self::$starting_exiting || !@self::$xxx_cache[b_crypt::hash($msg)]++ || b_cnf::get('noCompactMessages')){
	$msg = str_replace('__DEPTH__',self::s_e_depth(),$msg);
	if (b_cnf::get('nodrupal') || !function_exists('drupal_set_message')) print "<br>$msg\n";
	else drupal_set_message($msg,$message_class);
      }
    }
    if (self::$starting_exiting && $s_e_s) self::$s_e_depth++;
  }

  /*
   *
   */
  static function traceBack($id='',$object=Null,$ignore_args=False){
    static $recursion = 0;
    if (!$recursion++){
      locateAndInclude('b_table');
      ob_start();
      $callStack = debug_backtrace(False);
      $t = new b_table_zebra(array('n'=>' ','b'=>' ','l'=>' '));
      $t->showLineCounter = False;
      foreach($callStack as $k=>$call) $t->prt(array('n'=>x('tt class=smaller-text',"$k"),
						     'b'=>x('tt class=smaller-text',self::parse_tb_entry($call)),
						     'l'=>x('tt class=smaller-text',preg_replace(';(.*/caa|'.trim(`pwd`).')/;','',@$call['file'].'#'.@$call['line']))));
      $t->close();
      $tb = array(x('strong',(empty($id) ? 'TraceBack:' : $id)),
		  ob_get_contents());
      ob_end_clean();
      MSG::DEBUG(join('<br>',$tb));
    }
    $recursion--;
  }
  
  /*
   * Unrecoverable error, stop the processing
   */
  public static $_internalError = 0;
  static function internalError($txt='',$for_var_dump=Null){
    // Sanity...
    if (!self::$_internalError++){
      
      // Clean locks
      if (class_exists('bDB',False) && defined('YBhere')){
	if(cnf_dev || cnf_demo || YBhere) b_cnf::get('nodrupal',($_GET['nodrupal']=1));
	bDB::$_debug = cnf_dev || YBhere;
      }

      if (is_array($for_var_dump)) var_dump('requested var_dump follows...',$for_var_dump);
      
      // Print diagnostics
      if (cnf_CLI){
	debug_print_backtrace();
      }else{
	$diagnostics = self::get_diagnostics();
	if(cnf_dev || YBhere){ 
	  print $diagnostics;
	}else{
	  // Do not show the traceback to the production users, just return to the top page
	  self::mail_diagnostics($diagnostics,'BUG REPORT');
	  b_url::header(b_url::same('?','drop_all'));
	}
      }
    }
    die("INTERNAL ERROR: $txt\n");
  }
  
  static function mail_diagnostics($diagnostics=Null,$subject='BUG REPORT'){
    bAuth();
    if (empty($diagnostics)) $diagnostics = self::get_diagnostics();
    myPear_mailer()->send(array(),
			  cnf_superUser_email(),
			  $subject,
			  "<html><body><pre>\n".$diagnostics."\n</pre></body></html>",
			  True,
			  True);
  }
  
  /*
   *
   */
  static function get_diagnostics(){

      ob_start();
      b_cnf::get('nodrupal',($_GET['nodrupal']=1));

      print x('h2',__method__.'()');
      print x('pre',date('Y-m-d H:i:s',time()));
      
      printf("\n<h2>---Current module \"%s\"</h2>\n",b_reg::$current_module);
      
      print "\n<h2>---Database transactions</h2>\n";
      printf("%s = %s\n",x('strong','in_autocommit'), var_export(myPear_db()->in_autocommit, True));
      printf("%s = %s\n",x('strong','in_transaction'),var_export(myPear_db()->in_transaction,True));
      
      if (class_exists('bAuth',False) && is_object(bAuth::$av)){
	print "\n<h2>---__AUTHENTICATION</h2>\n";
	print bAuth::$av->__toString()."\n";
	print bAuth::$av->fmtName('Lf')."\n";
      }
      print "\n<h2>---__SERVER</h2>\n";
      foreach(array('HTTP_HOST','REQUEST_URI','REMOTE_ADDR','HTTP_REFERER','HTTP_USER_AGENT','PATH_TRANSLATED') as $k){
	if (!empty($_SERVER[$k])) printf("<code>%s = %s</code><br>\n",$k,$_SERVER[$k]); 
      }

      if (!empty($_GET)){
	print "\n<h2>---_GET</h2>\n".self::print_r($_GET,'get_diagnostics');
      }

      if (!empty($_POST)){
	print "\n<h2>---_POST</h2>\n".self::print_r($_POST,'get_diagnostics'); 
      }

      if (!empty($_SESSION[CONST_cnfOptions])){
	print "\n<h2>---CONST_cnfOptions</h2>\n".self::print_r($_SESSION[CONST_cnfOptions],'get_diagnostics'); 
      }

      print "\n<h2>---traceBack</h2>\n";
      echo '<pre>'; debug_print_backtrace(); echo '</pre>'; 
      if (!empty($_SESSION['messages'])){
	print "\n<h2>---messages</h2>\n".self::print_r($_SESSION['messages'],'get_diagnostics');
	unset($_SESSION['messages']);
      }
      b_cnf::get('nodrupal',($_GET['nodrupal']=0));
      $get_diagnostics = str_replace(trim(`pwd -P`).'/','',ob_get_contents());
      ob_end_clean();
      return $get_diagnostics;
  }

  
  /*
   *
   */
  static function dumpOptions(){
    if (@$GLOBALS['dejaVu_'.__METHOD__]++) return;
    $l = array(); 
    if (cnf_show_dd){
      ksort($_SESSION[CONST_cnfOptions]);  
      $l[] = x('td',self::print_r($_SESSION[CONST_cnfOptions],CONST_cnfOptions." (_SESSION)",$doNotPrint=True));
    }
    foreach(array('_GET'  =>$_GET,
		  '_POST' =>$_POST,
		  '_FILES'=>$_FILES,
		  // '_SERVER'=>$_SERVER,
		  // '_SESSION' =>array_keys($_SESSION),
		  ) as $req=>$a){
      if (!empty($a)){
	ksort($a);
	$l[] = x('td',self::print_r($a, "$req", $doNotPrint=True));
      }
    }
    MSG::DEBUG(str_replace(array('[',']'),'',x("table class='align_left b_table'",x('tr',join('',$l)))));
  }
  
  /*
   *
   */
  static function print_r($array,$txt='',$doNotPrint=False) {

    if ($txt === 'get_diagnostics'){
      $txt = '';
      $doNotPrint = True;
    }elseif (!defined('cnf_dev') || !defined('YBhere') || (!cnf_dev && !YBhere)){
      return;
    }

    if (is_string($array)){
      self::var_dump($array,"redirected from b_debug::print_r $txt");
      return;
    }

    if ($nosort = ($doNotPrint === 'nosort')) $doNotPrint = False;
    
    if ($b_crypt_key = @$array[b_crypt_key]){
      unset($array[b_crypt_key]);
      foreach(b_crypt::parse_query_string('?'.b_crypt::hex_decode($b_crypt_key)) as $k=>$v){
	$array["__uncrypted__  $k"] = $v;
      }
    }
      
    static $pattern = array("/(Array|Object)( *)?[\r\n]( *)?\(/" => '',
			    "/(Array|Object)[\r\n]\(/"      => '',
			    "/[\r\n]( *)?[\)][\r\n]/"       => '');
    if (!empty($array)){
      $dropped = $o = array();
      if (!empty($array)) foreach($array as $k=>$v){
	  if (b_posix::is_empty($v) && !$nosort) $dropped[] = $k;
	  else $o[$k] = (is_string($v) ? trim(strip_tags($v)) : $v);
	}
      if (!$nosort) ksort($o);
  
      if (empty($txt)) $txt = self::__(array(3));
      if ($dropped){
	sort($dropped);
	$txt .= " (hidden: ".join(',',$dropped).")";
      }
      if (cnf_CLI){
	ob_start();
	print_r($o);
	$print_r = ob_get_contents();
	ob_end_clean();
	//	 preg_replace('/(Array|Object)[\r\n](\s+)?\(|[\r\n](\s+)?[\)][\r\n]|(Array|Object)[\r\n]\(/','',
	$print_r = preg_replace(array_keys($pattern),array_values($pattern),$print_r);
	print str_replace('[','',str_replace(']','',"array-------- $txt$print_r\n"));
      }else{
	ob_start();
	b_fmt::printt($txt);
	print_r($o);
	$print_r = b_fmt::escape(ob_get_contents());
	ob_end_clean();
	//	$print_r = preg_replace("/^.*\=\>[\\s\r\n]*|\=(..062.|\>).*(Array|Object)[\r\n](\s+)?\(|[\r\n](\s+)?[\)][\r\n]|(Array|Object)[\r\n]\(/","", $print_r);
	$print_r = preg_replace(array_keys($pattern),array_values($pattern),$print_r);
	$print_r = str_replace(' ','&nbsp;',
			       str_replace('[','',
					   str_replace(']','',
						       str_replace("\n",'<br/>',$print_r))));
	if ($doNotPrint) return x('pre class="print_r"',$print_r);
	else      MSG::DEBUG(x('pre class="print_r"',$print_r));
      }
    }
  }

  /*
   *
   */
  static function var_dump($var,$id='',$return=False){
    static $c = 0;
    ob_start();
    var_dump($var);
    $dump = ob_get_contents();
    ob_end_clean();
    $reply = sprintf("<pre>%s(%s)\n%s</pre>",__METHOD__,$id,$dump);
    if ($return)        return $reply;
    else         MSG::DEBUG($reply);
  }

  /*
   * Print php back_trace in the "debug" section
   */
  static function traceBack_OBSOLETE($id='',$object=Null,$ignore_args=False){
    if (cnf_CLI){
      print "\n".self::getTraceBack(False,$ignore_args)."\n";
      return;
    }
    if (!defined('cnf_dev') || !defined('YBhere') || (!cnf_dev && !YBhere)){
      return;
    }elseif(b_cnf::get('nodrupal')){
      print self::getTraceBack();
    }else{
      if (empty($id)) $id = 'TraceBack:';
      MSG::DEBUG(x('strong',$id).'<br/>'.		    
		    self::getTraceBack(True,$ignore_args));
      self::print_r($object,__FUNCTION__);
    }
  }

  /*
   *
   */
  public static function getTraceBack($cutLongStrings=True,$ignore_args=False){
    ob_start();
    if ($ignore_args)  debug_print_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);
    else               debug_print_backtrace();
    $trace = ob_get_contents();
    ob_end_clean();
    if (cnf_CLI){
      self::$backtraceData = x('pre',$trace);
    }else{
      static $reps = array('/\)$/'        => ')]', // yes...
			   '/\]$/'        =>'</td><td></td></tr>',
			   '/^#(\d*) /'   =>'<tr><td class="traceback align-right">\\1 - </td><td>',
			   '/called at ./'=>'</td><td>',
			   );
      $srv = trim(dirname(dirname(dirname(__FILE__))));
      foreach(explode("\n",$trace) as $line){
	foreach(array('call_user_func_array',
		      'menu_execute_active_handler',
		      '#[0-9]* *(Drupal|Stack|Symfony)',
		      __FUNCTION__) as $drop) if((strpos($line,'#')!==0) || preg_match("/$drop/i",$line)) continue 2;
	$lines[] = preg_replace(array_keys($reps),
				array_values($reps),
				str_replace("$srv/",'',
					    str_replace(getcwd(),'&lt;cwd&gt',$line)));
      }
      if (True||$cutLongStrings){
	$lines_cut = array(); 
	foreach($lines as $line){
	  $line_cut = array();
	  foreach(explode('</td><td>',$line) as $l) $line_cut[] .= (strpos($l,'<') === False
								    ? b_fmt::strcut($l, 100, '...')
								    : $l);
	  $lines_cut[] = implode('</td><td>',$line_cut);
	}
	$lines = $lines_cut;
      }
      self::$backtraceData = x("table",x("tbody",str_replace('<td>','<td class="traceback">',join("\n",$lines))));
    }
    return self::$backtraceData;
  }
}

/*
 * Date / Time treatment
 */ 
class b_time{

  static public function isRedDay($date){
    /* Holidays:
     *       easter
     *       easter - 2
     *       easter + 1
     *       easter + 39 
     *       easter + 49  (Not in Sweden!)
     *       easter + 50  
     */
    if (b_time::strftime('%u',$date)>5)           return True; // Sunday Saturday
    if (b_time::strftime("%m%d",$date) == '0101') return True; // 1 Jan
    if (b_time::strftime("%m%d",$date) == '0106') return True; // Swedish 6-Jan
    if (b_time::strftime("%m%d",$date) == '0501') return True;
    if (b_time::strftime("%m%d",$date) == '0606') return True; // Swedish national day
    if (b_time::strftime("%m%d",$date) == '1225') return True;
    if (b_time::strftime("%m%d",$date) == '1226') return True;

    $year = b_time::strftime('%Y',$date);
    if ((b_time::is_valid($date) <= 0) || ($year > 2037)){
      //      b_debug::traceBack();
      b_debug::xxx("Y=$year? ".date('Y-m-d',$date),array(cnf_dev));
      return False;
    }
    if (function_exists('easter_date') && $date>0 && ($year >= 1970) && ($year <= 2037)){
      $edate = easter_date($year);
    }else{
      b_debug::traceBack();
      $edate = self::easter_date($year);
    }

    $easter_holidays = array(-2, 0, 1, 39, 49);
    if (False) $easter_holidays[] = 50; // The country code should be checked. Say, in Sweden this is NOT holiday
    foreach ($easter_holidays as $daysafter)
      if (b_time::strftime("%m",$edate+$daysafter*86400)==b_time::strftime("%m",$date) &&
          b_time::strftime("%d",$edate+$daysafter*86400)==b_time::strftime("%d",$date)) return True;
    return False;
  }

  /**
   * Get the Easter date with a formula "a la Gauss".
   *        Iouri Belokopytov, 1998
   *        Crafted after Christian Walck's VMS Fortran code
   *
   * Those days this for sure might be done in a more simple way, 
   * but i want to keep this code... 
   * One day you will understand why.
   */
  static function easter_date($year) {
    $m = 24;
    $n =  5;
    $a = $year - 19*floor($year/19);
    $b = $year -  4*floor($year/ 4);
    $c = $year -  7*floor($year/ 7);
    $x = 19 * $a + $m;
    $d = $x - 30*floor($x/30);
    $x = 2*$b + 4*$c + 6*$d + $n;
    $e = $x - 7*floor($x/7);
    $x = 22 + $d + $e;
    if ($x <= 31) {
      $easter['m'] = 3;
    } else {
      $easter['m'] = 4;
      $x = $d + $e - 9;
      if ($x == 26) $x = 19;
      if ($a >=10 && $d == 28 && $x==25) $x = 18;
    }
    $easter['y']= $year;
    $easter['d'] = $x;
    return b_time::noon($easter['y'],$easter['m'] ,$easter['d']); 
  }

  /*
   * Get the unix time from the year.
   * Set the number of seconds to "13" as the flag "this is an 'artificial' date"
   *
   * YB Well... this does not work... Bad idea to conver year to "first of July", very misleading...
   */
  private static $y2d_conversion = 13;
  static public function y2d($year){
    $reply = $year;
    return $reply;
  }  

  /*
   * Make sure that $start < $end
   */ 
  public static function adjust_start_end(&$start,&$end){
    if (b_posix::is_int($start) && b_posix::is_int($end) && ($start > $end)){
      $t = $start; 
      $start = $end; 
      $end = $t; 
    }
  }

  /*
   * Same as the standard function 'date', 
   * but returns the input argument if the input is not a date
   */
  public static function date($format,$date=Null){
    if (empty($date)) $date = date();
    $date = b_time::txt2unix($date);
    $reply = (b_posix::is_int($date)
	      ? date($format,$date)
	      : $date);
    return $reply;
  }

  public static function strftime($fmt,$time=Null){
    if (empty($time)) $time = time();
    $time = self::txt2unix(str_replace('20016-08-31','2016-08-31',$time));
    try{
      $reply = b_crypt::encode_utf8(strftime($fmt,$time));
      if (($x=strftime($fmt,$time)) != $reply) b_debug::xxx(date('Y-m-d ',$time)." $reply != $x ");
    }catch(Exception $e){
      if (cnf_dev) var_dump($time,$time,$e->xdebug_message);
    }
    return $reply;
  }

  public static function full_date($date=Null){
    return self::_date('full',$date);
  }

  public static function long_date($date=Null){
    return self::_date('long',$date);
  }

  public static function medium_date($date=Null){
    return self::_date('medium',$date);
  }

  public static function medium_date_time($date=Null){
    return self::_date('medium',$date). ' ' .self::time($date,False);
  }

  public static function short_date($date=Null){
    return self::_date('short',$date);
  }

  public static function sql_timestamp($date=Null){
    if (empty($date)) $date = time();
    return date('Y-m-d G:i:s',$date);
  }

  public static function long_date_time($date=Null){
    return trim(self::long_date($date).' '.self::time($date));
  }

  public static function short_date_time($date=Null){
    return trim(self::short_date($date).' '.self::time($date,False));
  }

  public static function long_week($date=Null){
    return b_time::strftime(b_t::_('week').' '.rbs_weekNumberCode.', '.b_t::_('year').' %Y',$date);
  }

  public static function short_week($date=Null){
    return b_time::strftime(rbs_weekNumberCode,$date);
  }

  public static function long_month($date=Null){
    return b_time::strftime("%B %Y",$date);
  }
  public static function long_year($date=Null){
    return b_time::strftime(b_t::_('year').' %Y',$date);
  }

  /*
   * Date presentation.
   * Valid options: short      / medium      / long            /full
   *                2011-01-05 / 05 Jan 2011 / 05 January 2011 / Sunday, 05 January 2011
   * 
   */
  static function _date($presentation,$date=Null){
    $force_internal_format = True;
    
    if (empty($date)) $date = time();
    $date = self::txt2unix($date);

    if (self::is_valid($date) <= 0){
      return $date;
    }else{
      switch($presentation){
      case 'short': 
	if (!$force_internal_format) if (function_exists('format_date')) return format_date($date, 'small');
	$f = '%Y-%m-%d';   
	break;
      case 'medium': 
	if (!$force_internal_format) if (function_exists('format_date')) return format_date($date, 'medium');
	$f = '%d %b %Y';
	break;
      case 'long':  
	if (!$force_internal_format) if (function_exists('format_date')) return format_date($date, 'large');
	$f = '%d %B %Y'; 
	break;
      case 'full': 
	if (!$force_internal_format) if (function_exists('format_date')) return format_date($date, 'large');
	$f = '%A, %d %B %Y';
	break;
      default:
	$f = $presentation;
      }
      return b_crypt::encode_utf8(strftime($f,$date));
    }
  }

  private static function time($time,$show_seconds=True){
    if (empty($time)) $time = time();
    if ($show_seconds)              return date('H:i:s', $time);
    else                            return date('H:i',   $time);
  }

  /*
   * check-in/check-out 
   */
  public static function check_in ($date=Null) { return b_time::_($date,CONST_TIME_CI); }
  public static function check_out($date=Null) { return b_time::_($date,CONST_TIME_CO); }
  public static function oa_start ($date=Null) { return b_time::_($date,CONST_TIME_OAS); }
  public static function oa_end   ($date=Null) { return b_time::_($date,CONST_TIME_OAE); }
  public static function _($date=Null,$time=Null){
    if ($date===Null) $date = time();
    if ($time===Null) $time = CONST_TIME;
    $date = self::txt2unix($date);
    switch($time){
    case 0:             $time =  0; break;
    case 24:            $time = 24;  break;
    case CONST_TIME_CO: $time = 10; break;
    case CONST_TIME_CI: $time = 15; break;
    case CONST_TIME:    $time = 12; break;
    }
    return mktime((int)$time,0,0,
		  (int)date('n',(int)$date),
		  (int)date('j',(int)$date),
		  (int)date('Y',(int)$date));
  }

  /*
   * Catch the the integer "year"
   *
   * @reply:
   *   -2 - NOT valid, empty input value
   *   -1 - NOT valid, "not considered"
   *    0 - NOT valid, probably just a year number
   *    1 -     valid, integer unix time 
   *    2 -     valid, string "YYYY-MM-DD" or
   *                   string "YYYY-MM-DD HH:MM"
   *                   string Registrant_RegistrationDate=12-January-2015-20:29
   *    3 -     valid, SSN (personnummer)
   */
  public  static $is_valid = Null;
  private static $match = array(); 
  public static function is_valid($arg){

    static $months = array('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');
    self::$match = array();

    locateAndInclude('bForm_Avatar');
    if(empty($arg) || ($arg === '000000')){
      $reply =-2;
      $dbg = 'empty'; 
    }elseif($arg == CONST_year_not_considered){
      $reply =-1;
      $dbg = 'not considered'; 
    }elseif(SSN::is_valid($arg,'check_only')){
      $reply = 3;
      $dbg = 'YES'; 
    }elseif(is_numeric($arg) && ($arg < CONST_eternity_t+365*3600) && (($arg<0) || ($arg>CONST_eternity_year))){
      $reply = 1;
      $dbg = date('Y-m-d',$arg);
    }elseif(preg_match('/^\d?\d?\d\d-\d?\d-\d?\d/',$arg)){
      $reply = 2;
      $dbg = 'YES'; 
    }elseif(preg_match('/^(\d\d?)-('.join('|',$months).')[^-]*-(\d\d\d\d)-(\d\d?):(\d\d?)/',trim($arg),self::$match)){
      foreach($months as $k=>$n) if ($n == self::$match[2]) self::$match[2] = $k+1;
      $reply = 2;
      $dbg = 'YES'; 
    }else{
      $reply = 0;
      $dbg = 'NO';
    }
    if (defined('cnf_dev') && cnf_dev && ($reply <= 0) && ($reply != -2))  b_debug::xxx("$dbg, error code=$reply");
    
    // Save the result for the later use
    return (self::$is_valid = $reply);
  }

  /*
   * Unix time for the midday y/m/d
   */
  public static function noon($y=0,$m=0,$d=0,$date=0) {
    if (func_num_args() == 1) $date = $y;
    if (empty($date) || $date === 'now') $date = time();
    if (empty($y)) $y = date('Y',$date);
    if (empty($m)) $m = date('n',$date);
    if (empty($d)) $d = date('j',$date);
    if ($y < 1900) $y += 1900;
    return mktime(12,0,0,$m,$d,$y);
  }

  /*
   * Monday of the current week, midday
   */
  public static function monday($date=Null){
    if (!(int)$date)  $date = time();
    $wd = date('w',$date);    // 0 for Sunday through 6 for Saturday
    return self::_($date - ($wd -1) * 86400); 
  }

  /*
   * Unix time for Sunday of day 1 of the month
   */
  public static function f_sunday($date=Null,$mode='month',$direction='left'){

    if (empty($date) || $date === 'now')  $date = time();
    else                                  $date = self::txt2unix($date);
    $y = date('Y',$date);
    $m = date('m',$date);
    $d = date('d',$date);
    switch($mode){
    case 'month':  $day1 = self::noon(0,0,1,$date);           break;
    case 'month-': $day1 = self::noon(0,0,0,$date- 31*86400); break;
    case 'month+': $day1 = self::noon(0,0,0,$date+ 31*86400); break;
    case 'year-':  $day1 = self::noon(date('Y',$date)-1,1,1); break;
    case 'year-':  $day1 = self::noon(date('Y',$date)+1,1,1); break;
    case 'week':   $day1 = $date;                             break;
    default:      b_debug::internalError("Unpredicted mode = '$mode'");
    }

    // Shift to the first Sunday
    // if (strpos($mode,'+') !== False) $direction = 'right';
    // if (strpos($mode,'-') !== False) $direction = 'left';
    while (strftime('%w',$day1) != 0) { 
      if ($direction == 'right') $day1 += 86400;  
      else                       $day1 -= 86400;  
    }
    // b_debug::xxx(date('Y-m-d H:i',$date).' --> '.date('Y-m-d H:i',$day1));
    return $day1;
  }
  
  /*
   * Convert text date to the Unix Time
   *
   * The input might come in various forms, like
   * - 2012-06-17 18:00  = String
   * - 2012-06-17        = String
   * - 1577876400        = Unix time
   * - 2001              = Year
   * - 550302-1232       = Swedish personal number
   * - etc.
   */
  public static function txt2unix($date,$time='') {
    switch($case=b_time::is_valid($date)){ 
    case -2: // empty
      $reply = Null;
      break;
    case -1: // "not considered"
    case  0: // YYYY
    case  1: // Unix time
      $reply = $date;
      break;
    case  2: // YYYYY-MM-DD
      $reply = self::unpack_date_string($date,$time);
      break;
    case  3: // SSN
      $reply = SSN::ssn2birthDate($date);
      break;
    }
    if (defined('cnf_dev') && cnf_dev) if(($date != $reply) &&
					  ($date != ($r=date('Y-m-d',(int)$reply))) &&
					  ($date != ($r=date('Y-m-d H:i',(int)$reply))) &&
					  ($date != ($r=date('d-F-Y-H:i',(int)$reply)))
					  ) b_debug::xxx("case=$case? $reply != $r");
    return $reply;
  }
  
  /*
   *
   */
  private static function unpack_date_string($date,$time=''){
    
    if (!empty(self::$match)){
      /*
    return mktime(12,0,0,$m,$d,$y);
    [0] => 12-January-2015-20:29
    [1] => 12
    [2] => 1
    [3] => 2015
    [4] => 20
    [5] => 29
       */
      return mktime((int)self::$match[4],
		    (int)self::$match[5],
		    0,
		    (int)self::$match[2],
		    (int)self::$match[1],
		    (int)self::$match[3]
		    );
    }elseif(preg_match('/^(\d?\d?\d\d)-(\d?\d)-(\d?\d) ?(\d?\d)?:?(\d?\d)?:?(\d?\d?)?/',$date,$match)){
      
      // Catch time (the second argument), set time to the midday if not provided
      $t = explode(':',$time);
      foreach(array(1=>12, // HH
		    2=> 0, // MM
		    3=> 0, // SS
		    ) as $k=>$v){
	if (!empty($t[$k-1]))    $match[$k+3] = (string)$t[$k-1];
	if (empty($match[$k+3])) $match[$k+3] = (string)$v; 
      }
      
      // w2k problem...
      if ($match[1] < 100) $match[1] += 1900;
      $match[1] = min($match[1],CONST_eternity_year);
      
      return mktime((int)$match[4],
		    (int)$match[5],
		    (int)$match[6],
		    (int)$match[2],
		    (int)$match[3],
		    (int)$match[1]);
    }elseif(cnf_dev){
      b_debug::traceBack('?? Argument is not a date/time');
    }
  }
  
  /*
   * Convert unix time to a human-readable form
   */   
  public static function unix2txt($value,$fmt='Y-m-d H:i'){
    switch(b_time::is_valid($value)){
    case  -1:
      $reply = 'Not considered';
      break;
    case  1:
      $reply = date($fmt,$value); 
      break;
    default:
      $reply = $value;
    }
    return $reply;
  }
  
  /*
   * human-readable time period between $t2 & $t2
   */
  public static function delta($t1, $t2=Null) {
    if (!is_numeric($t1))  return '???';
    if ($t2 === Null){
      $s = $t1;
    }else{
      if (!is_numeric($t2))  return '???';
      $s = ($t1 > $t2) ? $t1-$t2 : $t2-$t1;
    }
    $m = (int)($s/60);
    $h = (int)($m/60);
    $d = (int)($h/24);
    $w = (int)($d/7);
    $x = (int)($d/30);
    $y = (int)($d/365);
    if     ($y>2) $reply = "$y years";
    elseif ($x>1) $reply = "$x months";
    elseif ($w>1) $reply = "$w weeks";
    elseif ($d>1) $reply = "$d days";
    elseif ($h>1) $reply = sprintf("%s h. %s min. %s sec.",$h,($m-$h*60),($s-$m*60));
    elseif ($m>1) $reply = sprintf("%s min. %s sec.",$m,($s-$m*60));
    else          $reply = "$s seconds";
    return $reply;
  }

  /*
   * Number of days from $time1 to $time2
   */
  public static function diff($time1,$time2){ // $reply = $time1 - $time2
    return (int)((b_time::_($time1) - b_time::_($time2))/86400 + 0.5);
  }

  public static function inInterval($time1,$time2,$start,$end){
    if (!$time1 || !$time2 || !$start || !$end) b_debug::internalError(b_debug::__());
    if ((int)$time2 <  (int)$time1) { $x = (int)$time1; $time1=(int)$time2; $time2=$x; }
    if     ((int)$time1 >= (int)$start && (int)$time1 <= (int)$end) $r = True;
    elseif ((int)$time2 >= (int)$start && (int)$time2 <= (int)$end) $r = True;
    elseif ((int)$time1 <= (int)$start && (int)$time2 >= (int)$end) $r = True;
    else                                                            $r = False;
    if (False) MSG::DEBUG(x('pre',
			       "test ".b_time::strftime("%F",$time1).' '.b_time::strftime("%F",$time2)."\n".($r?'IN ':'OUT').
			       "  ".   b_time::strftime("%F",$start).' '.b_time::strftime("%F",$end)));
    return $r;
  }
  
  public static function microtime($asInt=''){
    global $microtime0, $microtime9;
    
    $microtimeNorma = 1000;
    
    list($usec, $sec) = explode(" ", microtime());  if (!isset($microtime0)) $microtime0 = $sec;
    $time = (float)$usec + (float)$sec - (float)$microtime0;
    $microtime9 = max($microtime9,$time);
    
    if (!is_string($asInt)) $asInt = (string)$asInt;
    if ($asInt == 'getNorma') return       $microtimeNorma;
    if ($asInt == 'getTotal') return (int)($microtimeNorma*$microtime9);
    
    if ($asInt) return (int)($microtimeNorma*$time);                                                  
    else        return                       $time;
  }
  
  public static function duration($duration) {
    list($h,$m,$s) = explode(':',$duration);
    return $s + 60*($m + 69*$h);
  }
  
  /*
   *
   */
  public static function period($start,$end=0,$noDate=False){
    if (is_array($start)) list($start,$end) = $start;
    if (self::is_valid($start) == 2) $start = self::txt2unix($start);
    if (self::is_valid($end)   == 2) $end   = self::txt2unix($end);
    if (empty($start) && empty($end)){
      $reply = '';
    }elseif ($noDate === 'full'){
      self::adjust_start_end($start,$end);
      $reply = date('Y-m-d H:i:s',self::txt2unix($start)).' --> '.date('Y-m-d H:i:s',self::txt2unix($end));
    }elseif ($noDate === 'short'){
      self::adjust_start_end($start,$end);
      $reply = date('Y-m-d',self::txt2unix($start)).' - '.date('Y-m-d',self::txt2unix($end));
    }else{
      $reply = '';
      if (is_numeric($start) && is_numeric($end)){
	self::adjust_start_end($start,$end);
	$fmt = (date('Y',$start)==date('Y',$end)
		? '%h'
		: '%h %Y');
	if ($noDate === 'h'){ // human format
	  $reply = 'from '. b_time::strftime('%e %B',$start).' to '.b_time::strftime('%e %B %Y',$end);
	}else{
	  if (!$noDate) $fmt = "%d $fmt";
	  $reply = b_time::strftime($fmt,$start).' -> '.b_time::strftime(($noDate?'':'%d ').'%h %Y',$end);
	}
      }elseif(!empty($start) || !empty($end)){
	if ($start) $reply .= b_time::strftime('%F',$start);
	$reply .= ' -> ';
	if ((int)$end)           $reply .= b_time::strftime('%F',$end);
	elseif($end == 'now')    $reply .= 'now';
	elseif($end == 'present')$reply .= 'present';
      }
    }
    return $reply;
  }

  static function parse($time){
    return array(date('Y',$time), date('m',$time), date('d',$time));
  }
}

/*
 * Compatibility support for the  historical changes in the way the 
 * users are stored in the database(s) during the myPear lifetime
 */
class b_posix{

  /*
   * (string)'0' & (int)0 are NOT empty value for this function
   */
  public static function is_empty($value){
    if (is_string($value))     $value = trim($value);
    $reply = empty($value) && ($value !== 0) && ($value !== '0');
    return $reply;
  }

  /*
   * 0 is not int for this function
   */
  public static function is_int($value){
    $reply = !empty($value) && (is_int($value) || is_numeric($value));
    return $reply;
  }


  /*
   * Compare two values (start '===')
   */
  public static function is_equal($v1,$v2){
    if (is_string($v1)) $v1 = trim($v1);
    if (is_string($v2)) $v2 = trim($v2);
    $reply = ($v1 === $v2);
    if (($v1 !== Null) && ($v2!==Null)){
      if (!$reply && (is_numeric($v1) || is_numeric($v2))) $reply = ((string)$v1 === (string)$v2);
      if (!$reply) $reply = ($v1 == $v2);
    }
    return $reply;
  }

  /*
   * name   The name element contains the username of the user. 
   *        This is a short, usually less than 16 character "handle" of the user, not the real, full name. 
   *        This should be the same as the username parameter used when calling the function, and hence redundant.
   * passwd The passwd element contains the user's password in an encrypted format. 
   *        Often, for example on a system employing "shadow" passwords, an asterisk is returned instead.
   * uid    User ID of the user in numeric form.
   * gid    The group ID of the user. Use the function posix_getgrgid() to resolve the group name and a list of its members.
   * gecos  GECOS is an obsolete term that refers to the finger information field on a Honeywell batch processing system.
   *        The field, however, lives on, and its contents have been formalized by POSIX. 
   *        The field contains a comma separated list containing the user's full name, office phone, office number, and home phone number. 
   *        On most systems, only the user's full name is available.
   * dir    This element contains the absolute path to the home directory of the user.
   * shell
   */
  public static function getpwnam($login){
    $reply = array();
    if (!empty($login)){
      if ($r = posix_getpwnam($login)){
	$reply = $r;
      }elseif ((cnf_CLI || cnf_dev || cnf_demo) && preg_match('/^[a-z][a-z0-9]*$/i',$login)){
	$reply = array('name'  =>$login,
		       'passwd'=>'secret',
		       'uid'   =>500,
		       'gid'   =>1,
		       'dir'   =>'/tmp',
		       'shell' =>'/bin/bash');
	if ($av = bForm_Avatar::hook(array('av_identity'=>$login))){
	  $reply['gecos'] = $av->fmtName('fl');
	}else{	  
	  $reply['gecos'] = "local $login";
	}
      }
    }
    return $reply;
  }
}

/*
 * Country Codes support
 */
class b_cc{
  
  public static $cc2continent = array();
  public static $cc2country   = array();
  
  /*
   *
   */
  private static function init(){
    if (empty(self::$cc2country)){
      $q = myPear_db()->query("SELECT * FROM zzz_countries");
      while($r = myPear_db()->next_record($q)){
	self::$cc2continent[$r['country_code']]= $r['country_continent'];
	self::$cc2country[$r['country_code']]  = $r['country_name'];
      }
    }
  }

  /*
   * get country name from county_code and/or locale
   */
  static function getName($country_code){
    self::init();
    if (in_array($country_code,array_values(self::$cc2country))){
      // The CC is in fact the country name
      $reply = $country_code;
    } elseif(!($reply = @self::$cc2country[preg_replace('/^.*_/','',$country_code)])){
      //$reply = $country_code;
      $reply = '';
    }
    if (cnf_show_c) b_debug::xxx("$reply ");
    return (strlen($reply) > 2
	    ? b_t::_(array('key'=>'cc;'.$reply,'item'=>$reply))
	    : $reply);
  }
  
  static function getContinent($country_code){
    self::init();
    if (!($reply = @self::$cc2continent[self::getCC($country_code)])){
      $reply = $country_code;
    }
    if (cnf_show_c) b_debug::xxx("$reply ");
    return (strlen($reply) > 2
	    ? b_t::_($reply)
	    : $reply);
  }

  static function getCC($country_name){
    self::init();
    foreach (self::$cc2country as $cc=>$name){
      if (strToLower($country_name)===strToLower($name)) $reply = $cc;
      if (strToLower($country_name)===strToLower($cc))   $reply = $cc;
    }
    if (empty($reply))  $reply = '';
    if (cnf_show_c) b_debug::xxx("$reply ");
    return $reply;
  }
  
  static function getList(){
    self::init();
    return self::$cc2country;
  }

  /*
   * 
   */
  static function autocomplete($name){

    self::init();
    foreach(self::$cc2country as $cc=>$cname) $names[] = "\"$cname\"";
    bJS()->add_js_inline("$(function(){ var names=[".join(',',$names)."];$('#$name').autocomplete({source:names});});");
  //bJS()->add_js_inline("$(function(){ $('#$name').autocomplete({source:[".join(',',$names)."]});});");
  }
}


/*
 * Various coding / decoding functions
 */
class b_crypt{
  
  private static $key     = b_crypt_key;  // the key for the encoded URL
  private static $md6List = array();
  private static $md6Cont = 999;
  
  /*
   * trivial hash function
   */
  static function md6($string1,$string2='',$module=''){
    if (empty(self::$md6List[$id="$_GET[q]/$string1/$string2/$module"])) self::$md6List[$id] = ++self::$md6Cont;
    return self::$md6List[$id];
  }

  /*
   * Wrapper around the hash function.
   * To get the short hash string "crc32" might be used,
   * but the reliable hash function is "md5".
   */
  static function hash($string,$function='crc32'){
    if (is_array($string)) $string = serialize($string);
    if ($function == 'crc32') return sprintf("%u",crc32($string));
    else                      return md5($string);
  }

  /*
   * Hide the URL from hackers
   */
    // if any of those keys present in the input string, the URL is not encoded
  private static $untouchable = array(b_crypt_no,
				      b_crypt_key,
				      'data',
				      'z');
  static function encodeURL($input=array()){
    if (cnf_show_e || cnf_inside_fb) return $input;

    // list of the keys which should not be encoded
    static $leaveAsIS   = array('PHPSESSID', 'q', 'option');     
    if (array_intersect(self::$untouchable,array_keys($input))) return $input;

    $output = $toBeEncoded = array();
    foreach($input as $k=>$v){
      if(in_array($k,$leaveAsIS))                 $output[$k] = $v; // drupal and Joomla!
      elseif(!preg_match('/^SESS[0-9a-f]*$/',$k)) $toBeEncoded[$k] = $v; 
    }
    if ($toBeEncoded) $output[b_crypt_key] = self::hex_encode(b_fmt::joinX('&',$toBeEncoded));
    return $output;
  }
  
  /*
   * Parse _SERVER[QUERY_STRING] into an array
   */
  static public  $parse_query_string_arg0 = '';
  static function parse_query_string($string=Null){

    if ($string === Null) $string = '?'. str_replace('%20',' ',@$_SERVER["QUERY_STRING"]);

    if (is_array($string)){
      $reply = $string;
    }else{

      if (strpos($string,'?') === False)      b_debug::traceBack($string,'missing "?" in the URL');

      self::$parse_query_string_arg0 = preg_replace('/\?.*/','?',$string);
      $string = str_replace(self::$parse_query_string_arg0,'',$string);

      $reply = array();
      $string = str_replace('&amp;','&',$string);
      foreach(explode('&',$string) as $get){
	if (strpos($get,'=')===False) $get .= '=';
	list($kk,$vv) = explode('=',$get,2);
	$kk = trim($kk);
	if (!empty($kk)) $reply[$kk] = trim($vv);
      }
    }
    return $reply;
  }
  
  /*
   * Only for the apache server
   */
  static function decodeURL(){
    $qs = @$_SERVER["QUERY_STRING"];
    if ((strpos($qs,'?'.b_crypt_key.'=') !== False) ||
	(strpos($qs,'&'.b_crypt_key.'=') !== False)){
      foreach(self::parse_query_string() as $kk=>$vv){
	if ($kk == b_crypt_key){
	  $hex_decode = self::hex_decode($vv);
	  foreach(self::parse_query_string("?$hex_decode") as $k=>$v){
	    $_GET[$k] = $_REQUEST[$k] = $v;
	  }
	}else{
	  $_GET[$kk] = $_REQUEST[$kk] = str_replace('%20',' ',$vv);
	}
      }
      b_cnf::_unset(b_crypt_key);	
      $_SERVER["QUERY_STRING"] = b_fmt::joinX('&',$_GET,'');
      $_SERVER["REQUEST_URI"]  = preg_replace('/\?.*/','',$_SERVER["REQUEST_URI"]) . '?' . $_SERVER["QUERY_STRING"];
    }
  }

  static function data2utf8($data) {
    if (is_array($data)) {
      $result = array();
      foreach($data as $key => $value) {
	if (is_array($value)) 
	  $result[self::encode_utf8($key)] = self::data2utf8($value);
	else
	  $result[self::encode_utf8($key)] = self::encode_utf8($value);
      }
    }elseif(is_string($data)){
      $result = self::encode_utf8($data);
    }else{
      $result = $data;
    }
    return $result;
  }

  function is_latin1($string){ 
    $reply = (preg_match("/^[\\x00-\\xFF]*$/u", $string) === 1); 
    b_debug::xxx($reply,array(2,True));
    return $reply;
  } 
  
  static function is_utf8($string){
    $reply = (strToUpper(self::detect_encoding($string)) == 'UTF-8');
    return $reply;
  }
  
  /*
   *
   */
  static function detect_encoding($string){
    $mb = mb_detect_encoding($string,array(
					   'UTF-8',
					   'ISO-8859-1',
					   'ISO-8859-2',
					   'ISO-8859-3',
					   'ISO-8859-4',
					   'ISO-8859-5',
					   'ISO-8859-9',
					   'ISO-8859-10',
					   'KOI8-R',
					   ),True);
    $reply = (empty($mb) 
	      ? mb_detect_encoding($string,'auto',True)
	      : $mb);
    return $reply;
  }
  
  /*
   *
   */
  static function encode_utf8($str) {
    // Those are easy cases...
    if ( is_numeric($str)) return $str;
    if (!is_string($str))  return $str;
    if (empty($str))       return '';

    switch ($encoding=self::detect_encoding($str)){
    case 'KOI8-R':     // the detection is not perfect...
    case 'ISO-8859-5': 
      return b_fmt::cyr_utf8($str,'i');
    case 'ASCII':
    case 'UTF-8': 
      return $str;  
    default: 
      $str = utf8_encode($str); // Encodes an ISO-8859-1 string to UTF-8
      // b_debug::xxx("$encoding - $str");
      return $str;
    }
  }
  
  /*
   * Lightweight version...
   */
  static function encode_latin1($str) {
    if (!is_string($str)) return $str;
    if (empty($str))      return '';
    if ($str === mb_convert_encoding(mb_convert_encoding($str,'ISO-8859-1','UTF-8'), 'UTF-8', 'ISO-8859-1'))   return $str;
    if (self::is_latin1($str))  return $str;
    else                        return utf8_decode($str);
  }

  /*
   * utf8/latin1 encoding
   */
  static function encode($string,$encoding='utf8'){
    switch($encoding){
    case 'latin1': 
      $reply = self::encode_latin1($string);
      break;
      
    case 'utf8':   
      $reply = self::encode_utf8($string); 
      break;
      
    default:
      MSG::ERROR(b_debug::__());
      return $string;
    }
    b_debug::xxx("$encoding $reply",array(2,'M',True));
    return $reply;
  }

  /*
   * encode / decode as hexadecimal
   */
  private static $offsets = array(0);
  static function hex_decode($string) {
    $reply = "";
    for ($i = 0; $i < strlen ($string); $i += 2) {
      $ch1 = substr ($string, $i, 1);
      $ch2 = substr ($string, $i + 1, 1);
      $val = self::hex2int ($ch1) * 16 + self::hex2int ($ch2);
      $j = $i % count (self::$offsets);
      $newval = $val + self::$offsets[$j];
      $newval %= 256;
      $reply .= chr ($val);
    }
    return $reply;
  }
  
  static function hex_encode($string) {
    $reply = "";
    for ($i = 0; $i < strlen ($string); $i++) {
      $ch1 = substr ($string, $i, 1);
      $val = ord ($ch1);
      $j = $i % count (self::$offsets);
      $newval = $val + self::$offsets[$j];
      $newval %= 256;
      $reply .= bin2hex(chr($newval));
    }
    return $reply;
  }

  /*
   * Encode by replacing text string by the string number
   */
  public  static $debug = False;
  private static $encryption_X = array('='  => 'e',
				       ','  => 'c');
  public static function encodeX($key_values_pairs){
    static $key,$value;

    $key_values_pairs_encryped = array(); 
    foreach($key_values_pairs as $key=>$value){
      foreach(array('key','value') as $i){
	$now = $was = ($i == 'key'
		       ? $key
		       : $value);
	if (!empty($was) && !is_numeric($was)){
	  $q = myPear_db()->qquery("SELECT c_code,c_value FROM zzz_crypt WHERE c_value='$was'",self::$debug);
	  switch($n=myPear_db()->num_rows($q)){
	  case 0:   // Insert in the database the encoded values
	    // lock the database 
	    myPear_db()->transaction('start');
	    // Update the database
	    try {
	      $q = myPear_db()->qquery("SELECT MAX(c_code) as c_max FROM zzz_crypt",self::$debug); 
	      $c_code = -1;
	      while($r = myPear_db()->next_record($q)) $c_code = max($c_code, $r['c_max']);
	      $q = myPear_db()->qquery("INSERT INTO zzz_crypt SET c_code = ".(++$c_code).", c_value='$was'",self::$debug);
	      $now = 'x'.$c_code;
	    }catch (Exception $e) {
	      b_debug::internalError(x('pre',$e->xdebug_message));
	    }
	    // Release the lock
	    myPear_db()->transaction('end');
	    break;
	    
	  case 1: // The encoding is known
	    while($r=myPear_db()->next_record($q)) $now = 'x'.$r['c_code'];
	    break;
	  default:
	    b_debug::internalError("$n replies for c_value='$was'");
	  }
	}
	if ($i == 'key')  $key   = $now;
	else              $value = $now;
      }
      $key_values_pairs_encryped[] = $key.self::$encryption_X['='].$value;
    }
    $string = join(self::$encryption_X[','],$key_values_pairs_encryped);
    if (self::$debug) b_debug::xxx($string);
    return $string;
  }

  /*
   *
   */
  public static function decodeX($string){
    static $key,$value;

    // Get back [,] & [=]
    foreach(self::$encryption_X as $f=>$t) $string = str_replace($t,$f,$string);    

    // decode the args
    $reply_int = $reply_txt = array();
    foreach(explode(',',$string) as $pair){
      list($key,$value) = explode('=',$pair);
      foreach(array('key','value') as $i){
	$decoded = $encoded = $$i;
	if (strpos($encoded,'x') === 0){
	  $q = myPear_db()->qquery("SELECT c_code,c_value FROM zzz_crypt WHERE c_code='".str_replace('x','',$encoded)."'",self::$debug);
	  while($r=myPear_db()->next_record($q)) $decoded = $r['c_value'];
	}
	$$i = $decoded;
      }
      if (is_numeric($key)) $reply_int[$key] = $value;
      else                  $reply_txt[$key] = $value;
    }
    b_debug::xxx('reply_int: '.b_fmt::joinX(',',$reply_int));
    b_debug::xxx('reply_txt: '.b_fmt::joinX(',',$reply_txt));
    return array($reply_int,$reply_txt);
  }

  /*
   * Encryption 2012
   */
  static function decodeZ($coded) {
    $string = '';
    $s = array();
    // ZZZ and Z is now the same, shrink ZZZ to Z
    foreach(explode(',',str_replace('Z',',',str_replace('ZZZ',',',$coded))) as $v){
      if (empty($item)) $item = $v;
      else { $s[] = "$item=$v"; $item = ''; }
    }
    $c = join(',',$s);
    for($i=strlen($c); $i>=0; $i--) $string .= substr($c,$i,1);
    return $string;
  }

  /*
   * The spaces are optionally suppressed
   */
  public static function urlize($str, $delimiter='', $replace=array()) {
    if(!empty($replace)) $str = str_replace((array)$replace, ' ', $str);
    return b_fmt::translit(trim($str));
  }

  private static function hex2int ( $val ) {
    if ( ! strlen ( $val))    return 0;
    switch ( strToUpper ( $val ) ) {
    case "0": return 0;
    case "1": return 1;
    case "2": return 2;
    case "3": return 3;
    case "4": return 4;
    case "5": return 5;
    case "6": return 6;
    case "7": return 7;
    case "8": return 8;
    case "9": return 9;
    case "A": return 10;
    case "B": return 11;
    case "C": return 12;
    case "D": return 13;
    case "E": return 14;
    case "F": return 15;
    }
    return 0;
  }
}

/*
 *
 */
function bTiming(){
  static $class = Null;
  if ($class === Null) $class = new b_timing();
  return $class;
}

class b_timing{

  var $call_stack = array();
  var $tags = array();
  var $cpus = array();
  var $shet = array();
  var $cald = array();
  var $level= 0;


  function __construct(){
    $this->time0 = b_time::microtime(True); 
    $this->doTheWork = (b_cnf::get('timing') && !cnf_inside_fb); 
  }
  
  function cpu($identificationString=Null){
    if ($this->doTheWork){

      if(!empty($identificationString)){

	if (is_array($identificationString)) $identificationString = join('.',$identificationString);

	$this->call_stack[] = $identificationString;
	if (count($this->call_stack)>33) b_debug::internalError('too deep call stack... why?',$this->call_stack);

	$id = join('/',array_merge($this->call_stack));

	$this->times0[$id][] = b_time::microtime(True);
	$this->cald[$id] = max(count(@$this->times0[$id]),@$this->cald[$id]);

      }else{

	$id = join('/',array_merge($this->call_stack));
	if (empty($this->times0[$id])) $this->times0[$id] = array();
	array_pop($this->call_stack);

	$cpu = b_time::microtime(True) - array_pop($this->times0[$id]);
	if (!isset($this->cpuMin[$id])) $this->cpuMin[$id] = $cpu;
	@$this->cpus[$id] += $cpu;
	$this->cpuMin[$id] = min($cpu,$this->cpuMin[$id]);
	if (@$this->cpuMax[$id] < $cpu)	$this->cpuMax[$id] = $cpu;
	@$this->shet[$id]++;
      }
    }
  }

  /*
   *
   */
  function show(){
    static $own_level = array('bAuth',
			      'transaction',
			      'locateAndInclude','load_located_file',
			      'getValue','getMembers','getMember','get_memberInfo','getButton',
			      'qSELECT*','qSELECTx','qNEXT','qUPDATE','qALTER','qINSERT',
			      'outOfScope','e_ID');
    
    if ($this->doTheWork && !empty($this->cpus)){
      $tt = new b_table();      $tt->tro(); $tt->tdo();

      // timings
      $t  = new b_table_zebra(array('id' =>'',
				    'pr' =>'CPU<br/>%',
				    'cpu'=>'CPU<br/>sec.',
				    'n'  =>'N<br/>calls',
				    's'  =>'mSecs<br/>aver.',
				    'mx' =>'mSecs<br/>range',
				    'fr' =>'form',
				    ));
      $t->showLineCounter = False;
      $keys = array_keys($this->cpus);
      foreach ($keys as $id){
	foreach($own_level as $item){
	  if (strpos($id,$item) !== (strlen($id)-strlen($item))) continue;
	  $id_virt = '___'.$item.'___';
	  if (empty($this->cpuMin[$id_virt])) $this->cpuMin[$id_virt] = $this->cpuMin[$id];
	  if (empty($this->cpuMax[$id_virt])) $this->cpuMax[$id_virt] = 0;
	  @$this->cpus[$id_virt] += $this->cpus[$id];
	  @$this->shet[$id_virt] += $this->shet[$id];
	  $this->cpuMin[$id_virt] = min(@$this->cpuMin[$id],@$this->cpuMin[$id_virt]);
	  $this->cpuMax[$id_virt] = max(@$this->cpuMax[$id],@$this->cpuMax[$id_virt]);
	  break;
	}
      }
      
      ksort($this->cpus);
      $lost = $this->cpus[''] = b_time::microtime('getTotal');
      foreach (array_keys($this->cpus) as $id){
	$pr = (int)(0.5+$this->cpus[$id]*100/$this->cpus['']);
	$cpu= (float)$this->cpus[$id]/(float)b_time::microtime('getNorma');
	$strong = 'x';
	if (empty($id)){
	  $cpul = (float)$lost/(float)b_time::microtime('getNorma');
	  $prl = (int)(0.5+$lost*100/$this->cpus['']);
	  print "<tr><td></td></tr>";
	  $t->prt(array('id' =>x('em','??? black hole'),
			'cpu'=>x('em',$cpul),
			'pr' =>x('em',"<b>$prl%</b>")));
	  print "<tr><td></td></tr>";
	}elseif (strpos($id,'/')===False && strpos($id,'___')===False){
	  $strong = 'strong';
	  $lost = $lost - $this->cpus[$id];
	  if ($pr > 0) print "<tr><td></td></tr>";
	}
	// Don't show entries with less then 1% of the total cpu 
	if ($pr > 0){
	  $resultat = array('id' =>x($strong,preg_replace('/bDB_myPear.|bHolder_|bList_|bUnit_|bForm_|getInstance|\(|\)/','',
							  str_replace('processTemplates','templates',
								      str_replace('/load_located_file','/load',
										  str_replace('____','___locateAndInclude___',
											      str_replace('locateAndInclude','l&i',
													  str_replace('___locateAndInclude___','____',
														      str_replace('__construct','NEW',
																  str_replace('isWritable','isWrtbl',
																	      str_replace('::','.',$id)))))))))),
			    'n'  =>x($strong,@$this->shet[$id]),
			    'mx' =>(@$this->shet[$id] > 1 ? x($strong,$this->cpuMin[$id].'-'.$this->cpuMax[$id]) : ''),
			    's'  =>@$this->shet[$id] ? x($strong,(int)(0.5+$this->cpus[$id]/$this->shet[$id])) : '',
			    'cpu'=>x($strong,$cpu),
			    'pr' =>(empty($pr)  ? '' : x($strong,"<b>$pr%</b>")),
			    'd'  =>@$this->cald[$id]>1 ? x($strong,$this->cald[$id]) : '',
			    );
	  if (cnf_CLI) b_debug::print_r($resultat);
	  else	                $t->prt($resultat);
	}
      }
      $t->close();
      
      $tt->tdc();  $tt->tdo();

      // counters
      if (b_cnf::get('timing')=='all'){
	$t = new b_table_zebra(array('id' =>'',
				     't'  =>'Time tag'));
	$level= 1;
	foreach ($this->tags as $tag){
	  foreach($tag as $id=>$timeStemp){
	    if (preg_match('/END/',  $id)) $level--;
	    $t->prt(array('id' =>$this->_prefix($level).$id,
			  't'  =>(float)($timeStemp - $this->time0)/(float)b_time::microtime('getNorma')));
	    if (preg_match('/START/',$id)) $level++;
	  }
	}
	$t->close();
      }
      $tt->tdc();  $tt->trc();  $tt->close();
    }
    $this->cpus = array();
    
    // bList counters
    locateAndInclude('bUnit');
    ksort(bList::$counterL);
    ksort(bUnit::$counterU);
    new b_table_simple(array(b_debug::print_r(bList::$counterL,'bList',True),
			     b_debug::print_r(bUnit::$counterU,'bUnit',True)),
		       "id='counters' class='centralized' style='width:90%;'");
  }
  
  function tag() {}
  
  protected function _prefix($level){
    $s=''; 
    for($n=0; $n< $level; $n++) $s .= '&nbsp;&nbsp;&nbsp;';
    return $s;
  }

  protected function _id($identificationString,$class=''){
    $i = array();
    if (is_object($class)){
      if    (!empty($class->ID))       $i[] = $class->ID;
      elseif(!empty($class->parent_ID))$i[] = $class->parent_ID;
      if ($identificationString)       $i[] = $identificationString;
      return get_class($class).x('(',join(',',$i));
    }else{
      return $identificationString;
    }
  }
}

/*
 *
 */
function bCount(){
  static $class = Null;
  if ($class === Null) $class = new counter();
  return $class;
}

class counter {
  var $count  = array();
  var $verbose=True;
  
  function __construct($verbose=True) { 
    $this->verbose = $verbose;
    $this->reset(); 
  }

  function reset() { 
    $this->count = array(); 
    $this->timer = b_time::microtime();
  }
  
  function add($class, $id, $id2='') {
    if (!cnf_inside_fb) @$this->count[$this->c_id($class)][$this->id($id,$id2)]++;
  }
  
  function get($class, $id=False, $id2='') {
    if ($id)  return $this->count[$this->c_id($class)][$this->id($id,$id2)];
    else      return $this->count[$this->c_id($class)];
  }

  function show() {
    if ($this->count && !cnf_CLI && (b_cnf::get('counter')||b_cnf::get('count')||b_cnf::get('timing'))){
      $timer = b_time::microtime();
      print "<div class='align_left'><blockquote><b>counters (timer=".(int)(1000*($timer - $this->timer))." Msecs)</b><blockquote>";
      $t = new b_table_zebra(array('class'=>'class',
				   'id'   =>'id',
				   'c'    =>'N'));
      $t->class['class'] = 'b_nowrap';
      $t->class['id']    = 'b_nowrap';
      $t->repetValuesCol1only = 'class';
      $t->skipDuplicatedLines = False;
      $t->showLineCounter     = False;
      
      ksort  ($this->count);
      foreach($this->count as $class=>$x){ 
	ksort  ($x);
	foreach($x as $id=>$c) $t->prt(array('class'=>($class ? $class : '???'),
					     'id'   =>str_replace('bForm_','',$id),
					     'c'    =>$c));
      }
      $t->close();
      print "</blockquote></blockquote></div>";
    }
  }

  private function c_id($class) {
    if (is_object($class)) return get_class($class);
    else                   return $class;
  }

  private function id($id, $id2='') {
    return (string)$id;
  }
}

function internalCall() {
  if (bAuth::authenticated())           return True;
  return preg_match("/^130.(237.20[58]|242.12[89])/",
                    $_SERVER["REMOTE_ADDR"]);
}


/*
 * CMS specific functions
 */
class b_cms{
  
  /*
   * Get the URL path element (whatever it means) 
   */
  static public function _($pathElement=Null){
    locateAndInclude('b_registry');
    b_reg::_(False);
    $reply = b_reg::$current_module;
    if ($pathElement === Null) $pathElement = -1;
    switch((int)$pathElement){
    case -1: $reply = b_reg::$current_path;   break;
    case  0: $reply = b_reg::$current_module; break;
    default: $reply =@b_reg::$current_path[$pathElement];
    }
    b_debug::xxx($reply,array(2,'oliveText',cnf_show_l));
    return $reply;
  }

  /*
   * Check that the theme (aka "flavor") exists.
   */ 
  static function themeExists($flavor='',$only_active=False){
    $reply = False;
    if (empty($flavor)) $flavor = b_cnf::get('flavor');
    if(!empty($flavor)){
      switch(HOSTED_BY_CMS){
      case 'drupal':
	if (function_exists('drupal_bootstrap') && function_exists('list_themes')){
	  drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);
	  $themes = list_themes();
	  $reply = ($only_active
		    ? !empty($themes[$flavor]->status) 
		    : !empty($themes[$flavor]));
	}
      }
    }
    return $reply;
  }

  /*
   * Impose the theme (aka "flavor" aka "template") 
   */ 
  static function setTheme($desired_theme='') {

    // The priorities for the theme are:
    myOrg();
    foreach(array($desired_theme,
		  @$_GET['theme'],
		  @$_GET['flavor'],
		  myOrg_theme,
		  b_cnf::get('flavor'),
		  ) as $f){
      if (!empty($f) && self::themeExists($f)){
	$flavor = $f;
	if ($flavor === b_cnf::get('flavor')){
	  if(cnf_dev) b_debug::xxx("'$flavor' already is the desired theme");
	  return;
	}else{
	  b_cnf::set('flavor',$flavor);
	  if(cnf_dev) b_debug::xxx("set '$flavor' as a desired theme");
	}
	break;
      }
    }

    if (empty($flavor) || cnf_CLI){
      if (!empty($desired_theme)) MSG::WARNING("The desired theme \"$desired_theme\" does not exist");
      return;
    }elseif (($current_flavor = @$_SESSION[myPear_cache][__METHOD__]) === $flavor){
      return;
    }
    
    // Impose the theme 
    b_debug::xxx("Trying to change theme to '$flavor'...",cnf_dev);
    switch(HOSTED_BY_CMS){
    case 'drupal':	
      
      global $theme, $theme_key;

      switch(HOSTED_BY_CMS_vrsn){
      case 8:
	break;

      case 7:
	// Ensure that system.module is loaded.
	if (!function_exists('_system_rebuild_theme_data')) {
	  $module_list['system']['filename'] = 'modules/system/system.module';
	  module_list(TRUE, FALSE, FALSE, $module_list);
	  drupal_load('module', 'system');
	}
	if (!empty($current_flavor)) MSG::WARNING("Fail (don't know how to do it) to change theme to '$current_flavor --> $flavor'");
	variable_set('theme_default', $flavor);
	break;
      case 6:
	// If $theme is already set, try to reset it
	if (function_exists('drupal_bootstrap')){
	  drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);
	  $themes = list_themes();
	  $theme  = !empty($themes[$flavor]->status) 
	    ? $flavor 
	    : variable_get('theme_default', 'garland');
	  
	  // Store the identifier for retrieving theme settings with.
	  $theme_key = $theme;
	  
	  // Find all our ancestor themes and put them in an array.
	  $base_theme = array();
	  $ancestor = $theme;
	  while ($ancestor && isset($themes[$ancestor]->base_theme)) {
	    $base_theme[] = $new_base_theme = $themes[$themes[$ancestor]->base_theme];
	    $ancestor = $themes[$ancestor]->base_theme;
	  }
	  if (function_exists('_init_theme')){
	    _init_theme($themes[$theme], array_reverse($base_theme));
	  }else{
	    require_once DRUPAL_ROOT . '/includes/theme.maintenance.inc';
	    _drupal_theme_initialize($themes[$theme], array_reverse($base_theme), '_theme_load_offline_registry');

	    // These are usually added from system_init() -except maintenance.css.
	    // When the database is inactive it's not called so we add it here.
	    $path = drupal_get_path('module', 'system');
	    drupal_add_css($path . '/system.base.css');
	    drupal_add_css($path . '/system.admin.css');
	    drupal_add_css($path . '/system.menus.css');
	    drupal_add_css($path . '/system.messages.css');
	    drupal_add_css($path . '/system.theme.css');
	    drupal_add_css($path . '/system.maintenance.css');
	  }
	}
	$_SESSION[myPear_cache][__METHOD__] = $theme;
	break;
	
      case 8:
      default:
	break;
      }
      break;
      
    default:
    }
  }
}

/*
 * URL support
 */
class b_url{

  /*
   * add http(s)://host in front of url
   */
  static function _($url_,$replace=False){
    $url = trim($url_);
    $http = sprintf('http%s://',($_SERVER['SERVER_PORT'] == 80 ? '' : 's'));
    if ($replace){
      if (stripos($url,'https://') === False) $url = preg_replace('|^http:/+|i',$http,$url);
    }else{
      $url = sprintf('%s%s/%s',$http,@$_SERVER['HTTP_HOST'],$url);
    }
    if ($url !== $url_) b_debug::xxx($url,array(2,'redText',True));
    return $url;
  }

  /*
   * add http(s)://host/PHP_SELF/ in front of query_string to get a complete url
   */
  static function complete_url($QUERY_STRING,$PHP_SELF=''){
    $QUERY_STRING = trim($QUERY_STRING);
    if (empty($PHP_SELF)) $PHP_SELF = $_SERVER['PHP_SELF'];
    $url = self::_('') . dirname($PHP_SELF) . (strpos($QUERY_STRING,'?') === 0 ? '' : '?') . $QUERY_STRING;
    return $url;
  }

  /*
   * Wrapper around standard function "header"
   * Returns FALSE if the redirect was not succesful
   */
  static function header($query){

    if (is_string($query)) $query = trim(str_ireplace('location:','',$query));
    b_debug::xxx($query);
    $repack = self::_(is_array($query) || (is_string($query) && (strpos($query,'?') === 0))
		      ? self::repack($query)
		      : $query);
    b_debug::xxx($repack);
    $location = str_ireplace('&amp;','&',$repack);

    // Debugging message
    $query_string = (is_array($query)
		     ? b_fmt::joinX('&',$query)
		     : $query);
    $msg = __METHOD__."($query_string)<br/>Redirect (see below) is ignored, execution continues<br/>$location";
    
    // Go on...
    if (cnf_show_h){
      // ignore redirects. Not safe, but needed for debugging
      $msg = "Redirects are cancelled by the configuration<br/>$msg";
    }elseif (cnf_useTrickyHeader){
      // JS-bases redirect
      $location .= (strpos($location,'?')===False ? '?' : '&').'cnf_useTrickyHeader_once=Y';
      if (cnf_dev) b_debug::xxx($location);

      session_write_close();
      ob_get_contents();
      ob_end_clean();
      $js = "<script>location.href='$location'</script>";
      b_debug::xxx(b_fmt::escape($js));
      print $js;
      return True;
    }else{
      // Usual header(location: url)
      if (headers_sent($file,$line)) b_debug::internalError("Headers already sent in \"$file:$line\"",headers_list());

      // It seems that adding "session" to the url ends up in "locking" the page (???)
      if (False)      $location .= (strpos($location,'?')===False ? '?' : '&').session_name().'='.session_id();
      if (cnf_dev) b_debug::xxx($location);

      session_write_close();
      header("location: $location");
      b_debug::internalError("header(location:url) IS NOT EXECUTED",headers_list());
    }
    b_debug::xxx($msg);
    return False;
  }
  
  /*
   * invoke php function if available, otherwise the curl executable
   */
  static function curl($url){
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
      $page = `$page`;
    }
    return $page;
  }

  /*
   *
   */  
  private static function repack_init(){
    if (!empty(b_cnf::$_GET_dropped_by_drop_all_array)){
      locateAndInclude('b_registry');
      foreach(b_cnf::$_GET_dropped_by_drop_all_array as $item)b_cnf::$_GET_never_dropped[] = $item;
      foreach(b_reg::get_modules() as $m)            b_cnf::$_GET_never_dropped[] = "${m}_.*";
      //b_debug::print_r(b_cnf::$_GET_never_dropped,'_GET_never_dropped');
      //b_debug::print_r(b_cnf::$_GET_dropped_by_drop_all_array,   '_GET_dropped_by_drop_all_array');
      b_cnf::$_GET_never_dropped = '^('.join('|',b_cnf::$_GET_never_dropped).')$';
      b_cnf::$_GET_dropped_by_drop_all = '^('.join('|',b_cnf::$_GET_dropped_by_drop_all_array).')$';
      b_cnf::$_GET_dropped_by_drop_all_array = array();
    }
  }

  /*
   * The URL builder engine
   */
  public static $no_crypt = False;
  static function repack($url,$myGET=array(),$myDrop=array()){
    self::repack_init();

    // Parse the initial URL, add the "php_self" if missing
    if (strpos($url,'#') === False)    $anchor = '';
    else list($url,$anchor) = explode('#',$url,2);

    $url = trim(preg_replace('/^location:/i','',trim($url)));
    if (is_string($url)){
      if (preg_match('#https?:/#',$url)) return $url;
      if (strpos($url,'?') === 0) $url = str_replace('/index.php','/',$_SERVER['PHP_SELF'] . $url);
      $_url = b_crypt::parse_query_string($url);
      $_prf = b_crypt::$parse_query_string_arg0;
    }elseif(is_array($url)){
      $_url = $url;
      $_prf = $_SERVER['PHP_SELF'] .'?';
    }else{
      b_debug::internalError("unexpected argument");
    }

    // Add the "environment" 
    foreach(b_cnf::$_GET_to_keep as $keep){
      if (isset($_url[$keep])) continue;
      if(!($v = @$_GET[$keep])) $v = b_cnf::get($keep);
      if  ($v)   $_url[$keep] = $v;
    }

    // Drop the authentication (z,x,data) and browser-generated items (_button_x,_button_y),
    // Always keep the all the modules items.
    if (!is_array($myDrop)) $myDrop = array();
    $_myDrop = array_diff(//$a1=self::get_GET(array_merge($myDrop,array('z','x','data','_button_x','_button_y'))),
			  $a1=array_merge($myDrop,array('z','x','data','_button_x','_button_y')),
			  $a2=self::get_GET(b_cnf::$_GET_never_dropped,True),
			  $a3=b_cnf::$_GET_to_keep);
    
    // Get the final clean list
    $_myGET = $myGET;
    if (!fancybox3) $_url['modal_once'] = bJS()->modal_once();
    if (!is_array($_myGET)) $_myGET = array();
    foreach(array_diff(array_keys($_url),
		       array_keys($_myGET),
		       $_myDrop) as $k){
      $_myGET[$k] = $_url[$k];
    }
    
    // Keep the "checkbox" selections 
    $_myDrop = array_unique($_myDrop);
    //$clean_once=preg_grep('#^(?!(.*_once)$)#i',...
    if ($clean_once = array_intersect(array_merge($_myDrop,explode(',',@$_myGET['clean_once'])),
				      self::get_GET())){
      $_myGET['clean_once'] = join(',',$clean_once);
    }

    // Encode the URL
    if (!cnf_show_e && !self::$no_crypt) $_myGET = b_crypt::encodeURL($_myGET);
    self::$no_crypt = False;

    // Workaround for D8
    if ((HOSTED_BY_CMS_vrsn == 8) && ($_prf =='?')) $_prf = '/'.$_myGET['q'].$_prf;

    // Return encoded the result in the form of URL
    $reply = trim($_prf) . trim(b_fmt::joinX('&amp;',$_myGET,''));
    if ($anchor) $reply .= '#'.$anchor;
    return $reply;
  }


  /**
   * Parse the arguments and create URL to the same page as the current one
   * @param 
   *   $myGET (array) keys to be added to the resulting query string
   * @param 
   *   $drop  (array) keys to be dropped from the resulting query string
   */
  static function same($myGET=array(), $myDrop=array(), $asA=''){

    self::repack_init();

    if (!is_array($myDrop)){
      $anchor = empty($myDrop) ? '' : '#'.$myDrop;
      $myDrop = array();
    }else{
      $anchor = '';
    }
    
    $_myDrop = $myDrop;
    $_myGET  = b_crypt::parse_query_string($myGET);
    
    foreach(array($myDrop,$asA) as $arg){
      if (is_string($arg) && !empty($arg)){
	switch($arg){
	case 'drop_all':
	  $_myDrop = self::get_GET(b_cnf::$_GET_dropped_by_drop_all);
	  break;
	  
	case 'keepAll':
	  $remove_gets = array(); 
	case 'keepAllBut':
	  if (!isset($remove_gets)) $remove_gets = self::get_GET('^(.*_once|action)$',True);
	  foreach(array_diff(array_keys($_GET),
			     array_keys($_myGET),
			     $remove_gets) as $k){
	    $_myGET[$k] = $_GET[$k];
	  }
	  break;
	  
	default:
	  b_debug::traceBack("??? forgot '$arg'");
	}
      }
    }
    $reply = self::repack('?',$_myGET,$_myDrop);
    // Workaround for D8
    if (HOSTED_BY_CMS_vrsn == 8) $reply = D8::compat($_GET['q']).$reply;
    $reply .= $anchor;
    return $reply; 
  }
  

  /*
   * Get names of the (also cached) components of $_SERVER["QUERY_STRING"]
   */
  public static function get_GET($include_GET=True,$regexp=False){
    if (empty($_SESSION[CONST_cnfOptions]))  $_SESSION[CONST_cnfOptions] = array();
    $reply = array_keys($_SESSION[CONST_cnfOptions]);

    if (!empty($include_GET)) $reply = array_unique(array_merge(array_keys($_GET),$reply));
    if ($regexp)              $reply = array_unique(preg_grep("/$include_GET/i",$reply));

    // Never report the "untouchable"
    $reply = array_diff($reply,b_cnf::$_GET_to_keep);

    sort($reply); // kill the association
    return $reply;
  }


  
  /*
   * Get referrer
   */
  static public  $getReferrer_comment = '';
  static public  $referrer_debug = False;
  static function getReferrer($args=array(),$id='common'){
    $url = '';
    if (!cnf_inside_fb){
      if ((func_num_args()==1) && is_string($args)){
	$id  = $args;
	$args= array();
      }
      if ($url = @$_SESSION['core_referrer'][$id]['url']){
	$url = b_url::repack($url,$args);
	self::$getReferrer_comment = @$_SESSION['core_referrer'][$id]['cmt'];
	b_debug::xxx($url,array(2,'oliveText',cnf_show_r||self::$referrer_debug));
      }
    }
    return $url;
  }
  
  /*
   * Set referrer
   */
  static function setReferrer($id='common', $url=Null, $comment=''){
    if (empty($_SESSION['core_referrer'])) $_SESSION['core_referrer'] = array();
    if (!cnf_inside_fb){
      if ($url === 'reset'){
	unset($_SESSION['core_referrer'][$id]);
      }else{
	if ($url === Null) $url = @$_SERVER["REQUEST_URI"];
	if (strpos($url,'?') === False) $url .= '?';
	$_SESSION['core_referrer'][$id] = array('url'=>b_url::repack($url,array(b_crypt_no=>1)),
						'cmt'=>$comment);
      }
      b_debug::xxx(@$_SESSION['core_referrer'][$id]['url'],array(2,'oliveText',cnf_show_r||self::$referrer_debug));
    }
  }
  
}

/*
 * Server OS specific functions
 */
class b_os{
  
  private static $os    = Null;
  public  static $cache = Null;

  /*
   *
   */
  static function needPhpVersion($vrsn,$file='') {
    list($najor,$ninor,) = preg_split('/\./',$vrsn,3);
    list($major,$minor,) = preg_split('/\./',phpversion(),3);
    if (!$file) $file = __FILE__;
    if (100*$major+$minor < 100*$najor+$ninor)
      b_debug::internalError("'".basename($file)."' requires the php version better then $vrsn<br/>".
			     'Your system runs php version '.phpversion().',<br/>'.
			     'please consider to upgrade');
  }

  private function __construct(){
    locateAndInclude('b_cache');
    self::$cache = new b_cache_S($this);
  }

  static function unserialize($serialized){
    $reply = array();
    $er = error_reporting(0);
    if (is_array($p = unserialize($serialized))) $reply = $p;
    error_reporting($er);
    return $reply;
  }

  static function unlink($file){
    $er = error_reporting(0);
    unlink($file);
    error_reporting($er);
  }
  
  /**
   * Match a file name from a repository.
   * In case of multiple matches, the first one is taken
   * @param return the filename which matches input arguments
   */
  static function file_match($match=array(),$repository=myPear_photos_path,$reset=False) {

    if (is_dir($repository)){
      static $cache = Null;
      if  (!($cache instanceof b_cache_S)) $cache = new b_cache_S(__METHOD__);
      
      if  ($cache->wasSet($repository) && !$reset){
	$c = $cache->get();
      }else{  
	$c = scandir($repository,1);
	$c = $cache->set($c);
      }
      if (!empty($c)){
        foreach($match as $m){
          if ($reply = array_values(preg_grep("/^$m/i",$c)))   return  $reply[0];
        }
      }
    }
    return '';
  }

  /*
   * Write an array as an Excel file
   */  
  public static function write_excel_file($file,$input,$header = array()){
    $bytes = 0;
    
    if (!empty($input)){
      $fn = myPear_tmp . date('Y-m-d_',time()) . basename($file);
      $fp = fopen($fn, 'w');
      // Inform Excel that the encoding is UTF8
      // fputs($fp, "\xEF\xBB\xBF"); // UTF-8 BOM !!!!!
      foreach ($input as $fields){
	// foreach($fields as $k=>$f) $fields[$k] = mb_convert_encoding($f, 'UTF-16LE', 'UTF-8');
	if (($n = fputcsv($fp,$fields,',','"')) === False) b_debug::internalError("Can't export $file");
	$bytes += $n;
      }
      fclose($fp);
    }
    if (empty($bytes)){
      MSG::ERROR("Can not open Excel output file");
    }else{
      $bytes = (($bytes > 1023)
		? ((int)(($bytes+512)/1024)).'KB'
		: "$bytes Bytes");
      MSG::MESSAGE(// "Written $bytes of data.". 
		      " Click <a href='$fn'>".bIcons()->get('i-m_excel').
		      "</a> to download Excel file \"".str_ireplace('csv','',basename($file))."\"");
    }
  }

  /*
   *
   */
  static function realpath($path){

    $reply = False;
    switch(self::type()){
    case 'lin':
    case 'dar':
      // Unix-like systems
      if ($is_file = (file_exists($path) && !is_dir($path))){
	$fn   = basename($path);
	$path = dirname($path);
      }

      $virt_path = array();
      $existing_path = explode(DIRECTORY_SEPARATOR,$path);
      
      // Go down the directory tree if the path is not yet created
      while(count($existing_path)){
	$p = implode(DIRECTORY_SEPARATOR,$existing_path);
	if ($r=is_dir($p) || is_dir(getcwd().DIRECTORY_SEPARATOR.$p)){
	  break;
	}else{
	  array_unshift($virt_path,array_pop($existing_path));
	}
      }
      
      // Get the true path 
      $reply = implode(DIRECTORY_SEPARATOR,array_merge(explode(DIRECTORY_SEPARATOR,getcwd()),$existing_path,$virt_path));
      if (!empty($reply) && $is_file) $reply .= DIRECTORY_SEPARATOR . $fn; 
      break;

    default:
      $reply = realpath($path);
    }

    // Just for beauty...
    $reply = str_replace(DIRECTORY_SEPARATOR.DIRECTORY_SEPARATOR,DIRECTORY_SEPARATOR,$reply);
    // b_debug::xxx($reply);
    return $reply;
  }

  /*
   *
   */  
  static function is_what($object){
    if (!file_exists($object))$reply = Null;
    elseif(is_link($object))  $reply = 'link';
    elseif(is_file($object))  $reply = 'file';
    elseif(is_dir ($object))  $reply = 'dir';
    return $reply;
  }

  // ln -s $target $link
  static function symlink($target,$link){
    $er = error_reporting(0);
    $link = self::url2path($link);
    if (is_dir($pd=dirname($link)) && is_dir($pd.DIRECTORY_SEPARATOR.$target) && !is_dir($link)){
      $cmd = "ln -s '$target' '$link'";
      self::exec($cmd,'fatal');
    }
    error_reporting($er);
  }

  /*
   * Same as mkdir, but with an option to skip the dir creation
   */
  static function mkdir($dir){
    static $umask = 0755;
    $reply = True;
    if (!cnf_noFS){
      //      $l = error_reporting(0);
      if (!is_dir($dir)){
	$oldumask = umask(0); 
	try{
	  $ok = mkdir($dir,$umask,True);
	}catch(Exception $e){
	  b_debug::traceBack("Cant create $dir: ".$e->xdebug_message);
	}
	umask($oldumask); 
	$text   = preg_replace(';.*/trunk/;','&lt;trunk&gt;/',str_replace(getcwd(),'&lt;cwd&gt;',$dir));
	if (cnf_dev && $ok) MSG::MESSAGE("creating $text umask=".$umask);
      }
      if (!is_dir($dir)){
	MSG::ERROR(b_debug::__()." can't create '$dir'");
	$reply = False;
      }
      //      $l = error_reporting($l);
    }
    return $reply;
  }

  /**
   * @return convert '/' to 'DIRECTORY_SEPARATOR'
   */
  static function _($path){
    return str_replace('/',DIRECTORY_SEPARATOR,$path);
  }

  static function php_settings($parameter){ 
    $v = ini_get($parameter);
    $l   = substr($v, -1);
    $ret = substr($v, 0, -1);
    switch(strToUpper($l)){ //Transforms php notation (like '2M') to an integer (2*1024*1024 in this case)
    case 'P':    $ret *= 1024;
    case 'T':    $ret *= 1024;
    case 'G':    $ret *= 1024;
    case 'M':    $ret *= 1024;
    case 'K':    $ret *= 1024;
    }
    if ($v != $ret) b_debug::_debug("$v --> $ret",cnf_dev);
    return $ret;
  }

  /*
   *
   */
  static function max_upload_size(){
    $max_upload_size = min(($p=self::php_settings('post_max_size')), ($f=self::php_settings('upload_max_filesize')));
    b_debug::_debug(sprintf("%dMB (post_max_size=%dMB upload_max_filesize=%dMB",
			    (int)$max_upload_size/1024/1024,(int)$p/1024/1024,(int)$f/1024/1024),
		    True);
    return $max_upload_size;
  }

  /**
   * Convert URL to the system path on the server
   * @param  @url
   * @return path
   */
  public static function url2path($url,$createIfMissing=False){
    $path = self::realpath($url);
    //    b_debug::xxx($path);
    if (!file_exists($path) && $createIfMissing) self::mkdir($path);
    return $path;
  }

  /*
   * Same as the standard PHP function 'pathinfo', but this one parses properly 
   * a path without a filename, i.e. 'dirname/'
   */
  public static function pathinfo($path,$option=Null){
    switch($option){
    case PATHINFO_DIRNAME:
      if (empty($path) || !is_string($path)) return '';
      if (substr($path,-1) === '/')          return substr($path,0,strlen($path)-1);
      else                                   return pathinfo($path,PATHINFO_DIRNAME);
      
    default:
      if (empty($path) || !is_string($path)) return array();
      if (substr($path,-1) === '/')          return array('dirname'=>substr($path,0,strlen($path)-1));
      else                                   return pathinfo($path);
    }
  }

  /**
   * Get the code name of the server operating system
   *
   * @return
   *  lin for Linux
   *  win for Windows
   *  dar for Mac OSX (Darwin)
   * ...
   */ 
  static function type(){
    return (self::$os = strToLower(substr(PHP_OS,0,3)));
  }

  /*  
   * NOTE - any output is considered as a failure !!
   */
  static function exec($cmd,$ifFails='print') {
    $reply = '';
    if (!preg_match('/(2>|2\s>)/',$cmd)) $cmd = "{ $cmd; } 2>&1";
    if (cnf_debug || cnf_show_exec){
      if (cnf_CLI) print "$cmd\n";
      else         MSG::DEBUG("b_os::exec:<br/>".str_replace(';','<br/>',$cmd));
    }
    ob_start();
    system("$cmd");
    $err = ob_get_contents();
    ob_end_clean();
    if ($err) {
      switch ($ifFails) {
      case '>>>':
	// Do not print if the output does not contain symbol '>>>'
	if (strpos($err,$ifFails) === False) break;

      case 'print':
	MSG::WARNING($msg=x('pre',b_fmt::escape($err))); 
	if (cnf_dev) b_debug::xxx($msg);
	break;

      case 'fatal':
	b_debug::internalError("$cmd<br/>$err");

      default:   
       $reply = $err;
      }
    }
    return $reply;
  }

  static function join($dirs = array()){
    if (!is_array($dirs)) b_debug::internalError(__FUNCTION__." '$dirs' is not an array");
    return join(DIRECTORY_SEPARATOR,$dirs);
  }

  static function getFilename_noExt($path) {
    $i = strrpos($path, '.');
    if ($i == false)               return $path;
    $j = strrpos($path, '/');
    if ($j == false || $j < $i)    return substr($path, 0, $i);
    else                           return $path;
  }
  
  static function path(){
    return 'PATH=/opt/local/bin:/sw/bin:/usr/local/bin:$PATH; export PATH;';
  }

  /*
   * @param $name
   * @param $file
   * @param $localURL (optional)
   *   reference to the local copy of the software
   * $param $develURL (optional)
   *   url to the developer site, takes precedence over $localURL
   */ 
  static function needExecutable($name,$file='',$localURL='',$develURL=''){
    $cmd = b_os::path() . "which $name 2>/dev/null";
    $cmd = trim(`$cmd`);
    if (!$cmd){
      if (!$file) $file = __FILE__;
      if ($url=$develURL){
	$tmpFile = $develURL;
      }elseif ($url=$localURL){
	$tmpFile  = myPear_tmp . basename($url);
	$from = realpath(dirname($file).DIRECTORY_SEPARATOR.$url);
	if (file_exists($from)){
	  if (!is_file($tmpFile)) copy(realpath(dirname($file).DIRECTORY_SEPARATOR.$url),$tmpFile);
	}else{
	  MSG::WARNING_T("'$from' does not exist");
	}
      }
      $path = b_os::path() . 'echo $PATH;';
      MSG::WARNING("'".basename($file)."' needs an executable '$name',".
		      "which in not installed on your server '".`hostname`."'<br/>within the search PATH=".`$path`.'<br/><br/>'.
		      'Please upgrade your software. '.($url
							? "<br/><a href='$tmpFile'>Download $name from here.</a><br/>"
							:"<br/>"));
    }
  }
}

/*
 * Keep configuration settings
 */
class b_cnf{

  /*
   * Basic settings
   */
  private static $never_cache = array(); 
  //
  // Those _GET variables are always kept across the session, 
  // they define the global myPear context 
  //
  public static $_GET_to_keep = array('flavor',   // Current flavor (aka theme)
				      'group',    // Current organization
				      'org',      // Current organization
				      'option',   // Joomla URL
				      'q');       // Drupal URL
  //
  // Those variables are used durng edit/search sessions and should be cleaned 
  // if the edit/search session is interrupted by the "cancel" button 
  //
  public static $_GET_dropped_by_drop_all_array= array('function',     //
						       'ed_bList',     // bUnit / bList dialog
						       '(lm|act)_.*',  // bEdit dialog 
						       'form',         // bForm class-name & ID 
						       '_after_.*',    // search dialog 
						       '(id|av_id)',         
						       );
  public static $_GET_dropped_by_drop_all = Null;
  
  // 
  // Those variables define the "local context", and should not be cleaned if (any) session is interrupted by "cancel"
  //
  public static $_GET_never_dropped      = array('myPear_.*',    // myPear defaults
						 'cnf_.*',       // Configuration sections
						 'sBy.*',        // Sort By when showing various lists
						 '_checkbox_.*', // Various selections during the session
						 'timing',       // timing request 
						 'YBhere',       // as stated
						 '(warnings|.*debug.*|show.*|no(drupal|cache))',  // debug switched
						 );
  /*
   * Give back the names of the caches _GET variables
   */
  static function _(){
    if (empty($_SESSION[CONST_cnfOptions])) $_SESSION[CONST_cnfOptions] = array();
    return array_keys($_SESSION[CONST_cnfOptions]);
  }


  /*
   * Check the set of conditions
   *
   */
  public static function check_conditions($c_array,$class,$verbose=False){
    if (cnf_dev && !is_object($class)) b_debug::traceBack("arg is not object");
    $reply = Null;
    $msg   = '?';
    foreach($c_array as $answer=>$conditions){
      if (!is_array($conditions)) $conditions = array($conditions);
      foreach($conditions as $msg=>$condition){
	if ($answer === 'default'){
	  $reply = $condition;
	  $msg   = 'default';
	  break 2;
	}elseif ($condition){ 
	  $reply = $answer; 
	  break 2;
	}
      }
    }
    if (empty($reply)) $reply = False;
    if (cnf_show_w || $verbose || $class::$_debug) b_debug::_debug(($reply ? 'YES - ' : 'NO - ').$msg,array(2,$class,True,$class::$_dbg_color));
    return $reply;
  }


  /**
   * get the option from the _GET and/or _POST 
   * and store it in the _SESSION
   */
  public static function get($key,$default=Null){
    static $inside = 0;    if ($inside++) die("recursion in b_cnf::get($key,$default)");
    if ($ask_only = ($default === 'ask_only')) $default = Null;
    if(!$ask_only) self::set($key);
    $keyL = strToLower($key);
    if (     isset($_SESSION[CONST_cnfOptions][$keyL])){
      $reply= trim($_SESSION[CONST_cnfOptions][$keyL]);
      switch(strToLower($reply)){
      case '1':
      case 'on':
      case 'yes': 
	$reply = True;  
	break;
      case 'no':
      case 'off':  
	$reply = False; 
	break;
      }
    }elseif ($default !== Null){
      $reply = $default;
      if (!$ask_only) self::set($key,$reply);
    }
    if (stripos($keyL,'_once') !== False) unset($_SESSION[CONST_cnfOptions][$keyL]); 
    $inside = 0;

    if (isset($reply))      return $reply;
    else                    return Null;
  }

  /*
   *
   */  
  static function _GET_and_unset($name) {
    static $tmp;
    $tmp = @$_GET[$name]; b_cnf::_unset($name);
    return $tmp; 
  }
  
  /*
   * Save the key from _REQUEST into the session
   * Note, the _POST & _GET arrays might be modified during the execution.
   * _REQUEST is never changed (? to be confirmed) 
   */ 
  public static function set($key, $forcedValue=Null){
    $keyL  = strToLower($key);
    if (empty($_SESSION[CONST_cnfOptions]))                  $_SESSION[CONST_cnfOptions] = array();
    if (isset($_SESSION[CONST_cnfOptions][$keyL])) $oldValue=$_SESSION[CONST_cnfOptions][$keyL];
    if    ($forcedValue !== Null)             $value=$forcedValue;
    elseif(isset($_GET[$key]))                $value=$_GET[$key];
    elseif(isset($_GET[$keyL]))               $value=$_GET[$keyL];
    elseif(isset($_POST[$key]))               $value=$_POST[$key];
    elseif(isset($_POST[$keyL]))              $value=$_POST[$keyL];
    elseif(isset($oldValue))                  $value=$oldValue;
    if (isset($value) && ($keyL === 'org'))   $value=strToLower($value);
    if (!in_array($keyL,self::$never_cache) && isset($value)){
      $_SESSION[CONST_cnfOptions][$keyL] = (string)$value;
    }
    //    if ($keyL == 'act_code') var_dump($_POST,True);
    if (isset($oldValue))  return $oldValue;
  }

  
  /*
   * Unset a GET / POST / Cache variable
   */
  public static function _unset($key_arg,$regexp=False){
    // The key is a regular expression
    if ($regexp) $key_arg = b_url::get_GET($key_arg,True);

    // The key is the key
    if (!is_array($key_arg)) $key_arg = array($key_arg);
    
    // Clean the cache
    foreach($key_arg as $key){
      if (!empty($key)){
	$keyL  = strToLower($key);
	if (($v=@$_POST[$key]) || 
	    ($v=@$_POST[$keyL]) || 
	    ($v= @$_GET[$key]) || 
	    ($v= @$_GET[$keyL]) || 
	    ($v=@$_SESSION[CONST_cnfOptions][$keyL])) $dbg[$key] = $v;
	unset($_GET[$key],
	      $_GET[$keyL],
	      $_POST[$key],
	      $_POST[$keyL],
	      $_REQUEST[$key],
	      $_REQUEST[$keyL],
	      $_SESSION[CONST_cnfOptions][$key],
	      $_SESSION[CONST_cnfOptions][$keyL]);
      }
    }
    if (@$dbg && defined('cnf_dev')) b_debug::print_r($dbg,__method__);
    //    if (@$dbg) b_debug::var_dump($dbg);
}

  
  /*
   * Get input value from (in this order):
   * - $_GET
   * - $_POST
   * - (bForm) $class->formDB
   * - (bForm) $class->_POST[expected] (for check-boxes only)
   * - (no oo) $_SESSION
   */
  static function getValue($name,$class=Null){
    $value = Null;
    if(isset($_GET[$name]))                        list($value,$r) = array(strip_tags($_GET[$name]),      '_GET'); 
    if($class instanceof bForm){
      $debug = (int)$class::$_debug;
      if($value===Null &&$class->myPost_bForm() && isset($_POST[$name]))    
	                                           list($value,$r) = array(strip_tags(@$_POST[$name]),    '_POST');
      if($value===Null)                            list($value,$r) = array(@$class->formDB_modified[$name],'formDB_modified'); 
      if($value===Null)                            list($value,$r) = array(@$class->formDB[$name],        'formDB'); 
    }else{
      $debug = cnf_debug_level;
      if($value===Null && isset($_POST[$name]))  list($value,$r) = array(strip_tags($_POST[$name]),   '_POST');
      if($value===Null)                          list($value,$r) = array(b_cnf::get($name),           '_getOption');
    }
    
    if ($value !== Null){
      $value =  b_crypt::encode_utf8($value);
      b_debug::_debug($value);
    }
    return $value;
  }
  
  /*
   * Interface with CMS.
   * If the variable is not found in the CMS repository, it is taken from $GLOBALS
   */
  static function variable_get($variable,$default=Null){
    $reply = Null;
    if (function_exists('variable_get')){
      $reply = variable_get($variable,$default);
    }elseif(isset($_SESSION[$variable])){
      $reply = $_SESSION[$variable];
    }else{
      $reply = b_cnf::get($variable,$default);
      if (is_null($reply))   $reply = $default;
    }
    return $reply;
  }

  static function variable_set($variable,$value,$media='globals'){
    switch ($media){
    case 'db':
      if (function_exists('variable_set')) $reply = variable_set($variable,$value);
      else b_debug::internalError(b_debug::__()." can't save variable");
      b_debug::xxx('?');
      break;
    default:
      $GLOBALS[$variable] = $value;
    }
  }
}
