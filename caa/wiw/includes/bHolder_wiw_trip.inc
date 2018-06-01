<?php
locateAndInclude('bHolder');

class bHolder_wiw_trip extends bHolder{

  public $av_id = Null;
  public $sortBy_default = 't_departure';
  public static $_debug = True;

  /*
   *
   */
  function __construct(){
    $this->dbg(@$_REQUEST['av_id']);
    parent::__construct();

    if (is_numeric(@$_REQUEST['av_id']) && $_REQUEST['av_id'] > 0){
      $this->av_id = $_REQUEST['av_id'];
    }elseif(!empty($_GET['all_trips_once'])){
      // Select the employees
      $in = array(-999);
      foreach(myOrg()->eaMembers()->get_memberInfo(Null,EA_role_employee,'get_byStatus') as $av_id=>$r) $in[] = $av_id;

      // Select the employee's trips
      $q = myPear_db()->qquery('SELECT t_avid,av_lastname,av_firstname FROM wiw_trips LEFT JOIN zzz_avatars ON t_avid=av_id '.
			       ' WHERE t_avid != '.bAuth::$av->ID.' AND t_avid IN ('.join(',',$in).') GROUP BY t_avid ORDER BY av_lastname ASC',cnf_dev);
      while($r = myPear_db()->next_record($q)) $ids[$r['t_avid']] = bForm_Avatar::_fmtName('Lf',$r);
      if (empty($ids)) $ids = array();
      else $ids['*select_name*'] = 'the traveler';

      // Select the employee to edit
      $f = new bForm('new','RO',False);
      $f->defineVariable('Select the traveler','av_id');
      print x("form action='".b_url::same()."' method='post'",
	      x('strong','Select the traveler').
	      $f->selectMenu_exec('av_id',$ids,"onchange='submit()'",$onlyRW=True));
    }else{
      $this->av_id = bAuth::$av->ID;
    }

    if (!empty($this->av_id)){
      $this->where[] = "av_id=".$this->av_id;
      $this->dbg($this->where);
      unset($_GET['all_trips_once']);
    }
    
    // Info message
    MSG::INFO(array(wiw_acl_public. ' - '.wiw_acl_public_text,
		       wiw_acl_protected.' - '.wiw_acl_protected_text),
		 'The trip privacy might be "'.join('" OR "',array(wiw_acl_public,wiw_acl_protected)).
		 '", the default is "'.wiw_acl_default.'"',
		 $only_once=True);
    MSG::INFO(array('all trips - click "Make all trips protected/public"',
		       'one trip - click on the privacy value of the desired trip'),
		 'You can change the privacy of your trips at any time:',
		 $only_once=True);
    
    
    // Toggle the trip privacy
    if (isset($_REQUEST['acl_once'])){
      $this->set_acl($_REQUEST['acl_once'],		     
		     $_REQUEST['set_once']);
    }

    // Toggle the trip privacy
    if (!empty($_REQUEST['all_acls_once'])){
      $q = $this->query('t_id');
      while($r = myPear_DB()->next_record($q)) $this->set_acl($r['t_id'],$_REQUEST['all_acls_once']);
    }
    
  }

  /*
   *
   */
  private function set_acl($t_id,$new_value){
    locateAndInclude('bForm_wiw_trip');
    $t = new bForm_wiw_trip($t_id);
    $this->show_gv = cnf_dev;
    $old_value = $t->getValue('t_acl');
    if ($old_value != $new_value){
      $t->updateDB(array('t_acl'=>$new_value));
      MSG::MESSAGE(sprintf($t->name(bAuth::$av->ID != $this->av_id).' is now "%s"',$new_value));
    }
  }

  /*
   *
   */
  function header() {
    $this->dbg('');
    $header = array('icon_nosort_afirst' => ' ',
		    'av_lastname'=> 'Who',
		    't_departure'=> "When",
		    'lm_key'     => "What",
		    't_destination'=> "Where",
		    't_acl'        => "Privacy",
		    );
    if ($this->av_id) unset($header['av_lastname']);
    return $header;
  }

  /*
   *
   */  
  function query($select=Null) {
    $_GET['sBy'] = 't_departure';
    $_GET['sByDir'] = 'DESC';
    if (empty($this->where)) $this->where = array(0);
    if (empty($select)) $select = 'av_photo,av_email,av_lastname,av_firstname,av_lastname,t_acl,t_orgid,t_id,t_departure,t_arrival,t_destination,lm_key';
    $q = myPear_db()->qquery("SELECT $select ".
			     " FROM wiw_trips ".
			     " LEFT JOIN zzz_avatars       ON  t_avid = av_id ".
			     " LEFT JOIN zzz_list_members  ON  t_type = lm_id ".
			     " WHERE ".join(' AND ',$this->where).
			     " ORDER BY $_GET[sBy] $_GET[sByDir]",cnf_dev);
    return $q;
  }
  
