<?php

class ChadoEntityChildCvterm {

  /**
   * Fully indexes all entity-CVterm associations.
   * Currently only feature and biomaterial entities are indexed.
   *
   */

  public function update_all_entities() {
    //First, empty db
    db_truncate("public.tripal_cvterm_entity_index")->execute();

    print("\nupdating all entity cvterm mappings.\n");
    $o_array = [];

    $bundles = $this->get_all_bundles();

    foreach ($bundles as $bundle) {

      //Right now we're only populating feature and biomaterial.
      //TODO: write more bundle cases.
      if ($bundle->data_table !== "feature" && $bundle->data_table !== "biomaterial") {
        continue;
      }
      print ("indexing $bundle->label\n");

      $n = 100; // Set appropriate chunk size for your dataset.
      $position = 0;

      $cbd_table = "public.chado_" . $bundle->name;

      $total = db_select($cbd_table, 'cbd')->fields('cbd', [
        NULL,
      ])->countQuery()->execute()->fetchField();
      //cast as int

      while ($position <= $total) {
        $cvterm_tracker = [];

        //all subjects are related to some object

        $memory = memory_get_usage();
        echo "\nMemory at position $position $memory bytes\r";

        $entities = db_select($cbd_table, 'cbd')->fields('cbd', [
          'entity_id',
          'record_id',
        ])->range($position, $n)
          ->execute()->fetchAll();

        //do your thing
        foreach ($entities as $entity) {
          $types = NULL;
          $eid = $entity->entity_id;
          $rid = $entity->record_id;
          //directly associated cvterms
          $cvterms = $this->get_all_associated_cvterms($bundle->data_table, $rid);
          if (!$cvterms) {
            continue;
          }
          foreach ($cvterms as $cvterm => $relationship) {
            $cvterm_tracker[$cvterm][$eid]["relationship"] = $relationship;
            $cvterm_tracker[$cvterm][$eid]["path_length"] = 0;
          }
        }
        $position += $n;


        //Loop through the cvterms, retrieve all child terms defined in cvtermpath, and add them in copying the relationship type
        foreach ($cvterm_tracker as $cvterm => $entity_list) {
          $children = $this->fetch_ancestors($cvterm);
          $entities = array_keys($entity_list);

          foreach ($children as $child_cvterm => $ancestor_info) {
            //add each individual ancestor term
            foreach ($entities as $entity) {
              $cvterm_tracker[$child_cvterm][$entity]["path_length"] = $ancestor_info["path_length"];
              $cvterm_tracker[$child_cvterm][$entity]["relationship"] = $cvterm_tracker[$cvterm][$entity]["relationship"];
            }
          }
        }


        //Insert into public.tripal_cvterm_entity_index

        foreach ($cvterm_tracker as $cvterm => $entities) {

          //look up db and accession for cvterm

          $query = db_select("chado.cvterm", "CVT");
          $query->join("chado.dbxref", "DBX", "CVT.dbxref_id = DBX.dbxref_id");
          $query->fields("DBX", ["accession"]);
          $query->join("chado.db", "DB", "DBX.db_id = DB.db_id");
          $query->fields("DB", ["name"]);
          $query->condition("CVT.cvterm_id", $cvterm);

          $full_term = $query->execute()->fetchObject();

          if ($full_term) {
            $accession = $full_term->accession;
            $database = $full_term->name;


            //Check if theres an entry for this info

            $pre_existing = db_select("public.tripal_cvterm_entity_index", 't')
              ->fields('t',
                ["entities_array"]
              )
              ->condition("cvterm_id", $cvterm)
              ->condition("bundle_id", $bundle->id)
              ->execute()->fetchField();

            if ($pre_existing) {
              $pre_existing = unserialize($pre_existing);
              $entities = array_merge($pre_existing, $entities);

              $serialized_entities = serialize($entities);

              db_update('public.tripal_cvterm_entity_index')
                ->fields([
                  "database" => $database,
                  "accession" => $accession,
                  "entities_array" => $serialized_entities,
                ])
                ->condition('bundle_id', $bundle->id)
                ->condition('cvterm_id', $cvterm)
                ->execute();
            }

            else {
              $serialized_entities = serialize($entities);

              db_insert("public.tripal_cvterm_entity_index")
                ->fields([
                  "cvterm_id" => $cvterm,
                  "database" => $database,
                  "accession" => $accession,
                  "bundle_id" => $bundle->id,
                  "entities_array" => $serialized_entities,
                ])
                ->execute();
            }
          }
        }
      }
    }
  }


  public function fetch_ancestors($cvterm) {

    if (!$cvterm) {
      print ("Error: No CVterm provided");

      return NULL;
    }

    $ancestors = [];


    $query = db_select("chado.cvtermpath", 't')
      ->fields('t', ['subject_id', 'pathdistance']);
    $query->condition('object_id', $cvterm);
    //all subjects are related to some object
    $results = $query->execute()->fetchAll();

    foreach ($results as $result) {
      $new_term = $result->subject_id;
      $path = $result->pathdistance;
      $ancestors[$new_term]["path_length"] = $path;
    }

    return $ancestors;

  }


