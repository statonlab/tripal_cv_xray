<?php

namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

module_load_include('install', 'tripal_cv_xray', 'tripal_cv_xray');


class InstallBasicsTest extends TripalTestCase {

  // Uncomment to auto start and rollback db transactions per test method.
  // use DBTransaction;


  /**
   * Check that Schemas dont error.
   *
   * @group install
   */

  public function testHookSchema() {

    $schema = tripal_cv_xray_schema();
    $this->assertNotFalse($schema);
    $this->assertNotEmpty($schema);
  }
}
