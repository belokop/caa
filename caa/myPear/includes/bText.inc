<?php
/*
 * Multi-language support
 *
 * The translation is done by a standard(?) method,
 * when all translatable strings are saved as "key = translation" pairs.
 * There is always a base file en_US, containing all the translatable items,
 * and a optional files with translations to desired languages.
 *
 * The full version has:
 *  - editor for the translation tables
 *  - table builder for the new languages
 */

define('bText' , 'bText');

new   bText();
class bText{

  //public  static $localesToShow = array('sv_SE','ru_RU', 'en_US');
  public  static $localesToShow = array();
  public  static $acceptNewEntries = True;
  public  static $doTranslation = False;

  /*
   * Strings to be ignored when building the key for the translatable string
   */
  private static $dropInKey    = array('/AP_REFEREE./i'  =>'AP_REFEREE',
				       '/AP_EXTRA_DOC./i'=>'AP_EXTRA_DOC',
				       '/AP_REC._/i'     =>'AP_REC_',
				       '/-SKIP-/i'       => '',
				       '/[\s_](A|THE)[\s_]/'     => '',
				       '/\s/'           => '_',
				       '/\W/'           => '',
				       '/[^\x00-\x7F]/' => '',
				       );
  public static $doNotTranslate = array('_post_bForm_ID_once','_post_bForm_class_once','ID','DEBUG','AV_PWD');
  public static $debug = False;  
  public static $dropAfterSKIP = False;
  private static $initDone = False;
  
  public function __construct(){
    if (isset($_GET['doTranslation_once'])) self::$doTranslation = (bool)$_GET['doTranslation_once'];
  }
  
  private static function init(){
    if (!self::$initDone){
      
      /*
       * Set the default language
       */
      if ( cnf_show_t)   self::$debug = True;
      if (!b_cnf::get('apilang')) b_cnf::set('apilang',CONST_localeBase);
      setLocale(LC_TIME, self::localeCompat(b_cnf::get('apilang')));
      if (!in_array(CONST_localeBase,self::$localesToShow)) self::$localesToShow[] = CONST_localeBase;
      
      /*
       * Check the repository of the available languages
       */
      if (empty($_SESSION[bText]['repo'])){
	$_SESSION[bText]['repo'] = array();
	if ($handle = opendir(self::repository())){
	  while (false !== ($file = readdir($handle))) if (preg_match('/^.._..$/',$file)) $_SESSION[bText]['repo'][] = $file;
	  closedir($handle);
	}
      }
      if (($def=b_cnf::get('apilang')) && !in_array($def,$_SESSION[bText]['repo'])) $_SESSION[bText]['repo'][] = $def;
      
      /*
       * load the translation matrix
       */
      if (self::$acceptNewEntries)  $_SESSION[bText]['TM'] = array();
      self::load();
      self::$initDone = True;
      self::dbg(b_cnf::get('apilang'));
    }
  }
  
  private static function localeCompat($code){
    switch ($code = str_replace('-','_',$code)){
    case 'ka_GE':  
    case 'uk_UA':  
    case 'fi_FI':  
    case 'pl_PL':  return $code.'.utf8';
    case 'en_US':  return 'C';  
    default:       return $code;
    }
  }

  /*
   * Get back the translated phrase
   */
  private static function getTranslation($key,$lang=Null){
    if (!$lang) $lang = CONST_localeBase;
    self::load($lang);
    $reply = @$_SESSION[bText]['TM'][$key][$lang];
    return $reply;
  }

  /*
   * NYI
   */
  private static function getIcon($key){
    return '';
    if (isset($_SESSION[bText]['icons']))      b_debug::print_r($_SESSION[bText]['icons']);

    if ($i=$_SESSION[bText]['icons'][$key]) return $i;
    else                                     return '';
  }
  
