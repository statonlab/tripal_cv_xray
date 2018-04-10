<?php

/**
 * @class
 * Purpose:
 *
 * Display:
 * Configuration:
 */
class data__ontology_data_formatter extends TripalFieldFormatter {

  // The default label for this field.
  public static $default_label = 'Ontology data';
  // The list of field types for which this formatter is appropriate.
  public static $field_types = array('data__ontology_data');

  private $entity_term_map = NULL;

  // The list of default settings for this formatter.
  public static $default_settings = array(
    'setting1' => 'default_value',
  );

  /**
   * Provides the field's setting form.
   *
   * This function corresponds to the hook_field_formatter_settings_form()
   * function of the Drupal Field API.
   *
   * The settings form appears on the 'Manage Display' page of the content
   * type administration page. This function provides the form that will
   * appear on that page.
   *
   * To add a validate function, please create a static function in the
   * implementing class, and indicate that this function should be used
   * in the form array that is returned by this function.
   *
   * This form will not be displayed if the formatter_settings_summary()
   * function does not return anything.
   *
   * param $field
   *   The field structure being configured.
   * param $instance
   *   The instance structure being configured.
   * param $view_mode
   *   The view mode being configured.
   * param $form
   *   The (entire) configuration form array, which will usually have no use
   *   here.  Typically for reference only.
   * param $form_state
   *   The form state of the (entire) configuration form.
   *
   * @return
   *   A Drupal Form array containing the settings form for this field.
   */
  public function settingsForm($view_mode, $form, &$form_state) {

  }

  /**
   *  Provides the display for a field
   *
   * This function corresponds to the hook_field_formatter_view()
   * function of the Drupal Field API.
   *
   *  This function provides the display for a field when it is viewed on
   *  the web page.  The content returned by the formatter should only include
   *  what is present in the $items[$delta]['values] array. This way, the
   *  contents that are displayed on the page, via webservices and downloaded
   *  into a CSV file will always be identical.  The view need not show all
   *  of the data in the 'values' array.
   *
   * @param $element
   * @param $entity_type
   * @param $entity
   * @param $langcode
   * @param $items
   * @param $display
   *
   * @return
   *    An element array compatible with that returned by the
   *    hook_field_formatter_view() function.
   */
  public function view(&$element, $entity_type, $entity, $langcode, $items, $display) {

    // Get the settings
    $settings = $display['settings'];

    if (!$items[0]['value']) {
      return NULL;
    }

    $child_entities = $items[0]['value'];
    $vocabulary_name = $items[0]['cv']['short_name'];
    $vocabulary_id = $items[0]['cv']['id'];
    $target_bundle_id = $items[0]['target'];

    if (!isset($child_entities['public.chado_bio_data_' . $target_bundle_id])){
      return NULL;
    }

    $vocab = tripal_get_vocabulary_details($vocabulary_name); //short name

    // If we can't find the term then just return a message.
    if (!$vocab) {
      drupal_set_message('The vocabulary ' . $vocabulary_name . ' cannot be found on this site', 'error');

      return '';
    }

    $has_root = TRUE;
    $root_terms = tripal_get_vocabulary_root_terms($vocabulary_name);
    if (count($root_terms) == 0) {
      $root_terms = tripal_get_vocabulary_terms($vocabulary_name, 25);
      $has_root = FALSE;
    }
    $onscreen_entity_maps = $this->lookup_entities_for_terms($root_terms, $target_bundle_id, $child_entities);

    $items = $this->field_lookup_term_children_format($entity->id, $root_terms, $onscreen_entity_maps);

    if (count($root_terms) == 0) {
      $items = '<p>This vocabulary has no terms loaded</p>';
    }
    else {
      $items = '<p>Click the + icon (if present) to expand the tree. If ' .
        'the full ontology or the term heirarchy is not loaded into this site, ' .
        'then the tree will consist of all terms at the same level. ' .
        'For some vocabularies, only a subset of terms are loaded</p>' . $items;
    }


    drupal_add_js(array(
      'tripal' => array(
        'cv_lookup' => array(
          'vocabulary' => $vocabulary_name,
          'entities' => $onscreen_entity_maps,
        ),
      ),
    ), 'setting');


    $content = array(
      'vocab_table' => array(
        '#type' => 'item',
        '#title' => 'Details',
        '#markup' => '<p>A vocabulary is always identified by its short name and sometimes it may offer multiple sub-vocabularies with different names. Both are listed below.</p>',
      ),
      'vocab_browser' => array(
        '#type' => 'item',
        '#title' => 'Term Browser',
        '#markup' => $items,
      ),
    );
    if (!$has_root) {
      $content['pager'] = array(
        '#type' => 'markup',
        '#markup' => theme('pager'),
      );
    }

    // Add support for our custom tree viewer
    drupal_add_css(drupal_get_path('module', 'tripal') . '/theme/css/tripal.cv_lookup.css');
    drupal_add_js(drupal_get_path('module', 'tripal') . '/theme/js/tripal.cv_lookup.js', 'file');

    $element[0] = $content;
  }


