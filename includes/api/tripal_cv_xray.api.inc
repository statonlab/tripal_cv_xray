<?php
//
///**
// * Updates all entities with all CVs.
// *
// * DEPRECATED
// */
//function tripal_cv_xray_cvterm_entity_job() {
//  $converter = new ChadoEntityChildCvterm();
//  $converter->update_all_entities();
//}

/**
 * Run the indexer.
 */
function tripal_cv_xray_run_indexer() {
  //  CVBrowserIndexer::run();
}

/**
 * looks up all the entities associated with an array of cvterms.
 *
 * @param array $terms : An array of term objects
 * @param int $target_bundle An integer for the bundle id, ie chado_bio_data_x
 * @param array $child_entities An array of entities keyed by the bundle type.
 *   These should be created by...
 *
 * @return array
 */
function tripal_cv_xray_lookup_entities_for_terms($terms, $target_bundle_id, $child_entities) {
  $child_entity_cvterms = [];
  $target_bundle_table = "public.chado_bio_data_" . $target_bundle_id;

  foreach ($terms as $term) {
    $accession = $term["accession"];
    $short_name = $term["vocabulary"]["short_name"];

    $query = db_select("public.tripal_cvterm_entity_linker", "CEL");
    $query->fields("CEL",
      [
        "entity_id",
      ])
      ->condition("database", $short_name)
      ->condition("accession", $accession);
    //->condition("entity_id", array_keys($child_entities[$target_bundle_table]), "IN");

    $entities = $query->execute()->fetchAllAssoc('entity_id');
    $key = $short_name . ":" . $accession;
    $child_entity_cvterms[$key] = $entities;
  }

  return $child_entity_cvterms;
}

/**
 * Get the count of entities per term
 *
 * @param array $terms
 * @param int $target_bundle_id
 * @param int $entity_id Anchor entity id
 *
 * @return array
 */
function tripal_cv_xray_lookup_entities_for_terms_count($terms, $target_bundle_id, $entity_id) {
  // Loading entity here is fine since the static cache holds all of the values
  // anyway so there won't be any performance issues.
  $entity = tripal_load_entity('TripalEntity', [$entity_id]);
  $entity = reset($entity);

  $query = db_select('public.chado_bundle', 'tb')
    ->fields('tb', ['bundle_id', 'data_table'])
    ->condition('bundle_id', $target_bundle_id, '=');
  $target_info = $query->execute()->fetchObject();

  $target_chado_table = $target_info->data_table;
  $target_bundle_table = "chado_bio_data_" . $target_bundle_id;

  $target_chado_column = 'feature_id';
  $record_field = 'organism_id';
  if ($target_chado_table === 'biomaterial') {
    $record_field = 'taxon_id';
    $target_chado_column = 'biomaterial_id';
  }

  $mapped = [];
  $database = '';
  $accessions = [];
  foreach ($terms as $term) {
    $accessions[] = $term["accession"];
    $database = $term["vocabulary"]["short_name"];
  }

  if (empty($accessions)) {
    return $mapped;
  }

  $counts = (int) db_query("SELECT TCEL.database, TCEL.accession, COUNT(TCEL.entity_id) AS term_count
              FROM {tripal_cvterm_entity_linker} TCEL
              INNER JOIN {{$target_bundle_table}} CB ON CB.entity_id = TCEL.entity_id
              INNER JOIN {chado.{$target_chado_table}} CF ON CF.{$target_chado_column} = CB.record_id
              WHERE CF.{$record_field} = :record_id 
                    AND TCEL.database = :database
                    AND TCEL.accession IN (:accessions)
              GROUP BY TCEL.database, TCEL.accession",
    [
      ':record_id' => $entity->chado_record_id,
      ':database' => $database,
      ':accessions' => $accessions,
    ])->fetchAll();

  foreach ($counts as $count) {
    $key = $count['database'] . ":" . $count['accession'];
    $mapped[$key] = $count['term_count'];
  }

  return $mapped;
}

/**
 * Builds the item displayed on the tip of the tree for a given term.
 *
 * @param int $anchor_entity_id
 * @param array $children
 * @param array $map Counts of entities per term
 *
 * @return string
 */
