<?php

if (function_exists('locateAndInclude')) locateAndInclude('APImenu');

foreach(array('tab_jam_administer'      =>  'Administer JAM',
              'tab_jam_applicants'      =>  'Applicants',
              'tab_jam_rate_referees'   =>  'See referees rating',
              'tab_jam_quickStatus'     =>  'Quick overview',
              'tab_jam_admins'          =>  'Position manager',
              'tab_jam_secretaries'     =>  'Position secretary',
              'tab_jam_home'            =>  'My contact info',
              'tab_jam_myRecLetters'    =>  'My letters of recommendation',
              'tab_jam_myApplications'  =>  'My applications',
              'tab_jam_search'          =>  'Search',
              'tab_jam_whoIsWho'        =>  'Who is who',
              'tab_jam_sc'              =>  'Search & Selection Committee',
              'tab_jam_positions'       =>  'Setup positions',
              'tab_jam_managers'        =>  'Position managers',
              'tab_jam_toolbox'         =>  'Administer JAM',
              'tab_rfields'             =>  'Setup Research fields',
              'tab_reqdocs'             =>  'Setup Required documents',
              ) as $name=>$descr) APItabs::define_tab($name,$descr);
