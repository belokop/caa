<?php
function getValue($name,$default=NULL) {
  if    (!is_null($_GET[$name])) $GLOBALS[$name] = $_GET[$name];
  elseif(!is_null($default))     $GLOBALS[$name] = $default;
  else                           $GLOBALS[$name] = "<font color=red>$name?</font>";
}

function printValue($value){
  print "<em>$value</em>";
}

getValue('f_admin');
getValue('f_date',date('Y-m-d'));
getValue('f_kund');
getValue('f_kund_adr','');
getValue('f_kund_pn', '');
getValue('f_kund_nr', '');

getValue('f_hyra1');
getValue('f_hyra2','');
getValue('f_hyra3','');
getValue('f_hyra4','');
getValue('f_adress1');
getValue('f_period1');
getValue('f_period2','');
getValue('f_apris1');
getValue('f_belopp1');

getValue('f_cmt1','');
getValue('f_cmt2','');
getValue('f_cmt3','');

getValue('f_attestansvarig', 'Anne Jifält');
getValue('f_type',           'NORDITA_faktureringsunderlag');
getValue('f_institution',    'VCB');
getValue('f_avdelning',      'UF');
getValue('f_referens',       'ANNE KTHUF');
getValue('f_attestansvarig', 'Anne Jifält');
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html lang="en">
<head>
<meta http-equiv='content-type' content='text/html; charset=utf-8'>
<title><?php print $f_type;?></title>
<base href='http://www.nordita.org/'>
<meta name='robots' content='noindex,nofollow'>
<meta http-equiv='content-style-type'  content='text/css'>
<meta http-equiv='pragma' content='no-cache'>
<meta http-equiv='cache-control' content='no-cache, must-revalidate'>
<meta http-equiv='expires' content='0'>
<meta http-equiv='imagetoolbar' content='no'>
<!--[if !IE]>--><link rel='shortcut icon' href='img/favicon_star.png'><!--<![endif]-->
<script type='text/javascript'>if (self != top) top.location=self.location</script>

<style type='text/css'>
html, body {
  padding 0px;
  margin: 0px;
  background: #fff;
  font-family: Verdana, serif;
}

h1{
  font-size: 16pt;
  margin-bottom: 10px;
}

h2{
  font-size: 10pt;
  margin-bottom: 2px;
}

p{
  font-size: 8pt;
  white-space: nowrap ! important;
}

img {
  margin-top: -10px;
}

/* ---- BOX CLASSES ---- */

.box{
 margin:  0px -1px -1px 0px;
 padding: 1px 4px 1px 8px;
 border:  1px solid #999;
}

.box strong{
  font-size:larger;
}

.box.noborder{
 border: 0px solid transparent ! important;
}

.box.line{
  border-width: 0px 0px 1px 0px ! important;
  border-top-style:   hidden ! important;
  border-left-style:  hidden ! important;
  border-right-style: hidden ! important;
}

.box.top{
  border-width: 1px 1px 0px 1px ! important;
  border-bottom-style: hidden ! important;
}

.box.bottom{
  border-width: 1px 1px 1px 1px ! important;
  border-top-style: dashed;
}

/* ---- VERTICAL SIZE OF BOXES ---- */

/* body height: 297mm */
html, body { }

/*
.v4 { height: 4mm;   }
.v3 { height: 6mm;   }
.v2 { height: 8.5mm; }
.v1 { height: 20mm;  }
*/

.v4 { height: 14px; }
.v3 { height: 18px; }
.v2 { height: 32px; }
.v1 { height: 61px; }

/* ---- HORIZONTAL SIZE OF BOXES ---- */

/* body width: 100% 210mm 645px */
html, body { width: 645px; }

/* pixel width estimated for 645px body width */
.c0  { width: 480px; }
.c1  { width: 628px; }
.c2  { width: 283px; }
.c3  { width: 150px; }
.c4  { width: 131px; }
.c5  { width: 115px; }
.c6  { width:  80px; }
.c8  { width:  72px; }
.c9  { width:  30px; }

/* ---- ALIGNMENT ---- */

.left   { float: left; }
.right  { float: right; }
.center { text-align: center; }

.clear{
 height: 10px;
 clear: both;
}

.newline{
 height: 0px;
 clear: both;
}

</style>

<!--[if IE]><style type='text/css'>

/* MSIE box model problems:
   STANDARD: total width = 2*border + 2*padding + width
   MSIE    : total width = width
*/

/* .box extra height =  4px */

.v4 { height: 18px; }
.v3 { height: 22px; }
.v2 { height: 36px; }
.v1 { height: 65px; }

/* .box extra width  = 14px */

.c0  { width: 494px; }
.c1  { width: 642px; }
.c2  { width: 297px; }
.c4  { width: 145px; }
.c5  { width: 129px; }
.c8  { width:  86px; }
.c9  { width:  44px; }

</style><![endif]-->

<!--[if IE 9]><style type='text/css'></style><![endif]-->
<!--[if IE 8]><style type='text/css'></style><![endif]-->
<!--[if IE 7]><style type='text/css'></style><![endif]-->
<!--[if IE 6]><style type='text/css'></style><![endif]-->
<!--[if IE 5.5]><style type='text/css'></style><![endif]-->
</head>
<body>

