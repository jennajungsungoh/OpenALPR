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
            var first_col = '<td><div class="ight-arrow btnHidden"><svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-caret-right-fill" viewBox="0 0 16 16"><path d="m12.14 8.753-5.482 4.796c-.646.566-1.658.106-1.658-.753V3.204a1 1 0 0 1 1.659-.753l5.48 4.796a1 1 0 0 1 0 1.506z"/></svg></div></td>'
            tableRow.append(first_col);

            $.each(value, function (key, val) {
                // if (index == 0 && $.inArray(key, hidecolumns) <= -1) {
                //     if ($('th').length == 0) {
                //         var theaddata = $('<th>').text(key);
                //         tblHeaderRow.append(theaddata);
                //     }
                // }
                if ($.inArray(key, hidecolumns) <= -1) {
                    var tbodydata;
                    switch (key) {
                        case 'plate_number':
                            tbodydata = $('<td>').addClass('rowA').text(val);
                            break;
                        case 'confidence_avg':
                            tbodydata = $('<td>').addClass('rowA').text(val);
                            break;
                        case 'time':
                            tbodydata = $('<td>').addClass('rowA').text(val);
                            break;
                    }


                    var tbodydata = $('<td>').addClass('rowA').text(val);
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