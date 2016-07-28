<?php
error_reporting(E_ALL); 
session_start();

if (!function_exists('t')){
  function t($text){
    return $text;
  }
}

$root = dirname(__FILE__);
$cwd  = explode('/',str_replace("$root/",'',getcwd()));
$area = array_shift($cwd);
$module = str_replace(array('preprints'),array('prp'),$area);

$_GET['q'] = $_REQUEST['q'] = $module;
$_GET['org'] = 'nordita';
if ($_GET['org'] == 'nordita'){
  define('myOrg_ID',40090);
  define('myOrg_favicon','Nordita ');
  define('myOrg_name','Nordita');
  define('myOrg_nameS','Nordita');
  define('myOrg_domain','nordita.org');
  define('myOrg_code','nordita');
  define('myOrg_theme','nordita');
}else{
  define('myOrg_favicon','AlbaNova University Center ');
  define('myOrg_ID',52603);
  define('myOrg_name','AlbaNova University Center');
  define('myOrg_nameS','AlbaNova');
  define('myOrg_domain','albanova.se');
  define('myOrg_code','an');
  define('myOrg_theme','an');
}

print "... Build for module '$module'\n";

foreach(array(
	      "$root/myPear/config.inc",
	      "$root/myPear/includes/drupal8_compat.inc",
	      "$root/myPear/includes/APImenu.inc",
	      "$root/$area/config.inc",
	      "$root/$area/config.tabs.inc",
	      ) as $f){
  if (empty($argv[1])) print "... loading $f";
  require_once $f;
  if (empty($argv[1])) print "           ...............OK\n";
}

switch($module){
case 'ea':  $menu =  EA::_MENU(); break;
case 'vm':  $menu =  VM::_MENU(); break;
case 'wiw': $menu = WIW::_MENU(); break;
case 'lic': $menu = LIC::_MENU(); break;
case 'prp': $menu = PRP::_MENU(); break;
case 'jam': $menu = JAM::_MENU(); break;
default: 
  die("??? unknown case '$module'\n");
}

foreach(array('links.menu','routing') as $yml){
  $fn[$yml] = sprintf("/tmp/%s.%s.yml",$module,$yml);
  system("touch ".$fn[$yml]);
  $file[$yml] = fopen($fn[$yml],'a');
}

$_title_callback = "_".$module."_title_callback";
if (!function_exists($_title_callback)) die("Where is $_title_callback???");

$menu_tree = $menu->build_menuTree('dummy_page_callback','dummy_access_callback');

$ids = array();
foreach($menu_tree as $path=>$item){
  
  $path_e = explode('/',$path);
  $top_of_the_tree = (count($path_e) == 1);
  
  if ($top_of_the_tree){
    // Build "horizontal menu"
    $title = $item['title'];
    $tab = $module;
    $weight = 0;
    $menu_name = 'main';
    $route_name = "${module}.${menu_name}${module}";
    $route_name = "${module}.${menu_name}_menu";
    $parent = '';
    $access = 'main_menu';
    $expanded = False;
    build_routing_yml($file['routing']);
    build_links_menu_yml($file['links.menu']);  
  }

  // Build "left menu"
  foreach(array('access','title','page') as $a){
    $tab = $item["$a arguments"][0];
    if (!empty($tab)) break;
  }

  $access = 'custom';
  $expanded = $top_of_the_tree;
  $expanded = True; // well... otherwise it always collapses
  $route_name = "${module}.".implode('_',$path_e);
  array_pop($path_e);
  $parent = implode('_',$path_e);
  $weight = (int)$item['weight'];
  $menu_name = 'tools';

  if (!$top_of_the_tree) $title = call_user_func($_title_callback,$tab,'',True);
  if ((strpos($title,$tab)!==False) && ($tab != $module)){
    //    die("??? $_title_callback($tab) --> $title\n");
  }
  
  build_routing_yml($file['routing']);
  build_links_menu_yml($file['links.menu']);  
}  

foreach(array('links.menu','routing') as $yml)  fclose($file[$yml]);
// system("ls -lrt /tmp/*yml");
system("grep -E '<.*>' /tmp/*yml");

exit;

/*
 * title: (required) The untranslated title of the menu link.
 * description: The untranslated description of the link.
 * route_parameters: (optional) The route parameters to build the path. An array.
 * route_name: (optional) The route name to be used to build the path. Either a route_name or a link_path must be provided.
 * link_path: (optional) If you have an external link use link_path instead of providing a route_name.
 * parent: (optional) The machine name of the link that is this link's menu parent.
 * weight: (optional, defaults to 0) An integer that determines the relative position of items
 *   in the menu; higher-weighted items sink.  Menu items with the same weight are ordered alphabetically.
 * menu_name: (optional) The machine name of a menu to put the link in, if not the default Tools menu. 
 *   Common names are "account", "admin", "footer", "main", "tools"
 * expanded: (optional) If set to TRUE, and if a menu link is provided for this
 *   menu item (as a result of other properties), then the menu link is always
 *   expanded, equivalent to its 'always expanded' checkbox being set in the UI.
 * options: (optional) An array of options to be passed to l() when generating a link from this menu item.
 */
function build_links_menu_yml(){
  global $file;

  fwrite($file['links.menu'],process_template(
"<route_name>:
  title: <title>
  route_name: <route_name>
  weight: <weight>
  menu_name: <menu_name>
# route_parameters:
#  - q: <path>
#  - tab: <tab>
# options:
" 
. (empty($GLOBALS['parent']) ? ""   : "  parent: <module>.<parent>\n")
//. (empty($GLOBALS['expanded']) ? "" : "  expanded: 'TRUE'\n")
. "
"));
}
	 
	 /*
 * path:
 *   The path for this route
 * defaults:
 *   _content: This is a content page, our class provides a render array
 *   _controller: A controller class does all of the processing for this path
 *   _title: A static title for our page
 *   _title_callback: The page title is generated by a method in our class
 * requirements:
 *   _permission: only give access to users with this permission
 *   _role: only give access to users with this role
 *   _access: access is either granted (TRUE) or not (FALSE)
 *   _custom_access: access is determined by a method in our class
 */
function build_routing_yml($file){
  fwrite($file,process_template(
"<route_name>:
  path: /<path>
  defaults:
    _controller:     Drupal\\myPear\\components\\PageController::getPageContent
    _title_callback: Drupal\\myPear\\components\\PageController::getPageTitle
  requirements:
    _access_check_token: 'TRUE'

"));
}

/*
 *
 */
function process_template($template){
  $template = implode("\n",preg_grep('/^#/',explode("\n",$template),PREG_GREP_INVERT));
  foreach(array('module',
		'path',
		'route_name',
		'weight',
		'title',
		'tab',
		'parent',
		'expanded',
		'menu_name',
		) as $item){
    $template = str_replace("<$item>",$GLOBALS[$item],$template);
  }
  return $template;
}

function dummy_page_callback(){
  printf("%s called\n",__FUNCTION__);
}
function dummy_access_callback(){
  printf("%s called\n",__FUNCTION__);
}
