/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   html_output.cpp
*/
#include "pch.h"

/*********************************************/
/* STANDARD INCLUDES                         */
/*********************************************/

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#ifndef _WIN32
#include <unistd.h>                           /* normal stuff             */
#endif

/* ensure sys/types */
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

/* some systems need this */
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "lang.h"
#include "hashtab.h"
#include "linklist.h"
#include "util.h"
#include "history.h"
#include "exception.h"
#include "html_output.h"
#include "preserve.h"

//
//
//
html_output_t::html_output_t(const config_t& config, const state_t& state) : 
      output_t(config, state), 
      buffer(new char[BUFSIZE]),
      graph(config), 
      buffer_formatter(buffer, BUFSIZE, buffer_formatter_t::overwrite)
{
}

html_output_t::~html_output_t(void)
{
   delete [] buffer;
}

bool html_output_t::init_output_engine(void)
{
   // initialize the graph engine even if we don't need to make images (e.g. read configuration)
   graph.init_graph_engine();
   
   return true;
}

void html_output_t::cleanup_output_engine(void)
{
   graph.cleanup_graph_engine();
}

const char *html_output_t::fmt_printf(const char *fmt, ...)
{
   va_list args;

   va_start(args, fmt);

   // make sure we free va_list even if fmt_vprintf throws
   struct va_list_wrapper_t {
      va_list& args;
      ~va_list_wrapper_t() {va_end(args);}
   } va_list_wrapper = {args};

   return buffer_formatter.format(fmt_vprintf, fmt, args);
}

const char *html_output_t::html_encode(const char *str)
{
   return buffer_formatter.format(encode_string<encode_char_html>, str);
}

const char *html_output_t::js_encode(const char *str)
{
   return buffer_formatter.format(encode_string<encode_char_js>, str);
}

const char *html_output_t::fmt_xfer(uint64_t xfer, bool pre)
{
   auto fmt_kbyte = [](string_t::char_buffer_t& buffer, double xfer) -> size_t
   {
      size_t olen;
      
      olen = snprintf(buffer, buffer.capacity(), "%8.0f", xfer);

      if(olen >= buffer.capacity())
         throw exception_t(0, "Cannot format a transfer amount because the output buffer is too small");

      return olen + 1;
   };

   // check if we need to output classic transfer amounts in KBytes
   if(config.classic_kbytes)
      return buffer_formatter.format(fmt_kbyte, xfer / (config.decimal_kbytes ? 1000. : 1024.));

   // buffer_formatter_t::format always returns a holder buffer, so we can return a pointer to the buffer memory
   return buffer_formatter.format(fmt_hr_num, xfer, pre ? " " : "&nbsp;", config.lang.msg_unit_pfx, config.lang.msg_xfer_unit, config.decimal_kbytes);
}

void html_output_t::write_js_charts_head(FILE *out_fp, page_type_t page_type)
{
   buffer_formatter.set_scope_mode(buffer_formatter_t::append),
   fprintf(out_fp, "<script type=\"text/javascript\" src=\"%swebalizer_%s.js\"></script>\n", html_encode(config.html_js_path.c_str()), html_encode(config.js_charts.c_str()));

   if(config.js_charts_paths.empty()) {
      if(config.js_charts == "highcharts") {
         fputs("<script type=\"text/javascript\" src=\"https://code.highcharts.com/4.2/adapters/standalone-framework.js\"></script>\n", out_fp);
         // link to a Highcharts package within the 4.2 release, so we get bug fixes, but no major changes
         fputs("<script src=\"https://code.highcharts.com/stock/4.2/highstock.js\"></script>\n", out_fp);
      }
   }
   else {
      // output all alternative JavaScript charts paths
      for(std::vector<string_t>::const_iterator i = config.js_charts_paths.begin(); i != config.js_charts_paths.end(); i++)
         fprintf(out_fp, "<script src=\"%s\"></script>\n", html_encode(i->c_str()));
   }

   // output the script block for JavaScripts charts
   fputs("<script type=\"text/javascript\">\n", out_fp);

   if(page_type == page_index)
      write_js_charts_head_index(out_fp);
   else
      write_js_charts_head_usage(out_fp);

   fputs("</script>\n", out_fp);      
}

void html_output_t::write_js_charts_head_js_config(FILE *out_fp)
{
   fputs("   var config = createChartConfig({\n", out_fp);

   //
   // Configuration object versions:
   //
   //   v1  - doesn't exist
   //   v2  - added human-readable variables
   //
   fputs("      version: 2,\n", out_fp);

   //
   // Output custom colors, if any are defined in the configuration. Chart colors 
   // can also be changed in the chart prototype in webalizer.js. Prototype colors 
   // will take effect immediately in all reports that don't override colors via 
   // configuration.
   //
   // Note that ECMAScript 5.1 allows a trailing comma after a property definition.
   //
   if(!graph.is_default_background_color()) fprintf(out_fp, "      background_color: \"#%06X\",\n", graph.get_background_color());
   if(!graph.is_default_gridline_color()) fprintf(out_fp, "      gridline_color: \"#%06X\",\n", graph.get_gridline_color());
   if(!graph.is_default_title_color()) fprintf(out_fp, "      title_color: \"#%06X\",\n", graph.get_title_color());
   if(!graph.is_default_hits_color()) fprintf(out_fp, "      hits_color: \"#%06X\",\n", graph.get_hits_color());
   if(!graph.is_default_files_color()) fprintf(out_fp, "      files_color: \"#%06X\",\n", graph.get_files_color());
   if(!graph.is_default_pages_color()) fprintf(out_fp, "      pages_color: \"#%06X\",\n", graph.get_pages_color());
   if(!graph.is_default_visits_color()) fprintf(out_fp, "      visits_color: \"#%06X\",\n", graph.get_visits_color());
   if(!graph.is_default_hosts_color()) fprintf(out_fp, "      hosts_color: \"#%06X\",\n", graph.get_hosts_color());
   if(!graph.is_default_xfer_color()) fprintf(out_fp, "      xfer_color: \"#%06X\",\n", graph.get_xfer_color());
   if(!graph.is_default_weekend_color()) fprintf(out_fp, "      weekend_color: \"#%06X\",\n", graph.get_weekend_color());

   // output unit prefixes, transfer unit and whether one KB is 1000 or 1024 bytes
   fputs("      unit_prefix: [", out_fp);
   for(size_t i = 0; i < sizeof(config.lang.msg_unit_pfx) / sizeof(config.lang.msg_unit_pfx[0]); i++) {
      if(i) fputs(", ", out_fp);
      fprintf(out_fp, "\"%s\"", js_encode(config.lang.msg_unit_pfx[i]));
   }
   fputs("],\n", out_fp);
      
   fprintf(out_fp, "      xfer_unit: \"%s\",\n", js_encode(config.lang.msg_xfer_unit));
   fprintf(out_fp, "      decimal_kbytes: %s,\n", config.decimal_kbytes ? "true" : "false");
   fprintf(out_fp, "      classic_kbytes: %s,\n", config.classic_kbytes ? "true" : "false");

   // output the language code for this report
   fprintf(out_fp, "      lang: \"%s\"\n", js_encode(config.lang.language_code));

   fputs("   });\n\n", out_fp);
}

void html_output_t::write_js_charts_head_index(FILE *out_fp)
{
   fputs("function onLoadIndexPage()\n", out_fp); 
   fputs("{\n", out_fp);

   write_js_charts_head_js_config(out_fp);

   fputs("   setupCharts(config);\n\n", out_fp);

   //
   // Monthly Summary Chart
   //
   // Versions:
   //    v2    - added the data-xfer attribute
   //
   fputs("   var monthly_summary_chart = new MonthlySummaryChart(2, config, {\n", out_fp);

   buffer_formatter.set_scope_mode(buffer_formatter_t::append),
   fprintf(out_fp, "      title: \"%s %s\",\n", js_encode(config.lang.msg_main_us), js_encode(config.hname.c_str()));

   fputs("      shortMonths: [", out_fp);
   for(u_int i = 0; i < sizeof(lang_t::s_month)/sizeof(lang_t::s_month[0]); i++) {
      if(i) fputs(", ", out_fp);
      fprintf(out_fp, "\"%s\"", js_encode(lang_t::s_month[i]));
   }
   fputs("],\n", out_fp);

   fputs("      longMonths: [", out_fp);
   for(u_int i = 0; i < sizeof(lang_t::l_month)/sizeof(lang_t::l_month[0]); i++) {
      if(i) fputs(", ", out_fp);
      fprintf(out_fp, "\"%s\"", js_encode(lang_t::l_month[i]));
   }
   fputs("],\n", out_fp);

   fprintf(out_fp, "      monthCount: %u,\n", state.history.disp_length());
   fprintf(out_fp, "      firstMonth: {year: %u, month: %u},\n", state.history.first()->year, state.history.first()->month);

   fputs("      seriesNames: {\n", out_fp);
   fprintf(out_fp, "         hits: \"%s\",\n", js_encode(config.lang.msg_h_hits)); 
   fprintf(out_fp, "         files: \"%s\",\n", js_encode(config.lang.msg_h_files));
   fprintf(out_fp, "         pages: \"%s\",\n", js_encode(config.lang.msg_h_pages));
   fprintf(out_fp, "         xfer: \"%s\",\n", js_encode(config.lang.msg_h_xfer));
   fprintf(out_fp, "         visits: \"%s\",\n", js_encode(config.lang.msg_h_visits));
   fprintf(out_fp, "         hosts: \"%s\"\n", js_encode(config.lang.msg_h_hosts));
   fputs("      }\n", out_fp);
   fputs("   });\n\n", out_fp);

   fputs("   renderMonthlySummaryChart(monthly_summary_chart);\n", out_fp);
   fputs("}\n", out_fp);
}

void html_output_t::write_js_charts_head_usage(FILE *out_fp)
{
   fputs("function onLoadUsagePage()\n", out_fp); 
   fputs("{\n", out_fp);

   write_js_charts_head_js_config(out_fp);

   fputs("   setupCharts(config);\n\n", out_fp);

   //
   // Daily usage chart
   //
   // Versions:
   //    v2    - added the data-xfer attribute
   //
   u_int last_day = state.totals.cur_tstamp.last_month_day();
   fputs("   var daily_usage_chart = new DailyUsageChart(2, config, {\n", out_fp);

   buffer_formatter.set_scope_mode(buffer_formatter_t::append),
   fprintf(out_fp, "      title: \"%s %s %d\",\n", js_encode(config.lang.msg_hmth_du), js_encode(lang_t::l_month[state.totals.cur_tstamp.month-1]), state.totals.cur_tstamp.year);

   fprintf(out_fp, "      maxDay: %d,\n", last_day);

   //
   // Use ISO week day numbering to compute the number of days to the next
   // Saturday because Saturday (6) and Sunday (7) are next to each other
   // and we don't have to use use modulo division. The result may be -1 if 
   // the 1st is a Sunday.
   //
   fputs("      weekends: [", out_fp);
   int to_sat = 6 - tstamp_t::wday_iso(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, 1);
   for(u_int i = 1+to_sat; i <= last_day; i += 7) {
      // 1st Saturday will be in the [0-6] range
      if(i > 6)
         fputs(", ", out_fp);

      //
      // If both, Sat (i) and Sun (i+1) are within the month, print both
      // numbers. Otherwise, print either Sun (1st of the month) or Sat
      // (last day of the month).
      //
      if(i > 0 && i < last_day)
         fprintf(out_fp, "%d, %d", i, i+1);
      else
         fprintf(out_fp, "%d", i == 0 ? i+1 : i);
   }
   fputs("],\n", out_fp);

   fputs("      seriesNames: {\n", out_fp);
   fprintf(out_fp, "         hits: \"%s\",\n", js_encode(config.lang.msg_h_hits)); 
   fprintf(out_fp, "         files: \"%s\",\n", js_encode(config.lang.msg_h_files));
   fprintf(out_fp, "         pages: \"%s\",\n", js_encode(config.lang.msg_h_pages));
   fprintf(out_fp, "         xfer: \"%s\",\n", js_encode(config.lang.msg_h_xfer));
   fprintf(out_fp, "         visits: \"%s\",\n", js_encode(config.lang.msg_h_visits));
   fprintf(out_fp, "         hosts: \"%s\"\n", js_encode(config.lang.msg_h_hosts));
   fputs("      }\n", out_fp);
   fputs("   });\n\n", out_fp);

   fputs("   renderDailyUsageChart(daily_usage_chart);\n\n", out_fp);

   //
   // Hourly usage chart
   //
   // Versions:
   //    v2    - added the data-xfer attribute
   //
   fputs("   var hourly_usage_chart = new HourlyUsageChart(2, config, {\n", out_fp);

   buffer_formatter.set_scope_mode(buffer_formatter_t::append),
   fprintf(out_fp, "      title: \"%s %s %d\",\n", js_encode(config.lang.msg_hmth_hu), js_encode(lang_t::l_month[state.totals.cur_tstamp.month-1]), state.totals.cur_tstamp.year);

   fputs("      seriesNames: {\n", out_fp);
   fprintf(out_fp, "         hits: \"%s\",\n", js_encode(config.lang.msg_h_hits)); 
   fprintf(out_fp, "         files: \"%s\",\n", js_encode(config.lang.msg_h_files));
   fprintf(out_fp, "         pages: \"%s\",\n", js_encode(config.lang.msg_h_pages));
   fprintf(out_fp, "         xfer: \"%s\"\n", js_encode(config.lang.msg_h_xfer));
   fputs("      }\n", out_fp);
   fputs("   });\n\n", out_fp);

   fputs("   renderHourlyUsageChart(hourly_usage_chart);\n\n", out_fp);

   //
   // Country usage chart
   //
   // Versions:
   //    v2    - added a column for page counts
   //    v3    - added the data-xfer attribute
   //
   fputs("   var country_usage_chart = new CountryUsageChart(3, config, {\n", out_fp);

   buffer_formatter.set_scope_mode(buffer_formatter_t::append),
   fprintf(out_fp, "     title: \"%s %s %d\",\n", js_encode(config.lang.msg_ctry_use), js_encode(lang_t::l_month[state.totals.cur_tstamp.month-1]), state.totals.cur_tstamp.year);

   fprintf(out_fp, "     totalVisits: %" PRIu64 ",\n", state.totals.t_hvisits_end);
   fprintf(out_fp, "     otherLabel : \"%s\",\n", config.lang.msg_h_other);
   fputs("     seriesNames: {\n", out_fp);
   fprintf(out_fp, "         visits: \"%s\"\n", js_encode(config.lang.msg_h_visits));
   fputs("      }\n", out_fp);
   fputs("   });\n\n", out_fp);

   fputs("   renderCountryUsageChart(country_usage_chart);\n", out_fp);
   fputs("}\n", out_fp);
}

/*********************************************/
/* WRITE_HTML_HEAD - output top of HTML page */
/*********************************************/

