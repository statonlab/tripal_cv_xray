(function ($) {

  var anchor_id = null
  var target_bundle_id = null

  Drupal.behaviors.TripalCVLookup = {
    attach: function (context, settings) {
       anchor_id = settings.tripal.cv_lookup.anchor_id
       target_bundle_id = settings.tripal.cv_lookup.target_bundle_id
      $('.tree-node-closed').click(function(event) {
        expandNode($(this));
      });
    }
  }
  
  /**
   * Collapses a node in the CV tree browser and removes its children.
   */
  function collapseNode(item) {
    var parent = $(item).parent('li');
    parent.children('i').removeClass('tree-node-open');
    parent.children('i').addClass('tree-node-closed');
    parent.children('ul').remove();
    
   // Add the click for expanding the node now that this node is expaded.
    parent.children('i').unbind('click');
    parent.children('i').click(function(event){
      expandNode($(this));
    }) 
  }
  
  /**
   * Expands a node in the CV tree browser and loads it's children via AJAX.
   */
  function expandNode(item){
    var parent = $(item).parent('li');
    var vocabulary = parent.attr('vocabulary');
    var accession = parent.attr('accession');
    parent.children('i').removeClass('tree-node-closed');
    parent.children('i').addClass('tree-node-loading');
    
    // Add the click for collapsing the node now that this node is expanded.
    parent.children('i').unbind('click');
    parent.children('i').click(function(event){
      collapseNode($(this));
    }) 
    
    $.ajax({
      //    //$vocabulary, $accession, $target_bundle_id, $anchor_entity_id)
      url : baseurl + '/cv_entities/lookup/' + vocabulary + '/' + accession + '/' + target_bundle_id + '/' + anchor_id + '/children',
      success: function(data) {
        parent.append(data.content);
        parent.children('i').removeClass('tree-node-loading');
        parent.children('i').addClass('tree-node-open');
        // Add the click event to new DOM elements.
        var nodes = parent.find('.tree-node-closed');
        nodes.click(function(event) {
          expandNode($(this));
        });
      }
    });
  }
})(jQuery);
