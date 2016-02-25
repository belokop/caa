<?php
/*
 * Location of the class "agenda" is
 * <myPear root>/myIndico/agenda.inc
 */

function array_sort($array, $on, $order=SORT_ASC) {
    $new_array = array();
    $sortable_array = array();

    if (count($array) > 0) {
        foreach ($array as $k => $v) {
            if (is_array($v)) {
                foreach ($v as $k2 => $v2) {
                    if ($k2 == $on)           $sortable_array[$k] = $v2;
                }
            } else    $sortable_array[$k] = $v;
        }

        switch ($order) {
        case SORT_ASC:
            asort($sortable_array);
            break;
        case SORT_DESC:
            arsort($sortable_array);
            break;
        }

        foreach ($sortable_array as $k => $v) {
            $new_array[$k] = $array[$k];
        }
    }

    return $new_array;
}

require_once './agenda.inc';

$agenda = new agenda('agenda.albanova.se');
$agenda->debug = True;

// Print a short "how to"
// print $agenda->help();

// Show conferences in category 315
if (0) $reply = $agenda->getCategory(270,
                                     join('&',array("after=2015-08-01",
                                                    "before=2016-09-01",
                                                    "show_reg=all",
                                                    "show_contrib=all")));
$reply = $agenda->getConference(4462,
                                join('&',array(
                                               "show_reg=all",
                                               "show_contrib=all")));

krsort($reply);
foreach($reply as $date=>$data1){
    foreach($data1 as $key=>$data2){
        if (!is_array($data2)) continue;
        switch ($key) {
        case 'chairs':
            // print_r($data2);                                                                                                                                                                                        
            $reply[$date][$key] = array_sort($data2, 'av_lastname', SORT_ASC);
            //      print_r($data1[$key]);                                                                                                                                                                             
            break;
        case 'registrant':
            $reply[$date][$key] = array_sort($data2, 'familyname', SORT_ASC);
            break;
        case 'session':
            break;
        case 'chair':
            break;
        }
    }
}
print_r($reply);
exit;

// Show conferences in category 315 with dates restricted
$reply = $agenda->getCategory(315,"after=2015-08-01");
print_r($reply);

// Show DETAILED conferences info in category 315 with dates restricted
$reply = $agenda->getCategory(315,join('&',array("after=2015-08-01",
                                                 "before=2015-09-01",
                                                 "show_reg=all",
                                                 "show_contrib=all")));
print_r($reply);

