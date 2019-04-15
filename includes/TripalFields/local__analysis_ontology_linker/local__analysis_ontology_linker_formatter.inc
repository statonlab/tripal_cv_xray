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

      $go_link = l("Gene Ontology browser", "bio_data/{$item['value']['entity_id']}", [
        'query' => [
          'tripal_pane' => 'group_ontology',
        ],
      ]);

      $kegg_link = l("KEGG Ontology browser", "bio_data/{$item['value']['entity_id']}", [
        'query' => [
          'tripal_pane' => 'group_kegg_browser',
        ],
      ]);

      $content = "View $go_link or $kegg_link for {$item['value']['title']}";

      $element[] = [
        '#type' => 'item',
        '#markup' => $content,
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
