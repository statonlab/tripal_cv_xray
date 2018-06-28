<?php


/**
 * Form for cvterm entity association job submission.
 *
 * @param $form
 * @param $form_state
 *
 * @return mixed
 */
function tripal_cv_xray_cvterm_entity_form($form, &$form_state) {
  $welcome = '<h3>Tripal CV X-ray admin</h3>
<p>Welcome to the Tripal CV X-ray module.  When properly configured, this module allows you to create browsable CV tree fields.  When attached to a bundle, it can map entities associated with that bundle entity onto the tree.  For example, an organism can have a GO cv tree, with genes associated with the terms mapped onto it.  Users can explore terms and easily find and view the entities associated with various terms.
</p>
<p>To begin, you must populate the Entity: CVterm mapping index.  This associates the entities that will be mapped with CVterms.
</p>
  ';
  $form['welcome'] = [
    '#markup' => $welcome,
  ];

  //create table of previously indexed bundles

  $headers = [
    'DB Short Name',
    'Number of Total Entities',
    'Entities By Bundle',
  ];
  $rows = [];
  $dbs = tripal_cv_xray_retrieve_indexed_dbs();

  if (count($dbs) > 0) {
    foreach ($dbs as $db) {


      $name = $db['name'];
      $sub_count = tripal_cv_xray_count_indexed_entities($name);

      $count = $db['count'];

      $cbyb = theme('table', [
        'header' => ['Bundle', 'Count'],
        'rows' => $sub_count,
      ]);
      $rows[] = [$name, $count, $cbyb];
    }

    $form['entity_table'] = [
      '#type' => 'markup',
      '#markup' => theme('table', ['header' => $headers, 'rows' => $rows]),
    ];
  }


  //get all site vocabs

  $vocabs = tripal_get_vocabularies();

  $rows = [];

  foreach ($vocabs as $vocab) {

    if ($vocab['num_terms'] < 10) {
      continue;
    }

    $rows[$vocab['short_name']] = [
      $vocab['name'],
      $vocab['short_name'],
      $vocab['description'],
    ];
  }


  $header = ['Name', 'Short Name', 'Description'];

  $form['cv_select'] = [
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#prefix' => '<p>Select a CV to index.  Only index CVs which you will create browsers for.</p>',
  ];


  $supported_tables = ['biomaterial', 'feature'];

  $query = db_select("public.chado_bundle", 'CB')
    ->fields('CB', ['bundle_id']);
  $query->join('public.tripal_bundle', 'TB', 'CB.bundle_id = TB.id');
  $query->fields('TB', ['label']);
  $query->condition('CB.data_table', $supported_tables, 'IN');
  $bundles = $query->execute()->FetchAll();

  $bundle_rows = [];

  foreach ($bundles as $bundle) {
    $bundle_id = $bundle->bundle_id;
    $label = $bundle->label;
    $bundle_rows[$bundle_id] = [$label];
  }

  $form['bundle_select'] = [
    '#type' => 'tableselect',
    '#header' => ['Bundle Label'],
    '#options' => $bundle_rows,
    '#prefix' => '<p>Select a bundle to index.  These are the entities which will be associated with nodes on the CV tree.  For example, if mapping genes to an organism, select the gene bundle.</p>',
  ];


  $form['submit'] = [
    '#type' => 'submit',
    '#value' => 'Populate All',
  ];

  return $form;
}


/**
 * Hook validate for form.
 *
 * @param $form
 * @param $form_state
 */
function tripal_cv_xray_cvterm_entity_form_validate($form, &$form_state) {
  $bundle_ids = $form_state['values']['bundle_select'];
  $cvs = $form_state['values']['cv_select'];

  $bundle_ids = array_filter($bundle_ids);
  $cvs = array_filter($cvs);

  if (count($bundle_ids) == 0) {
    form_set_error("bundle_select", "please select at least one bundle to index.");

  }
  if (count($cvs) == 0) {
    form_set_error("cv_select", "please select at least one CV to index.");
  }
}

/**
 * Hook submit for form.
 *
 * @param $form
 * @param $form_state
 */
