<?php

/**
 * @class
 * Purpose:
 *
 * Display:
 * Configuration:
 */
class local__analysis_ontology_linker_formatter extends ChadoFieldFormatter{

  // The default label for this field.
  public static $default_label = 'Ontology Browser';

  // The list of field types for which this formatter is appropriate.
  public static $field_types = ['local__analysis_ontology_linker'];

  // The list of default settings for this formatter.
  public static $default_settings = [
    'setting1' => 'default_value',
  ];

  /**
   * @see ChadoFieldFormatter::settingsForm()
   *
   **/

  public function settingsForm($view_mode, $form, &$form_state) {

  }

  /**
   * @see ChadoFieldFormatter::View()
   */
  public function view(&$element, $entity_type, $entity, $langcode, $items, $display) {
    foreach ($items as $item) {
      $element[] = [
        '#type' => 'item',
        '#markup' => l('View ontology browser for ' . $item['value']['title'], "bio_data/{$item['value']['entity_id']}?tripal_pane=group_ontology"),
      ];
    }
  }

  /**
   * @see ChadoFieldFormatter::settingsSummary()
   *
   **/

  public function settingsSummary($view_mode) {
    return '';
  }
}
