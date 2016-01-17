<?php
/*
 * Extend the TCPDF class to create custom Header and Footer
 */

require_once(dirname(__FILE__).'/_tcpdf/config/lang/eng.php');
require_once(dirname(__FILE__).'/_tcpdf/tcpdf.php');

class bTCPDF extends TCPDF {

  public $subHeader = '';
  
  function __construct(){
    
    parent::__construct(PDF_PAGE_ORIENTATION, PDF_UNIT, PDF_PAGE_FORMAT, true, 'UTF-8', false);
    
    // set document information
    $this->SetCreator(PDF_CREATOR);
    $this->SetAuthor('Nicola Asuni');
    $this->SetTitle('TCPDF title');
    $this->SetSubject('TCPDF subject');
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
    // $this->setImageScale(PDF_IMAGE_SCALE_RATIO);

    //set some language-dependent strings
    $this->setLanguageArray($GLOBALS['l']);

    // ---------------------------------------------------------
    
    // set default font sub-setting mode
    $this->setFontSubsetting(true);
  }

  public function Header() {
    // Logo
    $favicon = b_fmt::compact_html(myOrg_favicon,$keepQuotes='pdf');

    // Title
    $this->SetFont('helvetica', 'N', 10);
    $this->Cell(0, 15, ' ', 0, false, 'C', 0, '', 0, false, 'M', 'M');
    if (empty($subHeader)) $subHeader = myOrg_name;
    $html = x('div style="text-align:right;"',
	      $favicon . $this->subHeader)."<hr/>";
    $this->writeHTMLCell($w=0, $h=0, $x=15, $y='', $html, $border=0, $ln=1, $fill=0, $reseth=true, $align='J', $autopadding=true);
  }
  
  public function Footer_ZZZ() {
    // Position at 15 mm from bottom
    $this->SetY(-15);
    // Set the footer font 
    $this->SetFont('helvetica', 'I', 8);
    // Page number
    $this->Cell(0, 10, 'Page '.$this->getAliasNumPage().'/'.$this->getAliasNbPages(), 0, false, 'C', 0, '', 0, false, 'T', 'M');
  }

  /**
   * Output an html code as a pdf file
   */
  function html2pdf($output_file,$html){

    // dejavusans is a UTF-8 Unicode font, if you only need to
    // print standard ASCII chars, you can use core fonts like
    // helvetica or times to reduce the output file size.
    $this->SetFont('dejavusans', '', 8, '', true);

    // Add a page
    // This method has several options, check the source code documentation for more information.
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

    // ??? kills the UTF8 encoding.
    // The Non-ACII characters are replaced by the HTML analog &xxx;
    // but the conversion table seem to be wrong. 
    // $html = $this->fixHTMLCode($html);

   $this->writeHTMLCell($w=0, $h=0, $x='', $y='', 
			 $html,
			 $border=0, $ln=1, $fill=0, $reseth=true, $align='', $autopadding=true);
    if (cnf_show_pdf)  if (!cnf_CLI && ($GLOBALS['jjjyyy']++ < 3))    print 
							     "\n------------------------------------------------------html2pdf<br/>\n".
							     str_replace("><",">\n<",$html).
							     "\n<br>------------------------------------------------------html2pdf<br>\n".
							     "\n<br>------------------------------------------------------html2pdf\n"
							     ;
    // Close and output PDF document
      $this->Output($output_file, 'F');
  }    
}
