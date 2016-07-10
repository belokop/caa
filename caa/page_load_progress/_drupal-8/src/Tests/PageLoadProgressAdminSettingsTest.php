<?php

namespace Drupal\page_load_progress\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Tests for the page_load_progress module's admin settings.
 *
 * @group page_load_progress
 */
class PageLoadProgressAdminSettingsTest extends WebTestBase {
  /**
   * User account with administrative permissions.
   *
   * @var \Drupal\Core\Session\AccountInterface
   */
  protected $adminUser;

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = ['page_load_progress'];

  /**
   * The installation profile to use with this test.
   *
   * @var string
   */
  protected $profile = 'minimal';

  /**
   * {@inheritdoc}
   */
  public static function getInfo() {
    return [
      'name' => 'Page Load Progress admin settings',
      'description' => 'Tests the Page Load Progress admin settings.',
      'group' => 'Page Load Progress',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();
    // Admin user account only needs a subset of admin permissions.
    $this->adminUser = $this->drupalCreateUser([
      'administer site configuration',
      'access administration pages',
      'administer permissions',
      'administer page load progress',
    ]);
    $this->drupalLogin($this->adminUser);
  }

  /**
   * Test menu link and permissions.
   */
  public function testAdminPages() {

    // Verify admin link.
    $this->drupalGet('admin/config/user-interface');
    $this->assertResponse(200, 'The User interface page is available.');
    $this->assertLink('Page Load Progress');
    $this->assertLinkByHref('admin/config/user-interface/page-load-progress');

    // Verify that there's no access bypass.
    $this->drupalLogout();
    $this->drupalGet('admin/config/user-interface');
    $this->assertResponse(403, 'Access denied for non-admin user.');
  }

}
