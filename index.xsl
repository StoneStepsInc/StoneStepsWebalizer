<?xml version="1.0" encoding="utf-8" ?>
<!--
   webalizer - a web server log analysis program

   Copyright (c) 2004-2015, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
 -->
<!DOCTYPE xsl:stylesheet [
<!ENTITY copy "&#169;"> <!ENTITY nbsp "&#160;"> <!ENTITY bull "&#8226;">
]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- import named templates and common configuration settings -->
<xsl:import href="webalizer.xsl"/>

<!-- configure output as HTML 4.01 strict -->
<xsl:output method="html" indent="no" encoding="utf-8" doctype-public="-//W3C//DTD HTML 4.01//EN" doctype-system="http://www.w3.org/TR/html4/strict.dtd" />

<!-- include Flash graph templates -->
<xsl:include href="graphs-ofc.xsl"/>
<xsl:include href="graphs-amc.xsl"/>

<!-- ***************************************************************** -->
<!-- ***                   Main Report Template                    *** -->
<!-- ***************************************************************** -->
<xsl:template match="/sswdoc">

<!-- create a report path alias variable -->
<xsl:variable name="report_summary" select="report[attribute::id=&quot;summary&quot;]"/>

<!-- output the copyright comment -->
<xsl:call-template name="output_copyright_comment"/>

<html><xsl:attribute name="lang"><xsl:value-of select="attribute::lang"/></xsl:attribute>
<head>
<xsl:call-template name="output_html_head"/>

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

<body onload="onloadpage(PAGE_INDEX)" onkeyup="return onpagekeyup(event)">

<!-- page header -->
<xsl:call-template name="output_page_header"/>

<!-- monthly summary table -->
<xsl:comment> Monthly Summary Table </xsl:comment>
<xsl:call-template name="output_graph"><xsl:with-param name="report" select="$report_summary"/></xsl:call-template>
<xsl:apply-templates select="$report_summary"/>

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
<!-- ***           Monthly Summary Graph Template (PNG)            *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;summary&quot;]" mode="graph-png">
<div id="monthly_summary_graph" class="graph_holder">
<!-- set div width to match image size -->
<xsl:attribute name="style">width: <xsl:value-of select="graph/image/attribute::width"/>px</xsl:attribute>
<xsl:call-template name="output_image"/>
</div>
<!-- output report notes -->
<xsl:call-template name="output_notes"/>
</xsl:template>

<!-- ***************************************************************** -->
<!-- ***                Monthly Summary Template                   *** -->
<!-- ***************************************************************** -->
<xsl:template match="report[attribute::id=&quot;summary&quot;]">

<table class="report_table monthly_summary_table">
<thead>
<tr class="table_title_tr">
<th colspan="13"><xsl:value-of select="attribute::title"/></th>
</tr>
<xsl:call-template name="output_two_row_header"/>
</thead>

<tbody class="summary_data_tbody">
   <xsl:for-each select="rows/row">
   <tr>
   <!-- loop through the data nodes -->
   <xsl:for-each select="data">
      <xsl:variable name="pos" select="position()"/>

      <xsl:choose>
      <xsl:when test="position()=1">
         <!-- output the first column as a link -->
         <th><a><xsl:attribute name="href"><xsl:value-of select="attribute::url"/></xsl:attribute>
         <xsl:value-of select="value"/></a>
         </th>
      </xsl:when>
      
      <xsl:otherwise>
         <!-- loop through data sub-columns (sum, value, etc) -->
         <xsl:for-each select="child::*">
            <td><xsl:value-of select="child::text()"/></td>

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

<!-- the totals row -->
<tbody class="summary_footer_tbody">
<tr class="table_footer_tr">
<th><xsl:value-of select="totals/attribute::title"/></th>
<xsl:for-each select="totals/data">
      <!-- loop through data sub-columns (sum, value, etc) -->
      <xsl:for-each select="child::*">
         <td><xsl:value-of select="child::text()"/></td>
      </xsl:for-each>
</xsl:for-each>
</tr>
</tbody>

</table>
</xsl:template>

</xsl:stylesheet>
