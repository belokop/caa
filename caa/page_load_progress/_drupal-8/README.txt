CONTENTS OF THIS FILE
---------------------

* Introduction
* Configuration
* Caveats
* Maintainers
* Credits

INTRODUCTION
------------

This module will allow you to set a screen lock showing a spinner when the
user clicks on an element that triggers a time consuming task.

The module will work amazingly in cases where, for example, the user submits a
form that takes a long time to post and stays in the current page avoiding the
submitter the chance to poke around the page while waiting.

CONFIGURATION
------------

To configure the behavior of this module go to
admin/config/user-interface/page-load-progress. The last field, "Elements that
will trigger the throbber", takes HTML elements, classes or ids, separated by
commas, and assigns the behavior when they are clicked.

CAVEATS
-------

Even though the behavior assignment is configurable, it is strongly recommended
for form submits only. If you must, assign the behavior to "A" elements
carefully. "A" elements can be opened in a new browser tab or window, which
would leave the original window locked waiting for reload. Also, "A" elements
are sometimes used with modals, so make sure that you identify what classes
trigger modal windows and you use :not() to avoid them, or use specific classes
when assigning the behavior (example "a.not-modal").

MAINTAINERS
-----------

* anavarre (https://www.drupal.org/u/anavarre)
* Dom. (https://www.drupal.org/u/dom)

CREDITS
-------

This module was originally created and maintained for Drupal 7 by Mariano
(https://www.drupal.org/u/mariano)
