<?php
class pdf_form{
  private static $fdf_header = "%FDF-1.2
%âãÏÓ
1 0 obj 
<< /FDF 
<< /Fields [
";
  private static $fdf_footer = "
]
>>
>>
endobj 
trailer
<< /Root 1 0 R >>
%%EOF
";

  /*
   * Create a pdf file from the template, optionally filling the form fields
   *
   * @param $data       array  - the data fields
   * @param $pdf_tpl    string - the pdf file name (aka template)
   * @param $pdf_result string - the resulting pdf file path
   */
  public static function fill($data=array(),$pdf_tpl,$pdf_result=''){
    $fdf_file = sys_get_temp_dir() . '/' . mt_rand() . '.fdf';
    if (empty($pdf_result)) $pdf_result = sys_get_temp_dir() . '/' . mt_rand() . '.pdf';

    // Create fdf file with the data fields
    $fdf_body[] = self::$fdf_header;
    foreach ($data as $t=>$v) $fdf_body[] = "<< /V (".utf8_decode($v).") /T ($t) >>";
    $fdf_body[] = self::$fdf_footer;
    file_put_contents($fdf_file,join("\n",$fdf_body));

    // Inject data fields to the pdf file
    if (!file_exists($pdf_tpl)) $pdf_tpl = vm_root . 'templates/' . $pdf_tpl;
    b_os::exec(b_os::path()."pdftk $pdf_tpl fill_form $fdf_file output $pdf_result");

    //    unlink($fdf_file);
    return $pdf_result;
  }

  public static function x(){
    foreach(nordea() as $code=>$a) $fdf[$a['t']] = $a['d'];
    print pdf_form::fill($fdf,vm_root.'templates/nordea.pdf');
  }
}
