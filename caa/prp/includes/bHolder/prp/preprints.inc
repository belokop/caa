<?php
locateAndInclude('APImenu_prp.inc');
locateAndInclude('bForm_prp');
locateAndInclude('bHolder');

class bHolder_prp_preprints extends bHolder{
  
  public static $_debug = False;
  var $noSort   = True;
  var $doing    = 'list';
   var $managingWhat = '';
   var $av_id    = Null; 
   var $showLineCounter = False;
   
   public  $split_by_years = False;
   private $year_selected  = False;
   private $field_selected = False;
   
   public function __construct($doing='list'){
     parent::__construct();
     if (is_object(bAuth::$av)) $this->av_id = bAuth::$av->ID;
     else                       $doing = 'list';
     $this->doing = $doing;
     
     if ($id = @$_GET['prp_accept_once']){
       $p = new bForm_prp($id);
       $p->setAccepted();
     }
     
     if ($id = @$_GET['prp_delete_once']){
       $p = new bForm_prp($id);
       $p->setCanceled();
     }
   }

   /*
    * Select Year and/or Reseach Field
    */
   private function year_selector(){
     static $dejaVu = 0;
     if ($dejaVu++) return;

     if (!($this->year_selected  = b_cnf::get('_prp_year'))) $this->year_selected = 'all';
     if (!($this->field_selected = b_cnf::get('prp_field'))) $this->field_selected = 'all';
     if (!(int)$this->year_selected && $this->field_selected=='all') $this->year_selected = prp_year9;
     $years = array('all'=>'all years'); for($y=prp_year9; $y>=prp_year0; $y--) $years[$y] = $y;
     $f = new bForm('new','RO',False);
     $f->formDB['_prp_year'] = $this->year_selected;
     $f->formDB['prp_field'] = $this->field_selected;

     myPear::H1(PRP::$fields[$this->field_selected].(($y=$this->year_selected) ? " - ".$years[$y]:""));
     $selector = array($f->selectMenu_exec('prp_field',PRP::$fields,"onchange='submit()'",$onlyRW=True).
		       x('noscript','<input type="submit" value="go"/>'),
		       $f->selectMenu_exec('_prp_year',$years,"onchange='submit()'",$onlyRW=True).
		       x('noscript','<input type="submit" value="go"/>'));
     myPear::set_main_selector($selector,False);
   }

   /*
    * Count preprints for the authenticated user
    * @param $what can be:
    *        - 'my_pending_preprints'
    *        - 'my_all_preprints'  
    */
   public function count($what){
     $doing = $this->doing;
     $split_by_years = $this->split_by_years;
     $this->split_by_years = False;
     if (is_object(bAuth::$av)){

       $this->doing = 'managing';

       switch($what){
       case 'my_pending_preprints':
	 $this->managingWhat = tab_prp_pending;
	   break;
       case 'my_all_preprints':
	 $this->managingWhat = '';
	 break;
       }

       $reply = myPear_db()->num_rows($this->query($split_by_year=False));
     }else{
       $reply = 0;
     }
     $this->dbg($reply);
     $this->split_by_years = $split_by_years;
     $this->doing          = $doing;
     return $reply;
   }

   /*
    *
    */
   function query($split_by_year=True){

     $where[] = "prp_orgid = ".myOrg_ID;
     switch($this->doing){
     case 'list':
       $where[] = "prp_status = '".PRP_OK."'";
       break;

     case 'managing':
	 switch($this->managingWhat){
	 case tab_prp_pending:
	   $where[] = "prp_status = '".PRP_RESERVED."'";
	 default:
	   if (!PRP_censor_here)  $where[] = "prp_avid = ".bAuth::$av->ID;
	 }
       break;
     default:
       b_debug::internalError('unexpected option "'.$this->doing.'"');
     }

     // Let the adminstrator see the canceled preprints    
     if (!(is_object(bAuth::$av) && PRP_manager_here))$where[] = "prp_status != '".PRP_CANCELED."'";

     // Optionally split by years
     $q = $this->query_select($where);
     if ($split_by_year){
       // Count how many rows we have selected
       if (myPear_db()->num_rows($q) > 100){
	 $this->year_selector();
	 if (($y=$this->year_selected) && ($y!='all'))          $where[] = "YEAR(FROM_UNIXTIME(prp_day0)) = $y";
	 if (($f=$this->field_selected) && ($f!='all'))         $where[] = "prp_field = '$f'";
	 return $this->query_select(array_unique($where));
       }
     }
     return $q;
   }

   private function query_select($where=array()){
     // if (!myPear_db()->columnExists(@$_GET['sBy'],'prp_list')) $_GET['sBy'] = 'prp_day0'; 
     // if (@$_GET['sBy'] === 'prp_day0')                         $_GET['sByDir'] = 'DESC'; 
     $_GET['sBy']    = 'pn';
     $_GET['sByDir'] = 'DESC'; 
     return myPear_db()->qquery("SELECT *,YEAR(FROM_UNIXTIME(prp_day0))*1000+prp_report AS pn FROM prp_list LEFT JOIN zzz_avatars ON av_id = prp_avid ".
				" WHERE ".join(' AND ',$where).
				" ORDER BY $_GET[sBy] $_GET[sByDir]",
				cnf_dev || (bool)b_cnf::get('prpdebug') || $this->debug);
   }

