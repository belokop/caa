<?php
require_once(myPear_root.'/includes/_tcpdf/config/lang/eng.php');
require_once(myPear_root.'/includes/_tcpdf/tcpdf.php');

class bForm_Igor extends bForm{

  /*
   * This form does not have database table
   */
  function __construct(){
    parent::__construct('empty','RW',False);
  }

  public function defineVariables(){
    $this->defineVariable(array('d_firstname'=> 'Firstname',
				'd_lastname' => 'Lastname',
				'd_invoice'  => 'Invoice',
				'd_price'    => 'Price per night',
				'd_room'     => 'Room number',
				'd_group'    => 'CERN group',
				'_d_period'  => 'Check-in / Check-out',
				),True);
    $this->defineTypes(array('isMONEY' => array('d_price'),
			     'isVITAL' => preg_grep('/^d_/',array_keys($this->vars)),
			     ));
  }

  protected function getFormHeader(){
    if (empty($this->formDB['d_room']))    $this->formDB['d_room']    = '9404';
    if (empty($this->formDB['d_price']))   $this->formDB['d_price']   = 60;
    if (empty($this->formDB['d_invoice'])) $this->formDB['d_invoice'] = 80000;
    if (empty($this->formDB['d_group']))   $this->formDB['d_group']   = 'PH-ULB';
  }

  protected function getFormBody(){
    $this->formBlock('',
		     'Ну вот мы и в '.'CERN...',
		     array('d_firstname'=> array('textField'),
			   'd_lastname' => array('textField'),
			   'd_group'    => array('textField'),
			   '_d_period'  => array('datePickerField2','_virt_start','_virt_end'),
			   'd_room'     => array('textField'),
			   'd_price'    => array('textField'),
			   'd_invoice'  => array('textField'),
			   ));
  }

  function getFormFooter(){
    $this->closeForm('get the Receipt'); 
  }

  protected function displayErrors_preflight(){
    return array();
  }
}

class TCPDF_Igor extends TCPDF {
  
  public $subHeader = '';
  
  function __construct(){
    
    parent::__construct(PDF_PAGE_ORIENTATION, PDF_UNIT, PDF_PAGE_FORMAT, true, 'UTF-8', false);
    
    // set document information
    $this->SetCreator(PDF_CREATOR);
    $this->SetAuthor('CERN hostel');
    $this->SetTitle('CERN hostel');
    $this->SetSubject('CERN hostel');
    $this->SetKeywords('TCPDF, PDF');
    
    // set default header & footer data
    $this->SetHeaderData(PDF_HEADER_LOGO, 
                         PDF_HEADER_LOGO_WIDTH, 
                         PDF_HEADER_TITLE.' 001', 
                         PDF_HEADER_STRING,
                         array(0,64,255), array(0,64,128));
    $this->setFooterData($tc=array(0,64,0), 
                         $lc=array(0,64,128));
    
    // set header and footer fonts
    $this->setHeaderFont(Array(PDF_FONT_NAME_MAIN, '', PDF_FONT_SIZE_MAIN));
    $this->setFooterFont(Array(PDF_FONT_NAME_DATA, '', PDF_FONT_SIZE_DATA));
    
    // set default mono-spaced font
    $this->SetDefaultMonospacedFont(PDF_FONT_MONOSPACED);
    
    //set margins
    $this->SetMargins(PDF_MARGIN_LEFT, PDF_MARGIN_TOP, PDF_MARGIN_RIGHT);
    $this->SetHeaderMargin(PDF_MARGIN_HEADER);
    $this->SetFooterMargin(PDF_MARGIN_FOOTER);
    
    //set auto page breaks
    $this->SetAutoPageBreak(TRUE, PDF_MARGIN_BOTTOM);
    
    //set image scale factor
    //$this->setImageScale(PDF_IMAGE_SCALE_RATIO);
    $this->setImageScale(1);

    //set some language-dependent strings
    $this->setLanguageArray($GLOBALS['l']);

    // ---------------------------------------------------------
    
    // set default font sub-setting mode
    $this->setFontSubsetting(true);
  }
  