  /*
   * Fetch the translation
   */
  public static function _($text,$autoLearn=True,$baseLanguage=Null){
    if (self::$doTranslation){
      self::init();
      $reply = cnf_emptyString;
      if (!$baseLanguage)   $baseLanguage = CONST_localeBase;
      
      /*
       * Store the new key (auto-register the new text string).
       * However, this auto-registration should not be abused, 
       * not everything has to be in the translation table,  that's why @param $autoLearn is there
       */
      if (is_array($text)){
	$key = self::_key($text['key']);
	$translation = @$text['item'];
	if ($icon = @$text['icon']){
	  $_SESSION[bText]['icons'][$key] = $icon;
	  b_debug::print_r($text);
	}
      }else{
	$key = self::_key($text,True);
	$translation = $text;
      }
      
      if ($translation){
	// small optimization for the keys build
	if (self::$acceptNewEntries)
	  if ($baseLanguage == CONST_localeBase) $_SESSION[bText]['MT'][strip_tags($translation)] = $key;
	if (!self::getTranslation($key,$baseLanguage) && $autoLearn){
	  self::collect(array($translation=>$key), $baseLanguage);	
	}
      }
      self::autoTranslate($key);
      
      /*
       * Return the translation if it exists or
       * the original text (normally English) if there is no translation
       */
      foreach(array(self::getTranslation($key,b_cnf::get('apilang')), 
		    self::getTranslation($key,$baseLanguage),
		    self::getTranslation($key,CONST_localeBase),
		    $text) as $translation){
	if ($reply = $translation) break;
      }
      
      if ($reply == cnf_emptyString)   $reply = '&nbsp;';
      $reply = self::getIcon($key) . preg_replace('/-SKIP-'.(self::$dropAfterSKIP ? '.*' : '').'/','',$reply);
      self::dbg($reply);
    }else{
      if (is_array($text)){
	if ($v=@$text['item']) $reply =  $v;
	else                   $reply = @$text['key'];
      }else{
	$reply = $text;
      }
    }
    return $reply;
  }
  
  public static function getLangName($locale=Null){
    self::init();
    if (!$locale)  $locale = b_cnf::get('apilang',CONST_localeBase);
    if ($l = @$_SESSION[bText]['TM']['___lang'][$locale]) return $l;
    else                                                   return self::key2lang($locale);
  }

  public static function getFlag($locale=Null){
    self::init();
    if ($locale) return bIcons()->getFlag($locale);
    else         return bIcons()->getFlag(b_cnf::get('apilang'));
  }

  public static function getLangIcons(){
    self::init();
    $list = array();
    foreach(self::$localesToShow as $l){
      if ($l != b_cnf::get('apilang')) $list[] = x("form method='post' action='".b_url::same("?apilang=$l")."'",
						   bIcons()->getFlag($l));
    }
    return $list;
  }

  /*
   * Build the language translation table
   *
   * @param $translationMatrix 
   *   - associated array 'translation'=>'key'
   *
   * @param $locale
   *   Tell the language (the locale) of the translation. 
   */
  private static function collect($translationMatrix,$locale){
    if (self::$acceptNewEntries){
      $reply = 0;
      foreach($translationMatrix as $translation=>$key){
	if (!empty($key) && !is_numeric($translation) && !in_array($key,self::$doNotTranslate)){
	  $translation = b_crypt::encode_utf8(trim(b_fmt::unEscape($translation)));
	  if (  empty($translation) && ($locale==CONST_localeBase))      $translation = cnf_emptyString;
	  if (        $translation == self::getTranslation($key,$locale)) unset($translation);
	  if (!empty( $translation)){
	    self::dbg($translation);
	    $_SESSION[bText]['TM'][$key][$locale] = $translation;
	    $reply++;
	  }
	}
	self::autoTranslate($key);
      }
      if ($reply)    self::save($locale);
    }
  }

