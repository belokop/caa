/**
 * @file
 * Page Load Progress allows you to set a screen lock showing a spinner when the
 * user clicks on an element that triggers a time consuming task.
 */

(function($){
    Drupal.behaviors.page_load_progress = {
	attach: function(context, settings) {
	    var delay = Number(Drupal.settings.page_load_progress.delay);
	    var exit_elements = String(Drupal.settings.page_load_progress.elements).split(',');
	    var screen_lock = '<div class="page_load_progress_lock_screen plp_hidden"><div class="page_load_progress_spinner"></div></div>';
	    var body = $('body', context);
	    for (i in exit_elements) {
		$(exit_elements[i]).click(function() { setTimeout(lockScreen, delay); });
	    }
	    var lockScreen = function () {
		body.append(screen_lock);
		body.css({'overflow' : 'hidden'});
	    //? body.css({'overflow' : 'plp_hidden'});
		$('.page_load_progress_lock_screen').fadeIn('slow');
	    }
	}
    };
})(jQuery);
