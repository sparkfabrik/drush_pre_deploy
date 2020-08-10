<?php

/**
 * @file
 * Onetime functions drush commands for d7.
 */

use Drush\Log\LogLevel;

/**
 * Gets the last executed onetime func for each installed module.
 */
function drush_get_last_executed_onetime_functions($type = 'predeploy') {
  $last_executed = variable_get('drush_deploy_last_executed_' . $type, []);
  $toreturn = [];
  foreach (module_list() as $module) {
    $toreturn[$module] = $last_executed[$module] ?? 0;
  }
  return $toreturn;
}

/**
 * Sets the last executed onetime func for a module.
 */
function drush_set_last_executed_onetime_function($module, $number, $type = 'predeploy') {
  $last_executed = variable_get('drush_deploy_last_executed_' . $type, []);
  $last_executed[$module] = $number;
  variable_set('drush_deploy_last_executed_' . $type, $last_executed);
}

/**
 * Get IDs of "one-time" functions of a specific type.
 *
 * @see drupal_get_schema_versions
 */
function drush_get_onetime_function_ids($module, $type = 'predeploy') {
  $function_ids = &drupal_static(__FUNCTION__, NULL);
  if (!isset($function_ids[$module])) {
    $function_ids = [];

    foreach (module_list() as $loaded_module) {
      $function_ids[$loaded_module] = [];
    }

    // Prepare regular expression to match all possible defined hook_$type_N().
    $regexp = '/^(?P<module>.+)_' . $type . '_(?P<version>\d+)$/';
    $functions = get_defined_functions();
    // Narrow this down to functions ending with an integer, since all
    // hook_$type_N() functions end this way, and there are other
    // possible functions which match '_$type_'. We use preg_grep() here
    // instead of foreaching through all defined functions, since the loop
    // through all PHP functions can take significant page execution time.
    foreach (preg_grep('/_\d+$/', $functions['user']) as $function) {
      if (preg_match($regexp, $function, $matches)) {
        $function_ids[$matches['module']][] = $matches['version'];
      }
    }
    // Ensure that function_ids are applied in numerical order.
    foreach ($function_ids as &$module_funcs) {
      sort($module_funcs, SORT_NUMERIC);
    }
  }
  return $function_ids[$module] ?? FALSE;
}

/**
 * List of onetime functions hooks.
 *
 * @see update_get_update_list
 */
function drush_get_pending_onetime_functions($type = 'predeploy') {
  $ret = [];

  $last_executed_funcs = drush_get_last_executed_onetime_functions();
  foreach (module_list() as $module) {
    $last_executed_func = $last_executed_funcs[$module];
    $func_ids = drush_get_onetime_function_ids($module, $type);
    if (!$func_ids) {
      continue;
    }
    foreach ($func_ids as $function_id) {
      if ($function_id > $last_executed_func) {
        // The description for an function_id comes from its Doxygen.
        $func = new ReflectionFunction(drush_get_onetime_function_name($module, $type, $function_id));
        $description = str_replace(["\n", '*', '/'], '', $func->getDocComment());
        $ret[$module]['pending'][$function_id] = "$function_id - $description";
        if (!isset($ret[$module]['start'])) {
          $ret[$module]['start'] = $function_id;
        }
      }
    }
    if (!isset($ret[$module]['start']) && isset($ret[$module]['pending'])) {
      $ret[$module]['start'] = $last_executed_func;
    }
  }
  return $ret;
}

/**
 * Get the onetime function name.
 */
function drush_get_onetime_function_name($module, $type, $number) {
  return $module . '_' . $type . '_' . $number;
}

/**
 * List of onetime funcs per type.
 *
 * @see updatedb_status
 */
function drush_d7_onetime_functions_status($type = 'predeploy') {
  drush_include_onetime_functions_files($type);
  $pending = drush_get_pending_onetime_functions($type);
  $return = [];
  $start = [];
  // Print a list of pending onetime funcs for this module and get confirmation.
  foreach ($pending as $module => $onetime_funcs) {
    if (!isset($onetime_funcs['start'])) {
      continue;
    }
    foreach ($onetime_funcs['pending'] as $number => $description) {
      // Strip cruft from front.
      $description = str_replace($number . ' -   ', '', $description);
      $return[] = [
        'module' => ucfirst($module),
        'number' => $number,
        'description' => $description,
      ];
    }
    $start[$module] = $onetime_funcs['start'];
  }
  return [$return, $start];
}

