<?php

/**
 * @file
 */

/**
 * Looks up all the entities associated with an array of cvterms.
 *
 * @param array $terms
 *   : An array of term objects.
 * @param int $target_bundle_id
 *   An integer for the bundle id, ie chado_bio_data_x.
 * @param array $child_entities
 *   An array of entities keyed by the bundle type.
 *   These should be created by...
 *
 * @return array
 */
function tripal_cv_xray_lookup_entities_for_terms(array $terms, int $target_bundle_id, array $child_entities) {
  $child_entity_cvterms = [];
  $target_bundle_table = "public.chado_bio_data_" . $target_bundle_id;

  foreach ($terms as $term) {
    $accession = $term["accession"];
    $short_name = $term["vocabulary"]["short_name"];

    $query = db_select("public.tripal_cvterm_entity_linker", "CEL");
    $query->fields("CEL", [
      "entity_id",
    ])->condition("database", $short_name)->condition("accession", $accession);
    // ->condition("entity_id", array_keys($child_entities[$target_bundle_table]), "IN");.
    $entities = $query->execute()->fetchAllAssoc('entity_id');
    $key = $short_name . ":" . $accession;
    $child_entity_cvterms[$key] = $entities;
  }

  return $child_entity_cvterms;
}

/**
 * Get the count of entities per term.
 *
 * @param array $terms
 * @param int $target_bundle_id
 * @param int $entity_id
 *   Anchor entity id.
 * @param int $analysis_id If provided, results will be limited to analysis ids
 *
 * @return array
 */
