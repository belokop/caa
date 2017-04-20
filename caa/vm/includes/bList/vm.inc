<?php
locateAndInclude('bList');

class bList_vm extends bList{

  // The parent event (if set)
  protected $e = Null;

  public $title = '';
  public $summary = array();

  /*
   *
   */
  public function __construct($parent_ID_p){
    locateAndInclude('bForm_vm_Event');
    
    // We might be called before the "official start"
    start_VM();
    
    // Hook the parent Event Class
    if($parent_ID_p instanceof bForm_vm_Event){
      $parent_ID = $parent_ID_p->ID;
      if (!is_object($this->e) || ($this->e->ID != $parent_ID)) $this->e = $parent_ID_p;
    }elseif(!myPear::is_equal($parent_ID_p,myOrg_ID) && is_numeric($parent_ID_p)){
      $parent_ID = $parent_ID_p;
      if (!is_object($this->e) || ($this->e->ID != $parent_ID)) $this->e = myPear::getInstance_new('bForm_vm_Event',$parent_ID,array('strict','fatal'));
    }elseif($parent_ID_p == myOrg_ID){
      $parent_ID = myOrg_ID;
      $this->e = Null;
    }else{
      b_debug::internalError("Cant't parse argument parent_ID='$parent_ID_p'");
    }
    
    // Sanity
    $this->dbg($parent_ID);
    if (($this->e !== Null) && ($this->e->ID != @VM::$e->ID)){
      $this->dbg('Strange e_ID, VM::$e->ID="'.var_export(@VM::$e->ID,True).'" $this->e->ID="'.var_export($this->e->ID,True).'"');
    }
    
    // Call the parent
    parent::__construct($parent_ID);
    
    // Set the list title
    $this->title = (($parent_ID == myOrg_ID)
                    ? myOrg_name. ' '. strToLower($this->getValue('l_name'))
		    : $this->getValue('l_name') .' for ' . x('"',b_fmt::strcut($this->e->name(),33,'...')));
    $this->dbg($this->title);
  }

  function isManager(){
    return VM_manager_here;
  }

  function isAdmin(){
    return VM_prg_coordinator_here;
  }

  /*
   *
   */
  function draw_summary() {}

  /*
   *
   */
  private $isWritable_vm = Null;
  function isWritable(){
    if ($this->isWritable_vm === Null){
      if ($this instanceof bForm_vm_Event) $e =  $this;
      else                                 $e = @$this->e;
      $this->isWritable_vm = (bool)b_cnf::check_conditions(array(False=> array('fb'      => cnf_inside_fb,
									       'archived'=> (is_object($e) ? $e->isArchived() : False),
                                                                               'too late'=> !VM_manager_here && (is_object($e) &&
                                                                                                                 ($e->getValue('e_end') < time() ||
                                                                                                                  $e->isArchived() ||
                                                                                                                  $e->isEventEndorsed()))),
                                                                 True => array('mngr'    => VM_manager_here && !cnf_inside_fb,
                                                                               'admin'   => VM_administrator_here && !cnf_inside_fb,
									       'orgnzer' => (is_object($e)  && $e->isOrganizer())),
                                                                 'default'=> parent::isWritable()),
                                                           $this,
                                                           cnf_dev);
    }
    return $this->isWritable_vm;
  }
}
