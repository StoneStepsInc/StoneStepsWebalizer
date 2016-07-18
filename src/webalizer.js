/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

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

var helpbox = null;           // help box div
var helptext = null;          // hidden help text container div

//
// Page load event handlers for all three page types 
//
// The caller may pass in a callback function that will be called after all
// standard initialization is done, but before the key-up event handler is 
// installed.
//
function onload_index_page(initCB)
{
   initHelp();

   if(initCB)
      initCB();

   document.onkeyup = onPageKeyUpDispatcher;
}

function onload_usage_page(initCB)
{
   // innitialize fragment navigation
   initPageFragments();

   initHelp();

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

// ----------------------------------------------------------------------------
//
// Fragment navigation
//
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
//
// Help
//
// ----------------------------------------------------------------------------

function onKeyUpHelpHandler(event)
{
   // check if it's the ESC key
   if(event.keyCode == KEY_ESC) {
      // if the help is visible, hide it and return
      if(isHelpShown()) {
         hideHelp();
         return;
      }
   }      
}

function initHelp()
{
   // find the help box div
   if((helpbox = document.getElementById("helpbox")) != null) 
      helpbox.normalize();
   
   // find the help container div
   helptext = document.getElementById("helptext");

   // set up help keyboard handler only if we can show help
   if(helpbox && helptext)
      registerOnKeyUpHandler(onKeyUpHelpHandler);
}

//
// findHelpTopic
//
// Walks the help container and returns a copy of the node whose title
// attribute matches topic
//
function findHelpTopic(topic)
{
   var node;

   if(helptext) {
      // start with the first child (may be text node)
      node = helptext.firstChild;
   
      // and loop until we found the topic div
      while(node) {
         if(node.nodeType == ELEMENT_NODE && node.title == topic) {
            // clone the node to keep the source remains intact
            node = node.cloneNode(true);
            
            // remove the title attribute, so the browser doesn't show it as a tool tip
            node.removeAttribute("title");
            
            return node;
         }
            
         node = node.nextSibling;
      }
   }
   
   // return a text node if the topic is not found
   return document.createTextNode("Unknown topic (" + topic + ")");
}

//
// showHelp
//
// Copies text from the title node into the help box title div. Also
// locates help definition div by topic and copies it into the help 
// box.
//
//                                      help topic
//                                      v
// <span onclick="showHelp(event, this, topic)">Hits</span>
// ^                              ^             ^
// title node                     title node     title
//
function showHelp(event, titlenode, topic)
{
   var top, left;
   var title = "";
   var titlediv;
   var scrwidth, scrheight;
   var offsetx, offsety;
   
   // nothing to show
   if(!helpbox || !titlenode)
      return;
   
   // get the text from the title node
   titlenode.normalize();
   title = titlenode.firstChild.data;
   
   // and put it in the title div, after the div with x
   titlediv = helpbox.firstChild;
   
   if(titlediv.firstChild.nextSibling)
      titlediv.replaceChild(document.createTextNode(title), titlediv.firstChild.nextSibling);
   else
      titlediv.appendChild(document.createTextNode(title));
   
   // find a help entry by topic and put it after the title div
   if(titlediv.nextSibling)
      helpbox.replaceChild(findHelpTopic(topic), titlediv.nextSibling);
   else
      helpbox.appendChild(findHelpTopic(topic));

   // place the help box around the mouse pointer
   scrwidth = document.body.parentNode.clientWidth;
   scrheight = document.body.parentNode.clientHeight;
   
   offsetx = (event.clientX > scrwidth/2) ? -350 : 20;
   offsety = (event.clientY > scrheight/2) ? -75 : 20;
   
	top = Math.round(document.body.parentNode.scrollTop + event.clientY + offsety);
	left = Math.round(document.body.parentNode.scrollLeft + (event.clientX + offsetx));
	
	helpbox.style.top = top + "px";
	helpbox.style.left = left + "px";

   // and make it visible
   helpbox.style.display = "block";
}

function hideHelp()
{
   if(helpbox.style.display != "none")
      helpbox.style.display = "none";
}

function isHelpShown()
{
   return helpbox.style.display != "none";
}

// ----------------------------------------------------------------------------
//
// Charts
//
// ----------------------------------------------------------------------------

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
// all tbody elements of the table identified by table_id.
//
function getTableDataRows(table_id)
{
   var rows = [];
   var tbodies = document.getElementById(table_id).tBodies;

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

//
// DailyUsageChart
//

function DailyUsageChart(version, config, chart)
{
   this.version = version;                      // daily usage table layout version
   this.chart = chart;                          // chart information (no series data)
   this.config = config;                        // chart config object
   this.data = getDailyUsageData(this.version); // extract report data from HTML tables
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
// {days: [], hits: [], files: [], pages: [], xfer: [], visits: [], hosts: []}
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
function getDailyUsageData(version)
{
   var rows;
   var usage = {days: [], hits: [], files: [], pages: [], xfer: [], visits: [], hosts: []};

   if(version != 1)
      return usage;

   rows = getTableDataRows("daily_usage_table");

   if(!rows || !rows.length)
      return usage;

   //
   // Given that we generated report tables, we can do minimum validation of
   // the table structure. The version parameter identifies the table layout.
   // There's only one version at this point.
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

      if(cells.item(21).dataset.xfer)
         usage.xfer.push(parseInt(cells.item(21).dataset.xfer));
      else
         usage.xfer.push(parseInt(cells.item(21).firstChild.data));
   }

   return usage;
}

//
// HourlyUsageChart
//

function HourlyUsageChart(version, config, chart)
{
   this.version = version;                      // hourly usage table layout version
   this.chart = chart;
   this.config = config;
   this.data = getHourlyUsageData(this.version);
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
function getHourlyUsageData(version)
{
   var rows;
   var usage = {hours: [], hits: [], files: [], pages: [], xfer: []};

   if(version != 1)
      return usage;

   rows = getTableDataRows("hourly_usage_table");

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

      if(cells.item(11).dataset.xfer)
         usage.xfer.push(parseInt(cells.item(11).dataset.xfer));
      else
         usage.xfer.push(parseInt(cells.item(11).firstChild.data));
   }

   return usage;
}

//
// CountryUsageChart
//

function CountryUsageChart(version, config, chart)
{
   this.version = version;                   // country table layout version
   this.chart = chart;
   this.config = config;
   this.data = getCountryUsageData(this.version, this.chart.totalVisits);
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
function getCountryUsageData(version, total_visits)
{
   var rows;
   var usage = {countries: [], visits: [], percent: [], visits_other: 0, percent_other: 0};
   var visits = 0, disp_visits = 0;
   var percent;
   var layout;
   var layouts = [{columns: 10, i_visits: 7, i_country: 9},
                  {columns: 12, i_visits: 9, i_country: 11}];

   // make sure we have a table layout for this version
   if(version < 1 || version >= layouts.length)
      return usage;

   layout = layouts[version-1];

   rows = getTableDataRows("country_usage_table");

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

      usage.visits.push(visits);
      usage.countries.push(cells.item(layout.i_country).firstChild.data);
      usage.percent.push(percent);
   }

   // if there are more countries than the number of slices, update the roll-up slice
   if(disp_visits < total_visits) {
      usage.visits_other = total_visits - disp_visits;
      usage.percent_other = ((usage.visits_other * 100) / total_visits).toFixed(1) + "%";
   }

   return usage;
}

//
// MonthlySummaryChart
//

function MonthlySummaryChart(version, config, chart)
{
   this.version = version;                   // monthly usage table layout version
   this.chart = chart;
   this.config = config;
   this.data = getMonthlySummaryData(version);
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
// {months: [{year: 2016, month: 4}], hits: [], files: [], pages: [], visits: [], hosts: [], xfer: []}
//   
// All arrays contain the same number of elements, one for each month present in 
// the monthly summary report table. The months array contains objects describing
// data point year and month, so its possible to use it with any other data array
// to form a complete x/y series.
//
function getMonthlySummaryData(version)
{
   var rows;
   var usage = {months: [], hits: [], files: [], pages: [], visits: [], hosts: [], xfer: []};

   if(version != 1)
      return usage;

   rows = getTableDataRows("monthly_summary_table");

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

      if(cells[6].dataset.xfer)
         usage.xfer.push(parseInt(cells[6].dataset.xfer));
      else
         usage.xfer.push(parseInt(cells[6].firstChild.data));
   }

   return usage;
}
