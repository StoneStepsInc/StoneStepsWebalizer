<?xml version="1.0" encoding="utf-8" ?>
<!--
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
 -->
<!DOCTYPE xsl:stylesheet [
<!ENTITY copy "&#169;"> <!ENTITY nbsp "&#160;"> <!ENTITY bull "&#8226;">
]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- import named templates and common configuration settings -->
<xsl:import href="webalizer.xsl"/>

<!-- configure output as HTML 4.01 strict -->
<xsl:output method="html" indent="no" encoding="utf-8"/>

<!-- include Flash graph templates -->
<xsl:include href="graphs-ofc.xsl"/>
<xsl:include href="graphs-amc.xsl"/>

<!-- ***************************************************************** -->
<!-- ***                   Main Report Template                    *** -->
<!-- ***************************************************************** -->
<xsl:template match="/sswdoc">

<!-- create report variables, so it's easier to refer to individual reports -->
<xsl:variable name="report_totals" select="report[attribute::id=&quot;monthly_totals&quot;]"/>
<xsl:variable name="report_daily_totals" select="report[attribute::id=&quot;daily_totals&quot;]"/>
<xsl:variable name="report_hourly_totals" select="report[attribute::id=&quot;hourly_totals&quot;]"/>
<xsl:variable name="report_top_url_hits" select="report[attribute::id=&quot;top_url_hits&quot;]"/>
<xsl:variable name="report_top_url_xfer" select="report[attribute::id=&quot;top_url_xfer&quot;]"/>
<xsl:variable name="report_top_entry_urls" select="report[attribute::id=&quot;top_entry_urls&quot;]"/>
<xsl:variable name="report_top_exit_urls" select="report[attribute::id=&quot;top_exit_urls&quot;]"/>
<xsl:variable name="report_top_downloads" select="report[attribute::id=&quot;top_downloads&quot;]"/>
<xsl:variable name="report_top_errors" select="report[attribute::id=&quot;top_errors&quot;]"/>
<xsl:variable name="report_top_host_hits" select="report[attribute::id=&quot;top_host_hits&quot;]"/>
<xsl:variable name="report_top_host_xfer" select="report[attribute::id=&quot;top_host_xfer&quot;]"/>
<xsl:variable name="report_top_referrers" select="report[attribute::id=&quot;top_referrers&quot;]"/>
<xsl:variable name="report_top_search_strings" select="report[attribute::id=&quot;top_search_strings&quot;]"/>
<xsl:variable name="report_top_users" select="report[attribute::id=&quot;top_users&quot;]"/>
<xsl:variable name="report_top_user_agents" select="report[attribute::id=&quot;top_user_agents&quot;]"/>
<xsl:variable name="report_top_countries" select="report[attribute::id=&quot;top_countries&quot;]"/>

<!-- output the copyright comment -->
<xsl:call-template name="output_copyright_comment"/>

<html><xsl:attribute name="lang"><xsl:value-of select="attribute::lang"/></xsl:attribute>
<head>
<xsl:call-template name="output_html_head"/>

<script type="text/javascript">
<![CDATA[
var ie6 = false;
var ie7 = false;
var ie8 = false;
var ie9 = false;

var allitems = null;          // instance of ViewAllInfo, if there is an expanded report

function ViewAllInfo()
{
   this.rptlink = null;       // a link in front of each report
   this.viewall = null;       // view all link (tbody)
   this.allitems = null;      // all items (tbody)
   this.titlenode = null;     // title table cell (th)
   this.title = null;         // original report title (String)
}

function onload_usage_page_xml()
{
   initAllItemsLinks();

   registerOnKeyUpHandler(onKeyUpAllItemsHandler);
}

function onKeyUpAllItemsHandler(event)
{
   if(event.keyCode == KEY_ESC)
      hideAllItems(true);
}

function initAllItemsLinks()
{
   // check for versions of IE
   ie6 = /MSIE[ \t]+6/.test(window.navigator.userAgent);
   ie7 = /MSIE[ \t]+7/.test(window.navigator.userAgent);
   ie8 = /MSIE[ \t]+8/.test(window.navigator.userAgent);
   ie9 = /MSIE[ \t]+9/.test(window.navigator.userAgent);
}

//
// hideAllItems
//
// Collapse an expanded all-items section, if there's any
//
function hideAllItems(gotorep)
{
   if(allitems) {
      // make the View All link visible
      allitems.viewall.style.display = (ie6 || ie7 || ie8 || ie9) ? "block" : "table-row-group";
      
      // hide the remaining items
      allitems.allitems.style.display = "none";

      // and restore the report title
      allitems.titlenode.firstChild.data = allitems.title;
      
      // if requested, go to the top of the report
      if(gotorep)
         document.location.href = "#" + allitems.rptlink.name;
         
      // all done, reset the descriptor
      allitems = null;
   }
}

//
// showAllItems
//
// Shows the hidden table section with the remainder of report items,
// fixes up the title and hides the View All table section.
//
function showAllItems(node, top, count)
{
   if(!node)
      return false;

   // hide the expanded section, if there's any
   hideAllItems(false);
   
   // create a descriptor to hold table sections
   allitems = new ViewAllInfo();
   
   //
   // Hide the table section containing the View All link
   //
   node = findParentNode(node, "tbody")

   // store the link node
   allitems.viewall = node;
   
   // and hide the section
   node.style.display = "none";
   
   //
   // Expand the section containing remaining report items
   //
   node = findNextSibling(node, "tbody");

   // store it, so we can use it later to collapse the section
   allitems.allitems = node;

   // and make the section visible (IE6/7 do not understand table-row-group)
   node.style.display = (ie6 || ie7 || ie8 || ie9) ? "block" : "table-row-group";

   //
   // Fix the title to reflect the number of displayed items
   //
   allitems.titlenode = findReportTitle(node);
   
   // keep the title, so we can restore it later
   allitems.title = allitems.titlenode.firstChild.data;
   
   // replace the top number with the new value
   allitems.titlenode.firstChild.data = allitems.title.replace(new RegExp(" " + top + " "), " " + count + " ");

   //
   // We may have collapsed a report that is located before the one we are
   // expanding, which would cause the browser to show some arbitrary part
   // of the markup. In order to avoid this, let's tell the browser to go 
   // to the top of the current report. 
   //
   
   // find the anchor element before the report table
   allitems.rptlink = findPrevSibling(findParentNode(node, "table"), "a");

   // and change the document location
   document.location.href = "#" + allitems.rptlink.name;
   
   return true;
}

//
// findReportTitle
//
// Given any node inside a report table, returns the report title cell.
// The text data within the cell is normalized before the call returns,
// so it is possible to use firstChild against the returned value to
// access title text.
//
function findReportTitle(node)
{
   // find the parent table
   node = findParentNode(node, "table");
   
   // find the head section
   node = node.firstChild;

   while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != "thead")
      node = node.nextSibling;

   // find the title row
   node = node.firstChild;

   while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != "tr" || node.className.indexOf("table_title_tr") == -1)
      node = node.nextSibling;
   
   // find the title cell
   node = node.firstChild;

   while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != "th")
      node = node.nextSibling;
   
   node.normalize();
   
   return node;
}   

]]>
</script>

