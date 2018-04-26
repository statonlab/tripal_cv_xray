<?php

use StatonLab\TripalTestSuite\TripalTestCase;
use StatonLab\TripalTestSuite\DBTransaction;

class entity_child_cvterm_test extends TripalTestCase {

  use DBTransaction;


  public function test_get_all_associated_cvterms() {
    $obj = new ChadoEntityChildCvterm();

    //look up a random record ID with a prop
    $query = db_select("chado.feature", 'F')
      ->fields('F', ['feature_id']);
    $query->join('chado.featureprop', 'FP', 'F.feature_id = FP.feature_id');

    $record_id = $query->execute()->fetchObject()->feature_id;

    $cvterms = $obj->get_all_associated_cvterms("feature", $record_id);


    $this->arrayHasKey("what should this be now?", $cvterms);

    //we don't check prop values because that's not in chado 1.4 yet.

    //throwing a random library entity combo should return null
    $this->assertNull($obj->get_all_associated_cvterms("library", 10));
  }

  public function test_associate_entity_across_ancestors() {

    //look up a random record ID children in cvtermpath
    $query = db_select("chado.featureprop", 'FP')
      ->fields('FP', ['type_id']);

    $query->join('chado.cvtermpath', 'CVTP', 'FP.type_id = CVTP.subject_id');

    $cvterm_id = $query->execute()->fetchObject();

    if (!$cvterm_id) {
      print ("\nwarning: No featureprops with relational cvterms to check\n");

      return;
    }

    $obj = new ChadoEntityChildCvterm();
    $ancestors = $obj->fetch_ancestors($cvterm_id->type_id); //prop_type parameters are defined in get_all_associated_cvterms

    $this->assertNotEmpty($ancestors);

  }


  //WARNING: this test runs the update all entities job!
  //It's going to take a long time!  Run it to populate your entity term index...
  public function tests_update_all_entities() {

    //look up cvterm for mrna

    $mrna_id = db_select("chado.cvterm", "CVT")
      ->fields('CVT', ['cvterm_id'])
      ->condition('CVT.name', 'mRNA')
      ->execute()->fetchField();


    $organism = factory('chado.organism')->create();

    $features = factory('chado.feature', 100)->create([
      'organism_id' => $organism->organism_id,
      'type_id' => $mrna_id,
    ]);

    $related_terms = $this->set_fake_cvterm_relationships();
    //randomly assign these features some props with relationships

    factory('chado.feature_cvterm')->create();

    foreach ($features as $feature) {
      $term = $related_terms[array_rand($related_terms)];

      factory('chado.feature_cvterm')->create([
        'feature_id' => $feature->feature_id,
        'cvterm_id' => $term->cvterm_id,
      ]);
    }

    $obj = new ChadoEntityChildCvterm();
       $obj->update_all_entities();

    $result = db_select("public.tripal_cvterm_entity_index", 't')
      ->fields('t')
      ->execute();

    $this->assertNotEquals($result->rowCount(), 0);
  }

  private function set_fake_cvterm_relationships() {
    //Add some fake cvterms with relationships

    $cv = factory("chado.cv")->create();

    $cvterms = factory("chado.cvterm", 20)->create([
      'cv_id' => $cv->cv_id,
    ]);

    $cvterms_to_pop = $cvterms;

    $count = 0;
    while ($count < 10) {
      $subject = array_pop($cvterms_to_pop)->cvterm_id;
      $object = array_pop($cvterms_to_pop)->cvterm_id;

      $cvterm_relationships = factory('chado.cvterm_relationship')->create([
        'subject_id' => $subject,
        'object_id' => $object,
      ]);

      $count++;
    }
    return $cvterms;

  }

}