  public function Header() {
    
    // Title
    $this->SetFont('helvetica', 'N', 8);
    //$this->SetFont('times', '', 8);

    $this->Cell(0, 15, ' ', 0, false, 'C', 0, '', 0, false, 'M', 'M');
    if (0) $html = x('div style="text-align:center;"',
		     join('<br/>',array(x('strong',strToUpper('Organisation europeenne pour la recherche nucleaire')),
					x('strong',strToUpper('European Organization for Nuclear Research')),
					'Organisation européenne pour la recherche nucléaire',
					'European Laboratory for Particle Physics')));
    // Header

    $t = new b_table('width=100%'); 
    $t->tro();
    $t->td('<img src="'.myPear_root.'images/logo_CERN_80b.png" width=50 height=50/>',"width=51");
    $t->td(x('div style="text-align:center;"',
	     join('<br/>',array(
				x('strong',strToUpper('Organisation europeenne pour la recherche nucleaire')),
				x('strong',strToUpper('European Organization for Nuclear Research')),
				'Organisation européenne pour la recherche nucléaire',
				'European Laboratory for Particle Physics'))),
	   "width=90%");
    $t->trc();
    $t->tr(' <br/>');    $t->tr(' <br/>');    $t->tr(' <br/>');
    $t->close();
    $html = ob_get_contents();
    ob_end_clean();

    // Signatures

    ob_start();   
    $t = new b_table('width=20%'); 
    //$t->tr('<img src="'.myPear_root.'images/logo_CERN_80b.png" width=50 height=50/>');
    $t->tr(array('Organization'));
    //$t->tr(array('Addresse postale / Mailing address*:',
    $t->tr(array('Mailing address*:',
		 'Foyer Hostel CH - 1211 GENEVA 23 Switzerland')); 
    $t->tr(array('Téléphone / Telephone:',
		 '+41 22 767 4481'));
    $t->tr(array('Télécopieur / Telefax:',
		 '+41 22 767 3800'));
    $t->tr(array('Courriel / Email:',
		 'cern.hostel@cern.ch'));
    $t->close();
    $tt = ob_get_contents();
    ob_end_clean();
    $html .= $tt;
    
    $html = str_replace('&nbsp;',' ',b_fmt::compact_html($html,$keepQuotes='pdf'));

    $this->writeHTMLCell($w=0, $h=0, 
			 $x=15, $y='', 
			 $html, $border=0, 
			 $ln=1, $fill=0, $reseth=true, 
			 $align='J', $autopadding=true);
  }
  
  /**
   * Output an html code as a pdf file
   */
  function html2pdf($output_file,$html,$mode='I'){

    // dejavusans is a UTF-8 Unicode font, if you only need to
    // print standard ASCII chars, you can use core fonts like
    // helvetica or times to reduce the output file size.
    //$this->SetFont('dejavusans', '', 8, '', true);
    $this->SetFont('helvetica', 'N', 8);

    $this->AddPage();

    // set text shadow effect
    $this->setTextShadow(array('enabled'=>true, 
			       'depth_w'=>0.2, 
			       'depth_h'=>0.2, 
			       'color'=>array(196,196,196),
			       'opacity'=>1,
			       'blend_mode'=>'Normal'));
    
    // Print text using writeHTMLCell()
    $html = str_replace('&nbsp;',' ',b_fmt::compact_html($html,$keepQuotes='pdf'));
    $this->writeHTMLCell($w=0, $h=0, 
			 $x='', $y=80., 
			 $html,
			 $border=0, $ln=1, $fill=0, $reseth=true, $align='', $autopadding=true);
    // Close and output PDF document
    $this->Output($output_file, $mode);
  }    
}

function igor_hr($t){
  $t->tro(); 
  $t->td('<div style="margin-top:0;margin-bottom:0;padding-top:0;padding-bottom:0;">'.
	 '<hr  style="border:0;margin-top:0;margin-bottom:0;padding-top:0;padding-bottom:0;" />'.
	 '</div>','colspan="5"'); 
  $t->trc();
}

myPear::H1('Ну вот мы и в CERN\'е... Hostel calculator','reset');

$f = new bForm_Igor();
ob_start();
$f->getForm();
$form = ob_get_contents();
ob_end_clean();

