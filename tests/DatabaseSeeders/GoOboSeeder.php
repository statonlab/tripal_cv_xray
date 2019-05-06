<?php

namespace Tests\DatabaseSeeders;

use StatonLab\TripalTestSuite\Database\Seeder;
use OBOImporter;

class GoOboSeeder extends Seeder {

  /**
   * Whether to run the seeder automatically before
   * starting our tests and destruct them automatically
   * once the tests are completed.
   *
   * @var bool
   */
  public static $auto_run = FALSE;

  /**
   * Runs the GO mini OBO.
   *
   * @return void
   */
  public function up() {
    module_load_include('inc', 'tripal_chado', 'includes/TripalImporter/OBOImporter');

    $loader = new OBOImporter();
    $form = [];
    $form_state = [];

    $form_state['values'] = [
      'obo_name' => 'GOslim',
      'obo_url' => 'http://www.geneontology.org/ontology/subsets/goslim_plant.obo',
    ];

    $loader->formSubmit($form, $form_state);

    $loader->create([
      'obo_id' => $form_state['values']['obo_id'],
    ]);

    $loader->run();

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

    $bundle_id = db_select('chado_bundle', 'cb')
      ->fields('cb', ['bundle_id'])
      ->condition('type_id', $mrna_term->cvterm_id)
      ->execute()
      ->fetchField();

    $bundle = tripal_load_bundle_entity(['id' => $bundle_id]);
    $bundle->bundle_id = $bundle_id;

    // Tell our config table that we want to index this entity type
    // We delete first to make sure we don't create any duplicate entries
    db_delete('tripal_cv_xray_config')
      ->condition('shortname', 'GO')
      ->condition('bundle_id', $bundle->bundle_id)
      ->execute();
    db_insert('tripal_cv_xray_config')
      ->fields([
        'shortname' => 'GO',
        'bundle_id' => $bundle->bundle_id,
      ])->execute();

    $this->publish('feature');
  }

  /**
   * Not implemented
   *
   * @return void
   */
  public function down() {
  }
}
