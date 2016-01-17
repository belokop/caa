<?php
function b_reCAPTCHA($header_message='<h3>Please type the text</h3>'){
  static
    $publickey  = "6LfkwN4SAAAAAB6Un8fcWBcmHRv--p51lf-WTr6e",
    $privatekey = "6LfkwN4SAAAAAJaWG7RrV8Awr9tPPWkMa9xVq-eH",
    $options    = "var RecaptchaOptions = { theme:'clean',lang:'en', };";

  if (!isset($_SESSION)) session_start();

  if (!($reply = @$_SESSION[__FUNCTION__])){  
    require_once(dirname(__FILE__).'/recaptchalib.php');
    
    // Get the response from reCAPTCHA
    if (@$_POST["recaptcha_response_field"]) {
      $resp = recaptcha_check_answer ($privatekey,
				      $_SERVER["REMOTE_ADDR"],
				      $_POST["recaptcha_challenge_field"],
				      $_POST["recaptcha_response_field"]);
      
      if ($resp->is_valid) {
	$reply = $_SESSION[__FUNCTION__] = True;
      } else {
	$error = $resp->error;
      }
    }else{
      $resp  = null;
      $error = null;
    }
    
    if (!isset($reply))  $reply = 
			   x("script type='text/javascript'",$options).
			   x("div style='text-align:-moz-center;'",
			     $header_message.
			     x("form action='' method='post'",
			       recaptcha_get_html($publickey, $error).
			       "<input type='submit' value='submit' />"
			       ));
  }
  return $reply;
}