<!-- graph-related HTML markup -->
<xsl:choose>
<xsl:when test="$graphtype=&quot;graph-flash-ofc&quot;">
<xsl:call-template name="output_html_head_ofc"/>
</xsl:when>
<xsl:when test="$graphtype=&quot;graph-flash-amc&quot;">
<xsl:call-template name="output_html_head_amc"/>
</xsl:when>
</xsl:choose>

</head>

<body onload="onload_usage_page(onload_usage_page_xml)">

<a name="top"></a>

<!-- page header -->
<xsl:call-template name="output_page_header"/>

<!-- main menu links -->
<table id="main_menu" class="page_links_table"><tr>
<xsl:if test="$report_daily_totals">
<td><a href="#daily"><xsl:value-of select="key(&quot;text&quot;, &quot;link_daily_stats&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_hourly_totals">
<td><a href="#hourly"><xsl:value-of select="key(&quot;text&quot;, &quot;link_hourly_stats&quot;)"/></a></td>
</xsl:if>

<xsl:choose>
<!-- if there's a top URLs report, always link to it -->
<xsl:when test="$report_top_url_hits">
<td><a href="#urls"><xsl:value-of select="key(&quot;text&quot;, &quot;link_urls&quot;)"/></a>
<!-- if there's also a URL transfer report, create a hidden link -->
<xsl:if test="$report_top_url_xfer"><a href="#urlxfer" style="display: none"/></xsl:if>
</td>
</xsl:when>
<xsl:when test="$report_top_url_xfer">
<!-- if there's just a URL transfer report, link to it -->
<td><a href="#urlxfer"><xsl:value-of select="key(&quot;text&quot;, &quot;link_urls&quot;)"/></a></td>
</xsl:when>
</xsl:choose>

<xsl:if test="$report_top_entry_urls">
<td><a href="#entry"><xsl:value-of select="key(&quot;text&quot;, &quot;link_entry&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_top_exit_urls">
<td><a href="#exit"><xsl:value-of select="key(&quot;text&quot;, &quot;link_exit&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_top_downloads">
<td><a href="#downloads"><xsl:value-of select="key(&quot;text&quot;, &quot;link_downloads&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_top_errors">
<td><a href="#errors"><xsl:value-of select="key(&quot;text&quot;, &quot;link_errors&quot;)"/></a></td>
</xsl:if>

<xsl:choose>
<!-- if there's a top hosts report, always link to it -->
<xsl:when test="$report_top_host_hits">
<td><a href="#hosts"><xsl:value-of select="key(&quot;text&quot;, &quot;link_hosts&quot;)"/></a></td>
<!-- if there's also a hosts transfer report, create a hidden link -->
<xsl:if test="$report_top_host_xfer"><a href="#hostxfer" style="display: none"></a></xsl:if>
</xsl:when>
<!-- if there's just a hosts transfer report, link to it -->
<xsl:when test="$report_top_host_xfer">
<td><a href="#hosts"><xsl:value-of select="key(&quot;text&quot;, &quot;link_hosts&quot;)"/></a></td>
</xsl:when>
</xsl:choose>

<xsl:if test="$report_top_referrers">
<td><a href="#referrers"><xsl:value-of select="key(&quot;text&quot;, &quot;link_referrers&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_top_search_strings">
<td><a href="#search"><xsl:value-of select="key(&quot;text&quot;, &quot;link_search&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_top_users">
<td><a href="#users"><xsl:value-of select="key(&quot;text&quot;, &quot;link_users&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_top_user_agents">
<td><a href="#useragents"><xsl:value-of select="key(&quot;text&quot;, &quot;link_agents&quot;)"/></a></td>
</xsl:if><xsl:if test="$report_top_countries">
<td><a href="#countries"><xsl:value-of select="key(&quot;text&quot;, &quot;link_countries&quot;)"/></a></td>
</xsl:if>
</tr></table>

<!-- Monthly Totals -->
<xsl:if test="$report_totals">
<xsl:comment> Monthly Totals </xsl:comment>
<div id="monthly_totals_report">
<a name="totals"></a>
<xsl:apply-templates select="$report_totals"/>
</div>
</xsl:if>

<!-- Daily Totals -->
<xsl:if test="$report_daily_totals">
<xsl:comment> Daily Totals </xsl:comment>
<div id="daily_stats_report">
<a name="daily"></a>
<xsl:call-template name="output_graph"><xsl:with-param name="report" select="$report_daily_totals"/></xsl:call-template>
<xsl:apply-templates select="$report_daily_totals"/>
</div>
</xsl:if>

