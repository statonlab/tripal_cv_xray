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

    if (!isset($this->instance['settings']['container'])) {
      return NULL;
    }

    $settings = $this->instance['settings']['container'];


    // $cv_id = $settings['CV'];
    $cv_shortname = $settings['ontology'];


    $target_bundle = $settings['target_bundle'];

    //TODO:  Inconsistent.  Do we allow single or multiple target bundles?

    $child_entities = tripal_cv_xray_fetch_associated_entities($entity, $target_bundle);
    //this is an array with key 1 = bundle, key 2 = entity ids. (in case we need to filter by bundle type).

    //just entities
    $entity->{$field_name}['und'][0]['value'] = $child_entities;
    $entity->{$field_name}['und'][0]['cv']['short_name'] = $cv_shortname;
    $entity->{$field_name}['und'][0]['target'] = $target_bundle;
  }

  public function instanceSettingsForm() {


    $bundle = $this->instance["bundle"];
    $bundle_info = tripal_load_bundle_entity(['name' => $bundle]);
    $anchor_base_table = $bundle_info->data_table;

    $settings = $this->instance['settings'];
    $element['container'] = [
      '#type' => 'fieldset',
      '#title' => $this->field["field_name"],
    ];

    //We could be way more sophisticated here...
    if ($anchor_base_table != "organism" || $anchor_base_table == "analysis") {
      $element['container']['target_bundle'] = array(
        '#markup' => '<b>Error:</b>
          The base table for this bundle, ' . $anchor_base_table . ', is not configured to work with this field.
          Please attach this field to an organism or analysis bundle instead.',
      );

      return $element;
    }

    $element['container']['instructions'] = array(
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

    $ontologies = db_select("public.tripal_cvterm_entity_index", "t")
      ->fields("t", ["database"])
      ->distinct()
      ->execute()
      ->fetchAssoc();


    $element['container']['ontology'] = array(
      '#type' => 'select',
      '#title' => 'Ontology',
      '#description' => 'The ontology you would like to map entities to.',
      '#options' => $ontologies,
    );

    $query = db_select("public.tripal_cvterm_entity_index", "tcei")
      ->fields("tcei", ["bundle_id"])
      ->distinct();
    $query->join("public.tripal_bundle", "tb", "tb.term_id = tcei.bundle_id");
    $query->fields("tb", ["label"]);
    $bundles = $query->execute()
      ->fetchAll();
    $target_bundles = [];

    foreach ($bundles as $bundle) {
  $bundle_id = $bundle->bundle_id;
  $label = $bundle->label;
  $target_bundles[$bundle_id] = $label;
    }

    if ($anchor_base_table == "organism" || $anchor_base_table == "analysis") {
    //  $base_tables = ["feature", "biomaterial"];
    }
    
    $element['container']['target_bundle'] = array(
      '#type' => 'select',
      '#title' => 'Target Bundle to Map Onto CV Tree',
      '#description' => 'The entity type you are mapping.  For example, if mapping features to an organism, you would select the feature bundle here.',
      '#options' => $target_bundles,
    );

    return $element;
  }

}
