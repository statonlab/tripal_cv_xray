<?php

/**
 * Updates all entities with all CVs.
 */
function tripal_cv_xray_cvterm_entity_job() {
  $converter = new ChadoEntityChildCvterm();
  $converter->update_all_entities();
}


/**
 * Updates provided entities with all CVs.
 * @param $entities
 */
function tripal_cv_xray_cvterm_entity_specific_update($entities) {
  $converter = new ChadoEntityChildCvterm();
  $converter->update_specific_entities($entities);
}
