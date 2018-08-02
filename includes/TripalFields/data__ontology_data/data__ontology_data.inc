<?php

/**
 * @class
 * Purpose:
 *
 * Data:
 * Assumptions:
 */
class data__ontology_data extends TripalField {

  /**
   * Field label.
   *
   * @var string
   */
  public static $default_label = 'Ontology data';

  /**
   * Description.
   *
   * @var string
   */
  public static $default_description = 'Ontology mappings of related entities.';

  /**
   * Widget class.
   *
   * @var string
   */
  public static $default_widget = 'data__ontology_data_widget';

  /**
   * Formatter class.
   *
   * @var string
   */
  public static $default_formatter = 'data__ontology_data_formatter';

  /**
   * Module name.
   *
   * @var string
   */
  public static $module = 'tripal_cv_xray';

  /**
   * A list of global settings. These can be accessed within the
   * globalSettingsForm.  When the globalSettingsForm is submitted then
   * Drupal will automatically change these settings for all fields.
   * Once instances exist for a field type then these settings cannot be
   * changed.
   *
   * @var array
   */
  public static $default_settings = [
    'storage' => 'tripal_no_storage',
    // It is expected that all fields set a 'value' in the load() function.
    // In many cases, the value may be an associative array of key/value pairs.
    // In order for Tripal to provide context for all data, the keys should
    // be a controlled vocabulary term (e.g. rdfs:type). Keys in the load()
    // function that are supported by the query() function should be
    // listed here.
    'searchable_keys' => [],
  ];

  /**
   * Provide a list of instance specific settings. These can be access within
   * the instanceSettingsForm.  When the instanceSettingsForm is submitted
   * then Drupal with automatically change these settings for the instance.
   * It is recommended to put settings at the instance level whenever possible.
   * If you override this variable in a child class be sure to replicate the
   * term_name, term_vocab, term_accession and term_fixed keys as these are
   * required for all TripalFields.
   *
   * @var array
   */
  public static $default_instance_settings = [
    // The DATABASE name, as it appears in chado.db.  This also builds the link-out url.  In most cases this will simply be the CV name.  In some cases (EDAM) this will be the SUBONTOLOGY.
    'term_vocabulary' => 'data',
    // The name of the term.
    'term_name' => 'Ontology data',
    // The unique ID (i.e. accession) of the term.
    'term_accession' => '2353',
    // Set to TRUE if the site admin is not allowed to change the term
    // type, otherwise the admin can change the term mapped to a field.
    'term_fixed' => FALSE,
    // Indicates if this field should be automatically attached to display
    // or web services or if this field should be loaded separately. This
    // is convenient for speed.  Fields that are slow should for loading
    // should have auto_attach set to FALSE so tha their values can be
    // attached asynchronously.
    'auto_attach' => FALSE,
  ];

  /**
   * A boolean specifying that users should not be allowed to create
   * fields and instances of this field type through the UI. Such
   * fields can only be created programmatically with field_create_field()
   * and field_create_instance().
   *
   * @var bool
   */
  public static $no_ui = FALSE;

  /**
   * A boolean specifying that the field will not contain any data. This
   * should exclude the field from web services or downloads.  An example
   * could be a quick search field that appears on the page that redirects
   * the user but otherwise provides no data.
   *
   * @var bool
   */
  public static $no_data = FALSE;