<!-- Hourly Totals -->
<xsl:if test="$report_hourly_totals">
<xsl:comment> Hourly Totals </xsl:comment>
<div id="hourly_stats_report">
<a name="hourly"></a>
<xsl:call-template name="output_graph"><xsl:with-param name="report" select="$report_hourly_totals"/></xsl:call-template>
<xsl:apply-templates select="$report_hourly_totals"/>
</div>
</xsl:if>

<!-- Top URLs Reports -->
<xsl:if test="$report_top_url_hits or $report_top_url_xfer">
<xsl:comment> Top URLs Reports </xsl:comment>
<div id="top_url_report">
<xsl:if test="$report_top_url_hits">
<a name="urls"></a>
<xsl:apply-templates select="$report_top_url_hits"/>
</xsl:if>
<xsl:if test="$report_top_url_xfer">
<a name="urlxfer"></a>
<xsl:apply-templates select="$report_top_url_xfer"/>
</xsl:if>
</div>
</xsl:if>

<!-- Top Entry URLs Reports -->
<xsl:if test="$report_top_entry_urls">
<xsl:comment> Top Entry URLs Reports </xsl:comment>
<div id="top_entry_url_report">
<a name="entry"></a>
<xsl:apply-templates select="$report_top_entry_urls"/>
</div>
</xsl:if>

<!-- Top Exit URLs Reports -->
<xsl:if test="$report_top_exit_urls">
<xsl:comment> Top Exit URLs Reports </xsl:comment>
<div id="top_exit_url_report">
<a name="exit"></a>
<xsl:apply-templates select="$report_top_exit_urls"/>
</div>
</xsl:if>

<!-- Top Downloads Report -->
<xsl:if test="$report_top_downloads">
<xsl:comment> Top Downloads Report </xsl:comment>
<div id="top_downloads_report">
<a name="downloads"></a>
<xsl:apply-templates select="$report_top_downloads"/>
</div>
</xsl:if>

<!-- Top Errors Report -->
<xsl:if test="$report_top_errors">
<xsl:comment> Top Errors Report </xsl:comment>
<div id="top_errors_report">
<a name="errors"></a>
<xsl:apply-templates select="$report_top_errors"/>
</div>
</xsl:if>

<!-- Top Hosts Reports -->
<xsl:if test="$report_top_host_hits or $report_top_host_xfer">
<xsl:comment> Top Hosts Reports </xsl:comment>
<div id="top_host_report">
<xsl:if test="$report_top_host_hits">
<a name="hosts"></a>
<xsl:apply-templates select="$report_top_host_hits"/>
</xsl:if>
<xsl:if test="$report_top_host_xfer">
<a name="hostxfer"></a>
<xsl:apply-templates select="$report_top_host_xfer"/>
</xsl:if>
</div>
</xsl:if>

<!-- Top Referrers Reports -->
<xsl:if test="$report_top_referrers">
<xsl:comment> Top Referrers Reports </xsl:comment>
<div id="top_referrers_report">
<a name="referrers"></a>
<xsl:apply-templates select="$report_top_referrers"/>
</div>
</xsl:if>

<!-- Top Search Strings Report -->
<xsl:if test="$report_top_search_strings">
<xsl:comment> Top Search Strings Report </xsl:comment>
<div id="top_search_report">
<a name="search"></a>
<xsl:apply-templates select="$report_top_search_strings"/>
</div>
</xsl:if>

<!-- Top Users Report -->
<xsl:if test="$report_top_users">
<xsl:comment> Top Users Report </xsl:comment>
<div id="top_users_report">
<a name="users"></a>
<xsl:apply-templates select="$report_top_users"/>
</div>
</xsl:if>

<!-- Top User Agents Report -->
<xsl:if test="$report_top_user_agents">
<xsl:comment> Top User Agents Report </xsl:comment>
<div id="top_user_agents_report">
<a name="useragents"></a>
<xsl:apply-templates select="report[attribute::id=&quot;top_user_agents&quot;]"/>
</div>
</xsl:if>

<!-- Top Countries Report -->
<xsl:if test="$report_top_countries">
<xsl:comment> Top Countries Report </xsl:comment>
<div id="top_countries_report">
<a name="countries"></a>
<xsl:call-template name="output_graph"><xsl:with-param name="report" select="$report_top_countries"/></xsl:call-template>
<xsl:apply-templates select="$report_top_countries"/>
</div>
</xsl:if>

<!-- page footer -->
<xsl:call-template name="output_page_footer"/>

<!-- hidden help box -->
<xsl:call-template name="output_help_box"/>

<!-- hidden help entries -->
<xsl:call-template name="output_help_text"/>
</body>
</html>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***                Monthly Totals Report Template             *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;monthly_totals&quot;]">
<table class="report_table monthly_totals_table">
   <colgroup><col/><col span="2" class="totals_data_col"/></colgroup>
   <!-- report title -->
   <thead>
   <tr class="table_title_tr"><th colspan="3"><xsl:value-of select="attribute::title"/></th></tr>
   </thead>

   <!-- loop through sections -->
   <xsl:for-each select="section">
      <xsl:variable name="colcount" select="count(columns/col)"/>
      
      <!-- section header -->
      <tbody class="totals_header_tbody">
      <tr>
      <xsl:for-each select="columns/col">
         <xsl:choose>
            <!-- output the first column as a <th> cell -->
            <xsl:when test="position() = 1">
               <th>
               <!-- wrap title in a help span -->
               <xsl:call-template name="output_title_help">
                  <xsl:with-param name="title" select="attribute::title"/>
                  <xsl:with-param name="topic" select="attribute::help"/>
               </xsl:call-template>
               </th>
            </xsl:when>
            
            <!-- output remaining columns -->
            <xsl:otherwise>
               <td>
               <!-- if there are two columns, span the last one two cells -->
               <xsl:if test="$colcount=2">
                  <xsl:attribute name="colspan"><xsl:value-of select="2"/></xsl:attribute>
               </xsl:if>
               
               <!-- output optional column title -->
               <xsl:value-of select="attribute::title"/>
               </td>
            </xsl:otherwise>
         </xsl:choose>
      </xsl:for-each>
      </tr>
      </tbody>
      
      <!-- data rows -->
      <tbody class="totals_data_tbody">
      <xsl:for-each select="data">
      <tr>
      
      <!-- output data title -->
      <th>
         <!-- wrap title in a help span -->
         <xsl:call-template name="output_title_help">
            <xsl:with-param name="title" select="attribute::title"/>
            <xsl:with-param name="topic" select="attribute::help"/>
         </xsl:call-template>
      </th>
      
      <!-- loop through data values -->
      <xsl:for-each select="child::*">
         <td>
         <!-- if there are two data columns, span data column two cells -->
         <xsl:if test="$colcount=2">
            <xsl:attribute name="colspan"><xsl:value-of select="2"/></xsl:attribute>
         </xsl:if>
         
         <!-- output data value -->
         <xsl:value-of select="child::text()"/>
         </td>
      </xsl:for-each>
      </tr>
      </xsl:for-each>
      </tbody>
      
   </xsl:for-each>
