<?php
locateAndInclude("bHolder");
class bHolder_Organizations extends bHolder{

  var $sortBy_default = 'org_name';
  var $org = Null; // current Organization
  var $showLineCounter = False;

  public function __construct(){
    parent::__construct();
    $this->title = 'All Organizations';
    $_GET['sBy'] = 'org_id';
  }

  function header(){
   return array('org_id'        => ' ',
		'org_code'      => 'code',
		'org_name'      => 'Full name',
		'org_name_short'=> 'Short name',
		'org_domain'    => 'DNS domain',
		'org_theme'     => 'Theme',
		);
  }
  
  function rejectRecord(){ 
    $this->org = myOrg($this->rec['org_id']);
    return !$this->org->isReadable();
  }

  function format(){

    $this->t->class['org_id'] = 'bg_transp';

    $this->rec['org_id'] = $this->org->getFavicon();
    $this->t->extraTD[] = ($this->org->isWritable()
			   ? bIcons()->getButton(array('i'=>'i-edit',
							       // 'd'=>'modify',
							       'l'=>b_url::same("?form=Organization&id=".$this->org->ID)))
			   : '');
    if ($this->org->ID != myOrg_ID) $this->t->extraTD[] = b_btn::link('set as current',b_url::same('?org='.$this->org->ID).'&flavor='.$this->org->getValue('org_theme'));
  }
  
  function query(){
    $q  = myPear_db()->query("SELECT * FROM zzz_organizations ORDER BY org_code");
    return $q;
  }

  protected function _close(){
    if (superUser_here){
      print b_btn::submit('create new institution/organization',b_url::same("?form=Organization&mode=RW&id=new"));
    }
  }
}
