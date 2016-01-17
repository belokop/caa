<?php
/**
 * Templates editor & render
 */

define('tpl_edit',    'edit');
define('tpl_add',     'add new template');
define('tpl_drop',    'delete this template');
define('tpl_preview', 'preview non-rendered template');
define('tpl_previewr','preview rendered template (random example)');
define('tpl_cancel',  'cancel');
define('tpl_save',    'save');

class bTpl{

  /*
   * The owner of the templates
   */
  public static $owner  = '';

  /*
   * Rendering function
   */  
  public $tpl_render = '';

  /*
   * The default title & body
   */
  public $tpl_title_default = '';
  public $tpl_body_default  = '';

  public  $id     = 'new';
  private $record = array();

  protected $cols = 40;
  protected $rows = 20;
  protected $mode = tpl_preview;
  public    $debug = False;

  private   $token     = 'green';
  private   $token_bad = 'red';

  public static function getTplID($tpl_title){
    $q = myPear_db()->query("SELECT * FROM zzz_templates WHERE tpl_title = '$tpl_title'");
    while ($r = myPear_db()->next_record($q)) $reply = $r['tpl_id'];
    return @$reply;
  }

  function __construct($arg = array()){
    if (!is_null($d = cnf_show_t)) $this->debug = $d;

    if ($key = array_values(preg_grep('/^tpl_id:/',array_keys($_POST)))){
      $this->id   = str_replace('tpl_id:','',$key[0]);
      $this->mode = $_POST[$key[0]];
    }

    if ($tpl_title = @$arg['title'])  $this->tpl_title_default = $tpl_title;
    if ($tpl_body  = @$arg['body'])   $this->tpl_body_default  = $tpl_body;
    if ($tpl_render= @$arg['render']) $this->tpl_render        = $tpl_render;

    // Initialise the template
    if (!$this->id && $tpl_title) $this->id = self::getTplID($tpl_title);
    if (!empty($this->tpl_title_default) && !empty($this->tpl_body_default)) $this->get();

    $this->same = b_url::same("?id=$this->id");
  }

  /*
   * Present the template selector and execute the selected template
   */
  function editor(){

    $_POST['_post_bForm_ID_once'] = 'new';
    if ((int)$this->id)    $_POST['_selected_template'] = $this->id;
    else                   $this->id = @$_POST['_selected_template'];

    if (!(int)$this->id) myPear::H1('Select the template');
    //else    print x('h4','Select another template');

    $templates = array();
    $q  =  myPear_db()->query("SELECT * FROM zzz_templates WHERE tpl_owner = '".self::$owner."' ORDER BY tpl_title");
    while ($r=myPear_db()->next_record($q)) $templates[$r['tpl_id']] = $r['tpl_title'];
    
    $formDummy  = new bForm('new','RO',False);
    print
      "<br/><form action='".b_url::same()."&mode=$this->mode' method='post'>".
      $formDummy->selectMenu_exec('_selected_template',$templates,"onchange='submit()'",$onlyRW=True);
    
    if ((int)$this->id)      print $this->exec();

  }

  /*
   * Get the template from the database.
   * Create a default record if the db is empty
   * The search in the db is done either by ID or by the title.
   */
  function get(){
    if (empty($this->record)){
      $trigger = ((int)$this->id 
		  ? "tpl_id = '$this->id'"
		  : "tpl_title = '$this->tpl_title_default'");
      $q  =  myPear_db()->query("SELECT * FROM zzz_templates WHERE $trigger AND tpl_owner = '".self::$owner."'");
      while ($r=myPear_db()->next_record($q)){
	$this->id = $r['tpl_id'];
	$this->record = $r;
      }
    }
    if (!empty($this->record))     return $this->record['tpl_body'];

    // There is no such template yet...
    if ((int)$this->id)	b_debug::internalError('??? template not found in the db');

    // Create it then and try again...  
    $this->id = bMaster()->reserve_masterCounter();
    myPear_db()->query("INSERT INTO zzz_templates SET ".
			 join(',',array( "tpl_id=$this->id",
					 myPear_db()->quote(self::$owner,            True,'tpl_owner'),
					 myPear_db()->quote($this->tpl_render,       True,'tpl_render'),
					 myPear_db()->quote($this->tpl_title_default,True,'tpl_title'),
					 myPear_db()->quote($this->tpl_body_default, True,'tpl_body')))); 
    $this->dbg("creating tpl $this->tpl_title_default");
   return $this->get();
  }

  /*
   * Execute the editor request
   */
  function exec(){
    $this->dbg($this->mode);

    $this->get();
    
    if ($t = @$this->record['tpl_title']) myPear::H1($t);

    switch($this->mode){
      
    case 'edit':         return $this->edit();
      
    case 'drop':         return $this->drop();
      
    case 'save':         return $this->save();

    case 'cancel':       return $this->cancel();

    case 'previewr': 
      $this->get_randomExample();
    default:
    case 'preview':      return $this->preview();
    }
  }