</table>

<!-- output report notes -->
<xsl:call-template name="output_notes"/>

</xsl:template>

<!-- ***************************************************************** -->
<!-- ***              Daily Totals Graph Template (PNG)            *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;daily_totals&quot;]" mode="graph-png">
<div id="daily_usage_graph" class="graph_holder">
<xsl:call-template name="output_image"/>
</div>
<xsl:call-template name="output_notes"/>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***               Daily Totals Report Template                *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;daily_totals&quot;]">
<table class="report_table totals_table">
<thead>
<tr class="table_title_tr"><th colspan="25"><xsl:value-of select="attribute::title"/></th></tr>

<!-- two-row header -->
<xsl:call-template name="output_two_row_header"/>
</thead>

<!-- data rows -->
<tbody class="totals_data_tbody">
<xsl:for-each select="rows/row">
   <xsl:variable name="weekday" select="data[position()=1]/value/attribute::weekday"/>
   
   <tr>
   <!-- highlight weekends -->
   <xsl:if test="$weekday=6 or $weekday=0">
      <xsl:attribute name="class">weekend</xsl:attribute>
   </xsl:if>
   
   <xsl:call-template name="output_totals_row"/>
   </tr>
</xsl:for-each>
</tbody>

</table>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***            Hourly Totals Graph Template (PNG)             *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;hourly_totals&quot;]" mode="graph-png">
<div id="daily_usage_graph" class="graph_holder">
<xsl:call-template name="output_image"/>
</div>
<xsl:call-template name="output_notes"/>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***               Hourly Totals Report Template                *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;hourly_totals&quot;]">

<table class="report_table totals_table">

<thead>
<tr class="table_title_tr"><th colspan="13"><xsl:value-of select="attribute::title"/></th></tr>

<!-- two-row header -->
<xsl:call-template name="output_two_row_header"/>
</thead>

<tbody class="totals_data_tbody">

<xsl:for-each select="rows/row">
   <tr>
   <xsl:call-template name="output_totals_row"/>
   </tr>
</xsl:for-each>

</tbody>
</table>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***          Top URL Hits/Trasfer Report Templates            *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_url_hits&quot;]">
<xsl:call-template name="top_url_report"/>
</xsl:template>

<xsl:template match="report[attribute::id=&quot;top_url_xfer&quot;]">
<xsl:call-template name="top_url_report"/>
</xsl:template>

<xsl:template name="top_url_report">
   <xsl:variable name="report" select="self::node()"/>
   <xsl:variable name="rows" select="rows"/>
   <xsl:variable name="i_url" select="5"/>

<table class="report_table stats_table">

<thead>
<tr class="table_title_tr"><th colspan="8"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_two_row_header"/>
</thead>

<!-- output the top URL section -->
<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>
   
   <xsl:call-template name="output_url_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_url" select="$i_url"/>
   </xsl:call-template>
</xsl:for-each>
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">
   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td colspan="8">
         <a>
         <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
         <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_urls&quot;)"/>
         </a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
         <xsl:call-template name="output_url_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_url" select="$i_url"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr"><th colspan="8"><xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/></th></tr>
   </xsl:if>
   </tbody>
</xsl:if>   

</table>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***              Entry/Exit URL Report Templates              *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_entry_urls&quot;]">
<xsl:call-template name="top_entry_exit_url_report"/>
</xsl:template>

<xsl:template match="report[attribute::id=&quot;top_exit_urls&quot;]">
<xsl:call-template name="top_entry_exit_url_report"/>
</xsl:template>

<xsl:template name="top_entry_exit_url_report">
   <xsl:variable name="report" select="self::node()"/>
   <xsl:variable name="i_url" select="4"/>

<table class="report_table stats_table">

<thead>
<tr class="table_title_tr"><th colspan="6"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_one_row_header"/>
</thead>

<tbody class="stats_data_tbody">
<xsl:for-each select="rows/row">
   <xsl:call-template name="output_url_row">
      <xsl:with-param name="row" select="self::node()"/>
      <xsl:with-param name="i_url" select="$i_url"/>
   </xsl:call-template>
</xsl:for-each>
</tbody>

</table>

<xsl:call-template name="output_notes"/>

</xsl:template>

<!-- ***************************************************************** -->
<!-- ***               Top Downloads Report Template               *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_downloads&quot;]">

<xsl:variable name="report" select="self::node()"/>
<xsl:variable name="rows" select="rows"/>

<xsl:variable name="i_dlname" select="6"/>
<xsl:variable name="i_country" select="7"/>
<xsl:variable name="i_host" select="8"/>

<table class="report_table stats_table">
<thead>
<tr class="table_title_tr">
   <th>
      <xsl:attribute name="colspan"><xsl:value-of select="attribute::colspan"/></xsl:attribute>
      <xsl:value-of select="attribute::title"/>
   </th>
</tr>
<xsl:call-template name="output_two_row_header"/>
</thead>
<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>
   
   <xsl:call-template name="output_download_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_dlname" select="$i_dlname"/>
      <xsl:with-param name="i_country" select="$i_country"/>
      <xsl:with-param name="i_host" select="$i_host"/>
   </xsl:call-template>
   
