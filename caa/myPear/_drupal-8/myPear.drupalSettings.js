(function ($, Drupal, drupalSettings) {
    Drupal.behaviors.yourbehavior = {
	attach: function (context, settings) {
	    console.log('Loaded');
	}
    };
})(jQuery, Drupal, drupalSettings);
