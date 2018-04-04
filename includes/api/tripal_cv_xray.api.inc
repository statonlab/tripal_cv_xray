<?php


function tripal_cv_xray_cvterm_entity_job() {
  $converter = new ChadoEntityChildCvterm();
  $converter->update_all_entities();
}