  /**
   * Loads the field values from the underlying data store.
   *
   * @param $entity
   *
   * @return void
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

    // Set default value
    $field_name = $this->field['field_name'];
    $entity->{$field_name}['und'][0]['value'] = NULL;

    if (!isset($this->instance['settings']['container'])) {
      return;
    }

    $settings = $this->instance['settings']['container'];
    $cv_shortname = $settings['ontology'];
    $target_bundle = $settings['target_bundle'];

    $set_value = $this->shouldAttachBrowser($entity, $target_bundle);

   if ( !$set_value) {
     unset($entity->{$field_name});
     return;
   }
    $entity->{$field_name}['und'][0]['value'] = $set_value;
    $entity->{$field_name}['und'][0]['cv']['short_name'] = $cv_shortname;
    $entity->{$field_name}['und'][0]['target'] = $target_bundle;
  }

  /**
   * Checks if the browser should be displayed for a given entity based
   * on the which target bundle is requested.
   *
   * @param object $entity Anchor entity
   * @param int $target_bundle_id bundle id
   *
   * @return bool|null
   */
  protected function shouldAttachBrowser($entity, $target_bundle_id) {
    $record_id = $entity->chado_record_id;

    $query = db_select('public.chado_bundle', 'tb');
    $query->fields('tb', ['bundle_id', 'data_table']);
    $query->condition('bundle_id', $target_bundle_id, '=');
    $target_info = $query->execute()->fetchObject();

    $bundle_id = $target_info->bundle_id;
    $target_chado_table = $target_info->data_table;
    $target_bundle_table = "public.chado_bio_data_" . $bundle_id;
    $results = NULL;
    $target_record_column = $target_chado_table . "_id";

    if ($entity->chado_table == 'organism') {
      $query = db_select($target_bundle_table, 'tbt');
      $query->join('chado.' . $target_chado_table, "tct", 'tbt.record_id = tct.' . $target_record_column);
      $query->join('public.tripal_entity', 'te', 'te.id = tbt.entity_id');

      if ($target_chado_table == 'biomaterial') {
        $query->condition('tct.taxon_id', $record_id, '=');
      }
      else {
        // Works with feature in mind, and will work when chado table has organism_id column
        $query->condition('tct.organism_id', $record_id, '=');
      }

      $query->join('public.tripal_cvterm_entity_linker', 'tcel', 'tcel.entity_id = tbt.entity_id');
      $results = ((int) $query->countQuery()->execute()->fetchField()) > 1;
    }

    if ($entity->chado_table == 'analysis') {
      return NULL;
    }

    return $results;
  }

  /**
   * Field settings form.
   *
   * @return array|mixed
   */
  public function instanceSettingsForm() {
    $settings = $this->instance['settings'];
    $bundle = $this->instance["bundle"];
    $bundle_info = tripal_load_bundle_entity(['name' => $bundle]);
    $anchor_base_table = $bundle_info->data_table;
    $element['container'] = [
      '#type' => 'fieldset',
      '#title' => $this->field["field_name"],
    ];

    // We could be way more sophisticated here...
    if ($anchor_base_table != "organism" || $anchor_base_table == "analysis") {
      $element['container']['target_bundle'] = [
        '#markup' => '<b>Error:</b>
          The base table for this bundle, ' . $anchor_base_table . ', is not configured to work with this field.
          Please attach this field to an organism or analysis bundle instead.',
      ];

      return $element;
    }

    $element['container']['instructions'] = [
      '#type' => 'markup',
      '#markup' => '<div>
                    <p>
                    The ontology data browser provides an interactive
                    tree of a CV.  Records that relate to the entity this field
                    displays on will appear on the tree.
                    </p>
                    <p>
                    Before you can use this field, you must choose which
                    CV to display, and what entity types will map to this
                    bundle. Currently, the following mappings are supported:
                    Organism <- feature.
                    </p>
                    </div>',
    ];

    $ontologies = db_select("tripal_cvterm_entity_linker", "t")
      ->fields("t", ["database"])
      ->distinct()
      ->execute()
      ->fetchAll();

    $ontology_options = [];
    foreach ($ontologies as $ontology) {
      $ontology_options[$ontology->database] = $ontology->database;
    }

    $element['container']['ontology'] = [
      '#type' => 'select',
      '#title' => 'Ontology',
      '#description' => 'The ontology you would like to map entities to.',
      '#options' => $ontology_options,
      '#default_value' => isset($settings['container']) ? $settings['container']['ontology'] : NULL,
    ];

    $query = db_select("chado_bundle", 'CB');
    $query->fields('CB', ['bundle_id']);
    $query->condition('CB.data_table', ['feature', 'biomaterial'], 'IN');
    $query->join('public.tripal_bundle', 'TB', 'CB.bundle_id = TB.id');
    $query->fields('TB', ['label']);
    $bundles = $query->execute()->FetchAll();
    $target_bundles = [];

    foreach ($bundles as $bundle) {
      $bundle_id = $bundle->bundle_id;
      $label = $bundle->label;
      $target_bundles[$bundle_id] = $label;
    }

    if ($anchor_base_table == "organism" || $anchor_base_table == "analysis") {
      //  $base_tables = ["feature", "biomaterial"];
    }

    $element['container']['target_bundle'] = [
      '#type' => 'select',
      '#title' => 'Target Bundle to Map Onto CV Tree',
      '#description' => 'The entity type you are mapping.  For example, if mapping features to an organism, you would select the feature bundle here.',
      '#options' => $target_bundles,
      '#default_value' => isset($settings['container']) ? $settings['container']['target_bundle'] : NULL,
    ];

    return $element;
  }
}
