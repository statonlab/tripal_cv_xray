<?php

/**
 * @file
 */

/**
 * Page for viewing entities mapped to a specific term from a particular anchor
 * entity. IE, all the genes with term X:Y for organism Z.
 *
 * @param $db_shortname
 * @param $accession
 * @param $anchor_id
 * @param $mapping_bundle_id
 *
 * @return string|null
 *
 * @throws \Exception
 */
function tripal_cv_xray_entity_view($db_shortname, $accession, $anchor_id, $mapping_bundle_id, $analysis_id = NULL) {
  $per_page = 20;
  $anchor = tripal_load_entity('TripalEntity', [$anchor_id]);
  $anchor = reset($anchor);
  $mapping_entity = tripal_load_bundle_entity(['id' => $mapping_bundle_id]);
  $map_label = $mapping_entity->label;

  $term = tripal_get_term_details($db_shortname, $accession);

  if ($term === FALSE) {
    tripal_set_message("Could not look up term {$db_shortname}:{$accession}", TRIPAL_ERROR);

    return NULL;
  }

  $accession = $term["accession"];
  $short_name = $term["vocabulary"]["short_name"];
  $target_chado_column = 'feature_id';
  $record_field = 'organism_id';
  $analysis_join_query = '';
  $analysis_condition = '';
  // Load paginated mapped entities.
  if ($mapping_entity->data_table === 'biomaterial') {
    $record_field = 'taxon_id';
  }
  elseif ($analysis_id !== NULL) {
    $analysis_join_query = "INNER JOIN {chado.analysisfeature} AF ON AF.feature_id = CF.{$target_chado_column}";
    $analysis_condition = "AND AF.analysis_id = :analysis_id";
  }

  $bundle_table = 'chado_bio_data_' . $mapping_bundle_id;
  $args = [
    ':record_id' => $anchor->chado_record_id,
    ':database' => $short_name,
    ':accession' => $accession,
  ];
  if (!empty($analysis_condition)) {
    $args[':analysis_id'] = $analysis_id;
  }
  $count = (int) db_query("SELECT COUNT(TCEL.entity_id)
              FROM {tripal_cvterm_entity_linker} TCEL
              INNER JOIN {{$bundle_table}} CB ON CB.entity_id = TCEL.entity_id
              INNER JOIN {chado.{$mapping_entity->data_table}} CF ON CF.{$target_chado_column} = CB.record_id
              {$analysis_join_query}
              WHERE CF.{$record_field} = :record_id 
                    {$analysis_condition} 
                    AND TCEL.database = :database
                    AND TCEL.accession = :accession", $args)->fetchField();

  $current_page = pager_default_initialize($count, $per_page);

  $args += [
    ':offset' => $current_page * $per_page,
    ':limit' => $per_page,
  ];

  $entities = db_query("SELECT TE.id, TE.title
              FROM {tripal_cvterm_entity_linker} TCEL
              INNER JOIN {tripal_entity} TE ON TE.id = TCEL.entity_id
              INNER JOIN {{$bundle_table}} CB ON CB.entity_id = TCEL.entity_id
              INNER JOIN {chado.{$mapping_entity->data_table}} CF ON CF.{$target_chado_column} = CB.record_id
              {$analysis_join_query}
              WHERE CF.{$record_field} = :record_id 
                    {$analysis_condition} 
                    AND TCEL.database = :database
                    AND TCEL.accession = :accession
              ORDER BY TCEL.entity_id ASC
              OFFSET :offset LIMIT :limit", $args)->fetchAll();

  $rows = [];
  foreach ($entities as $entity) {
    $name = l($entity->title, "/bio_data/" . $entity->id);
    $rows[] = [$name];
  }

  $term_link = l($term['name'] . ' (' . $short_name . ':' . $accession . ')', 'cv/lookup/' . $short_name . '/' . $accession);
  $anchor_link = l($anchor->title, 'bio_data/' . $anchor_id);
  $collection_link = l('Create Downloadable Collection', 'tripal_cv_xray/collections/' . $db_shortname . '/' . $accession . '/' . $anchor_id . '/' . $mapping_bundle_id, [
    'attributes' => [
      'id' => 'create-collection-btn',
      'class' => 'btn btn-primary',
    ],
  ]);

  $content = '';

  // Make sure Tripal collections are supported.
  if (function_exists('tripal_create_collection')) {
    $content = '<p>' . $collection_link . '</p>';
  }

  if ($analysis_id) {
    $entity_id = chado_get_record_entity_by_table('analysis', $analysis_id);
    $title = db_query('SELECT title FROM {tripal_entity} WHERE id = :id', [':id' => $entity_id]);
    $content .= '<p>Records related to '.$title.'</p>';
  }

  $content .= '<p>The below ' . $map_label . ' are associated with 
              ' . $anchor_link . ' and the vocabulary term ' . $term_link . '. There are ' . $count . ' ' . $map_label . ' associated. Showing ' . $per_page . ' per page.</p>';

  $table = theme('table', ['header' => [$map_label], 'rows' => $rows]);
  $pager = theme('pager', ['quantity', count($entities)]);

  $content .= $table;
  $content .= $pager;

  return $content;
}