/**
 * Loads .$type.php files for installed modules.
 */
function drush_include_onetime_functions_files($type = 'predeploy') {
  foreach (module_list() as $module) {
    module_load_include('php', $module, $module . '.' . $type);
  }
}

/**
 * Execute onetime funcs per type.
 *
 * @see update_main
 */
function drush_d7_onetime_functions_execute($type = 'predeploy') {
  // Looks like calling update_main_prepare is not necessary here.
  list($pending, $start) = drush_d7_onetime_functions_status();
  if ($pending) {
    drush_print_table($pending);
    if (!drush_confirm(dt("Do you wish to run all pending $type?"))) {
      return drush_user_abort();
    }
    drush_onetime_functions_execute_batch($type);
  }
  else {
    drush_log(dt("No @type required.", ['@type' => $type]), LogLevel::SUCCESS);
  }

  return count($pending);
}

/**
 * Transformation function.
 *
 * This function transforms the return value from
 * drush_get_pending_onetime_functions to an input valid for
 * drush_onetime_functions_update_build_dependency_graph.
 *
 * @param array $pending
 *   An array as returned from drush_get_pending_onetime_functions.
 *   Example:
 *   $pending = [
 *     'book' => [
 *       'pending' => [
 *         '0001' => '0001 -   Some text. ',
 *         '0002' => '0002 -   Some text. ',
 *       ],
 *       'start' => '0001',
 *     ],
 *     'color' => [
 *       'pending' => [
 *         '0001' => '0001 -   Some text. ',
 *         '0002' => '0002 -   Some text. ',
 *       ],
 *       'start' => '0001',
 *     ],
 *   ];.
 * @param string $type
 *   The onetime func type.
 *
 * @return array
 *   An array that can be passed as input to
 *   drush_onetime_functions_update_build_dependency_graph.
 *   Example:
 *   [
 *     'color' => [
 *       7003 => 'color_update_7003',
 *     ],
 *   ]
 */
function drush_transform_pending_onetime_functions_to_input_for_dependency(array $pending, $type = 'predeploy') {
  $toreturn = [];
  foreach ($pending as $module => $struct) {
    foreach (array_keys($struct['pending'] ?? []) as $number) {
      $toreturn[$module][$number] = drush_get_onetime_function_name($module, $type, $number);
    }
  }
  return $toreturn;
}

/**
 * Like update_resolve_dependencies but for onetime funcs.
 *
 * Does not call update_is_missing and update_already_performed.
 *
 * @see update_resolve_dependencies
 */
function drush_onetime_functions_update_resolve_dependencies($pending, $type = 'predeploy') {
  $update_functions = drush_transform_pending_onetime_functions_to_input_for_dependency($pending, $type);
  $graph = drush_onetime_functions_update_build_dependency_graph($update_functions);

  // Perform the depth-first search and sort the results.
  require_once DRUPAL_ROOT . '/includes/graph.inc';
  drupal_depth_first_search($graph);
  uasort($graph, 'drupal_sort_weight');

  foreach ($graph as $function => &$data) {
    $module = $data['module'];
    $number = $data['number'];
    if (!isset($data['allowed'])) {
      $data['allowed'] = TRUE;
      $data['missing_dependencies'] = [];
    }
    // Now that we have finished processing this function, remove it from the
    // graph if it was not part of the original list. This ensures that we
    // never try to run any updates that were not specifically requested.
    if (!isset($update_functions[$module][$number])) {
      unset($graph[$function]);
    }
  }

  return $graph;
}

/**
 * Onetime function specific update_build_dependency_graph.
 *
 * Does not call update_retrieve_dependencies.
 *
 * @see update_build_dependency_graph
 */
