<?php
locateAndInclude('bMailer');

/*
 * Mailer class holder
 */
function EA_mailer(){
  static $mailer = Null;
  if ($mailer === Null)    $mailer = new bMailer_ea();
  return $mailer;
}

class bMailer_ea extends bMailer{

  var $debug  = False;
  var $isMailAutomatic = False;

  public function remind_password($av_id){

    static $alumni_email = 'alumni@nordita.org';

    if ($av_id instanceof bForm_Avatar_ea) $av = $av_id;
    else  $av = myPear::getInstance('bForm_Avatar_ea',$av_id,array('strict','fatal'));

    switch($av->role()){
    case EA_role_alumnus:
      $this->from = $alumni_email;
      $db = 'alumni';
      $dear = 'Dear '.$av->fmtName('sfl');
      $dear = 'Dear Colleague';
      $org_name = myOrg_name;
      $text = "$dear,

$org_name is collecting information about former fellows, faculty members, and long-term visitors into an internal database. The purpose of the database is to obtain reliable demographic data on the subsequent careers of $org_name scientists and to help record the history of the institute. 

For each individual, the database will contain information about education, employment history, other career activities (visiting positions, membership on governing boards and panels, etc.) , and special awards and honors. Access to the database is password restricted and data on individuals will not be publicly available. 

Most of the entries are still incomplete and the purpose of this e-mail is to ask you to check and update the information about yourself in the database. 

You can see your entry via a password protected web page, which has been created for you on our server.  For your convenience the password for your personal page is encoded into the following URL, providing direct access to the page:
<packPWD>

Please save this e-mail or book-mark the URL to facilitate return access.

You can update items in the database and add new ones via your personal page, but, if you prefer, you can also send the relevant information (f.ex. in  the form of a recent CV) to 
'@alumni_email' and our administrative staff will update the entry for you.  

Thank you in advance for your assistance.

Best regards,
Director of $org_name
";
      break;

    case EA_role_employee:
      $db = 'employees';
      $text = "Dear ".$av->fmtName('f').",

As you may already know, we are collecting information about all people who work or have ever worked for $org_name into a $org_name Employee & Alumni database. It is available online at
http://cms.nordita.org?q=ea

We now ask you to help us by reviewing and completing the information about yourself: dates for your degrees, major positions you have held before coming to $org_name, your current address, etc. The only data for any alumnus or employee that will be publicly available is: the name, current position, a photo if available, and dates for the stay at $org_name. All other information is for internal $org_name use only.

To access your information you need to login to a personalized editing web page. For your convenience the password is encoded into the following web address, hence you can access your web page directly by clicking:
<packPWD>

If this does not work, please copy the web address above and paste it into the address field of your web browser. Remember to save this mail or book-mark the address to facilitate later access to the editing web page.

Thanks for helping us to preserve a record of $org_name history!

The $org_name Alumni team";
      break;

    default:
      myPear::WARNING_T("Cancel sending password to non-EA-member ".$av->name());
      return;
    }
    
    $this->send(array($av),
		$av->getEmail(),
		myOrg_name.' '." $db pages",
		$text,
		$syslog_thisMail=True,
		$signThisMail =False);
  }
}
