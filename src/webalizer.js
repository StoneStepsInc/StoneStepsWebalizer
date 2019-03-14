/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     
   
	webalizer.js
*/

var ELEMENT_NODE  = 1;        // DOM node types
var TEXT_NODE     = 3;

var KEY_ESC       = 27;       // key-up event key codes
var KEY_HOME      = 36;
var KEY_END       = 35;
var KEY_PAGEUP    = 33;
var KEY_PAGEDOWN  = 34;
var KEY_UP        = 38;
var KEY_DOWN      = 40;
var KEY_LEFT      = 37;
var KEY_RIGHT     = 39;

var keyUpHandlers = [];       // functions to call on keyup event

var frags = null;             // report identifiers (fragments)

//
// Page load event handlers for all three page types 
//
// The caller may pass in a callback function that will be called after all
// standard initialization is done, but before the key-up event handler is 
// installed.
//
function onload_index_page(initCB)
{
   if(initCB)
      initCB();

   document.onkeyup = onPageKeyUpDispatcher;
}

function onload_usage_page(initCB)
{
   // initialize fragment navigation
   initPageFragments();

   if(initCB)
      initCB();

   document.onkeyup = onPageKeyUpDispatcher;
}

function onload_page_all_items(initCB)
{
   if(initCB)
      initCB();

   document.onkeyup = onPageKeyUpDispatcher;
}

//
// Registers a key-up event handler
//
function registerOnKeyUpHandler(handler)
{
   if(handler)
      keyUpHandlers.push(handler);
}

//
// Dispatches page-level key-up events to all registered handlers
//
function onPageKeyUpDispatcher(event)
{
   for(var i = 0; i < keyUpHandlers.length; i++) {
      keyUpHandlers[i](event);
   }

   return true;
}

// ----------------------------------------------------------------------------
//
// Common code
//
// ----------------------------------------------------------------------------

//
// findParentNode
//
// Returns the parent node with the matching tag name of the specified node
//
function findParentNode(node, tagname)
{
   do {
      node = node.parentNode;
   } while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != tagname);
   
   return node;
}

//
// findNextSibling
//
// Returns the next sibling node with the matching tag name of the specified node
//
function findNextSibling(node, tagname)
{
   do {
      node = node.nextSibling;
   } while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != tagname);
   
   return node;
}

//
// findPrevSibling
//
// Returns the previous sibling node with the matching tag name of the specified node
//
function findPrevSibling(node, tagname)
{
   do {
      node = node.previousSibling;
   } while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != tagname);
   
   return node;
}

///
/// @name   Report functions
///
/// @{

//
// Returns the version of the report from the data-version attribute in reportTable. 
// If none is found, version 1 is assumed and if the reportTable is null, a zero
// is returned. 
//
function reportVersion(reportTable)
{
   // check if the report table exists
   if(!reportTable)
      return 0;
   
   // unversioned reports are considered version one
   if(!reportTable.dataset || !reportTable.dataset.version)
      return 1;

   // return the version as a number
   return Number(reportTable.dataset.version);
}

/// @}

//
// Checks if the on-click even target contains a country code and a latitude/longitude 
// and if it does, opens a window using the external map URL passed as an argument, with 
// {{lat}} and {{lon}} patterns replaced with actual latitude/longitude values. 
//
function openExtMapURL(event, ext_map_url)
{
   if(event && event.target && event.target.dataset && event.target.dataset.ccode) {
      if(event.target.dataset.lat && event.target.dataset.lon) {
         ext_map_url = ext_map_url.replace(/{{lat}}/g, event.target.dataset.lat);
         ext_map_url = ext_map_url.replace(/{{lon}}/g, event.target.dataset.lon);

         window.open(ext_map_url, "SSW_ext_map_url");
      }
   }
}

///
/// @name   Fragment navigation functions
///
/// @{

function onKeyUpFragHandler(event)
{
   // if frags is initialized and
   if(frags) {
      // if Ctrl and Alt are down, process navigation keys
      if(event.ctrlKey && event.altKey) {
         switch(event.keyCode) {
            case KEY_HOME:
               document.location.href = "#" + frags[0];
               break;

            case KEY_END:
               document.location.href = "#" + frags[frags.length-1];
               break;
               
            case KEY_UP:
               document.location.href = "#" + getPrevFragId(getFragId(document.location.href));
               break;

            case KEY_DOWN:
               document.location.href = "#" + getNextFragId(getFragId(document.location.href));
               break;
         }
      }
   }
}

