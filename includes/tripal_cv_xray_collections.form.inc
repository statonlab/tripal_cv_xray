<?php

/**
 * Implements hook_form.
 */
function tripal_cv_xray_collections_form($form, &$form_state, $db_shortname, $accession, $anchor_id, $mapping_bundle_id) {
  $bundle_table = 'chado_bio_data_' . $mapping_bundle_id;
  $mapping_entity = tripal_load_bundle_entity(['id' => $mapping_bundle_id]);
  $record_field = $mapping_entity->data_table === 'bio_material' ? 'taxon_id' : 'organism_id';
  $anchor = tripal_load_entity('TripalEntity', [$anchor_id]);
  $anchor = reset($anchor);
  $term = tripal_get_term_details($db_shortname, $accession);
  $short_name = $term["vocabulary"]["short_name"];
  $bundle = 'bio_data_' . $mapping_bundle_id;

  $entities = db_query("SELECT TE.id, TE.title
              FROM {tripal_cvterm_entity_linker} TCEL
              INNER JOIN {tripal_entity} TE ON TE.id = TCEL.entity_id
              INNER JOIN {{$bundle_table}} CB ON CB.entity_id = TCEL.entity_id
              INNER JOIN {chado.{$mapping_entity->data_table}} CF ON CF.feature_id = CB.record_id
              WHERE CF.{$record_field} = :record_id 
                    AND TCEL.database = :database
                    AND TCEL.accession = :accession
              ORDER BY TCEL.entity_id ASC", [
    ':record_id' => $anchor->chado_record_id,
    ':database' => $short_name,
    ':accession' => $accession,
  ])->fetchAll();

  $form['bundle_type'] = ['#value' => $bundle, '#type' => 'value'];
  $form['entities'] = ['#value' => $entities, '#type' => 'value'];

  $field_ids = tripal_cv_xray_get_fields_as_options($bundle);

  $form['fields'] = [
    '#title' => t('Fields'),
    '#description' => t('Please select the fields to include in this data
      collection.'),
    '#type' => 'checkboxes',
    '#options' => $field_ids,
  ];

  $form['name'] = [
    '#title' => t('Name'),
    '#description' => t('The name for this collection.'),
    '#type' => 'textfield',
    '#required' => TRUE,
  ];

  $form['description'] = [
    '#title' => t('Description'),
    '#description' => t('Give an optional description for this collection.'),
    '#type' => 'textarea',
  ];

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => 'Create Collection',
  ];



  return $form;
}

/**
 * Form validation handler for tripal_cv_xray_collections_form().
 */
function tripal_cv_xray_collections_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $collection_name = $values['name'];

  if (!$collection_name) {
    $error_msg = t('Please set a collection name.');
    form_set_error($error_msg);
  }

  global $user;

  $collections = db_select('tripal_collection', 'tc')
    ->fields('tc', ['collection_id'])
    ->condition('uid', $user->uid)
    ->orderBy('tc.collection_name')
    ->execute();

  foreach ($collections as $collection) {
    if ($collection_name == $collection) {
      $error_msg = t('Collection name must be unique.');
      form_set_error($error_msg);
    }
  }
}

/**
 * Implements hook_form_submit.
 */
function tripal_cv_xray_collections_form_submit($form, &$form_state) {

  $values = $form_state['values'];
  $collection_name = $values['name'];
  $bundle_name = $values['bundle_type'];
  $fields = $values['fields'];
  $description = $values['description'];
  $entities = $values['entities'];

  global $user;

  tripal_create_collection([
    'uid' => $user->uid,
    'collection_name' => $collection_name,
    'bundle_name' => $bundle_name,
    'ids' => $entities,
    'fields' => $fields,
    'description' => $description,
  ]);
}

/**
 * Get options for downloadable fields.
 *
 * @credit Shamelessly copied from
 *          tripal/views_handlers/tripal_views_handler_area_collections.inc
 *
 * @param $bundle
 *
 * @return mixed
 */
function tripal_cv_xray_get_fields_as_options($bundle) {
  $fields = field_info_instances('TripalEntity', $bundle);

  $field_ids = [];

  foreach ($fields as $field_name => $instance) {
    $field = field_info_field($field_name);
    $field_type = $field['type'];
    if ($instance['field_name'] == 'entity_id') {
      continue;
    }

    // Skip hidden fields.
    if ($instance['display']['default']['type'] == 'hidden') {
      continue;
    }
    $field_label = $instance['label'];

    // Add in in any non CSV or Tab formatters to the label.
    $formatters = [];
    if (tripal_load_include_field_class($field_type)) {
      $instance_settings = $field_type::$default_instance_settings;
      if (array_key_exists('download_formatters', $instance_settings)) {
        foreach ($instance_settings['download_formatters'] as $class_name) {
          if ($class_name != 'TripalTabDownloader' and $class_name != 'TripalCSVDownloader') {
            tripal_load_include_downloader_class($class_name);
            $formatters[] = $class_name::$label;
          }
        }
      }
    }

    if (count($formatters) > 0) {
      $field_label .= ' (' . implode(',', $formatters) . ')';
    }

    // Add the field to those supported.
    $field_ids[$instance['field_id']] = t($field_label);
  }

  return $field_ids;
}
