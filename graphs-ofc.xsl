<?xml version="1.0" encoding="utf-8" ?>
<!--
   webalizer - a web server log analysis program

   Copyright (c) 2004-2014, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
 -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- 
   This XSL template uses Open Flash Chart, which can be downloaded at this
   location:

   http://teethgrinder.co.uk/open-flash-chart/

   If you use FireFox v3 to browse XML report from disk, you will need
   to relax a security restriction. Type about:config in the URL line, 
   then type "fileuri" (without quotes) and change this setting to false:

   security.fileuri.strict_origin_policy

 -->      

<!-- Additional OFC colors (not configurable in webalizer.conf) -->
<xsl:variable name="graph_hits_bg_color">00603C</xsl:variable>
<xsl:variable name="graph_files_bg_color">0000CF</xsl:variable>
<xsl:variable name="graph_pages_bg_color">00A0CF</xsl:variable>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_html_head_ofc
   Context      : /sswdoc
   Parameters   : none
   Description  : Outputs OFC-related part of the HTML head element
-->
<xsl:template name="output_html_head_ofc">
<script type="text/javascript"><xsl:attribute name="src"><xsl:value-of select="$ofcpath"/>swfobject.js</xsl:attribute></script>
<script type="text/javascript">
<![CDATA[
function onrollout(name)
{
  document.getElementById(name).rollout();
}

function max_series_value(series)
{
   var arr = series.split(",");
   var max = 0;
   var value;
   
   for(var i = 0; i < arr.length; i++) {
      if(arr[i] != "null") {
         if(parseInt(arr[i]) > max)
            max = arr[i];
      }
   }
   
   return max;
}
]]>
</script>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***              Daily Totals Graph Template (OFC)            *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;daily_totals&quot;]" mode="graph-flash-ofc">

<!-- series identifiers -->
<xsl:variable name="i_hits" select="2"/>
<xsl:variable name="i_files" select="3"/>
<xsl:variable name="i_pages" select="4"/>
<xsl:variable name="i_visits" select="5"/>
<xsl:variable name="i_hosts" select="6"/>
<xsl:variable name="i_xfer" select="7"/>

<!-- series variables -->
<xsl:variable name="s_hits"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_hits"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_files"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_files"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_pages"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_pages"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_visits"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_visits"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_hosts"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_hosts"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_xfer"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_xfer"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>

<div id="daily_usage_graph" class="graph_holder" style="width: 768px;">

<script type="text/javascript">
<![CDATA[
//
// JavaScript code below sets up daily usage chart for Open Flash Chart. 
//
// If you need to use any of these characters (even inside comments and 
// quoted JavaScript literals),
//
//   < > &
//
// you will need to wrap them in a CDATA section, same way this comment 
// is. Alternatively, you can replace these characters with:
//
//   &lt; &gt; &amp;
//
// For example:
//
// so.addVariable("filled_bar","100,#00805C,#00603C,Hits&lt;,10");
//                                                      ^^^^
]]>

//
// Create the chart object
//
var sody = new SWFObject("<xsl:value-of select="$ofcpath"/>open-flash-chart.swf", "ofc_daily", "768", "300", "9", "#FFFFFF");

sody.addParam("allowScriptAccess", "sameDomain" );
sody.addParam("onmouseout", "onrollout(\"ofc_daily\");" );
sody.addVariable("variables","true");
sody.addVariable("num_decimals", 0);

