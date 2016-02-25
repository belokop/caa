<?php
locateAndInclude('pdf_form');

function SU_Travel_reimbursement_form(){
  return array(
	       't01' => array('t'=>'Institutionsnummer',  'c' => ''),
	       't02' => array('t'=>'Address',             'c' => ''),
	       't03' => array('t'=>'Country',             'c' => ''),
	       't04' => array('t'=>'Total',               'c' => ''),
	       't05' => array('t'=>'VAT',                 'c' => ''),
	       't06' => array('t'=>'Currency',            'c' => ''),
	       't07' => array('t'=>'Please',              'c' => ''),
	       't08' => array('t'=>'Fedwire',             'c' => ''),
	       't09' => array('t'=>'Bankname',            'c' => ''),
	       't10' => array('t'=>'Bankaddress',         'c' => ''),
	       't11' => array('t'=>'Account',             'c' => ''),
	       't12' => array('t'=>'Swift',               'c' => ''),
	       't13' => array('t'=>'Project',             'c' => ''),
	       't14' => array('t'=>'N',                   'c' => ''),
	       't15' => array('t'=>'IBAN',                'c' => ''),
	       't16' => array('t'=>'CanadianCodeRouting', 'c' => ''),
	       't17' => array('t'=>'BSBAustralian',       'c' => ''),
	       't18' => array('t'=>'CodeSouthAfrica',     'c' => ''),
	       't19' => array('t'=>'CodeNewZealand',      'c' => ''),
	       't20' => array('t'=>'CodeIndianBank',      'c' => ''),
	       't21' => array('t'=>'Name',                'c' => 'pdf_Lf'),
	       't22' => array('t'=>'PersNumber',          'c' => ''),
	       't23' => array('t'=>'Zip',                 'c' => ''),
	       't24' => array('t'=>'City',                'c' => ''),
	       't25' => array('t'=>'HTelefonnummer',      'c' => ''),
	       't26' => array('t'=>'RDLevId',             'c' => ''),
	       't27' => array('t'=>'Datum',               'c' => ''),
	       't28' => array('t'=>'Forfallodatum',       'c' => ''),
	       );
}