void html_output_t::write_html_head(const char *report_title, FILE *out_fp, page_type_t page_type)
{
   list_t<nnode_t>::iterator iter;                 /* used for HTMLhead processing */

   /* HTMLPre code goes before all else    */
   if (config.html_pre.isempty())
   {
      /* Default 'DOCTYPE' header record if none specified */
      fputs("<!DOCTYPE HTML>\n", out_fp);
   }
   else
   {
      iter = config.html_pre.begin(); 
      while(iter.next())
      {
         fprintf(out_fp,"%s\n", iter.item()->string.c_str());
      }
   }

   /* Standard header comments */
   fprintf(out_fp,"<!--  Stone Steps Webalizer  Ver. %s\n\n", state_t::get_app_version().c_str());
   fputs("     Copyright (c) 2004-2017, Stone Steps Inc.\n", out_fp);
   fputs("             http://www.stonesteps.ca\n\n", out_fp);
   fputs("       Based on v2.01.10 of The Webalizer\n", out_fp);
   fputs("     Copyright 1997-2000 Bradford L. Barrett\n", out_fp);
   fputs("     (brad@mrunix.net  http://www.mrunix.net)\n\n", out_fp);
   fputs("     Distributed under the GNU GPL, Version 2\n", out_fp);
   fputs("            Full text may be found at:\n", out_fp);
   fputs("      http://www.stonesteps.ca/legal/gpl.asp\n", out_fp);
   fputs("-->\n\n", out_fp);

   fprintf(out_fp,"<!-- Generated: %s -->\n\n", cur_time(config.local_time).c_str());

   fprintf(out_fp, "<html lang=\"%s\">\n<head>\n", config.lang.language_code);
   fprintf(out_fp,"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
   if(config.html_meta_noindex)
      fputs("<meta name=\"robots\" content=\"noindex,nofollow\">\n", out_fp);
   fprintf(out_fp,"<title>%s %s - %s</title>\n", config.rpt_title.c_str(), config.hname.c_str(), report_title);
   fprintf(out_fp,"<link rel=\"stylesheet\" type=\"text/css\" href=\"%swebalizer.css\">\n", !config.html_css_path.isempty() ? config.html_css_path.c_str() : "");
   if(!config.html_js_path.isempty())
      fprintf(out_fp,"<script type=\"text/javascript\" src=\"%swebalizer.js\"></script>\n", config.html_js_path.c_str());

   if(!config.js_charts.isempty())
      write_js_charts_head(out_fp, page_type);

   iter = config.html_head.begin();
   while(iter.next())
   {
      fprintf(out_fp,"%s\n", iter.item()->string.c_str());
   }
   fputs("</head>\n\n", out_fp);

   if(config.html_body.isempty()) {
      if(config.enable_js) {
         switch (page_type) {
            case page_index:
               if(!config.js_charts.isempty())
                  fputs("<body onload=\"onload_index_page(onLoadIndexPage)\">\n", out_fp);
               else
                  fputs("<body onload=\"onload_index_page()\">\n", out_fp);
               break;
            case page_usage:
               if(!config.js_charts.isempty())
                  fputs("<body onload=\"onload_usage_page(onLoadUsagePage)\">\n", out_fp);
               else
                  fputs("<body onload=\"onload_usage_page()\">\n", out_fp);
               break;
            case page_all_items:
               fputs("<body onload=\"onload_page_all_items()\">\n", out_fp);
               break;
         }
      }
      else
         fputs("<body>\n", out_fp);
   }
   else
   {
      iter = config.html_body.begin();
      while(iter.next())
      {
         fprintf(out_fp,"%s\n", iter.item()->string.c_str());
      }
   }
   
   fputs("\n<a name=\"top\"></a>", out_fp);

   fputs("\n<!-- Page Header -->\n", out_fp);
   fputs("<div class=\"page_header_div\">\n", out_fp);
   fprintf(out_fp,"<h1>%s %s</h1>\n", config.rpt_title.c_str(), config.hname.c_str());
   fprintf(out_fp,"<div class=\"usage_summary_div\">\n<em>%s: %s</em><br>\n",config.lang.msg_hhdr_sp,report_title);
   fprintf(out_fp,"%s %s\n</div>\n",config.lang.msg_hhdr_gt,cur_time(config.local_time).c_str());
   fputs("</div>\n\n", out_fp);

   iter = config.html_post.begin();
   while(iter.next())
   {
      fprintf(out_fp,"%s\n", iter.item()->string.c_str());
   }
}

/*********************************************/
/* WRITE_HTML_TAIL - output HTML page tail   */
/*********************************************/

void html_output_t::write_html_tail(FILE *out_fp)
{
   list_t<nnode_t>::iterator iter;

   fputs("\n<!-- Page Footer -->\n", out_fp);
   fputs("<div class=\"page_footer_div\">\n", out_fp);

   if (!config.html_tail.isempty())
   {
      fputs("<div>", out_fp);
      iter = config.html_tail.begin();
      while(iter.next())
      {
         fprintf(out_fp,"%s",iter.item()->string.c_str());
      }
      fputs("</div>\n", out_fp);
   }

   fprintf(out_fp,"<a href=\"http://www.stonesteps.ca/webalizer\">Stone Steps Webalizer</a> (v%s)\n", state_t::get_app_version().c_str());
   fputs("</div>\n", out_fp);

   /* wind up, this is the end of the file */
   fprintf(out_fp,"\n<!-- Stone Steps Webalizer Version %s -->\n", state_t::get_app_version().c_str());
   if (!config.html_end.isempty())
   {
      iter = config.html_end.begin();
      while(iter.next())
      {
         fprintf(out_fp,"%s\n", iter.item()->string.c_str());
      }
   }
   else 
      fputs("</body>\n</html>\n", out_fp);
}

void html_output_t::write_url_report(void)
{
   // Top URL's (by hits)
   if (config.ntop_urls) 
      top_urls_table(0);

   // Top URL's (by kbytes)
   if (config.ntop_urlsK)
      top_urls_table(1); 

   // Top Entry Pages
   if(config.ntop_entry)
      top_entry_table(0);

   // Top Exit Pages
   if(config.ntop_exit)
      top_entry_table(1);
}

void html_output_t::write_download_report(void)
{
   // top downloads
   if (config.ntop_downloads) 
      top_dl_table();
}

void html_output_t::write_error_report(void)
{
   // top HTTP errors table
   if (config.ntop_errors) 
      top_err_table();
}

void html_output_t::write_host_report(void)
{
   // Top sites table (by hits)
   if (config.ntop_hosts) 
      top_hosts_table(0); 

   // Top Sites table (by kbytes)
   if (config.ntop_hostsK)                      
      top_hosts_table(1);
}

void html_output_t::write_referrer_report(void)
{
   // Top referrers table
   if (config.ntop_refs) 
      top_refs_table();   
}

void html_output_t::write_search_report(void)
{
   // top search strings table
   if (config.ntop_search) 
      top_search_table(); 
}

void html_output_t::write_user_report(void)
{
   // top usernames table
   if (config.ntop_users) 
      top_users_table(); 
}

void html_output_t::write_user_agent_report(void)
{
   // top user agents table
   if (config.ntop_agents) 
      top_agents_table(); 
}

void html_output_t::write_country_report(void)
{
   if(config.ntop_ctrys) 
      top_ctry_table();     /* top countries table            */
}

/*********************************************/
/* WRITE_MONTH_HTML - does what it says...   */
/*********************************************/

int html_output_t::write_monthly_report()
{
   string_t html_fname, html_fname_lang;           /* filename storage areas...       */
   string_t png1_fname;
   string_t png2_fname;
   string_t dtitle, htitle;

   /* fill in filenames */
   html_fname.format("usage_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      html_fname_lang = html_fname + '.' + config.lang.language_code;
   else
      html_fname_lang = html_fname;

   /* now do html stuff... */
   /* first, open the file */
   if ( (out_fp=open_out_file(html_fname_lang))==NULL ) return 1;

   const char *report_title = fmt_printf("%s %d", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year);
   write_html_head(report_title, out_fp, page_usage);

   month_links();

   if(config.monthly_totals_stats)
      month_total_table();

   if (config.daily_graph || config.daily_stats)        /* Daily stuff */
   {
      fputs("\n<div id=\"daily_stats_report\">\n", out_fp);
      fputs("\n<a name=\"daily\"></a>\n", out_fp);
      if (config.daily_graph) {
         if(!config.js_charts.isempty())
            fputs("<div id=\"daily_usage_chart\" class=\"chart_holder\"></div>\n", out_fp);
         else {
            // create daily stats PNG image
            string_t png1_fname_lang;

            png1_fname.format("daily_usage_%04d%02d.png",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

            if(config.html_ext_lang)
               png1_fname_lang = png1_fname + '.' + config.lang.language_code;
            else
               png1_fname_lang = png1_fname;

            dtitle.format("%s %s %d",config.lang.msg_hmth_du, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);

            if(makeimgs)
               graph.month_graph6(png1_fname_lang, dtitle, state.totals.cur_tstamp.month, state.totals.cur_tstamp.year, state.t_daily);

            fprintf(out_fp,"<div id=\"daily_usage_graph\" class=\"graph_holder\"><img src=\"%s\" alt=\"%s\" height=\"400\" width=\"512\"></div>\n", png1_fname.c_str(), dtitle.c_str());
         }
      }

      if (config.daily_stats) 
         fprintf(out_fp,"<p class=\"note_p\">%s</p>\n", config.lang.msg_misc_pages);

      if (config.daily_stats) 
         daily_total_table();

      fputs("</div>\n", out_fp);
   }

   if (config.hourly_graph || config.hourly_stats)      /* Hourly stuff */
   {
      fputs("\n<div id=\"hourly_stats_report\">\n", out_fp);
      fputs("<a name=\"hourly\"></a>\n", out_fp);
      if (config.hourly_graph) { 
         if(!config.js_charts.isempty())
            fputs("<div id=\"hourly_usage_chart\" class=\"chart_holder\"></div>\n", out_fp);
         else {
            // create hourly stats PNG image
            string_t png2_fname_lang;

            png2_fname.format("hourly_usage_%04d%02d.png",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

            if(config.html_ext_lang)
               png2_fname_lang = png2_fname + '.' + config.lang.language_code;
            else
               png2_fname_lang = png2_fname;

            htitle.format("%s %s %d", config.lang.msg_hmth_hu, lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year);

            if(makeimgs)
               graph.day_graph3(png2_fname_lang, htitle, state.t_hourly);

            fprintf(out_fp,"<div id=\"hourly_usage_graph\" class=\"graph_holder\"><img src=\"%s\" alt=\"%s\" height=\"340\" width=\"512\"></div>\n", png2_fname.c_str(), htitle.c_str());
         }
      }

      if (config.hourly_stats) hourly_total_table();
      fputs("</div>\n", out_fp);
   }

   write_url_report();

   if(config.log_type == LOG_SQUID)
      write_search_report();

   write_download_report();
   write_error_report();
   write_host_report();
   write_referrer_report();

   if(config.log_type != LOG_SQUID)
      write_search_report();

   write_user_report();
   write_user_agent_report();
   write_country_report();

   write_html_tail(out_fp);               /* finish up the HTML document    */
   fclose(out_fp);                        /* close the file                 */

   return (0);                            /* done...                        */
}

/*********************************************/
/* MONTH_LINKS - links to other page parts   */
/*********************************************/

void html_output_t::month_links()
{
   string_t onclick;

   fputs("<table id=\"main_menu\" class=\"page_links_table\"><tr>\n", out_fp);
   
   if (config.daily_stats || config.daily_graph)
      fprintf(out_fp,"<td><a href=\"#daily\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_ds);
   if (config.hourly_stats || config.hourly_graph)
      fprintf(out_fp,"<td><a href=\"#hourly\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_hs);
   if (config.ntop_urls || config.ntop_urlsK)
      fprintf(out_fp,"<td><a href=\"#urls\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_u);
   if (config.ntop_entry)
      fprintf(out_fp,"<td><a href=\"#entry\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_en);
   if (config.ntop_exit)
      fprintf(out_fp,"<td><a href=\"#exit\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_ex);
   if(config.log_type == LOG_SQUID && config.ntop_search && state.totals.t_srchits)
      fprintf(out_fp,"<td><a href=\"#search\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_sr);
   if (config.ntop_downloads && state.totals.t_downloads)
      fprintf(out_fp,"<td><a href=\"#downloads\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_dl);
   if (config.ntop_errors && state.totals.t_err)
      fprintf(out_fp,"<td><a href=\"#errors\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_err);
   if (config.ntop_hosts || config.ntop_hostsK)
      fprintf(out_fp,"<td><a href=\"#hosts\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_s);
   if (config.ntop_refs && state.totals.t_ref)
      fprintf(out_fp,"<td><a href=\"#referrers\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_r);
   if(config.log_type != LOG_SQUID && config.ntop_search && state.totals.t_srchits)
      fprintf(out_fp,"<td><a href=\"#search\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_sr);
   if (config.ntop_users && state.totals.t_user)
      fprintf(out_fp,"<td><a href=\"#users\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_i);
   if (config.ntop_agents && state.totals.t_agent)
      fprintf(out_fp,"<td><a href=\"#useragents\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_a);
   if (config.ntop_ctrys)
      fprintf(out_fp,"<td><a href=\"#countries\"%s>%s</a></td>\n", onclick.c_str(), config.lang.msg_hlnk_c);

   fputs("</tr></table>\n", out_fp);
}

/*********************************************/
/* MONTH_TOTAL_TABLE - monthly totals table  */
/*********************************************/

void html_output_t::month_total_table()
{
   u_int i, days_in_month;
   uint64_t max_files=0,max_hits=0,max_visits=0,max_pages=0;
   uint64_t max_xfer=0;

   days_in_month=(state.totals.l_day-state.totals.f_day)+1;
   for (i=0;i<31;i++)
   {  /* Get max/day values */
      if (state.t_daily[i].tm_hits > max_hits)     max_hits  = state.t_daily[i].tm_hits;
      if (state.t_daily[i].tm_files > max_files)   max_files = state.t_daily[i].tm_files;
      if (state.t_daily[i].tm_pages > max_pages)   max_pages = state.t_daily[i].tm_pages;
      if (state.t_daily[i].tm_visits > max_visits) max_visits= state.t_daily[i].tm_visits;
      if (state.t_daily[i].tm_xfer > max_xfer)     max_xfer  = state.t_daily[i].tm_xfer;
   }

   fputs("\n<!-- Monthly Totals Table -->\n", out_fp);
   fputs("\n<a name=\"totals\"></a>\n", out_fp);
   fputs("\n<div id=\"monthly_totals_report\">\n", out_fp);
   fputs("<table class=\"report_table monthly_totals_table\">\n", out_fp);
   fputs("<colgroup><col><col span=\"2\" class=\"totals_data_col\"></colgroup>\n", out_fp);

   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"3\">%s %s %d</th></tr>\n", config.lang.msg_mtot_ms, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
   /* Total Hits */
   fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_th, state.totals.t_hit);
   /* Total Files */
   fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tf, state.totals.t_file);
   /* Total Pages */
   fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tp, state.totals.t_page);
   /* Total Visits */
   fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tv, state.totals.t_visits);
   /* Total XFer */
   fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\" data-xfer=\"%" PRIu64 "\">%s</td></tr>\n", config.lang.msg_mtot_tx, state.totals.t_xfer, fmt_xfer(state.totals.t_xfer));
   /* Total Downloads */
   if(state.totals.t_downloads)
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_dl, state.totals.t_downloads);

   /**********************************************/

   /* Unique Hosts */
   fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_us, state.totals.t_hosts);
   /* Unique URL's */
   fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_uu, state.totals.t_url);
   /* Unique Referrers */
   if (state.totals.t_ref != 0)
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_ur, state.totals.t_ref);
   /* Unique Usernames */
   if (state.totals.t_user != 0)
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_ui, state.totals.t_user);
   /* Unique Agents */
   if (state.totals.t_agent != 0)
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_ua, state.totals.t_agent);
   fputs("</tbody>\n", out_fp);

   // output human totals if robot or spammer filters are configured
   if(config.spam_refs.size() || config.robots.size()) {
      fputs("<tbody class=\"totals_header_tbody\">\n", out_fp);
      fprintf(out_fp,"<tr><th colspan=\"3\">%s</th></tr>\n", config.lang.msg_mtot_htot);
      fputs("</tbody>\n", out_fp);

      fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
      
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_th, state.totals.t_hit - state.totals.t_rhits - state.totals.t_spmhits);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tf, state.totals.t_file - state.totals.t_rfiles - state.totals.t_sfiles);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tp, state.totals.t_page - state.totals.t_rpages - state.totals.t_spages);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\" data-xfer=\"%" PRIu64 "\">%s</td></tr>\n\n", config.lang.msg_mtot_tx, state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer, fmt_xfer(state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer));

      /* Total Non-Robot Hosts */
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n\n", config.lang.msg_mtot_us, state.totals.t_hosts - state.totals.t_rhosts - state.totals.t_shosts);

      // Total Human Visits
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n\n", config.lang.msg_mtot_tv, state.totals.t_hvisits_end);

      // output the conversion section only if target URLs or downloads are configured
      if(config.target_urls.size() || config.downloads.size()) {
         /* Unique Converted Hosts */
         fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tch, state.totals.t_hosts_conv);
         /* Total Converted Visits */
         fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tcv, state.totals.t_visits_conv);
         /* Host Conversion Rate */
         fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%.2f</td></tr>\n", config.lang.msg_mtot_hcr, (double)state.totals.t_hosts_conv*100./(state.totals.t_hosts - state.totals.t_rhosts - state.totals.t_shosts));
      }
      
      fputs("</tbody>\n", out_fp);

      // output human per-visit totals if there are ended human visits
      if(state.totals.t_hvisits_end) {
         fputs("<tbody class=\"totals_header_tbody\">\n", out_fp);
         fprintf(out_fp,"<tr><th>&nbsp;</th><td>%s</td><td>%s</td></tr>\n", config.lang.msg_h_avg, config.lang.msg_h_max);
         fputs("</tbody>\n", out_fp);

         fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
         
         fprintf(out_fp,"<tr><th>%s</th><td>%" PRIu64 "</td><td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mhv, (state.totals.t_hit - state.totals.t_rhits - state.totals.t_spmhits)/state.totals.t_hvisits_end, state.totals.max_hv_hits);
         fprintf(out_fp,"<tr><th>%s</th><td>%" PRIu64 "</td><td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mfv, (state.totals.t_file - state.totals.t_rfiles - state.totals.t_sfiles)/state.totals.t_hvisits_end, state.totals.max_hv_files);
         fprintf(out_fp,"<tr><th>%s</th><td>%" PRIu64 "</td><td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mpv, (state.totals.t_page - state.totals.t_rpages - state.totals.t_spages)/state.totals.t_hvisits_end, state.totals.max_hv_pages);

         buffer_formatter.set_scope_mode(buffer_formatter_t::append),
         fprintf(out_fp,"<tr><th>%s</th><td data-xfer=\"%" PRIu64 "\">%s</td><td data-xfer=\"%" PRIu64 "\">%s</td></tr>\n", config.lang.msg_mtot_mkv, 
            (state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer)/state.totals.t_hvisits_end, 
            fmt_xfer((state.totals.t_xfer - state.totals.t_rxfer - state.totals.t_sxfer)/state.totals.t_hvisits_end), 
            state.totals.max_hv_xfer,
            fmt_xfer(state.totals.max_hv_xfer));
         
         fprintf(out_fp,"<tr><th>%s</th><td>%.02f</td><td>%.02f</td></tr>\n", config.lang.msg_mtot_mdv, state.totals.t_visit_avg/60., state.totals.t_visit_max/60.);

         if(state.totals.t_visits_conv)
            fprintf(out_fp,"<tr><th>%s</th><td>%.02f</td><td>%.02f</td></tr>\n", config.lang.msg_mtot_cvd, state.totals.t_vconv_avg/60., state.totals.t_vconv_max/60.);
            
         fputs("</tbody>\n", out_fp);
      }
   }

   /**********************************************/

   // Robot Totals
   if(state.totals.t_rhits) {
      fputs("<tbody class=\"totals_header_tbody\">\n", out_fp);
      fprintf(out_fp,"<tr><th colspan=\"3\">%s</th></tr>\n", config.lang.msg_mtot_rtot);
      fputs("</tbody>\n", out_fp);

      fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_th, state.totals.t_rhits);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tf, state.totals.t_rfiles);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tp, state.totals.t_rpages);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_terr, state.totals.t_rerrors);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\" data-xfer=\"%" PRIu64 "\">%s</td></tr>\n", config.lang.msg_mtot_tx, state.totals.t_rxfer, fmt_xfer(state.totals.t_rxfer));
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tv, state.totals.t_rvisits_end);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_us, state.totals.t_rhosts);
      fputs("</tbody>\n", out_fp);
   }

   // Spammer Totals
   if(state.totals.t_spmhits) {
      fputs("<tbody class=\"totals_header_tbody\">\n", out_fp);
      fprintf(out_fp,"<tr><th colspan=\"3\">%s</th></tr>\n", config.lang.msg_mtot_stot);
      fputs("</tbody>\n", out_fp);

      fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_th, state.totals.t_spmhits);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\" data-xfer=\"%" PRIu64 "\">%s</td></tr>\n", config.lang.msg_mtot_tx, state.totals.t_sxfer, fmt_xfer(state.totals.t_sxfer));
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_tv, state.totals.t_svisits_end);
      fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_us, state.totals.t_shosts);
      fputs("</tbody>\n", out_fp);
   }
      
   /**********************************************/

   /* Hit/file/page processing time (output only if there's data) */
   if(state.totals.m_hitptime) {
      fputs("<tbody class=\"totals_header_tbody\">\n", out_fp);
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%s</td>\n<td>%s</td></tr>\n", config.lang.msg_mtot_perf, config.lang.msg_h_avg, config.lang.msg_h_max);
      fputs("</tbody>\n", out_fp);

      fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%.3f</td>\n<td>%.3f</td></tr>\n", config.lang.msg_mtot_sph, state.totals.a_hitptime, state.totals.m_hitptime);
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%.3f</td>\n<td>%.3f</td></tr>\n", config.lang.msg_mtot_spf, state.totals.a_fileptime, state.totals.m_fileptime);
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%.3f</td>\n<td>%.3f</td></tr>\n", config.lang.msg_mtot_spp, state.totals.a_pageptime, state.totals.m_pageptime);
      fputs("</tbody>\n", out_fp);
   }

   /* Hourly/Daily avg/max totals */
   fputs("<tbody class=\"totals_header_tbody\">\n", out_fp);
   fprintf(out_fp,"<tr><th>%s</th>\n<td>%s</td>\n<td>%s</td></tr>\n", config.lang.msg_mtot_hdt, config.lang.msg_h_avg, config.lang.msg_h_max);
   fputs("</tbody>\n", out_fp);

   fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);

   /* Max/Avg Hits per Hour */
   fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mhh, state.totals.t_hit/(24*days_in_month), state.totals.hm_hit);
   /* Max/Avg Hits per Day */
   fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mhd, state.totals.t_hit/days_in_month, max_hits);
   /* Max/Avg Hits per Visit */
   if(state.totals.t_visits)
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mhv, state.totals.t_hit/state.totals.t_visits, state.totals.max_v_hits);

   /* Max/Avg Files per Day */
   fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mfd, state.totals.t_file/days_in_month, max_files);
   /* Max/Avg Files per Visit */
   if(state.totals.t_visits)
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mfv, state.totals.t_file/state.totals.t_visits, state.totals.max_v_files);

   /* Max/Avg Pages per Day */
   fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mpd, state.totals.t_page/days_in_month, max_pages);
   /* Max/Avg Pages per Visit */
   if(state.totals.t_visits)
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mpv, state.totals.t_page/state.totals.t_visits, state.totals.max_v_pages);

   /* Max/Avg Visits per Day */
   fprintf(out_fp,"<tr><th>%s</th>\n<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td></tr>\n", config.lang.msg_mtot_mvd, state.totals.t_visits/days_in_month, max_visits);
   fprintf(out_fp,"<tr><th>%s</th>\n<td>%.02f</td>\n<td>%.02f</td></tr>\n", config.lang.msg_mtot_mdv, state.totals.t_visit_avg/60., state.totals.t_visit_max/60.);
   if(state.totals.t_visits_conv)
      fprintf(out_fp,"<tr><th>%s</th>\n<td>%.02f</td>\n<td>%.02f</td></tr>\n", config.lang.msg_mtot_cvd, state.totals.t_vconv_avg/60., state.totals.t_vconv_max/60.);

   /* Max/Avg Transfer per Day */
   buffer_formatter.set_scope_mode(buffer_formatter_t::append),
   fprintf(out_fp,"<tr><th>%s</th>\n<td data-xfer=\"%" PRIu64 "\">%s</td>\n<td data-xfer=\"%" PRIu64 "\">%s</td></tr>\n", 
      config.lang.msg_mtot_mkd, 
      state.totals.t_xfer/days_in_month,
      fmt_xfer(state.totals.t_xfer/days_in_month),
      max_xfer,
      fmt_xfer(max_xfer));

   /* Max/Avg KBytes per Visit */
   if(state.totals.t_visits) {
      buffer_formatter.set_scope_mode(buffer_formatter_t::append),
      fprintf(out_fp,"<tr><th>%s</th>\n<td data-xfer=\"%" PRIu64 "\">%s</td>\n<td data-xfer=\"%" PRIu64 "\">%s</td></tr>\n", 
         config.lang.msg_mtot_mkv, 
         (uint64_t) ((double) state.totals.t_xfer / (double) state.totals.t_visits + .5), 
         fmt_xfer((uint64_t) ((double) state.totals.t_xfer / (double) state.totals.t_visits + .5)), 
         state.totals.max_v_xfer,
         fmt_xfer(state.totals.max_v_xfer));
   }

   fputs("</tbody>\n", out_fp);

   /**********************************************/
   /* response code totals */
   fputs("<tbody class=\"totals_header_tbody\">\n", out_fp);
   fprintf(out_fp,"<tr><th colspan=\"3\">%s</th></tr>\n", config.lang.msg_mtot_rc);
   fputs("</tbody>\n", out_fp);

   fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
   for (i=0; i < state.response.size(); i++) {
      if (state.response[i].count != 0)
         fprintf(out_fp,"<tr><th>%s</th>\n<td colspan=\"2\">%" PRIu64 "</td></tr>\n", config.lang.get_resp_code(state.response[i].get_scode()).desc, state.response[i].count);
   }
   fputs("</tbody>\n", out_fp);
   fputs("</table>\n", out_fp);
   fprintf(out_fp,"<p class=\"note_p\">%s</p>", config.lang.msg_misc_visitors);
   fputs("</div>\n", out_fp);
}

