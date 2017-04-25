<?php
locateAndInclude("bHolder");
locateAndInclude(bForm_Avatar::$bForm_Avatar);
class bHolder_Avatar extends bHolder {

  var $debug = False;
  
  var $sortBy_default = 'name';
  var $av = Null; // Avatar class
  public $show_icons = False;

  public function __construct(){ 
    parent::__construct();
    if ($av_id = @$_GET['av_id']){
      $this->av = myPear::getInstance(bForm_Avatar::$bForm_Avatar,$av_id,array('fatal','nocache'));
      $this->av->aceptAsIs_DoNotSave = True;
    }
  }
  
  /*
   *
   */
  function header(){
    if (empty($this->custom_header)){
      $this->header = array('icon_nosort_afirst' => ' ',
			    'name'               => 'Name',
			    'av_ssn'             => ' ',
			    'av_email'           => 'Email',
			    'av_institute'       => "Affiliation",
			    'av_lastlogin'       => 'login');
      if (!EA_ORG_manager_here) unset($this->header['av_ssn']);
    }else{
      $this->header = $this->custom_header;
    }
    if (!myPear_access()->authorized()) unset($this->header['av_email'],
					      $this->header['av_lastlogin']);
    return $this->header;
  }
  
  /*
   *
   */
  protected $only_my_avatars = Null;
  function query($only_my_avatars=False) {
    $this->only_my_avatars = $only_my_avatars;
    if ($this->av) $this->where[] = "av_id = ".$this->av->ID;
    if (empty($this->where)) $this->where = array(1);
    $q = myPear_db()->qquery("SELECT *, av_lastname AS name, av_email AS email ".
			    " FROM zzz_avatars ".
			    " LEFT JOIN zzz_unit_members    ON um_avid = av_id ".
			    " LEFT JOIN zzz_units           ON um_uid = u_id ".
			    join(" ",$this->left_join).
			    " WHERE ".
			    join(' AND ',$this->where).
			     myPear_db()->orderBy('av_lastname'),cnf_dev);
    return $q;
  }

  /*
   *
   */  
  function rejectRecord(){
    if (empty($this->rec['av_id'])){
      if (cnf_dev) b_debug::traceBack('empty($this->rec[av_id])');
      return True;
    }
    elseif (@$this->dejaVu[$this->rec['av_id']]++)                return True;
    elseif ( $this->av && ($this->rec['av_id'] != $this->av->ID)) return True;
    else   return False;
  }

  /*
   *
   */
  protected function avatar_anker($av_id){
    $reply = b_url::same(sprintf("?form=%s&id=%d",bForm_Avatar::$bForm_Avatar,$av_id),
			 b_url::get_GET('^(act_|ed_blist)',True));
    $this->dbg($reply);
    return $reply;
  }

  /*
   *
   */  
  function format() {

    $this->t->class['icon_nosort_afirst'] = 'bg_transp';

    // Escape names
    foreach (array('av_institute',
		   ) as $k){
      if (isset($this->rec[$k])) $this->rec[$k] = b_fmt::escape($this->rec[$k]);
    }
    
    // Shorten up the verbose institute names
    foreach($this->header() as $k=>$v) if(isset($this->rec[$k])) $this->rec[$k] = b_fmt::strcut(@$this->rec[$k], $this->strLength, '...');
    
    // Name is a link to an edit page
    $this->rec['name'] = x("a href='".$this->avatar_anker($this->rec['av_id'])."'",
			   bForm_Avatar::_fmtName('Lf',$this->rec));
    if (!empty($this->rec['av_ssn'])) $this->rec['av_ssn'] = x('em style="font-size:smaller;"',$this->rec['av_ssn']);
    $this->rec['icon_nosort_afirst'] = bForm_Avatar::_getThumbnail($this->rec);

    if ($d = $this->rec['av_lastlogin']) $this->rec['av_lastlogin'] = b_time::strftime('%Y-%m-%d',$d);

    if ($this->show_icons && EA_member_here()){
      // Get membership in all the bUnits for the organization
      $ranks = array();
      $look_for = array(RANK__ea_member,
			RANK__ea_admin,
			RANK__ea_org_manager,
			RANK__superuser);
      $look_for = array();		      
      foreach(bUnit::get_unitsMembership(myOrg_ID, $look_for, $this->rec['av_id']) as $av_id=>$rrs){
	foreach($rrs as $r) $ranks[] = $r['u_rank'];
      }
      sort($ranks);   
      foreach($ranks as $rank){
	$icon = '';
	switch($rank){
	case RANK__ea_member:
	  $icon = myOrg()->eaMembers()->unitMemberIcon($this->rec['av_id']);
	  break;
	case RANK__ea_admin:
	case RANK__ea_org_manager:
	case RANK__superuser:
	  $icon = b_reg_bUnit::get_icon($rank,myPear_MODULE);
	  break;
	default:
	  $icon = b_reg_bUnit::get_icon($rank);
	}
	if ($icon) $this->t->extraTD[] = $icon;
      }
    }
  }
}
