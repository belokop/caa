<?php
locateAndInclude('b_checkBox');
locateAndInclude('bIcal_matrix');
class bIcal_matrix_offices extends bIcal_matrix{
  
  var $see_day9 = True;
  var $seeAgenda     = True;
  var $imposedEntries = array();
  var $o_id           = array();

  function __construct(){
    parent::__construct();
    if ($id=(int)b_cnf::getValue('lease_id')) { // optionally show only 1 office
      $v = myPear::getInstance('bForm_vm_Visit',$id);
      $this->o_id = array($v->getValue('v_oid',1)); 
    }
    $_GET['sBy'] = 'o_name';
  }
  
  function show($arg=Null){    
    myPear::H1("Office space allocation");
    $joinOffices = " LEFT JOIN abs_offices ON o_id = v_oid  LEFT JOIN abs_expenses ON v_id = exp_vid LEFT JOIN zzz_list_members ON v_projectid = lm_id ";

    $this->mUsers = $mPrograms = $onlyThoseOffices = array();
    if ($this->day1 < b_time::txt2unix('2000-01-01'))  $this->day1 = time()      - 15*24*3600;
    if ($this->day9 < b_time::txt2unix('2000-01-01'))  $this->day9 = $this->day1 + 33*24*3600;
    $this->day9 = min($this->day9,$this->day1 + $this->nDaysToShow*24*3600);

    if ($this->seeAgenda){  
      $q = myPear_db()->query("SELECT *,e_start AS name  FROM abs_events WHERE e_code > 0 ORDER BY e_start ASC");
      while ($r = myPear_db()->next_record($q))
	if (b_time::inInterval($r['e_start'],$r['e_end'],$this->day1,$this->day9))
	  $mPrograms[0][] = array('day1' => b_time::_($r['e_start'],08),
				  'day9' => b_time::_($r['e_end']  ,20),
				  'text' => $r['e_name'],
				  'tips' => x('strong',myOrg_name.' program').'<br/>'.nbsp($r['e_name']),
				  'clas' => $this->getRandomColor(abs_colorLegend($r)),
				  'name' => '  NORDITA EVENTS',
				  'styl' => 'strong',
				  'url'  => 'em');
    }
    
    foreach ($this->o_id as $o) if ($o) $onlyThoseOffices[] = "o_id=$o";
    if (!empty($this->imposedEntries) || !empty($onlyThoseOffices) || is_object($this->tClass)){
      $this->noNavig = True;
      $this->allowCollapse = False;
    }
    if ($this->noNavig) $this->hideCheckBox = True; 
    
    $where = array("v_end  >= $this->day1",
		   "v_start<= $this->day9",
		   "v_type != '".VISIT_TYPE_RENT."'");
    if ($this->hideCheckBox){
      b_checkBox::set('seeClochards','off');
    }else{
      // check do we have "officeless people"
      // and if so, show the selection checkbox 
      $q = VM_query(array_merge($where,array('(v_oid = 0 OR v_oid IS NULL)')),$joinOffices);
      if ($n = myPear_db()->num_rows($q)) $count["officeless"] = $n;
      else   b_checkBox::set('seeClochards','off');
    }
    
    /*
     * fill the "standard" entries, pay attention to the checkbox selection 
     */
    $_GET['sBy']    = 'o_name';
    $_GET['sByDir'] = 'ASC';
    if (!b_checkBox::isOn('seeClochards')) $where[] = 'v_oid > 0';
    if (!empty($onlyThoseOffices))        $where[] = x('(',join (' OR ',$onlyThoseOffices));
    $q = VM_query($where,$joinOffices);
    $haveTenants = myPear_db()->num_rows($q);
    while($r = myPear_db()->next_record($q)) $this->fillEntry($r);

    /*
     * fill the optional "external" entries (passed as the argument) 
     */
    foreach ($this->imposedEntries as $entry) $this->fillEntry($entry);

    /*
     * show all matrices on the table
     */
    if (is_object($this->tClass)){
      $this->t =  $this->tClass;
      $this->t->titleText = "Office space ".
	b_time::strftime('%e %h %Y',$this->day1).' <-> '.
	b_time::strftime('%e %h %Y',$this->day9);
    }else {
      $this->t = new b_table("class='bIcal'");
    }
    
    if ($haveTenants && !$this->hideCheckBox)
      if (empty($onlyThoseOffices) && !is_object($this->tClass)){
	$boxes = array();
	if ($count["officeless"] > 0){
	  $boxes['seeClochards'] = "See ".$count["officeless"]." visitors WITHOUT office";
	  $this->footer[] = b_checkBox::create($boxes,'<br/>');
	}
      }

    $this->showHeader();
    $this->showBody($mPrograms,True);
    $this->showBody($this->mUsers);
    $this->showFooter();

  }
  
  function fillEntry($r){
    $r['name'] = bForm_Avatar::_fmtName('Lf',$r);
    $url = b_url::same("?".b_crypt_no."=1&mode=RO&form=vm_Visit&id=$r[v_id]");

    if ($o_name=$r['o_name']) $o_name = $o_name.($r['o_capacity']>0 && $r['o_capacity']<99 ? "   [$r[o_capacity]]" : '');
    else                      $o_name = 'Officeless';
    $entry=array('day1' => $r['v_start'],
		 'day9' => $r['v_end'],
		 'text' => "<a href='$url'>$r[name]</a>",
		 'clas' => $this->getRandomColor(abs_colorLegend($r)),
		 'name' => nbsp($o_name),
		 'tips' => nbsp(preg_replace('/<br .><br .>/','<br/>',
					     join('<br/>',array(x('strong',$r['name']),
							       $r['e_code']<0 ? $r['e_name'] : '',
							       x('i',$r['av_institute']),
							       x('i',b_time::strftime('%e-%h',$r['v_start']).' <-> '.b_time::strftime('%e-%h',$r['v_end'])))))));
    $this->mUsers[$r['o_id']][] = $entry;
  }
}