/*********************************************/
/* DAILY_TOTAL_TABLE - daily totals          */
/*********************************************/

void html_output_t::daily_total_table()
{
   u_int wday;
   int i;
   const hist_month_t *hptr;

   if((hptr = state.history.find_month(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month)) == NULL)
      return;
      
   wday = tstamp_t::wday(state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, 1);

   /* Daily stats */
   fputs("\n<!-- Daily Totals Table -->\n", out_fp);
   fputs("<table id=\"daily_usage_table\" class=\"report_table totals_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   /* Daily statistics for ... */
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"25\">%s %s %d</th></tr>\n", config.lang.msg_dtot_ds, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);

   fprintf(out_fp,"<tr><th rowspan=\"2\" class=\"counter_th\">%s</th>\n"                        \
                  "<th class=\"hits_th\" colspan=\"4\">%s</th>\n"                  \
                  "<th class=\"files_th\" colspan=\"4\">%s</th>\n"                \
                  "<th class=\"pages_th\" colspan=\"4\">%s</th>\n"                \
                  "<th class=\"visits_th\" colspan=\"4\">%s</th>\n"               \
                  "<th class=\"hosts_th\" colspan=\"4\">%s</th>\n"                \
                  "<th class=\"kbytes_th\" colspan=\"4\">%s</th></tr>\n",
                  config.lang.msg_h_day,
                  config.lang.msg_h_hits,
                  config.lang.msg_h_files,
                  config.lang.msg_h_pages,
                  config.lang.msg_h_visits,
                  config.lang.msg_h_hosts,
                  config.lang.msg_h_xfer);

   fputs("<tr>\n", out_fp);
   
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th small_font_th\">%s</th>\n<th class=\"hits_th small_font_th\">%s</th>\n<th class=\"hits_th small_font_th\">%s</th>\n", config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp,"<th colspan=\"2\" class=\"files_th small_font_th\">%s</th>\n<th class=\"files_th small_font_th\">%s</th>\n<th class=\"files_th small_font_th\">%s</th>\n", config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp,"<th colspan=\"2\" class=\"pages_th small_font_th\">%s</th>\n<th class=\"pages_th small_font_th\">%s</th>\n<th class=\"pages_th small_font_th\">%s</th>\n", config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th small_font_th\">%s</th>\n<th class=\"visits_th small_font_th\">%s</th>\n<th class=\"visits_th small_font_th\">%s</th>\n", config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hosts_th small_font_th\">%s</th>\n<th class=\"hosts_th small_font_th\">%s</th>\n<th class=\"hosts_th small_font_th\">%s</th>\n", config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);
   fprintf(out_fp,"<th colspan=\"2\" class=\"kbytes_th small_font_th\">%s</th>\n<th class=\"kbytes_th small_font_th\">%s</th>\n<th class=\"kbytes_th small_font_th\">%s</th>\n", config.lang.msg_h_total, config.lang.msg_h_avg, config.lang.msg_h_max);

   fputs("</tr>\n", out_fp);
   fputs("</thead>\n", out_fp);

   /* skip beginning blank days in a month */
   for (i=0; i < hptr->lday; i++) {
      if (state.t_daily[i].tm_hits != 0)
         break;
   }

   if(i == hptr->lday)
      i=0;

   fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);
   for (; i < hptr->lday; i++) {
      fprintf(out_fp,"<tr%s><th>%d</th>\n", ((wday + i) % 7 == 6 || (wday + i) % 7 == 0) ? " class=\"weekend_tr\"" : "", i+1);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n<td>%.0f</td>\n<td>%" PRIu64 "</td>\n", state.t_daily[i].tm_hits, PCENT(state.t_daily[i].tm_hits, state.totals.t_hit), state.t_daily[i].h_hits_avg, state.t_daily[i].h_hits_max);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n<td>%.0f</td>\n<td>%" PRIu64 "</td>\n", state.t_daily[i].tm_files, PCENT(state.t_daily[i].tm_files, state.totals.t_file), state.t_daily[i].h_files_avg, state.t_daily[i].h_files_max);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n<td>%.0f</td>\n<td>%" PRIu64 "</td>\n", state.t_daily[i].tm_pages, PCENT(state.t_daily[i].tm_pages, state.totals.t_page), state.t_daily[i].h_pages_avg, state.t_daily[i].h_pages_max);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n<td>%.0f</td>\n<td>%" PRIu64 "</td>\n", state.t_daily[i].tm_visits, PCENT(state.t_daily[i].tm_visits, state.totals.t_visits), state.t_daily[i].h_visits_avg, state.t_daily[i].h_visits_max);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n<td>%.0f</td>\n<td>%" PRIu64 "</td>\n", state.t_daily[i].tm_hosts, PCENT(state.t_daily[i].tm_hosts, state.totals.t_hosts), state.t_daily[i].h_hosts_avg, state.t_daily[i].h_hosts_max);

      buffer_formatter.set_scope_mode(buffer_formatter_t::append),
      fprintf(out_fp,"<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
         "<td class=\"data_percent_td\">%3.02f%%</td>\n"
         "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
         "<td data-xfer=\"%" PRIu64 "\">%s</td>\n", 
         state.t_daily[i].tm_xfer,
         fmt_xfer(state.t_daily[i].tm_xfer), 
         PCENT(state.t_daily[i].tm_xfer, state.totals.t_xfer), 
         (uint64_t) (state.t_daily[i].h_xfer_avg + .5), 
         fmt_xfer((uint64_t) (state.t_daily[i].h_xfer_avg + .5)), 
         state.t_daily[i].h_xfer_max,
         fmt_xfer(state.t_daily[i].h_xfer_max));

      fputs("</tr>\n", out_fp);
   }
   fputs("</tbody>\n", out_fp); 
   fputs("</table>\n", out_fp);
}

/*********************************************/
/* HOURLY_TOTAL_TABLE - hourly table         */
/*********************************************/

