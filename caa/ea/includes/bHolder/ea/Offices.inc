<?php
/*
 * Show offices used by staff members
 */
locateAndInclude('bHolder_ea');

class bHolder_ea_Offices extends bHolder_ea{
  
  private $doing = Null;
  protected $offices = array();

  public function __construct($doing,$h1=''){

    // Init
    if (empty($h1)) $h1 = myOrg_nameS.' staff member offices';
    myPear::h1($h1);
    
    switch($this->doing = $doing){
    case 'summary':
    case 'doorLabels':
      break;
    default:
      b_debug::internalError("??? doing $doing");
    }
    
    // Delete the office if requested
    if ($o_id = @$_GET['delete_once']){
      loader::getInstance_new('bForm_ea_Office',$o_id,'delete');
    }

    // Ask parents...
    parent::__construct();
    
    $this->init();    
  }

  /*
   * Create list of all offices used by the staff members
   */  
  protected function init(){
    foreach(EA_members()->getMembers_byStatus() as $av_id=>$rec){
      $av = loader::getInstance_new('bForm_Avatar_'.b_reg::$current_module,$av_id,'fatal');
      $o_name = $av->get_staffOffice();
      if (empty($o_name))	$o_name = ' No office';
      else       loader::getInstance_new('bForm_'.b_reg::$current_module.'_Office',array('o_name'=>$o_name));
      $this->offices[$o_name][] = $av;
    }
    ksort($this->offices);
  }
  
  /*
   *
   */
  function query($where='1'){
    return self::queryOffices($where);
  }

  public static function queryOffices($where=1){
    return myPear_db()->query("SELECT o_id,o_name,o_status,o_capacity FROM abs_offices WHERE $where ORDER BY o_name ASC");
  }

  function header(){

    switch($this->doing){
    case 'summary':
      $header = array('hus'       => ' ',
		      'o_name'    => ' ',
		      '_o_status' => ' ', // 'assignment',
		      'o_capacity'=> 'capacity',
		      // 'o_phone'   => 'phone',
		      //'_n_sits'   => 'usage',
		      '_tenants'  => ' ',
		      );
      if (!EA_member_here()) unset($header['_o_status'],$header['o_capacity']);

      foreach ($header as $k=>$v) $this->th_attr[$k] = "style='text-align:right;'";
      foreach ($header as $k=>$v) $this->td_attr[$k] = "style='text-align:right;'";
      break;
    default:
      $header = array();
    }
    return $header; 
  }
  
  protected $building_was = '?';
  protected function format_summary($v_eid=null) {
    //
    // Summary list of office space usage
    $this->t->class['hus'] = 'b_transparent';
    $this->t->class['o_status'] = 'align_right';
    $this->t->showLineCounter = False;
    
    $this->rec['_o_status'] = ($this->rec['o_status'] == OFFICE_AUTO_ALLOCATABLE
			       ? x('em style="font-size:smaller"',x('strong','Office for '.OFFICE_AUTO_ALLOCATABLE))
			       : x('em style="font-size:smaller"','Office for '.$this->rec['o_status']));

    $o = loader::getInstance_new('bForm_'.b_reg::$current_module.'_Office',$this->rec['o_id'],'fatal');
    $name = explode(':',$o_name = $o->getValue('o_name'),2);
    $building = substr($name[0],0,strlen($name[0])-1);
    if ($building != $this->building_was){
      if ($this->building_was != '?') $this->t->tr(array('&nbsp'));	
      $this->rec['hus'] = x('h4',"Building $building");
    }
    $this->building_was = $building;

    // Edit button
    if ($o->isWritable())  $this->t->extraTD[] = bIcons()->getButton(array('l'=>b_url::same("?form=vm_Office&id=$o->ID"),
									   'd'=>'modify',
									   'i'=>'i-edit'));
    return (array($o,$o_name));
  }

  function format() {
    switch($this->doing){
    case 'summary':
      if ($this->isWritable() || !empty($this->offices[$this->rec['o_name']])){

	list($o,$o_name) = $this->format_summary();
	
	if(empty($this->offices[$o_name])) $this->offices[$o_name] = array(0);
	foreach($this->offices[$o_name] as $av){
	  $this->rec['_tenants'] = (empty($av)
				    ? ' '
				    : x(sprintf('a href="%s"',b_url::same('?mode_once=RO&id='.$av->ID.'&form=Avatar_ea')),$av->fmtName('fl')));
	  $this->t->prt($this->rec);
	  $this->rec = array();
	}
      }
      $this->rec = array();
      break;
      
    case 'doorLabels':
      $rec = $this->rec;
      if (empty($this->offices[$this->rec['o_name']])){
	MSG::MESSAGE("No tenants for $rec[o_name]");
	$this->offices[$this->rec['o_name']] = array();
      }
      foreach($this->offices[$this->rec['o_name']] as $av){
	$this->rec = array_merge($rec,array('av_id'=>$av->ID,
					    'av_position'=>$av->getValue('av_position',1)));
	$this->doorLabels('collect',$this->rec['o_id'],array(),array('av_position'));
      }
      $this->rec = array();
      if (is_object($this->t)){
	$this->t->close();
	unset($this->t);
      }
    }
  }
  
  
  /*
   *
   */
  function listEntries_summary(){
    switch($this->doing){
    case 'summary':
      $o_name = ' No office';
      if (!EA_member_here()) unset($this->offices[$o_name]);
      if (empty($this->offices[$o_name])) $this->offices[$o_name] = array();
      $this->t->tr(array('&nbsp'));	
      foreach($this->offices[$o_name] as $av){
	$user = (empty($av)
		 ? ' '
		 : x(sprintf('a href="%s"',b_url::same('?mode_once=RO&id='.$av->ID.'&form=Avatar_ea')),$av->fmtName('fl')));
	$this->t->prt(array('hus'    =>x('h4',$o_name),
			    '_tenants'=> $user));
	$o_name = '';
      }
      
      bIcons()->reset();
      $edit_buttons = array();
      if ($this->isWritable()){
	$edit_buttons[] = b_btn::submit('add new office',
					b_url::same('?id=new&form=vm_Office&clean_once=act_code,act_id'));
      }
      $this->t->tr(b_btn::UL_buttons($edit_buttons),'colspan=6');
      break;
      
    case 'doorLabels':
      $this->doorLabels('print');
    }
  }
}
  