  /**
   * Fetches all cvterms associated with a chado record.
   * Terms can be associated with a record in the following ways:
   *
   *  - Type ID in record table
   *  - Property type in recordprop table
   *  - Property cvalue in recordprop table
   *  - _cvterm tables
   *
   * @param $base_table - the chado base table (ie feature)
   * @param $record_id
   *
   * @return array|null  A nested array where each cvterm ID is a key, and the
   *   value is the relationship to the record. (type, prop_type, or
   *   prop_value)
   */

  public function get_all_associated_cvterms($base_table, $record_id) {
    $chado_table = "chado." . $base_table;
    $prop_table = "chado." . $base_table . "prop";
    $cvterm_table = "chado." . $base_table . "_cvterm";

    if ($base_table != 'feature' && $base_table != 'biomaterial') {
      return NULL;
    }
    $out = [];

    $id_column = $base_table . "_id";
    $records = $this->get_related_records($chado_table, $record_id);

    //1  the type_id in the base table
    if (db_field_exists($chado_table, "type_id")) {
      $query = db_select($chado_table, "CT");
      $query->fields("CT", ["type_id"])
        ->condition("CT.feature_id", $records, 'IN');
      $results = $query->execute()->fetchAll();

      foreach ($results as $result) {
        $out[$result->type_id] = "type";
      }
    }

    //2 the prop table info
    $prop_fields = ["type_id"];
    $prop_fields_alias = ["prop_type_id"];

    if (db_field_exists($prop_table, "cvalue_id")) {
      $prop_fields = ["type_id", "cvalue_id"];
      $prop_fields_alias = ["prop_type_id", "prop_cvalue_id"];
    }

    $query = db_select($prop_table, "P");
    $query->fields("P", $prop_fields, $prop_fields_alias)
      ->condition("P." . $id_column, $records, 'IN');
    $result = $query->execute()->fetchAll();
    if ($result) {
      foreach ($result as $column => $cvterm) {
        if ($cvterm->type_id) {
          $out[$cvterm->type_id] = "proptype";
        }
        if ($cvterm->cvalue_id) {
          $out[$cvterm->cvalue_id] = "propcval";
        }
      }
    }

    //3 the _cvterm table
    if (db_table_exists($cvterm_table)) {
      $query = db_select($cvterm_table, "CTCV");
      $query->fields("CTCV", ["cvterm_id"])
        ->condition("CTCV." . $id_column, $records, 'IN');

      $results = $query->execute()->fetchAll();
      if ($results) {
        foreach ($results as $column => $cvterm) {
          $out[$cvterm->cvterm_id] = "cvterm";
        }
      }
    }

    return $out;
  }

  /**
   * Gets array of all Tripal bundle entities
   *
   * @return array|bool
   */
  private function get_all_bundles() {
    $bundles = [];
    // Get all bundle names to cycle through.
    $all_bundles = db_select('tripal_bundle', 'tb')
      ->fields('tb', ['name'])
      ->execute()
      ->fetchAll();
    foreach ($all_bundles as $bundle_name) {
      // Get the bundle object.
      $bundle = tripal_load_bundle_entity(['name' => $bundle_name->name]);
      if (!$bundle) {
        tripal_report_error('tripal', TRIPAL_ERROR, "Unrecognized bundle name '%bundle'.",
          ['%bundle' => $bundle_name]);

        return FALSE;
      }
      array_push($bundles, $bundle);
    }

    return $bundles;
  }


  /**
   * Gets related records linked via _relationship linker table.
   *
   * @param $chado_table
   * @param $record_id
   *
   * @return array
   */
  private function get_related_records($chado_table, $record_id) {
    $records = [$record_id];
    $query = db_select($chado_table . "_relationship", "CR");
    $query->fields("CR", ["subject_id"])
      ->condition("CR.object_id", $record_id);
    $subjects = $query->execute()->fetchAll();

    foreach ($subjects as $subject) {
      $records[] = $subject->subject_id;
    }
    $query = db_select($chado_table . "_relationship", "CR");
    $query->fields("CR", ["object_id"])
      ->condition("CR.subject_id", $record_id);
    $objects = $query->execute()->fetchAll();

    foreach ($objects as $object) {
      $records[] = $object->object_id;
    }

    return $records;
  }


//
//  public function update_specific_entities($entities) {
//
//    foreach ($entities as $entity) {
//      $types = NULL;
//      $eid = $entity->entity_id;
//      $rid = $entity->record_id;
//      //directly associated cvterms
//
//      //data table needs to coem from entity instead
//
//      $cvterms = $this->get_all_associated_cvterms($bundle->data_table, $rid);
//      //why is this always NULL?
//      if (!$cvterms) {
//        continue;
//      }
//
//    }
//  }
}