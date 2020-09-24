<?php

namespace Drush\Commands\core;

use Consolidation\SiteAlias\SiteAliasManagerAwareInterface;
use Drush\Commands\DrushCommands;
use Consolidation\SiteAlias\SiteAliasManagerAwareTrait;
use Drush\Drush;

/**
 * Class DrushPreDeployHookCommands.
 */
class DrushPreDeployHookCommands extends DrushCommands implements SiteAliasManagerAwareInterface {
  use SiteAliasManagerAwareTrait;

  /**
   * Provides a pre-command hook when deploy command is executed.
   *
   * @hook pre-command deploy
   */
  public function preDeployHook() {
    $self = $this->siteAliasManager()->getSelf();
    $redispatchOptions = Drush::redispatchOptions();
    $manager = $this->processManager();

    $this->logger()->notice("Drush pre-deploy hook start.");
    $process = $manager->drush($self, 'deploy:pre-hook', [], $redispatchOptions);
    $process->mustRun($process->showRealtime());
  }

}
