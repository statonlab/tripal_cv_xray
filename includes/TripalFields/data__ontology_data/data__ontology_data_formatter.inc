<?php

/**
 * @class
 * Purpose:
 *
 * Display:
 * Configuration:
 */
class data__ontology_data_formatter extends TripalFieldFormatter{

  // The default label for this field.
  public static $default_label = 'Ontology data';

  // The list of field types for which this formatter is appropriate.
  public static $field_types = ['data__ontology_data'];

  // The list of default settings for this formatter.
  public static $default_settings = [
    'setting1' => 'default_value',
  ];

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
   * This function provides the display for a field when it is viewed on
   * the web page. The content returned by the formatter should only include
   * what is present in the $items[$delta]['values] array. This way, the
   * contents that are displayed on the page, via webservices and downloaded
   * into a CSV file will always be identical. The view need not show all
   * of the data in the 'values' array.
   *
   * @param $element
   * @param $entity_type
   * @param $entity
   * @param $langcode
   * @param $items
   * @param $display
   *
   * @return void
   *    An element array compatible with that returned by the
   *    hook_field_formatter_view() function.
   */
  public function view(
    &$element,
    $entity_type,
    $entity,
    $langcode,
    $items,
    $display
  ) {
    if (!isset($items[0])) {
      return NULL;
    }

    elseif (!$items[0]['value']) {
      return NULL;
    }

    // $child_entities = $items[0]['value'];
    $vocabulary_name = $items[0]['cv']['short_name'];
    $target_bundle_id = $items[0]['target'];

    // Add support for our custom tree viewer
    $wrapper_id = uniqid();
    drupal_add_js([
      'cv_lookup' => [
        [
          'vocabulary' => $vocabulary_name,
          'anchor_id' => $entity->id,
          'target_bundle_id' => $target_bundle_id,
          'wrapper_id' => $wrapper_id,
          'field_name' => $this->field['field_name'],
        ],
      ],
    ], 'setting');
    drupal_add_css(drupal_get_path('module',
        'tripal') . '/theme/css/tripal.cv_lookup.css');
    drupal_add_js(drupal_get_path('module',
        'tripal_cv_xray') . '/theme/js/tripal.cv_lookup.js', 'file');

    if (isset($_GET['page']) && $_GET['page'] >= 1) {
      $element[0] = static::loadRootTree($this->field['field_name'], TRUE,
        $vocabulary_name, $target_bundle_id, $entity->id);
      return;
    }

    $element[0] = [
      '#markup' => '<div id="' . $wrapper_id . '">Loading browser. Please wait. <i class="fa fa-spin fa-refresh"></i></div>',
    ];
  }

  /**
   * Get's and renders the root tree.
   *
   * @throws \Exception
   *
   * This prints a json response.
   */
  public static function loadRootTree(
    $field_name,
    $pager = FALSE,
    $vocabulary_name = NULL,
    $target_bundle_id = NULL,
    $entity_id = null
  ) {
    if (is_null($vocabulary_name)) {
      $vocabulary_name = isset($_GET['vocabulary']) ? $_GET['vocabulary'] : '';
    }
    if (is_null($target_bundle_id)) {
      $target_bundle_id = isset($_GET['target_bundle_id']) ? $_GET['target_bundle_id'] : '';
    }
    if (is_null($entity_id)) {
      $entity_id = isset($_GET['anchor_id']) ? $_GET['anchor_id'] : '';
    }
    $path = isset($_GET['path']) ? $_GET['path'] : '';

    // Short name
    $vocab = tripal_get_vocabulary_details($vocabulary_name);

    // If we can't find the term then just return a message.
    if (!$vocab) {
      drupal_json_output([
        'error' => 'The vocabulary ' . $vocabulary_name . ' cannot be found on this site',
        'content' => NULL,
      ]);

      return;
    }

    // Get the root terms
    $has_root = TRUE;
    $root_terms = tripal_get_vocabulary_root_terms($vocabulary_name);
    if (count($root_terms) == 0) {
      $root_terms = tripal_get_vocabulary_terms($vocabulary_name, 25);
      $has_root = FALSE;
    }

    $counts = tripal_cv_xray_lookup_entities_for_terms_count($root_terms,
      $target_bundle_id, $entity_id);
    $items = tripal_cv_xray_field_lookup_term_children_format($entity_id,
      $root_terms, $counts, $target_bundle_id);

    if (count($root_terms) == 0) {
      $items = '<p>This vocabulary has no terms loaded</p>';
    }
    else {
      //Bundle label for target mapped entity bundle
      $record_type = db_select('public.tripal_bundle', 't')
        ->fields('t', ['label'])
        ->condition('t.id', $target_bundle_id)
        ->execute()
        ->fetchField();

      $items = '<p><strong>Ontology: </strong>' . $vocabulary_name . '</p>
                <p><strong>Record type: </strong> ' . $record_type . '</p>
                <p>Click the + icon (if present) to expand nodes in the tree.
                 To the right of each node is a count of the number of
                 ' . $record_type . ' records associated with this term, or any
                 child term, for this organism. If no records are mapped to the
                 term or its children, the node will only display the ontology
                 term.</p><p> Click on the records to view those individual
                 records or create a collection.</p>' . $items;
    }

    $content = [
      'vocab_browser' => [
        '#type' => 'item',
        '#title' => 'Term Browser',
        '#markup' => $items,
      ],
    ];

    if (!$has_root) {
      $content['pager'] = [
        '#type' => 'markup',
        '#markup' => static::ajaxifyStaticPager(theme('pager'), $entity_id,
          $field_name),
      ];
    }

    if (!$pager) {
      drupal_json_output([
        'error' => FALSE,
        'content' => render($content),
      ]);
    }
    else {
      return $content;
    }
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

  protected static function ajaxifyStaticPager(
    $pager,
    $entity_id,
    $field_name
  ) {
    $field_id = 'tripal-entity-' . $entity_id . '--' . $field_name;

    $pager = preg_replace('/href="\/(.*)page=([0-9]+)(.*)?"/',
      'href="javascript:void(0)" onclick="tripal_navigate_field_pager(\'' . $field_id . '\', $2)"',
      $pager);

    $pager = preg_replace('/href="\/.+"/',
      'href="javascript:void(0)" onclick="tripal_navigate_field_pager(\'' . $field_id . '\', 0)"',
      $pager);

    $pager = '<img src="/' . drupal_get_path('module',
        'tripal') . '/theme/images/ajax-loader.gif" id="' . $field_id . '-spinner" class="tripal-field-ajax-spinner">' . $pager;

    return $pager;
  }
}
