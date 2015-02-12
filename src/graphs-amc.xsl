<?xml version="1.0" encoding="utf-8" ?>
<!--
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
 -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- 
   This XSL template uses amCharts, which can be downloaded at this
   location:

   http://www.amcharts.com/

   If you use FireFox v3 to browse XML report from disk, you will need
   to relax a security restriction. Type about:config in the URL line, 
   then type "fileuri", without quotes and change this setting to false:

   security.fileuri.strict_origin_policy
   
   If the Flash chart object is loaded, but no graph is shown, you also 
   may need to add the path to the reports to the list of allowed paths 
   in the Flash configuration. In order to do this, right click on the 
   chart and select Settings and then Advanced. The browser will go to 
   the Adobe website. On the left hand side, you will see a link Global 
   Security Settings Panel. Click this link to see security configuration. 
   Click Edit Locations and then Add Location and add the path to your 
   reports directory.

 -->      

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_html_head_amc
   Context      : /sswdoc/report
   Parameters   : none
   Description  : Outputs amCharts-related part of the HTML head element
-->
<xsl:template name="output_html_head_amc">
<script type="text/javascript"><xsl:attribute name="src"><xsl:value-of select="$amcpath"/>swfobject.js</xsl:attribute></script>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***           Daily Totals Graph Template (amCharts)          *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;daily_totals&quot;]" mode="graph-flash-amc">

<!-- series identifiers -->
<xsl:variable name="i_hits" select="2"/>
<xsl:variable name="i_files" select="3"/>
<xsl:variable name="i_pages" select="4"/>
<xsl:variable name="i_visits" select="5"/>
<xsl:variable name="i_hosts" select="6"/>
<xsl:variable name="i_xfer" select="7"/>

<!-- series variables -->
<xsl:variable name="s_hits"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_hits"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_files"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_files"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_pages"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_pages"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_visits"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_visits"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_hosts"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_hosts"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_xfer"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_xfer"/><xsl:with-param name="items" select="graph/days"/></xsl:call-template></xsl:variable>

<div id="daily_usage_graph" class="graph_holder" style="width: 768px;">

<script type="text/javascript"><!--
// 
// Create dynamic settings (colors, titles, etc). Make sure to encode all 
// values so they are compatible with XML and JavaScript, in that order.
//
// For example, all XML data has to be wrapped in CDATA sections. If the
// enclosed XML contains a CDATA section, split the trailing sequence of
// the inner CDATA section as shown below to hide it from the XML 
// parser.
//
//    <![CDATA["<text><![CDATA[<b>...<\/b>]]]]><![CDATA[><\/text>"]]>
//    ^               ^____inner CDATA_____^  ^         ^
//    |_______________________________________|      inner CDATA
//
// The text above will be decoded by the XSL processor as this:
//
//    "<text><![CDATA[<b>...<\/b>]]><\/text>"
//
// Escape double quotes with a back slash or use alternating type of 
// quotes:
//
//   "... name=\"value\" />";
//   "... name='value' />";
//
// Escape left angle bracket followed by a forward slash, even if it's 
// in a CDATA section or inside a JavaScript string. This prevents the
// script tag from being incorrectly ended. Most browsers will handle
// these sequences unencoded, but it's safer to do so, especially if
// output type is changed to XHTML.
// --> 

//
// Create additional chart settings
//
var chart_settings = 
<![CDATA["<settings>"]]> + 
<![CDATA["<start_on_axis>true<\/start_on_axis>"]]>+