function drush_onetime_functions_update_build_dependency_graph($update_functions) {
  // Initialize an array that will define a directed graph representing the
  // dependencies between update functions.
  $graph = [];

  // Go through each update function and build an initial list of dependencies.
  foreach ($update_functions as $module => $functions) {
    $previous_function = NULL;
    foreach ($functions as $number => $function) {
      // Add an edge to the directed graph representing the fact that each
      // update function in a given module must run after the update that
      // numerically precedes it.
      if ($previous_function) {
        $graph[$previous_function]['edges'][$function] = TRUE;
      }
      $previous_function = $function;

      // Define the module and update number associated with this function.
      $graph[$function]['module'] = $module;
      $graph[$function]['number'] = $number;
    }
  }
  return $graph;
}

/**
 * Start the onetime functions batch process.
 *
 * @see drush_update_batch
 */
function drush_onetime_functions_execute_batch($type = 'predeploy') {
  $pending = drush_get_pending_onetime_functions($type);
  $updates = drush_onetime_functions_update_resolve_dependencies($pending, $type);

  $dependency_map = [];
  foreach ($updates as $function => $update) {
    $dependency_map[$function] = array_keys($update['reverse_paths'] ?? []);
  }
  $operations = [];
  foreach ($updates as $update) {
    if (!$update['allowed']) {
      continue;
    }
    $module = $update['module'];
    $number = $update['number'];
    $function = drush_get_onetime_function_name($module, $type, $number);
    $operations[] = [
      'drush_onetime_func_do_one',
      [$module, $number, $dependency_map[$function], $type],
    ];
  }
  $batch['operations'] = $operations;
  $batch += [
    'title'         => 'Updating',
    'init_message'  => 'Starting ' . $type,
    'error_message' => 'An unrecoverable error has occurred. You can find the error message below. It is advised to copy it to the clipboard for reference.',
    'finished'      => 'drush_update_finished',
  ];
  batch_set($batch);
  drush_backend_batch_process('updatedb-batch-process');
}

/**
 * Like drush_update_do_one but for onetime func.
 *
 * @see drush_update_do_one
 */
function drush_onetime_func_do_one($module, $number, $dependency_map, $type, &$context) {
  drush_include_onetime_functions_files($type);
  $function = drush_get_onetime_function_name($module, $type, $number);

  // If this update was aborted in a previous step, or has a dependency that
  // was aborted in a previous step, go no further.
  if (!empty($context['results']['#abort']) && array_intersect($context['results']['#abort'], array_merge($dependency_map, [$function]))) {
    return;
  }

  $context['log'] = FALSE;

  $ret = [];
  if (function_exists($function)) {
    try {
      if ($context['log']) {
        Database::startLog($function);
      }

      drush_log("Executing " . $function, LogLevel::OK);
      $ret['results']['query'] = $function($context['sandbox']);

      // If the update hook returned a status message (common in batch updates),
      // show it to the user.
      if ($ret['results']['query']) {
        drush_log($ret['results']['query'], LogLevel::OK);
      }

      $ret['results']['success'] = TRUE;
    }
    catch (\Exception $e) {
      $ret['#abort'] = ['success' => FALSE, 'query' => $e->getMessage()];
      drush_set_error('DRUPAL_EXCEPTION', $e->getMessage());
    }

    if ($context['log']) {
      $ret['queries'] = Database::getLog($function);
    }
  }

  if (isset($context['sandbox']['#finished'])) {
    $context['finished'] = $context['sandbox']['#finished'];
    unset($context['sandbox']['#finished']);
  }

  if (!isset($context['results'][$module])) {
    $context['results'][$module] = [];
  }
  if (!isset($context['results'][$module][$number])) {
    $context['results'][$module][$number] = [];
  }
  $context['results'][$module][$number] = array_merge($context['results'][$module][$number], $ret);

  if (!empty($ret['#abort'])) {
    // Record this function in the list of updates that were aborted.
    $context['results']['#abort'][] = $function;
  }

  // Record the schema update if it was completed successfully.
  if ($context['finished'] == 1 && empty($ret['#abort'])) {
    drush_set_last_executed_onetime_function($module, $number, $type);
  }

  $context['message'] = "Performed $type: " . $function;
}