</xsl:for-each>
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">
   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td><xsl:attribute name="colspan"><xsl:value-of select="attribute::colspan"/></xsl:attribute>
            <a>
            <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
            <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_downloads&quot;)"/></a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
         <xsl:call-template name="output_download_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_dlname" select="$i_dlname"/>
            <xsl:with-param name="i_country" select="$i_country"/>
            <xsl:with-param name="i_host" select="$i_host"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr">
		<th><xsl:attribute name="colspan"><xsl:value-of select="attribute::colspan"/></xsl:attribute>
			<xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/>
		</th>
	</tr>
   </xsl:if>
   </tbody>
</xsl:if>   
         
</table>
</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_download_row
   Description  : Generates one row of the download report
   ________________________________________________________________________
-->
<xsl:template name="output_download_row">
   <xsl:param name="row"/>
   <xsl:param name="i_dlname"/>
   <xsl:param name="i_country"/>
   <xsl:param name="i_host"/>

   <tr>

   <!-- loop through the remaining data elements -->
   <xsl:for-each select="$row/data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>
      
      <xsl:choose>
      
      <!-- output the first column as <th> -->
      <xsl:when test="position()=1">
      <th><xsl:value-of select="value"/></th>
      </xsl:when>
      
      <xsl:otherwise>
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
         <!-- add stats_data_item_td class for download name, country and host name -->
         <xsl:if test="$pos=$i_dlname or $pos=$i_country or $pos=$i_host">
            <xsl:attribute name="class">stats_data_item_td</xsl:attribute>
         </xsl:if>
         
         <xsl:choose>
         <!-- wrap the host name in a span, so the IP address is shown only when the mouse hovers over the host name -->
         <xsl:when test="$pos=$i_host">
            <xsl:element name="span">
               <xsl:attribute name="title"><xsl:value-of select="$data/attribute::ipaddr"/></xsl:attribute>
               <xsl:value-of select="child::text()"/>
            </xsl:element>
         </xsl:when>
         
         <!-- otherwise, just output node content -->
         <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
         </xsl:choose>
         </td>
         
         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
   </xsl:for-each>
   
   </tr>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***              Top HTTP Errors Report Template              *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_errors&quot;]">
<xsl:variable name="rows" select="rows"/>
<xsl:variable name="report" select="self::node()"/>

<!-- define column identifiers for the error URL path -->
<xsl:variable name="i_path" select="5"/>

<table class="report_table stats_table">
<thead>
<tr class="table_title_tr"><th colspan="6"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_one_row_header"/>
</thead>

<!-- output the top section -->
<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>

   <xsl:call-template name="output_error_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_path" select="$i_path"/>
   </xsl:call-template>
</xsl:for-each>
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">
   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td colspan="15">
            <a>
            <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
            <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_errors&quot;)"/></a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
         <xsl:call-template name="output_error_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_path" select="$i_path"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>   
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr"><th colspan="15"><xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/></th></tr>
   </xsl:if>
   </tbody>
</xsl:if>

</table>

</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_error_row
   Description  : Generates one row of the error report
   ________________________________________________________________________
-->
<xsl:template name="output_error_row">
   <xsl:param name="row"/>
   <xsl:param name="i_path"/>

   <tr>

   <!-- loop through the remaining data elements -->
   <xsl:for-each select="$row/data">
      <xsl:choose>
      
      <!-- output the first column as <th> -->
      <xsl:when test="position()=1">
      <th><xsl:value-of select="value"/></th>
      </xsl:when>

      <xsl:otherwise>
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>
      
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
         <!-- add stats_data_item_td class if it's the path column -->
         <xsl:if test="$pos=$i_path">
            <xsl:attribute name="class">stats_data_item_td</xsl:attribute>
         </xsl:if>
         
         <!-- if there's a title attribute, add it to the column-->
         <xsl:if test="$data/attribute::title">
            <xsl:attribute name="title"><xsl:value-of select="$data/attribute::title"/></xsl:attribute>
         </xsl:if>
         
         <!-- output the value -->
         <xsl:value-of select="child::text()"/>
         </td>

         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      </xsl:choose>
   </xsl:for-each>
   
   </tr>   
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***                 Top Hosts Report Template                 *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_host_hits&quot;]">
   <xsl:call-template name="top_host_report"/>
</xsl:template>

<xsl:template match="report[attribute::id=&quot;top_host_xfer&quot;]">
   <xsl:call-template name="top_host_report"/>
</xsl:template>

<xsl:template name="top_host_report">
<xsl:variable name="rows" select="rows"/>
<xsl:variable name="report" select="self::node()"/>

<!-- define column identifiers for country and host columns -->
<xsl:variable name="i_country" select="8"/>
<xsl:variable name="i_host" select="9"/>

<table class="report_table stats_table">
<thead>
<tr class="table_title_tr">
	<th><xsl:attribute name="colspan"><xsl:value-of select="attribute::colspan"/></xsl:attribute>
		<xsl:value-of select="attribute::title"/>
	</th>
</tr>
<xsl:call-template name="output_two_row_header"/>
</thead>

<!-- output the section with top items -->
<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>
   
   <xsl:call-template name="output_host_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_country" select="$i_country"/>
      <xsl:with-param name="i_host" select="$i_host"/>
   </xsl:call-template>
</xsl:for-each>   
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">
   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td><xsl:attribute name="colspan"><xsl:value-of select="attribute::colspan"/></xsl:attribute>
            <a>
            <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
            <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_hosts&quot;)"/></a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
         <xsl:call-template name="output_host_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_country" select="$i_country"/>
            <xsl:with-param name="i_host" select="$i_host"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>   
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr">
	   <th><xsl:attribute name="colspan"><xsl:value-of select="attribute::colspan"/></xsl:attribute>
	   <xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/>
		   </th>
	   </tr>
   </xsl:if>
   </tbody>
</xsl:if>