//background
<![CDATA["<background>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_bg_color"/><![CDATA[<\/color>"]]> + 
<![CDATA["<\/background>"]]> + 

// grid
<![CDATA["<grid>"]]> +
<![CDATA["<x>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/x>"]]> +
<![CDATA["<y_left>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/y_left>"]]> +
<![CDATA["<\/grid>"]]> +

// lables
<![CDATA["<labels>"]]> + 
<![CDATA["<label lid=\"graph_title\">"]]> +
<![CDATA["<text_color>]]>#<xsl:value-of select="$graph_title_color"/><![CDATA[<\/text_color>"]]> +
<![CDATA["<text><![CDATA[<b>]]><xsl:value-of select="graph/attribute::title"/><![CDATA[<\/b>]]]]><![CDATA[><\/text>"]]> + 
<![CDATA["<\/label>"]]> + 
<![CDATA["<\/labels>"]]> +

// legend
<![CDATA["<legend>"]]> + 
<![CDATA["<values>"]]> + 
<![CDATA["<width>54<\/width>"]]> + 
<![CDATA["<\/values>"]]> + 
<![CDATA["<\/legend>"]]> + 

// graphs
<![CDATA["<graphs>"]]> + 

// hits
<![CDATA["<graph gid=\"1\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_hits]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_hits]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_hits_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// files
<![CDATA["<graph gid=\"2\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_files]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_files]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_files_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// pages
<![CDATA["<graph gid=\"3\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_pages]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_pages]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_pages_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// hosts
<![CDATA["<graph gid=\"4\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_hosts]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_hosts]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_hosts_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// visits
<![CDATA["<graph gid=\"5\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_visits]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_visits]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_visits_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

<![CDATA["<\/graphs>"]]> +
<![CDATA["<\/settings>"]]>;

//
// Create graph data
//
var xml_data = 
<![CDATA["<chart><series>"]]> + 

"<xsl:call-template name="series_labels_amc"><xsl:with-param name="items" select="graph/days"/></xsl:call-template>" + 
   
<![CDATA["<\/series><graphs>"]]> + 

<![CDATA["<graph gid=\"1\">"]]> + "<xsl:value-of select="$s_hits"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"2\">"]]> + "<xsl:value-of select="$s_files"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"3\">"]]> + "<xsl:value-of select="$s_pages"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"4\">"]]> + "<xsl:value-of select="$s_hosts"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"5\">"]]> + "<xsl:value-of select="$s_visits"/>" + <![CDATA["<\/graph>"]]> + 

<![CDATA["<\/graphs><\/chart>"]]>;

//
// Create an SWF object
//
var sody = new SWFObject("<xsl:value-of select="$amcpath"/>amline.swf", "amc_daily", "768", "300", "8", "#<xsl:value-of select="$graph_bg_color"/>");

//
// Point to the amCharts directory and the location of the daily graph configuration file
//
sody.addVariable("path", "<xsl:value-of select="$amcpath"/>");
sody.addVariable("settings_file", "<xsl:value-of select="$config/xslpath"/>graphs-amc-line.xml");

//
// Merge static settings with dynamic values
//
sody.addVariable("additional_chart_settings", encodeURIComponent(chart_settings));

//
// Provide data to plot
//
sody.addVariable("chart_data", encodeURIComponent(xml_data));

//
// Create the chart object (daily_usage_graph)
//
sody.write("daily_usage_graph");
</script>
</div>
<xsl:call-template name="output_notes"/>

</xsl:template>

<!-- ***************************************************************** -->
<!-- ***         Hourly Totals Graph Template (amCharts)           *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;hourly_totals&quot;]" mode="graph-flash-amc">

<!-- series identifiers -->
<xsl:variable name="i_hits" select="2"/>
<xsl:variable name="i_files" select="3"/>
<xsl:variable name="i_pages" select="4"/>
<xsl:variable name="i_xfer" select="5"/>

<!-- series variables -->
<xsl:variable name="s_hits"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_hits"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_files"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_files"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_pages"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_pages"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_xfer"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_xfer"/><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>

<div id="hourly_usage_graph" class="graph_holder" style="width: 768px">

<script type="text/javascript">
//
// See comments in the daily usage graph template
//
var chart_settings = 
<![CDATA["<settings>"]]> + 
<![CDATA["<start_on_axis>true<\/start_on_axis>"]]>+

//background
<![CDATA["<background>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_bg_color"/><![CDATA[<\/color>"]]> + 
<![CDATA["<\/background>"]]> + 

// grid
<![CDATA["<grid>"]]> +
<![CDATA["<x>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/x>"]]> +
<![CDATA["<y_left>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/y_left>"]]> +
<![CDATA["<\/grid>"]]> +

// lables
<![CDATA["<labels>"]]> + 
<![CDATA["<label lid=\"graph_title\">"]]> +
<![CDATA["<text_color>]]>#<xsl:value-of select="$graph_title_color"/><![CDATA[<\/text_color>"]]> +
<![CDATA["<text><![CDATA[<b>]]><xsl:value-of select="graph/attribute::title"/><![CDATA[<\/b>]]]]><![CDATA[><\/text>"]]> + 
<![CDATA["<\/label>"]]> + 
<![CDATA["<\/labels>"]]> +

// legend
<![CDATA["<legend>"]]> + 
<![CDATA["<values>"]]> + 
<![CDATA["<width>54<\/width>"]]> + 
<![CDATA["<\/values>"]]> + 
<![CDATA["<\/legend>"]]> + 

// graphs
<![CDATA["<graphs>"]]> + 

// hits
<![CDATA["<graph gid=\"1\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_hits]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_hits]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_hits_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// files
<![CDATA["<graph gid=\"2\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_files]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_files]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_files_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// pages
<![CDATA["<graph gid=\"3\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_pages]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_pages]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_pages_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// xfer
<![CDATA["<graph gid=\"6\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_xfer]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_xfer]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_xfer_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

<![CDATA["<\/graphs>"]]> +
<![CDATA["<\/settings>"]]>;

//
// Output graph data
//
var xml_data = 
<![CDATA["<chart><series>"]]> + 

"<xsl:call-template name="series_labels_amc"><xsl:with-param name="items" select="graph/hours"/><xsl:with-param name="offset" select="-1"/></xsl:call-template>" + 
   
<![CDATA["<\/series><graphs>"]]> + 

<![CDATA["<graph gid=\"1\">"]]> + "<xsl:value-of select="$s_hits"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"2\">"]]> + "<xsl:value-of select="$s_files"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"3\">"]]> + "<xsl:value-of select="$s_pages"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"6\">"]]> + "<xsl:value-of select="$s_xfer"/>" + <![CDATA["<\/graph>"]]> + 

<![CDATA["<\/graphs><\/chart>"]]>;

//
// Create an SWF object
//
var sohr = new SWFObject("<xsl:value-of select="$amcpath"/>amline.swf", "amc_hourly", "768", "300", "8", "#<xsl:value-of select="$graph_bg_color"/>");

//
// Point to the amCharts directory and the location of the daily graph configuration file
//
sohr.addVariable("path", "<xsl:value-of select="$amcpath"/>");
sohr.addVariable("settings_file", "<xsl:value-of select="$config/xslpath"/>graphs-amc-line.xml");

//
// Merge static settings with dynamic values
//
sohr.addVariable("additional_chart_settings", encodeURIComponent(chart_settings));

//
// Provide data to plot
//
sohr.addVariable("chart_data", encodeURIComponent(xml_data));

//
// Create the chart object (hourly_usage_graph)
//
sohr.write("hourly_usage_graph");
</script>
</div>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***         Top Countries Report Template (amCharts)          *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_countries&quot;]" mode="graph-flash-amc">

<!-- series identifiers -->
<xsl:variable name="i_visits" select="5"/>
<xsl:variable name="i_country" select="6"/>

<!-- series variables -->
<xsl:variable name="s_visits">
   <xsl:call-template name="series_slices_amc">
      <xsl:with-param name="series" select="$i_visits"/>
      <xsl:with-param name="label" select="$i_country"/>
   </xsl:call-template>
</xsl:variable>

<div id="country_usage_graph" class="graph_holder" style="width: 512px;">

<script type="text/javascript">
//
// See comments in the daily usage graph template
//
var chart_settings = 
<![CDATA["<settings>"]]> + 

//background
<![CDATA["<background>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_bg_color"/><![CDATA[<\/color>"]]> + 
<![CDATA["<\/background>"]]> + 

// grid
<![CDATA["<grid>"]]> +
<![CDATA["<x>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/x>"]]> +
<![CDATA["<y_left>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/y_left>"]]> +
<![CDATA["<\/grid>"]]> +

// lables
<![CDATA["<labels>"]]> + 
<![CDATA["<label lid=\"graph_title\">"]]> +
<![CDATA["<text_color>]]>#<xsl:value-of select="$graph_title_color"/><![CDATA[<\/text_color>"]]> +
<![CDATA["<text><![CDATA[<b>]]><xsl:value-of select="graph/attribute::title"/><![CDATA[<\/b>]]]]><![CDATA[><\/text>"]]> + 
<![CDATA["<\/label>"]]> + 
<![CDATA["<\/labels>"]]> +

// slice colors
<![CDATA["<pie>"]]> + 
<![CDATA["<colors>#007F5C,#FF7F00,#0000FF,#FF0000,#00C0FF,#FFFF00,#7F007F,#7FFFC0,#FF00FF,#FFC47F,#FFFFFF</colors>"]]> +
<![CDATA["<\/pie>"]]> + 

<![CDATA["<\/settings>"]]>;

//
// Output graph data
//
var xml_data = 
<![CDATA["<pie>"]]> + 
"<xsl:value-of select="$s_visits"/>" + 
<![CDATA["<\/pie>"]]>;

//
// Create an SWF object
//
var sohr = new SWFObject("<xsl:value-of select="$amcpath"/>ampie.swf", "amc_country", "512", "300", "8", "#<xsl:value-of select="$graph_bg_color"/>");

//
// Point to the amCharts directory and the location of the daily graph configuration file
//
sohr.addVariable("path", "<xsl:value-of select="$amcpath"/>");
sohr.addVariable("settings_file", "<xsl:value-of select="$config/xslpath"/>graphs-amc-pie.xml");

//
// Merge static settings with dynamic values
//
sohr.addVariable("additional_chart_settings", encodeURIComponent(chart_settings));

//
// Provide data to plot
//
sohr.addVariable("chart_data", encodeURIComponent(xml_data));

//
// Create the chart object (hourly_usage_graph)
//
sohr.write("country_usage_graph");
</script>
</div>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***        Monthly Summary Graph Template (amCharts)          *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;summary&quot;]" mode="graph-flash-amc">

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
<xsl:variable name="s_hits"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_hits"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_files"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_files"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_pages"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_pages"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_visits"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_visits"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_hosts"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_hosts"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>
<xsl:variable name="s_xfer"><xsl:call-template name="series_data_amc"><xsl:with-param name="series" select="$i_xfer"/><xsl:with-param name="items" select="graph/months"/><xsl:with-param name="index" select="2"/></xsl:call-template></xsl:variable>

<div id="monthly_summary_graph" class="graph_holder">
<!-- set div width to match image size -->
<xsl:attribute name="style">width: <xsl:value-of select="$width"/>px</xsl:attribute>

<script type="text/javascript">
var chart_settings = 
<![CDATA["<settings>"]]> + 
<![CDATA["<start_on_axis>false<\/start_on_axis>"]]>+

//background
<![CDATA["<background>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_bg_color"/><![CDATA[<\/color>"]]> + 
<![CDATA["<\/background>"]]> + 

// grid
<![CDATA["<grid>"]]> +
<![CDATA["<x>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/x>"]]> +
<![CDATA["<y_left>"]]> +
<![CDATA["<color>]]>#<xsl:value-of select="$graph_grid_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/y_left>"]]> +
<![CDATA["<\/grid>"]]> +

// lables
<![CDATA["<labels>"]]> + 
<![CDATA["<label lid=\"graph_title\">"]]> +
<![CDATA["<text_color>]]>#<xsl:value-of select="$graph_title_color"/><![CDATA[<\/text_color>"]]> +
<![CDATA["<text><![CDATA[<b>]]><xsl:value-of select="graph/attribute::title"/><![CDATA[<\/b>]]]]><![CDATA[><\/text>"]]> + 
<![CDATA["<\/label>"]]> + 
<![CDATA["<\/labels>"]]> +

// legend
<![CDATA["<legend>"]]> + 
<![CDATA["<values>"]]> + 
<![CDATA["<width>0<\/width>"]]> + 
<![CDATA["<text><![CDATA[ ]]"+"></text>"]]>+
<![CDATA["<\/values>"]]> + 
<![CDATA["<\/legend>"]]> + 

// graphs
<![CDATA["<graphs>"]]> + 

// hits
<![CDATA["<graph gid=\"7\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_hits]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_hits]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_hits_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// files
<![CDATA["<graph gid=\"8\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_files]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_files]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_files_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// pages
<![CDATA["<graph gid=\"9\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_pages]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_pages]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_pages_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// hosts
<![CDATA["<graph gid=\"4\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_hosts]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_hosts]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_hosts_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

// visits
<![CDATA["<graph gid=\"5\">"]]> +
<![CDATA["<title>]]><xsl:value-of select="columns/col[$i_visits]/attribute::title"/><![CDATA[<\/title>"]]> +
<![CDATA["<balloon_text><![CDATA[]]><xsl:value-of select="columns/col[$i_visits]/attribute::title"/>: {value}<![CDATA[]]]]><![CDATA[></balloon_text>"]]> + 
<![CDATA["<color>]]>#<xsl:value-of select="$graph_visits_color"/><![CDATA[<\/color>"]]> +
<![CDATA["<\/graph>"]]> + 

<![CDATA["<\/graphs>"]]> +
<![CDATA["<\/settings>"]]>;

//
// Output graph data
//
var xml_data = 
<![CDATA["<chart><series>"]]> + 

"<xsl:call-template name="series_labels_amc"><xsl:with-param name="items" select="graph/months"/></xsl:call-template>" + 
   
<![CDATA["<\/series><graphs>"]]> + 

<![CDATA["<graph gid=\"7\">"]]> + "<xsl:value-of select="$s_hits"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"8\">"]]> + "<xsl:value-of select="$s_files"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"9\">"]]> + "<xsl:value-of select="$s_pages"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"4\">"]]> + "<xsl:value-of select="$s_hosts"/>" + <![CDATA["<\/graph>"]]> + 
<![CDATA["<graph gid=\"5\">"]]> + "<xsl:value-of select="$s_visits"/>" + <![CDATA["<\/graph>"]]> + 

<![CDATA["<\/graphs><\/chart>"]]>;

//
// Create an SWF object
//
var soidx = new SWFObject("<xsl:value-of select="$amcpath"/>amline.swf", "amc_index", "<xsl:value-of select="$width"/>", "300", "8", "#<xsl:value-of select="$graph_bg_color"/>");

//
// Point to the amCharts directory and the location of the daily graph configuration file
//
soidx.addVariable("path", "<xsl:value-of select="$amcpath"/>");
soidx.addVariable("settings_file", "<xsl:value-of select="$config/xslpath"/>graphs-amc-line.xml");

//
// Merge static settings with dynamic values
//
soidx.addVariable("additional_chart_settings", encodeURIComponent(chart_settings));

//
// Provide data to plot
//
soidx.addVariable("chart_data", encodeURIComponent(xml_data));

//
// Create the chart object (daily_usage_graph)
//
soidx.write("monthly_summary_graph");
</script>

</div>
<!-- output report notes -->
<xsl:call-template name="output_notes"/>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : series_data_amc
   Context      : /sswdoc/report
   Parameters   :
   
      $items   - data items to iterate through (e.g. days, hours, etc)
      $series  - numeric data item identifier (i.e. data)
      $index   - value index within the data element (e.g. sum, avg, etc)
      $percent - a boolean indicating whether to output the value of the 
                 percent attribute (true) or the node (false)

   Description  :
   
   Generates a value element for each data item in $items. The template 
   walks the node list and selects only rows with the rowid attribute 
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
   
   , the template will output following element list (assuming $index = 2)
   
      <value xid="2">20</value><value xid="3">40</value>
      
   amCharts can handle missing data items (uses xid attributes to match 
   existing values and labels), so the template below generates only  
   non-empty values. If you are using some other charting package and it 
   requires all values, including empty ones, move the lines that generate 
   start and end value tags out of the xsl:if block.
   
-->
<xsl:template name="series_data_amc">
   <xsl:param name="items"/>
   <xsl:param name="series"/>
   <xsl:param name="index" select="1"/>
   <xsl:param name="percent" select="false()"/>

   <xsl:variable name="rows" select="rows"/>
   
   <!-- loop through the list items (e.g. days) -->
   <xsl:for-each select="$items/child::*">
   
      <!-- remember the item position (e.g. a day) -->
      <xsl:variable name="item" select="position()"/>
      
      <!-- locate the row with rowid equal to the item position  -->
      <xsl:variable name="row" select="$rows/row[attribute::rowid=$item]"/>

      <!-- check if the row exists -->
      <xsl:if test="$row">
         <!-- output the start value tag -->
         <xsl:text><![CDATA[<value xid=\"]]></xsl:text><xsl:value-of select="$item"/><xsl:text><![CDATA[\">]]></xsl:text>

         <!-- use series and index identifiers to point to the right value node -->
         <xsl:variable name="node" select="$row/data[$series]/child::*[$index]"/>
         
         <!-- choose whether to output the node or the percent value -->
         <xsl:choose>
         <xsl:when test="$percent=true()"><xsl:value-of select="$node/attribute::percent"/></xsl:when>
         <xsl:otherwise><xsl:value-of select="$node"/></xsl:otherwise>
         </xsl:choose>

         <!-- output the end tag -->
         <xsl:text><![CDATA[<\/value>]]></xsl:text>         
      </xsl:if>

   </xsl:for-each>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : series_labels_amc
   Context      : /sswdoc/report
   Parameters   :
   
      $items   - data items to iterate through (e.g. months)
      $offset  - a numeric value to subtract from the position

   Description  : 
   
   Generates a label for each element in $items. The label is ether 
   item position or the value of the title attribute, if it exists. 
   Position is one-based. Use $offset to shift generated values.
-->
<xsl:template name="series_labels_amc">
   <xsl:param name="items"/>
   <xsl:param name="offset" select="0"/>

   <!-- loop through the list items (e.g. days) -->
   <xsl:for-each select="$items/child::*">
      <!-- <value xid=\"item-id\"> -->
      <xsl:text><![CDATA[<value xid=\"]]></xsl:text><xsl:value-of select="position()"/><xsl:text><![CDATA[\">]]></xsl:text>

      <xsl:choose>
         <!-- output the title, if there's one -->
         <xsl:when test="attribute::title"><xsl:value-of select="attribute::title"/></xsl:when>
         <!-- otherwise just output the node position -->
         <xsl:otherwise><xsl:value-of select="position()+$offset"/></xsl:otherwise>
      </xsl:choose>
      
      <xsl:text><![CDATA[<\/value>]]></xsl:text>
      
   </xsl:for-each>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : series_slices_amc
   Context      : /sswdoc/report
   Parameters   :
   
      $series  - data item identifier (i.e. visits)
      $label   - data label item identifier (i.e. country)

   Description  : 
   
   Generates slice elements for the country chart. amCharts does not
   support maximum number of slices, so use graph/other from the XML
   report.
-->
<xsl:template name="series_slices_amc">
   <xsl:param name="series"/>
   <xsl:param name="label"/>

   <xsl:variable name="rows" select="rows"/>
   <xsl:variable name="other" select="graph/other"/>

   <!-- loop through the list of slices -->
   <xsl:for-each select="graph/slices/child::*">
      <!-- remember slice position -->
      <xsl:variable name="rowid" select="position()"/>
      
      <!-- locate the row that matches the slice position -->
      <xsl:variable name="row" select="$rows/row[attribute::rowid=$rowid]"/>
      
      <!-- <slice title=\"label\">value</slice> -->
      <xsl:text><![CDATA[<slice title=\"]]></xsl:text><xsl:value-of select="$row/data[$label]/value"/><xsl:text><![CDATA[\">]]></xsl:text>
      <xsl:value-of select="$row/data[$series]/sum"/>
      <xsl:text><![CDATA[<\/slice>]]></xsl:text>
      
   </xsl:for-each>

   <!-- output a slice for the remaining countries, if there's one -->
   <xsl:if test="$other">
      <xsl:text><![CDATA[<slice title=\"]]></xsl:text><xsl:value-of select="$other/data[$label]/value"/><xsl:text><![CDATA[\">]]></xsl:text>
      <xsl:value-of select="$other/data[$series]/sum"/>
      <xsl:text><![CDATA[<\/slice>]]></xsl:text>
   </xsl:if>
</xsl:template>

</xsl:stylesheet>
