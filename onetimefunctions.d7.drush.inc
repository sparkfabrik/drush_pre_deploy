<?php

/**
 * @file
 * Onetime functions drush commands for d7.
 */

use Drush\Log\LogLevel;

require_once __DIR__ . '/onetimefunctions.utils.inc';

/**
 * Implements hook_drush_command().
 */
function onetimefunctions_drush_command() {
  $types = ['predeploy', 'postdeploy'];
  foreach ($types as $type) {
    $items["$type-status"] = [
      'description' => "Prints information about pending $type update hooks.",
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
      'callback' => 'drush_onetimefunctions_status',
      'callback arguments' => [$type],
      'outputformat' => [
        'default' => 'table',
        'pipe-format' => 'csv',
        'field-labels' => [
          'module' => 'Module',
          'number' => ucfirst($type) . ' ID',
          'description' => 'Description',
        ],
        'fields-default' => ['module', 'number', 'description'],
        'output-data-type' => 'format-table',
      ],
      'aliases' => ["deploy:$type-status"],
    ];
    $items[$type] = [
      'description' => "Runs $type hooks.",
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
      'callback' => 'drush_onetimefunctions_execute',
      'callback arguments' => [$type],
      'aliases' => ["deploy:$type"],
    ];
  }
  return $items;
}

/**
 * Implements hook_drush_help().
 */
function onetimefunctions_drush_help($section) {
  $items = onetimefunctions_drush_command();
  $name = str_replace('drush:', '', $section);
  if (isset($items[$name])) {
    return dt($items[$name]['description']);
  }
}

/**
 * Implements deploy:pre-hook-status.
 *
 * @see drush_core_updatedb_status
 */
function drush_onetimefunctions_status($type) {
  require_once DRUSH_DRUPAL_CORE . '/includes/install.inc';
  list($pending, $start) = drush_d7_onetime_functions_status($type);
  if (empty($pending)) {
    drush_log(dt("No @type required.", ['@type' => $type]), LogLevel::OK);
  }
  return $pending;
}

/**
 * Implements deploy:pre-hook.
 *
 * @see drush_core_updatedb
 */
function drush_onetimefunctions_execute($type) {
  $result = drush_d7_onetime_functions_execute($type);
  if ($result === FALSE) {
    return FALSE;
  }
  elseif ($result > 0) {
    // Clear all caches in a new process. We just performed major surgery.
    drush_drupal_cache_clear_all();
    drush_log(dt('Finished performing @type.', ['@type' => $type]), LogLevel::OK);
  }
}
