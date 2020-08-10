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
  $items['pre-hook-status'] = [
    'description' => "Prints information about pending pre-deploy update hooks.",
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'outputformat' => [
      'default' => 'table',
      'pipe-format' => 'csv',
      'field-labels' => ['module' => 'Module', 'number' => 'Predeploy ID', 'description' => 'Description'],
      'fields-default' => ['module', 'number', 'description'],
      'output-data-type' => 'format-table',
    ],
    'aliases' => ['deploy:pre-hook-status'],
  ];
  $items['pre-hook'] = [
    'description' => "Runs pre-deploy hooks.",
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'aliases' => ['deploy:pre-hook'],
  ];
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
function drush_onetimefunctions_pre_hook_status() {
  require_once DRUSH_DRUPAL_CORE . '/includes/install.inc';
  $type = 'predeploy';
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
function drush_onetimefunctions_pre_hook() {
  $type = 'predeploy';
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
