/**
 * Adds a checkmark after each cvterm on the admin settings form
 * indcating whether the term exists or not.
 *
 * NOTE: Uses the data-cv property for the form field, as well as,
 * the autocomplete ajax to check for existence of the term.
 */
(function ($) {
  Drupal.behaviors.adminCheckCVTerms = {
    attach: function (context, settings) {

    var elementNames = ['sample_type', 'stock_rel', 'feature_rel', 'featureprop_type_marker','featureprop_type_variant'];
console.log('initial check.');

    var error_ico = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB4AAAAeCAYAAAA7MK6iAAAGB0lEQVRIS42XW2xTdRzHv+fS27oe1q4XWNsNtiGOIBEeSGA1gCbEhKgkRk2MiRF9B0V40wd9EkXxXTAmxkQSE7zFaGIgboOEBxZRubmNsa5ja7t2nLKul9Nz/P3P6elOb5snadqe8///Pv/v7/b/Hw7/49IA7kog+pTKaYc5cMOAOki3vMZULQvwE/R9hdO4n/al4iMc/VnPLI1pf9Fs/kowelzjtDPu/l5I2wfh3hKFw++F0OHUJ1byBRTTWSzfm4V8818sT82AFnBiXzJ+loyr7ay3BY8Fom9oPM779uxE8Jm9EG0kI5OGKi9BW8lDU8q6TY4ecK4O8FIXOJ8f7Hby96vIXLsBTsXR4VT8y1bwluDRYPgDu8/7Xu9rL8BhV1GJT+kwrjq6cZLpV43FhBYhRPtRLPGY+fp7lDLZD2PJxPuN8CbwWCBy1j3Yeyz60iFg+g7U3EMdSLE1FLbxXQ1O4WUL4D0bgM3bMHPhV+Qn458Pp2aPW6fW2RkJRj6Stm4+1XtkPyq3/wSUUhPQVN3IZzA91aoPCA+IdghDT2Lm4mXk7k5/HEvOnjLn1cAspvZg1/n+V59F5dY4ATUd2uje9RVXF0BcBmdWhKHdmPrmF5SSS7WY63YuAEJ4Y1Tpf/15CInb4CpKDZpJp5GTZd2aJEno9vsNl1dXpJlS6d4ijZWrYz001tvtN+CCiEr4cUx99QMS83HxZSoGHTwWjL7j3T10JhC2Q8stga8qzWQyyOVyCH13VYctvLgXkscDf3d3U6TTi4uQG8Z6aKzX66OaIt2eLqQSJWSv3zoxnIx/yrFEHAtF1MFXngafuGso1RUB92cTNahJ0uGdnQh4u2rwVHYJ8qNHLcf2hsN63JlyNbwVE99ewvDCLM/94Y/s92zpuRwedAFUMnwVyuDTDxaajNWUuzsQ2CAh9VCGvJxvO65vU0jPcr2TUKklJlaQuzd3gGOZvHH4iVOSsmAotYAzuWXkVlbaw10uyGs897ic8Ho6dbChGpDFEObH/jrNUbMY6Tu4I2aX53Qor9fsat1m8iuQC8W2cDP+1qCzcHgcDnS7XdXMJsVVeFHqwcylf0YYeH5geHOIX85aoAbYLKXFlQLkYhk9F681JVXjjbkjeyA5bOgmtYZSvaL1D4Orbi8mx6YXCBwpDu722VEq1Kk1Xa6XDisVAstlBX0/j7eF3z+8C5JNhI/A7Fp1sQFnYNidmLieKengge1OO6epa4KZocVyBbKiYvC3v5vgE4d2QBJ5dNuE1e5Vi+0qWON4TN4sMHB4PvqYIyTyVGuW+DYqzpZVPKqoGLp0p63iWwe3oVPg0WVjtdGsmHlAUTnE7xaZq8MjgQF3zCGW68GWnYipXCHoztGpdWN8I9YPF8E9pH4VXo0zgYuKDanJPEuuyGmpVzrZ4TS2PbMXm8mVJ2iBgrPr6v0m6Pjevrb3HVQeHbQAA1ktJ/qZL7jwcCZ3Wm8gTr/7ssdXrCsntoQixb20BpQZL67x3EZKnDTGTCz2ncs4UEgvHzBaZjCienoFCJSMq3GmRraOURNcpuC184gJZvGt0OkkF1epZcaF2iYhesUzdm/FoppDK4PMvWyWaNmYFbJaIcONcDZWZAevanaXsgKUrGJsEsz/bFvsCUUV+yYOvGNVtRlU0yAzZHS3+l1ZbxAEZ9/WsdbYqkWg9EDD3IJlW2QDRgLRo7yLPycEqZ1TQpouF+iHiTE7UMvUZjGzJCfbFCpsMXqA6XeSEq1YeSu2MHuOza9bOstwdHAnwfZ6ZqQhy5smWFZQd+SxZLHuhjR98lrro49pgx321A7uWMVPDcUoRf2yllkrxdayMZ9TUUBIU/jy2tqHPXMCU67ZcLLsI3N2Q7n1ajx3mWprQOb2EmDLUJjKqFPaKKRJxGgo8ia56QvFDZQ9dHoQmxfQOEk/1ioEzHEQl3U31WLaOLZx8XXPmZ3RYPRtagGfKJTtilODYjcWoZkttQoTSwQr0Ieyl4L0biwZ/4xVUquwrJUrTQugl7YYvUM9R5x9tJAB60sbgSYJwl7afqSXttG1gKbh/wBUhcqkqq2mOQAAAABJRU5ErkJggg==';
    var success_ico = 'data:image/png;base64,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';

    // Define a function that adds a checkmark or X after a cvterm textfield
    // to denote if it exists or not.
    var notifyfn = function(name) {
      // Grab the element.
      var element = $('input[name="' + name + '"]');

      // Retrieve the cv_id and value for use with the autocomplete URL.
      var cv_id = element.attr("data-cv_id");
      var value = element.attr("value");
      var url = autocomplete_url_prefix + cv_id + '/' + value;

      console.log(url);
      // Grab the JSON found using the autocomplete callback...
      $.getJSON(url, function( data ) {
        
        console.log(data);
        // If (only one element is returned) OR (is an object with the value set as a property)
        // then we're set!
        if (data.length === 1 || data.hasOwnProperty(value)) {
          element.after('<img alt="Term Exists" title="Term Exists" class="cvterm-status-ico ' + name + '" src="' + success_ico + '" />');
        }
        // Otherwise there are no terms or too many -both of which are a problem :-(.
        else {
          element.after('<img alt="Term Missing" title="Term Missing" class="cvterm-status-ico ' + name + '" src="' + error_ico + '" />');
        }
      });
    };

    // Used to look up the term ([url prefix]/[cv_id]/[term name])
    var autocomplete_url_prefix = Drupal.settings.basePath + 'ajax/genotypes_loader/cvterm/';

    // For each of our cvterm form elements...
    elementNames.forEach(notifyfn);

    // Now we need to detect when they're changing...
    elementNames.forEach( function(name) {
      // Grab the element.
      var element = $('input[name="' + name + '"]');

      // When the element changes...
      element.bind('input', function() {
        // Remove any previous images...
        $('img.' + name).remove();
        // Then add the new notification.
        notifyfn(name);
      });
    });

  }};
})(jQuery);
