<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

class HookEntityIndexerTest extends TripalTestCase {
  // Uncomment to auto start and rollback db transactions per test method.
  // use DBTransaction;

  /**
   * @throws \Exception
   * @group test
   */
 public function testHookAddsToIndex(){
   //Set config.

   //Create an entity.  This should trigger hook.

   $mrna_term = chado_get_cvterm(['id' => 'SO:0000234']);
   $feature = factory('chado.feature')->create(['type_id' => $mrna_term->cvterm_id]);
   $this->publish('feature', [$feature->feature_id]);

   //Check linker index, confirm stuff was indexed.
   //TODO:  should the hook trigger a job?  If so how do we test that it worked?
 }
}