void html_output_t::hourly_total_table()
{
   int i,days_in_month;

   days_in_month=(state.totals.l_day-state.totals.f_day)+1;

   /* Hourly stats */
   fputs("\n<!-- Hourly Totals Table -->\n", out_fp);
   fputs("<table id=\"hourly_usage_table\" class=\"report_table totals_table\">\n", out_fp);

   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"13\">%s %s %d</th></tr>\n", config.lang.msg_htot_hs, lang_t::l_month[state.totals.cur_tstamp.month-1], state.totals.cur_tstamp.year);

   fprintf(out_fp,"<tr><th rowspan=\"2\" class=\"counter_th\">%s</th>\n"   \
                  "<th colspan=\"3\" class=\"hits_th\">%s</th>\n"            \
                  "<th colspan=\"3\" class=\"files_th\">%s</th>\n"         \
                  "<th colspan=\"3\" class=\"pages_th\">%s</th>\n"         \
                  "<th colspan=\"3\" class=\"kbytes_th\">%s</th></tr>\n",
                  config.lang.msg_h_hour,
                  config.lang.msg_h_hits,
                  config.lang.msg_h_files,
                  config.lang.msg_h_pages,
                  config.lang.msg_h_xfer);
   fprintf(out_fp,"<tr><th class=\"hits_th small_font_th\">%s</th>\n<th colspan=\"2\" class=\"hits_th small_font_th\">%s</th>\n", config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<th class=\"files_th small_font_th\">%s</th>\n<th colspan=\"2\" class=\"files_th small_font_th\">%s</th>\n", config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<th class=\"pages_th small_font_th\">%s</th>\n<th colspan=\"2\" class=\"pages_th small_font_th\">%s</th>\n", config.lang.msg_h_avg, config.lang.msg_h_total);
   fprintf(out_fp,"<th class=\"kbytes_th small_font_th\">%s</th>\n<th colspan=\"2\" class=\"kbytes_th small_font_th\">%s</th></tr>\n", config.lang.msg_h_avg, config.lang.msg_h_total);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"totals_data_tbody\">\n", out_fp);

   for (i=0;i<24;i++)
   {
      fprintf(out_fp,"<tr><th>%d</th>\n", i);
      fprintf(out_fp, "<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n", state.t_hourly[i].th_hits/days_in_month, state.t_hourly[i].th_hits, PCENT(state.t_hourly[i].th_hits, state.totals.t_hit));
      fprintf(out_fp, "<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n", state.t_hourly[i].th_files/days_in_month, state.t_hourly[i].th_files, PCENT(state.t_hourly[i].th_files, state.totals.t_file));
      fprintf(out_fp, "<td>%" PRIu64 "</td>\n<td>%" PRIu64 "</td>\n<td class=\"data_percent_td\">%3.02f%%</td>\n", state.t_hourly[i].th_pages/days_in_month, state.t_hourly[i].th_pages, PCENT(state.t_hourly[i].th_pages, state.totals.t_page));

      buffer_formatter.set_scope_mode(buffer_formatter_t::append),
      fprintf(out_fp, "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
         "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
         "<td class=\"data_percent_td\">%3.02f%%</td></tr>\n", 
         (uint64_t) (((double)state.t_hourly[i].th_xfer/days_in_month) + .5), 
         fmt_xfer((uint64_t) (((double)state.t_hourly[i].th_xfer/days_in_month) + .5)), 
         state.t_hourly[i].th_xfer, 
         fmt_xfer(state.t_hourly[i].th_xfer), 
         PCENT(state.t_hourly[i].th_xfer, state.totals.t_xfer));
   }

   fputs("</tbody>\n", out_fp); 
   fputs("</table>\n", out_fp);
}

/*********************************************/
/* TOP_SITES_TABLE - generate top n table    */
/*********************************************/

void html_output_t::top_hosts_table(int flag)
{
   uint64_t a_ctr;
   u_int i;
   uint32_t tot_num, ntop_num;
   const hnode_t *hptr;
   const char *cdesc = NULL;
   hnode_t *h_array;

   // return if nothing to process
   if (state.totals.t_hosts == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_hosts + state.totals.t_grp_hosts) == 0)
      return;

   /* get max to do... */
   ntop_num = (flag) ? config.ntop_hostsK : config.ntop_hosts;
   tot_num = (a_ctr > ntop_num) ? ntop_num : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   h_array = new hnode_t[tot_num];

   i = 0;

   // for the hits report, if groups are bundled, put them first
   if(!flag && config.bundle_groups) {
      database_t::reverse_iterator<hnode_t> iter = state.database.rbegin_hosts(flag ? "hosts.groups.xfer" : "hosts.groups.hits");

      while(i < tot_num && iter.prev(h_array[i]))
         i++;

      iter.close();
   }

   // populate the remainder of the array
   if(i < tot_num) {
      database_t::reverse_iterator<hnode_t> iter = state.database.rbegin_hosts(flag ? "hosts.xfer" : "hosts.hits");

      while(i < tot_num && iter.prev(h_array[i])) {
         if(h_array[i].flag == OBJ_REG) {
            // ignore hosts matching any of the hiding patterns
            if(config.hide_hosts || h_array[i].robot && config.hide_robots || config.hidden_hosts.isinlist(h_array[i].string) || config.hidden_hosts.isinlist(h_array[i].name))
               continue;
         }
         else if(h_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }

         i++;
      }

      iter.close();
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] h_array;
      return;
   }

   // adjust array size if it's not filled up
   if(i < tot_num)
      tot_num = (uint32_t) i;

   fputs("\n<!-- Top Hosts Table -->\n", out_fp);

   if(!flag)
      fputs("<div id=\"top_hosts_report\">\n", out_fp);
   else
      fputs("<div id=\"top_hosts_kbytes_report\">\n", out_fp);

   if(!flag || (flag && !config.ntop_hosts))                  /* now do <a> tag   */
      fputs("<a name=\"hosts\"></a>\n", out_fp);

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   if (flag) 
      fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"%d\">%s %u %s %" PRIu64 " %s %s %s</th></tr>\n", config.ntop_ctrys?config.geoip_city?16:15:14, config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_hosts, config.lang.msg_top_s, config.lang.msg_h_by, config.lang.msg_h_xfer);
   else      
      fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"%d\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.ntop_ctrys?config.geoip_city?16:15:14, config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_hosts, config.lang.msg_top_s);

   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"files_th\">%s</th>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<th colspan=\"2\" class=\"pages_th\">%s</th>\n", config.lang.msg_h_pages);
   fprintf(out_fp,"<th colspan=\"2\" class=\"kbytes_th\">%s</th>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"duration_th\" title=\"%s\">%s</th>\n", "avg/max (in minutes)", config.lang.msg_h_duration);
   if(config.ntop_ctrys) {
      fprintf(out_fp,"<th class=\"country_th\">%s</th>\n", config.lang.msg_h_ctry);
      if(config.geoip_city)
         fprintf(out_fp,"<th class=\"country_th\">%s</th>\n", config.lang.msg_h_city);
   }
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_hname);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   hptr = &h_array[0];
   for(i = 0; i < tot_num; i++) {
      // if the country report is enabled, look up country name
      if(config.ntop_ctrys) {
         if(hptr->flag==OBJ_GRP)
            cdesc = "";
         else
            cdesc = state.cc_htab.get_ccnode(hptr->get_ccode()).cdesc;
      }

      /* shade grouping? */
      if (config.shade_groups && (hptr->flag==OBJ_GRP))
         fputs("<tr class=\"group_shade_tr\">\n", out_fp);
      else 
         fputs("<tr>\n", out_fp);

      fprintf(out_fp,
           "<th>%u</th>\n"
           "<td>%" PRIu64 "</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td>%" PRIu64 "</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td>%" PRIu64 "</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td>%" PRIu64 "</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td>%0.2f</td>\n"
           "<td>%0.2f</td>\n",
           i+1,
           hptr->count, (state.totals.t_hit==0)?0:((double)hptr->count/state.totals.t_hit)*100.0,
           hptr->files, (state.totals.t_file==0)?0:((double)hptr->files/state.totals.t_file)*100.0,
           hptr->pages, (state.totals.t_page==0)?0:((double)hptr->pages/state.totals.t_page)*100.0,
           hptr->xfer, fmt_xfer(hptr->xfer),
           (state.totals.t_xfer==0)?0:(hptr->xfer/state.totals.t_xfer)*100.0,
           hptr->visits,(state.totals.t_visits==0)?0:((double)hptr->visits/state.totals.t_visits)*100.0,
           hptr->visit_avg/60., hptr->visit_max/60.);

      if(config.ntop_ctrys) {
         fprintf(out_fp, "<td class=\"stats_data_item_td\">%s</td>\n", cdesc);
         if(config.geoip_city)
            fprintf(out_fp, "<td class=\"stats_data_item_td\">%s</td>\n", hptr->city.c_str());
      }

      // output the span with the IP address as a title
      fprintf(out_fp, 
           "<td class=\"stats_data_item_td%s\"><span title=\"%s\">",
           hptr->spammer ? " spammer" : hptr->robot ? " robot" : hptr->visits_conv ? " converted" : "", hptr->string.c_str());

      // output the data item
      if ((hptr->flag==OBJ_GRP) && config.hlite_groups)
         fprintf(out_fp,"<strong>%s</strong></span></td></tr>\n", hptr->string.c_str());
      else 
         fprintf(out_fp,"%s</span></td></tr>\n", hptr->hostname().c_str());

      hptr++;
   }
   fputs("</tbody>\n", out_fp);

   delete [] h_array;

   if(!flag || (flag && !config.ntop_hosts))
   {
      if(config.all_hosts && tot_num == ntop_num && a_ctr > ntop_num) {
         if (all_hosts_page())
         {
            fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
            fputs("<tr class=\"all_items_tr\">", out_fp);
            fprintf(out_fp, "<td colspan=\"%d\">\n", config.ntop_ctrys?config.geoip_city?16:15:14);
            fprintf(out_fp,"<a href=\"./site_%04d%02d.%s\">", state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, config.html_ext.c_str());
            fprintf(out_fp,"%s</a></td></tr>\n", config.lang.msg_v_hosts);
            fputs("</tbody>\n", out_fp);
         }
      }
   }
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

/*********************************************/
/* ALL_SITES_PAGE - HTML page of all sites   */
/*********************************************/

int html_output_t::all_hosts_page(void)
{
   hnode_t  hnode;
   string_t site_fname;
   FILE     *out_fp;

   /* generate file name */
   site_fname.format("site_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      site_fname = site_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(site_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_hosts);
   write_html_head(report_title, out_fp, page_all_items);

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp, " %12s      %12s      %12s      %12s      %12s      %11s   ", config.lang.msg_h_hits, config.lang.msg_h_files, config.lang.msg_h_pages, config.lang.msg_h_xfer, config.lang.msg_h_visits, config.lang.msg_h_duration);
   if(config.ntop_ctrys) {
      fprintf(out_fp, "   %-22s", config.lang.msg_h_ctry);
      if(config.geoip_city)
         fprintf(out_fp, "   %-22s", config.lang.msg_h_city);
   }
   fprintf(out_fp, "   %s\n", config.lang.msg_h_hname);

   fputs("----------------  ----------------  ----------------  ----------------  ----------------  ---------------", out_fp);
   if(config.ntop_ctrys) {
      fputs("  ----------------------", out_fp);   // country
      if(config.geoip_city)
         fputs("  ----------------------", out_fp);   // city
   }
   fputs("   --------------------\n\n", out_fp);

   if(state.totals.t_grp_hosts) {
      database_t::reverse_iterator<hnode_t> iter = state.database.rbegin_hosts("hosts.groups.hits");

      /* Do groups first (if any) */
      while(iter.prev(hnode)) {
         if (hnode.flag == OBJ_GRP)
         {
            fprintf(out_fp, "%-8" PRIu64 " %6.02f%%  %8" PRIu64 " %6.02f%%  %8" PRIu64 " %6.02f%%  <span data-xfer=\"%" PRIu64 "\">%8s</span> %6.02f%%  %8" PRIu64 " %6.02f%%  %7.02f %7.02f",
               hnode.count,(state.totals.t_hit==0)?0:((double)hnode.count/state.totals.t_hit)*100.0,
               hnode.files,(state.totals.t_file==0)?0:((double)hnode.files/state.totals.t_file)*100.0,
               hnode.pages,(state.totals.t_page==0)?0:((double)hnode.pages/state.totals.t_page)*100.0,
               hnode.xfer, fmt_xfer(hnode.xfer, true),
               (state.totals.t_xfer==0) ? .0 : PCENT(hnode.xfer, state.totals.t_xfer),
               hnode.visits,(state.totals.t_visits==0)?0:((double)hnode.visits/state.totals.t_visits)*100.0,
               hnode.visit_avg/60., hnode.visit_max/60.);

            if(config.ntop_ctrys) {
               fprintf(out_fp, "  %22c", ' ');  // country
               if(config.geoip_city)
                  fprintf(out_fp, "  %22c", ' ');  // city
            }

            fprintf(out_fp, "   %s\n", hnode.string.c_str());
         }
      }
      iter.close();

      fputs("\n", out_fp);
   }


   /* Now do individual sites (if any) */
   if (!config.hide_hosts) {
      database_t::reverse_iterator<hnode_t> iter = state.database.rbegin_hosts("hosts.hits");

      while(iter.prev(hnode)) {
         if(hnode.flag == OBJ_REG) {
            if(hnode.robot && config.hide_robots || config.hidden_hosts.isinlist(hnode.string) || config.hidden_hosts.isinlist(hnode.name))
               continue;

            fprintf(out_fp, "%-8" PRIu64 " %6.02f%%  %8" PRIu64 " %6.02f%%  %8" PRIu64 " %6.02f%%  <span data-xfer=\"%" PRIu64 "\">%8s</span> %6.02f%%  %8" PRIu64 " %6.02f%%  %7.02f %7.02f",
               hnode.count,(state.totals.t_hit==0)?0:((double)hnode.count/state.totals.t_hit)*100.0,
               hnode.files,(state.totals.t_file==0)?0:((double)hnode.files/state.totals.t_file)*100.0,
               hnode.pages,(state.totals.t_page==0)?0:((double)hnode.pages/state.totals.t_page)*100.0,
               hnode.xfer, fmt_xfer(hnode.xfer, true),
               (state.totals.t_xfer==0) ? .0 : PCENT(hnode.xfer, state.totals.t_xfer),
               hnode.visits,(state.totals.t_visits==0)?0:((double)hnode.visits/state.totals.t_visits)*100.0,
               hnode.visit_avg/60., hnode.visit_max/60.);

            if(config.ntop_ctrys) {
               fprintf(out_fp, "  %-22s", state.cc_htab.get_ccnode(hnode.get_ccode()).cdesc.c_str());
               if(config.geoip_city)
                  fprintf(out_fp, "  %-22s", hnode.city.c_str());
            }

            fprintf(out_fp, " %c <span %stitle=\"%s\">%s</span>\n",
               hnode.spammer ? '*' : ' ',
               hnode.spammer ? "class=\"spammer\" " : hnode.robot ? "class=\"robot\" " : hnode.visits_conv ? "class=\"converted\" " : "",
               hnode.string.c_str(), hnode.hostname().c_str());
         }
      }
      iter.close();
   }

   fputs("</pre>\n", out_fp);
   write_html_tail(out_fp);
   fclose(out_fp);
   return 1;
}

/*********************************************/
/* TOP_URLS_TABLE - generate top n table     */
/*********************************************/