</table>
</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_host_row
   Description  : Generates one row of a host report
   ________________________________________________________________________
-->
<xsl:template name="output_host_row">
   <xsl:param name="row"/>
   <xsl:param name="i_country"/>
   <xsl:param name="i_host"/>
   
   <tr>
   <!-- highlight group rows -->
   <xsl:if test="$row/data[$i_host]/attribute::group=&quot;yes&quot;">
   <xsl:attribute name="class">group_shade_tr</xsl:attribute>
   </xsl:if>

   <!-- loop through the remaining data elements -->
   <xsl:for-each select="$row/data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>
      
      <xsl:choose>
      <xsl:when test="position()=1">
      <!-- output the first column as <th> -->
      <th><xsl:value-of select="value"/></th>
      </xsl:when>
   
      <xsl:otherwise>
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
         <xsl:choose>

         <!-- add stats_data_item_td class for country -->
         <xsl:when test="$pos=$i_country">
            <xsl:attribute name="class">stats_data_item_td</xsl:attribute>
            <xsl:value-of select="child::text()"/>
         </xsl:when>

         <!-- check if the host column -->
         <xsl:when test="$pos=$i_host">
            <!-- add all host classes -->
            <xsl:attribute name="class">stats_data_item_td<xsl:if test="$data/attribute::robot=&quot;yes&quot;"> robot</xsl:if><xsl:if test="$data/attribute::spammer=&quot;yes&quot;"> spammer</xsl:if><xsl:if test="$data/attribute::converted=&quot;yes&quot;"> converted</xsl:if>
            </xsl:attribute>

            <xsl:choose>
               <!-- highlight groups -->
               <xsl:when test="$data/attribute::group=&quot;yes&quot;">
               <strong><xsl:value-of select="$data/value"/></strong>
               </xsl:when>
               
               <xsl:otherwise>
                  <!-- wrap the host name in a span, so the IP address is shown only when the mouse hovers over the host name -->
                  <xsl:element name="span">
                     <xsl:attribute name="title"><xsl:value-of select="$data/attribute::ipaddr"/></xsl:attribute>
                     <xsl:value-of select="child::text()"/>
                  </xsl:element>
               </xsl:otherwise>
            </xsl:choose>
         </xsl:when>
         
         <!-- otherwise, just output node content -->
         <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
         </xsl:choose>
         </td>
         
         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
   </xsl:for-each>

   </tr>   
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***            Top HTTP Referrers Report Template             *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_referrers&quot;]">
<xsl:variable name="report" select="self::node()"/>
<xsl:variable name="rows" select="rows"/>
<xsl:variable name="i_url" select="4"/>

<table class="report_table stats_table">

<!-- output report header section -->
<thead>
<tr class="table_title_tr"><th colspan="6"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_one_row_header"/>
</thead>

<!-- output the section with top items -->
<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>
   
   <!-- output data row -->
   <xsl:call-template name="output_referrer_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_url" select="$i_url"/>
   </xsl:call-template>
</xsl:for-each>   
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">

   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td colspan="6">
            <a>
            <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
            <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_referrers&quot;)"/></a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
      
         <!-- output data row -->
         <xsl:call-template name="output_referrer_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_url" select="$i_url"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>   
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr"><th colspan="6"><xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/></th></tr>
   </xsl:if>
   </tbody>

</xsl:if>

</table>

</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_referrer_row
   Description  : Generates one row of the referrer report
   ________________________________________________________________________
-->
<xsl:template name="output_referrer_row">
   <xsl:param name="row"/>
   <xsl:param name="i_url"/>

   <tr>
   <!-- highlight group rows -->
   <xsl:if test="$row/data[$i_url]/attribute::group=&quot;yes&quot;">
   <xsl:attribute name="class">group_shade_tr</xsl:attribute>
   </xsl:if>

   <!-- loop through the data elements -->
   <xsl:for-each select="$row/data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>

      <xsl:choose>
      <!-- output the first column as <th> -->
      <xsl:when test="position()=1">
      <th><xsl:value-of select="value"/></th>
      </xsl:when>
   
      <xsl:otherwise>
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
            <xsl:choose>
            <xsl:when test="$pos=$i_url">
               <!-- add stats_data_item_td class to the referrer column -->
               <xsl:attribute name="class">stats_data_item_td</xsl:attribute>

               <xsl:choose>
               <!-- highlight groups -->
               <xsl:when test="$data/attribute::group=&quot;yes&quot;">
               <strong><xsl:value-of select="child::text()"/></strong>
               </xsl:when>

               <!-- if there is a URL, create a link -->
               <xsl:when test="$data/attribute::url">
               <a><xsl:attribute name="href"><xsl:value-of select="$data/attribute::url"/></xsl:attribute><xsl:value-of select="child::text()"/></a>
               </xsl:when>
               
               <!-- not a link or group - output as is -->
               <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
               </xsl:choose>
            </xsl:when>

            <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
            </xsl:choose>
         </td>

         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
   </xsl:for-each>
   
   </tr>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***          Top HTTP Search Strings Report Template          *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_search_strings&quot;]">
<xsl:variable name="report" select="self::node()"/>
<xsl:variable name="rows" select="rows"/>
<xsl:variable name="i_srch" select="4"/>

<table class="report_table stats_table">
<thead>
<tr class="table_title_tr"><th colspan="6"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_one_row_header"/>
</thead>

<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>

   <xsl:call-template name="output_search_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_srch" select="$i_srch"/>
   </xsl:call-template>
</xsl:for-each>
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">

   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td colspan="6">
            <a>
            <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
            <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_search&quot;)"/></a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
      
         <!-- output data row -->
         <xsl:call-template name="output_search_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_srch" select="$i_srch"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>   
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr"><th colspan="6"><xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/></th></tr>
   </xsl:if>
   </tbody>

</xsl:if>

</table>
</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_search_row
   Description  : Generates one row of the search string report
   ________________________________________________________________________