if ($f->MODE == 'RW'){
  print $form;
}else{
  // Proforma header
  ob_start();
  $t = new b_table('width=100%');
  $t->tr(array(x('strong','Invoice'),
	       $f->getValue('d_invoice',1),
	       x('strong','Date'),
	       date('d.m.Y',$f->getValue('_virt_end'))));
  $t->tr(array(x('strong','Cashier'),
	       '1 / SAN',
	       x('strong','VAT code'),
	       'VAT-Code in Stammdaten ändern!'));
  $t->tro();
  $t->td(x('strong','Reservation'));
  $t->td($f->getValue('d_room').', '.date('d.m.Y',$f->getValue('_virt_start')).' - '.date('d.m.Y',$f->getValue('_virt_end')),'colspan=2');
  $t->trc();
  $t->close();
  $t1 = ob_get_contents();
  ob_end_clean();

  //
  // The day-per-day receipt
  //
  ob_start();
  $t = new b_table('width=100%');
  //  $t->tro('style="text-align:right"');
  $t->tro();
  $w  = 16;
  $w1 = 30;
  $wf = (int)(100 - $w1 - 2.5*$w); 
  $t->td(x('strong','Description'),'style="text-align:left" width="'.$w1.'%"');
  $t->td(x('strong','Date'),  'style="text-align:right" width="'.$wf.'%"');
  $t->td(x('strong','Debit'), 'style="text-align:right" width="'.$w. '%');
  $t->td(x('strong','Credit'),'style="text-align:right" width="'.$w. '%');
  $t->td(x('strong','Code'),  'style="text-align:right" width="'.($w/2).'%"');
  $t->trc();

  igor_hr($t);
  $total = 0;
  for($date=$f->getValue('_virt_start'); $date<$f->getValue('_virt_end'); $date+=86400){
    $t->tro();
    $t->td('Hébergement B38','style="text-align:left"');
    $t->td(date('d.m.Y',$date),'style="text-align:right"');
    $t->td(sprintf('%5.2F',($pay=$f->getValue('d_price'))),'style="text-align:right"');
    $t->td('0.00','style="text-align:right"');
    $t->td('6','style="text-align:right"');
    $t->trc();
    $total += $pay;
  }
  // add credit
  $t->tr(array('style="text-align:left"'=>'Espèces',
	       date('d.m.Y',$f->getValue('_virt_end')),
	       '0.00',
	       sprintf('%5.2F',$total),
	       ''),
	   'style="text-align:right"');
  igor_hr($t);

  // Total
  $t->tr(array('',
	       x('strong','Total'),
	       x('strong',sprintf('%5.2F CHF',$total)),
	       x('strong',sprintf('%5.2F CHF',$total)),
	       ''),
	 'style="text-align:right"');
  $t->tr(array('',
	       x('strong','Open Balance'),
	       x('strong',sprintf('%5.2F CHF',0)),
	       '',
	       ''),
	   'style="text-align:right"');
  igor_hr($t);

  $t->tr(array('',
	       x('strong','Informative total'),
	       x('strong',sprintf('%5.2F CHF',0)),
	       x('strong',sprintf('%5.2F CHF',0)),
	       ''),
	   'style="text-align:right"');
  $t->tr(array('',
	       x('strong','Informative open balance'),
	       x('strong',sprintf('%5.2F CHF',0)),
	       '',
	       ''),
	   'style="text-align:right"');

  // VAT description
  $t->tr(' ');  $t->tr(' ');  $t->tr();
  $t->tr(array('style="text-align:left"'=>x('strong','VAT Description'),
	       x('strong','Net'),
	       x('strong','Tax'),
	       x('strong','Gross'),
	       x('strong','Code')),
	 'style="text-align:right"');
  igor_hr($t);

  $t->tr(array('style="text-align:left"'=>'Tax Free',
	       sprintf('%5.2F',$total),
	       '0.00',
	       sprintf('%5.2F',$total),
	       '6'),
	 'style="text-align:right"');
  igor_hr($t);

  $t->tr(array('style="text-align:left"'=>x('strong','Total'),
	       x('strong',sprintf('%5.2F',$total)),
	       x('strong','0.00'),
	       x('strong',sprintf('%5.2F',$total)),
	       ''),
	 'style="text-align:right"');
  $t->close();
  print "<br/><br/>Thank you for staying with us. We are looking forward being your host again.";
  $t2 = ob_get_contents();
  ob_end_clean();

  // attention
  ob_start();   
  $t = new b_table('width="100%"');
  $t->tro();
  $t->td('Attn: ','div style="text-align:right" width="50%"');
  $t->td('Dr.', 'div style="text-align:left" width="50%"');
  $t->trc();
  $t->tr(array('',ucwords($f->getValue('d_firstname')).' '.strToUpper($f->getValue('d_lastname'))));
  $t->tr(array('',$f->getValue('d_group')));
  $t3 = ob_get_contents();
  ob_end_clean();

  //
  // output
  //
  $html = join('<br/>',
	       array($t3,
		     '',
		     $t1,
		     '','',
		     $t2,
		     ));
  //print $html;
  $pdf = new TCPDF_Igor(); 
  $pdf->html2pdf('receipt.pdf',$html,'I');

}
