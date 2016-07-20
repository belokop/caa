<?php

if (function_exists('locateAndInclude')) locateAndInclude('APImenu');

foreach(array('tab_ea'              => 'People at '.(defined('myOrg_name' ? myOrg_name : 'here')),
              'tab_ea_home'         => 'Home',
              'tab_ea_home_contacts'=> 'My Contact Persons',
              'tab_ea_a'            => 'Alumni',
              'tab_ea_a_years'      => 'As the years pass...',
              'tab_ea_e'            => 'Staff',
              'tab_ea_e_cv'         => 'CV\'s',
              'tab_ea_e_groups'     => 'Groups',
              'tab_ea_admin'        => 'Administer',
              'tab_ea_admin_rb'     => 'Rebuild EA',
              'tab_ea_admin_dup'    => 'Find duplicate avatars',
              'tab_ea_admin_org'    => 'Organization structure',
              'tab_ea_admin_org_mngr'=> 'Organization managers',
              'tab_ea_admin_org_structure'=> 'Employee types&groups',
              'tab_ea_admin_orgs'         => 'All Organizations',

              'tab_ea_off'               => 'Offices',
              'tab_ea_off_assign'        => 'Allocate offices',
              'tab_ea_off_wipe'          => 'Wipe allocations',
              'tab_ea_off_door_labels'   => 'Print door labels',
              'tab_ea_off_desk_labels'   => 'Print desk labels',
              'tab_ea_off_list'          => 'Who sits where List',
              'tab_ea_off_omatrix'       => 'Who sits where Matrix',
	      ) as $name=>$descr) APItabs::define_tab($name,$descr);
