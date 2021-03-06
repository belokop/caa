<?php
define('seeVisitors', 'see_visitors');

locateAndInclude('bIcal_matrix');
locateAndInclude('vm_functions.inc');

class bIcal_matrix_visits extends bIcal_matrix{
  
  var $see_day9 = True;
  var $tClass   = NULL;
  public  $imposedEntries = array();
  public  $graphTitle = '';

  public function __construct(){
    locateAndInclude('b_checkBox');
    parent::__construct();
    if (is_object(VM::$e)) $this->graphTitle = 'Who arrives when?';
  }
  
  function show($arg=0){    
    MSG::H1('empty');
    if (!empty($this->graphTitle)) print x('h3',$this->graphTitle);
    
    bTiming()->cpu(__function__);

    if (empty($this->day9)) $this->day9 = $this->day1 + 40*86400;
    $this->nDaysToShow = (int)(($this->day9 - $this->day1)/86400);
    $boxes = $mRegistrants = $mPrograms = array();
    
    $v_where = array("v_type != '".VISIT_TYPE_RENT."'",
		     "v_start <= ".$this->day9,
		     "v_end   >= ".$this->day1);
    if (VM_organizer_here)  $v_where[] = "v_status != '".STATUS_NO."'";
    
    /******************************
     * Get the number of "normal" visitors, i.e. those whose who are not attenders of
     * "big event (say, conferences)" and not whose who rent space from us
     */
    $q = myPear_db()->query("SELECT v_id FROM abs_visits WHERE ".
			    join(' AND ',array_merge(array("(v_eid = 0 OR v_eid IS NULL)"),$v_where)));
    $n_visitors = myPear_db()->num_rows($q);

    /******************************
     * First line of the "matrix" is reserved for the "big events", like programs/conferences.
     * Those have 'e_code' > 0 and come from the Agenda system
     */
    $q = myPear_db()->query("SELECT e_start,e_end,e_name,e_id,count(v_id) AS n  FROM abs_events "
			    ." LEFT JOIN abs_visits ON v_eid = e_id WHERE "
			    .join(' AND ',array("e_code > 0",
						"e_end  >= $this->day1", 
						"e_start<= $this->day9"))
			    ." GROUP BY e_id ORDER BY e_start ASC");
    
    while ($r = myPear_db()->next_record($q)){
      $ndays = (int)(($r['e_end']-$r['e_start'])/86400 + 1);
      $text = b_fmt::strcut($r['e_name'],2*$ndays,'...');
      if (bAuth::authenticated()) $text = x('a href='.x('"',b_url::same(array('vm_eventSelector'=>$r['e_id']))),$text);
      $mPrograms[' NORDITA EVENTS'][] = array('day1' => $r['e_start'],
					      'day9' => $r['e_end'],
					      'text' => $text,
					      'tips' => x('strong',nbsp(myOrg_name.' program')).'<br/>'.nbsp($r['e_name']),
					      'clas' => $this->getRandomColor($r['e_name']),
					      'styl' => 'strong',
					      'url'  => 'em');
      if ($n=$r['n']) $boxes[$r['e_id']] = sprintf("%s %d %s <i>%s</i>", b_t::_('See'), $n, b_t::_('attenders of'), $r['e_name']);
      if (b_checkBox::isOn($r['e_id'])) $where[] = "e_id = $r[e_id]";
    }
    if (empty($where) && !empty($n_visitors))  b_checkBox::set(seeVisitors,'on');
    
    /******************************
     * Now get visitors from the database
     */
    if (!empty($this->imposedEntries)){
      $boxes = array();
      $this->noNavig = True;
      $nnn = 0;
      foreach($this->imposedEntries as $r){
	$mRegistrants['clash'][] = $this->fillEntry($r,'visit '.(++$nnn));
      }
    }else{
      if (VM::e_ID() > 0){
	$this->dbg("Selecting attenders of event ".VM::e_ID());
	$boxes = array();
	$where = array("v_eid = ".VM::e_ID());
      }elseif ($n_visitors > 0){
	$this->dbg("Selecting all visitors");
	if (b_checkBox::isOn(seeVisitors))       $where[] = "( v_code <= 0 OR v_code IS NULL )";
	$boxes[seeVisitors] = sprintf("%s %d %s", b_t::_('See'), $n_visitors, b_t::_('visitors'));
      }
      
      // Show nothing if no visitors is selected for the view .
      // To see all visits set "$where = array(1)"
      if (empty($where)) $where = array(0); 
      $orderBy = (show_guests_by_budget
                  ? 'lm_key'
                  : 'v_group');
      $q = myPear_db()->query("SELECT v_id,v_start,v_end,v_group,v_comment,e_name,lm_key,lm_value,".
			      "av_lastname,av_firstname,av_institute FROM abs_visits ".
			      "    LEFT JOIN abs_events   ON v_eid = e_id ".
			      "    LEFT JOIN abs_expenses ON exp_vid = v_id ".
			      "    LEFT JOIN zzz_avatars  ON v_avid = av_id ".
			      "    LEFT JOIN zzz_list_members ON v_projectid = lm_id ".
			      " WHERE ".join(' AND ',
					     array_merge(array('('.join(' OR ',$where).')'),$v_where)).
			      " ORDER BY e_start,v_start,$orderBy,av_lastname ASC ");
      while($r = myPear_db()->next_record($q)){
	// Group visitors by the program attended
	$displayName = $r['e_name'];
	if (show_guests_by_budget){
	  // group visitors by projects
	  if (empty($r['lm_value']))  $r['lm_value'] = 'undefined';
	  if (empty($displayName)) $displayName = (empty(VM::$description[$r['lm_key']])
						   ? 'project '.$r['lm_value']
						   : 'project '.$r['lm_key']);
	}else{
	  // group visitors by reseach groups
	  if (empty($displayName)){
	    if (empty($r['v_group'])) $r['v_group'] = '?';
	    $displayName = 'group '.myOrg()->orgGroups()->get_groups($r['v_group']);
	  }
	}	

	$mRegistrants[$displayName][] = $this->fillEntry($r,b_fmt::strcut(b_fmt::unescape($displayName),30,'...'));
	if (empty($boxes) && !empty($e_id)){
	  $boxes = array($e_id => sprintf("%s %d %s <i>%s</i>", b_t::_('See'), 
					  myPear_db()->num_rows($q), b_t::_('attenders of'), b_fmt::strcut($r['e_name'],30,'...')));
	  b_checkBox::set($e_id,'on');
	}
      }
    }
    
    /******************************
     * show all matrices (programs + visitors + registrants)
     */
    $this->showHeader();
    
    $this->showBody($mPrograms, True);

    $this->showBody($mRegistrants);

    $this->footer[] = b_checkBox::create($boxes,'<br/>','default','form-submit');
    $this->showFooter();
    if (!empty($arg)) print $arg;
    bTiming()->cpu();
  }
    
  function fillEntry($r,$displayName='?'){
    $av_name = bForm_Avatar::_fmtName('Lf',$r);
    $r['tname']= nbsp(bForm_Avatar::_fmtName('Lf',$r));
    $url = b_url::same(array(b_crypt_no=>1,
			     'form'=>'vm_Visit',
			     'id'  =>$r['v_id']));
    return array('day1' => $r['v_start'],
		 'day9' => $r['v_end'],
		 'clas' => $this->getRandomColor(vm_colorLegend($r)),	
		 'icon' => bForm_Avatar::_getThumbnail($r),
		 'tips' => nbsp(join('<br/>',array(trim(x('strong',$r['tname'])),x('em',@$r['av_institute'])))),
		 'name' => $displayName,
		 'text' => (bAuth::authenticated() ? "<a href='$url'>$av_name</a>" : $av_name), 
		 );
  }
}
