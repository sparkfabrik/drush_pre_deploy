# drush-pre-deploy

This project provides the commands:

- `predeploy` (alias `deploy:predeploy`
- `predeploy-status` (alias `deploy:predeploy-status`
- `postdeploy` (alias `deploy:postdeploy`
- `postdeploy-status` (alias `deploy:postdeploy-status`

which are similar to the [updatedb](https://drushcommands.com/drush-8x/core/updatedb/) and [updatedb-status](https://drushcommands.com/drush-8x/core/updatedb-status/) commands.

The first runs pending "predeploy" (or postdeploy) hooks and the second prints information about pending "predeploy" (or postdeploy) update hooks.
These two commands are meant to be called in your D7 deploy procedure (along with other drush commands like `updatedb`, `features-revert-all`, ...)

## Example Predeploy

If your modules is named `foo` then in a `foo.predeploy.php`  file you can write a function like this:

```
/**
 * Hook description here.
 */
function foo_predeploy_0001(&$sandbox) {
}
```

## Example Postdeploy

If your modules is named `foo` then in a `foo.postdeploy.php`  file you can write a function like this:

```
/**
 * Hook description here.
 */
function foo_postdeploy_0001(&$sandbox) {
}
```

# Installation

`composer require sparkfabrik/drush-pre-deploy:7.x-dev`

Make sure you have `"minimum-stability": "dev"`
