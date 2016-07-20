<?php

if (function_exists('drupal_get_path')) require_once drupal_get_path('module','ea') . '/'. basename(__FILE__);

foreach(array('tab_prp_see'         => (myOrg_ID ? myOrg_nameS.' ' : '').'Preprints',
              'tab_prp_mng'         => 'Register/Manage preprints',
              'tab_prp_pending'     => 'Manage pending preprints',
              'tab_prp_update'      => 'Update preprints',
              'tab_prp_new'         => 'Register new preprint',
              'tab_prp_adm'         => 'Administer preprints',
              ) as $name=>$descr) APItabs::define_tab($name,$descr);
