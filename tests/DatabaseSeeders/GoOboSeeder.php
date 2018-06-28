<?php

namespace Tests\DatabaseSeeders;

//require_once __DIR__.'/../bootstrap.php';


use StatonLab\TripalTestSuite\Database\Seeder;


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

    $loader = new \OBOImporter();
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
  }

  /**
   * Not implemented
   *
   * @return void
   */
  public function down() {
  }
}
