<?php

/**
 * Page for viewing entities mapepd to a specific term from a particular anchor
 * entity. IE, all the genes with term X:Y for organism Z.
 *
 * @param $db_shortname
 * @param $accession
 * @param $anchor_id
 * @param $mapping_bundle_id
 *
 * @return string|NULL
 * @throws \Exception
 */
function tripal_cv_xray_entity_view($db_shortname, $accession, $anchor_id, $mapping_bundle_id) {
  $per_page = 20;
  $anchor = tripal_load_entity('TripalEntity', [$anchor_id]);
  $anchor = reset($anchor);
  $mapping_entity = tripal_load_bundle_entity(['id' => $mapping_bundle_id]);
  $map_label = $mapping_entity->label;

  $query = db_select("chado.cvterm", 'CVT');
  $query->innerjoin('chado.dbxref', 'DBX', 'DBX.dbxref_id = CVT.dbxref_id');
  $query->innerjoin('chado.db', 'DB', 'DBX.db_id = DB.db_id');
  $query->fields('CVT', ["cvterm_id"])
    ->condition('DBX.accession', $accession)
    ->condition('DB.name', $db_shortname);

  $term_id = $query->execute()->fetchField();

  if (!$term_id) {
    tripal_set_message("Could not look up term {$db_shortname}:{$accession}", TRIPAL_ERROR);

    return NULL;
  }

  $term = tripal_get_term_details($db_shortname, $accession);
  $accession = $term["accession"];
  $short_name = $term["vocabulary"]["short_name"];

  // Load paginated mapped entities
  if ($mapping_entity->data_table === 'biomaterial') {
    $record_field = 'taxon_id';
  }
  else {
    $record_field = 'organism_id';
  }

  $bundle_table = 'chado_bio_data_' . $mapping_bundle_id;
  $count = (int) db_query("SELECT COUNT(TCEL.entity_id)
              FROM {tripal_cvterm_entity_linker} TCEL
              INNER JOIN {{$bundle_table}} CB ON CB.entity_id = TCEL.entity_id
              INNER JOIN {chado.{$mapping_entity->data_table}} CF ON CF.feature_id = CB.record_id
              WHERE CF.{$record_field} = :record_id 
                    AND TCEL.database = :database
                    AND TCEL.accession = :accession", [
    ':record_id' => $anchor->chado_record_id,
    ':database' => $short_name,
    ':accession' => $accession,
  ])->fetchField();

  $current_page = pager_default_initialize($count, $per_page);

  $entities = db_query("SELECT TCEL.entity_id
              FROM {tripal_cvterm_entity_linker} TCEL
              INNER JOIN {{$bundle_table}} CB ON CB.entity_id = TCEL.entity_id
              INNER JOIN {chado.{$mapping_entity->data_table}} CF ON CF.feature_id = CB.record_id
              WHERE CF.{$record_field} = :record_id 
                    AND TCEL.database = :database
                    AND TCEL.accession = :accession
              OFFSET :offset LIMIT :limit
              ORDER BY TCEL.entity_id", [
    ':record_id' => $anchor->chado_record_id,
    ':database' => $short_name,
    ':accession' => $accession,
    ':offset' => $current_page * $per_page,
    ':limit' => $per_page,
  ])->fetchAll();

  $entities_to_load = [];
  foreach ($entities as $one) {
    $entities_to_load[] = $one->entity_id;
  }

  $pager = theme('pager', ['quantity', $count / $per_page]);
  $loaded_entities = tripal_load_entity('TripalEntity', $entities_to_load, FALSE);

  $rows = [];
  foreach ($loaded_entities as $entity) {
    $name = l($entity->title, "/bio_data/" . $entity->id);
    $rows[] = [$name];
  }

  $term_link = l($term['name'] . ' (' . $short_name . ':' . $accession . ')', 'cv/lookup/' . $short_name . '/' . $accession);
  $anchor_link = l($anchor->title, 'bio_data/' . $anchor_id);

  $content = '<p>The below ' . $map_label . ' are associated with 
              ' . $anchor_link . ' and the vocabulary term ' . $term_link . '. There are ' . $count . ' ' . $map_label . ' associated.</p>';

  $table = theme('table', ['header' => [$map_label], 'rows' => $rows]);
  $content .= $table;
  $content .= $pager;

  return $content;
}