  // NYI
  private function get_randomExample(){
    if (function_exists('bTpl_hook')) return bTpl_hook_example();
  }

  /*
   * drop the object
   */
  function drop(){
    $this->dbg("$this->mode $this->id $this->tpl_title_default");
    list($tpl_title,$tpl_body,$tpl_render) = $this->_getRecord();
    switch(b_btn::confirm()){

    case 'no':
      return $this->cancel();

    case 'yes':
      return "yes, delete $this->id";
      
    case Null:
    default:      // ask for confirmation

      $msg  = "Are you sure you want to delete the template<br/><h3>$tpl_title</h3>?";
      $args = array($this->id=>tpl_drop);

      $confirm = b_btn::confirm(#tpl_drop,
				$msg,False,$args);
      return bJS()->modal_inline($text, $confirm, $showTouteSuite=True, $showCloseButton=False);
      return $confirm;
    }
  }
  
  /*
   * Preview the template
   */
  function preview(){
    $this->dbg($this->tpl_title_default);
    ob_start();

    myPear::H1($this->id);

    list($tpl_title,$tpl_body,$tpl_render)  = $this->_getRecord();

    $tpl_body = $this->preview_rendered($tpl_body,$tpl_render);

    $this->t = new b_table('class="b_wrap c_table"',
			   "<form action='$this->same' method='post'>");

    //$this->t->tr(array('Template',$tpl_title), array());
    //if (function_exists($r=$tpl_render))  $this->t->tr(array('Render',$r),array());
    $this->t->tr(array('',x('pre',$tpl_body)));
    $this->_submit_field();
    $this->t->close('</form>');

    $output = ob_get_contents();
    ob_end_clean();
    return $output;
  }

  private function includeTpl(&$tpl_body,$highlight=array()){
    while (preg_match(";<include\s(\S+)/>;",$tpl_body,$m)){
      $included_tpl_title = $m[1]; 
      
      $included_tpl     = new bTpl();
      $included_tpl->id = self::getTplID($included_tpl_title);  
      $included_tpl_body= $included_tpl->render();
      $tpl_body = str_replace("<include $included_tpl_title/>",$included_tpl_body,$tpl_body);
      $this->dbg($included_tpl_title);
    }
  }

  /*
   * Return the rendered template
   *
   * @param $args - array to be transmitted to the render of this template
   */
  function render($args=array()){
    
    list($tpl_title,$tpl_body,$tpl_render)  = $this->_getRecord(True);
    
    if (function_exists($tpl_render)){
      foreach($tpl_render('SHOW_TRANSLATION',$args) as $from=>$to){
	if (!empty($to)) $tpl_body = str_ireplace("<$from/>",$to,$tpl_body);
      }
      $this->includeTpl($tpl_body);
    }
    //$this->dbg($tpl_body);
    return $tpl_body;
  }

  /*
   * More complicated version of the render, used by the editor
   */
  private function preview_rendered($text,$tpl_render){
    static $highlight_ok  = '__HL_OK_O__';
    static $highlight_bad = '__HL_BAD_O__';
    static $highlight_    = '__HL_C__';

    if (function_exists($tpl_render)){
      if ($this->mode === tpl_previewr)  $args = bTpl_hook_example();
      else                               $args = array();
      
      // Highlight "good" translation
      foreach($tpl_render('SHOW_TRANSLATION',$args,True) as $from=>$to){
	if (empty($to)) {
	  $text = str_replace('<',"$highlight_ok &lt;",$text);
	  $text = str_replace('>',"&gt; $highlight_",$text);
	}else{
	  $text = str_replace("<$from/>","$highlight_ok$to$highlight_",$text);
	  $text = str_replace("<$from>", "$highlight_ok$to$highlight_",$text);
	}
      }
    }
    
    $this->includeTpl($text);

    // Highlight "bad" (i.e. undefined) translation
    if ($this->mode === tpl_previewr){
      $text = preg_replace(';(&lt.|<)([\s\:\w\/]*)(&gt.|>);',"$highlight_bad &lt;\\2&gt; $highlight_",$text);
    }

    if ($nbad = substr_count($text,$highlight_bad)) myPear::WARNING("The template has $nbad broken token".($nbad>1 ? 's' : ''));

    $text = str_replace($highlight_bad,"<span class='".$this->token_bad."Text'>",$text);
    $text = str_replace($highlight_ok, "<span class='".$this->token.    "Text'>",$text);
    $text = str_replace($highlight_,   '</span>',$text);

    // show URL's
    $text = preg_replace(';(http:/[\~\w\/\.\?\=\&]*[\w\/]);','<a href="\\1">\\1</a>',$text);
    
    return $text;
  }

