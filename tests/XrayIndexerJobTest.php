<?php

namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

class XrayIndexerJobTest extends TripalTestCase {

  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;

  /**
   * @ticket 39
   * @group failing
   * The indexer currently ends up with many identical entries: for example,
   *   this entity https://hardwoodgenomics.org/bio_data/132373
   * @throws \Exception
   */
  public function test_XrayIndexerNoDuplicate() {
    // Pick three go terms that are all nearby
    $termA = chado_get_cvterm(['id' => 'GO:0003723']);
    $termB = chado_get_cvterm(['id' => 'GO:0008135']);
    $termC = chado_get_cvterm(['id' => 'GO:0003677']);

    $terms = [
      $termA,
      $termB,
      $termC,
    ];

    $mrna_term = chado_get_cvterm(['id' => 'SO:0000234']);
    $mrna = factory('chado.feature')->create(['type_id' => $mrna_term->cvterm_id]);

    foreach ($terms as $term) {
      factory('chado.feature_cvterm')->create([
        'feature_id' => $mrna->feature_id,
        'cvterm_id' => $term->cvterm_id,
      ]);
    }

    $this->publish('feature');

    $entity_id = chado_get_record_entity_by_table('feature', $mrna->feature_id);

    if (!$entity_id) {
      print("Entity could not be published!");
      return;
    }

    $bundle_id = db_select('chado_bundle', 'cb')
      ->fields('cb', ['bundle_id'])
      ->condition('type_id', $mrna_term->cvterm_id)
      ->execute()
      ->fetchField();

    $bundle = tripal_load_bundle_entity(['id' => $bundle_id]);
    $bundle->bundle_id = $bundle_id;

    $job = new \XRayIndexerJob([
      'bundle' => $bundle,
      'cv_shortnames' => ['GO'],
    ]);
    $job->offset(0);
    $job->limit(500000);
    $job->handle();


    $query = 'SELECT t.cvterm_id AS id, count(*) AS count FROM {tripal_cvterm_entity_linker} t 
              WHERE t.entity_id = :entity_id
              GROUP BY t.cvterm_id';

    $results = db_query($query, [':entity_id' => $entity_id])->fetchAll();

    /**
     * For any given entity, a given cvterm should not be in the linker more than once.
     */
    foreach ($results as $result) {
      $this->assertLessThan(2, (int) $result->count);
    }
  }
}
