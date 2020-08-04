# drush-pre-deploy

This project is a drush integration that enables "pre-deploy" hooks.

Drush 10 introduced the [drush deploy](https://www.drush.org/deploycommand/) command with the intent to standardize drupal deployment.
`drush deploy` is implemented in terms of a standard sequence of drush commands:

```
drush updatedb --no-cache-clear
drush cache:rebuild
drush config:import
drush cache:rebuild
drush deploy:hook
```

The vary last step (`drush deploy:hook`) invokes [HOOK\_deploy\_NAME](https://github.com/drush-ops/drush/blob/10.x/tests/functional/resources/modules/d8/woot/woot.deploy.php) hooks.
The "deploy" hooks are similar to [post\_update hooks](https://api.drupal.org/api/drupal/core%21lib%21Drupal%21Core%21Extension%21module.api.php/function/hook_post_update_NAME/9.1.x) and are useful when you need to execute code at the very end of the deploy process.

This project introduces the concepts of "pre-deploy" hooks that are run at the *very beginning* of the deploy process.

They take a similar form of the existing hooks, if your modules is named `foo` then in a `foo.predeploy.php` file you can write a function like this:

```
/**
 * Hook description here.
 */
function foo_predeploy_some_text_here(&$sandbox) {
}
```

Additionally this project provides the `deploy:pre-hook` and `deploy:pre-hook-status` commands which are similar to the [deploy:hook](https://www.drush.org/commands/10.x/deploy_hook/) and [deploy:hook-status](https://www.drush.org/commands/10.x/deploy_hook-status/) commands. The first runs pending "pre-deploy" hooks and the second prints information about pending "pre-deploy" update hooks.

# Installation

`composer require sparkfabrik/drush-pre-deploy`

This project requires drush at least at version 10.3.0.

There are some additional manual install steps while some upstream packages like [Composer-installers](https://github.com/composer/installers)) adapt to Drush 10:

* In your project's main composer.json, change the 'type:drupal-drush' installer-path from `drush/contrib/{$name}` to `drush/Commands/{$name}`.
* If your repository includes a legacy `drush/contrib` folder, rename it to `drush/Commands`.