function tripal_cv_xray_cvterm_entity_form_submit($form, &$form_state) {

  $bundle_ids = $form_state['values']['bundle_select'];
  $cvs = $form_state['values']['cv_select'];


  $bundle_ids = array_keys(array_filter($bundle_ids));
  //Note: this is the SHORT NAME.  IE THE CHADO DB NAME.
  $cvs = array_keys(array_filter($cvs));


  //First, empty the table.  We MUST do this currently, otherwise you'll never be able to remove CV/entity combos.
  //TODO: It would be better if one could set CV/entity combos individually, IE only index GO terms for mRNAs, but sequence INTERPRO for biomaterials.

  db_query('TRUNCATE {tripal_cv_xray_config}');

  tripal_cv_xray_save_db_bundle_config($bundle_ids, $cvs);

  $job = new XRayDispatcherJob($bundle_ids, $cvs, 500);

  XRayQueue::dispatch($job);

  drupal_set_message('Dispatcher job launched successfully.');
  drupal_set_message('You may run the jobs using `drush cron`');
}

/**
 * Create a bundle config record.
 *
 * @param $bundle_ids
 * @param $db_names
 *
 * @throws \Exception
 */
function tripal_cv_xray_save_db_bundle_config($bundle_ids, $db_names) {
  foreach ($db_names as $db_name) {
    foreach ($bundle_ids as $bundle_id) {
      $result = db_select('tripal_cv_xray_config', 'TCXC')
        ->fields('TCXC', ['cv_xray_config_id'])
        ->condition('shortname', $db_name)
        ->condition('bundle_id', $bundle_id)
        ->execute()
        ->fetchField();

      if (!$result) {
        db_insert('tripal_cv_xray_config')
          ->fields([
            'shortname' => $db_name,
            'bundle_id' => $bundle_id,
          ])
          ->execute();
      }
    }
  }
}

function tripal_cv_xray_retrieve_indexed_dbs() {

  $list = [];
  $query = db_select('public.tripal_cvterm_entity_linker', 't');
  $query->fields('t', ['database']);
  $query->distinct();
  $results = $query->execute()->fetchAll();

  foreach ($results as $item) {
    $database = $item->database;

    $query = db_select('public.tripal_cvterm_entity_linker', 't');
    $query->fields('t', ['entity_id']);
    $query->distinct();
    $query->condition('database', $database);
    $count = $query->countQuery()->execute()->fetchField();

    $list[$database] = [
      'name' => $database,
      'count' => $count,
    ];
  }
  return $list;

}

function tripal_cv_xray_count_indexed_entities($short_name) {

  $count = NULL;
  $list = [];
  $query = db_select('public.tripal_cvterm_entity_linker', 't');
  $query->join('public.tripal_entity', 'e', 't.entity_id = e.id');
  $query->join('public.tripal_bundle', 'tb', 'e.term_id = tb.term_id');
  $query->fields('tb', ['label']);
  $query->condition('database', $short_name);
  $query->distinct();
  $bundles = $query->execute()->fetchAll();

  foreach ($bundles as $bundle) {
    $label = $bundle->label;
    $query = db_select('public.tripal_cvterm_entity_linker', 't');
    $query->join('public.tripal_entity', 'e', 't.entity_id = e.id');
    $query->join('public.tripal_bundle', 'tb', 'e.term_id = tb.term_id');
    $query->fields('tb', ['label']);
    $query->condition('database', $short_name);
    $query->condition('label', $label);
    $count = $query->countQuery()->execute()->fetchField();

    $list[$label] = [
      'name' => $label,
      'count' => $count,
    ];
  }

  $query = 'select tb.label AS name, count(*) AS count FROM {tripal_cvterm_entity_linker} t 
INNER JOIN {tripal_entity} e ON t.entity_id = e.id 
INNER JOIN {tripal_bundle} tb ON tb.term_id = e.term_id
WHERE database = :db
GROUP BY tb.label';
  $results = db_query($query, [':db' => $short_name])->fetchAll();

  $list = [];

  foreach ($results as $result) {
    $count = $result->count;
    $name = $result->name;
    $list[] = ['count' => $count, 'name' => $name];

  }

  return $list;
}
