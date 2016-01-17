<?php
locateAndInclude('bHolder_vm');
class bHolder_vm_Offices extends bHolder_vm{

  public function __construct(){
    parent::__construct();
  }
  
  function query(){
    return self::queryOffices();
  }
  
  function header(){
    $header = array('o_name'    => 'name',
		    'o_status'  => 'status',
		    'o_capacity'=> 'capacity<br/>(desks)',
		    'o_phone'   => 'phone',
		    );
    foreach ($header as $k=>$v) 
      if ($k!='o_name' && $k!='o_address') 
	$this->th_attr[$k] = "style='text-align:right;'";
    return $header; 
  }

  function format() {
    if ($o = myPear::getInstance('bForm_vm_Office',$this->rec['o_id'])){
      if ($o->isWritable())  $this->t->extraTD[] = bIcons()->getButton(array('l'=>b_url::same("?form=vm_Office&id=$o->ID"),
										     'd'=>'modify',
										     'i'=>'i-edit'));
    }
  }

  public static function queryOffices(){
    if (!myPear_db()->columnExists($_GET['sBy'],'abs_offices')) $_GET['sBy'] = 'o_name';
    $q = myPear_db()->query("SELECT * FROM abs_offices ORDER BY $_GET[sBy] $_GET[sByDir]"); 
    return $q;
  }
}