<!--
<p style='width:100%; border: 1px solid black;padding:0px; margin:0px;'>100%</p>
<p style='width:210mm;border: 1px solid black;padding:0px; margin:0px;'>210mm</p>
<p style='width:794px;border: 1px solid black;padding:0px; margin:0px;'>794px</p>
<p style='width:600px;border: 1px solid black;padding:0px; margin:0px;'>600px</p>
<p style='width:625px;border: 1px solid black;padding:0px; margin:0px;'>625px</p>
<p style='width:635px;border: 1px solid black;padding:0px; margin:0px;'>635px</p>
<p style='width:640px;border: 1px solid black;padding:0px; margin:0px;'>640px</p>
<p style='width:645px;border: 1px solid black;padding:0px; margin:0px;'>645px</p>
<p style='width:650px;border: 1px solid black;padding:0px; margin:0px;'>650px</p>
<p style='width:675px;border: 1px solid black;padding:0px; margin:0px;'>675px</p>
-->

<p class='center c1 noborder'><img src='img/logo_kth_110.png' alt=''></p>

<h1>Faktureringsunderlag</h1>

<p class='box v2 c2 left noborder'>Institution, avdelning
<br /><strong><?php printValue("$f_institution $f_avdelning");?></strong>
<br /><strong></strong></p>

<p class='box v2 c4 right noborder'>Ver.nr / fakturanr<br /><strong></strong></p>

<p class='box v2 c4 right noborder'>Datum<br /><strong><?php printValue($f_date);?></strong></p>

<div class='clear'></div>

<div class='c2 left'>
<p class='box v2'>Attestansvarig, Vår referen<br
><strong><?php printValue($f_attestansvarig);?></strong></p>
<p class='box v2'>Bokföringstext
<br /><strong></strong></p>
</div>

<div class='c2 right'>
<p class='box v2'>Kund
<br /><strong><?php printValue($f_kund);?></strong></p>
<p class='box v2'>Adress
<br /><strong><?php printValue($f_kund_adr);?></strong></p>
<p class='box v2'>Postadress
<br /><strong><?php printValue($f_kund_pn);?></strong></p>
<p class='box v2'>Kundnr
<br /><strong><?php printValue($f_kund_nr);?></strong></p>
<p class='box v2'>Beställare / referens
<br /><strong><?php printValue($f_referens);?></strong></p>
</div>

<div class='newline'></div>
<h2>Fakturatext</h2>

<p class='box v4 c5 left'>Hyra</p>
<p class='box v4 c5 left'>Adress</p>
<p class='box v4 c3 left'>Period</p>
<p class='box v4 c6 left'>A-pris</p>
<p class='box v4 c5 left'>Belopp</p>

<?php for ($n=1; $n<5; $n++) { ?>
<div class='newline'></div>
<p class='box v3 c5 left'><?php printValue($GLOBALS["f_hyra$n"]);  ?></p>
<p class='box v3 c5 left'><?php printValue($GLOBALS["f_adress$n"]);?></p>
<p class='box v3 c3 left'><?php printValue($GLOBALS["f_period$n"]);?></p>
<p class='box v3 c6 left'><?php printValue($GLOBALS["f_apris$n"]); ?></p>
<p class='box v3 c5 left'><strong><?php printValue($GLOBALS["f_belopp$n"]);?></strong></p>
<?php } ?>
<div class='clear'></div>

<h2>Övriga noteringar</h2>

<p class='box v3 c1 line'><strong><?php printValue($GLOBALS["f_cmt1"]);?></strong></p>
<p class='box v3 c1 line'><strong><?php printValue($GLOBALS["f_cmt2"]);?></strong></p>
<p class='box v3 c1 line'><strong><?php printValue($GLOBALS["f_cmt3"]);?></strong></p>

<div class='clear'></div>

<h2>Kontering</h2>

<p class='box v4 c8 left'>Konto</p>
<p class='box v4 c8 left'>Org.enhet</p>
<p class='box v4 c8 left'>Projekt</p>
<p class='box v4 c8 left'>Resurs</p>
<p class='box v4 c8 left'>Fritt/MP</p>
<p class='box v4 c8 left'>Finansiär</p>
<p class='box v4 c8 left'>Belopp</p>
<p class='box v4 c9 left'>D/K</p>

<?php for ($n=1; $n<5; $n++) { ?>
<div class='newline'></div>
<p class='box v3 c8 left'><strong></strong></p>
<p class='box v3 c8 left'><strong><?php printValue($f_institution);?></strong></p>
<p class='box v3 c8 left'><strong></strong></p>
<p class='box v3 c8 left'><strong></strong></p>
<p class='box v3 c8 left'><strong></strong></p>
<p class='box v3 c8 left'><strong></strong></p>
<p class='box v3 c8 left'><strong></strong></p>
<p class='box v3 c9 left'><strong><?php printValue('K');?></strong></p>
<? } ?>
<div class='clear'></div>
<div class='clear'></div>
<div class='clear'></div>

<p class='box v1 c0 top left'>Fakturering begärd av</p>
<p class='box v1 c4 top left'>Registrerad av</p>

<div class='newline'></div>

<p class='box v3 c0 bottom left'><strong><?php printValue($f_admin);?></strong></p>
<p class='box v3 c4 bottom left'>Signatur, administratör</p>

</body>

</html>
