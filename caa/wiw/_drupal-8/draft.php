<?php
/*
wiw.content:
  path: '/wiw'
  defaults:
    _controller: '\Drupal\wiw\Controller\WiwController::content'
    _title: 'Who Is Where'
  requirements:
    _permission: 'access content'
*/

define('prf','http://127.0.0.1/');
define('wp_root','/Users/yb/Sites/_drupal8');

build_menu(test_tree());

function build_menu($menu_tree){
  
  $ids = array();
  foreach($menu_tree as $url=>$item){

    if (count(explode('/',$url)) == 1){
      $nav_menu_title = sprintf("%s menu",strToUpper($url));
      $item_title  = $item['title'];
      $module = $url;
      $controller = ucfirst($module).'Controller';
      print "$module.content:";
    }else{
      $item_title  = $item['page arguments'][0];
    }
    
    
    print "
  path: $url
  defaults:
    _controller: \\Drupal\\$module\\Controller\\$controller::content
    _title: $item_title
  requirements:
    _permission: 'access content'
";
    continue;


    $page_id = insert_post($item_title,$item_title.' dummy page');
    
    // Get the parent ID
    $url_parent = explode('/',$url);
    array_pop($url_parent);
    $id_parent = (int)@$ids[implode('/',$url_parent)];
    print "<br>$url: id_parent = $id_parent";
    
      // Hook the created to a menu item
      $args = array ('menu-item-object-id' => $page_id,
		     'menu-item-db-id' => 0,
		     'menu-item-object' => 'page',
		     'menu-item-parent-id' => $id_parent,
		     'menu-item-type' => 'post_type',
		     'menu-item-title' => $item_title,
		     'menu-item-url' => prf . "?preview=true&page_id=".$page_id,
		     'menu-item-target' => '',
		     'menu-item-classes' => '',
		     'menu-item-xfn' => '',
		     'menu-item-description' => '', 
		     );
      an_traceBack(True);
      $item_ids = wp_save_nav_menu_items(0,array($args));
      if (is_wp_error($item_ids)){
	printf("Can't build %d - '%s', abandone <br>",$menu_object->term_id,$menu_object->name);
	return False;
      }

      // Save the menu item ID
      $item_id = $ids[$url] = $item_ids[0];
      print "<br>wp_save_nav_menu_items: item_id = $item_id";

      // Mimic the ajax dialog...
      foreach($args as $k=>$v) $_POST[$k][$item_id] = $v;

      $menu_data[] = array('url' => $url,
		       'menu-item-title'      => $item_title,
		       'menu-item-url'        => prf . "?preview=true&page_id=".$page_id,
		       'menu-item-type'       => 'post_type',
		       'menu-item-object-id'  => $page_id,
		       'menu-item-db-id'      => 0,
		       'menu-item-object'     => 'page',
		       );
  }
}

function test_tree(){
  return array(
	       'wiw' => array(
			      'menu_name' => 'navigation',
			      'page callback' => '_wiw_output',
			      'page arguments' => array('wiw'),
			      'title' => 'Who Is Where?',
			      'access callback' => 'myPear__menu_access',
			      'access arguments' => array('wiw'),
			      ),
	       'wiw/3969012486' => array(
					 'page callback' => '_wiw_output',
					 'title callback' => '_wiw_title_callback',
					 'access callback' => '_wiw_access_callback',
					 'page arguments' => array('Administer'),
					 'title arguments' => array('Administer'),
					 'access arguments' => array('Administer'),
					   'weight' => 0,
					 'type' => '6',
					 'menu_name' => 'navigation',
					 ),
	       'wiw/3969012486/3977248320' => array(
						    'page callback' => '_wiw_output',
						    'title callback' => '_wiw_title_callback',
						    'access callback' => '_wiw_access_callback',
						    'weight' => '30',
						    'page arguments' => array('Edit Trip Colors'),
						    'title arguments' => array(
									       'Edit Trip Colors',
									       ),
						    'access arguments' => array(
										  'Edit Trip Colors',
										),
						    'menu_name' => 'navigation',
						    ),
		 'wiw/3183671515' => array(
					   'page callback' => '_wiw_output',
					   'title callback' => '_wiw_title_callback',
					   'access callback' => '_wiw_access_callback',
					   'page arguments' => array(
								     'Search Trips',
								     ),
					   'title arguments' => array
					   (
					    'Search Trips',
					    ),
					   'access arguments' => array
					   (
					    'Search Trips',
					    ),
					   'weight' => '0',
					   'type' => '6',
					   'menu_name' => 'navigation',
					   ),
		 'wiw/74854488' => array(
					 'page callback' => '_wiw_output',
					 'title callback' => '_wiw_title_callback',
					 'access callback' => '_wiw_access_callback',
					 'page arguments' => array
					 (
					  'My Trips',
					  ),
					 'title arguments' => array
					 (
					  'My Trips',
					  ),
					 'access arguments' => array
					 (
					  'My Trips',
					  ),
					 'weight' => '-10',
					 'type' => '6',
					 'menu_name' => 'navigation',
					 ),
		 'wiw/47143390' => array(
					 'page callback' => '_wiw_output',
					 'title callback' => '_wiw_title_callback',
					 'access callback' => '_wiw_access_callback',
			   'page arguments' => array
			   (
			    'Register new trip',
			    ),
			   'title arguments' => array
			   (
			    'Register new trip',
			    ),
			   'access arguments' => array
					 (
					  'Register new trip',
					  ),
					 'weight' => '-20',
					 'type' => '6',
					 'menu_name' => 'navigation',
					 ),
		 );
    
  }
  