  /*
   * Initialize the current locale.
   * Some languages like Georgian, Russian, etc. have specific problems,
   * see also b_crypt::xxx_utf8
   */
  private static function load($loadLocale=Null){
    if (empty($_SESSION[bText]['TM']))  $_SESSION[bText]['TM'] = array();
    
    if (empty($loadLocale)) $list = array(CONST_localeBase, b_cnf::get('apilang'));
    else                    $list = array($loadLocale);
    
    foreach($list as $locale){
      if (empty($_SESSION[bText]['TM']['___lang'][$locale])){
	if ($handle = @fopen(self::repository($locale), 'r')){
	  $lineConter = 0;
	  while (!feof($handle)) {
	    if($line = trim(fgets($handle, 4096))){
	      list($key,$value) = explode('=',$line,2);
	      switch (empty($_SESSION[bText]['TM'][$key][CONST_localeBase])){
	      case True: $toKeep = ($locale == CONST_localeBase); break;
	      default:   $toKeep = empty($_SESSION[bText]['TM'][$key][$locale]);
	      }
	      if ($toKeep && !empty($key) && !empty($value)){
		$lineConter++;
		$_SESSION[bText]['TM'][$key][$locale] = $value;
	      }
	    }
	  }
	  fclose($handle);
	}
	
	if (empty($_SESSION[bText]['TM']['___lang'][$locale])){
	  if (!($l = googleTranslate::_(self::key2lang($locale),
					CONST_localeBase,
					$locale)))  $l = $locale;
	  $_SESSION[bText]['TM']['___lang'][$locale] = $l;
	}
      }
    }
  }

  private static function repository($locale=Null){
    if (!cnf_CLI){
      $reply = myPear_languages_path . b_cms::_(0) . '/';
      if (!is_dir($reply)) b_os::mkdir($reply);
      if (is_dir($reply)){
	if ($locale)       $reply .= $locale;
	return $reply;
      }else{
	if (!cnf_noFS) myPear::WARNING(b_debug::__()." can't create $reply");
	return Null;
      }
    }
  }
  
  private static function autoTranslate($key){
    if (empty($_SESSION[bText]['repo'])) $_SESSION[bText]['repo'] = array();
    if (!in_array($key,self::$doNotTranslate)){
      if ($defaultText = self::getTranslation($key)){
	foreach($_SESSION[bText]['repo'] as $locale){
	  if (!self::getTranslation($key,$locale)){
	    if ($defaultText!=cnf_emptyString && 
		($translation = googleTranslate::_($defaultText,
						   CONST_localeBase,
						   $locale))){
		self::collect(array($translation=>$key),$locale);
	    }
	  }
	}
      }
    }
  }
  
  public static function getForm($locales=array()){
    self::init();
    if (!is_array($locales)) $locales = array($locales);
    if (empty($locales))     $locales = array(b_cnf::get('apilang'));
    
    $header = array('key'=>'', CONST_localeBase=>CONST_localeBase.'&nbsp;'.self::getFlag(CONST_localeBase));
    foreach($locales as $l)  $header[$l] = "$l&nbsp;" . bIcons()->getFlag($l);
    $t = new b_table_zebra($header);
    $t->autoBreakLine = True; 
   
    /*
     * Check if we got a response
     */
    if (($v=$_POST['value']) && ($k=$_POST['key']) && ($l=$_POST['locale'])) self::collect(array($v=>$k),$l);
    
    /*
     * Present the dictionary for editing
     */
    ksort($_SESSION[bText]['TM']);
    foreach(array_keys($_SESSION[bText]['TM']) as $key){
      self::autoTranslate($key); 
      if ($baseTran=self::getTranslation($key)){
	$line = array('key'=>x("a name='$key'",substr($key,0,15)),
		      CONST_localeBase=>$baseTran);
	foreach($locales as $l){
	  $value  = (($v=self::getTranslation($key,$l)) ? $v : '???');
	  if($_GET['langkey']==$key && $_GET['langv']==$l){
	    $line[$l] = 
	      "<form action='".b_url::same('',array('langkey','langv'))."' method=POST>".
	      "<input type='text'   value='$value' name='value' size='".min(100,max(30,strlen($value)))."'>".
	      "<input type='hidden' value='$l'  name='locale'>".
	      "<input type='hidden' value='$key' name='key'>".
	      bIcons()->get('ok','submit').
	      "</form>";
	  }elseif(!empty($_GET['langkey']) && !empty($_GET['langv'])){
	    $line[$l] = $value;
	  }else{
	    $line[$l] = x("a href='".b_url::same("?langkey=$key&langv=$l")."#$key'",$value);
	  }
	}
	$t->prt($line);
      }
    }
    $t->close();
  }