void html_output_t::top_urls_table(int flag)
{
   uint64_t a_ctr;
   u_int i;
   uint32_t tot_num, ntop_num; 
   const unode_t *uptr;
   unode_t *u_array;
   string_t str;

   // return if nothing to process
   if (state.totals.t_url == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_url + state.totals.t_grp_urls) == 0)
      return;

   /* get max to do... */
   ntop_num = (flag) ? config.ntop_urlsK : config.ntop_urls;
   tot_num = (a_ctr > ntop_num) ? ntop_num : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   u_array = new unode_t[tot_num];

   i = 0;

   // if groups are bundled, put them first
   if(config.bundle_groups) {
      database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls(flag ? "urls.groups.xfer" : "urls.groups.hits");

      while(i < tot_num && iter.prev(u_array[i]))
         i++;

      iter.close();
   }

   // populate the remainder of the array
   if(i < tot_num) {
      database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls(flag ? "urls.xfer" : "urls.hits");

      while(i < tot_num && iter.prev(u_array[i])) {
         if(u_array[i].flag == OBJ_REG) {
            // ignore URLs matching any of the hiding patterns
            if(config.hidden_urls.isinlistex(u_array[i].string, u_array[i].pathlen, true))
               continue;
         }
         else if(u_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }

         i++;
      }

      iter.close();
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] u_array;
      return;
   }

   // adjust array size if it's not filled up
   if(i < tot_num)
      tot_num = (uint32_t) i;

   fputs("\n<!-- Top URLs Table -->\n", out_fp);
   if(flag)
      fputs("<div id=\"top_urls_kbytes_report\">\n", out_fp);
   else
      fputs("<div id=\"top_urls_report\">\n", out_fp);

   if(!flag || flag && !config.ntop_urls)                      /* now do <a> tag   */
      fputs("<a name=\"urls\"></a>\n", out_fp);

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   if (flag) 
      fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"8\">%s %u %s %" PRIu64 " %s %s %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_url,config.lang.msg_top_u, config.lang.msg_h_by, config.lang.msg_h_xfer);
   else 
      fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"8\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_url, config.lang.msg_top_u);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"kbytes_th\">%s</th>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<th class=\"time_th\" colspan=\"2\" title=\"%s\">%s</th>\n", "avg/max (in seconds)", config.lang.msg_h_time);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_url);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   uptr = &u_array[0];
   for(i = 0; i < tot_num; i++) {

      /* shade grouping? */
      if (config.shade_groups && (uptr->flag==OBJ_GRP))
         fputs("<tr class=\"group_shade_tr\">\n", out_fp);
      else 
         fputs("<tr>\n", out_fp);

      fprintf(out_fp,
         "<th>%u</th>\n"
         "<td>%" PRIu64 "</td>\n"
         "<td class=\"data_percent_td\">%3.02f%%</td>\n"
         "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
         "<td class=\"data_percent_td\">%3.02f%%</td>\n"
         "<td>%0.3f</td><td>%0.3f</td>\n"
         "<td class=\"stats_data_item_td%s\">", i+1, uptr->count, 
         (state.totals.t_hit==0)?0:((double)uptr->count/(double)state.totals.t_hit)*100.0,
         uptr->xfer, fmt_xfer(uptr->xfer),
         (state.totals.t_xfer==0)?0:((double)uptr->xfer/(double)state.totals.t_xfer)*100.0,
         uptr->avgtime, uptr->maxtime,
         uptr->target ? " target" : ""
         );

      if (uptr->flag==OBJ_GRP)
      {
         if (config.hlite_groups)
            fprintf(out_fp,"<strong>%s</strong></td></tr>\n", uptr->string.c_str());
         else 
            fprintf(out_fp,"%s</td></tr>\n", uptr->string.c_str());
      }
      else {
         const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

         if(!is_safe_url(uptr->string)) {
            // output unsafe URLs without a link and leave multibyte characters unencoded
            fprintf(out_fp, "%s\n", html_encode(uptr->string));
         }
         else {
            const char *href, *dispurl;

            dispurl = html_encode(uptr->string);
            href = html_encode(url_encode(uptr->string, str));

            /* check for a service prefix (ie: http://) */
            if (strstr_ex(uptr->string, "://", 10, 3)!=NULL) {
               fprintf(out_fp,"<a href=\"%s\">%s</a></td></tr>\n", href, dispurl);
            }
            else {
               /* Web log  */
               if(config.is_secure_url(uptr->urltype))
                  /* secure server mode, use https:// */
                  fprintf(out_fp, "<a href=\"https://%s%s\">%s</a></td></tr>\n", config.hname.c_str(), href, dispurl);
               else
                  /* otherwise use standard 'http://' */
                  fprintf(out_fp, "<a href=\"http://%s%s\">%s</a></td></tr>\n", config.hname.c_str(), href, dispurl);
            }
         }
      }
      uptr++;
   }
   fputs("</tbody>\n", out_fp);

   delete [] u_array;

   if ((!flag) || (flag&&!config.ntop_urls))
   {
      if (config.all_urls && tot_num == ntop_num && a_ctr > ntop_num)
      {
         if (all_urls_page())
         {
            fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
            fputs("<tr class=\"all_items_tr\">", out_fp);
            fputs("<td colspan=\"8\">", out_fp);
            fprintf(out_fp,"<a href=\"url_%04d%02d.%s\">", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());
            fprintf(out_fp,"%s</a></td></tr>\n", config.lang.msg_v_urls);
            fputs("</tbody>\n", out_fp);
         }
      }
   }
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

/*********************************************/
/* ALL_URLS_PAGE - HTML page of all urls     */
/*********************************************/

int html_output_t::all_urls_page(void)
{
   unode_t  unode;
   string_t url_fname;
   FILE     *out_fp;

   /* generate file name */
   url_fname.format("url_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      url_fname = url_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(url_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_url);
   write_html_head(report_title, out_fp, page_all_items);

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp," %12s      %12s  %12s  %12s        %s\n",
           config.lang.msg_h_hits,config.lang.msg_h_xfer,config.lang.msg_h_avgtime,config.lang.msg_h_maxtime,config.lang.msg_h_url);
   fputs("----------------  ----------------  ------------  ------------   --------------------\n\n", out_fp);

   /* do groups first (if any) */
   if(state.totals.t_grp_urls) {
      database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls("urls.groups.hits");
      
      while (iter.prev(unode)) {
         if (unode.flag == OBJ_GRP) {
            fprintf(out_fp,"%-8" PRIu64 " %6.02f%%  <span data-xfer=\"%" PRIu64 "\">%8s</span> %6.02f%%  %12.3f  %12.3f   %s\n",
               unode.count,
               (state.totals.t_hit==0)?0:((double)unode.count/state.totals.t_hit)*100.0,
               unode.xfer, fmt_xfer(unode.xfer, true),
               (state.totals.t_xfer==0)?0:(unode.xfer/state.totals.t_xfer)*100.0,
               unode.avgtime, unode.maxtime,
               unode.string.c_str());
         }
      }

      iter.close();

      fputs("\n", out_fp);
   }

   /* now do invididual sites (if any) */
   database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls("urls.hits");

   while (iter.prev(unode)) {
      if(unode.flag == OBJ_REG) {
         if(config.hidden_urls.isinlistex(unode.string, unode.pathlen, true))
            continue;

         buffer_formatter.set_scope_mode(buffer_formatter_t::append),
         fprintf(out_fp,"%-8" PRIu64 " %6.02f%%  <span data-xfer=\"%" PRIu64 "\">%8s</span> %6.02f%%  %12.3f  %12.3f %c <span%s>%s</span>\n",
            unode.count,
            (state.totals.t_hit==0)?0:((double)unode.count/state.totals.t_hit)*100.0,
            unode.xfer, fmt_xfer(unode.xfer, true),
            (state.totals.t_xfer==0)?0:(unode.xfer/state.totals.t_xfer)*100.0,
            unode.avgtime, unode.maxtime,
            (unode.urltype == URL_TYPE_HTTPS) ? '*' : (unode.urltype == URL_TYPE_MIXED) ? '-' : ' ',
            unode.target ? " class=\"target\"" : "",
            html_encode(unode.string));
      }
   }
   iter.close();

   fputs("</pre>\n", out_fp);
   write_html_tail(out_fp);
   fclose(out_fp);
   return 1;
}

/*********************************************/
/* TOP_ENTRY_TABLE - top n entry/exit urls   */
/*********************************************/

void html_output_t::top_entry_table(int flag)
{
   uint64_t a_ctr;
   uint32_t tot_num;
   u_int i;
   unode_t unode;
   unode_t *u_array;
   const unode_t *uptr;
   string_t str;

   // return if nothing to process
   if (state.totals.t_url == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = (flag ? state.totals.u_exit : state.totals.u_entry)) == 0)
      return;

   /* get max to do... */
   if(flag)
      tot_num = (a_ctr > config.ntop_exit) ? config.ntop_exit : (uint32_t) a_ctr;
   else
      tot_num = (a_ctr > config.ntop_entry) ? config.ntop_entry : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   u_array = new unode_t[tot_num];

   i = 0;

   // traverse the entry/exit tables and populate the array
   database_t::reverse_iterator<unode_t> iter = state.database.rbegin_urls(flag ? "urls.exit" : "urls.entry");

   while(i < tot_num && iter.prev(u_array[i])) {
      if(u_array[i].flag == OBJ_REG && !config.hidden_urls.isinlistex(u_array[i].string, u_array[i].pathlen, true)) {
         // do not show entries with zero entry/exit values
         if(!flag && u_array[i].entry || flag && u_array[i].exit)
            i++;
      }
   }

   iter.close();

   // check if all items are hidden or have zero entry/exit counts
   if(i == 0) {
      delete [] u_array;
      return;
   }

   // adjust array size if it's not filled up
   if(i < tot_num)
      tot_num = i;

   fputs("\n<!-- Top Entry/Exit Table -->\n", out_fp);

   if (flag) {
      fputs("<div id=\"top_exit_urls_report\">\n", out_fp);
      fputs("<a name=\"exit\"></a>\n", out_fp); /* do anchor tag */
   }
   else {
      fputs("<div id=\"top_entry_urls_report\">\n", out_fp);
      fputs("<a name=\"entry\"></a>\n", out_fp);
   }

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"6\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of,
           (flag) ? state.totals.u_exit : state.totals.u_entry, (flag) ? config.lang.msg_top_ex : config.lang.msg_top_en);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_url);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   uptr = &u_array[0];
   for(i = 0; i < tot_num; i++) {
      const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

      fputs("<tr>\n", out_fp);
      fprintf(out_fp,
          "<th>%d</th>\n" \
          "<td>%" PRIu64 "</td>\n" \
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"   \
          "<td>%" PRIu64 "</td>\n" \
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"   \
          "<td class=\"stats_data_item_td\">",
          i+1,uptr->count,
          (state.totals.t_hit==0)?0:((double)uptr->count/state.totals.t_hit)*100.0,
          (flag)?uptr->exit:uptr->entry,
          (flag)?((state.totals.t_exit==0)?0:((double)uptr->exit/state.totals.t_exit)*100.0)
                :((state.totals.t_entry==0)?0:((double)uptr->entry/state.totals.t_entry)*100.0));

      if(!is_safe_url(uptr->string)) {
         // output unsafe URLs without a link and leave multibyte characters unencoded
         fprintf(out_fp, "%s\n", html_encode(uptr->string));
      }
      else {      
         const char *href, *dispurl;

         dispurl = html_encode(uptr->string);
         href = html_encode(url_encode(uptr->string, str));

         /* check for a service prefix (ie: http://) */
         if (strstr_ex(uptr->string, "://", 10, 3)!=NULL)
            fprintf(out_fp, "<a href=\"%s\">%s</a></td></tr>\n", href, dispurl);
         else
         {
            if(config.is_secure_url(uptr->urltype))
               /* secure server mode, use https:// */
               fprintf(out_fp, "<a href=\"https://%s%s\">%s</a></td></tr>\n", config.hname.c_str(), href, dispurl);
            else
               /* otherwise use standard 'http://' */
               fprintf(out_fp, "<a href=\"http://%s%s\">%s</a></td></tr>\n", config.hname.c_str(), href, dispurl);
         }
      }

      uptr++;
   }
   fputs("</tbody>\n", out_fp);
   fputs("</table>\n", out_fp);

   // output a note that robot activity are not included in this report 
   if(state.totals.t_rhits)
      fprintf(out_fp,"<p class=\"note_p\">%s</p>", config.lang.msg_misc_robots);

   fputs("</div>\n", out_fp);

   delete [] u_array;
}

/*********************************************/
/* TOP_REFS_TABLE - generate top n table     */
/*********************************************/

void html_output_t::top_refs_table()
{
   uint64_t a_ctr;
   uint32_t tot_num;
   u_int i;
   const rnode_t *rptr;
   rnode_t *r_array;
   const char *cp1;
   string_t str;

   // return if nothing to process
   if (state.totals.t_ref==0) return;        

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_ref + state.totals.t_grp_refs) == 0)
      return;

   /* get max to do... */
   tot_num = (a_ctr > config.ntop_refs) ? config.ntop_refs : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   r_array = new rnode_t[tot_num];

   i = 0;

   // if groups are bundled, put them first
   if(config.bundle_groups) {
      database_t::reverse_iterator<rnode_t> iter = state.database.rbegin_referrers("referrers.groups.hits");

      while(i < tot_num && iter.prev(r_array[i]))
         i++;

      iter.close();
   }

   // populate the remainder of the array
   if(i < tot_num) {
      database_t::reverse_iterator<rnode_t> iter = state.database.rbegin_referrers("referrers.hits");

      while(i < tot_num && iter.prev(r_array[i])) {
         if(r_array[i].flag == OBJ_REG) {
            // ignore referrers matching any of the hiding patterns
            if(config.hidden_refs.isinlist(r_array[i].string))
               continue;
         }
         else if(r_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }

         i++;
      }

      iter.close();
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] r_array;
      return;
   }

   // adjust array size if it's not filled up
   if(i < tot_num)
      tot_num = i;

   fputs("\n<!-- Top Referrers Table -->\n", out_fp);
   fputs("<div id=\"top_referrers_report\">\n", out_fp);
   fputs("<a name=\"referrers\"></a>\n", out_fp);

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"6\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_ref, config.lang.msg_top_r);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_ref);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   rptr = &r_array[0]; 
   for(i = 0; i < tot_num; i++) {
      /* shade grouping? */
      if(config.shade_groups && (rptr->flag==OBJ_GRP))
         fputs("<tr class=\"group_shade_tr\">\n", out_fp);
      else 
         fputs("<tr>\n", out_fp);

      fprintf(out_fp,
          "<th>%d</th>\n"            \
          "<td>%" PRIu64 "</td>\n"            \
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"      \
          "<td>%" PRIu64 "</td>\n"            \
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"      \
          "<td class=\"stats_data_item_td\">",
          i+1,
          rptr->count, (state.totals.t_hit==0)?0:((double)rptr->count/state.totals.t_hit)*100.0,
          rptr->visits, (state.totals.t_visits==0) ? 0 : ((double)rptr->visits/state.totals.t_visits)*100.0);

      if (rptr->flag==OBJ_GRP)
      {
         if (config.hlite_groups)
            fprintf(out_fp,"<strong>%s</strong>", rptr->string.c_str());
         else 
            fprintf(out_fp,"%s", rptr->string.c_str());
      }
      else
      {
         const buffer_formatter_t::scope_t& fmt_scope = buffer_formatter.set_scope_mode(buffer_formatter_t::append);

         if (rptr->string.isempty())
            fprintf(out_fp,"%s", config.lang.msg_ref_dreq);
         else {
            if(!is_safe_url(rptr->string)) {
               // output unsafe URLs without a link and leave multibyte characters unencoded
               fprintf(out_fp, "%s\n", html_encode(rptr->string));
            }
            else {
               const char *href, *dispurl;

               dispurl = html_encode(rptr->string);
               href = html_encode(url_encode(rptr->string, str));

               // make a link only if the scheme is http or https
               if(!string_t::compare_ci(href, "http", 4) && 
                     (*(cp1 = &href[4]) == ':' || (*cp1 == 's' && *++cp1 == ':')) && *++cp1 == '/' && *++cp1 == '/')
                  fprintf(out_fp,"<a href=\"%s\">%s</a>", href, dispurl);
               else
                  fprintf(out_fp,"%s", dispurl);
            }
         }
      }
      fputs("</td></tr>\n", out_fp);
      rptr++;
   }
   fputs("</tbody>\n", out_fp);

   delete [] r_array;

   //
   // Generate the all-referrers report if tot_num equal to config.ntop_refs,
   // even though all remaining items may be hidden (there is no way to see
   // if that's  the case without either traversing the database until a
   // non-hidden item is found or keeping hidden item counts in the totals)
   //
   if(config.all_refs && tot_num == config.ntop_refs && a_ctr > config.ntop_refs)
   {
      if (all_refs_page()) {
         fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
         fputs("<tr class=\"all_items_tr\">", out_fp);
         fputs("<td colspan=\"6\">\n", out_fp);
         fprintf(out_fp,"<a href=\"./ref_%04d%02d.%s\">", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());
         fprintf(out_fp,"%s</a></td></tr>\n",config.lang.msg_v_refs);
         fputs("</tbody>\n", out_fp);
      }
   }
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

