<?xml version="1.0" encoding="utf-8" ?>
<!--
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
 -->
<!DOCTYPE xsl:stylesheet [
<!ENTITY copy "&#169;"> <!ENTITY nbsp "&#160;"> <!ENTITY bull "&#8226;">
]>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- configuration node shortcut -->
<xsl:variable name="config" select="/sswdoc/application/config"/>

<!-- localized text fragments -->
<xsl:key name="text" match="/sswdoc/dictionary/text" use="attribute::name"/>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Graph type:
   
      graph-png         - classic PNG image graphs 
      graph-flash-amc   - amCharts (see graphs-amc.xsl)
      graph-flash-ofc   - Open Flash Charts (see graphs-ofc.xsl)

-->
<xsl:variable name="graphtype">graph-<xsl:value-of select="$config/graphtype"/></xsl:variable>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Specify the location of amCharts and OFC files. These are URL paths, 
   so make sure to use forward slashes and terminate the path with a slash.
   
   Relative paths will be resolved using the directory of the XML file 
   including this stylesheet.
   
   Avoid using any special characters in paths, such as double quotes.
-->
<xsl:variable name="amcpath">amcharts/</xsl:variable>
<xsl:variable name="ofcpath">ofcharts/</xsl:variable>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Grah colors (Flash)
-->

<!-- grab graph colors from the XML file -->
<xsl:variable name="graph_bg_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_bg_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_bg_color&quot;]"/></xsl:when><xsl:otherwise>E0E0E0</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_grid_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_grid_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_grid_color&quot;]"/></xsl:when><xsl:otherwise>808080</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_title_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_title_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_title_color&quot;]"/></xsl:when><xsl:otherwise>0000FF</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_hits_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_hits_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_hits_color&quot;]"/></xsl:when><xsl:otherwise>00805C</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_files_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_files_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_files_color&quot;]"/></xsl:when><xsl:otherwise>0000FF</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_pages_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_pages_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_pages_color&quot;]"/></xsl:when><xsl:otherwise>00C0FF</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_xfer_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_xfer_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_xfer_color&quot;]"/></xsl:when><xsl:otherwise>FF0000</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_visits_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_visits_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_visits_color&quot;]"/></xsl:when><xsl:otherwise>FFFF00</xsl:otherwise></xsl:choose></xsl:variable>
<xsl:variable name="graph_hosts_color"><xsl:choose><xsl:when test="$config/color[attribute::name=&quot;graph_hosts_color&quot;]"><xsl:value-of select="$config/color[attribute::name=&quot;graph_hosts_color&quot;]"/></xsl:when><xsl:otherwise>FF8000</xsl:otherwise></xsl:choose></xsl:variable>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_copyright_comment
   Context      : /sswdoc
   Parameters   : none
   Description  : Outputs the comment at the top of the HTML report
-->
<xsl:template name="output_copyright_comment" xml:space="preserve">
<xsl:comment>      Stone Steps Webalizer v<xsl:value-of select="application/version"/>      </xsl:comment>
<xsl:comment>                                          </xsl:comment>
<xsl:comment> Copyright (c) 2004-2013 Stone Steps Inc. </xsl:comment>
<xsl:comment>         http://www.stonesteps.ca         </xsl:comment>
<xsl:comment>                                          </xsl:comment>
<xsl:comment> Distributed under the GNU GPL  Version 2 </xsl:comment>
<xsl:comment>        Full text may be found at:        </xsl:comment>
<xsl:comment>  http://www.stonesteps.ca/legal/gpl.asp  </xsl:comment>
<xsl:comment>                                          </xsl:comment>
<xsl:comment> *** Generated: <xsl:value-of select="summary/created/date"/> <xsl:value-of select="summary/created/time"/> (<xsl:value-of select="summary/created/time/attribute::tzname"/>) *** </xsl:comment>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_summary_period
   Context      : /sswdoc
   Parameters   : none
   Description  : Outputs the summary period in the HTML header & title
