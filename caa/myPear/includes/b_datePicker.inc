<?php
/*
 * Wrapper around the jQuery datepicker
 */
function bDatePicker(){
  static $class = Null;
  if ($class === Null) $class = new b_datePicker();
  return $class;
}

class b_datePicker{
  
  var $size = 10;
  var $message = '';
  var $messageDef = 'select date';
  var $imposeConversion; // if set, the value is considered as integer
  var $imposeConversionDef = False;

  var $icon1 = '';
  var $icon2 = '';

  function __construct(){
    $this->reset();
  }

  function reset(){
    $this->message = $this->messageDef;
    $this->imposeConversion = $this->imposeConversionDef;
  }
  
  /*
   * Ask for a pair of dates - from / to
   *
   * @param $fr  - field name 'from'
   * @param $vfr - initial value 'from'
   * @param $to  - field name 'to'
   * @param $vto - initial value 'to'
   * @param $returnResult - (bool)  flag saying to print the <input> tag or to return it
   *                        (array) 
   * @param $label - (string) separator between the input fields 
   */
  function get2($fr,$vfr,$to,$vto,$returnResult=False){
    $vfr = $this->convert($vfr);
    $vto = $this->convert($vto);
    if (cnf_show_dp) b_debug::xxx("$vfr -> $vto");

    bJS()->add_js_inline("$(function() {
      var dates = $('#$fr, #$to').datepicker({
	changeMonth: false,
	numberOfMonths: 1,
        dateFormat:'".CONST_dp_format."',
	showAnim:'slideDown',
	firstDay:1,
	onSelect: function(selectedDate) {
	  var option   = this.id == '$fr' ? 'minDate' : 'maxDate',
	      instance = $(this).data('datepicker'),
	      date     = $.datepicker.parseDate(instance.settings.dateFormat || $.datepicker._defaults.dateFormat,
			   		        selectedDate, 
                                                instance.settings);
	  dates.not(this).datepicker('option', option, date);
	}
      });
    });", True);
    $fr = "<input id='$fr' type='text' name='$fr' value='$vfr' size=10 />";
    $to = "<input id='$to' type='text' name='$to' value='$vto' size=10 />";
    $reply = (empty($this->icon1)
	      ? array($fr, " -&gt; ", $to)
	      : array($this->icon1,$fr,'&nbsp;',$this->icon2,$to));
    if ($returnResult) return $reply;
    else               print  join("&nbsp;\n",$reply);
  }


  /*
   * Ask for a date
   */
  function get($fName,$value='',$returnResult=False){
    $value = $this->convert($value);
    $opts = '';
    if (is_array($returnResult)){
      $min_max = $returnResult;
      $returnResult = False;
      if (!empty($min_max)){
	list($mnY,$mnM,$mnD) = b_time::parse($min_max[0]); $mnM--;
	list($mxY,$mxM,$mxD) = b_time::parse($min_max[1]); $mxM--;
	$opts = ", minDate: new Date($mnY,$mnM,$mnD), maxDate:new Date($mxY,$mxM,$mxD)";
      }
    }
    
    bJS()->add_js_inline("$('#date$fName').datepicker({dateFormat:'".CONST_dp_format."',showAnim:'slideDown',firstDay:1$opts});",True);
    $reply = "<input type='text' id='date$fName' name='$fName' value='$value' />";
    if ($returnResult) return $reply;
    else               print  $reply;
  }
  
  // magic passes - converting integer <-> string. See also bForm::getValue
  private function convert($value){

    if (empty($value))           $value = time();
    elseif ($value == 'present') $value = CONST_eternity;

    switch(CONST_dp_format){
    case 'yy-mm-dd':   
      // everything which is 25 years from now is the eternity...
      $reply = b_time::txt2unix($value);
      if ($reply > time() + 25 * 366*24*3600)          $reply = CONST_eternity;
      elseif($reply<1900 || $reply>CONST_eternity_year)$reply = date('Y-m-d',$reply);
      break;
    default:
      myPear::ERROR("Please upgrade ".__METHOD__." to deal with '".CONST_dp_format."' datepicker format");
      $reply = Null;
    }
    if (cnf_show_dp) b_debug::xxx($reply);
    return $reply;
  }

  function __toString() { return get_class($this); }
}
