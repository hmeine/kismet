(
  typeof define === "function" ? function (m) { define("kismet-ui-js", m); } :
  typeof exports === "object" ? function (m) { module.exports = m(); } :
  function(m){ this.kismet_ui = m(); }
)(function () {

"use strict";

var exports = {};

var batteryTid; 

exports.BatteryUi = function(spinner, power, time) {
    spinner.show();
    kismet.GetSystemStatus(function(status) {
        if (status['kismet.system.battery.charging'] == "charging") {
            power.attr("src", "/images/icon_battery_full_charge.svg");
            time.text("Charging " + 
                    status['kismet.system.battery.percentage'] + "% ");
        } else if (status['kismet.system.battery.charging'] == "charged") {
            power.attr("src", "/images/icon_battery_full_charge.svg");
            time.text("Charged");
        } else {
            if (status['kismet.system.battery.percentage'] < 25)
                power.attr("src", "/images/icon_battery_0.svg");
            else if (status['kismet.system.battery.percentage'] < 50)
                power.attr("src", "/images/icon_battery_25.svg");
            else if (status['kismet.system.battery.percentage'] < 75)
                power.attr("src", "/images/icon_battery_50.svg");
            else if (status['kismet.system.battery.percentage'] < 90)
                power.attr("src", "/images/icon_battery_75.svg");
            else 
                power.attr("src", "/images/icon_battery_100.svg");

            var s = status['kismet.system.battery.remaining'];

            if (s > 0) {
                var h = Math.floor(s / 3600);
                s -= 3600 * h;
                var m = Math.floor(s / 60);
                s -= 60 * m;

                if (m < 10)
                    m = '0' + m
                
                time.text(status['kismet.system.battery.percentage'] + "% " + h+"h "+m+"m");
            } else {
                time.text(status['kismet.system.battery.percentage'] + "%")
            }
        }

    }, function() {
        power.attr("src", "/images/icon/battery_no_battery_power.svg");
        power.text("ERROR");
    });
    spinner.hide();

    batteryTid = setTimeout(exports.BatteryUi, 5000, spinner, power, time);
}

// List of datatable columns we have available
exports.DeviceColumns = new Array();

/* Add a jquery datatable column that the user can pick from, with various 
 * options:
 *
 * sTitle: datatable column title
 * name: datatable 'name' field (optional)
 * mData: datatable field spec
 * cbmodule: string name of callback module (ie "kismet_dot11") (optional)
 * renderfunc: string name of datatable render function, taking DT arguments
 *  (data, type, row, meta), found in cbmodule (optional)
 * drawfunc: string name of a draw function, taking arguments:
 *  dyncolumn - The dynamic column (this)
 *  datatable - A DataTable() object of the table we're operating on
 *  row - The row we're operating on, which should be visible
 *  found in the namespace cbmodule.  This will be called during the drawCallback
 *  stage of the table, on visible rows. (optional)
 */
exports.AddDeviceColumn = function(id, options) {
    var coldef = {
        kismetId: id,
        sTitle: options.sTitle,
        mData: options.mData
    };

    if ('name' in options) {
        coldef.name = options.name;
    }

    if ('orderable' in options) {
        coldef.bSortable = options.orderable;
    }

    if ('visible' in options) {
        coldef.bVisible = options.visible;
    }

    if ('searchable' in options) {
        coldef.bSearchable = options.searchable;
    }

    // Set the render function to proxy through the module+function
    if ('cbmodule' in options && 'renderfunc' in options) {
        coldef.mRender = function(data, type, row, meta) {
            return window[options.cbmodule][options.renderfunc](data, type, row, meta);
        }
    }

    // Set an arbitrary draw hook we call ourselves during the draw loop later
    if ('cbmodule' in options && 'drawfunc' in options) {
        coldef.kismetdrawfunc = function(col, datatable, row) {
            return window[options.cbmodule][options.drawfunc](col, datatable, row);
        }
    }

    exports.DeviceColumns.push(coldef);
}

/* Return columns from the selected list of column IDs */
exports.GetDeviceColumns = function(selected) {
    var ret = new Array();

    for (var i in exports.DeviceColumns) {
        // console.log("Adding column " + exports.DeviceColumns[i].kismetId);
        ret.push(exports.DeviceColumns[i]);
    }

    return ret;
}

exports.DeviceDetails = new Array();

/* Register a device detail accordion panel, taking an id for the panel 
 * content, a title presented to the user, a position in the list, and
 * options.  Because details are directly rendered all the time and
 * can't be moved around / saved as configs like columns can, callbacks
 * are just direct functions here.
 *
 * filter and render take one argument, the data to be shown
 * filter: function(data) {
 *  return false;
 * }
 *
 * render: function(data) {
 *  return "Some content";
 * }
 *
 * draw takes the device data and a container element as an argument:
 * draw: function(data, element) {
 *  e.append("hi");
 * }
 * */
exports.AddDeviceDetail = function(id, title, position, options) {
    var settings = $.extend({
        "filter": null,
        "render": null,
        "draw": null
    }, options);

    var det = {
        id: id,
        title: title,
        position: position,
        options: settings
    };

    exports.DeviceDetails.push(det);

    exports.DeviceDetails.sort(function(a, b) {
        return b.position < a.position;
    });
}

exports.GetDeviceDetails = function() {
    return exports.DeviceDetails;
}

exports.DeviceDetailWindow = function(key) {
    // Generate a unique ID for this dialog
    var dialogid = "devicedialog" + key;
    var dialogmatch = '#' + dialogid;

    var h = $(window).height() - 10;

    // If we're on a wide-screen browser, try to split it into 3 details windows
    var w = ($(window).width() / 3) - 10;

    // If we can't, split it into 2.  This seems to look better when people 
    // don't run full-size browser windows.
    if (w < 450) {
        w = ($(window).width() / 2) - 5;
    }

    // Finally make it full-width if we're still narrow
    if (w < 450) {
        w = $(window).width() - 5;
    }

    $.jsPanel({
        id: dialogid,
        headerTitle: 'Device Details',
        position: {
            "my": "left-top",
            "at": "left-top",
            "of": "window",
            "offsetX": 2,
            "offsetY": 10,
            "autoposition": "RIGHT"
        },

        resizable: {
            minWidth: 450,
            maxWidth: 600,
            minHeight: 400,
            stop: function(event, ui) {
                $('div#accordion', ui.element).accordion("refresh");
            }
        },

        onmaximized: function() {
            $('div#accordion', this.content).accordion("refresh");
        },

        onnormalized: function() {
            $('div#accordion', this.content).accordion("refresh");
        },

        callback: function() {
            var panel = this;
            var content = this.content;

            $.get("/devices/by-key/" + key + "/device.json")
                .done(function(fulldata) {
                    panel.headerTitle(fulldata['kismet_device_base_name']);

                    var accordion = $('<div />', {
                        id: 'accordion'
                    });

                    content.append(accordion);

                    var detailslist = kismet_ui.GetDeviceDetails();

                    for (var dii in detailslist) {
                        var di = detailslist[dii];

                        // Do we skip?
                        if ('filter' in di.options &&
                                typeof(di.options.filter) === 'function') {
                            if (di.options.filter(fulldata) == false) {
                                continue;
                            }
                        }

                        var vheader = $('<h3 />', {
                            id: "header" + di.id,
                            html: di.title
                        });

                        var vcontent = $('<div />', {
                            id: di.id,
                            //class: 'autosize'
                        });

                        // Do we have pre-rendered content?
                        if ('render' in di.options &&
                                typeof(di.options.render) === 'function') {
                            vcontent.html(di.options.render(fulldata));
                        }

                        accordion.append(vheader);
                        accordion.append(vcontent);

                        if ('draw' in di.options &&
                                typeof(di.options.draw) === 'function') {
                            di.options.draw(fulldata, vcontent);
                        }
                    }
                    accordion.accordion({ heightStyle: 'fill' });
                });
        }
    }).resize({
        width: w, 
        height: h,
        callback: function(panel) {
            $('div#accordion', this.content).accordion("refresh");
        },
    });
};

exports.RenderTrimmedTime = function(opts) {
    return (new Date(opts['value'] * 1000).toString()).substring(4, 25);
}

exports.RenderHumanSize = function(opts) {
    return kismet.HumanReadableSize(opts['value']);
};

return exports;

});