  /**
   * Given a list of children terms, format the tips of the cv tree.
   * The field specific functionality here is we
   *
   * @param $children
   *
   * @return string
   */

  function field_lookup_term_children_format($anchor_entity_id, $children, $onscreen_entity_maps) {
    $map = $onscreen_entity_maps;
    $items = '<ul id="tripal-cv-lookup-tree">';
    foreach ($children as $child) {

      $child_name = $child['vocabulary']['short_name'] . ":" . $child['accession'];

      $grand = tripal_get_term_children($child['vocabulary']['short_name'], $child['accession']);
      $num_grand = count($grand);
      $items .= '<li vocabulary = "' . $child['vocabulary']['short_name'] . '" ' .
        'accession = "' . $child['accession'] . '" ' .
        'children = "' . $num_grand . '" ' .
        'state = "closed" ' .
        'class = "cv-lookup-tree-node">';
      $class = 'tree-node-closed';
      if ($num_grand == 0) {
        $class = 'tree-node-single';
      }
      $items .= '<i class = "tree-node-icon ' . $class . '"></i>';
      $items .= l($child['name'], 'cv/lookup/' . $child['vocabulary']['short_name'] . '/' . $child['accession'], array('attributes' => array('target' => '_blank')));
      if ($child['accession'] != $child['name']) {
        $items .= ' [' . $child_name . '] ';
      }

      $child_mapping = NULL;
      if (isset($map[$child_name])) {
        $child_mapping = $map[$child_name];
      }
      $num_mapped = count($child_mapping);

      if ($num_mapped == 0) {
        $items .= "  No associated records.";
      }
      else {
        $record_word = "records.";
        if ($num_mapped == 1) {
          $record_word = "record.";
        }
        //TODO: we want to build a linkout here.  Entity_id isnt exposed for views though.
        $flattened_entities = implode(",", array_keys($child_mapping));
        $items .= l("  {$num_mapped} " . $record_word, 'cv_entities/' . $anchor_entity_id . '/' . $flattened_entities);
      }
      $items .= '</li>';
    }
    $items .= '</ul>';
    if (count($children) == 0) {
      $items = '';
    }

    return $items;
  }


  /**
   * Provides a summary of the formatter settings.
   *
   * This function corresponds to the hook_field_formatter_settings_summary()
   * function of the Drupal Field API.
   *
   * On the 'Manage Display' page of the content type administration page,
   * fields are allowed to provide a settings form.  This settings form can
   * be used to allow the site admin to define how the field should be
   * formatted.  The settings are then available for the formatter()
   * function of this class.  This function provides a text-based description
   * of the settings for the site developer to see.  It appears on the manage
   * display page inline with the field.  A field must always return a
   * value in this function if the settings form gear button is to appear.
   *
   * See the hook_field_formatter_settings_summary() function for more
   * information.
   *
   * @param $field
   * @param $instance
   * @param $view_mode
   *
   * @return string
   *   A string that provides a very brief summary of the field settings
   *   to the user.
   *
   */
  public function settingsSummary($view_mode) {
    return '';
  }

  /**
   * @param $terms
   * @param $target_bundle
   * @param $child_entities
   *
   * @return array
   */
  private function lookup_entities_for_terms($terms, $target_bundle, $child_entities) {

    $child_entity_cvterms = [];

    $target_bundle_table = "public.chado_bio_data_" . $target_bundle;

    foreach ($terms as $term) {
      $accession = $term["accession"];
      $short_name = $term["vocabulary"]["short_name"];

      $query = db_select("public.tripal_cvterm_entity_index", "CEI");
      $query->fields("CEI", [
        "entities_array",
      ])
        ->condition("bundle_id", $target_bundle)
        ->condition("database", $short_name)
        ->condition("accession", $accession);

      $ent_array = $query->execute()->fetchField();
      //Check if null here.

      if (!$ent_array) {
        continue;
      }

      $entities_with_term_info = unserialize($ent_array);
      //NEXT: filter the array to just entities mapped on this instance of the field
      $mapped_entities = $child_entities[$target_bundle_table];
      $matched = array_intersect_key($entities_with_term_info, $mapped_entities);

      $key = $short_name . ":" . $accession;
      $child_entity_cvterms[$key] = $matched;
    }

    return ($child_entity_cvterms);
  }
}