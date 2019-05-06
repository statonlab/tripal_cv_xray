<?php

/**
 * @class
 * Purpose:
 *
 * Data:
 * Assumptions:
 */
class local__analysis_ontology_linker extends ChadoField{

  // --------------------------------------------------------------------------
  //                     EDITABLE STATIC CONSTANTS
  //
  // The following constants SHOULD be set for each descendant class.  They are
  // used by the static functions to provide information to Drupal about
  // the field and it's default widget and formatter.
  // --------------------------------------------------------------------------

  // The default label for this field.
  public static $default_label = 'Ontology Browser';

  // The default description for this field.
  public static $default_description = 'Link to ontology browser on organism page';

  // The default widget for this field.
  public static $default_widget = 'local__analysis_ontology_linker_widget';

  // The default formatter for this field.
  public static $default_formatter = 'local__analysis_ontology_linker_formatter';

  // The module that manages this field.
  // If no module manages the field (IE it's added via libraries)
  // set this to 'tripal_chado'
  public static $module = 'tripal_cv_xray';

  // A list of global settings. These can be accessed within the
  // globalSettingsForm.  When the globalSettingsForm is submitted then
  // Drupal will automatically change these settings for all fields.
  // Once instances exist for a field type then these settings cannot be
  // changed.
  public static $default_settings = [
    'storage' => 'field_chado_storage',
    // It is expected that all fields set a 'value' in the load() function.
    // In many cases, the value may be an associative array of key/value pairs.
    // In order for Tripal to provide context for all data, the keys should
    // be a controlled vocabulary term (e.g. rdfs:type). Keys in the load()
    // function that are supported by the query() function should be
    // listed here.
    'searchable_keys' => [],
  ];

  // Indicates the download formats for this field.  The list must be the
  // name of a child class of the TripalFieldDownloader.
  public static $download_formatters = [
    'TripalTabDownloader',
    'TripalCSVDownloader',
  ];

  // Provide a list of instance specific settings. These can be access within
  // the instanceSettingsForm.  When the instanceSettingsForm is submitted
  // then Drupal with automatically change these settings for the instance.
  // It is recommended to put settings at the instance level whenever possible.
  // If you override this variable in a child class be sure to replicate the
  // term_name, term_vocab, term_accession and term_fixed keys as these are
  // required for all TripalFields.
  public static $default_instance_settings = [
    // The DATABASE name, as it appears in chado.db.  This also builds the link-out url.  In most cases this will simply be the CV name.  In some cases (EDAM) this will be the SUBONTOLOGY.
    'term_vocabulary' => 'local',
    // The name of the term.
    'term_name' => 'analysis_ontology_linker',
    // The unique ID (i.e. accession) of the term.
    'term_accession' => 'analysis_ontology_linker',
    // Set to TRUE if the site admin is not allowed to change the term
    // type, otherwise the admin can change the term mapped to a field.
    'term_fixed' => FALSE,
    // Indicates if this field should be automatically attached to display
    // or web services or if this field should be loaded separately. This
    // is convenient for speed.  Fields that are slow should for loading
    // should have auto_attach set to FALSE so tha their values can be
    // attached asynchronously.
    'auto_attach' => FALSE,
    // The table in Chado that the instance maps to.
    'chado_table' => '',
    // The column of the table in Chado where the value of the field comes from.
    'chado_column' => '',
    // The base table.
    'base_table' => '',
  ];

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
   * @throws \Exception
   * @see ChadoField::load()
   */
  public function load($entity) {
    $entity->local__analysis_ontology_linker['und'] = NULL;

    // ChadoFields automatically load the chado column specified in the
    // default settings above. If that is all you need then you don't even
    // need to implement this function. However, if you need to add any
    // additional data to be used in the display, you should add it here.
    if (!db_table_exists('chado.organism_analysis')) {
      return;
    }

    if ($entity->chado_table !== 'analysis') {
      return NULL;
    }

    $analysis = $entity->chado_record;

    // Get related organisms
    $organisms = $this->getOrganisms($analysis);
    $bundles = $this->getFeatureBundles();

    if ($this->shouldAttach($organisms, $bundles)) {
      $data = [];

      foreach ($organisms as $organism) {
        $eid = chado_get_record_entity_by_table('organism', $organism);
        $data[] = [
          'value' => [
            'title' => $this->getEntityTitle($eid),
            'entity_id' => $eid,
          ],
        ];
      }

      $entity->local__analysis_ontology_linker['und'] = $data;
    }
  }

  /**
   * @param $entity_id
   *
   * @return mixed
   */
  public function getEntityTitle($entity_id) {
    return db_query('SELECT title FROM {tripal_entity} WHERE id=:id', [
      ':id' => $entity_id,
    ])->fetchField();
  }

  /**
   * @param array $organisms
   * @param array $feature_bundles
   *
   * @return bool
   */
  public function shouldAttach(array $organisms, array $feature_bundles) {
    if (empty($organisms)) {
      return FALSE;
    }

    foreach ($feature_bundles as $bundle) {
      $bundle_table = 'chado_bio_data_' . $bundle;
      $query = db_select('chado.feature', 'F');
      $query->fields('F');
      $query->join($bundle_table, 'BT', 'F.feature_id = BT.record_id');
      $query->join('tripal_cvterm_entity_linker', 'TCEL', 'TCEL.entity_id = BT.entity_id');
      $query->condition('F.organism_id', $organisms, 'IN');
      if ((int) $query->countQuery()->execute()->fetchField() > 0) {
        return TRUE;
      }
    }

    return FALSE;
  }

  /**
   * @return array Array of integers
   */
  public function getFeatureBundles() {
    $bundles = db_query('SELECT bundle_id FROM {chado_bundle} WHERE data_table = :table', [
      ':table' => 'feature',
    ])->fetchAll();

    return array_map(function ($bundle) {
      return $bundle->bundle_id;
    }, $bundles);
  }

  /**
   * @param $analysis
   *
   * @return mixed
   * @throws \Exception
   */
  public function getOrganisms($analysis) {
    $organisms = chado_query('SELECT OA.organism_id FROM {organism_analysis} OA
                        WHERE analysis_id=:id', [
      ':id' => $analysis->analysis_id,
    ])->fetchAll();

    return array_map(function ($organism) {
      return $organism->organism_id;
    }, $organisms);
  }

  /**
   * @see ChadoField::query()
   *
   */
  public function query($query, $condition) {
  }

  /**
   * @see ChadoField::queryOrder()
   */
  public function queryOrder($query, $order) {
  }

  /**
   * @see ChadoField::elementInfo()
   *
   **/

  public function elementInfo() {
    $field_term = $this->getFieldTermID();
    return [
      $field_term => [
        'operations' => ['eq', 'ne', 'contains', 'starts'],
        'sortable' => TRUE,
        'searchable' => TRUE,
      ],
    ];
  }
}


