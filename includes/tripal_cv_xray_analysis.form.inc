<?php

function tripal_cv_xray_analysis_form(array $form, array &$form_state, $organism_id = NULL): array {
  if (!db_table_exists('chado.organism_analysis') || !$organism_id) {
    return [];
  }

  // Get analysis related to this organism
  $analyses = db_query("SELECT A.analysis_id, A.name 
              FROM {chado.organism_analysis} OA 
              INNER JOIN {chado.analysis} A ON A.analysis_id = OA.analysis_id 
              INNER JOIN {chado.analysisprop} p ON p.analysis_id = a.analysis_id
              WHERE p.value IN ('tripal_analysis_unigene', 'genome_assembly')
                AND organism_id = :organism_id
  ", [
    ':organism_id' => $organism_id,
  ])->fetchAll();

  $options = [];
  foreach ($analyses as $analysis) {
    $options[$analysis->analysis_id] = $analysis->name;
  }
  foreach ($analyses as $analysis) {
    $options[$analysis->analysis_id * 800] = $analysis->name . ' -- '.$analysis->analysis_id;
  }

  // Do not change the name! Changing this affects the JS.
  $form['cv_xray_analysis_id'] = [
    '#title' => 'Select an Analysis',
    '#type' => 'select',
    '#options' => $options,
  ];

  return $form;
}
