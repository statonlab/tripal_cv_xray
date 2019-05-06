(function ($) {

  var load_request = {}

  Drupal.behaviors.TripalCVLookup = {
    attach: attach
  }

  function attach(context, settings) {
    settings.cv_lookup.map(function (cv_lookup) {
      loadTree(cv_lookup)
    })
  }

  function loadTree(cv_lookup) {
    var anchor_id        = cv_lookup.anchor_id
    var target_bundle_id = cv_lookup.target_bundle_id
    var vocabulary       = cv_lookup.vocabulary
    var wrapper_id       = cv_lookup.wrapper_id
    var $analysis_field  = $(wrapper_id).find('[name="cv_xray_analysis_id"]')
    var analysis_id      = $analysis_field.lenght > 0 ? $analysis_field.val() : null

    $('#'+wrapper_id).html('Loading browser. Please wait. <i class="fa fa-spin fa-refresh"></i>')

    if(analysis_id) {
      $analysis_field.unbind('change')
      $analysis_field.on('change', function () {
        loadTree(cv_lookup)
      })
    }

    if (typeof load_request[wrapper_id] === 'undefined') {
      load_request[wrapper_id] = null
    }

    // Cancel any pending requests before establishing a new one
    if (load_request[wrapper_id] !== null) {
      load_request[wrapper_id].abort()
    }

    load_request[wrapper_id] = $.get(baseurl + '/tripal_cv_xray/root-tree/' + cv_lookup.field_name, {
      vocabulary         : vocabulary,
      anchor_id          : anchor_id,
      target_bundle_id   : target_bundle_id,
      cv_xray_analysis_id: analysis_id
      // path            : window.location.pathname
    }, function (data) {
      if (!data.error) {
        $('#' + wrapper_id).html(data.content)
        $('.tree-node-closed', '#' + wrapper_id).click(function (event) {
          new Tree($(this), cv_lookup, 'expand')
        })
      }

      load_request = null
    })
  }

  function Tree(element, options, op) {
    this.anchor_id        = options.anchor_id
    this.target_bundle_id = options.target_bundle_id
    this[op](element)
  }

  /**
   * Collapses a node in the CV tree browser and removes its children.
   */

  Tree.prototype.collapse = function (item) {
    var parent           = $(item).parent('li')
    var anchor_id        = this.anchor_id
    var target_bundle_id = this.target_bundle_id
    parent.children('i').removeClass('tree-node-open')
    parent.children('i').addClass('tree-node-closed')
    parent.children('ul').remove()

    // Add the click for expanding the node now that this node is expaded.
    parent.children('i').unbind('click')
    parent.children('i').click(function (event) {
      new Tree($(this), {
        target_bundle_id,
        anchor_id
      }, 'expand')
    })
  }

  /**
   * Expands a node in the CV tree browser and loads it's children via AJAX.
   */
  Tree.prototype.expand = function (item) {
    var parent           = $(item).parent('li')
    var vocabulary       = parent.attr('vocabulary')
    var accession        = parent.attr('accession')
    var anchor_id        = this.anchor_id
    var target_bundle_id = this.target_bundle_id

    parent.children('i').removeClass('tree-node-closed')
    parent.children('i').addClass('tree-node-loading')

    // Add the click for collapsing the node now that this node is expanded.
    parent.children('i').unbind('click')
    parent.children('i').click(function (event) {
      new Tree($(this), {
        target_bundle_id,
        anchor_id
      }, 'collapse')
    })

    $.ajax({
      //    //$vocabulary, $accession, $target_bundle_id, $anchor_entity_id)
      url    : baseurl + '/cv_entities/lookup/' + vocabulary + '/' + accession + '/' + this.target_bundle_id + '/' + this.anchor_id + '/children',
      success: function (data) {
        parent.append(data.content)
        parent.children('i').removeClass('tree-node-loading')
        parent.children('i').addClass('tree-node-open')
        // Add the click event to new DOM elements.
        var nodes = parent.find('.tree-node-closed')

        nodes.click(function (event) {
          new Tree($(this), {
            target_bundle_id,
            anchor_id
          }, 'expand')
        })
      }
    })
  }
})(jQuery)