   function listEntries(){

     parent::listEntries();

     if (!myPear_db()->num_rows($this->query())){
       switch($this->managingWhat){
       case tab_prp_pending:
	 $message = "There is no pending preprints";
	 break;

       default:
	 $message = EMPTY_SELECTION;
       }
       myPear::MESSAGE($message);
     }

   }

   /*
    *
    */
   private $t_classes = array('_serial' => array('prp_serial'),
			      );
   function drawHeader(){
     static $dejaVu = 0;
     parent::drawHeader();
     if (is_object($this->t) && !$dejaVu++){
       $this->t->autoBreakLine = True;
       foreach($this->header() as $k=>$v) if (empty($this->t_classes[$k])) $this->t_classes[$k] = array();
       print x('tr',x('td',''));
     }
   }

   /*
    *
    */
   function header(){
     $header = array('_serial' => ' ');
     if ($this->doing == 'managing'){
       if (PRP_censor_here)      $header['prp_avid']  = 'Entered by';
       $header['prp_field'] = 'Field';
     }
     $header['_preprint']     = 'Title and author(s)';
     $header['prp_publisher']= 'Published in';
     if ($this->field_selected != 'all') unset($header['prp_field']);
     $this->th_attr['prp_publisher'] = 'style="width:25%;"';
     return $header; 
   }

   /*
    *
    */
   private function href($title){
     if (PRP::valid_extension($l=trim($this->rec['prp_archive']))){
       $reply = (file_exists($l)
		 ? x("a href='$l'",$title)
		 : $title);
     }elseif ($url=$this->rec['prp_archive']){
       $reply = ($this->rec["prp_status"] == PRP_CANCELED
		 ? $title
		 : bJS()->modal_iframe($url,$title,'',True));
     }elseif($this->rec['prp_local']){
       $p = new bForm_prp($this->rec['prp_id']);
       $l = $p->url_local();
       $reply = (file_exists($l)
		 ? x("a href='$l'",$title)
		 : $title);
     }else{
       $reply = $title;
     }
     return $reply;
   }

   /*
    *
    */
   function rejectRecord(){ 
     $reply = False;
     return $reply;
   }
   
   /*
    *
    */
   function format() {
     print x('tr',x('td')); // spacer... better to be done by CSS
     foreach($this->t_classes as $k=>$v) $this->t->class[$k] = $v;

     $this->rec['prp_avid'] = bForm_Avatar::_fmtName('fl',$this->rec);

     // Title & authors
     $preprint[] = $this->href(x('span class="prp_title"',b_fmt::escape($this->rec['prp_title'],'<')));
     if (($a = b_fmt::escape($this->rec['prp_authors'],'<'))) $preprint[] = x('span class="prp_authors"',$a);
     $this->rec['_preprint'] = join('<br>',$preprint);

     // Publisher
     if (($this->rec["prp_status"] !== PRP_CANCELED) && ($doi=$this->rec['prp_doi'])){
       $this->rec['prp_publisher'] = bJS()->modal_iframe("http://dx.doi.org/doi:$doi",$this->rec['prp_publisher']);
     }

     //    
     // if ($this->rec["prp_status"] == PRP_CANCELED) foreach(array('_preprint','prp_publisher') as $f) $this->rec[$f] = strip_tags($this->rec[$f]);

     $this->rec['_serial'] = bForm_prp::_serial($this->rec,False);
     //     $this->rec['prp_publisher'] = str_replace(',','&#44;',$this->rec['prp_publisher']);
     //     $this->rec['prp_publisher'] = b_fmt::escape($this->rec['prp_publisher']);
     
     if ($this->doing == 'managing'){
       $p = new bForm_prp($this->rec['prp_id']);
       if ($p->isWritable() && $p->getValue('prp_status') != PRP_CANCELED){
	 $this->t->extraTD[]=b_btn::submit_icon('ICON_edit','modify',b_url::same("?form=bForm_prp&id=$p->ID&mode=RO"),False);
       }
     }
     
     switch ($this->rec["prp_status"]) {
     case PRP_RESERVED:
       foreach($this->header() as $k=>$v) $this->t->class[$k][] = 'prp_reserved';
       $this->rec['prp_avid'] .= '<br/>'.date(' Y-m-d',(empty($this->rec['prp_tm'])
							? $this->rec['prp_day0']
							: $this->rec['prp_tm']));
       if (is_object($p) && $p->isWritable()){
	 if (PRP_censor_here) $this->t->extraTD[]=x("a href='".b_url::same("?prp_accept_once=$p->ID").
						   "' onClick='return confirm(\"Really accept?\");'",
						    bIcons()->get(array('d'=>'accept',
									'i'=>'ok')));
	 $this->t->extraTD[]=x("a href='".b_url::same("?prp_delete_once=$p->ID")."' onClick='return confirm(\"Really delete?\");'",
			       bIcons()->get(array('d'=>'delete',
						   'i'=>'drop')));
       }
       break;
       
     case PRP_CANCELED:
       foreach($this->header() as $k=>$v) $this->t->class[$k][] = 'prp_canceled';
       $this->t->extraTD[] = bIcons()->get(array('d'=>'canceled','i'=>'i-drop2'));
       break;
       
     case PRP_OK:
       $this->t->class = $this->t_classes;
     }
   }
   
   function getAllEvents(){
     $q = $this->query();
     $list = array();
     while ($r=myPear_db()->next_record($q)) $list[]=$r['prp_id'];
     return $list;
   }
}
