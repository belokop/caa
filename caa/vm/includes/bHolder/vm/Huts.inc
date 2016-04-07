<?php
locateAndInclude("bHolder_vm");
class bHolder_vm_Huts extends bHolder_vm{

  var $debug = False;
  var $repetValuesCol1only = True;

  public function __construct($title='All huts'){
    $this->dbg();
    parent::__construct();
    $this->title = bText::_($title);
  }
  
  function query(){
    $_GET['sBy'] = 'hut_code';
    return myPear_db()->qquery("SELECT * FROM abs_huts ".
				 " WHERE hut_code != '".HUT_BI."'".
				 " ORDER BY hut_code,hut_name ASC ",$this->debug); 
  }
    
  function header(){
    $header = array('hut_code'    => 'Accommodation<br/>option',
		    'hut_name'    => 'Name',
		    'hut_status'  => 'Default?',
		    'hut_capacity'=> 'N rooms',
		    'hut_address' => 'Address',
		    // 'hut_entrance_code' => 'entrance<br/>code',
		    		    );
    return $header; 
  }

  function isWritable(){
    return VM_rooms_updater_here || VM_administrator_here;
  }

  /*
   * Add buttons "new Building"
   */
  function listEntries_summary(){
    bIcons()->reset();
    $edit_buttons = array();
    if ($this->isWritable()){
    $edit_buttons[] = b_btn::submit('add new '.
				    join(' / ',array_values(VM_hutCodes()->getCodes(Null,array(time(),time()+720*24*3600)))),
				    b_url::same('?id=new&form=vm_Hut&clean_once=act_code,act_id'));
    }
    $this->t->tr(b_btn::UL_buttons($edit_buttons),'colspan=6');
  }

  /*
   *
   */  
  public static function address($rec, $icludeCountry=False){
    $a = array();
    if ($x=trim(@$rec['hut_street']))                         $a[] = $x;
    if ($x=trim(@$rec['hut_postcode'].' '.@$rec['hut_city'])) $a[] = $x;
    if(($x=$rec['hut_country']) && $icludeCountry)            $a[] = $x;
    return join('<br/>',$a);
  }

  protected function isDeletable(){
    return True;
  }

  /*
   *
   */
  function format() {
    if (empty($this->rec['hut_name']))  $this->rec['hut_name'] = $this->rec['hut_street'];

    $this->rec['hut_address'] = self::address($this->rec);
    $this->rec['hut_code']    = x('em',VM_hutCodes()->getName($this->rec['hut_code']));

    if (in_array($this->rec['hut_status'],b_checkBox::$ON)) $this->rec['hut_status'] = bIcons()->get('ok');

    $hut = myPear::getInstance_new('bForm_vm_Hut',$this->rec['hut_id'],'fatal');
    if ($W=$hut->isWritable())$this->t->extraTD[]=bIcons()->getButton(array('l'=>b_url::same("?form=vm_Hut&id=".$hut->ID),
										    'd'=>'modify',
										    'i'=>'i-edit'));
    else                    $this->t->extraTD[]='';
    if (0 &&   $W)          $this->t->extraTD[]=bIcons()->getButton(array('l'=>b_url::same("?form=vm_Hut&id=".$hut->ID),
										  'd'=>btn_deleteAp,
										  'i'=>'drop'));
  }
  
  function getAllEvents(){
    $q = $this->query();
    $list = array();
    while ($r=myPear_db()->next_record($q)) $list[]=$r['hut_id'];
    return $list;
  }                                                                                                                                                                                               
}