  /*
   * Save the updated translation matrix to file,
   * the file name corresponds to the locale.
   * Locale names can be found in RFC 1766  and ISO 639, 
   * see http://php.net/manual/en/function.setlocale.php 
   */
  private static function save($locale){

    self::load($locale);

    if (is_dir(dirname($lfl = self::repository($locale)))){
      $tmp = tempnam('tmp',$locale);
      if ($handle = fopen($tmp,'w')){
	ksort($_SESSION[bText]['TM']);
	foreach(array_keys($_SESSION[bText]['TM']) as $key){
	  if ($trans = @$_SESSION[bText]['TM'][$key][$locale])  fwrite($handle, "$key=$trans\n");
	}
	fclose($handle);
	
	if (trim(`diff $tmp $lfl 2>&1`)){
	  self::dbg($locale);
	  b_os::exec((0&&self::$debug?"echo '------------------------------ $lfl';":"").
		     "sdiff -sbB   $tmp $lfl ".(0&&self::$debug?"":">/dev/null 2>&1")
		     ." || mv -f $tmp $lfl"
		     );
	}
      }
    }else{
      myPear::WARNING_T("Can't save");
    }
  }
  
  static function _key($text){
    if (!empty($text)){
      if ($key = @$_SESSION[bText]['MT'][@strip_tags($text)]) $_SESSION[bText]['keys'][$text] = $key;
      if (!isset($_SESSION[bText]['keys'][$text])){
	if ($key = preg_replace(array_keys(self::$dropInKey),array_values(self::$dropInKey),mb_strToUpper(strip_tags($text)))){
	  $_SESSION[bText]['keys'][$text] = trim(substr($key,0,33));
	}
      }
    }
    return $_SESSION[bText]['keys'][$text];
  }
  
  public static function dbg($text){
    b_debug::xxx($text,array(3,'purpleText',(bool)self::$debug));
  }

  // en-GB -> en
  static function getLang($locale=Null){
    self::init();    
    if (!$locale) $locale = b_cnf::get('apilang');
    return mb_strToLower(preg_replace('/[_-].*/','',$locale));
  }

