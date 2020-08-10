# drush-pre-deploy

This project provides the `deploy:pre-hook` and `deploy:pre-hook-status` commands which are similar to the [updatedb](https://drushcommands.com/drush-8x/core/updatedb/) and [updatedb-status](https://drushcommands.com/drush-8x/core/updatedb-status/) commands. The first runs pending "pre-deploy" hooks and the second prints information about pending "pre-deploy" update hooks.
These two commands are meant to be called in your D7 deploy procedure (along with other drush commands like `updatedb`, `features-revert-all`, ...)

# Installation

`composer require sparkfabrik/drush-pre-deploy:7.x-dev`

Make sure you have `"minimum-stability": "dev"`
