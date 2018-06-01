<?php 
if (class_exists('myPear',0)){
  if (JAM_posID())   MSG::H1(JAM_pos()->name());
  else                MSG::H1('JAM - Job Applications Manager','reset');
  JAM_access()->selectPosition(); 
}
?>
<div class='align_left'>
  <dl>
    <h2>How JAM works?</h2>
    <dd>
      Job application starts by filling the web-form for the 
  <?php 
if (class_exists('myPear',0))print bJS()->modal_image_file('desired&nbsp;position','./'.drupal_get_path('module','jam').'/images/best-job-in-the-world.jpg','The desired position');
?>.
    </dd>
    <dd>
      <br/>
      All the documents required for the position (CV, job record, etc.) are uploaded from the same web form.
      <br/>After the form is completed, the system sends requests for the recommendation letters to the referee(s).
    </dd>
    <dd>
      <br/>
      Finally the system creates a set of secure web pages for the applicant and for the referees (if any).
      <br/>Those pages allow to check the status of the application,  correct eventual errors, submit and re-submit the documents, etc.
    </dd>
      <dd>
      <br/>
      The system keeps trace of all the relevant information, like the e-mails, dossiers of candidates, reviews, etc...
      </dd>
    
    <h2>Who participates in the Application procedure</h2>
    <dd>
      The main participants are:
      <dl>
	<dd><b>applicants</b> - individuals who apply for positions.</dd>
	<dd><b>referees</b> - persons who provide recommendation letters for the applicants.
	  <br/>The system assumes that the referees upload their reference letters using their "referee web portal",
	  <br/>however the recommendations sent via E-mail and/or by a normal post might be exceptionally accepted as well 
	  <br/>(they are "injected" into the system by administrators)
	</dd>
	<dd><b>search committee (SC)</b> - a group of senior professionals who evaluate the applications and rank the candidates.
      </dl>
      JAM creates secure "web portals" for the each participant of the application campaign, and all the communications are done via those portals.
    </dd>
    
    
    <h2>Who else is in the game? Access control.</h2>
    <dd>
      The job applications contain highly confidential information which should be accessible to authorized persons only. 
      <br/>
      The system has the following access levels:
      <ul>
	<li> <b>Selection Committee</b> has the following structure:</li>
	<ul>
	  <li> <b>faculty members</b> (if any) take decision about hiring the selected candidates,  may assign rating to the applicants,</li>
	  <li> <b>research committee members</b> (if any) may assign rating to the applicants,</li>
	  <li> <b>observers</b> (if any) may just browse the applicants data.</li>
	</ul>
	<li> <b>secretaries</b> have read access to all information, may send mails to the applicants, referees, SC members.</li>
	<li> <b>admins</b> may also setup the SC, define the position policy, send mails to the applicants, referees, etc.</li>
	<li> <b>superusers</b> may also create positions, nominate admins, and perform maintenance tasks.</li>
      </ul>
    </dd>
    <!--
	Find a person (applicant, referee, SC member etc.) in the applications database.
	Standard regular expressions are supported, say 'hans|sven' will search for all persons called either Hans or Sven.
      -->
  </dl>
</div>