//
// initPageFragments
//
// Populates the fragment array with fragment identifiers from the 
// main menu
//
function initPageFragments()
{
   var menu = document.getElementById("main_menu");
   var links;

   // initialize the fragment array
   frags = ["top"];
   
   // index page doesn't have a menu
   if(menu) {
      links = menu.getElementsByTagName("a");
      
      if(links) {
         for(var i = 0; i < links.length; i++)
            frags.push(getFragId(links.item(i).href));
      }
   }

   registerOnKeyUpHandler(onKeyUpFragHandler);
}

//
// getFragId
//
// Returns the fragment identifier in href or an empty string if there 
// is none.
//
function getFragId(href)
{
   var fpos = href? href.lastIndexOf("#") : -1;
   return fpos != -1 ? href.substr(fpos+1) : "";
}

//
// getPrevFragId
//
// Finds fragid in the frags array and returns the previous element. If
// fragid is not found, returns the last element of the array.
//
function getPrevFragId(fragid)
{
   if(fragid) {
      for(var i = 1; i < frags.length; i++) {
         if(frags[i] == fragid)
            return frags[i-1];
      }
   }
   
   return frags[frags.length-1];
}

//
// getNextFragId
//
// Finds fragid in the frags array and returns the next element. If fragid
// is not found, returns the first element of the array.
//
function getNextFragId(fragid)
{
   if(fragid) {
      for(var i = 0; i < frags.length-1; i++) {
         if(frags[i] == fragid)
            return frags[i+1];
      }
   }
   
   return frags[0];
}

/// @}

///
/// @name   JavaScript chart functions
///
/// @{

//
// Define a global config prototype object, so we can define global properties
// in one place and make the config object defined in the report as light as
// possible.
//
var chart_config_prototype = {
   background_color: "#E8E8E8",
   gridline_color: "#808080",
   hits_color: "#00805C",
   files_color: "#0000FF",
   pages_color: "#00C0FF",
   visits_color: "#FFFF00",
   hosts_color: "#FF8000",
   xfer_color: "#FF0000",
   title_color: "#0000FF",
   weekend_color: "#00805C",
   pie_other_color: "#FFFFFF",
   pie_colors: ["#007F5C", "#FF7F00", "#0000FF", "#FF0000", "#00C0FF", "#FFFF00", "#7F007F", "#7FFFC0", "#FF00FF", "#FF5C7F"]
};

//
// Create a chart configuration object with the global configuration object 
// as its prototype and copy enumerable report object properties into the new 
// object.
//
function createChartConfig(report_config)
{
   var config = Object.create(chart_config_prototype);
   
   for(var i in report_config)
      config[i] = report_config[i];

   return config;
}

//
// formatAxisLabel is intended to format numeric chart labels that cannot be 
// precomputed in reports because they are generated by the JavaScript charting 
// package. Unlike the C++ fmt_hr_num function, formatAxisLabel always returns 
// formatted numbers with no digits after the decimal point.
//
function formatAxisLabel(num, sep, config)
{
   var kbyte = config.version && config.decimal_kbytes ? 1000 : 1024;
   var result = num;
   var prefix = 0;

   // config.version didn't exist prior to v2, when human-readable variables were introduced
   if(!config.version || config.classic_kbytes)
      return (result / kbyte).toFixed(0);

   if(num < kbyte)
      return num.toFixed(0);

   do {
      prefix++;
      result /= kbyte;
   } while(result >= kbyte);

   return result.toFixed(0) + sep + config.unit_prefix[prefix-1] + config.xfer_unit;
}

//
// Returns the maximum value from an array of numeric values
//
function getMaxChartValue(values)
{
   var max = 0;

   for(var i = 0; i < values.length; i++) {
      if(values[i] > max)
         max = values[i];
   }

   return max;
}

