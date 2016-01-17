<?php

class bForm_BugReport extends bForm {
  
  var $TABLE   = 'bugs';
  var $TABLEID = 'bug_id';
  var $title   = '';

  function defineVariables() {
    $this->defineVariable("bug_id",          'bug_id');
    $this->defineVariable("Report title",    'bug_title_RO');
    $this->defineVariable("&nbsp;",          'bug_intro_skip_RO');
    $this->defineVariable("Report date",     'bug_time_skip_RO');
    $this->defineVariable("Report date",     'bug_time_skip');
    $this->defineVariable("Reporter address",'bug_ip_RO');
    $this->defineVariable("Your comment",    'bug_text');
    $this->defineVariable("Your E-mail",     'bug_email');
  }

  /*
   *
   */
  function __construct($id, $mode='RW', $title) {
    $this->title = preg_replace("/[\'\"]/i",' ',$title);
    parent::__construct($id, $mode, True);
  }

  function isReadable(){
    return True;
  }

  function isWritable(){
    return (int)$this->ID ? False : True;
  }

  function getForm() {
    global $formForBugReport;
    if ($this->MODE == 'RO') {
      print  "<h1>Bug report</h1>";
    }else{
      print  "<h1>Submitting bug report</h1>";
      $_POST['bug_title_RO']     = $this->title;
      $_POST['bug_time_RO']      = time();
      $_POST['bug_ip_RO']        = $_SERVER["REMOTE_ADDR"];
      if ($formForBugReport && ($id = $formForBugReport->formDB[$formForBugReport->TABLEID])) $_POST['bug_form_RO']  = $id;
      $_POST['bug_intro_skip_RO']=preg_replace("/\n/","&nbsp;<br/>",
"The system went into an undefined state,
which is most probably a bug.

The bug report is now ready to be sent to the development team,
you can help us to fix the problem by adding your comments.

");
    }
    $_POST['bug_time_skip_RO'] = date("Y-m-d",$this->getValue('bug_time_RO',$asIs=True));
    $this->getFormOpen();
    if ($this->MODE == 'RW')
    $this->textField ("bug_intro_skip_RO");
    $this->textField ("bug_title_RO");
    $this->textField ("bug_time_skip_RO");
    $this->textArea  ("bug_text");
    $this->textHidden("bug_form_RO");
    $this->textHidden("bug_ip_RO");
    $this->textHidden("bug_form_RO");
    if (!$this->getValue('bug_form_RO',$asIs=True))  $this->textField ("bug_email");
    $this->closeForm();
  }
  
  function formOKspecific() {
    $this->log('bug',$this->formDB_modified['bug_title_RO']);
  }
}

