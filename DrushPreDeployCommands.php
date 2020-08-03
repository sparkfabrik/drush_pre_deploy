<?php

namespace Drush\Commands\drush_pre_deploy;

use Consolidation\Log\ConsoleLogLevel;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\KeyValueStore\KeyValueFactoryInterface;
use Drupal\Core\Update\UpdateRegistry;
use Drupal\Core\Utility\Error;
use Drush\Drupal\Commands\core\DeployHookCommands;
use Drush\Exceptions\UserAbortException;
use Psr\Log\LogLevel;
use Consolidation\SiteAlias\SiteAliasManagerAwareInterface;
use Consolidation\SiteAlias\SiteAliasManagerAwareTrait;
use Drush\Drush;

/**
 * Pre-deploy drush command class.
 */
class DrushPreDeployCommands extends DeployHookCommands implements SiteAliasManagerAwareInterface {

  use SiteAliasManagerAwareTrait;

  /**
   * {@inheritDoc}
   *
   * Override the construct to be able to receive 0 params like a sitewide
   * drush command expects.
   * The `init()` function is later called in each command to initialize the members.
   * This is a technical limitation due to the fact that sitewide drush command
   * cannot use a drush.services.yml file.
   */
  public function __construct() {
  }

  /**
   * Prints information about pending pre-deploy update hooks.
   *
   * @usage deploy:pre-hook-status
   *   Prints information about pending pre-deploy hooks.
   *
   * @field-labels
   *   module: Module
   *   hook: Hook
   *   description: Description
   * @default-fields module,hook,description
   *
   * @command deploy:pre-hook-status
   *
   * @filter-default-field hook
   *
   * @return \Consolidation\OutputFormatters\StructuredData\RowsOfFields
   *   A list of pending hooks.
   *
   * @phpcs:disable Generic.CodeAnalysis.UselessOverridingMethod.Found
   */
  public function status() {
    $this->init();
    return parent::status();
  }

  /**
   * Runs pre-deploy hooks.
   *
   * @usage deploy:pre-hook
   *   Runs pending pre-deploy hooks.
   *
   * @command deploy:pre-hook
   *
   * @return int
   *   0 for success, 1 for failure.
   */
  public function preDeploy() {
    $this->init();
    $pending = $this->registry->getPendingUpdateFunctions();

    if (empty($pending)) {
      $this->logger()->success(dt('No pending pre-deploy hooks.'));
      return self::EXIT_SUCCESS;
    }

    $process = $this->processManager()->drush($this->siteAliasManager()->getSelf(), 'deploy:pre-hook-status');
    $process->mustRun();
    $this->output()->writeln($process->getOutput());

    if (!$this->io()->confirm(dt('Do you wish to run the specified pending pre deploy hooks?'))) {
      throw new UserAbortException();
    }

    $success = TRUE;
    if (!$this->getConfig()->simulate()) {
      $success = $this->doRunPendingHooks($pending);
    }

    $level = $success ? ConsoleLogLevel::SUCCESS : LogLevel::ERROR;
    $this->logger()->log($level, dt('Finished performing pre deploy hooks.'));
    return $success ? self::EXIT_SUCCESS : self::EXIT_FAILURE;
  }

  /**
   * Runs pending hooks.
   *
   * @param array $pending
   *   An array of hooks to execute.
   *
   * @return bool
   *   TRUE if everything ran correctly, FALSE otherwise.
   */
  protected function doRunPendingHooks(array $pending) {
    try {
      foreach ($pending as $function) {
        $func = new \ReflectionFunction($function);
        $this->logger()->notice('Predeploy hook started: ' . $func->getName());

        // Pretend it is a batch operation to keep the same signature
        // as the post update hooks.
        $sandbox = [];
        do {
          $return = $function($sandbox);
          if (!empty($return)) {
            $this->logger()->notice($return);
          }
        } while (isset($sandbox['#finished']) && $sandbox['#finished'] < 1);

        $this->registry->registerInvokedUpdates([$function]);
        $this->logger()->debug('Performed: ' . $func->getName());
      }

      return TRUE;
    }
    catch (\Throwable $e) {
      $variables = Error::decodeException($e);
      unset($variables['backtrace']);
      $this->logger()->error(dt('%type: @message in %function (line %line of %file).', $variables));
      return FALSE;
    }
  }

  /**
   * Constructor like function.
   *
   * This init function exists because it seems currently impossible to use a
   * drush.services.yml for a sitewide drush command.
   */
  protected function init() {
    $root = \Drupal::service('app.root');
    $site_path = \Drupal::service('site.path');
    $moduleHandler = \Drupal::service('module_handler');
    $keyValueFactory = \Drupal::service('keyvalue');
    $this->keyValue = $keyValueFactory->get('pre_deploy_hook');
    $this->registry = new class(
      $root,
      $site_path,
      array_keys($moduleHandler->getModuleList()),
      $this->keyValue
    ) extends UpdateRegistry {

      /**
       * Sets the update registry type.
       *
       * @param string $type
       *   The registry type.
       */
      public function setUpdateType($type) {
        $this->updateType = $type;
      }

    };
    $this->registry->setUpdateType('predeploy');
  }

}
