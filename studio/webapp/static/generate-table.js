(function ($) {
    /**
     * data - array of record
     * hidecolumns, array of fields to hide
     * usage : $("selector").generateTable(json, ['field1', 'field5']);
     */
    'use strict';
    $.fn.generateTable = function (data, hidecolumns) {
        if ($.isArray(data) === false) {
            console.log('Invalid Data');
            return;
        }
        var container = $(this),
            table = $('<table id="info">'),
            tableHead = $('<thead>'),
            tableBody = $('<tbody>'),
            tblHeaderRow = $('<tr>');

        // if ($('#info').children(tableBody).length > 0) {
        //     s$('#info').children(tableBody).remove();
        // }


        $.each(data, function (index, value) {
            var tableRow = $('<tr>').addClass(index % 2 === 0 ? 'even' : 'odd');
            $.each(value, function (key, val) {
                // if (index == 0 && $.inArray(key, hidecolumns) <= -1) {
                //     if ($('th').length == 0) {
                //         var theaddata = $('<th>').text(key);
                //         tblHeaderRow.append(theaddata);
                //     }
                // }
                if ($.inArray(key, hidecolumns) <= -1) {
                    var tbodydata = $('<td>').text(val);
                    tableRow.append(tbodydata);
                }
            });
            $(tableBody).append(tableRow);
        });

        $(this).append(tableBody);

        // $(tblHeaderRow).appendTo(tableHead);
        // if ($($this).length == 0) {
        //     tableHead.appendTo(table);
        //     tableBody.appendTo(table);
        //     $(this).append(table);
        // } else {
        //     $(this).append(tableBody);
        // }
        return this;
    };
})(jQuery);