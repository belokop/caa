<?php
/*
 */

locateAndInclude('pdf_form');

function nordea(){
  return array(
	       'xx1' => array('t'=>'form1[0].#subform[0].knp_skrivUt[0]',                 'c'=>'', 'x'=> ' '), // 'Skriv ut' 
	       'emp' => array('t'=>'form1[0].#subform[0].arbetsgivarnamn[0]',             'd'=> 'Kungliga Tekniska Högskolan'),
	       'acc' => array('t'=>'form1[0].#subform[0].arbetsgivarnummer[0]',           'd'=> '505751'),
	       'tlf' => array('t'=>'form1[0].#subform[0].telefonnummer[0]',               'c'=>'', 'x'=> 'telefonnummer'),
	       'cnp' => array('t'=>'form1[0].#subform[0].kontaktperson[0]',               'c'=>'', 'x'=> 'kontaktperson'),
	       'org' => array('t'=>'form1[0].#subform[0].lontagarnummer[0]',              'c'=>'', 'x'=> 'lontagarnummer'),
	       'xx2' => array('t'=>'form1[0].#subform[0].knp_normal[0]',                  'd'=> ' '),  // 'Börja här!',
	       'xx3' => array('t'=>'form1[0].#subform[0].knp_rensaData[0]',               'd'=> ' '), // 'Återställ',
	       'name'=> array('t'=>'form1[0].#subform[0].namn[0]',                        'c'=>'pdf_Lf', 'x'=> 'namn'),
	       'ssn' => array('t'=>'form1[0].#subform[0].personnr1[0]',                   'c'=>'',      'x'=> 'Personnummer/Organisationsnummer'),
	       'c_o' => array('t'=>'form1[0].#subform[0].c_o_adress[0]',                  'c'=>'',      'x'=> 'c_o_adress'),
	       'str' => array('t'=>'form1[0].#subform[0].utdelningsadress[0]',            'c'=>'',      'x'=> 'utdelningsadress'),
	       'city'=> array('t'=>'form1[0].#subform[0].postne_ort[0]',                  'c'=>'',      'x'=> 'postne_ort'),
	       'cc'  => array('t'=>'form1[0].#subform[0].landkod[0]',                     'c'=>'av_cc', 'x'=> 'landkod'),
	       'con' => array('t'=>'form1[0].#subform[0].land[0]',                        'c'=>'pdf_c', 'x'=> 'land'),
	       'bank'=> array('t'=>'form1[0].#subform[0].mottagande_bank[0]',             'c'=>'',      'x'=> 'mottagande_bank'),
	       'swift'=>array('t'=>'form1[0].#subform[0].mottagande_bank_swift[0]',       'c'=>'',      'x'=> 'mottagande_bank_swift'),
	       'clr' => array('t'=>'form1[0].#subform[0].betalningsmottagarens_kontor[0]','c'=>'',      'x'=> 'betalningsmottagarens_kontor'),
	       'iban'=> array('t'=>'form1[0].#subform[0].IBAN[0]',                        'c'=>'',      'x'=> 'IBAN'),
	       'curr'=> array('t'=>'form1[0].#subform[0].valutakod[0]',                   'd'=> 'SEK'),
	       'xx4' => array('t'=>'form1[0].#subform[0].from[0]',                        'd'=> date('Y-m-d',time() + 24*3600)), // 'from',
	       'xx5' => array('t'=>'form1[0].#subform[0].datum[0]',                       'd'=> date('Y-m-d',time())), 
	       );
}
