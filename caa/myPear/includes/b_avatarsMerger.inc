<?php
/*
 * Class to merge Avatars.
 *
 * People usually have multiple E-mail addresses, 
 * and it happens when the same person has multiple "Avatar records" in the database 
 * (e-mail is the major key for the record).
 * This class is designed to solve the problem and merge the duplicates.
 */

locateAndInclude('b_checkBox');

class b_avatarsMerger{

  /**
   *
   */
  function __construct(){}

  /*
   *
   */
  public function executor(){

    if (b_cnf::getValue('_virt_keep_me'))  $what = 'resolve';
    elseif(b_checkBox::whatIsOn())         $what = 'select';
    else                                   $what = 'search';

    switch($what){
    case 'search':
      // Go to the search for the clashing objects
      myPear_search()->getForm(array('_lookFor'=> 'X', 
				     '_header' => 'Search for duplicated entries in the database'));
      break;

    case 'select':
      // Select the pivot record
      $this->showSelection(b_checkBox::whatIsOn(),
			   'Records for the selection');
      break;

    case 'resolve':
      // Resolve & show the result
      $this->mergeAndShowResult(b_cnf::getValue('_virt_keep_me'));
    }
  }  

  /**
   * Present the duplicated entries (the selection results)
   * as a form with "radio buttons" to select the pivot record.
   */
  private function showSelection($listofavatars,$title){

    $header = array('key'=>'');
    $table  = array();
    foreach ($listofavatars as $av_id){
      if ($av = myPear::getInstance('bForm_Avatar',$av_id,array('nocache','strict'))){
	$av->defineVariables();
	$header[$av_id] = $av_id;
	print myPear::H1($av->fmtName(),array('reset','noTranslate'));
	foreach ($av->formDB as $k=>$v) {
	  if (!empty($v) && (strpos($k,'av_id')===False) && (strpos($k,'av_') === 0)){
	    $v = str_replace(',',' ',b_time::unix2txt($v));
	    if (count($listofavatars)>1) $v = b_fmt::strcut($v,40,'...');
	    $table[$k]['key']  = x('strong',nbsp($av->getDescr($k)));
	    $table[$k][$av_id] = $v;
	  }
	}
      }
    }

    // Print the selected Avatars
    foreach($table as $k=>$v){
      if (empty($t)){
	print x('h3',$title);
	$t = new b_table_zebra($header);
      }
      $t->autoBreakLine = True;
      $t->prt($v);
    }
    if (is_object($t)) $t->close();

    // Prompt for the selection
    if (empty($_POST['listofavatars']) && count($listofavatars)>1){
      print x('h2','Please select the record to keep');
      $header2 = $header;
      unset($header2['key']);
      $f = new bForm('new','RO',False);
      $f->modalRW_enabled = cnf_inside_fb_iframe;
      $f->MODE = 'RW';
      $f->bForm_action_custom = b_url::same('?function=b_avatarsMerger--executor&resetcache_once=1');
      $f->defineVariable('Please check the record which you think is the correct one,-BR-'.
			 'then confirm the selection.-BR-'.
			 '<i>(See above for the records content)</i>','_virt_keep_me');
      $f->defineTypes(array('isMBE'      =>array('_virt_keep_me'),
			    'isRW_forced'=>array('_virt_keep_me')));
      $f->getFormOpen();    
      $f->radioButton('_virt_keep_me',$header2,Null,Null);
      $f->textHidden('listofavatars',join(',',$listofavatars));
      $f->closeForm('confirm the selection');    
    }

  }
  
  /**
   * Merge the records and show the result
   */
  private function mergeAndShowResult($toKeep){

    $listofavatars = explode(',',b_cnf::getValue('listofavatars'));

    // Show the header - the last/first name of the avatar
    $av = myPear::getInstance('bForm_Avatar',$toKeep,array('fatal','nocache'));
    $av->defineVariables();
    print myPear::H1($av->fmtName(),array('reset','noTranslate'));
    
    // Show the records which will be merged now
    $this->showSelection($listofavatars,
			 'Records which are merged');
    
    // Perform the merge and show the result   
    foreach ($listofavatars as $n=>$av_id){
      bForm_Avatar::merge($av_id,$av->ID,$dryRun=0);
    }
    
    $this->showSelection(array($av->ID),"Resulting record $av->ID");
    myPear_logs()->add('merge',$av->ID);

    // Optionally print the "back" button
    if ($url = b_url::getReferrer('bToolBox')){
      print b_btn::UL_buttons(array(b_btn::submit('back to the Tool-Box', $url)));
    }
  }
}
