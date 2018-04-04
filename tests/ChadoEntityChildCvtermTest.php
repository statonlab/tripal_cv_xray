<?php

use PHPUnit\Framework\TestCase;

class entity_child_cvterm_test extends TestCase {

  static $obj;

  protected function setUp() {

    $this->obj = new ChadoEntityChildCvterm();

  }

  public function test_dummy() {
    $this->assertTrue(TRUE);
  }


  public function test_get_all_associated_cvterms() {
    $obj = $this->obj;

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

  public function test_associate_entity_across_ancestors(){

//look up a random record ID children in cvtermpath
    $query = db_select("chado.featureprop", 'FP')
      ->fields('FP', ['type_id']);

    $query->join('chado.cvtermpath', 'CVTP', 'FP.type_id = CVTP.subject_id');

    $cvterm_id = $query->execute()->fetchObject();

    if (!$cvterm_id){
      print ("\nwarning: No featureprops with relational cvterms to check\n");
      return;
    }

    $obj = $this->obj;
    $ancestors = $obj->fetch_ancestors($cvterm_id->type_id); //prop_type parameters are defined in get_all_associated_cvterms

    $this->assertNotEmpty($ancestors);

  }

  //WARNING: this test runs the update all entities job!
  //It's going to take a long time!  Run it to populate your entity term index...
  public function tests_update_all_entities(){
    $obj = $this->obj;
    $obj->update_all_entities();

    $result = db_select("public.tripal_cvterm_entity_index", 't')->fields('t')->execute();

    $this->assertNotEquals($result->rowCount(), 0);
  }

  private function _add_feature_with_pathy_prop(){

    //insert mother term
    tripal_insert_cvterm();

    //insert child term

    //insert feature

    //add prop to feature

  }

  private function _clean_up_feature_with_pathy_prop(){

  }




  }