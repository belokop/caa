<?php

/*
 * Menu items
 */

// Load dependencies
if (function_exists('drupal_get_path')) require_once drupal_get_path('module','ea') . '/'. basename(__FILE__);
elseif (file_exists($f=str_replace('/vm/','/ea/',__FILE__))) require_once $f;
		    
$VM_menu=array('tab_vm_home'              => 'Home',
	       
               'tab_vm_reg'               => 'My visit',
               
               'tab_vm_end'               => 'Events to endorse',
               'tab_vm_ogz'               => 'Events I organize',
               'tab_vm_end_vlist'         => 'A-Z applicant lists',
               'tab_vm_ogz_vlist'         => 'Approve & Accommodate applicants',
               'tab_vm_end_budget'        => 'See/Approve budget',
               'tab_vm_ogz_budget'        => 'Budget estimation',
               'tab_vm_end_mails'         => 'Mails exchange with applicants',
               'tab_vm_ogz_mails'         => 'Mails exchange with applicants',
               'tab_vm_end_vmatrix'       => 'Who arrives when',
               'tab_vm_ogz_vmatrix'       => 'Who arrives when',
               'tab_vm_vis_vmatrix'       => 'Who arrives when',
               'tab_vm_end_amatrix'       => 'Who lives where',
               'tab_vm_ogz_amatrix'       => 'Who lives where',
               'tab_vm_end_omatrix'       => 'Who sits where Matrix',
               'tab_vm_ogz_omatrix'       => 'Who sits where Matrix',
               'tab_vm_ogz_se_setup'      => 'Social events',
               'tab_vm_org_photos'        => 'Photos of attenders',
               'tab_vm_org_dup'           => 'Duplicate applications',
               
               'tab_vm_adm'               => 'Administer',
               'tab_vm_adm_projects'      => 'Financial projects',
               'tab_vm_adm_unitOrganizers'=> 'VM organizers',
               'tab_vm_adm_acl'           => 'ACL',
               'tab_vm_adm_agendaList'    => 'Edit agenda list',
               'tab_vm_adm_fixClashes'    => 'Fix clashes',
               'tab_vm_adm_sync'          => 'Agenda sync',
               'tab_vm_adm_dup'           => 'Duplicate avatars',
               'tab_vm_adm_se_setup'      => myOrg_name.' social events',
               'tab_vm_adm_sc_setup'      => myOrg_name.' reimbursement rates',
               
               'tab_vm_ap'                => 'Accommodation', 
               'tab_vm_ap_assign'         => 'Auto accommodate homeless',
               'tab_vm_ap_types'          => 'Accommodation options',
               'tab_vm_ap_add'            => 'Add apartment',
               'tab_vm_ap_aps'            => 'Rooms',
               'tab_vm_ap_huts'           => 'Buildings',
               'tab_vm_ap_sok'            => 'Search',
               'tab_vm_ap_usage'          => 'Apartments usage',

               'tab_vm_events'            => 'Events',
               'tab_vm_events_add'        => 'Add event',
               'tab_vm_events_see'        => 'Events',
               
               'tab_vm_vis'               => 'Visits within events',
               'tab_vm_vis_photos'        => 'Print photos',
               'tab_vm_vis_badges'        => 'Print badges',
               'tab_vm_vis_sok'           => 'Search',
               'tab_vm_vis_welcome'       => 'Welcome visitor',
               'tab_vm_vis_list'          => 'List of attenders',
               'tab_vm_vis_budget'        => 'Budget estimation',
               'tab_vm_vis_mails'         => 'Mails exchange with attenders',
               'tab_vm_vis_export'        => 'Export tenants Excel files',
               
               'tab_vm_guests'            => 'Visits outside '.myOrg_nameS.' events',
               'tab_vm_guests_to_approve' => 'Resolve pending approvals',
               'tab_vm_guests_budget'     => 'People I invite',
               'tab_vm_guests_budgetBP'   => 'Visits budget summary (by projects)',
               'tab_vm_guests_addVisit'   => 'Register new guest',
               
               'tab_vm_vmatrix'           => 'Visits',
               );

// Define all the tabs/menus used in VM module
foreach($VM_menu as $name=>$descr)  APItabs::define_tab($name,$descr);