-->
<xsl:template name="output_summary_period">
   <xsl:choose>
   <xsl:when test="summary/period/attribute::months > 1">
      <!-- Last N Months -->
      <xsl:value-of select="key(&quot;text&quot;, &quot;html_hdr_period_last&quot;)"/><xsl:text> </xsl:text>
      <xsl:value-of select="summary/period/attribute::months"/><xsl:text> </xsl:text>
      <xsl:value-of select="key(&quot;text&quot;, &quot;html_hdr_period_months&quot;)"/><xsl:text> </xsl:text>
   </xsl:when>
   <xsl:otherwise>
      <!-- Month Year -->
      <xsl:value-of select="summary/period/start/month/attribute::name"/><xsl:text> </xsl:text>
      <xsl:value-of select="summary/period/start/year"/>
   </xsl:otherwise>
   </xsl:choose>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_html_head
   Context      : /sswdoc
   Parameters   : none
   Description  : Outputs the common part of the HTML head element
-->
<xsl:template name="output_html_head">
<meta name="robots" content="noindex,nofollow"/>
<title>
   <!-- Usage Statisticts for -->
   <xsl:value-of select="key(&quot;text&quot;, &quot;html_hdr_usage_stats_for&quot;)"/><xsl:text> </xsl:text>
   <xsl:value-of select="summary/hostname"/> -<xsl:text> </xsl:text>

   <xsl:call-template name="output_summary_period"/>
</title>
<link rel="stylesheet" type="text/css"><xsl:attribute name="href"><xsl:value-of select="application/config/csspath"/>webalizer.css</xsl:attribute></link>
<script type="text/javascript"><xsl:attribute name="src"><xsl:value-of select="application/config/jspath"/>webalizer.js</xsl:attribute></script>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_page_header
   Context      : /sswdoc
   Parameters   : none
   Description  : Outputs the common HTML header of a report page
-->
<xsl:template name="output_page_header">
<div class="page_header_div">
<a name="top"></a>
<h1>
   <!-- Usage Statisticts for -->
   <xsl:value-of select="key(&quot;text&quot;, &quot;html_hdr_usage_stats_for&quot;)"/><xsl:text> </xsl:text>
   <xsl:value-of select="summary/hostname"/>
</h1>
<div class="usage_summary_div">
<em><xsl:value-of select="key(&quot;text&quot;, &quot;html_hdr_summary_period&quot;)"/>:<xsl:text> </xsl:text>
   <xsl:call-template name="output_summary_period"/>
</em><br/>

<!-- "Generated" -->
<xsl:value-of select="key(&quot;text&quot;, &quot;html_hdr_generated&quot;)"/><xsl:text> </xsl:text>
   <xsl:value-of select="summary/created/date"/><xsl:text> </xsl:text>
   <xsl:value-of select="substring(summary/created/time, 1, 8)"/><xsl:text> </xsl:text>
   (<xsl:value-of select="summary/created/time/attribute::tzname"/>)
</div>
</div>
</xsl:template>

<!--  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_page_footer
   Context      : /sswdoc
   Parameters   : none
   Description  : Outputs the common HTML footer of a report page
-->
<xsl:template name="output_page_footer">
<xsl:comment> Page Footer </xsl:comment>
<div class="page_footer_div">
<a href="http://www.stonesteps.ca/webalizer">Stone Steps Webalizer</a> (v<xsl:value-of select="application/version"/>)
</div>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_two_row_header
   Context      : /sswdoc/report
   Parameters   : none
   
   Description  :
   
   Generates a two-row table header. The first row contains a column for
   every first-level <col> element. Those first-row columns that don't 
   have children, span two rows. The second row contains one column for 
   each second-level <col> element. There is one additional column for 
   each percent attribute that exists in the first or second row column.
   
   If a sub-column of a second-row column has a class attribute, it will 
   be added to the output table cell. Otherwise, parent column's class 
   will be added to all sub-columns.
