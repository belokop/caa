<?php

namespace Drupal\Tests\page_load_progress\Functional;

use Drupal\Tests\BrowserTestBase;
use Drupal\Core\Url;

/**
 * Tests for the page_load_progress module.
 *
 * @group page_load_progress
 */
class PageLoadProgressAdminSettingsFormTest extends BrowserTestBase {
  /**
   * User account with page_load_progress permissions.
   *
   * @var \Drupal\Core\Session\AccountInterface
   */
  protected $privilegedUser;

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
      'name' => 'Page Load Progress settings form',
      'description' => 'Tests the Page Load Progress admin settings form.',
      'group' => 'Page Load Progress',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();
    // Privileged user should only have the page_load_progress permissions.
    $this->privilegedUser = $this->drupalCreateUser(['administer page load progress']);
    $this->drupalLogin($this->privilegedUser);
  }

  /**
   * Test the page_load_progress settings form.
   */
  public function testPageLoadProgressSettings() {
    // Verify if we can successfully access the page_load_progress form.
    $url = Url::fromRoute('page_load_progress.admin_settings');
    $this->drupalGet($url);
    $this->assertSession()->statusCodeEquals(200);

    $this->assertSession()->pageTextContains('Page Load Progress | Drupal');

    // Verify every field exists.
    $this->assertSession()->fieldExists('edit-page-load-progress-time');
    $this->assertSession()->fieldExists('edit-page-load-progress-elements');

    // Validate default form values.
    $this->assertSession()->fieldValueEquals('edit-page-load-progress-time', 10);
    $this->assertSession()->fieldValueEquals('edit-page-load-progress-elements', '.form-submit');

    // Verify that there's no access bypass.
    $this->drupalLogout();
    $this->drupalGet('admin/config/user-interface/page-load-progress');
    $this->assertSession()->statusCodeEquals(403);
  }

  /**
   * Test posting data to the page_load_progress settings form.
   */
  public function testPageLoadProgressSettingsPost() {
    // Post form with new values.
    $edit = [
      'page_load_progress_time' => 5000,
      'page_load_progress_elements' => '.sample_submit',
    ];
    $this->drupalPostForm('admin/config/user-interface/page-load-progress', $edit, 'Save configuration');

    // Load settings form page and test for new values.
    $this->drupalGet('admin/config/user-interface/page-load-progress');
    $this->assertSession()->fieldValueEquals('edit-page-load-progress-time', $edit['page_load_progress_time']);
    $this->assertSession()->fieldValueEquals('edit-page-load-progress-elements', $edit['page_load_progress_elements']);
  }

}