  public static function key2lang($code){
    static $langs = array('aa' => 'Afar',
			  'ab' => 'Abkhazian',
			  'ae' => 'Ave Stan',
			  'af' => 'Afrikaans',
			  'am' => 'Amharic',
			  'an' => 'Aragonese',
			  'ar' => 'Arabic',
			  'as' => 'Assamese',
			  'ay' => 'Aymara',
			  'az' => 'Azerbaijani',
			  'ba' => 'Bashkir',
			  'be' => 'Belarusian',
			  'bg' => 'Bulgarian',
			  'bh' => 'Bihari',
			  'bi' => 'Bislama',
			  'bn' => 'Bengali',
			  'bo' => 'Tibetan',
			  'br' => 'Breton',
			  'bs' => 'Bosnian',
			  'ca' => 'Catalan',
			  'ce' => 'Chechen',
			  'ch' => 'Chamorro',
			  'co' => 'Corsican',
			  'cs' => 'Czech',
			  'cu' => 'Church Slavic; Old Bulgarian',
			  'cv' => 'Chuvash',
			  'cy' => 'Welsh',
			  'da' => 'Danish',
			  'de' => 'German',
			  'dv' => 'Divehi; Dhivehi; Maldivian',
			  'dz' => 'Dzongkha',
			  'el' => 'Greek, Modern (1453-)',
			  'en' => 'English',
			  'eo' => 'Esperanto',
			  'es' => 'Spanish; Castilian',
			  'et' => 'Estonian',
			  'eu' => 'Basque',
			  'fa' => 'Persian',
			  'fi' => 'Finnish',
			  'fj' => 'Fijian',
			  'fo' => 'Faroese',
			  'fr' => 'French',
			  'fy' => 'Western Frisian',
			  'ga' => 'Irish',
			  'gd' => 'Gaelic; Scottish Gaelic',
			  'gl' => 'Galician',
			  'gn' => 'Guarani',
			  'gu' => 'Gujarati',
			  'gv' => 'Manx',
			  'ha' => 'Hausa',
			  'he' => 'Hebrew',
			  'hi' => 'Hindi',
			  'ho' => 'Hiri Motu',
			  'hr' => 'Croatian',
			  'ht' => 'Haitian; Haitian Creole',
			  'hu' => 'Hungarian',
			  'hy' => 'Armenian',
			  'hz' => 'Herero',
			  'ia' => 'Interlingua (International Auxiliary Language Association)',
			  'id' => 'Indonesian',
			  'ie' => 'Interlingue',
			  'ii' => 'Sichuan Yi',
			  'ik' => 'Inupiaq',
			  'io' => 'Ido',
			  'is' => 'Icelandic',
			  'it' => 'Italian',
			  'iu' => 'Inuktitut',
			  'ja' => 'Japanese',
			  'jv' => 'Javanese',
			  'ka' => 'Georgian',
			  'ki' => 'Kikuyu; Gikuyu',
			  'kj' => 'Kuanyama; Kwanyama',
			  'kk' => 'Kazakh',
			  'kl' => 'Kalaallisut',
			  'km' => 'Khmer',
			  'kn' => 'Kannada',
			  'ko' => 'Korean',
			  'ks' => 'Kashmiri',
			  'ku' => 'Kurdish',
			  'kv' => 'Komi',
			  'kw' => 'Cornish',
			  'ky' => 'Kirghiz',
			  'la' => 'Latin',
			  'lb' => 'Luxembourgish; Letzeburgesch',
			  'li' => 'Limburgan; Limburger; Limburgish',
			  'ln' => 'Lingala',
			  'lo' => 'Lao',
			  'lt' => 'Lithuanian',
			  'lv' => 'Latvian',
			  'mg' => 'Malagasy',
			  'mh' => 'Marshallese',
			  'mi' => 'Maori',
			  'mk' => 'Macedonian',
			  'ml' => 'Malayalam',
			  'mn' => 'Mongolian',
			  'mo' => 'Moldavian',
			  'mr' => 'Marathi',
			  'ms' => 'Malay',
			  'mt' => 'Maltese',
			  'my' => 'Burmese',
			  'na' => 'Nauru',
			  'nb' => 'Norwegian Bokmal',
			  'nd' => 'Ndebele, North',
			  'ne' => 'Nepali',
			  'ng' => 'Ndonga',
			  'nl' => 'Dutch',
			  'nn' => 'Norwegian Nynorsk',
			  'no' => 'Norwegian',
			  'nr' => 'Ndebele, South',
			  'nv' => 'Navaho, Navajo',
			  'ny' => 'Nyanja; Chichewa; Chewa',
			  'oc' => 'Occitan (post 1500); Provencal',
			  'om' => 'Oromo',
			  'or' => 'Oriya',
			  'os' => 'Ossetian; Ossetic',
			  'pa' => 'Panjabi',
			  'pi' => 'Pali',
			  'pl' => 'Polish',
			  'ps' => 'Pushto',
			  'pt' => 'Portuguese',
			  'qu' => 'Quechua',
			  'rm' => 'Raeto-Romance',
			  'rn' => 'Rundi',
			  'ro' => 'Romanian',
			  'ru' => 'Russian',
			  'rw' => 'Kinyarwanda',
			  'sa' => 'Sanskrit',
			  'sc' => 'Sardinian',
			  'sd' => 'Sindhi',
			  'se' => 'Northern Sami',
			  'sg' => 'Sango',
			  'si' => 'Sinhala; Sinhalese',
			  'sk' => 'Slovak',
			  'sl' => 'Slovenian',
			  'sm' => 'Samoan',
			  'sn' => 'Shona',
			  'so' => 'Somali',
			  'sq' => 'Albanian',
			  'sr' => 'Serbian',
			  'ss' => 'Swati',
			  'st' => 'Sotho, Southern',
			  'su' => 'Sundanese',
			  'sv' => 'Swedish',
			  'sw' => 'Swahili',
			  'ta' => 'Tamil',
			  'te' => 'Telugu',
			  'tg' => 'Tajik',
			  'th' => 'Thai',
			  'ti' => 'Tigrinya',
			  'tk' => 'Turkmen',
			  'tl' => 'Tagalog',
			  'tn' => 'Tswana',
			  'to' => 'Tonga (Tonga Islands)',
			  'tr' => 'Turkish',
			  'ts' => 'Tsonga',
			  'tt' => 'Tatar',
			  'tw' => 'Twi',
			  'ty' => 'Tahitian',
			  'ug' => 'Uighur',
			  'uk' => 'Ukrainian',
			  'ur' => 'Urdu',
			  'uz' => 'Uzbek',
			  'vi' => 'Vietnamese',
			  'vo' => 'Volapuk',
			  'wa' => 'Walloon',
			  'wo' => 'Wolof',
			  'xh' => 'Xhosa',
			  'yi' => 'Yiddish',
			  'yo' => 'Yoruba',
			  'za' => 'Zhuang; Chuang',
			  'zh' => 'Chinese',
			  'zu' => 'Zulu',
			  );
   return @$langs[preg_replace('/[-_].*/','',mb_strToLower($code))];
  }
}

