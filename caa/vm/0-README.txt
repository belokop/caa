Changes log                                                                                                                                                             
                                                                                                                                                                                 
2017-04-07, Version 5.12.rc7,
                                                                                                                                                                                 
2017-04-04, Version 5.12.rc6,
                                                                                                                                                                                 
2017-04-04, Version 5.12.rc5,

  * New procedure for inviting the guest researches (invitations are accepted by default,
    the group leaders revise them if needed) 

  * New display of the guest reseachers - based on the hosting group rather then finansial project (to pay from)                                                                                                                                                                                
2017-03-06, Version 5.12.rc3,
                                                                                                                                                                                 
2017-03-05, Version 5.87,
                                                                                                                                                                                 
2017-02-26, Version 5.86,
                                                                                                                                                                                 
2017-02-26, Version 5.85,
                                                                                                                                                                                 
2017-02-21, Version 5.84,
                                                                                                                                                                                 
2017-02-19, Version 5.83,
                                                                                                                                                                                 
2017-02-15, Version 5.82,
                                                                                                                                                                                 
2017-02-15, Version 5.81,
                                                                                                                                                                                 
2017-02-12, Version 5.80,
A       vm/_drupal-8/src
A       vm/_drupal-8/src/components
A       vm/_drupal-8/src/components/RouteBuilder.php

2017-02-11, Version 5.80, Update myPear (menu system), VM (accommodation options)

2017-02-10, Version 5.79
  * Redesign/regroup Menu, introduce item dividers
  * Redesign bList_vm_accommodationOptions class, keep rent prices for the each program 
    locally.
  * Remove "request only" accommodation optional
  * Redesign "break in the visit" accommodation optional

2015-04-19, Version 5.78

2015-04-05, Version 5.70

2015-04-03, Version 5.68

2015-04-01, Version 5.67

2015-03-16, Version 5.66
  * Transactions & InnoDB

2015-03-14, Version 5.65
  * Introducing database transactions

2015-03-10, Version 5.64

2015-03-01, Version 5.63, bug fixing, "easy approve"
A       vm/includes/bList/vm/reimbursementRates.inc
D       vm/includes/bList/vm/scholarshipRates.inc

2015-02-28, Version 5.62

2015-02-27, Version 5.61

2015-02-26, Version 5.60

2015-02-26, Version 5.59
  * Fix budget estimation

2015-02-25, Version 5.58

2015-02-23, Version 5.57

2015-02-22, Version 5.56
  * Still scholarships...

2015-02-22, Version 5.55
  * Still scholarships...

2015-02-22, Version 5.54
  * Still scholarships...

2015-02-22, Version 5.53
  * Still scholarships...

2015-02-16, Version 5.52

2015-02-15, Version 5.51
  * Make "request only" accommodation optional, not active by default.

2015-02-14, Version 5.50
  * Introducing class b_vars

2015-02-14, Version 5.49
  * Further scholarship integration

2015-02-13, Version 5.48
  * Integrate visit policies to bList_vm_reimbursementRates

2015-02-11, Version 5.47
  *  Fix VM budget reading problem

2015-02-11, Version 5.46

2015-02-11, Version 5.45

2015-02-11, Version 5.44

2015-02-11, Version 5.43

2015-02-10, Version 5.42

2015-02-10, Version 5.41

2015-02-10, Version 5.40

2015-02-10, Version 5.39

2015-02-10, Version 5.38

2015-02-10, Version 5.37
  * Improve bList usage

2015-02-08, Version 5.36
  * Optimizing the performance

2015-02-08, Version 5.35
  * Optimizing the performance

2015-02-08, Version 5.34
  * Optimizing the performance

2015-02-08, Version 5.33
  * Optimizing the performance

2015-02-08, Version 5.32
  * Optimizing the performance

2015-02-05, Version 5.31

2015-02-05, Version 5.30

2015-02-05, Version 5.29

2015-02-04, Version 5.28

2015-02-04, Version 5.27