function tripal_cv_xray_field_lookup_term_children_format($anchor_entity_id, $children, $onscreen_entity_maps, $target_bundle_id) {
  $map = $onscreen_entity_maps;

  $items = '<ul id="tripal-cv-lookup-tree">';
  foreach ($children as $child) {
    $child_name = $child['vocabulary']['short_name'] . ":" . $child['accession'];

    $grand = tripal_get_term_children($child['vocabulary']['short_name'],
      $child['accession']);
    $num_grand = count($grand);
    $items .= '<li vocabulary = "' . $child['vocabulary']['short_name'] . '" ' . 'accession = "' . $child['accession'] . '" ' . 'children = "' . $num_grand . '" ' . 'state = "closed" ' . 'class = "cv-lookup-tree-node">';

    $class = 'tree-node-closed';
    if ($num_grand == 0) {
      $class = 'tree-node-single';
    }

    $items .= '<i class = "tree-node-icon ' . $class . '"></i>';
    $items .= l($child['name'],
      'cv/lookup/' . $child['vocabulary']['short_name'] . '/' . $child['accession'],
      ['attributes' => ['target' => '_blank']]);

    if ($child['accession'] != $child['name']) {
      $items .= ' [' . $child_name . '] ';
    }

    $child_mapping = NULL;
    if (isset($map[$child_name])) {
      $child_mapping = $map[$child_name];
    }
    $num_mapped = is_array($child_mapping) ? count($child_mapping) : $child_mapping;

    if ($num_mapped == 0) {
      $items .= "";
    }
    else {
      $record_word = "records.";
      if ($num_mapped == 1) {
        $record_word = "record.";
      }
      // page arguments are db shortname, accession, anchor entity ID, mapping bundle ID
      $query_arguments = [
        'cv_entities',
        $child['vocabulary']['short_name'],
        $child['accession'],
        $anchor_entity_id,
        $target_bundle_id,
      ];
      $items .= l("  {$num_mapped} " . $record_word,
        implode('/', $query_arguments));
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

  $query = db_select('public.chado_bundle', 'tb')
    ->fields('tb', ['bundle_id', 'data_table'])
    ->condition('bundle_id', $target_bundle_id, '=');
  $target_info = $query->execute()->fetchObject();

  $bundle_id = $target_info->bundle_id;
  $target_chado_table = $target_info->data_table;
  $target_bundle_table = "public.chado_bio_data_" . $bundle_id;

  // End query, set to null
  $results = NULL;
  $target_record_column = $target_chado_table . "_id";

  if ($entity->chado_table == 'organism') {
    $query = db_select($target_bundle_table, 'tbt');
    $query->fields('tbt', ['entity_id']);
    $query->join('chado.' . $target_chado_table,
      "tct",
      'tbt.record_id = tct.' . $target_record_column);
    $query->join('public.tripal_entity', 'te', 'te.id = tbt.entity_id');
    $query->fields('te', ['title']);
    if ($target_chado_table == 'biomaterial') {
      $query->condition('tct.taxon_id', $record_id, '=');
    }
    else {
      //works with feature in mind, and will work when chado table has organism_id column
      $query->condition('tct.organism_id', $record_id, '=');
    }
    $results = $query->execute()
      ->fetchAllAssoc('entity_id');//this ensures the key is the entity ID!
  }

  if ($entity->chado_table == 'analysis') {
    return NULL;
    //we dont know what we're doing!
    $query = db_select($target_bundle_table, 'tbt');
    $query->fields('tbt', ['entity_id']);
    $query->join('chado.' . $target_chado_table,
      "tct",
      'tbt.record_id = tct.' . $target_record_column);
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
function tripal_cv_xray_entity_child_lookup_ajax($vocabulary, $accession, $target_bundle_id, $anchor_entity_id) {
  //$term = tripal_get_term_details($vocabulary, $accession);
  $children = tripal_get_term_children($vocabulary, $accession);

  // $mapped_entities = tripal_cv_xray_fetch_associated_entities($entity, $target_bundle_id);

  $counts = tripal_cv_xray_lookup_entities_for_terms_count($children,
    $target_bundle_id,
    $anchor_entity_id);

  // Format results
  $response = [
    'vocabulary' => $vocabulary,
    'accession' => $accession,
    'content' => tripal_cv_xray_field_lookup_term_children_format($anchor_entity_id,
      $children,
      $counts,
      $target_bundle_id),
  ];

  drupal_json_output($response);
}