  /*
   * Save the record from the SESSION in the database
   */
  function save(){
    $this->dbg("$this->mode $this->id $this->tpl_title_default");
    
    list($tpl_title,    $tpl_body,    $tpl_render)     = $this->_getRecord();
    list($tpl_title_old,$tpl_body_old,$tpl_render_old) = $this->_getRecord(True);
    
    if ($tpl_body===$tpl_body_old && $tpl_title===$tpl_title_old){
      myPear::MESSAGE("There is no changes for $table $tpl_title, the record is not updated ");
    }else{
      if (!(int)$this->id)	b_debug::internalError("missing record");
      $q[] = myPear_db()->quote($tpl_title,True,'tpl_title');
      $q[] = myPear_db()->quote($tpl_body, True,'tpl_body');
      myPear_db()->query("UPDATE zzz_templates SET ".join(',',$q)." WHERE tpl_id = $this->id");
    }
  
    // clean everyng and show the result
    $this->cancel();
  }
  
  function cancel(){
    $this->dbg("$this->mode $this->id $this->tpl_title_default");
    $this->id = 0;
    $_POST = array();
    $_SESSION['bTpl']  = array();
  }
  
  function edit(){
    $this->dbg("$this->mode $this->id $this->tpl_title_default");
    ob_start();
    myPear::H1($this->id);

    list($tpl_title,$tpl_body,$tpl_render) = $this->_getRecord();
    if (function_exists($tpl_render)) $description = $tpl_render('SHOW_DESCRIPTION');
    else                              $description = array();

    $this->t = new b_table('class="b_wrap c_table"',
			   "<form action='$this->same' method='post'>");
    $this->t->tr(array(empty($tpl_title)?'Title':'',
		       (empty($tpl_title)
			? $this->_input_field('tpl_title',$tpl_title)
			: $tpl_title)),
		 array());
    $this->t->tr(array('Text',
		       $this->_input_field('tpl_body',$tpl_body,'area'),
		       x('h4','Shortcuts:').
		       x('ul',b_fmt::joinMap('li',$description,'strong'))));
    $this->_submit_field();
    $this->t->close('</form>');
    $output = ob_get_contents();
    ob_end_clean();
    return $output;
  }
  
  public function isWritable(){
    return True;
  }

  /*
   * Read the record from disk and save it in the session array for the future editing.
   * The edited record (together with the dependacies) is kept in memory until 
   * the 'save' or 'cancel' button is pressed. 
   */
  private function _getRecord($db_record = False){
    $this->get();
    switch ($db_record){
  
      // real database record    
    case True:
      $reply = array($this->record['tpl_title'],
		     $this->record['tpl_body'],
		     $this->record['tpl_render']);
      break;
      
      // virtual record 
    case False:
      if (empty($_SESSION['bTpl'][$this->id])){
	$_POST = array();
	$_SESSION['bTpl'] = array(); // be nice with the memory...
	$_SESSION['bTpl'][$this->id] = array($this->record['tpl_title'],
					      $this->record['tpl_body'],
					      $this->record['tpl_render']);
	
      }else{	// apply the changes done by eventual editing.

	foreach(array('tpl_title','tpl_body') as $k=>$v){
	  if ($new = trim(@$_POST[$v])) $_SESSION['bTpl'][$this->id][$k] = $new;
	}
      }
      $reply = $_SESSION['bTpl'][$this->id];
    }
    return $reply;
  }

  function sb($button,$id=Null){ 
    if (!$id) $id = $this->id;
    print "<input type='submit' value='$button' name='tpl_id:$id'>&nbsp;&nbsp;"; 
  }
  function dbg($text=''){
    b_debug::xxx($text,array(3,'blueText',$this, (bool)$this->debug || cnf_show_t));    
  }
  
  private function _input_field($name,$value,$type='text'){
    $this->dbg();
    if ($type == 'text'){
      $this->cols = max($this->cols, strlen($value));
      return "<input size='$this->cols' 'type='text' name='$name' value='$value'/>";
    }else{
      $rows = (int)$this->rows;
      $cols = 99;
      return x("textarea name='$name' rows='$rows' cols='$cols'",$value);
    }
  }


  private function _submit_field(){ 
    $this->t->tro();
    $this->t->tdo('class="align_left" colspan=3');

    switch($this->mode){
    case tpl_edit:
      $this->sb(tpl_preview);
      $this->sb(tpl_previewr);
      break;
      
    case tpl_preview:
    case tpl_previewr:
      if ($this->mode === tpl_previewr)      $this->sb(tpl_preview);
      if ($this->mode === tpl_preview)       $this->sb(tpl_previewr);
      $this->sb(tpl_edit);
      $this->sb(tpl_save);
      $this->sb(tpl_cancel);
      if (0 && (int)$this->id){
	$this->sb(tpl_drop);
	$this->sb(tpl_add,'new');
      }
      break;
      
    default:
      $this->sb("?".$this->mode);
    }
    $this->t->tdc();
    $this->t->trc();
 }
}