2015-02-04, Version 5.26

2015-02-04, Version 5.25

2015-02-02, Version 5.24, VM fixes

2015-02-02, Version 5.23

2015-02-01, Version 5.22
  * Workaround - problem with getValue for the list members

2015-01-30, Version 5.21
  * VM bugs fixes

2015-01-29, Version 5.20

2015-01-28, Version 5.19

2015-01-26, Version 5.18
  * clean SQL indices

2015-01-25, Version 5.17
  * production release 2015-01-25

2015-01-24, Version 5.16
  * minor cleanup

2015-01-24, Version 5.15
  * minor cleanup

2015-01-24, Version 5.14
  * minor bugs fixing

2015-01-24, Version 5.13
  * fix Avatar search

2015-01-23, Version 5.12

2015-01-23, Version 5.11

2015-01-23, Version 5.10
  * fix bList options packing

2015-01-20, Version 5.9
  * Fix strict MySQL warnings & avatar merging problems

2015-01-16, Version 5.8
  * code cleanup

2015-01-16, Version 5.7, fix accommodation selection
A       vm/includes/vm_editAccommodation.inc
D       vm/includes/vm_editVisit.inc

2015-01-14, Version 5.6
  * Fix Emp-Records order

2014-03-01 4.3.1
  * New class bForm_vm_Hut
  * New class bList_vm_hutCodes

2012-12-20 4.2
  * Milestone release

2012-11-03 4.1.3
  * Bug fixes in "sending endorser mail" & "has right to" 

2012-08-30 4.1.2
  * Bug fix in function "mayBook"

2012-08-30 4.1.1
  * Add clean-up of the cotenants list. This is a workaround, should find the way users damage the database...

2012-07-01 4.1
  * Introduce detection of the clashing visit requests

2012-07-01 4.0.26
  * many clean-up since 4.0.23
  * introduce aggressive cache in bList classes
  * bList_vm_socialEvents (and some other bList classes) are not auto-populated, but wait for the explicit command.

2012-05-22 4.0.23
  * Set budget on the week basis rather then on the day basis

2012-05-20 4.0.23
  * setup mail reminders for the event organizers

2012-05-15 4.0.22
  * introduce RANK_vm_manager

2012-05-13 4.0.21

2012-05-11 4.0.20
  * Improve event endorsing, send mail to organizers

2012-05-08 4.0.19
  * Add sending bulk informational e-mails

2012-05-05 4.0.17
  * Add "organizer button" to the avatar roles

2012-05-05 4.0.16
  * Add "print reimbursement forms" to the dialogs

2012-04-30 4.0.14
  * "Almost final" version, practically ready for the release.
  * Re-arrange the menu, introduce the Endorser sub-menu

2012-04-24 4.0.13
  * Fix 'mayUpdate()', introduce '$visit->isHost()'

2012-04-22 4.0.11
  * Fix bug - clean cotenants when  lease is deleted

2012-04-21 4.0.10
  * Rewrite the accommodation dialog after fixing bug/feature in myPear
    concerning complicated bForm dialog with embedded forms

2012-04-19 4.0.9
  * Split may_accommodate => may_boo + may_boo_biz, not to be tighten to "selected event"

2012-04-18 4.0.8
  * introduce auto-generated policies

2012-04-17 4.0.7
  * getTravel bug fixed

2012-04-15 4.0.6
  * Bug fixing
  * Split mail exchange dialog, make it a dedicated page

2012-04-07 4.0.4
  * Clean the residential address, introduce the ZIP code
  * Introduce "early information mail" to the attenders.
  * Make the accept/deny mail automatically sent when the event is endorsed and/or
    when the tenants list is exported

2012-04-05 4.0.3

2012-04-02 4.0.2
  * Reimbursement claims are sent to the attenders pre-filled with the available information

2012-04-01 version 4.0-beta, major update

2012-03-06 version 3.5-m4 (design fixes release)
  * Introduce bList_vm_cotenants list (editor for the shared apartments)

