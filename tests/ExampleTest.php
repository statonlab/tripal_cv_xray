<?php

namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

/**
 * Class ExampleTest
 *
 * Note that test classes must have a suffix of Test.php and the filename
 * must match the class name.
 *
 * @package Tests
 */
class ExampleTest extends TripalTestCase {

  /** @test */
  public function testBasicExample() {
    $dispatcher = new \XRayDispatcherJob();
    $dispatcher->clearIndexTable();

    $count = (int) db_query('SELECT COUNT(*) FROM tripal_cvterm_entity_linker')->fetchField();
    $this->assertEquals($count, 0);

    $bundles = $dispatcher->bundles();
    foreach ($bundles as $bundle) {
      echo "\nProcessing: chado_bio_data_{$bundle->bundle_id}\n";
      $total = $dispatcher->bundleTotal($bundle);
      for ($i = 0; $i < $total; $i += 1) {
        $job = new \XRayIndexerJob($bundle, TRUE);
        $job->offset($i)->limit(1);
        $job->handle();
      }
    }

    $count = (int) db_query('SELECT COUNT(*) FROM tripal_cvterm_entity_linker')->fetchField();
    echo "FINAL COUNT $count\n";
  }
}
