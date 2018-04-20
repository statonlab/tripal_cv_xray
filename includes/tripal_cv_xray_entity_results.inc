<?php

/**
 * @param $anchor_id
 * @param $entities
 *
 * @return string
 */
function tripal_cv_xray_entity_view($db_shortname, $accession, $anchor_id, $mapping_bundle_id) {
  // 'page arguments' => array(1, 2, 3, 4), //db shortname, accession, anchor entity ID, mapping bundle ID

  $per_page = 20;

  $anchor = TripalEntity_load($anchor_id);


  $query = db_select("chado.cvterm", 'CVT');
    $query->innerjoin('chado.dbxref', 'DBX', 'DBX.dbxref_id = CVT.dbxref_id');
  $query->innerjoin('chado.db', 'DB', 'DBX.db_id = DB.db_id');
  $query->fields('CVT', ["cvterm_id"])
  ->condition('DBX.accession', $accession)
  ->condition('DB.name', $db_shortname);

  $term_id = $query->execute()->fetchField();

  if (!$term_id){
    tripal_set_message("Could not look up term {$db_shortname}:{$accession}", TRIPAL_ERROR);
    return;
  }


  $term = tripal_get_term_details($db_shortname, $accession);

  //load mapped entities

  $entities_all =  tripal_cv_xray_fetch_associated_entities($anchor, $mapping_bundle_id);


  //get entities for this cvterm
  $entities = tripal_cv_xray_lookup_entities_for_terms([$term], $mapping_bundle_id, $entities_all);
  $key = $db_shortname . ':' . $accession;
  $entities = $entities[$key];


    $content = 'the below entities are related to <a href="/bio_data/' . $anchor_id . '">' . $anchor->title . '</a> and annotated with the selected term in some way.';

  //load paginated mapped entities

  $current_page = pager_default_initialize(count($entities), $per_page);
  $chunks = array_chunk($entities, $per_page, TRUE);
  $pager = theme('pager', array('quantity', count($entities)));


  $ents_to_view = array_keys($chunks[$current_page]);


  $loaded_entities = tripal_load_entity('TripalEntity', $ents_to_view, FALSE);

  $rows = [];
  foreach ($loaded_entities as $entity) {

    $item = l($entity->title, "/bio_data/" . $entity->id);
    $rows[] = [$item];
  }
  $table = theme('table', ['header' => ['Entity'], 'rows' => $rows]);
  $content .= $table;
  $content .= $pager;

  return $content;
}
