/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2013, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   This software uses the gd graphics library, which is copyright by
   Quest Protein Database Center, Cold Spring Harbor Labs.  Please
   see the documentation supplied with the library for additional
   information and license terms, or visit www.boutell.com/gd/ for the
   most recent version of the library and supporting documentation.

   graphs.h
*/
#ifndef _GRAPHS_H
#define _GRAPHS_H

#include <gd.h>
#include <gdfonts.h>
#include <gdfontmb.h>

#include "daily.h"
#include "hourly.h"

//
// GD uses a 7-bit alpha channel. The most significant bit is not used and can
// be designated to track the default value. 
//
// TODO: consider using a NULL of sorts to track whether the color was specified 
// or not in the configuration. This bit may interfere with libraries using 8-bit 
// alpha channels.
//
#define DEFCOLOR ((u_long) 0x80000000ul)

class config_t;
class history_t;

//
//
//
class graph_t {
   private:
      const config_t& config;

      int font_size_small_px;
      int font_size_medium_px;
      int font_size_medium_bold_px;

      /* colors */
      u_int black, white, dkgrey, red, blue, orange, green, cyan, yellow, purple, ltpurple, ltgreen, brown;
      u_int c_shadow, c_background, c_gridline, c_hits, c_files, c_hosts, c_pages, c_visits, c_volume, c_outline, c_legend, c_weekend;

      u_int graph_background;
      u_int graph_gridline;
      u_int graph_shadow;
      u_int graph_title_color;
      u_int graph_hits_color;
      u_int graph_files_color;
      u_int graph_hosts_color;
      u_int graph_pages_color;
      u_int graph_visits_color;
      u_int graph_volume_color;
      u_int graph_outline_color;
      u_int graph_legend_color;
      u_int graph_weekend_color;

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

      static bool is_default_color(u_long color);

   public:
      graph_t(const config_t& _config);

      ~graph_t(void);

      static u_long make_color(const char *str);

      // these methods should be called only after the engine has been initialized
      u_long get_background_color(void) const {return graph_background;}
      u_long get_title_color(void) const {return graph_title_color;}
      u_long get_gridline_color(void) const {return graph_gridline;}
      u_long get_shadow_color(void) const {return graph_shadow;}
      u_long get_hits_color(void) const {return graph_hits_color;}
      u_long get_files_color(void) const {return graph_files_color;}
      u_long get_pages_color(void) const {return graph_pages_color;}
      u_long get_visits_color(void) const {return graph_visits_color;}
      u_long get_hosts_color(void) const {return graph_hosts_color;}
      u_long get_volume_color(void) const {return graph_volume_color;}
      u_long get_outline_color(void) const {return graph_outline_color;}
      u_long get_legend_color(void) const {return graph_legend_color;}
      u_long get_weekend_color(void) const {return graph_weekend_color;}

      // return true if the respective color is defined in the configuration
      bool is_default_background_color(void) const {return config.graph_background & DEFCOLOR ? true : false;}
      bool is_default_title_color(void) const {return config.graph_title_color & DEFCOLOR ? true : false;}
      bool is_default_gridline_color(void) const {return config.graph_gridline & DEFCOLOR ? true : false;}
      bool is_default_shadow_color(void) const {return config.graph_shadow & DEFCOLOR ? true : false;}
      bool is_default_hits_color(void) const {return config.graph_hits_color & DEFCOLOR ? true : false;}
      bool is_default_files_color(void) const {return config.graph_files_color & DEFCOLOR ? true : false;}
      bool is_default_pages_color(void) const {return config.graph_pages_color & DEFCOLOR ? true : false;}
      bool is_default_visits_color(void) const {return config.graph_visits_color & DEFCOLOR ? true : false;}
      bool is_default_hosts_color(void) const {return config.graph_hosts_color & DEFCOLOR ? true : false;}
      bool is_default_volume_color(void) const {return config.graph_volume_color & DEFCOLOR ? true : false;}
      bool is_default_outline_color(void) const {return config.graph_outline_color & DEFCOLOR ? true : false;}
      bool is_default_legend_color(void) const {return config.graph_legend_color & DEFCOLOR ? true : false;}
      bool is_default_weekend_color(void) const {return config.graph_weekend_color & DEFCOLOR ? true : false;}
            
      void init_graph_engine(void);
      void cleanup_graph_engine(void);

      int year_graph6x(const history_t& history, const char *fname, const char *title, u_int& graph_width, u_int& graph_height);
      int pie_chart(const char *fname, const char *title, u_long t_val, const u_long data1[], const char *legend[]);
      int month_graph6(const char *fname, const char *title, int month, int year, const daily_t daily[31]);
      int day_graph3(const char *fname, const char *title, const hourly_t hourly[24]);
};

#endif  /* _GRAPHS_H */
