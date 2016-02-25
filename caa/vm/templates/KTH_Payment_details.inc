<?php
locateAndInclude('pdf_form');
function KTH_Payment_details(){
  return KTH_Betalningsuppgifter();
}

function KTH_Betalningsuppgifter(){
  return array(
	       't1' => array('t'=>'Efternamn  Surname',                  'c'=>'pdf_l'),
	       't2' => array('t'=>'Fornamn  First name',                 'c'=>'pdf_f'),
	       't3' => array('t'=>'Personnummer  date of birth ddmmyy',  'c'=>'pdf_ddmmyy'),
	       't4' => array('t'=>'Adress  Street address',              'c'=>'av_address'),
	       't5' => array('t'=>'Postnr och ort  Postal code and city','c'=>'pdf_zc'),
	       't6' => array('t'=>'Land Country',                        'c'=>'pdf_c'),
	       );
}