  /*
   *
   */
  private $rec_counter = 0;
  private $recs = array(); 
  function format() {

    // Extract the trip year
    $year = date('Y',$this->rec['t_departure']);

    $this->rec['icon_nosort_afirst'] = bForm_Avatar::_getThumbnail($this->rec);

    $this->rec['av_lastname']= bForm_Avatar::_fmtName('Lf',$this->rec);

    $this->rec['t_departure'] = x("a href=".x("'",b_url::same("?form=wiw_trip&id=".$this->rec['t_id'])),
				  x('code',b_time::period($this->rec['t_departure'],$this->rec['t_arrival'])));

    if (empty($this->rec['t_acl'])) $this->rec['t_acl'] = wiw_acl_default; 
    $t_acl = (empty($this->rec['t_acl'])
	      ? wiw_acl_default 
	      : $this->rec['t_acl']);
    $new_acl = ($t_acl == wiw_acl_public ? wiw_acl_protected : wiw_acl_public);
    $this->rec['t_acl'] = bJS()->tip(x("a href=".x("'",b_url::same("?av_id=".$this->av_id."&acl_once=".$this->rec['t_id']."&set_once=".$new_acl)),$t_acl),
				     'Click to toggle');
    
    if (stripos($this->rec['lm_key'],$this->rec['t_destination']) !== False) unset($this->rec['t_destination']);
    // Select the first option out of multiple ones
    $options = explode(',',$this->rec['lm_key']);
    $this->rec['lm_key'] = array_shift($options);
    
    if (empty($this->recs[$year])) $this->recs[$year] = array();
    $this->recs[$year][] = $this->rec; 
    $this->rec_counter++;
    $this->rec = array();
  }
  
  /*
   * drawHeader is dummy for WIW
   */
  function drawHeader(){
    static $dejaVu = 0;
    if (!empty($this->av_id) && !$dejaVu++) print x('h3',bForm_Avatar::_fmtName('fl',$this->rec));
  }

  /*
   *
   */
  function listEntries_closing(){
    static $max_per_page = 30;

    if (!empty($_GET['all_trips_once'])) return;

    // Collect the years into tabs, ~30 entries per tab
    if ($this->rec_counter <= $max_per_page){
      $tabs = array('all'=>'.*');
    }else{
      $tabs = $years = array();
      $n_perPage = 0;
      $n_current = 0;
      foreach($this->recs as $year=>$recs){
	if ($n_perPage > $max_per_page) {
	  $n_perPage = 0;
	  $n_current++;
	}
	$years[$n_current][] = $year;
	$n_perPage += count($recs);
      }
      foreach($years as $yy){
	if (($n=count($yy)) == 1){
	  $tabs[$yy[0]] = $yy[0];
	}else{
	  // Save width, replace "2011-2012" by "2011-12"
	  if (0) $tabs[$yy[$n-1].'-'.preg_replace('/^'.substr($yy[$n-1],0,2).'/','',$yy[0])] = join('|',$yy);
	  else   $tabs[$yy[$n-1].'-'.preg_replace('/^20/','',$yy[0])] = join('|',$yy);
	}
      }
    }

    // Show the trips    
    locateAndInclude('b_tabs');
    $b_tabs = new b_tabs_myPear(array_keys($tabs));
    foreach($tabs as $tab=>$search_string){
      if (!$b_tabs->body()) continue;
      // Collect records for one tabs
      $years = preg_grep("/^($search_string)/",array_keys($this->recs));
      $recs = array();
      foreach($years as $year) $recs = array_merge($recs,$this->recs[$year]);
      // Print the tab
      $t = new b_table_zebra($this->header());
      $t->noSort = True;
      foreach($recs as $rec){
	$t->class['icon_nosort_afirst'] = 'bg_transp';
	$t->extraTD[] = myOrg($rec['t_orgid'])->getFavicon();
	$t->prt($rec);
      }
      $t->close();
    }
    $b_tabs -> close();
    print b_btn::UL_buttons(array(b_btn::big_button('Make all trips '.wiw_acl_protected, array('av_id' => $this->av_id,
											       'all_acls_once'=>wiw_acl_protected,
											       'resetcache_once'=>1),
						    'red',True),
				  b_btn::big_button('Make all trips '.wiw_acl_public,  array('av_id' => $this->av_id,
											     'all_acls_once'=>wiw_acl_public,
											     'resetcache_once'=>1),
						    'green',True)));
  }
}

