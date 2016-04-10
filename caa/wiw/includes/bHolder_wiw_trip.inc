<?php
locateAndInclude('bHolder');

class bHolder_wiw_trip extends bHolder{

  var $debug = cnf_dev;
  public $av_id = Null;
  public $sortBy_default = 't_departure';

  /*
   *
   */
  function __construct(){
    $this->dbg('');
    parent::__construct();
    //    $this->where = array("l_parent = '".myOrg_ID."'");
    if ($av_id = $_GET['av_id']){
      $this->av_id = $av_id;
      $this->where[] = "av_id='$av_id'";
      //      myPear::H1($this->av->name());
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
		    't_destination'=> '',
		    );
    if ($this->av_id) unset($header['av_lastname']);
    return $header;
  }

  /*
   *
   */  
  function query() {
    /*
      $h = $this->header();
    if (!($s=$_GET['sBy']) || !$h[$s]){
      $_GET['sBy']   = $this->sortBy_default;
      $_GET['sByDir']= 'DESC';
    }
    */
    $_GET['sBy'] = 't_departure';
    $_GET['sByDir'] = 'DESC';
    if (empty($this->where)) $this->where = array(0);
    $q = myPear_db()->qquery("SELECT av_photo,av_email,av_lastname,av_firstname,av_lastname,t_orgid,t_id,t_departure,t_arrival,t_destination,lm_key ".
			     " FROM wiw_trips ".
			     " LEFT JOIN zzz_avatars       ON  t_avid = av_id ".
			     " LEFT JOIN zzz_list_members  ON  t_type = lm_id ".
			     " WHERE ".join(' AND ',$this->where).
			     " ORDER BY $_GET[sBy] $_GET[sByDir]",$this->debug);
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

    if (stripos($this->rec['lm_key'],$this->rec['t_destination']) !== False) unset($this->rec['t_destination']);
    // Select the first option out of multiple ones
    $options = explode(',',$this->rec['lm_key']);
    $this->rec['lm_key'] = array_shift($options);
    
    if (empty($this->recs[$year])) $this->recs[$year] = array();
    $this->recs[$year][] = $this->rec; 
    $this->rec_counter++;
    $this->rec = array();
  }
  
  function listEntries_closing(){
    static $max_per_page = 30;

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
      // Collect records dor one tabs
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
  }
}

