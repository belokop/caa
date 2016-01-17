<?php
/*
 * Location of the class "agenda" is
 * <myPear root>/myIndico/agenda.inc
 */

require_once './agenda.inc';

$agenda = new agenda('agenda.albanova.se');
$agenda->debug = True;

// Print a short "how to"
print $agenda->help();

// Show conferences in category 315
$reply = $agenda->getCategory(315);
print_r($reply);

// Show conferences in category 315 with dates restricted
$reply = $agenda->getCategory(315,"after=2015-08-01");
print_r($reply);

// Show DETAILED conferences info in category 315 with dates restricted
$reply = $agenda->getCategory(315,join('&',array("after=2015-08-01",
                                                 "before=2015-09-01",
                                                 "show_reg=all",
                                                 "show_contrib=all")));
print_r($reply);

