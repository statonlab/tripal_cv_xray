<?php

/**
 * Updates all entities with all CVs.
 */
function tripal_cv_xray_cvterm_entity_job() {
  $converter = new ChadoEntityChildCvterm();
  $converter->update_all_entities();
}

/**
 * Run the indexer.
 */
function tripal_cv_xray_run_indexer() {
  CVBrowserIndexer::run();
}


/**
 * looks up all the entities associated with an array of cvterms.
 * 
 * @param $terms: An array of term objects
 * @param $target_bundle An integer for the bundle id, ie chado_bio_data_x
 * @param $child_entities  An array of entities keyed by the bundle type.  These should be created by...
 *
 * @return array
 */
function tripal_cv_xray_lookup_entities_for_terms($terms, $target_bundle_id, $child_entities) {

  $child_entity_cvterms = [];

  $target_bundle_table = "public.chado_bio_data_" . $target_bundle_id;

  foreach ($terms as $term) {
    $accession = $term["accession"];
    $short_name = $term["vocabulary"]["short_name"];

    $query= db_select("public.tripal_cvterm_entity_linker", "CEL");
    $query->fields("CEL", [
      "entity_id",
    ])
      ->condition("database", $short_name)
      ->condition("accession", $accession)
      ->condition("entity_id", array_keys($child_entities[$target_bundle_table]), "IN");

    $entities = $query->execute()->fetchAllAssoc('entity_id');
    $key = $short_name . ":" . $accession;
    $child_entity_cvterms[$key] = $entities;
  }
  return ($child_entity_cvterms);
}


/**
 * Builds the item displayed on the tip of the tree for a given term.
 * @param $anchor_entity_id
 * @param $children
 * @param $onscreen_entity_maps
 *
 * @return string
 */
function tripal_cv_xray_field_lookup_term_children_format($anchor_entity_id, $children, $onscreen_entity_maps, $target_bundle_id) {
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
      $items .= "";
    }
    else {
      $record_word = "records.";
      if ($num_mapped == 1) {
        $record_word = "record.";
      }
     // $items .= l("  {$num_mapped} " . $record_word, 'cv_entities/' . $anchor_entity_id . '/' . $flattened_entities);
       $items .= l("  {$num_mapped} " . $record_word, 'cv_entities/' .
         $child['vocabulary']['short_name']  . '/' .
         $child['accession'] . '/' .
         $anchor_entity_id . '/'
         . $target_bundle_id
       // 'page arguments' => array(1, 2, 3, 4), //db shortname, accession, anchor entity ID, mapping bundle ID

       );


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
 * Given an entity (the anchor entity), retrieve all entities that map to it of type target bundle.
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
    ->fields('tb', ['bundle_id',  'data_table'])
    ->condition('bundle_id', $target_bundle_id, '=');
  $target_info = $query->execute()->fetchObject();

  $bundle_id = $target_info->bundle_id;
  $target_chado_table = $target_info->data_table;
  $target_bundle_table = "public.chado_bio_data_" . $bundle_id;

  $results = NULL;//end query, set to null


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
function tripal_cv_xray_entity_child_lookup_ajax($vocabulary, $accession, $target_bundle_id, $anchor_entity_id) {
  $term = tripal_get_term_details($vocabulary, $accession);
  $children = tripal_get_term_children($vocabulary, $accession);
  //Get entity mapping
  $entity = tripal_load_entity('TripalEntity', [$anchor_entity_id]);
  //get entity from anchor id

  $entity = array_shift($entity);

  $mapped_entities = tripal_cv_xray_fetch_associated_entities($entity, $target_bundle_id);


  $onscreen_entity_maps = tripal_cv_xray_lookup_entities_for_terms($children, $target_bundle_id, $mapped_entities);
  //format results

  $response = array(
    'vocabulary' => $vocabulary,
    'accession' => $accession,
    'content' => tripal_cv_xray_field_lookup_term_children_format($anchor_entity_id, $children, $onscreen_entity_maps, $target_bundle_id),
  );

  drupal_json_output($response);
}
