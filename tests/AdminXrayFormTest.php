<?php

namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

class AdminXrayFormTest extends TripalTestCase {

  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;


  /**
   * @group testing
   */
  public function testBasicExample() {
    $this->assertTrue(TRUE);
  }

  /**
   * @group testing
   */
  public function testRetrieveIndexedDB() {

    //set database to have known state
    $populated_db = $this->populate_index();
    //retrieve chado.databases that are indexed

    $db = $populated_db['db'];

    $indexed = \tripal_cv_xray_retrieve_indexed_dbs();

    $this->assertNotEmpty($indexed);

    $this->assertArrayHasKey($db->name, $indexed, 'The retrieve indexed db method did not return the test DB from the linker index.');

    $our_db = $indexed[$db->name];
    $this->arrayHasKey('name', $our_db);
    $this->assertArrayHasKey('count', $our_db);
    $this->assertGreaterThan('0', $our_db['count']);

  }

  public function testRetrieveIndexedBundles() {

    //set database to have known state
    $populated_db = $this->populate_index();
    //retrieve chado.databases that are indexed

    $db = $populated_db['db'];

    $bundles  = \tripal_cv_xray_count_indexed_entities($db->name);
    $this->assertNotNull($bundles);
    $this->assertNotEmpty($bundles);

  }

  private function populate_index() {
    $db_name = 'XRAY_TEST';
    $acc = '0000001';
    $db = factory('chado.db')->create(['name' => $db_name]);
    $cv = factory('chado.cv')->create([]);
    $cvterm = factory('chado.cvterm')->create(['cv_id' => $cv->cv_id]);
    $dbxref = factory('chado.dbxref')->create([
      'db_id' => $db->db_id,
      'accession' => $acc,
    ]);

    $mrna_term = chado_get_cvterm(['id' => 'SO:0000234']);
    $feature = factory('chado.feature')->create(['type_id' => $mrna_term->cvterm_id]);

    $this->publish('feature', [], 'feature_id');


    $entity = chado_get_record_entity_by_table('feature', $feature->feature_id);

    db_insert('public.tripal_cvterm_entity_linker')
      ->fields([
        'cvterm_id' => $cvterm->cvterm_id,
        'database' => $db_name,
        'accession' => $acc,
        'entity_id' => $entity,
      ])
      ->execute();
    return ['db' => $db, 'feature' => $feature, 'entity_id' => $entity];

  }

}