function tripal_cv_xray_lookup_entities_for_terms_count(array $terms, int $target_bundle_id, int $entity_id, int $analysis_id = NULL) {
  // Loading entity here is fine since the static cache holds all of the values
  // anyway so there won't be any performance issues.
  $entity = tripal_load_entity('TripalEntity', [$entity_id]);
  $entity = reset($entity);

  $query = db_select('public.chado_bundle', 'tb')->fields('tb', [
    'bundle_id',
    'data_table',
  ])->condition('bundle_id', $target_bundle_id, '=');
  $target_info = $query->execute()->fetchObject();

  $target_chado_table = $target_info->data_table;
  $target_bundle_table = "public.chado_bio_data_" . $target_bundle_id;

  $target_chado_column = 'feature_id';
  $record_field = 'organism_id';
  $analysis_join_query = '';
  $analysis_condition = '';
  if ($target_chado_table === 'biomaterial') {
    $record_field = 'taxon_id';
    $target_chado_column = 'biomaterial_id';
  }
  elseif ($analysis_id !== NULL) {
    $analysis_join_query = "INNER JOIN {chado.analysisfeature} AF ON AF.feature_id = CF.{$target_chado_column}";
    $analysis_condition = "AND AF.analysis_id = :analysis_id";
  }

  $mapped = [];
  foreach ($terms as $term) {
    $accession = $term["accession"];
    $database = $term["vocabulary"]["short_name"];

    $args = [
      ':record_id' => $entity->chado_record_id,
      ':database' => $database,
      ':accession' => $accession,
    ];

    if (!empty($analysis_condition)) {
      $args[':analysis_id'] = $analysis_id;
    }

    $count = (int) db_query("SELECT COUNT(TCEL.entity_id)
              FROM {tripal_cvterm_entity_linker} TCEL
              INNER JOIN {{$target_bundle_table}} CB ON CB.entity_id = TCEL.entity_id
              INNER JOIN {chado.{$target_chado_table}} CF ON CF.{$target_chado_column} = CB.record_id
              {$analysis_join_query}
              WHERE CF.{$record_field} = :record_id 
                    {$analysis_condition}
                    AND TCEL.database = :database
                    AND TCEL.accession = :accession", $args)->fetchField();
    $key = $database . ":" . $accession;
    $mapped[$key] = $count;
  }

  return $mapped;
}

/**
 * Builds the item displayed on the tip of the tree for a given term.
 *
 * @param int $anchor_entity_id
 * @param array $children
 * @param array $map
 *   Counts of entities per term.
 *
 * @return string
 */
function tripal_cv_xray_field_lookup_term_children_format(int $anchor_entity_id, array $children, $onscreen_entity_maps, $target_bundle_id, $analysis_id = NULL) {
  $map = $onscreen_entity_maps;

  $items = '<ul id="tripal-cv-lookup-tree">';
  foreach ($children as $child) {
    $child_name = $child['vocabulary']['short_name'] . ":" . $child['accession'];

    $grand = tripal_get_term_children($child['vocabulary']['short_name'], $child['accession']);
    $num_grand = count($grand);
    $items .= '<li vocabulary="' . $child['vocabulary']['short_name'] . '" accession="' . $child['accession'] . '" children="' . $num_grand . '" state = "closed" ' . 'class = "cv-lookup-tree-node">';

    $class = 'tree-node-closed';
    if ($num_grand == 0) {
      $class = 'tree-node-single';
    }

    $items .= '<i class="tree-node-icon ' . $class . '"></i>';
    $items .= l($child['name'], 'cv/lookup/' . $child['vocabulary']['short_name'] . '/' . $child['accession'], ['attributes' => ['target' => '_blank']]);

    //    if ($child['accession'] != $child['name']) {
    $items .= ' [' . $child_name . '] ';
    //    }

    $child_mapping = NULL;
    if (isset($map[$child_name])) {
      $child_mapping = $map[$child_name];
    }
    $num_mapped = is_array($child_mapping) ? count($child_mapping) : $child_mapping;

    if ($num_mapped > 0) {
      $record_word = "records.";
      if ($num_mapped == 1) {
        $record_word = "record.";
      }
      // Page arguments are db shortname, accession, anchor entity ID, mapping bundle ID.
      $query_arguments = [
        'cv_entities',
        $child['vocabulary']['short_name'],
        $child['accession'],
        $anchor_entity_id,
        $target_bundle_id,
      ];
      if ($analysis_id) {
        $query_arguments[] = $analysis_id;
      }
      $items .= l(" {$num_mapped} {$record_word}", implode('/', $query_arguments));
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
 * Given an entity (the anchor entity), retrieve all entities that map to it of
 * type target bundle.
 *
 * @param $anchor_entity
 * @param $target_bundle_id
 *
 * @return array|null
 */
function tripal_cv_xray_fetch_associated_entities($anchor_entity, $target_bundle_id) {
  $entity = $anchor_entity;

  $child_entities = [];
  $record_id = $entity->chado_record_id;

  $query = db_select('public.chado_bundle', 'tb')->fields('tb', [
    'bundle_id',
    'data_table',
  ])->condition('bundle_id', $target_bundle_id, '=');
  $target_info = $query->execute()->fetchObject();

  $bundle_id = $target_info->bundle_id;
  $target_chado_table = $target_info->data_table;
  $target_bundle_table = "public.chado_bio_data_" . $bundle_id;

  // End query, set to null.
  $results = NULL;
  $target_record_column = $target_chado_table . "_id";

  if ($entity->chado_table == 'organism') {
    $query = db_select($target_bundle_table, 'tbt');
    $query->fields('tbt', ['entity_id']);
    $query->join('chado.' . $target_chado_table, "tct", 'tbt.record_id = tct.' . $target_record_column);
    $query->join('public.tripal_entity', 'te', 'te.id = tbt.entity_id');
    $query->fields('te', ['title']);
    if ($target_chado_table == 'biomaterial') {
      $query->condition('tct.taxon_id', $record_id, '=');
    }
    else {
      // Works with feature in mind, and will work when chado table has organism_id column.
      $query->condition('tct.organism_id', $record_id, '=');
    }
    $results = $query->execute()// This ensures the key is the entity ID!
    ->fetchAllAssoc('entity_id');
  }

  if ($entity->chado_table == 'analysis') {
    return NULL;
    // We dont have a way to map this yet.
    $query = db_select($target_bundle_table, 'tbt');
    $query->fields('tbt', ['entity_id']);
    $query->join('chado.' . $target_chado_table, "tct", 'tbt.record_id = tct.' . $target_record_column);
    $query->join('public.tripal_entity', 'te', 'te.id = tbt.entity_id');
    $query->fields('te', ['title']);
  }

  if ($results) {
    $child_entities[$target_bundle_table] = $results;
  }

  return $child_entities;
}

/**
 * The AJAX callback used by the js tree to look up the children of a term.
 *
 * @param $vocabulary
 * @param $accession
 * @param $target_bundle_id
 * @param $anchor_entity_id
 */
function tripal_cv_xray_entity_child_lookup_ajax($vocabulary, $accession, $target_bundle_id, $anchor_entity_id, $analysis_id = null) {
  $children = tripal_get_term_children($vocabulary, $accession);

  $counts = tripal_cv_xray_lookup_entities_for_terms_count($children, $target_bundle_id, $anchor_entity_id, $analysis_id);

  // Format results.
  $response = [
    'vocabulary' => $vocabulary,
    'accession' => $accession,
    'content' => tripal_cv_xray_field_lookup_term_children_format($anchor_entity_id, $children, $counts, $target_bundle_id, $analysis_id),
  ];

  drupal_json_output($response);
}