void html_output_t::top_dl_table(void)
{
   uint64_t a_ctr;
   uint32_t tot_num;
   u_int i;
   const dlnode_t *nptr;
   const char *cdesc = NULL;
   dlnode_t *dl_array;

   if((a_ctr = state.totals.t_downloads) == 0)
      return;

   /* get max to do... */
   tot_num = (a_ctr > config.ntop_downloads) ? config.ntop_downloads : (uint32_t) a_ctr;

   // get top tot_num xfer-ordered nodes from the state.database
   dl_array = new dlnode_t[tot_num];

   database_t::reverse_iterator<dlnode_t> iter = state.database.rbegin_downloads("downloads.xfer");

   for(i = 0; i < tot_num; i++) {
      // state_t::unpack_dlnode_const_cb does not modify state
      if(!iter.prev(dl_array[i], state_t::unpack_dlnode_const_cb, const_cast<state_t*>(&state)))
         break;
   }

   if(i < tot_num)
      fprintf(stderr, "Failed to retrieve download records (%d)", iter.get_error());

   iter.close();

   // generate the report
   fputs("\n<!-- Top Downloads Table -->\n", out_fp);
   fputs("<div id=\"top_downloads_report\">\n", out_fp);
   fputs("<a name=\"downloads\"></a>\n", out_fp);

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"%d\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.ntop_ctrys?config.geoip_city?12:11:10, config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_downloads, config.lang.msg_h_downloads);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"kbytes_th\">%s</th>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<th colspan=\"2\" class=\"time_th\" title=\"%s\">%s</th>\n", "average/total (in minutes)", config.lang.msg_h_time);
   fprintf(out_fp,"<th class=\"count_th\">%s</th>\n", config.lang.msg_h_count);
   fprintf(out_fp,"<th class=\"dlname_th\">%s</th>\n", config.lang.msg_h_download);
   if(config.ntop_ctrys) {
      fprintf(out_fp,"<th class=\"country_th\">%s</th>\n", config.lang.msg_h_ctry);
      if(config.geoip_city)
         fprintf(out_fp,"<th class=\"country_th\">%s</th>\n", config.lang.msg_h_city);
   }
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_hname);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   i=0;
   nptr = &dl_array[0];
   for(i = 0; i < tot_num; i++) {
      if(!nptr->hnode)
         throw exception_t(0, string_t::_format("Missing host in a download node (ID: %" PRIu64 ")", nptr->nodeid));

      if(config.ntop_ctrys) {
         if(!nptr->hnode)
            cdesc = "";
         else
            cdesc = state.cc_htab.get_ccnode(nptr->hnode->get_ccode()).cdesc;
      }

      fprintf(out_fp,
          "<tr>\n"
          "<th>%d</th>\n"
          "<td>%" PRIu64 "</td>\n"
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"
          "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"
          "<td>%3.02f</td>\n"
          "<td>%3.02f</td>\n"
          "<td>%" PRIu64 "</td>\n"
          "<td class=\"stats_data_item_td\">%s</td>\n",
          i+1,
          nptr->sumhits, (state.totals.t_hit == 0) ? 0 : ((double)nptr->sumhits/state.totals.t_hit)*100.0,
          nptr->sumxfer, fmt_xfer(nptr->sumxfer),
          (state.totals.t_xfer == 0) ? .0 : PCENT(nptr->sumxfer, state.totals.t_xfer),
          nptr->avgtime, nptr->sumtime, 
          nptr->count,
          nptr->string.c_str());

      if(config.ntop_ctrys) { 
         fprintf(out_fp, "<td class=\"stats_data_item_td\">%s</td>", cdesc);
         if(config.geoip_city)
            fprintf(out_fp, "<td class=\"stats_data_item_td\">%s</td>", nptr->hnode ? nptr->hnode->city.c_str() : "");
      }

      fprintf(out_fp,
          "<td class=\"stats_data_item_td\"><span title=\"%s\">%s</span></td>\n" \
          "</tr>\n",
          nptr->hnode->string.c_str(), nptr->hnode->hostname().c_str());

      nptr++;
   }
   fputs("</tbody>\n", out_fp);

   delete [] dl_array;

   // check if the all-downloads should be generated
   if (config.all_downloads && tot_num == config.ntop_downloads && a_ctr > config.ntop_downloads)
   {
      if (all_downloads_page())
      {
         fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
         fputs("<tr class=\"all_items_tr\">", out_fp);
         fprintf(out_fp,"<td colspan=\"%u\">\n", config.ntop_ctrys?config.geoip_city?12:11:10);
         fprintf(out_fp,"<a href=\"./dl_%04d%02d.%s\">", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());
         fprintf(out_fp,"%s</a></td></tr>\n", config.lang.msg_v_downloads);
         fputs("</tbody>\n", out_fp);
      }
   }
   
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

int html_output_t::all_downloads_page(void)
{
   const dlnode_t *nptr;
   FILE     *out_fp;
   string_t dl_fname;
   const char *cdesc = NULL;
   dlnode_t dlnode;

   if(state.totals.t_downloads == 0)
      return 0;

   // create a reverse state.database iterator (xfer-ordered)
   database_t::reverse_iterator<dlnode_t> iter = state.database.rbegin_downloads("downloads.xfer");

   /* generate file name */
   dl_fname.format("dl_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      dl_fname = dl_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(dl_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_download);
   write_html_head(report_title, out_fp, page_all_items);

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp,"  %9s      %15s    %12s    %6s    %-32s", config.lang.msg_h_hits, config.lang.msg_h_xfer, config.lang.msg_h_time, config.lang.msg_h_count, config.lang.msg_h_download);
   if(config.ntop_ctrys) {
      fprintf(out_fp," %-22s", config.lang.msg_h_ctry);
      if(config.geoip_city)
         fprintf(out_fp," %-22s", config.lang.msg_h_city);
   }
   fprintf(out_fp,"  %s\n", config.lang.msg_h_hname);

   fputs("-------------  -------------------  --------------  -------  --------------------------------", out_fp);
   if(config.ntop_ctrys) {
      fputs("  ----------------------", out_fp);
      if(config.geoip_city)
         fputs("  ----------------------", out_fp);
   }
   fputs("  -------------------------------\n\n", out_fp);

   // state_t::unpack_dlnode_const_cb does not modify state
   while(iter.prev(dlnode, state_t::unpack_dlnode_const_cb, const_cast<state_t*>(&state))) {
      nptr = &dlnode;

      if(!nptr->hnode)
         throw exception_t(0, string_t::_format("Missing host in a download node (ID: %" PRIu64 ")", nptr->nodeid));

      if(config.ntop_ctrys) {
         if(!nptr->hnode)
            cdesc = "";
         else
            cdesc = state.cc_htab.get_ccnode(nptr->hnode->get_ccode()).cdesc;
      }

      fprintf(out_fp,"%5" PRIu64 " %6.02f%%  <span data-xfer=\"%" PRIu64 "\">%11s</span> %6.02f%%  %6.02f  %6.02f   %6" PRIu64 "  %-32s",
         nptr->sumhits, (state.totals.t_hit == 0) ? 0 : ((double)nptr->sumhits/state.totals.t_hit)*100.0,
         nptr->sumxfer, fmt_xfer(nptr->sumxfer, true),
         (state.totals.t_xfer == 0) ? .0 : PCENT(nptr->sumxfer, state.totals.t_xfer),
         nptr->avgtime, nptr->sumtime, 
         nptr->count,
         nptr->string.c_str());

      if(config.ntop_ctrys) {
         fprintf(out_fp, "  %-22s", cdesc);
         if(config.geoip_city)
            fprintf(out_fp, "  %-22s", nptr->hnode ? nptr->hnode->city.c_str() : "");
      }
      
      fprintf(out_fp, "  <span title=\"%s\">%s</span>\n",
         nptr->hnode->string.c_str(), nptr->hnode->hostname().c_str());
   }

   iter.close();

   fputs("</pre>\n", out_fp);
   write_html_tail(out_fp);
   fclose(out_fp);

   return 1;
}

void html_output_t::top_err_table(void)
{
   uint64_t a_ctr;
   uint32_t tot_num;
   u_int i;
   const rcnode_t *rptr;
   rcnode_t rcnode;
   string_t str;

   if(state.totals.t_err == 0) return;

   if((a_ctr = state.totals.t_err) == 0)
      return;

   /* get max to do... */
   tot_num = (a_ctr > config.ntop_errors) ? config.ntop_errors : (uint32_t) a_ctr;

   // get top tot_num hit-ordered nodes from the state.database
   database_t::reverse_iterator<rcnode_t> iter = state.database.rbegin_errors("errors.hits");

   fputs("\n<!-- Top HTTP Errors Table -->\n", out_fp);
   fputs("<div id=\"top_errors_report\">\n", out_fp);
   fputs("<a name=\"errors\"></a>\n", out_fp);

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"6\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_err, config.lang.msg_h_errors);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th class=\"errors_th\">%s</th>\n", config.lang.msg_h_status);
   fprintf(out_fp,"<th class=\"method_th\">%s</th>\n", config.lang.msg_h_method);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_url);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   for(i=0; i < tot_num && iter.prev(rcnode); i++) {
      rptr = &rcnode;

      fprintf(out_fp,
          "<tr>\n"
          "<th>%d</th>\n"
          "<td>%" PRIu64 "</td>\n"
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"
          "<td title=\"%s\">%d</td>\n"
          "<td>%s</td>"
          "<td class=\"stats_data_item_td\">",
          i+1,rptr->count,
          (state.totals.t_hit == 0) ? 0: ((double)rptr->count/state.totals.t_hit)*100.0,
          config.lang.get_resp_code(rptr->respcode).desc, rptr->respcode, html_encode(rptr->method));

      fprintf(out_fp,"%s", html_encode(rptr->string));

      fputs("</td></tr>\n", out_fp);
   }
   fputs("</tbody>\n", out_fp);

   iter.close();

   if (config.all_errors && tot_num == config.ntop_errors && a_ctr > config.ntop_errors)
   {
      if (all_errors_page())
      {
         fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
         fputs("<tr class=\"all_items_tr\">", out_fp);
         fputs("<td colspan=\"6\">\n", out_fp);
         fprintf(out_fp,"<a href=\"./err_%04d%02d.%s\">", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());
         fprintf(out_fp,"%s</a></td></tr>\n",config.lang.msg_v_errors);
         fputs("</tbody>\n", out_fp);
      }
   }
   
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

int html_output_t::all_errors_page(void)
{
   rcnode_t rcnode;
   const rcnode_t *rptr;
   string_t err_fname;
   FILE     *out_fp;

   /* generate file name */
   err_fname.format("err_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      err_fname = err_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(err_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_status);
   write_html_head(report_title, out_fp, page_all_items);

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp,"  %12s      %8s      %8s      %s\n",config.lang.msg_h_hits,config.lang.msg_h_status,config.lang.msg_h_method,config.lang.msg_h_url);
   fputs("----------------  ------------  ------------  --------------------\n\n", out_fp);

   // get top tot_num hit-ordered nodes from the state.database
   database_t::reverse_iterator<rcnode_t> iter = state.database.rbegin_errors("errors.hits");

   while(iter.prev(rcnode)) {
      rptr = &rcnode;

      buffer_formatter.set_scope_mode(buffer_formatter_t::append),
      fprintf(out_fp,"%-8" PRIu64 " %6.02f%%           %d  %12s  %s\n",
         rptr->count,
         (state.totals.t_hit==0)?0:((double)rptr->count/state.totals.t_hit)*100.0,
         rptr->respcode,
         html_encode(rptr->method),
         html_encode(rptr->string));
   }

   iter.close();

   fputs("</pre>\n", out_fp);
   write_html_tail(out_fp);
   fclose(out_fp);
   return 1;
}

/*********************************************/
/* ALL_REFS_PAGE - HTML page of all refs     */
/*********************************************/

int html_output_t::all_refs_page(void)
{
   rnode_t  rnode;
   string_t ref_fname;
   FILE     *out_fp;
   string_t str;

   /* generate file name */
   ref_fname.format("ref_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      ref_fname = ref_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(ref_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_ref);
   write_html_head(report_title, out_fp, page_all_items);

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp," %12s      %12s      %s\n",config.lang.msg_h_hits, config.lang.msg_h_visits, config.lang.msg_h_ref);
   fputs("----------------  ----------------  --------------------\n\n", out_fp);

   /* do groups first (if any) */
   if(state.totals.t_grp_refs) {
      database_t::reverse_iterator<rnode_t> iter = state.database.rbegin_referrers("referrers.groups.hits");

      while(iter.prev(rnode)) {
         if (rnode.flag == OBJ_GRP) {
            fprintf(out_fp,"%-8" PRIu64 " %6.02f%%  %-8" PRIu64 " %6.02f%%  %s\n",
               rnode.count,
               (state.totals.t_hit==0)?0:((double)rnode.count/state.totals.t_hit)*100.0,
               rnode.visits, 
               (state.totals.t_visits==0) ? 0 : ((double)rnode.visits/state.totals.t_visits)*100.0,
               rnode.string.c_str());
         }
      }

      iter.close();

      fputs("\n", out_fp);
   }

   database_t::reverse_iterator<rnode_t> iter = state.database.rbegin_referrers("referrers.hits");

   while(iter.prev(rnode)) {
      if(rnode.flag == OBJ_REG) {
         if(config.hidden_refs.isinlist(rnode.string))
            continue;
      
         const char *dispurl;

         if(rnode.string.isempty())
            dispurl = config.lang.msg_ref_dreq;
         else
            dispurl = rnode.string;

         buffer_formatter.set_scope_mode(buffer_formatter_t::append),
         fprintf(out_fp,"%-8" PRIu64 " %6.02f%%  %-8" PRIu64 " %6.02f%%  %s\n",
            rnode.count,
            (state.totals.t_hit==0)?0:((double)rnode.count/state.totals.t_hit)*100.0,
            rnode.visits, 
            (state.totals.t_visits==0) ? 0 : ((double)rnode.visits/state.totals.t_visits)*100.0,
            html_encode(dispurl));
      }
   }

   iter.close();

   fputs("</pre>\n", out_fp);
   write_html_tail(out_fp);
   fclose(out_fp);
   return 1;
}

/*********************************************/
/* TOP_AGENTS_TABLE - generate top n table   */
/*********************************************/

void html_output_t::top_agents_table()
{
   uint64_t a_ctr;
   uint32_t tot_num;
   u_int i;
   const anode_t *aptr;
   anode_t *a_array;

   /* don't bother if we don't have any */
   if (state.totals.t_agent == 0) return;    

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_agent + state.totals.t_grp_agents) == 0)
      return;

   /* get max to do... */
   tot_num = (a_ctr > config.ntop_agents) ? config.ntop_agents : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   a_array = new anode_t[tot_num];

   i = 0;

   // if groups are bundled, put them first
   if(config.bundle_groups) {
      database_t::reverse_iterator<anode_t> iter = state.database.rbegin_agents("agents.groups.visits");

      while(i < tot_num && iter.prev(a_array[i]))
         i++;

      iter.close();
   }

   // populate the remainder of the array
   if(i < tot_num) {
      database_t::reverse_iterator<anode_t> iter = state.database.rbegin_agents("agents.visits");

      while(i < tot_num && iter.prev(a_array[i])) {
         if(a_array[i].flag == OBJ_REG) {
            // ignore agents matching any of the hiding patterns
            if(config.hide_robots  && a_array[i].robot || config.hidden_agents.isinlist(a_array[i].string))
               continue;
         }
         else if(a_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }

         i++;
      }

      iter.close();
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] a_array;
      return;
   }

   // adjust array size if it's not filled up
   if(i < tot_num)
      tot_num = i;

   fputs("\n<!-- Top User Agents Table -->\n", out_fp);
   fputs("<div id=\"top_user_agents_report\">\n", out_fp);
   fputs("<a name=\"useragents\"></a>\n", out_fp);

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"8\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_agent, config.lang.msg_top_a);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"kbytes_th\">%s</th>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_agent);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   aptr = &a_array[0];
   for(i = 0; i < tot_num; i++) {
      /* shade grouping? */
      if (config.shade_groups && (aptr->flag==OBJ_GRP))
         fputs("<tr class=\"group_shade_tr\">\n", out_fp);
      else 
         fputs("<tr>\n", out_fp);

      fprintf(out_fp,
          "<td>%d</td>\n"
          "<td>%" PRIu64 "</td>\n"
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"
          "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"
          "<td>%" PRIu64 "</td>\n"
          "<td class=\"data_percent_td\">%3.02f%%</td>\n"
          "<td class=\"stats_data_item_td\">",
          i+1,
          aptr->count, (state.totals.t_hit==0)?0:((double)aptr->count/state.totals.t_hit)*100.0,
          aptr->xfer, fmt_xfer(aptr->xfer),
          (state.totals.t_xfer==0) ? .0 : PCENT(aptr->xfer, state.totals.t_xfer),
          aptr->visits, (state.totals.t_visits==0)?0:((double)aptr->visits/state.totals.t_visits)*100.0);

      if(aptr->robot) {
         if (aptr->flag == OBJ_GRP && config.hlite_groups)
            fprintf(out_fp,"<strong><span class=\"robot\">%s</span></strong>\n", aptr->string.c_str()); 
         else 
            fprintf(out_fp,"<span class=\"robot\">%s</span>", html_encode(aptr->string.c_str()));
      }
      else {
         if (aptr->flag == OBJ_GRP && config.hlite_groups)
            fprintf(out_fp,"<strong>%s</strong>", aptr->string.c_str()); 
         else 
            fprintf(out_fp,"%s", html_encode(aptr->string.c_str()));
      }
      fputs("</td></tr>\n", out_fp);

      aptr++;
   }
   fputs("</tbody>\n", out_fp);

   delete [] a_array;

   if(config.all_agents && tot_num == config.ntop_agents && a_ctr > config.ntop_agents) {
      if (all_agents_page())
      {
         fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
         fputs("<tr class=\"all_items_tr\">", out_fp);
         fputs("<td colspan=\"8\">\n", out_fp);
         fprintf(out_fp,"<a href=\"./agent_%04d%02d.%s\">", state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, config.html_ext.c_str());
         fprintf(out_fp,"%s</a></td></tr>\n",config.lang.msg_v_agents);
         fputs("</tbody>\n", out_fp);
      }
   }
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