2012-02-06 version 3.5-m3 (design fixes release)
  * Improve bList / bUnit design

2011-11-03 version 3.5-m2 (design fixes release)
  * Improve VM_E_endorsed treatment, provide the "approve" button

2011-10-30 version 3.5-m1 (design fixes release)
  * Redesign the event organizers, make it "per event" rather then global.
    The unit membership is restricted to be unique, i.e. av_id can't be twice in the unit

2011-10-12 version 3.4-m3 (milestone release, done under pressure)
  * Improved automated "welcome message" to the event attenders
  * Redesign Search class, make it pluggable to the "general search"

2011-10-10 version 3.4-m1 (milestone release)
  * Introduce automated "welcome message" to the event attenders
  * Introduce personal pages for the attenders
  * Simplify the APIaccess classes, make getRank non-static
  * remove unit 'observers'

2011-09-24 version 3.3-m2 (milestone release)
  * rewrite/simplify bList editing procedure

2011-09-21 version 3.3-m1 (production milestone release)
  * add class bList_vm_reimbursementRates
  * add list of photos for the event attenders
  * agenda_vm: fix leases when OA toggles

2011-09-12 version 3.2-m1 (milestone release)
  * Optimize the budget pages for event organizers, let them accommodation rights

2011-09-10 version 3.1-m4 (milestone release)
  * Change the budget pages for event organizers according to new rules
    (новая метла по новому метет...):
   - introduce class bList_vm_budgetSource
   - modify  class bHolder_vm_Visitors
  * Add export of tenants to an Excel file

2011-06-13 version 3.1-m3 (milestone release)
  * Introduce "event outside the VM scope"

2011-06-06 version 3.1-m2 (milestone release)
  * Prepare the code for the employment list

2011-06-01 version 3.1-m1 (milestone release)
  * starting vm-3.1 with the registrant portal
 
2011-05-11 version 3.0-rc6
  * add class bList_vm_socialEventRates, hence rewrite bList_vm_socialEvents

2011-04-08 version 3.0-rc4
  * attempt (apparently successful) of a production release

2011-04-08 version 3.0-rc3
  * add auto-filling scholarship forms

2011-03-20 version 3.0-rc2
  * add bList_vm_socialEvents class to manage the happenings of the event
    (reception, lunch, etc.)

2011-03-15 version 3.0-rc1
  * add event organizer tools (accept / reject applicant)

2011-02-15 version 3.0-alpha
  * Change structure, introduce 'buildings' with 'apartments' instead of flat 'apartments'

2011-02-07 version 2.20
  * create class bMailer_vm with edit-able templates
  * introduce new roles
    - event organizer (approval of the attenders & other things)
    - event attender

2010-10-10 version 2.12
  * module is baptized VM - Visitors Manager

2010-Autumn, maintenance/bug fixing releases
 - 2.11.8 fix bug in bUnit::isWritable()
 - 2.11.7 switch to a generalized cc class for the country codes
 - 2.11.5 fix bug when photo updates were not visible
 - 2.11.4 fix bug in saving photos URL in the database
 - 2.11.3 fix bug in bForm_vm_Visit (static visitType...)

2010-08-26 version 2.11
 - New module "budget".

2010-06-18 version 2.8
 - introduce object 'lease', support for the accommodation change
   during the visit.

2010-06-10 version 2.7
 - menu rearranged to have on the top 5 main modules:
   * administer (clashing entries, merge avatars, db maintenance, etc.)
   * visitors
   * apartments (VM)
   * offices
   * finance reports
 - add blocks:
   * print badges
   * print office labels

2010-06-02 version 2.6
 - New auto-allocation of the offices, using the "desk allocation", which
   allows to keep the Set Cover algorithm as it is.

2010-05-31 version 2.5
 - New module "offices".
   Automatic office allocation via bCover algorithm is still dummy
   The latter should be upgraded to handle the "room capacity".

