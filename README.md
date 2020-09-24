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

The very last step (`drush deploy:hook`) invokes [HOOK\_deploy\_NAME](https://github.com/drush-ops/drush/blob/10.x/tests/functional/resources/modules/d8/woot/woot.deploy.php) hooks.
The "deploy" hooks are similar to [post\_update hooks](https://api.drupal.org/api/drupal/core%21lib%21Drupal%21Core%21Extension%21module.api.php/function/hook_post_update_NAME/9.1.x) and are useful when you need to execute code at the very end of the deploy process.

This project introduces the concepts of "pre-deploy" hooks that are executed at the *very beginning* of the deploy process.

They take a similar form of the existing hooks, if your modules is named `foo` then in a `foo.predeploy.php` file you can write a function like this:

```
/**
 * Hook description here.
 */
function foo_predeploy_some_text_here(&$sandbox) {
}
```

Additionally, this project provides the `deploy:pre-hook` and `deploy:pre-hook-status` commands which are similar to the [deploy:hook](https://www.drush.org/commands/10.x/deploy_hook/) and [deploy:hook-status](https://www.drush.org/commands/10.x/deploy_hook-status/) commands. The first command runs pending "pre-deploy" hooks and the second one prints information about pending "pre-deploy" update hooks.

# Installation

`composer require sparkfabrik/drush_pre_deploy`

This project requires drush at least at version 10.3.0.

There are some additional manual install steps while some upstream packages like [Composer-installers](https://github.com/composer/installers)) adapt to Drush 10:

* In your project's main composer.json make sure installer-paths folder is set for "drupal-module" type:

```
"extra": {
  ...
  "installer-paths": {
    ...
    "web/modules/contrib/{$name}": ["type:drupal-module"],
```

To make sure the hook command is discovered, you need to add a custom "drush.yml" configuration in a drush folder like this:
```
.
└── ROOT_PROJECT_PATH/
    └── drush/
        └── drush.yml
```
Drush will discover this file automatically and will use its configuration to load commands.
Add the following code into drush.yml:
```
drush:
  include:
  - ${env.PWD}/web/modules/contrib/drush_pre_deploy/src/global
```