/*********************************************/
/* ALL_AGENTS_PAGE - HTML user agent page    */
/*********************************************/

int html_output_t::all_agents_page(void)
{
   anode_t  anode;
   string_t agent_fname;
   FILE     *out_fp;

   /* generate file name */
   agent_fname.format("agent_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      agent_fname = agent_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(agent_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_agent);
   write_html_head(report_title, out_fp, page_all_items);

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp," %12s      %12s        %12s      %s\n", config.lang.msg_h_hits, config.lang.msg_h_xfer, config.lang.msg_h_visits, config.lang.msg_h_agent);
   fputs("----------------  ----------------  ----------------  ----------------------\n\n", out_fp);

   /* do groups first (if any) */
   if(state.totals.t_grp_agents) {
      database_t::reverse_iterator<anode_t> iter = state.database.rbegin_agents("agents.groups.visits");

      while(iter.prev(anode))
      {
         if (anode.flag == OBJ_GRP)
         {
            fprintf(out_fp,"%-8" PRIu64 " %6.02f%%  <span\"%" PRIu64 "\">%8s</span> %6.02f%%  %8" PRIu64 " %6.02f%%  ",
                anode.count, (state.totals.t_hit==0)?0:((double)anode.count/state.totals.t_hit)*100.0,
                anode.xfer, fmt_xfer(anode.xfer, true),
                (state.totals.t_xfer==0) ? .0 : PCENT(anode.xfer, state.totals.t_xfer),
                anode.visits, (state.totals.t_visits==0)?0:((double)anode.visits/state.totals.t_visits)*100.0);

            if(anode.robot)
               fprintf(out_fp, "<span class=\"robot\">%s</span>", anode.string.c_str());
            else
               fprintf(out_fp, "%s", anode.string.c_str());

            fputs("\n", out_fp);
         }
      }
      iter.close();

      fputs("\n", out_fp);
   }

   database_t::reverse_iterator<anode_t> iter = state.database.rbegin_agents("agents.visits");

   while(iter.prev(anode)) {
      if(anode.flag == OBJ_REG) {
         if(config.hide_robots  && anode.robot || config.hidden_agents.isinlist(anode.string))
            continue;
                     
         fprintf(out_fp,"%-8" PRIu64 " %6.02f%%  <span\"%" PRIu64 "\">%8s</span> %6.02f%%  %8" PRIu64 " %6.02f%%  ",
             anode.count, (state.totals.t_hit==0)?0:((double)anode.count/state.totals.t_hit)*100.0,
             anode.xfer, fmt_xfer(anode.xfer, true),
             (state.totals.t_xfer==0) ? .0 : PCENT(anode.xfer, state.totals.t_xfer),
             anode.visits, (state.totals.t_visits==0)?0:((double)anode.visits/state.totals.t_visits)*100.0);

            if(anode.robot)
               fprintf(out_fp, "<span class=\"robot\">%s</span>", html_encode(anode.string));
            else
               fprintf(out_fp, "%s", html_encode(anode.string));

            fputs("\n", out_fp);
      }
   }
   iter.close();

   fputs("</pre>\n", out_fp);
   write_html_tail(out_fp);
   fclose(out_fp);
   return 1;
}

/*********************************************/
/* TOP_SEARCH_TABLE - generate top n table   */
/*********************************************/

void html_output_t::top_search_table(void)
{
   uint64_t a_ctr;
   uint32_t tot_num;
   u_int i, termidx;
   snode_t snode;
   const snode_t *sptr;
   string_t type, str;
   const char *cp1;

   if(state.totals.t_srchits == 0)
      return;

   if((a_ctr = state.totals.t_search) == 0)
      return;

   tot_num = (a_ctr > config.ntop_search) ? config.ntop_search : (uint32_t) a_ctr;

   fputs("\n<!-- Top Search Strings Table -->\n", out_fp);
   fputs("<div id=\"top_search_report\">\n", out_fp);
   fputs("<a name=\"search\"></a>\n", out_fp);

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"6\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, a_ctr, config.lang.msg_top_sr);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_search);
   fputs("</thead>\n", out_fp);

   database_t::reverse_iterator<snode_t> iter = state.database.rbegin_search("search.hits");

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   for(i = 0; i < tot_num && iter.prev(snode); i++) {
      sptr = &snode;
      fprintf(out_fp,
         "<tr>\n"
         "<th>%d</th>\n"
         "<td>%" PRIu64 "</td>\n"
         "<td class=\"data_percent_td\">%3.02f%%</td>\n"
         "<td>%" PRIu64 "</td>\n"
         "<td class=\"data_percent_td\">%3.02f%%</td>\n",
         i+1, 
         sptr->count,
         state.totals.t_srchits ? ((double)sptr->count/state.totals.t_srchits)*100.0 : 0.0,
         sptr->visits,
         state.totals.t_visits ? ((double)sptr->visits/state.totals.t_visits)*100.0 : 0.0);

      //
      // [6]Phrase[13]webalizer css[9]File Type[3]any
      //
      cp1 = sptr->string;
      if(sptr->termcnt) {
         termidx = 0;
         fputs("<td class=\"stats_data_item_td\">", out_fp);
         while((cp1 = cstr2str(cp1, type)) != NULL && (cp1 = cstr2str(cp1, str)) != NULL) {
            if(termidx)
               fputc(' ', out_fp);
            if(!type.isempty())
               fprintf(out_fp,"<span class=\"search_type\">[%s]</span> %s", type.c_str(), html_encode(str));
            else 
               fprintf(out_fp,"%s%s", (termidx) ? "<span class=\"search_type\">&bull;</span> " : "", html_encode(str));
            termidx++;
         }
         fputs("</td></tr>\n", out_fp);
      } 
      else {
         // no search type info - just print the string
         fprintf(out_fp, "<td class=\"stats_data_item_td\">%s</td></tr>\n", html_encode(cp1));
      }
   }
   fputs("</tbody>\n", out_fp);

   iter.close();

   if ( (config.all_search) && tot_num == config.ntop_search && a_ctr > config.ntop_search)
   {
      if (all_search_page())
      {
         fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
         fputs("<tr class=\"all_items_tr\">", out_fp);
         fputs("<td colspan=\"6\">\n", out_fp);
         fprintf(out_fp,"<a href=\"./search_%04d%02d.%s\">", state.totals.cur_tstamp.year, state.totals.cur_tstamp.month, config.html_ext.c_str());
         fprintf(out_fp,"%s</a></td></tr>\n", config.lang.msg_v_search);
         fputs("</tbody>\n", out_fp);
      }
   }
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

/*********************************************/
/* ALL_SEARCH_PAGE - HTML for search strings */
/*********************************************/

int html_output_t::all_search_page(void)
{
   const snode_t *sptr;
   snode_t snode;
   string_t search_fname;
   const char *cp1;
   string_t type, str;
   FILE     *out_fp;
   u_int termidx;

   if(state.totals.t_srchits == 0)
      return 0;

   /* generate file name */
   search_fname.format("search_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      search_fname = search_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(search_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_search);
   write_html_head(report_title, out_fp, page_all_items);

   database_t::reverse_iterator<snode_t> iter = state.database.rbegin_search("search.hits");

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp," %12s       %12s      %s\n",config.lang.msg_h_hits, config.lang.msg_h_visits, config.lang.msg_h_search);
   fputs("----------------  ----------------  ----------------------\n\n", out_fp);

   while(iter.prev(snode)) {
      sptr = &snode;
      fprintf(out_fp,"%-8" PRIu64 " %6.02f%%  %-8" PRIu64 " %6.02f%%  ",
         sptr->count,
         state.totals.t_srchits ? ((double)sptr->count/state.totals.t_srchits)*100.0 : 0.0,
         sptr->visits,
         state.totals.t_visits ? ((double)sptr->visits/state.totals.t_visits)*100.0 : 0.0);

      cp1 = sptr->string;
      if(sptr->termcnt) {
         termidx = 0;
         while((cp1 = cstr2str(cp1, type)) != NULL && (cp1 = cstr2str(cp1, str)) != NULL) {
            if(termidx)
               fputc(' ', out_fp);
            if(!type.isempty())
               fprintf(out_fp,"<span class=\"search_type\">[%s]</span> %s", type.c_str(), html_encode(str));
            else 
               fprintf(out_fp,"%s%s", (termidx) ? "<span class=\"search_type\">&bull;</span> " : "", html_encode(str));
            termidx++;
         }
         fputc('\n', out_fp);
      }
      else
         fprintf(out_fp,"%s\n", html_encode(sptr->string));
   }
   fputs("</pre>\n", out_fp);

   iter.close();

   write_html_tail(out_fp);
   fclose(out_fp);
   return 1;
}

/*********************************************/
/* TOP_USERS_TABLE - generate top n table    */
/*********************************************/

void html_output_t::top_users_table()
{
   uint64_t a_ctr=0;
   uint32_t tot_num;
   u_int i;
   const inode_t *iptr;
   const char *dispuser;
   inode_t *i_array;
   string_t str;

   // return if nothing to process
   if (state.totals.t_user == 0) return;

   // get the total number, including groups and hidden items
   if((a_ctr = state.totals.t_user + state.totals.t_grp_users) == 0)
      return;

   /* get max to do... */
   tot_num = (a_ctr > config.ntop_users) ? config.ntop_users : (uint32_t) a_ctr;

   // allocate as if there are no hidden items
   i_array = new inode_t[tot_num];

   i = 0;

   // if groups are bundled, put them first
   if(config.bundle_groups) {
      database_t::reverse_iterator<inode_t> iter = state.database.rbegin_users("users.groups.hits");

      while(i < tot_num && iter.prev(i_array[i]))
         i++;

      iter.close();
   }

   // populate the remainder of the array
   if(i < tot_num) {
      database_t::reverse_iterator<inode_t> iter = state.database.rbegin_users("users.hits");

      while(i < tot_num && iter.prev(i_array[i])) {
         if(i_array[i].flag == OBJ_REG) {
            // ignore referrers matching any of the hiding patterns
            if(config.hidden_users.isinlist(i_array[i].string))
               continue;
         }
         else if(i_array[i].flag == OBJ_GRP) {
            // ignore groups if we did them before
            if(config.bundle_groups)
               continue;
         }

         i++;
      }

      iter.close();
   }

   // check if all items are hidden
   if(i == 0) {
      delete [] i_array;
      return;
   }

   // adjust array size if it's not filled up
   if(i < tot_num)
      tot_num = i;

   fputs("\n<!-- Top Users Table -->\n", out_fp);
   fputs("<div id=\"top_users_report\">\n", out_fp);
   fputs("<a name=\"users\"></a>\n", out_fp);       /* now do <a> tag   */

   fputs("<table class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"12\">%s %u %s %" PRIu64 " %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, state.totals.t_user, config.lang.msg_top_i);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"files_th\">%s</th>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<th colspan=\"2\" class=\"kbytes_th\">%s</th>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"time_th\" title=\"%s\">%s</th>\n", "avg/max (in seconds)", config.lang.msg_h_time);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_uname);
   fputs("</thead>\n", out_fp);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   iptr = &i_array[0]; 
   for(i = 0; i < tot_num; i++) {
      /* shade grouping? */
      if (config.shade_groups && (iptr->flag==OBJ_GRP))
         fputs("<tr class=\"group_shade_tr\">\n", out_fp);
      else 
         fputs("<tr>\n", out_fp);

      fprintf(out_fp,
           "<th>%d</td>\n"
           "<td>%" PRIu64 "</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td>%" PRIu64 "</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td data-xfer=\"%" PRIu64 "\">%s</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td>%" PRIu64 "</td>\n"
           "<td class=\"data_percent_td\">%3.02f%%</td>\n"
           "<td>%0.3f</td><td>%0.3f</td>\n"
           "<td class=\"stats_data_item_td\">",
           i+1,iptr->count,
           (state.totals.t_hit==0)?0:((double)iptr->count/state.totals.t_hit)*100.0,iptr->files,
           (state.totals.t_file==0)?0:((double)iptr->files/state.totals.t_file)*100.0,
           iptr->xfer, fmt_xfer(iptr->xfer),
           (state.totals.t_xfer==0)?0:((double)iptr->xfer/state.totals.t_xfer)*100.0,iptr->visit,
           (state.totals.t_visits==0)?0:((double)iptr->visit/state.totals.t_visits)*100.0,
           iptr->avgtime, iptr->maxtime);

      dispuser = iptr->string;
      dispuser = html_encode(dispuser);
      if(iptr->flag == OBJ_GRP && config.hlite_groups)
         fprintf(out_fp,"<strong>%s</strong></td></tr>\n", dispuser); 
      else 
         fprintf(out_fp,"%s</td></tr>\n", dispuser);
      iptr++;
   }
   fputs("</tbody>\n", out_fp);

   delete [] i_array;

   if(config.all_users && tot_num == config.ntop_users && a_ctr > config.ntop_users) {
      if (all_users_page())
      {
         fputs("<tbody class=\"stats_footer_tbody\">\n", out_fp);
         fputs("<tr class=\"all_items_tr\">\n", out_fp);
         fputs("<td colspan=\"12\">\n", out_fp);
         fprintf(out_fp,"<a href=\"./user_%04d%02d.%s\">", state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());
         fprintf(out_fp,"%s</a></td></tr>\n",config.lang.msg_v_users);
         fputs("</tbody>\n", out_fp);
      }
   }
   fputs("</table>\n", out_fp);
   fputs("</div>\n", out_fp);
}

/*********************************************/
/* ALL_USERS_PAGE - HTML of all usernames    */
/*********************************************/

