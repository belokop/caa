/**
 * @file
 * Defines the behavior of the Page Load Progress module.
 *
 * Page Load Progress sets a screen lock showing a spinner when the user clicks
 * on an element that triggers a time consuming task.
 */

(function ($, Drupal) {

  'use strict';

  Drupal.behaviors.page_load_progress = {
    attach: function (context, settings) {
      var delay = Number(settings.page_load_progress.delay);
      var exit_elements = String(settings.page_load_progress.elements).split(',');
      var screen_lock = '<div class="page-load-progress-lock-screen hidden">\n\
                         <div class="page-load-progress-spinner"></div>\n\
                         </div>';
      var body = $('body', context, settings);
      for (var i in exit_elements) {
        $(exit_elements[i]).click(function () {
          setTimeout(lockScreen, delay);
        });
      }
      var lockScreen = function () {
        body.append(screen_lock);
        body.css({
          'overflow' : 'hidden'
        });
        $('.page-load-progress-lock-screen').fadeIn('slow');
      }
    }
  };

})(jQuery, Drupal);