-->
<xsl:template name="output_search_row">
   <xsl:param name="row"/>
   <xsl:param name="i_srch"/>
   
   <tr>
   
   <!-- loop through the remaining data elements -->
   <xsl:for-each select="$row/data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>

      <xsl:choose>
      <xsl:when test="position()=1">
      <!-- output the first ID column -->
      <th><xsl:value-of select="$data[1]/value"/></th>
      </xsl:when>
      
      <xsl:otherwise>   
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
            <xsl:choose>
            <xsl:when test="$pos=$i_srch">
               <!-- add stats_data_item_td class to the referrer column -->
               <xsl:attribute name="class">stats_data_item_td</xsl:attribute>

               <xsl:choose>
               <xsl:when test="local-name(self::node())=&quot;sequence&quot;">
                  <!-- loop through all search terms -->
                  <xsl:for-each select="value">
                     <!-- wrap search term types into a span -->
                     <span class="search_type">
                     
                     <!-- for subsequent terms, add a space before -->
                     <xsl:if test="position()>1"><xsl:text> </xsl:text></xsl:if>
                     
                     <xsl:choose>
                        <!-- if there's a search type, output it before the term -->
                        <xsl:when test="attribute::type">
                           [<xsl:value-of select="attribute::type"/>]
                           <xsl:text> </xsl:text>
                        </xsl:when>
                        
                        <!-- otherwise, output a bullet to separate terms -->
                        <xsl:otherwise><xsl:if test="position()>1">&bull; </xsl:if></xsl:otherwise>
                     </xsl:choose>
                     
                     </span>
                     
                     <!-- output the search term -->
                     <xsl:value-of select="child::text()"/>
                  </xsl:for-each>
               </xsl:when>
               
               <!-- output a non-sequence value -->
               <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
               </xsl:choose>

            </xsl:when>
            
            <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
            </xsl:choose>
         </td>

         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
      
   </xsl:for-each>
   </tr>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***              Top HTTP Users Report Template               *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_users&quot;]">
<xsl:variable name="report" select="self::node()"/>
<xsl:variable name="rows" select="rows"/>
<xsl:variable name="i_user" select="7"/>

<table class="report_table stats_table">
<thead>
<tr class="table_title_tr"><th colspan="12"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_two_row_header"/>
</thead>

<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>

   <xsl:call-template name="output_user_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_user" select="$i_user"/>
   </xsl:call-template>
</xsl:for-each>
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">

   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td colspan="12">
            <a>
            <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
            <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_users&quot;)"/></a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
      
         <!-- output data row -->
         <xsl:call-template name="output_user_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_user" select="$i_user"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>   
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr"><th colspan="12"><xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/></th></tr>
   </xsl:if>
   </tbody>

</xsl:if>

</table>
</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_user_row
   Description  : Generates one row of the user report
   ________________________________________________________________________
-->
<xsl:template name="output_user_row">
   <xsl:param name="row"/>
   <xsl:param name="i_user"/>

   <tr>
   <!-- highlight group rows -->
   <xsl:if test="$row/data[$i_user]/attribute::group=&quot;yes&quot;">
   <xsl:attribute name="class">group_shade_tr</xsl:attribute>
   </xsl:if>
   
   <!-- loop through the remaining data elements -->
   <xsl:for-each select="$row/data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>

      <xsl:choose>
      <xsl:when test="position()=1">
      <!-- output the first column as <th> -->
      <th><xsl:value-of select="value"/></th>
      </xsl:when>
   
      <xsl:otherwise>
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
         <xsl:if test="$pos=$i_user">
            <!-- add stats_data_item_td class to the user column -->
            <xsl:attribute name="class">stats_data_item_td</xsl:attribute>
         </xsl:if>
         <xsl:value-of select="child::text()"/>
         </td>

         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
      
   </xsl:for-each>
   </tr>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***           Top HTTP Users Agents Report Template           *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_user_agents&quot;]">
<xsl:variable name="report" select="self::node()"/>
<xsl:variable name="rows" select="rows"/>
<xsl:variable name="i_agent" select="5"/>

<table class="report_table stats_table">
<thead>
<tr class="table_title_tr"><th colspan="8"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_one_row_header"/>
</thead>

<tbody class="stats_data_tbody">
<xsl:for-each select="top/child::*">
   <xsl:variable name="rowid" select="position()"/>
   <xsl:variable name="row" select="$rows/row[$rowid]"/>

   <xsl:call-template name="output_agent_row">
      <xsl:with-param name="row" select="$row"/>
      <xsl:with-param name="i_agent" select="$i_agent"/>
   </xsl:call-template>
</xsl:for-each>
</tbody>

<!-- check if there's more -->
<xsl:if test="count($rows/row) > $report/attribute::top">

   <!-- output the row with the link to view all items -->
   <tbody class="stats_footer_tbody">
      <tr class="all_items_tr">
         <td colspan="8">
            <a>
            <xsl:attribute name="onclick">showAllItems(this, <xsl:value-of select="$report/attribute::top"/>, <xsl:value-of select="count($rows/row)"/>)</xsl:attribute>
            <xsl:value-of select="key(&quot;text&quot;, &quot;html_view_all_agents&quot;)"/></a>
         </td>
      </tr>
   </tbody>

   <!-- output the section with the remaining items -->
   <tbody class="stats_data_tbody all_items_tbody">
   <xsl:for-each select="rows/row">
      <!-- skip those we already prcocessed -->
      <xsl:if test="position() &gt; $report/attribute::top">
      
         <!-- output data row -->
         <xsl:call-template name="output_agent_row">
            <xsl:with-param name="row" select="self::node()"/>
            <xsl:with-param name="i_agent" select="$i_agent"/>
         </xsl:call-template>
      </xsl:if>
   </xsl:for-each>   
   <!-- report if we reached the maximum -->
   <xsl:if test="count($rows/row) = $report/attribute::max">
   <tr class="max_items_tr"><th colspan="8"><xsl:value-of select="key(&quot;text&quot;, &quot;html_max_items&quot;)"/></th></tr>
   </xsl:if>
   </tbody>

</xsl:if>

