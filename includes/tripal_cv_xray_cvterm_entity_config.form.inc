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

  $bundle_ids = array_filter($bundle_ids);
  $cvs = array_filter($cvs);
  $job = new XRayDispatcherJob($bundle_ids, $cvs, 500);
  XRayQueue::dispatch($job);
  drupal_set_message('Dispatcher job launched successfully.');
  drupal_set_message('You may run the jobs using `drush cron`');
}
