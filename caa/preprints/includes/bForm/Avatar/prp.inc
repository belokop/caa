<?php
locateAndInclude('APImenu_prp.inc');
locateAndInclude('bForm_Avatar');
locateAndInclude('bMailer_prp');

class bForm_Avatar_prp extends bForm_Avatar{

  function __construct($id, $mode='RO') {
    parent::__construct($id, $mode);
  }

  function isManager(){
    if (empty($av_id))   $av_id = $this->ID;
    if (PRP_censors()->isMember($av_id)) $reply = RANK_prp_censor;
    elseif(superUser_here)$reply = RANK__superuser;
    else                                  $reply = False;
    $this->dbg($reply);
    return $reply;
  }

  protected function getFormBody(){
    switch((int)$this->ID > 0){
    case False:
      if (bAuth::authenticated()){
	return parent::getFormBody();
      }else{
	// New user request, start from "captcha"
	if (locateAndInclude('b_recaptchalib.inc')){
	  $captcha = b_reCAPTCHA();
	  if ($captcha !== True){
	    print $captcha;
	    break;
	  }
	}
	
	// Hopefully it is not a robot who asks for the account
	$this->MODE = 'RW';
	$this->formBlock("",'adding user',
			 array('av_firstname'=>'textField',
			       'av_lastname' =>'textField',
			       'av_email'    =>'textField'));
      }
      break;
      
    case True:      
      $this->contactsBlock();
    }
  }

  /*
   *
   */  
  function formOKspecific() {
    parent::formOKspecific();
    // Automatically add the newly created preprint user to the submitters list
    // and send him the password 
    if ($this->errorless() && !PRP_writers()->isMember($this->ID)){
      // Force creation of the Avatar
      $this->updateDB();
      $this->dbg("created",True);
      PRP_writers()->add_unitMember($this);
      PRP_mailer()->remind_password($this);
    }
  }
}
