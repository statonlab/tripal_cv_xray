<?php


function tripal_cv_xray_cvterm_entity_form($form, &$form_state){
  $welcome = '<h3>Tripal CV X-ray admin</h3>
<p>Welcome to the Tripal CV X-ray module.  When properly configured, this module allows you to create browsable CV tree fields.  When attached to a bundle, it can map entities associated with that bundle entity onto the tree.  For example, an organism can have a GO cv tree, with genes associated with the terms mapped onto it.  Users can explore terms and easily find and view the entities associated with various terms.
</p>
<p>To begin, you must populate the Entity: CVterm mapping index.  This associates the entities that will be mapped with CVterms.
</p>
  ';
$form['welcome'] = [
  '#markup' => $welcome
];

  return $form;
}
