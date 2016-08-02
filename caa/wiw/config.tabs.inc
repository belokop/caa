<?php

if (function_exists('locateAndInclude')) locateAndInclude('APImenu');

foreach(array('tab_wiw'        => 'Who Is Where?',
              'tab_wiw_adm'    => 'Administer',
              'tab_wiw_admTT'  => 'Edit Trip Colors',
              'tab_wiw_myTrips'=> 'My Trips',
              'tab_wiw_sok'    => 'Search Trips',
              'tab_wiw_edit'   => 'Modify Trip',
              'tab_wiw_reg'    => 'Register new trip',
              ) as $name=>$descr) APItabs::define_tab($name,$descr);