//
// Returns HTML-encoded value
//
function htmlEncode(value)
{
   function encode_html_char(ch)
   {
      switch(ch) {
         case '<':
            return "&lt;";
         case '>':
            return "&gt;";
         case '"':
            return "&quot;";
         case '\'':
            return "&#x27;";
         case '&':
            return "&amp;";
      }

      // make it visible that the regex is out of sync
      return "#";
   }

   return value.replace(/&|<|>|"|'/g, encode_html_char);
}

//
// getTableDataRows returns an array containing all TR elements from
// all tbody elements of the specified report table.
//
function getTableDataRows(reportTable)
{
   var rows = [];

   if(!reportTable)
      return rows;

   var tbodies = reportTable.tBodies;

   if(!tbodies || !tbodies.length)
      return rows;

   for(var i = 0; i < tbodies.length; i++) {
      var tbody = tbodies.item(i);

      // collapse adjacent text fragments
      tbody.normalize();
   
      // get all rows in this tbody element   
      var col_rows = tbody.rows;
   
      if(!col_rows || !col_rows.length)
         return rows;

      // insert all rows into the array
      for(var k = 0; k < col_rows.length; k++) {
         rows.push(col_rows.item(k));
      }
   }

   return rows;
}

/// @}

///
/// @name   DailyUsageChart
///
/// @{

function DailyUsageChart(unused_version, config, chart)
{
   this.chart = chart;                          // chart information (no series data)
   this.config = config;                        // chart config object
   this.data = getDailyUsageData();             // extract report data from HTML tables
}

DailyUsageChart.prototype = {
   isWeekend: function (value)
   {
      for(var i = 0; i < this.chart.weekends.length; i++) {
         if(this.chart.weekends[i].toString() == value)
            return true;
      }

      return false;
   },

   getDailyUsageMaxHits: function()
   {
      return getMaxChartValue(this.data.hits);
   },

   getDailyUsageMaxXfer: function()
   {
      return getMaxChartValue(this.data.xfer);
   },

   getDailyUsageMaxVisits: function()
   {
      return getMaxChartValue(this.data.visits);
   }
};

//
// getDailyUsageData extracts data from the daily usage table and returns an 
// object containing following data arrays:
//
// {days: [], hits: [], files: [], pages: [], xfer: [], xfer_hr: [], visits: [], hosts: []}
//   
// All arrays contain the same number of elemennts, one for each day present 
// in the daily usage table.
//
// Synchronized arrays are chosen over a single object array (i.e. one object 
// for each day containing all data for the day) because it is easier to split
// off a single series this way (e.g. days and hits or find a mximum value in
// a series) and also in hopes that array access may potentially be implemented 
// a bit faster than object property lookup, although tests do not show much 
// difference.
//
// Versions:
//    v2    - added the data-xfer attribute
//
function getDailyUsageData()
{
   var rows;
   var usage = {days: [], hits: [], files: [], pages: [], xfer: [], xfer_hr: [], visits: [], hosts: []};

   var reportTable = document.getElementById("daily_usage_table");

   if(!reportTable)
      return usage;

   var version = reportVersion(reportTable);

   if(version < 1 || version > 2)
      return usage;

   rows = getTableDataRows(reportTable);

   if(!rows || !rows.length)
      return usage;

   //
   // Given that we generated report tables, we can do minimum validation of
   // the table structure. The version parameter identifies the table layout.
   //
   for(var i = 0; i < rows.length; i++) {
      var cells = rows[i].cells;

      if(!cells || cells.length != 25)
         return usage;

      // extract the counters and push new values into their arrays
      usage.days.push(parseInt(cells.item(0).firstChild.data)); 
      usage.hits.push(parseInt(cells.item(1).firstChild.data));
      usage.files.push(parseInt(cells.item(5).firstChild.data));
      usage.pages.push(parseInt(cells.item(9).firstChild.data));
      usage.visits.push(parseInt(cells.item(13).firstChild.data));
      usage.hosts.push(parseInt(cells.item(17).firstChild.data));

      if(version >= 2) {
         usage.xfer.push(parseInt(cells.item(21).dataset.xfer));
         usage.xfer_hr.push(cells.item(21).firstChild.data);
      }
      else {
         usage.xfer.push(parseInt(cells.item(21).firstChild.data));
         usage.xfer_hr.push(null);
      }
   }

   return usage;
}

/// @}

///
/// @name   HourlyUsageChart
///
/// @{

function HourlyUsageChart(unused_version, config, chart)
{
   this.chart = chart;
   this.config = config;
   this.data = getHourlyUsageData();
}

HourlyUsageChart.prototype = {
   getHourlyUsageMaxHits: function()
   {
      return getMaxChartValue(this.data.hits);
   },

   getHourlyUsageMaxXfer: function()
   {
      return getMaxChartValue(this.data.xfer);
   }
};

//
// getHourlyUsageData extracts data from the hourly usage table and returns an 
// object containing following data arrays:
//
// {hours: [], hits: [], files: [], pages: [], xfer: []}
//   
// All arrays contain the same number of elemennts, one for each hour present 
// in the hourly usage table.
//
// Versions:
//    v2    - added the data-xfer attribute
//
function getHourlyUsageData()
{
   var rows;
   var usage = {hours: [], hits: [], files: [], pages: [], xfer: [], xfer_hr: []};

   var reportTable = document.getElementById("hourly_usage_table");

   if(!reportTable)
      return usage;

   var version = reportVersion(reportTable);

   if(version < 1 || version > 2)
      return usage;

   rows = getTableDataRows(reportTable);

   if(!rows || !rows.length)
      return usage;

   // see the comment in getDailyUsageData
   for(var i = 0; i < rows.length; i++) {
      var cells = rows[i].cells;

      if(!cells || cells.length != 13)
         return usage;

      // extract the counters and push them into their arrays
      usage.hours.push(parseInt(cells.item(0).firstChild.data)); 
      usage.hits.push(parseInt(cells.item(2).firstChild.data));
      usage.files.push(parseInt(cells.item(5).firstChild.data));
      usage.pages.push(parseInt(cells.item(8).firstChild.data));

      if(version >= 2) {
         usage.xfer.push(parseInt(cells.item(11).dataset.xfer));
         usage.xfer_hr.push(cells.item(11).firstChild.data);
      }
      else {
         usage.xfer.push(parseInt(cells.item(11).firstChild.data));
         usage.xfer_hr.push(null);
      }
   }

   return usage;
}

/// @}

///
/// @name   CountryUsageChart
///
/// @{

function CountryUsageChart(unused_version, config, chart)
{
   this.chart = chart;
   this.config = config;
   this.data = getCountryUsageData(this.chart.totalVisits);
}

CountryUsageChart.prototype = {
   getValueCount : function () 
   {
      return this.data.visits_other > 0 ? this.data.countries.length + 1 : this.data.countries.length;
   },

   getVisits : function (index)
   {
      return index < this.data.visits.length ? this.data.visits[index] : this.data.visits_other;
   },

   getLabel : function (index)
   {
      return index < this.data.countries.length ? this.data.countries[index] : this.chart.otherLabel;
   },

   getPercent : function (index)
   {
      return index < this.data.percent.length ? this.data.percent[index] : this.data.percent_other;
   },

   getColor : function (index)
   {
      return index < this.data.countries.length ? this.config.pie_colors[index] : this.config.pie_other_color;
   },

   getCCode : function (index)
   {
      return index < this.data.ccodes.length ? this.data.ccodes[index] : "";
   }
};

//
// getCountryUsageData extracts data from the country usage table and returns an 
// object containing following data members:
//
// {countries: [], visits: [], percent: [], visits_other: 0, percent_other: 0}
//
// The arrays may contain up to 10 elements, one for each country that will be 
// represented by a slice in a pie chart. If there are more than 10 countries in
// the table, visits for the remaining countries will be added up and stored in 
// `visits_other`. 
// 
// Versions:
//    v2    - added a column for page counts
//    v3    - added the data-xfer attribute
//    v4    - added the data-ccode attribute
//
function getCountryUsageData(total_visits)
{
   var rows;
   var usage = {ccodes: [], countries: [], visits: [], percent: [], visits_other: 0, percent_other: 0};
   var visits = 0, disp_visits = 0;
   var percent;
   var layout;
   var layouts = [{columns: 10, i_visits: 7, i_country: 9},
                  {columns: 12, i_visits: 9, i_country: 11}];

   var reportTable = document.getElementById("country_usage_table");

   if(!reportTable)
      return usage;

   var version = reportVersion(reportTable);

   // make sure we have a table layout for this version
   if(version < 1)
      return usage;

   // pick the column layout for the given version of the report
   switch(version) {
      case 1:
         layout = layouts[0];
         break;
      case 2:
      case 3:
      case 4:
         layout = layouts[1];
         break;
   }

   rows = getTableDataRows(reportTable);

   if(!rows || !rows.length)
      return usage;

   //
   // The countries table is never truncated and contains data for all countries.
   // Collect visit counts for the top ten countries in the arrays and keep the 
   // sum for rest of countries in their their own variables.
   //
   // See also the comment in getDailyUsageData
   //
   for(var i = 0; i < Math.min(rows.length, 10); i++) {
      var cells = rows[i].cells;

      // check that the total number of columns is right for this layout
      if(!cells || cells.length != layout.columns)
         return usage;

      visits = parseInt(cells.item(layout.i_visits).firstChild.data);

      // update the total number of visits for displayed countries
      disp_visits += visits;

      // save percentages as strings to avoid any precision issues if converted within charts
      percent = ((visits * 100) / total_visits).toFixed(1) + "%";

      var ctry_cell = cells.item(layout.i_country);

      // get the country code, if one exists, or push an empty string to keep arrays in sync
      if(version >= 4 && ctry_cell.dataset)
         usage.ccodes.push(ctry_cell.dataset.ccode);
      else
         usage.ccodes.push("");

      usage.visits.push(visits);
      usage.countries.push(ctry_cell.firstChild.data);
      usage.percent.push(percent);
   }

   // if there are more countries than the number of slices, update the roll-up slice
   if(disp_visits < total_visits) {
      usage.visits_other = total_visits - disp_visits;
      usage.percent_other = ((usage.visits_other * 100) / total_visits).toFixed(1) + "%";
   }

   return usage;
}

/// @}

///
/// @name   MonthlySummaryChart
///
/// @{

function MonthlySummaryChart(unused_version, config, chart)
{
   this.chart = chart;
   this.config = config;
   this.data = getMonthlySummaryData();
}

MonthlySummaryChart.prototype = {
   getMonthlySummaryMaxHits: function()
   {
      return getMaxChartValue(this.data.hits);
   },

   getMonthlySummaryMaxXfer: function()
   {
      return getMaxChartValue(this.data.xfer);
   },

   getMonthlySummaryMaxVisits: function()
   {
      return getMaxChartValue(this.data.visits);
   }
};

//
// getMonthlySummaryData extracts data from the monthly summary report table and 
// returns an object containing following data arrays:
//
// {months: [{year: 2016, month: 4}], hits: [], files: [], pages: [], visits: [], hosts: [], xfer: [], xfer_hr: []}
//   
// All arrays contain the same number of elements, one for each month present in 
// the monthly summary report table. The months array contains objects describing
// data point year and month, so its possible to use it with any other data array
// to form a complete x/y series.
//
// Versions:
//    v2    - added the data-xfer attribute
//
function getMonthlySummaryData()
{
   var rows;
   var usage = {months: [], hits: [], files: [], pages: [], visits: [], hosts: [], xfer: [], xfer_hr: []};

   var reportTable = document.getElementById("monthly_summary_table");

   if(!reportTable)
      return usage;

   var version = reportVersion(reportTable);

   if(version < 1 || version > 2)
      return usage;

   rows = getTableDataRows(reportTable);

   if(!rows || !rows.length)
      return usage;

   //
   // The table lists months in the descending order, but the chart shows earlier
   // dates on the left, so we need to reverse the order of elements in the usage
   // arrays to allow the code that translates these arrays to the chart format,
   // which typically will have an element for each actual month, to walk these 
   // arrays at different speeds instead of having to match usage year/month values
   // to the values on the axis.
   //
   for(var i = rows.length - 1; i >= 0; i--) {
      var cells = rows[i].cells;

      //
      // HTML4 required tfoot to appear before any tbody element, which made it 
      // less useful when totals were generated while tbody was being populated. 
      // HTML5 lifts this restriction and the report will eventually be reorked 
      // to have all footer rows inside tfoot elements. Until then, just check 
      // the number of columns to skip the footer tbody, which has fewer columns. 
      //
      if(cells.length == 7)
         continue;

      if(!cells || cells.length != 11)
         return usage;

      // make sure HTML5 dataset is supported
      if(!cells[0].dataset)
         return usage;

      // extract the counters and push them into their arrays
      usage.months.push({year: parseInt(cells[0].dataset.year), month: parseInt(cells[0].dataset.month)});
      usage.hits.push(parseInt(cells[10].firstChild.data));
      usage.files.push(parseInt(cells[9].firstChild.data));
      usage.pages.push(parseInt(cells[8].firstChild.data));
      usage.visits.push(parseInt(cells[7].firstChild.data));
      usage.hosts.push(parseInt(cells[5].firstChild.data));

      if(version >= 2) {
         usage.xfer.push(parseInt(cells[6].dataset.xfer));
         usage.xfer_hr.push(cells[6].firstChild.data);
      }
      else {
         usage.xfer.push(parseInt(cells[6].firstChild.data));
         usage.xfer_hr.push(null);
      }
   }

   return usage;
}

/// @}

///
/// @name   Compatibility functions (v3.10.3 to v4.0.4).
///
/// Keep these functions in the script file to avoid JavaScript errors when older 
/// reports are viewed.
///
/// @{

function onloadpage(pagetype)
{
   var PAGE_INDEX = 0
   var PAGE_USAGE = 1;

   // initialize fragment navigation (also was called without pagetype)
   if(pagetype == undefined || pagetype == PAGE_USAGE)
      initPageFragments();
}

function onpagekeyup(event)
{
   onPageKeyUpDispatcher(event);
}

function onclickmenu(a)
{
	return true;
}

/// @}