int html_output_t::all_users_page(void)
{
   inode_t  inode;
   string_t user_fname;
   FILE     *out_fp;

   /* generate file name */
   user_fname.format("user_%04d%02d.%s",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month,config.html_ext.c_str());

   if(config.html_ext_lang)
      user_fname = user_fname + '.' + config.lang.language_code;

   /* open file */
   if ( (out_fp=open_out_file(user_fname))==NULL ) return 0;

   const char *report_title = fmt_printf("%s %d - %s", lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year,config.lang.msg_h_uname);
   write_html_head(report_title, out_fp, page_all_items);

   fputs("<pre class=\"details_pre\">\n", out_fp);

   fprintf(out_fp," %12s      %12s      %12s      %12s  %12s  %12s      %s\n",
           config.lang.msg_h_hits, config.lang.msg_h_files, config.lang.msg_h_xfer, config.lang.msg_h_visits, config.lang.msg_h_avgtime,config.lang.msg_h_maxtime, config.lang.msg_h_uname);
   fputs("----------------  ----------------  ----------------  ----------------  ------------  ------------  --------------------\n\n", out_fp);

   /* Do groups first (if any) */
   if(state.totals.t_grp_users) {
      database_t::reverse_iterator<inode_t> iter = state.database.rbegin_users("users.groups.hits");

      while(iter.prev(inode)) {
         if (inode.flag == OBJ_GRP) {
            fprintf(out_fp, "%-8" PRIu64 " %6.02f%%  %8" PRIu64 " %6.02f%%  <span\"%" PRIu64 "\">%8s</span> %6.02f%%  %8" PRIu64 " %6.02f%%  %12.3f  %12.3f  %s\n",
               inode.count,
               (state.totals.t_hit==0)?0:((double)inode.count/state.totals.t_hit)*100.0,inode.files,
               (state.totals.t_file==0)?0:((double)inode.files/state.totals.t_file)*100.0,
               inode.xfer, fmt_xfer(inode.xfer, true),
               (state.totals.t_xfer==0)?0:((double)inode.xfer/state.totals.t_xfer)*100.0,inode.visit,
               (state.totals.t_visits==0)?0:((double)inode.visit/state.totals.t_visits)*100.0,
               inode.avgtime, inode.maxtime,
               inode.string.c_str());
         }
      }
      iter.close();

      fputs("\n", out_fp);
   }

   /* Now do individual users (if any) */
   database_t::reverse_iterator<inode_t> iter = state.database.rbegin_users("users.hits");

   while(iter.prev(inode)) {
      if(inode.flag == OBJ_REG) {
         if(config.hidden_users.isinlist(inode.string))
            continue;
               
         fprintf(out_fp, "%-8" PRIu64 " %6.02f%%  %8" PRIu64 " %6.02f%%  <span\"%" PRIu64 "\">%8s</span> %6.02f%%  %8" PRIu64 " %6.02f%%  %12.3f  %12.3f  %s\n",
            inode.count,
            (state.totals.t_hit==0)?0:((double)inode.count/state.totals.t_hit)*100.0,inode.files,
            (state.totals.t_file==0)?0:((double)inode.files/state.totals.t_file)*100.0,
            inode.xfer, fmt_xfer(inode.xfer, true),
            (state.totals.t_xfer==0)?0:((double)inode.xfer/state.totals.t_xfer)*100.0,inode.visit,
            (state.totals.t_visits==0)?0:((double)inode.visit/state.totals.t_visits)*100.0,
            inode.avgtime, inode.maxtime,
            inode.string.c_str());
      }
   }
   iter.close();

   fputs("</pre>\n", out_fp);
   write_html_tail(out_fp);
   fclose(out_fp);
   return 1;
}

/*********************************************/
/* TOP_CTRY_TABLE - top countries table      */
/*********************************************/

void html_output_t::top_ctry_table()
{
   u_int tot_num=0, tot_ctry=0;
   uint64_t i,j;
   uint64_t pie_data[10];
   uint64_t t_hit, t_file, t_page, t_visits;
   uint64_t t_xfer;
   const char *pie_legend[10];
   const ccnode_t **ccarray;
   const ccnode_t *tptr;

   if(state.cc_htab.size() == 0)
      return;

   // exclude robot activity
   t_hit = state.totals.t_hit - state.totals.t_rhits;
   t_file = state.totals.t_file - state.totals.t_rfiles;
   t_page = state.totals.t_page - state.totals.t_rpages;
   t_xfer = state.totals.t_xfer - state.totals.t_rxfer;
   
   // only include human activity
   t_visits = state.totals.t_hvisits_end;

   // allocate and load the country array
   ccarray = new const ccnode_t*[state.cc_htab.size()];

   state.cc_htab.load_array(ccarray);

   // find the first node with a zero count
   for(i = 0; i < state.cc_htab.size(); i++, tot_ctry++) {
      if(ccarray[i]->count == 0) 
         break;
   }

   // swap the nodes with zero and non-zero counts
   for(j = i+1; j < state.cc_htab.size(); j++) {
      if(ccarray[j]->count) {           
         tptr = ccarray[i];
         ccarray[i++] = ccarray[j];    // the next one always has count == 0
         ccarray[j] = tptr;
         tot_ctry++;
      }
   }

   // sort those at the beginning of the array
   qsort(ccarray, tot_ctry, sizeof(ccnode_t*), qs_cc_cmpv);

   // select how many top entries to report
   tot_num = (tot_ctry > config.ntop_ctrys) ? config.ntop_ctrys : tot_ctry;

   /* put our anchor tag first... */
   fputs("\n<!-- Top Countries Table -->\n", out_fp);
   fputs("<div id=\"top_countries_report\">\n", out_fp);
   fputs("<a name=\"countries\"></a>\n", out_fp);

   /* generate pie chart if needed */
   if (config.ctry_graph)
   {
      if(!config.js_charts.isempty())
         fputs("<div id=\"country_usage_chart\" class=\"chart_holder\"></div>\n", out_fp);
      else {
         string_t pie_title;
         string_t pie_fname, pie_fname_lang;

         for (i=0;i<10;i++) {
            pie_data[i] = 0;                           /* init data array      */
            pie_legend[i] = NULL;
         }
         j = MIN(tot_num, 10);                         /* ensure data size     */

         for(i = 0; i < j; i++) {
            pie_data[i]=ccarray[i]->visits;            /* load the array       */
            pie_legend[i]=ccarray[i]->cdesc;
         }

         pie_title.format("%s %s %d",config.lang.msg_ctry_use, lang_t::l_month[state.totals.cur_tstamp.month-1],state.totals.cur_tstamp.year);
         pie_fname.format("ctry_usage_%04d%02d.png",state.totals.cur_tstamp.year,state.totals.cur_tstamp.month);

         if(config.html_ext_lang)
            pie_fname_lang = pie_fname + '.' + config.lang.language_code;
         else
            pie_fname_lang = pie_fname;

         if(makeimgs)
            graph.pie_chart(pie_fname_lang, pie_title, t_visits, pie_data, pie_legend);  /* do it   */

         /* put the image tag in the page */
         fprintf(out_fp,"<div id=\"country_usage_graph\" class=\"graph_holder\"><img src=\"%s\" alt=\"%s\" height=\"300\" width=\"512\"></div>\n", pie_fname.c_str(), pie_title.c_str());
      }
   }

   /* Now do the table */
   fputs("<table id=\"country_usage_table\" class=\"report_table stats_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"12\">%s %u %s %d %s</th></tr>\n", config.lang.msg_top_top, tot_num, config.lang.msg_top_of, tot_ctry, config.lang.msg_top_c);
   fputs("<tr><th class=\"counter_th\">#</th>\n", out_fp);
   fprintf(out_fp,"<th colspan=\"2\" class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th colspan=\"2\" class=\"files_th\">%s</th>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<th colspan=\"2\" class=\"pages_th\">%s</th>\n", config.lang.msg_h_pages);
   fprintf(out_fp,"<th colspan=\"2\" class=\"kbytes_th\">%s</th>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<th colspan=\"2\" class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th class=\"item_th\">%s</th></tr>\n", config.lang.msg_h_ctry);

   fputs("<tbody class=\"stats_data_tbody\">\n", out_fp);

   for(i = 0; i < tot_num; i++) {
      if(ccarray[i]->count != 0) {
         fprintf(out_fp,"<tr>"                                                \
              "<th>%" PRIu64 "</th>\n" \
              "<td>%" PRIu64 "</td>\n" \
              "<td class=\"data_percent_td\">%3.02f%%</td>\n"   \
              "<td>%" PRIu64 "</td>\n" \
              "<td class=\"data_percent_td\">%3.02f%%</td>\n"   \
              "<td>%" PRIu64 "</td>\n" \
              "<td class=\"data_percent_td\">%3.02f%%</td>\n"   \
              "<td data-xfer=\"%" PRIu64 "\">%s</td>\n" \
              "<td class=\"data_percent_td\">%3.02f%%</td>\n"   \
              "<td>%" PRIu64 "</td>\n" \
              "<td class=\"data_percent_td\">%3.02f%%</td>\n"   \
              "<td class=\"stats_data_item_td\">%s</td></tr>\n",
              i+1, ccarray[i]->count,
              (t_hit==0)?0:((double)ccarray[i]->count/t_hit)*100.0,
              ccarray[i]->files,
              (t_file==0)?0:((double)ccarray[i]->files/t_file)*100.0,
              ccarray[i]->pages,
              (t_page==0)?0:((double)ccarray[i]->pages/t_page)*100.0,
              ccarray[i]->xfer, fmt_xfer(ccarray[i]->xfer),
              (t_xfer==0)?0:(ccarray[i]->xfer/t_xfer)*100.0,
              ccarray[i]->visits,
              (t_visits==0)?0:((double)ccarray[i]->visits/t_visits)*100.0,
              ccarray[i]->cdesc.c_str());
      }
   }
   fputs("</tbody>\n", out_fp);
   fputs("</table>\n", out_fp);

   // output a note that robot activity is not included in this report 
   if(state.totals.t_rhits)
      fprintf(out_fp,"<p class=\"note_p\">%s</p>", config.lang.msg_misc_robots);

   fputs("</div>\n", out_fp);

   delete [] ccarray;
}

/*********************************************/
/* WRITE_MAIN_INDEX - main index.html file   */
/*********************************************/

int html_output_t::write_main_index()
{
   /* create main index file */

   u_int days_in_month;
   uint64_t gt_hit=0;
   uint64_t gt_files=0;
   uint64_t gt_pages=0;
   uint64_t gt_xfer=0;
   uint64_t gt_visits=0;
   uint64_t gt_hosts=0;
   string_t index_fname, png_fname, title;
   const hist_month_t *hptr;
   history_t::const_reverse_iterator iter;

   /* now do html stuff... */
   index_fname.format("index.%s",config.html_ext.c_str());

   if(config.html_ext_lang)
      index_fname = index_fname + '.' + config.lang.language_code;

   if ( (out_fp=fopen(make_path(config.out_dir, index_fname),"w")) == NULL)
   {
      if (config.verbose)
      fprintf(stderr,"%s %s!\n",config.lang.msg_no_open,index_fname.c_str());
      return 1;
   }
   
   // Last N Months
   title.format("%s %d %s", config.lang.msg_main_plst, state.history.disp_length(), config.lang.msg_main_pmns);
   write_html_head(title, out_fp, page_index);

   /* year graph */
   if(!config.js_charts.isempty())
      fputs("<div id=\"monthly_summary_chart\" class=\"chart_holder\"></div>\n", out_fp);
   else {
      string_t png_fname_lang;

      png_fname = "usage.png";

      if(config.html_ext_lang)
         png_fname_lang = png_fname + '.' + config.lang.language_code;
      else
         png_fname_lang = png_fname;

      const char *chart_title = fmt_printf("%s %s",config.lang.msg_main_us,config.hname.c_str());

      if(makeimgs)
         graph.year_graph6x(state.history, png_fname_lang, chart_title, graphinfo->usage_width, graphinfo->usage_height);

      fprintf(out_fp,"<div id=\"monthly_summary_graph\" class=\"graph_holder\" style=\"width: %dpx\"><img src=\"%s\" alt=\"%s\" width=\"%d\" height=\"%d\" ></div>\n", graphinfo->usage_width, png_fname.c_str(), chart_title, graphinfo->usage_width, graphinfo->usage_height);
   }

   fprintf(out_fp,"<p class=\"note_p\">%s</p>\n", config.lang.msg_misc_pages);

   /* month table */
   fputs("\n<!-- Monthly Summary Table -->\n", out_fp);
   fputs("<table id=\"monthly_summary_table\" class=\"report_table monthly_summary_table\">\n", out_fp);
   fputs("<thead>\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_title_tr\"><th colspan=\"11\">%s</th></tr>\n", config.lang.msg_main_sum);

   /*
    *   Summary table header
    */
   fprintf(out_fp,"<tr><th rowspan=\"2\" style=\"text-align: left;\">%s</th>\n", config.lang.msg_h_mth);
   fprintf(out_fp,"<th colspan=\"4\">%s</th>\n", config.lang.msg_main_da);
   fprintf(out_fp,"<th colspan=\"6\">%s</th></tr>\n", config.lang.msg_main_mt);
   fprintf(out_fp,"<tr><th class=\"hits_th\">%s</th>\n", config.lang.msg_h_hits);
   fprintf(out_fp,"<th class=\"files_th\">%s</th>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<th class=\"pages_th\">%s</th>\n", config.lang.msg_h_pages);
   fprintf(out_fp,"<th class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th class=\"hosts_th\">%s</th>\n", config.lang.msg_h_hosts);
   fprintf(out_fp,"<th class=\"kbytes_th\">%s</th>\n", config.lang.msg_h_xfer);
   fprintf(out_fp,"<th class=\"visits_th\">%s</th>\n", config.lang.msg_h_visits);
   fprintf(out_fp,"<th class=\"pages_th\">%s</th>\n", config.lang.msg_h_pages);
   fprintf(out_fp,"<th class=\"files_th\">%s</th>\n", config.lang.msg_h_files);
   fprintf(out_fp,"<th class=\"hits_th\">%s</th></tr>\n", config.lang.msg_h_hits);
   fputs("</thead>\n", out_fp);

   /*
    * Summary data section
    */
   fputs("<tbody class=\"summary_data_tbody\">\n", out_fp);

   iter = state.history.rbegin();
   while(iter != state.history.rend()) {
      hptr = &*iter++;
      if(hptr->hits==0) continue;
      days_in_month=(hptr->lday-hptr->fday)+1;
      fprintf(out_fp,"<tr><th data-year=\"%u\" data-month=\"%u\"><a href=\"usage_%04d%02d.%s\">%s %d</a></th>\n", hptr->year, hptr->month, hptr->year, hptr->month, config.html_ext.c_str(), lang_t::s_month[hptr->month-1], hptr->year);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n", hptr->hits/days_in_month);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n",hptr->files/days_in_month);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n", hptr->pages/days_in_month);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n", hptr->visits/days_in_month);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n", hptr->hosts);
      fprintf(out_fp,"<td data-xfer=\"%" PRIu64 "\">%s</td>\n", hptr->xfer, fmt_xfer(hptr->xfer));
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n", hptr->visits);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n", hptr->pages);
      fprintf(out_fp,"<td>%" PRIu64 "</td>\n", hptr->files);
      fprintf(out_fp,"<td>%" PRIu64 "</td></tr>\n", hptr->hits);
      gt_hit   += hptr->hits;
      gt_files += hptr->files;
      gt_pages += hptr->pages;
      gt_xfer  += hptr->xfer;
      gt_visits+= hptr->visits;
      gt_hosts += hptr->hosts;
   }
   fputs("</tbody>\n", out_fp);

   /*
    *   Summary totals section
    */
   fputs("<tbody class=\"summary_footer_tbody\">\n", out_fp);
   fprintf(out_fp,"<tr class=\"table_footer_tr\"><th colspan=\"5\">%s</th>\n", config.lang.msg_h_totals);
   fprintf(out_fp,"<td>%" PRIu64 "</td>\n", gt_hosts);
   fprintf(out_fp,"<td data-xfer=\"%" PRIu64 "\">%s</td>\n", gt_xfer, fmt_xfer(gt_xfer));
   fprintf(out_fp,"<td>%" PRIu64 "</td>\n", gt_visits);
   fprintf(out_fp,"<td>%" PRIu64 "</td>\n", gt_pages);
   fprintf(out_fp,"<td>%" PRIu64 "</td>\n", gt_files);
   fprintf(out_fp,"<td>%" PRIu64 "</td></tr>\n", gt_hit);
   fputs("</tbody>\n", out_fp);

   fputs("</table>\n", out_fp);

   write_html_tail(out_fp);

   fclose(out_fp);
   return 0;
}

bool html_output_t::is_safe_url(const string_t& url)
{
   size_t chsz;
   const char *cp;
   char chr[2] = {0};

   // consider empty strings safe for the purposes of this method
   if(url.isempty())
      return true;

   cp = url.c_str();

   while(*cp) {
      // all characters must be UTF-8 encoded at this point
      if((chsz = utf8size(cp)) == 0)
         throw std::invalid_argument("Bad UTF-8 character in the URL");

      // consider all multibyte characters safe
      if(chsz > 1)
         cp += chsz;
      else if(*cp == '%') {
         cp++;

         // treat maformed percent encoding as unsafe
         if(!string_t::isxdigit(*cp) || !string_t::isxdigit(*(cp+1)))
            return false;

         from_hex(cp, chr);

         // consider control characters unsafe, even if encoded
         if((unsigned char) *chr < '\x20')
            return false;

         cp += 2;
      }
      else {
         // consider control characters unsafe
         if((unsigned char) *cp < '\x20')
            return false;

         cp++;
      }
   }

   return true;
}

//
// Include templates
//
#include "formatter_tmpl.cpp"
