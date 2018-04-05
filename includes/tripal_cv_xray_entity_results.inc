<?php
function tripal_cv_xray_entity_view($anchor_id, $entities) {
  $per_page = 20;
  $entities = explode(",", $entities);

  $anchor = TripalEntity_load($anchor_id);

  $content = 'the below entities are related to <a href="/bio_data/' . $anchor_id . '">' . $anchor->title . '</a> and annotated with the selected term in some way.';

  //load paginated mapped entities

  $current_page = pager_default_initialize(count($entities), $per_page);
  $chunks = array_chunk($entities, $per_page, TRUE);
  $pager = theme('pager', array('quantity', count($entities)));


  $ents_to_view = $chunks[$current_page];

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