//
// Set up chart colors
//
sody.addVariable("bg_colour", "#<xsl:value-of select="$graph_bg_color"/>");
sody.addVariable("x_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sody.addVariable("x_grid_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sody.addVariable("y_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sody.addVariable("y_grid_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sody.addVariable("y2_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");

//
// Set a tool tip to be the name of the series and the value
//
sody.addVariable("tool_tip", "#key#: #val#");

//
// Chart title (OFC does not support bold style)
//
sody.addVariable("title","<xsl:value-of select="graph/attribute::title"/>,{font-size: 14px; color: #<xsl:value-of select="$graph_title_color"/>; text-align: left; padding-left: 20px;}");

//
// Left Y axis (hits, files, pages)
//
sody.addVariable("y_label_size", "10",1);
sody.addVariable("y_ticks", "0,0,4");         // ticks-axis padding, axis offset, ticks
sody.addVariable("y_max", max_series_value("<xsl:value-of select="$s_hits"/>"));

//
// Right Y axis (visits, hosts)
//
sody.addVariable("show_y2", true);
sody.addVariable("y2_lines", "4,5");         // 4 and 5 are series IDs (below)
sody.addVariable("y2_max", max_series_value("<xsl:value-of select="$s_visits"/>") * 2);

//
// X axis (days of the month)
//
sody.addVariable("x_labels","<xsl:call-template name="series_labels_ofc"><xsl:with-param name="items" select="graph/days"/></xsl:call-template>");
sody.addVariable("x_axis_steps","1");

//
// Hits (bar: alpha, fg-color, bg-color, legend, legend-size)
//
sody.addVariable("filled_bar","100,#<xsl:value-of select="$graph_hits_color"/>,#<xsl:value-of select="$graph_hits_bg_color"/>,<xsl:value-of select="columns/col[$i_hits]/attribute::title"/>,10");
sody.addVariable("values","<xsl:value-of select="$s_hits"/>");

//
// Files (bar: alpha, fg-color, bg-color, legend, legend-size)
//
sody.addVariable("filled_bar_2","100,#<xsl:value-of select="$graph_files_color"/>,#<xsl:value-of select="$graph_files_bg_color"/>,<xsl:value-of select="columns/col[$i_files]/attribute::title"/>,10");
sody.addVariable("values_2","<xsl:value-of select="$s_files"/>");

//
// Pages (bar: alpha, fg-color, bg-color, legend, legend-size)
//
sody.addVariable("filled_bar_3","100,#<xsl:value-of select="$graph_pages_color"/>,#<xsl:value-of select="$graph_pages_bg_color"/>,<xsl:value-of select="columns/col[$i_pages]/attribute::title"/>,10");
sody.addVariable("values_3","<xsl:value-of select="$s_pages"/>");

//
// Visits (line: line-width, fg-color, legend, legend-size, point-size)
//
sody.addVariable("line_hollow_4","3,#<xsl:value-of select="$graph_visits_color"/>,<xsl:value-of select="columns/col[$i_visits]/attribute::title"/>,10,5");
sody.addVariable("values_4","<xsl:value-of select="$s_visits"/>");

//
// Hosts (line: line-width, fg-color, legend, legend-size, point-size)
//
sody.addVariable("line_hollow_5","3,#<xsl:value-of select="$graph_hosts_color"/>,<xsl:value-of select="columns/col[$i_hosts]/attribute::title"/>,10,5");
sody.addVariable("values_5","<xsl:value-of select="$s_hosts"/>");

//
// Create the chart inside the daily_usage_graph element
//
sody.write("daily_usage_graph");
</script>

</div>

<xsl:call-template name="output_notes"/>

</xsl:template>

<!-- ***************************************************************** -->
<!-- ***            Hourly Totals Graph Template (OFC)             *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;hourly_totals&quot;]" mode="graph-flash-ofc">

<!-- series identifiers -->
<xsl:variable name="i_hits" select="2"/>
<xsl:variable name="i_files" select="3"/>
<xsl:variable name="i_pages" select="4"/>
<xsl:variable name="i_xfer" select="5"/>

<!-- series variables -->
<xsl:variable name="s_hits"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_hits"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_files"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_files"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_pages"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_pages"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_xfer"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_xfer"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>

<div id="hourly_usage_graph" class="graph_holder" style="width: 768px">

<script type="text/javascript">
//
// See comments in the daily graph template for more information
//

//
// Create the chart object
//
var sohr = new SWFObject("<xsl:value-of select="$ofcpath"/>open-flash-chart.swf", "ofc_hourly", "768", "300", "9", "#FFFFFF");

sohr.addParam("allowScriptAccess", "sameDomain" );
sohr.addParam("onmouseout", "onrollout(\"ofc_hourly\");" );
sohr.addVariable("variables","true");
sohr.addVariable("num_decimals", 0);

//
//   Set up chart colors
//
sohr.addVariable("bg_colour", "#<xsl:value-of select="$graph_bg_color"/>");
sohr.addVariable("x_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sohr.addVariable("x_grid_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sohr.addVariable("y_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sohr.addVariable("y_grid_colour", "#<xsl:value-of select="$graph_grid_color"/>");
sohr.addVariable("y2_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");

//
// Set a tool tip to be the name of the series and the value
//
sohr.addVariable("tool_tip", "#key#: #val#");

//
// Chart title (OFC does not support bold style)
//
sohr.addVariable("title","<xsl:value-of select="graph/attribute::title"/>,{font-size: 14px; color: #<xsl:value-of select="$graph_title_color"/>; text-align: left; padding-left: 20px;}");

//
// Left Y axis (hits, files, pages)
//
sohr.addVariable("y_label_size", "10",1);
sohr.addVariable("y_ticks", "0,0,4");         // ticks-axis padding, axis offset, ticks
sohr.addVariable("y_max", max_series_value("<xsl:value-of select="$s_hits"/>"));

//
// Right Y axis (xfer)
//
sohr.addVariable("show_y2", true);
sohr.addVariable("y2_lines", "4");               // 4 is the series IDs (below)
sohr.addVariable("y2_max", max_series_value("<xsl:value-of select="$s_xfer"/>") * 1.5);

//
// X axis (days of the month)
//
sohr.addVariable("x_labels","<xsl:call-template name="series_labels_ofc"><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="offset" select="-1"/></xsl:call-template>");
sohr.addVariable("x_axis_steps","1");

//
// Hits (bar: alpha, fg-color, bg-color, legend, legend-size)
//
sohr.addVariable("filled_bar","100,#<xsl:value-of select="$graph_hits_color"/>,#<xsl:value-of select="$graph_hits_bg_color"/>,<xsl:value-of select="columns/col[$i_hits]/attribute::title"/>,10");
sohr.addVariable("values","<xsl:value-of select="$s_hits"/>");

//
// Files (bar: alpha, fg-color, bg-color, legend, legend-size)
//
sohr.addVariable("filled_bar_2","100,#<xsl:value-of select="$graph_files_color"/>,#<xsl:value-of select="$graph_files_bg_color"/>,<xsl:value-of select="columns/col[$i_files]/attribute::title"/>,10");
sohr.addVariable("values_2","<xsl:value-of select="$s_files"/>");

//
// Pages (bar: alpha, fg-color, bg-color, legend, legend-size)
//
sohr.addVariable("filled_bar_3","100,#<xsl:value-of select="$graph_pages_color"/>,#<xsl:value-of select="$graph_pages_bg_color"/>,<xsl:value-of select="columns/col[$i_pages]/attribute::title"/>,10");
sohr.addVariable("values_3","<xsl:value-of select="$s_pages"/>");

//
// Transfer (line: line-width, fg-color, legend, legend-size, point-size)
//
sohr.addVariable("line_hollow_4","3,#<xsl:value-of select="$graph_xfer_color"/>,<xsl:value-of select="columns/col[$i_xfer]/attribute::title"/>,10,5");
sohr.addVariable("values_4","<xsl:value-of select="$s_xfer"/>");

//
// Create the chart inside the daily_usage_graph element
//
sohr.write("hourly_usage_graph");
</script>
</div>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***            Top Countries Report Template (OFC)            *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_countries&quot;]" mode="graph-flash-ofc">

<!-- series identifiers -->
<xsl:variable name="i_visits" select="5"/>
<xsl:variable name="i_country" select="6"/>

<!-- series variables -->
<xsl:variable name="s_visits">
   <xsl:call-template name="series_data_ofc">
   <xsl:with-param name="series" select="$i_visits"/>
   <xsl:with-param name="items" select="graph/slices"/>
   <xsl:with-param name="percent" select="true()"/>
</xsl:call-template></xsl:variable>

<xsl:variable name="s_countries">
   <xsl:call-template name="series_data_ofc">
   <xsl:with-param name="series" select="$i_country"/>
   <xsl:with-param name="items" select="graph/slices"/>
</xsl:call-template></xsl:variable>

<div id="country_usage_graph" class="graph_holder" style="width: 512px;">

<script type="text/javascript">
//
// See comments in the daily graph template for more information
//

//
// Create the chart object
//
var socn = new SWFObject("<xsl:value-of select="$ofcpath"/>open-flash-chart.swf", "ofc_country", "512", "300", "9", "#FFFFFF");

socn.addParam("allowScriptAccess", "sameDomain" );
socn.addParam("onmouseout", "onrollout(\"ofc_country\");" );
socn.addVariable("variables","true");
socn.addVariable("num_decimals", 0);

//
//   Set up chart colors
//
socn.addVariable("bg_colour", "#<xsl:value-of select="$graph_bg_color"/>");

//
// Set a tool tip to be the name of the series and the value
//
socn.addVariable("tool_tip", "#x_label#: #val#%25");

//
// Chart title (OFC does not support bold style)
//
socn.addVariable("title","<xsl:value-of select="graph/attribute::title"/>,{font-size: 14px; color: #<xsl:value-of select="$graph_title_color"/>; text-align: left; padding-left: 20px;}");

//
// OFC chart labels overlaps with the chart title, so hide them
//
socn.addVariable("pie","80,#000000,{display: none},1,,1");
socn.addVariable("values","<xsl:value-of select="$s_visits"/><xsl:if test="graph/other">,<xsl:value-of select="graph/other/data[$i_visits]/sum/attribute::percent"/></xsl:if>");
socn.addVariable("pie_labels","<xsl:value-of select="$s_countries"/><xsl:if test="graph/other">,<xsl:value-of select="graph/other/data[$i_country]/value"/></xsl:if>");
socn.addVariable("colours","#007F5C,#FF7F00,#0000FF,#FF0000,#00C0FF,#FFFF00,#7F007F,#7FFFC0,#FF00FF,#FFC47F,#FFFFFF");

//
// Create the chart inside the country_graph element
//
socn.write("country_usage_graph");
</script>
</div>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***           Monthly Summary Graph Template (OFC)            *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;summary&quot;]" mode="graph-flash-ofc">

<!-- graph width -->
<xsl:variable name="width" select="/sswdoc/summary/period/attribute::months * 50"/>

<!-- series identifiers (position of the data node in the XML row) -->
<xsl:variable name="i_hits" select="2"/>
<xsl:variable name="i_files" select="3"/>
<xsl:variable name="i_pages" select="4"/>
<xsl:variable name="i_visits" select="5"/>
<xsl:variable name="i_hosts" select="6"/>
<xsl:variable name="i_xfer" select="7"/>

<!-- series variables -->
<xsl:variable name="s_hits"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_hits"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_files"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_files"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_pages"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_pages"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_visits"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_visits"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_hosts"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_hosts"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_xfer"><xsl:call-template name="series_data_ofc"><xsl:with-param name="series" select="$i_xfer"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>

<div id="monthly_summary_graph" class="graph_holder">
<!-- set div width to match image size -->
<xsl:attribute name="style">width: <xsl:value-of select="$width"/>px</xsl:attribute>

<script type="text/javascript">
//
// Create the chart object
//
var soidx = new SWFObject("<xsl:value-of select="$ofcpath"/>open-flash-chart.swf", "ofc_index", "<xsl:value-of select="$width"/>", "300", "9", "#FFFFFF");

soidx.addParam("allowScriptAccess", "sameDomain" );
soidx.addParam("onmouseout", "onrollout(\"ofc_index\");" );
soidx.addVariable("variables","true");
soidx.addVariable("num_decimals", 0);

//
//   Set up chart colors
//
soidx.addVariable("bg_colour", "#<xsl:value-of select="$graph_bg_color"/>");
soidx.addVariable("x_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");
soidx.addVariable("x_grid_colour", "#<xsl:value-of select="$graph_grid_color"/>");
soidx.addVariable("y_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");
soidx.addVariable("y_grid_colour", "#<xsl:value-of select="$graph_grid_color"/>");
soidx.addVariable("y2_axis_colour", "#<xsl:value-of select="$graph_grid_color"/>");

//
// Set a tool tip to be the name of the series and the value
//
soidx.addVariable("tool_tip", "#key#: #val#");

//
// Chart title (OFC does not support bold style)
//
soidx.addVariable("title","<xsl:value-of select="graph/attribute::title"/>,{font-size: 14px; color: <xsl:value-of select="$graph_title_color"/>; text-align: left; padding-left: 20px;}");

//
// Left Y axis (hits, files, pages)
//
soidx.addVariable("y_label_size", "10",1);
soidx.addVariable("y_ticks", "0,0,4");         // ticks-axis padding, axis offset, ticks
soidx.addVariable("y_max", max_series_value("<xsl:value-of select="$s_hits"/>"));

//
// Right Y axis (visits, hosts)
//
soidx.addVariable("show_y2", true);
soidx.addVariable("y2_lines", "4,5");         // 4 and 5 are series IDs (below)
soidx.addVariable("y2_max", max_series_value("<xsl:value-of select="$s_visits"/>") * 2);

//
// X axis (days of the month)
//
soidx.addVariable("x_labels","<xsl:call-template name="series_labels_ofc"><xsl:with-param name="items" select="graph/months"/></xsl:call-template>");
soidx.addVariable("x_axis_steps","1");

//
// Hits (bar: alpha, fg-color, bg-color, legend, legend-size)
//
soidx.addVariable("filled_bar","100,#<xsl:value-of select="$graph_hits_color"/>,#<xsl:value-of select="$graph_hits_bg_color"/>,<xsl:value-of select="columns/col[$i_hits]/attribute::title"/>,10");
soidx.addVariable("values","<xsl:value-of select="$s_hits"/>");

//
// Files (bar: alpha, fg-color, bg-color, legend, legend-size)
//
soidx.addVariable("filled_bar_2","100,#<xsl:value-of select="$graph_files_color"/>,#<xsl:value-of select="$graph_files_bg_color"/>,<xsl:value-of select="columns/col[$i_files]/attribute::title"/>,10");
soidx.addVariable("values_2","<xsl:value-of select="$s_files"/>");

//
// Pages (bar: alpha, fg-color, bg-color, legend, legend-size)
//
soidx.addVariable("filled_bar_3","100,#<xsl:value-of select="$graph_pages_color"/>,#<xsl:value-of select="$graph_pages_bg_color"/>,<xsl:value-of select="columns/col[$i_pages]/attribute::title"/>,10");
soidx.addVariable("values_3","<xsl:value-of select="$s_pages"/>");

//
// Visits (line: line-width, fg-color, legend, legend-size, point-size)
//
soidx.addVariable("line_hollow_4","3,#<xsl:value-of select="$graph_visits_color"/>,<xsl:value-of select="columns/col[$i_visits]/attribute::title"/>,10,5");
soidx.addVariable("values_4","<xsl:value-of select="$s_visits"/>");

//
// Hosts (line: line-width, fg-color, legend, legend-size, point-size)
//
soidx.addVariable("line_hollow_5","3,#<xsl:value-of select="$graph_hosts_color"/>,<xsl:value-of select="columns/col[$i_hosts]/attribute::title"/>,10,5");
soidx.addVariable("values_5","<xsl:value-of select="$s_hosts"/>");

//
// Create the chart inside the daily_usage_graph element
//
soidx.write("monthly_summary_graph");
</script>

</div>
<!-- output report notes -->
<xsl:call-template name="output_notes"/>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : series_data_ofc
   Context      : /sswdoc/report
   Parameters   :
   
      $items   - data items to iterate through (e.g. days, hours, etc)
      $series  - numeric data item identifier (i.e. data)
      $index   - value index within the data element (e.g. sum, avg, etc)

   Description  :
   
   Generates a comma separated series of data items. The number of values
   in the output is defined by the number of nodes in $items. The template 
   walks the $items node list and selects the row with the rowid attribute 
   matching the current position of the item element.
   
   For example, given this item list,

      <days><day/><day/><day/><day/></days>
   
   , and this node structure
   
      <rows>
         <row rowid="2">
            <data><value>2</value></data>
            <data><avg>10</avg><sum>20</sum></data>
         </row>
         <row rowid="3">
            <data><value>3</value></data>
            <data><avg>30</avg><sum>40</sum></data>
         </row>
      </rows>
   
   , the template will output following text (assuming $index = 2)
   
      null,20,40,null
-->
<xsl:template name="series_data_ofc">
   <xsl:param name="items"/>
   <xsl:param name="series"/>
   <xsl:param name="index" select="1"/>
   <xsl:param name="percent" select="false()"/>

   <xsl:variable name="rows" select="rows"/>
   
   <!-- loop through the list items (e.g. days) -->
   <xsl:for-each select="$items/child::*">
      <!-- remember the item position (e.g. a day) -->
      <xsl:variable name="item" select="position()"/>
      
      <xsl:choose>
         <!-- check if a row with rowid equal to the item position exists -->
         <xsl:when test="$rows/row[attribute::rowid=$item]">
            <!-- use all identifiers to point to the right value node -->
            <xsl:variable name="node" select="$rows/row[attribute::rowid=$item]/data[$series]/child::*[$index]"/>
            
            <!-- choose whether to output the node or the percent value -->
            <xsl:choose>
            <xsl:when test="$percent=true()"><xsl:value-of select="$node/attribute::percent"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="$node"/></xsl:otherwise>
            </xsl:choose>
         </xsl:when>
         
         <!-- if there's no matching row, output null -->
         <xsl:otherwise>null</xsl:otherwise>
      </xsl:choose>
      <!-- output a comma, except in the last position -->
      <xsl:if test="position()!=last()">,</xsl:if>
   </xsl:for-each>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : series_labels_ofc
   Context      : /sswdoc/report
   Parameters   :
   
      $items   - data items to iterate through (e.g. months)
      $offset  - a numeric value to subtract from the position

   Description  : 
   
   Generates a comma-separated list of series labels using ether node
   position or the title. Position is one-based. Use $offset to shift
   generated values.
-->
<xsl:template name="series_labels_ofc">
   <xsl:param name="items"/>
   <xsl:param name="offset" select="0"/>

   <!-- loop through the list items (e.g. days) -->
   <xsl:for-each select="$items/child::*">
      <xsl:choose>
         <!-- output the title, if there's one -->
         <xsl:when test="attribute::title"><xsl:value-of select="attribute::title"/></xsl:when>
         <!-- otherwise just output the node position -->
         <xsl:otherwise><xsl:value-of select="position()+$offset"/></xsl:otherwise>
      </xsl:choose>
      
      <!-- output a comma, except in the last position -->
      <xsl:if test="position()!=last()">,</xsl:if>
   </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
