<?php
/*
 *  The sendMailsInQueue method, since version 1.2.3, has callback support.
 * This may be utilised for report generation and postprocessing.
 * 
 * The callback function is called before the relevant entry is deleted from the mail_queue table 
 * in the database so if necessary you could add extra fields to it for inserting into your log/report table. 
 * The function should accept only one parameter - an associative array of values; 'id', 'queued_as' and 'greeting'.
 * 
 * You'll need to use recent releases of the PEAR::Mail (version 1.2.0b3 or higher) and 
 * PEAR::Net_SMTP (version 1.3.3 or higher) packages to be able to retrieve the esmtp id and greeting details 
 * if you need them for your reports. 
 * Also, if you want to decode the body of the email and store that for your report you'll need to install the 
 * PEAR::Mail_mimeDecode package.
 * 
 * Provide the callback function like so:
 * $returned = $mail_queue->sendMailsInQueue(MAILQUEUE_ALL,
 *                                           MAILQUEUE_START,  
 *                                           MAILQUEUE_TRY,
 *                                           'mailqueue_callback');
 */

function mailqueue_callback($args) {
    $row = get_mail_queue_row($args['id']);
    $headers = unserialize($row['headers']);
    $subject = $headers['Subject'];
    $body = unserialize($row['body']);
    
    $mail_headers = '';
    foreach($headers as $key=>$value) {
        $mail_headers .= "$key:$value\n";
    }
    $mail = $mail_headers . "\n" . $body;
    $decoder = new Mail_mimeDecode($mail);
    $decoded = $decoder->decode(array(
                                      'include_bodies' => TRUE,
                                      'decode_bodies'  => TRUE,
                                      'decode_headers'  => TRUE,
                                      ));
    $body = $decoded->body;
    
    $esmtp_id = $args['queued_as'];
    if (isset($args['greeting'])) {
        $greeting = $args['greeting'];
        $greets = explode(' ', $greeting);
        $server = $greets[0];
    } else {
        $server = 'localhost';
    }
    
    insert_to_log(compact('server', 'esmtp_id', 'subject', 'body'));
}