-->
<xsl:template name="output_two_row_header">
<tr>
   <!-- loop through the outer columns to form the first row -->
   <xsl:for-each select="columns/col">
   <th>
   <!-- add class attribute -->
   <xsl:attribute name="class"><xsl:value-of select="attribute::class"/></xsl:attribute>
   
   <xsl:choose>
      <xsl:when test="not(columns)">
         <!-- span two rows if the column doesn't have sub-columns -->
         <xsl:attribute name="rowspan">2</xsl:attribute>
         
         <!-- span two columns if there's a percent column -->
         <xsl:if test="attribute::percent=&quot;yes&quot;"><xsl:attribute name="colspan">2</xsl:attribute></xsl:if>
      </xsl:when>
      <xsl:otherwise>
         <!-- span multiple columns, one for each sub-column and one for each percent column -->
         <xsl:attribute name="colspan"><xsl:value-of select="count(columns/col)+count(columns/col[attribute::percent])"/></xsl:attribute>
      </xsl:otherwise>
   </xsl:choose>
   
   <!-- and output column title -->
   <xsl:call-template name="output_title_help">
      <xsl:with-param name="title" select="attribute::title"/>
      <xsl:with-param name="topic" select="attribute::help"/>
   </xsl:call-template>
   </th>
   </xsl:for-each>
</tr>
<tr>
   <!-- loop through the outer columns again to output the second row -->
   <xsl:for-each select="columns/col">
   <xsl:variable name="class" select="attribute::class"/>
   
   <!-- loop through the sub-columns -->
   <xsl:for-each select="columns/col">
      <th>
      <!-- span two columns if there's a percent column -->
      <xsl:if test="attribute::percent=&quot;yes&quot;"><xsl:attribute name="colspan">2</xsl:attribute></xsl:if>
      
      <!-- use the inner column class, if there is one; otherwise use parent's class -->
      <xsl:choose>
      <xsl:when test="attribute::class">
         <!-- just output column's class -->
         <xsl:attribute name="class"><xsl:value-of select="attribute::class"/></xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
         <!-- add parent's class and make text smaller -->
         <xsl:attribute name="class"><xsl:value-of select="$class"/> small_font_th</xsl:attribute>
      </xsl:otherwise>
      </xsl:choose>
      
      <!-- and output column title -->
      <xsl:call-template name="output_title_help">
         <xsl:with-param name="title" select="attribute::title"/>
         <xsl:with-param name="topic" select="attribute::help"/>
      </xsl:call-template>
      </th>
      </xsl:for-each>
   </xsl:for-each>
</tr>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_one_row_header
   Context      : /sswdoc/report
   Parameters   : none
   
   Description  :
   
   Generates a one-row table header. Each column that has the percent
   attrubute set to "yes" is spanned two columns.
-->
<xsl:template name="output_one_row_header">
<tr>
   <!-- loop through columns -->
   <xsl:for-each select="columns/col">
   <th>
   <!-- add class attribute -->
   <xsl:attribute name="class"><xsl:value-of select="attribute::class"/></xsl:attribute>
   
   <!-- span two columns if there's a percent column -->
   <xsl:if test="attribute::percent=&quot;yes&quot;"><xsl:attribute name="colspan">2</xsl:attribute></xsl:if>
   
   <!-- and output column title -->
   <xsl:call-template name="output_title_help">
      <xsl:with-param name="title" select="attribute::title"/>
      <xsl:with-param name="topic" select="attribute::help"/>
   </xsl:call-template>
   </th>
   </xsl:for-each>
</tr>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_url
   Context      : /sswdoc/report
   Parameters   : 
   
      $data      - URL or URL group data element
      
   Description  : Generates the contents of the URL column
-->
<xsl:template name="output_url">
   <xsl:param name="data"/>
   
   <xsl:variable name="hostname" select="/sswdoc/summary/hostname"/>

   <!-- choose between groups, fully-qualified links, secure and non-secure site links -->
   <xsl:choose>
      <!-- highlight groups -->
      <xsl:when test="$data/attribute::group=&quot;yes&quot;">
      <strong><xsl:value-of select="$data/value"/></strong>
      </xsl:when>
      
      <!-- output fully-qualified URLs without any prefix -->
      <xsl:when test="$data/attribute::complete=&quot;yes&quot;">
      <a><xsl:attribute name="href"><xsl:value-of select="$data/attribute::url"/></xsl:attribute><xsl:value-of select="$data/value"/></a>
      </xsl:when>
      
      <!-- prefix secure URLs with https://host-name -->
      <xsl:when test="$data/attribute::secure=&quot;yes&quot;">
      <a><xsl:attribute name="href">https://<xsl:value-of select="$hostname"/><xsl:value-of select="$data/attribute::url"/></xsl:attribute><xsl:value-of select="$data/value"/></a>
      </xsl:when>
      
      <!-- prefix other URLs with http://hostname -->
      <xsl:otherwise>
      <a><xsl:attribute name="href">http://<xsl:value-of select="$hostname"/><xsl:value-of select="$data/attribute::url"/></xsl:attribute><xsl:value-of select="$data/value"/></a>
      </xsl:otherwise>
   </xsl:choose>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_notes
   Context      : /sswdoc/report
   Parameters   : none
   Description  : Generates a paragraph for every note element within 
