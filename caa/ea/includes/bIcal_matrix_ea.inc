<?php
locateAndInclude('bIcal_matrix');
locateAndInclude('bForm_Avatar_ea');

/*
 * Set the graph limits to [year 1957] <-> [2 months from now]
 */
define('EA_org_founded',1957);

class bIcal_matrix_ea extends bIcal_matrix{
  
  protected $step = 7;
  var       $showLegend = False;
  public    $noNavig_months = True;
  
  function __construct(){
    parent::__construct();
  }
  
  /*
   * Select the year to show, set margin to 3 weeks, i.e. nDaysToShow is 52+6 = 58 weeks 
   */
  private function select_year(){
    $this->day1 = b_time::monday(b_time::txt2unix((ea_year-1).'-12-10'));
    $this->day9 = $this->day1 + 58 * 7 *86400;
    
    for($y=date('Y',time()); $y>=EA_org_founded; $y--) $years[$y] = $y;
    $f = new bForm('new','RO',False);
    $f->formDB['ea_year'] = ea_year;
    $f->submitOnChange['ea_year'] = True;
    $s = $f->selectMenu_exec('ea_year',$years,"class='ea-year-select'",$onlyRW=True);
    myPear::setPageHeader(myPear::set_main_selector(array(myOrg_name.' in '=>$s),True,True),
			  $strip_tags = False);
    if (empty($_REQUEST['ea_year'])) $_REQUEST['ea_year'] = $_POST['ea_year'] = date('Y',time());
  }
  
  function show($matrix=null){    
    static $colors = array('director'      => 'orange',
			   'vice director' => 'orange',
			   'faculty'       => '#26D4FE',
			   'fellows'       => '#9EC633',
			   'phd students'  => '#FDFA3D',
			   'visitors'      => '#FDDAD9',
			   'board'         => '#DD9988',
			   'administration'=> 'orange',
			   'IT support'    => 'orange');

    $this->select_year();
    
    myOrg()->eaMembers()->debug = $this->debug;
    foreach(myOrg()->eaMembers()->getMembers_byDate($this->day1,$this->day9) as $r){
      $av_id   = $r['av_id'];
      $av      = new bForm_Avatar_ea($av_id);
      $name_Lf = $av->fmtName('Lf');
      $name_sfl= $av->fmtName('sfl');
      $clas    = $this->av_rate($r['lm_value'],@$r['e_rate']); // employers "rate"
      $a2 = True ? $clas : $name_Lf;
      
      $photo = myPear_photos.$r['av_photo'];
      if (is_file($photo)) $photo = bJS()->modal_image_file($name_Lf,$photo,$name_Lf);
      else                 $photo = $name_Lf;

      $alumni[$clas][$a2][]=array('day1' => b_time::txt2unix($r['lm_key']),
				  'day9' => b_time::txt2unix($r['lm_status']),
				  'text' => (is_object(bAuth::$av)
					     ? ($av->isWritable()
						? x('a href='.b_url::same("?id=".$av->ID."&form=".bForm_Avatar::$bForm_Avatar),$name_sfl)
						: bJS()->popup($name_sfl,"?id=$av->ID&form=".bForm_Avatar::$bForm_Avatar)
						)
					     /*
					       ? bJS()->popup($name_sfl,"?id=$av->ID&form=".bForm_Avatar::$bForm_Avatar)
					     */
					     : $photo),
				  'clas' => $this->getCSS($clas,@$colors[$clas]),
				  'name' => nbsp($a2),
				  'tips' => nbsp(preg_replace('/<br ?.?><br ?.?>/','<br/>',
							      join('<br/>',array(x('strong',$name_Lf),
										 x('em',$r['lm_value']))))));
    }
    $this->showHeader();
    foreach(array_keys($colors) as $type) if ($r=@$alumni[$type]) $this->showBody($r);
    $this->showFooter();
  }
  
  protected function arrow_left(){
    if (ea_year <= EA_org_founded) return '';
    else return bIcons()->getButton(array('class'=>'page_load_progress',
						  'i'=>'i-arrow_left',
                                                  'c'=>'bIcal_nav',
                                                  'l'=>b_url::same('?ea_year='.max(EA_org_founded,ea_year-1))));
  }

  protected function arrow_right(){
    if (ea_year >= date('Y',time())) return '';
    else return bIcons()->getButton(array('class'=>'page_load_progress',
						  'i'=>'i-arrow_right',
						  'c'=>'bIcal_nav',
                                                  'l'=>b_url::same('?ea_year='.min(ea_year+1,date('Y',time())))));
  }

  private function av_rate($position,$e_rate){
    static $t1 = array(
		       'fellow'   =>'fellows',
		       'postdoc'  =>'fellows',
		       'professor'=>'faculty',
		       'student'  =>'phd students',
		       'adm'      =>'administration',
		       'computing'=>'IT support',
		       'web'      =>'IT support');
    static $t2 = array('visit'    =>'visitors',
		       'corresp'  =>'visitors',
		       'baltic'   =>'visitors',
		       'vice'     =>'deputy director',
		       'deputy'   =>'deputy director',
		       'board'    =>'board');
    static $rate1 = array('director'       => array('director'),
			  'faculty'        => array('professor'),
			  'fellows'        => array('fellow','postdoc'),
			  'phd students'   => array('student'),
			  'administration' => array('adm','hr officer','coordinator'),
			  'IT support'     => array('web','computing'));
    static $rate2 = array('visitors'       => array('visit','corresp','baltic'),
			  'board'          => array('board'),
			  'deputy director'=> array('vice','deputy'));
    
    $rateN = '?';
    foreach(array($rate1,$rate2) as $rates){
      foreach ($rates as $t=>$list){
	foreach($list as $pos) if (stristr($position,$pos)) $rateN = $rate = $t;
      }
    }

    foreach ($t1 as $pos=>$t)  if (stristr($position,$pos)) $rate = $t;
    foreach ($t2 as $pos=>$t)  if (stristr($position,$pos)) $rate = $t;
    if (empty($rate))                                       $rate = strToLower($position);
    if ($rate !== $rateN)    myPear::WARNING_T("rate '$rateN' ==> '$rate'");
    return $rate;
  }
}
