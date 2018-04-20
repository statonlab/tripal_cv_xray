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

  $pop_options = ["all" => "All", "custom" => "Custom"];
  $form['populate_type'] = [
    '#type' => 'select',
    '#options' => $pop_options,
    '#default' => "all",
    '#ajax' => [
      'callback' => 'tripal_cv_xray_pop_callback',
      'wrapper' => 'div1_wrapper',
      'effect' => 'fade',
    ],
  ];

  $form['div1_wrapper'] = [
    '#type' => 'fieldset',
    '#prefix' => '<div id="div1_wrapper">',
    '#suffix' => '</div>',
  ];


  $pop_type = isset($form_state['values']['populate_type']) ? $form_state['values']['populate_type'] : 'all';

  if ($pop_type == "all") {

    $form['div1_wrapper']['message'] = [
      '#markup' => 'This will submit a job that will clear and repopulate the entity:cvterm index.  It will run for all default entity types and CVs.',
    ];
  }
  if ($pop_type == "custom") {

    $form['div1_wrapper']['message'] = [
      '#markup' => 'This will allow you to populate a custom entity or CV.  Unfortunately this will get rewritten when all is run again, unless we approach this as a configuration settings...',
    ];
  }
  $form['submit'] = [
    '#type' => 'submit',
    '#value' => 'Populate All',
  ];

  return $form;
}

/**
 * Hook submit for form.
 *
 * @param $form
 * @param $form_state
 */
function tripal_cv_xray_cvterm_entity_form_submit($form, &$form_state) {

  global $user;

  $args = [];
  if ($form_state['values']['populate_type'] == "all") {
    //tripal_add_job("populate_cvterm_entity_index", "tripal_cv_xray", "tripal_cv_xray_cvterm_entity_job", $args, $user->uid);
    $job = new XRayDispatcherJob(500);
    XRayQueue::dispatch($job);
    drupal_set_message('Dispatcher job launched successfully.');
    drupal_set_message('You may run the jobs using `drush cron`');
  }
}

/**
 * AJAX callback for the run type.
 *
 * @param $form
 * @param $form_state
 *
 * @return mixed
 */
function tripal_cv_xray_pop_callback($form, &$form_state) {
  return $form['div1_wrapper'];
}