-->
<xsl:template name="output_notes">
<!-- output the content of the notes node -->
<xsl:for-each select="notes/note">
<p class="note_p"><xsl:value-of select="child::text()"/></p>
</xsl:for-each>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_image
   Context      : /sswdoc/report
   Parameters   : none
   Description  : Generates an image element
-->
<xsl:template name="output_image">
<img>
<xsl:attribute name="src"><xsl:value-of select="graph/image/attribute::filename"/></xsl:attribute>
<xsl:attribute name="width"><xsl:value-of select="graph/image/attribute::width"/></xsl:attribute>
<xsl:attribute name="height"><xsl:value-of select="graph/image/attribute::height"/></xsl:attribute>
<xsl:attribute name="alt"><xsl:value-of select="graph/attribute::title"/></xsl:attribute>
</img>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_graph
   Context      : /sswdoc
   Parameters   : 
   
      report - report node path
      
   Description  : Applies the appropriate graph template for the specified
                  report, based on the value of $graphtype.
-->
<xsl:template name="output_graph">
   <xsl:param name="report"/>
   
   <xsl:choose>
   <xsl:when test="$graphtype=&quot;graph-png&quot;">
   <xsl:apply-templates select="$report" mode="graph-png"/>
   </xsl:when>
   <xsl:when test="$graphtype=&quot;graph-flash-ofc&quot;">
   <xsl:apply-templates select="$report" mode="graph-flash-ofc"/>
   </xsl:when>
   <xsl:when test="$graphtype=&quot;graph-flash-amc&quot;">
   <xsl:apply-templates select="$report" mode="graph-flash-amc"/>
   </xsl:when>
</xsl:choose>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_title_help
   Context      : N/A
   Parameters   : 
   
      title    -  title text
      topic    -  help topic identifier
      
   Description  : If $topic is not empty, wraps $title into a clickable 
                  help span; otherwise, just outputs $title.
-->
<xsl:template name="output_title_help">
   <xsl:param name="title"/>
   <xsl:param name="topic"/>
   
   <xsl:choose>
      <!-- if there's a help attribute, wrap title into a clickable span -->
      <xsl:when test="$topic">
         <span class="help_span">
         <xsl:attribute name="onclick">showHelp(event, this, "<xsl:value-of select="$topic"/>")</xsl:attribute>
         <xsl:value-of select="$title"/>
         </span>
      </xsl:when>
      
      <!-- otherwise just output the title -->
      <xsl:otherwise><xsl:value-of select="$title"/></xsl:otherwise>
   </xsl:choose>
   
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_help_box
   Context      : N/A
   Parameters   : N/A
   Description  : Outputs a hidden absolutely positioned help div
-->
<xsl:template name="output_help_box">
<div id="helpbox" class="helpbox">
<div class="title"><div class="x" onclick="hideHelp()">&#x2612;</div>
</div>
</div>
</xsl:template>

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Name         : output_help_text
   Context      : /sswdoc
   Parameters   : N/A
   Description  : Outputs a hidden div containing help markup. Each help 
                  entry is wrapped in a div, whose title attribute is set
                  using help topic.
-->
<xsl:template name="output_help_text">
   <!-- help div container is always hidden -->
   <div id="helptext" style="display: none">
   <xsl:for-each select="help/text">
   
      <!-- skip empty nodes -->
      <xsl:if test="child::node()">
         <!-- wrap help markup in a div -->
         <div>
            <!-- add a title attribute, which is used by JavaScript to find the topic -->
            <xsl:attribute name="title"><xsl:value-of select="attribute::name"/></xsl:attribute>
            
            <!-- and copy all markup nodes (XHTML) -->
            <xsl:copy-of select="child::node()"/>
         </div>
      </xsl:if>
   </xsl:for-each>
   </div>
</xsl:template>

</xsl:stylesheet>
