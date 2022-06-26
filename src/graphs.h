/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   This software uses the gd graphics library, which is copyright by
   Quest Protein Database Center, Cold Spring Harbor Labs.  Please
   see the documentation supplied with the library for additional
   information and license terms, or visit www.boutell.com/gd/ for the
   most recent version of the library and supporting documentation.

   graphs.h
*/
#ifndef GRAPHS_H
#define GRAPHS_H

#include <gd.h>
#include <gdfonts.h>
#include <gdfontmb.h>

#include "daily.h"
#include "hourly.h"
#include "config.h"
#include "formatter.h"

class history_t;

///
/// @brief  A PNG chart generator class
///
class graph_t {
   private:
      const config_t& config;

      string_t::char_buffer_t xfer_fmt_buf;
      buffer_formatter_t buffer_formatter;

      int font_size_small_px;
      int font_size_medium_px;
      int font_size_medium_bold_px;

      /* colors */
      uint32_t black, white, dkgrey, red, blue, orange, green, cyan, yellow, purple, ltpurple, ltgreen, brown;
      uint32_t c_shadow, c_background, c_gridline, c_hits, c_files, c_hosts, c_pages, c_visits, c_xfer, c_outline, c_legend, c_weekend;

      uint32_t graph_background;
      uint32_t graph_gridline;
      uint32_t graph_shadow;
      uint32_t graph_title_color;
      uint32_t graph_hits_color;
      uint32_t graph_files_color;
      uint32_t graph_hosts_color;
      uint32_t graph_pages_color;
      uint32_t graph_visits_color;
      uint32_t graph_xfer_color;
      uint32_t graph_outline_color;
      uint32_t graph_legend_color;
      uint32_t graph_weekend_color;

      gdImagePtr im;                      /* image buffer        */
      FILE *out;                          /* output file for PNG */
      char maxvaltxt[32];                 /* graph values        */
      double percent;                     /* percent storage     */

      static const char *numchar[];

   private:
      void _gdImageString(gdImagePtr im, int fonttype, int x, int y, const char *str, int color, bool xyhead, u_int *textsize);
      void _gdImageStringUp(gdImagePtr im, int fonttype, int x, int y, const char *str, int color, bool xyhead, u_int *textsize);
      void _gdImageStringEx(gdImagePtr im, int fonttype, int x, int y, u_char *str, int color, bool up, bool xyhead, u_int *textsize);
   
      void init_graph(const char *title, int xsize, int ysize);

      void draw_section_line(gdImagePtr im, u_int gw, u_int y);
      void draw_grid_line(gdImagePtr im, u_int gw, u_int y);
      void draw_graph_bar(gdImagePtr im, u_int x1, u_int y1, u_int x2, u_int y2, int color);

      const char *fmt_xfer(uint64_t xfer);

   public:
      graph_t(const config_t& _config);

      ~graph_t(void);

      static uint32_t make_color(const char *str);

      // these methods should be called only after the engine has been initialized
      uint32_t get_background_color(void) const {return graph_background;}
      uint32_t get_title_color(void) const {return graph_title_color;}
      uint32_t get_gridline_color(void) const {return graph_gridline;}
      uint32_t get_shadow_color(void) const {return graph_shadow;}
      uint32_t get_hits_color(void) const {return graph_hits_color;}
      uint32_t get_files_color(void) const {return graph_files_color;}
      uint32_t get_pages_color(void) const {return graph_pages_color;}
      uint32_t get_visits_color(void) const {return graph_visits_color;}
      uint32_t get_hosts_color(void) const {return graph_hosts_color;}
      uint32_t get_xfer_color(void) const {return graph_xfer_color;}
      uint32_t get_outline_color(void) const {return graph_outline_color;}
      uint32_t get_legend_color(void) const {return graph_legend_color;}
      uint32_t get_weekend_color(void) const {return graph_weekend_color;}

      // return true if the respective color is defined in the configuration
      bool is_default_background_color(void) const {return config.graph_background.isempty() ? true : false;}
      bool is_default_title_color(void) const {return config.graph_title_color.isempty() ? true : false;}
      bool is_default_gridline_color(void) const {return config.graph_gridline.isempty() ? true : false;}
      bool is_default_shadow_color(void) const {return config.graph_shadow.isempty() ? true : false;}
      bool is_default_hits_color(void) const {return config.graph_hits_color.isempty() ? true : false;}
      bool is_default_files_color(void) const {return config.graph_files_color.isempty() ? true : false;}
      bool is_default_pages_color(void) const {return config.graph_pages_color.isempty() ? true : false;}
      bool is_default_visits_color(void) const {return config.graph_visits_color.isempty() ? true : false;}
      bool is_default_hosts_color(void) const {return config.graph_hosts_color.isempty() ? true : false;}
      bool is_default_xfer_color(void) const {return config.graph_xfer_color.isempty() ? true : false;}
      bool is_default_outline_color(void) const {return config.graph_outline_color.isempty() ? true : false;}
      bool is_default_legend_color(void) const {return config.graph_legend_color.isempty() ? true : false;}
      bool is_default_weekend_color(void) const {return config.graph_weekend_color.isempty() ? true : false;}
            
      void init_graph_engine(bool makeimgs);
      void cleanup_graph_engine(void);

      int year_graph6x(const history_t& history, const char *fname, const char *title, u_int& graph_width, u_int& graph_height);
      int pie_chart(const char *fname, const char *title, uint64_t t_val, const uint64_t data1[], const char *legend[]);
      int month_graph6(const char *fname, const char *title, int month, int year, const storable_t<daily_t> daily[31]);
      int day_graph3(const char *fname, const char *title, const storable_t<hourly_t> hourly[24]);
};

#endif  // GRAPHS_H