</table>
</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_agent_row
   Description  : Generates one row of the user agent report
   ________________________________________________________________________
-->
<xsl:template name="output_agent_row">
   <xsl:param name="row"/>
   <xsl:param name="i_agent"/>
   
   <tr>
   <!-- highlight group rows -->
   <xsl:if test="$row/data[$i_agent]/attribute::group=&quot;yes&quot;">
   <xsl:attribute name="class">group_shade_tr</xsl:attribute>
   </xsl:if>
   
   <!-- loop through the remaining data elements -->
   <xsl:for-each select="$row/data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>
      
      <xsl:choose>
      
      <xsl:when test="position()=1">
      <!-- output the first column as <th> -->
      <th><xsl:value-of select="value"/></th>
      </xsl:when>

      <xsl:otherwise>
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
            <xsl:choose>
            <xsl:when test="$pos=$i_agent">
               <!-- add stats_data_item_td class to the user column -->
               <xsl:attribute name="class">stats_data_item_td</xsl:attribute>
               
               <xsl:choose>
                  <!-- highlight groups -->
                  <xsl:when test="$data/attribute::group=&quot;yes&quot;">
                  <strong><xsl:value-of select="child::text()"/></strong>
                  </xsl:when>
                  
                  <!-- output other values as is -->
                  <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
               </xsl:choose>
            </xsl:when>
            
            <xsl:otherwise><xsl:value-of select="child::text()"/></xsl:otherwise>
            </xsl:choose>
         </td>

         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
      
   </xsl:for-each>
   </tr>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***            Top Countries Report Template (PNG)            *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_countries&quot;]" mode="graph-png">
<div id="daily_usage_graph" class="graph_holder">
<xsl:call-template name="output_image"/>
</div>
<xsl:call-template name="output_notes"/>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***              Top Countries Report Template                *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;top_countries&quot;]">
<xsl:variable name="i_country" select="6"/>

<table class="report_table stats_table">
<thead>
<tr class="table_title_tr"><th colspan="10"><xsl:value-of select="attribute::title"/></th></tr>
<xsl:call-template name="output_one_row_header"/>
</thead>

<tbody class="stats_data_tbody">
<xsl:for-each select="rows/row">
   <tr>
   
   <!-- loop through the remaining data elements -->
   <xsl:for-each select="data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>

      <xsl:choose>
      <xsl:when test="position()=1">
      <!-- output the first column as <th> -->
      <th><xsl:value-of select="value"/></th>
      </xsl:when>
      
      <xsl:otherwise>      
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
            <xsl:if test="$pos=$i_country">
               <!-- add stats_data_item_td class to the user column -->
               <xsl:attribute name="class">stats_data_item_td</xsl:attribute>
            </xsl:if>
            <xsl:value-of select="child::text()"/>
         </td>

         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
      
   </xsl:for-each>
   </tr>
</xsl:for-each>
</tbody>
</table>
<xsl:call-template name="output_notes"/>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***                       Miscellaneous                       *** -->
<!-- ***************************************************************** -->

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_totals_row
   Context      : /sswdoc/report[name]/rows/row
   Parameters   : none
   Description  :
   
   Generates a row for daily and hourly totals. The first column contains 
   a day or an hour. Remaining columns are generated out of data items, 
   one for each child of the data element and one for each percent 
   attribute.
   ________________________________________________________________________
-->
<xsl:template name="output_totals_row">
   
   <!-- loop through the data columns -->
   <xsl:for-each select="data">
      
      <xsl:choose>
      <xsl:when test="position()=1">
      <!-- output the item ID column (e.g. day, hour, etc) -->
      <th><xsl:value-of select="value"/></th>
      </xsl:when>
         
      <xsl:otherwise>
      <!-- loop through child elements of data -->
      <xsl:for-each select="child::*">
         <td><xsl:value-of select="child::text()"/></td>
         
         <!-- add a column if there's a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
      
   </xsl:for-each>
</xsl:template>

<!-- 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_url_row
   Parameters   :
   
      $row     - data row element
      $i_url   - URL position in the data row
      
   Description  : Generates a row of a URL report (e.g. top hits, entry, etc)
   ________________________________________________________________________
-->
<xsl:template name="output_url_row">
   <xsl:param name="row"/>
   <xsl:param name="i_url"/>

   <tr>
   <!-- highlight group rows -->
   <xsl:if test="$row/data[$i_url]/attribute::group=&quot;yes&quot;">
   <xsl:attribute name="class">group_shade_tr</xsl:attribute>
   </xsl:if>
   
   <!-- loop through the data columns -->
   <xsl:for-each select="$row/data">
      <xsl:variable name="pos" select="position()"/>
      <xsl:variable name="data" select="self::node()"/>

      <xsl:choose>
      <xsl:when test="position()=1">
      <!-- output the first ID column -->
      <th><xsl:value-of select="value"/></th>
      </xsl:when>
      
      <xsl:otherwise>   
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td>
         <xsl:choose>
            <!-- check if it's the URL column -->
            <xsl:when test="$pos = $i_url">
               <!-- output additional URL attributes (e.g. target) -->
               <xsl:attribute name="class">stats_data_item_td<xsl:if test="$data/attribute::target"> target</xsl:if></xsl:attribute>
               
               <!-- generate URL column value -->
               <xsl:call-template name="output_url"><xsl:with-param name="data" select="$data"/></xsl:call-template>
            </xsl:when>
            
            <xsl:otherwise>
               <!-- output other columns as-is -->
               <xsl:value-of select="child::text()"/>
            </xsl:otherwise>
         </xsl:choose>
         </td>
         
         <!-- add a column if there is a percent attribute -->
         <xsl:if test="attribute::percent">
         <td class="data_percent_td"><xsl:value-of select="attribute::percent"/>%</td>
         </xsl:if>
      </xsl:for-each>
      </xsl:otherwise>
      
      </xsl:choose>
      
   </xsl:for-each>
   </tr>

</xsl:template>

</xsl:stylesheet>
