<?php

/**
 * @class
 * Purpose:
 *
 * Data:
 * Assumptions:
 */
class data__ontology_data extends TripalField {

  //static constants
  public static $default_label = 'Ontology data';
  public static $default_description = 'Ontology mappings of related entities.';
  public static $default_widget = 'data__ontology_data_widget';
  public static $default_formatter = 'data__ontology_data_formatter';
  public static $module = 'tripal_cv_xray';

  // A list of global settings. These can be accessed within the
  // globalSettingsForm.  When the globalSettingsForm is submitted then
  // Drupal will automatically change these settings for all fields.
  // Once instances exist for a field type then these settings cannot be
  // changed.
  public static $default_settings = array(
    'storage' => 'tripal_no_storage',
    // It is expected that all fields set a 'value' in the load() function.
    // In many cases, the value may be an associative array of key/value pairs.
    // In order for Tripal to provide context for all data, the keys should
    // be a controlled vocabulary term (e.g. rdfs:type). Keys in the load()
    // function that are supported by the query() function should be
    // listed here.
    'searchable_keys' => array(),
  );

  // Provide a list of instance specific settings. These can be access within
  // the instanceSettingsForm.  When the instanceSettingsForm is submitted
  // then Drupal with automatically change these settings for the instance.
  // It is recommended to put settings at the instance level whenever possible.
  // If you override this variable in a child class be sure to replicate the
  // term_name, term_vocab, term_accession and term_fixed keys as these are
  // required for all TripalFields.
  public static $default_instance_settings = array(
    // The DATABASE name, as it appears in chado.db.  This also builds the link-out url.  In most cases this will simply be the CV name.  In some cases (EDAM) this will be the SUBONTOLOGY.
    'term_vocabulary' => 'data',
    // The name of the term.
    'term_name' => 'Ontology data',
    // The unique ID (i.e. accession) of the term.
    'term_accession' => '4231',
    // Set to TRUE if the site admin is not allowed to change the term
    // type, otherwise the admin can change the term mapped to a field.
    'term_fixed' => FALSE,
    // Indicates if this field should be automatically attached to display
    // or web services or if this field should be loaded separately. This
    // is convenient for speed.  Fields that are slow should for loading
    // should have auto_attach set to FALSE so tha their values can be
    // attached asynchronously.
    'auto_attach' => FALSE,
  );

  // A boolean specifying that users should not be allowed to create
  // fields and instances of this field type through the UI. Such
  // fields can only be created programmatically with field_create_field()
  // and field_create_instance().
  public static $no_ui = FALSE;

  // A boolean specifying that the field will not contain any data. This
  // should exclude the field from web services or downloads.  An example
  // could be a quick search field that appears on the page that redirects
  // the user but otherwise provides no data.
  public static $no_data = FALSE;

  /**
   * Loads the field values from the underlying data store.
   *
   * @param $entity
   *
   * @return
   *   An array of the following format:
   *     $entity->{$field_name}['und'][0]['value'] = $value;
   *   where:
   *     - $entity is the entity object to which this field is attached.
   *     - $field_name is the name of this field
   *     - 'und' is the language code (in this case 'und' == undefined)
   *     - 0 is the cardinality.  Increment by 1 when more than one item is
   *       available.
   *     - 'value' is the key indicating the value of this field. It should
   *       always be set.  The value of the 'value' key will be the contents
   *       used for web services and for downloadable content.  The value
   *       should be of the follow format types: 1) A single value (text,
   *       numeric, etc.) 2) An array of key value pair. 3) If multiple entries
   *       then cardinality should incremented and format types 1 and 2 should
   *       be used for each item.
   *   The array may contain as many other keys at the same level as 'value'
   *   but those keys are for internal field use and are not considered the
   *   value of the field.
   */
  public function load($entity) {
    parent::load($entity);
    $entity_id = $entity->id;
    $field_name = $this->field['field_name'];
    $field_type = $this->field['type'];

    $entity->{$field_name}['und'][0]['value'] = NULL;

    $child_entities = $this->fetch_associated_features($entity);
    //this is an array with key 1 = bundle, key 2 = entity ids. (in case we need to filter by bundle type).


    $settings = $this->instance['settings'];
    $cv_id = $settings['CV'];

    $query = db_select("chado.db2cv_mview", 'DBCV')
      ->fields("DBCV", ['dbname'])
      ->condition('cv_id', $cv_id);
    $cv_shortname = $query->execute()->fetchField();

    dpm($cv_shortname);

    //retrieve cvterms from cvterm->entity index as DB:accession

    $target_bundle = $settings['target_bundle'];
    $child_entity_cvterms = [];

    //Look up all cvterm:entity mappings for this field's CV
    $query = db_select("chado.cvterm", 'CVT')
      ->fields("CVT", ["cvterm_id"])
      ->condition("cv_id", $cv_id);
    $query->join("public.tripal_cvterm_entity_index", "CEI", "CVT.cvterm_id = CEI.cvterm_id");
    $query->fields("CEI", [
      "bundle_id",
      "database",
      "accession",
      "entities_array",
    ])
      ->condition("bundle_id", $target_bundle);

    $results = $query->execute();

    //NEXT: filter the array to just entities mapped on this instance of the field
    foreach ($results as $result) {
      $cvterm_id = $result->cvterm_id;
      $entities_serial = $result->entities_array;
      $entities_with_term_info = unserialize($entities_serial);

      $target_bundle_table = "public.chado_bio_data_" . $target_bundle;
      $mapped_entities = $child_entities[$target_bundle_table];
      $matched = array_intersect_key($entities_with_term_info, $mapped_entities);

      //build storage key

      $key = $result->database . ":" . $result->accession;
      $child_entity_cvterms[$key] = $matched;
      //TODO: we're missing the titles.  They are in $mapped_entities, keyed by the entity ID (just like the other arrray)  We could loop through to get them...
    }

    $entity->{$field_name}['und'][0]['value'] = $child_entity_cvterms;
    $entity->{$field_name}['und'][0]['cv'] = $cv_shortname;

  }


