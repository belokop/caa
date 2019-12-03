<?php

// Optional "Employees & Alumni" module
define('EA_MODULE', 'ea');
define('employment_record','Employment record');

/*
 * Shortcuts to the EA statuses.
 * Not to be edited.
 * Philosophy:
 * - a person in EA module is might be 
 *   = EA_role_employee (members of the personnel)
 *   = EA_role_alumnus (alumnus)
 * - employee is a member of an "employee type" group, se bList_orgEmpTypes class, which is normally
 *   = Administration (optional)
 *   = Visitors (normally long term ones, optional)
 *   = Other user defined groups like "faculty", "students", etc.
 */
define('EA_role_alumnus' ,   'a');
define('EA_role_employee',   'e');
define('EA_role_lt_visitor', 'v');

/*
 * Access level to the EA record is either public OR private.
 * In any organization there administrators
 */
define('EA_acl_public', 'p');
define('EA_group_adm','adm');

/*
 * Define the basic built-in Employers & Alumni roles in the organization.
 * - "EA_title_employee" is automatically (by a cron job or an explicit call to "rebuild EA") set if the current date is within the employment period,
 *                       and is automatically replaced by "EA_title_alumnus" when the employment period expired
 *
 * See also classes bForm_Avatar_ea & bUnit_ea_members
 */
define('EA_title_alumnus' ,   'Alumnus');
define('EA_title_employee',   'Employee');
define('EA_title_lt_visitor', 'Visitor');

/*
 * Define the ranks, the basis for the Access Control.
 * By default the system knows about the superuser, secretary, administrator and authenticated person.
 * The extra roles/ranks are might be defined by APIs
 */
define('RANK__ea_org_manager',  8);
define('RANK__ea_admin'      ,  6); // Employees & Alumni
define('RANK__ea_member'     ,  4); // Employees & Alumni
define('RANK__ea_contactPerson',1);

/*
 * Define the EA roles.
 *  i     - role favicon
 *  d     - role description for the role favicon
 *  class - (optional) bUnit class which handles this unit.
 *          If present, a bUnit class is created for this role  
 */
myPear_set_roles('roles',
		 array(EA_MODULE    =>array(RANK__ea_org_manager=> array('class' => 'bUnit',
									 'i'=>'bb-phone',
									 'd'=>'Organization Manager'),
                                            RANK__ea_admin      => array('class' =>'bUnit_ea_admins',
									 'i'=>'bb-man_folder',
									 'd'=>'E&A administrator'),
                                            RANK__ea_member     => array('class' =>'bUnit_ea_members',
									 'i'=> 'bb-man',
									 'd'=> 'Staff member'),
                                            RANK__ea_contactPerson=> array('class' =>'bUnit_ea_contacts',
                                                                           'i'=> 'bb-man',
                                                                           'd'=> 'Contact person'),
                                            
                                            )));


/*
 * Affiliation with the organization
 */
myPear_set_roles('affiliation',          
                 array(EA_role_employee    => array('title' => EA_title_employee,
                                                    'group' => 'Staff',
                                                    'mode'  => 'RO',        // Always present 
                                                    'page'  => 1,           // Starts web page
                                                    'active'=> 1),          // active by default
                       EA_role_alumnus     => array('title' => EA_title_alumnus,
                                                    'group' => 'Alumni',
                                                    'page'  => 1,           // Starts web page
                                                    'active'=> 1),          // active by default
		       EA_role_lt_visitor  => array('title' => EA_title_lt_visitor,
                                                    'group' => 'Visitors',
                                                    'active'=> 0),          // NOT active by default
		       ));