/*
 * Important:
 *   Google may update their API and change version. 
 *   If so, you must update version number in this class.
 */
class googleTranslate{
  private static $googleVersion = '1.0';
  private static $BaseUrl = 'http://ajax.googleapis.com/ajax/services/language/translate';
  private static $counter = 0;
  
  public static function _($text,$FromLang,$ToLang){
    if ($FromLang == $ToLang)                 return $text;
#   if ($_SERVER['HTTP_HOST'] == 'localhost') return $text; 

    $IL = 'invalid translation language';
    if (empty($_SESSION[$IL])) $_SESSION[$IL] = array();
    if (in_array($ToLang,$_SESSION[$IL])) return '';

    if($text){
     // Do not create bulk requests, there is a risk of being banned :-)
      if (self::$counter++ > 4) return '';
      if (@$_SESSION['autotranslate'][$text.$FromLang.$ToLang]++) return '';

      $CallUrl = self::$BaseUrl . "?v=" . self::$googleVersion . "&q=" . urlencode($text) . "&langpair=" .
	bText::getLang($FromLang) . "%7C" . bText::getLang($ToLang);
      
      $contents = '';
      if(function_exists('curl_init')){
	$ch = curl_init($CallUrl);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
	$contents=curl_exec($ch);
	$err_msg=curl_error($ch);
	curl_close($ch);
      }elseif($handle = fopen($CallUrl, "rb")){
	while (!feof($handle)) $contents .= fread($handle, 8192);
	fclose($handle);
      }

      $json = json_decode($contents, true);
      if($json['responseStatus'] == 200){
	bText::dbg($json['responseData']['translatedText']);
	return $json['responseData']['translatedText'];
      }
      
      if($json['responseStatus'] == 400) $_SESSION[$IL][] = $ToLang;
      myPear::WARNING_T($json['responseDetails']);
    }
  }
}