  /**
   * This function should probably be moved out of the field and performed at
   * regular intervals and teh results stored in a table. For now, it runs on
   * load, and retrieves ALL ENTITIES associated with the anchor entity.
   *
   * @param $entity
   *
   * @return array
   */
  private function fetch_associated_features($entity) {
    $entity_id = $entity->id;
    // $bundle = tripal_load_bundle_entity(array('name' => $entity->bundle));

    $child_entities = [];
    $record_id = $entity->chado_record_id;

    //If organism, we'll load all the features associated with it in the type_id column
    if ($entity->chado_table == 'organism') {

      $query = db_select('public.chado_bundle', 'tb')
        ->fields('tb', ['bundle_id', 'type_id', 'data_table'])
        ->condition('data_table', 'feature', '=');
      $feature_bundles = $query->execute()->fetchAll();

      //TODO: refactor this.  we're only mapping one entity type!
      foreach ($feature_bundles as $fb) {
        $type_id = $fb->type_id;
        $bundle_id = $fb->bundle_id;
        $bundle_table = "public.chado_bio_data_" . $bundle_id;

        //fetch all features of that type associated with this organism
        $query = db_select($bundle_table, 'bt');
        $query->fields('bt', ['entity_id']);
        $query->join('chado.feature', "cf", 'bt.record_id = cf.feature_id');
        $query->join('public.tripal_entity', 'te', 'te.id = bt.entity_id');
        $query->fields('te', ['title']);
        $query->condition('cf.organism_id', $record_id, '=');
        $results = $query->execute()
          ->fetchAllAssoc('entity_id');//this ensures the key is the entity ID!

        if ($results) {
          $child_entities[$bundle_table] = $results;
        }
      }
    }

    return $child_entities;
  }


  public
  function instanceSettingsForm() {

    // Retrieve the current settings.
    $settings = $this->instance['settings'];

    //defaults...

//    if (!isset($settings['CV'])) {
//      $settings['CV'] = self::$default_settings['CV'];
//    }
//    if (!isset($settings['target_bundle'])) {
//      $settings['target_bundle'] = self::$default_settings['target_bundle'];
//    }


    //lacey warns people :P
//    $element['msg'] = array(
//      '#type' => 'markup',
//      '#markup' => '<div class="messages warning">Settings are only saved after you click "Update" within this box <br />AND "Save" at the bottom of the page!</div>',
//    );

    $element['instructions'] = array(
      '#type' => 'markup',
      '#markup' => '<div><p>The ontology data browser provides an interactive tree of a CV.  Records that relate to the entity this field displays on will appear on the tree.</p><p>
Before you can use this field, you must choose which CV to display, and what entity types will map to this bundle. Currently, the following mappings are supported: Organism <- feature.
</p> </div>',
    );

    $reasonable_cvs = [
      'INTERPRO',
      'EDAM',
      'biological_process',
      'molecular_function',
      'cellular_component',
      'sequence',
      'SIO',
    ];

    $results = db_select("chado.cv", 'CV')
      ->fields("CV", ["cv_id", "name"])
      ->condition("name", $reasonable_cvs, 'IN')
      ->execute();
    $cv_options = [];
    foreach ($results as $result) {
      $cv_options[$result->cv_id] = $result->name;
    }

    // Add in all the general settings
    //-------------------------------------------
    $element['CV'] = array(
      '#type' => 'select',
      '#title' => 'CV',
      '#description' => 'The CV you would like to map entities to.',
      '#options' => $cv_options,
    );

    //TODO: Look at qualify base tables for this instance, then get all entities.

    $base_tables = ["feature", "biomaterial"];


    $target_bundles = [];

    $query = db_select("public.chado_bundle", 'CB')
      ->fields("CB", ["bundle_id", "data_table"])
      ->condition("data_table", $base_tables, 'IN');
    $query->join("public.tripal_bundle", "TB", "CB.bundle_id = TB.id");
    $results = $query->fields("TB", ["label"])
      ->execute();

    foreach ($results as $result) {
      $target_bundles[$result->bundle_id] = $result->label;
    }

    $element['target_bundle'] = array(
      '#type' => 'select',
      '#title' => 'Target Bundle to Map Onto CV Tree',
      '#description' => 'The entity type you are mapping.  For example, if mapping features to an organism, you would select the feature bundle here.',
      '#options' => $target_bundles,
    );

    return $element;


  }

}
