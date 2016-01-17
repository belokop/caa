<?php

$GLOBALS['blank_data'] = array('dossier'   => '33',
			       'sign_presented' => 'Elizabeth Yang, 553 784 35',
			       'sign_decision'  => 'Marianne P.Söderlind, 553 788 82',
			       //'sign_decision'=> 'Lárus Thorlacius, 553 788 81',
			       );
locateAndInclude('pdf_form');
/*
 * KTH "Application & decision re Travel Scholarship, granted by KTH ́s Schools"
 */
function Application_re_Travel_Scholarship(){
  return scholarship();
}

function scholarship(){
  return array(
	       't01' => array('t'=>'Text1',      'c'=>'',                               'x'=>'File no'),
	       't02' => array('t'=>'Text2',      'd'=>$GLOBALS['blank_data']['dossier'],'x'=>'Doss no'),
	       't03' => array('t'=>'Text3',      'c'=>'pdf_year',                       'x'=>'Year'),
	       // APPLICATION FOR TRAVEL SCHOLARSHIP
	       't04' => array('t'=>'Text4',      'c'=>'pdf_Lf',       'x'=>'Surname, first name'),
	       't05' => array('t'=>'Text5',      'c'=>'pdf_birthdate','x'=>'Date of birth'),
	       't06' => array('t'=>'Text6',      'c'=>'',             'x'=>'Street address'),
	       't07' => array('t'=>'Text7',      'c'=>'',             'x'=>'Postal address'),
	       't08' => array('t'=>'Text8',      'c'=>'av_phone',     'x'=>'Telephone'),
	       't09' => array('t'=>'Text9',      'c'=>'av_email',     'x'=>'E-mail address'),
	       'k44' => array('t'=>'Kryssruta44','c'=>'',             'x'=>'undergraduate studies'),
	       'k45' => array('t'=>'Kryssruta45','c'=>'',             'x'=>'postgraduate/doctoral studies'),
	       't10' => array('t'=>'Text10',     'c'=>'',             'x'=>'Date of admittance'),
	       'k46' => array('t'=>'Kryssruta46','c'=>'',             'x'=>'doctoral studies completed'),
	       't11' => array('t'=>'Text11',     'c'=>'',             'x'=>'Date'),
	       'k42' => array('t'=>'Kryssruta42','c'=>'pdf_K',        'x'=>'F'),
	       'k43' => array('t'=>'Kryssruta43','c'=>'pdf_M',        'x'=>'M'),
	       
	       // (Statement)
	       't12' => array('t'=>'Text12',     'c'=>'pdf_trip',       'x'=>'I hereby apply for a scholarship to cover the following travel'),
	       
	       // If scholarship is granted, I wish to have the money transferred to
	       't13' => array('t'=>'Text13',     'c'=>'', 'x'=>'Banks or personal accounts'),
	       
	       // Signature
	       't14' => array('t'=>'Text14',     'c'=>'pdf_Lf', 'x'=>'Applicant\'name'),
	       
	       // DECISION RE TRAVEL SCHOLARSHIP
	       'k47' => array('t'=>'Kryssruta47','d'=>'Ja', 'x'=>'YES Has this been approved by the financier?'),
	       'k48' => array('t'=>'Kryssruta48','c'=>'',   'x'=>'NO  Has this been approved by the financier?'),
	       't15' => array('t'=>'Text15',     'c'=>'',   'x'=>'Date of decision'),
	       't16' => array('t'=>'Text16',     'd'=>'UF/AlbaNova/Nordita', 'x'=>'School/Department/Unit etc.'),
	       't17' => array('t'=>'Text17',     'd'=>'NORDITA', 'x'=>'Name of financier'),
	       't18' => array('t'=>'Text18',     'c'=>'', 'x'=>'Scholarship amount, SEK'),
	       't19' => array('t'=>'Text19',     'd'=>$GLOBALS['blank_data']['sign_decision'] ,'x'=>'Decision taken by(Name & telephone no.)'),
	       't20' => array('t'=>'Text20',     'd'=>$GLOBALS['blank_data']['sign_presented'],'x'=>'Presented by (Name & telephone no.)'),
	       't21' => array('t'=>'Text21',     'c'=>'', 'x'=>'Decision to be communicated to'),
	       
	       // Accounting
	       't22' => array('t'=>'Text22', 'd'=>'74117',       'x'=>'Baskonto'),  // Text28, Text34
	       't23' => array('t'=>'Text23', 'd'=>'VCB',         'x'=>'Orgenhet'),
	       't24' => array('t'=>'Text24', 'c'=>'v_project',   'x'=>'Projekt'),
	       't25' => array('t'=>'Text25', 'c'=>'',            'x'=>'Aktivitet'),
	       't26' => array('t'=>'Text26', 'c'=>'',            'x'=>'Belopp'),
	       't27' => array('t'=>'Text27', 'd'=>'D'),
	       );
}
