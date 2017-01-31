/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2017, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   This software uses the gd graphics library, which is copyright by
   Quest Protein Database Center, Cold Spring Harbor Labs.  Please
   see the documentation supplied with the library for additional
   information and license terms, or visit www.boutell.com/gd/ for the
   most recent version of the library and supporting documentation.

   graphs.cpp  - produces graphs used by the Webalizer
*/
#include "pch.h"

#include <math.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/types.h>

#include <gd.h>
#include <gdfonts.h>
#include <gdfontmb.h>

#include "lang.h"
#include "graphs.h"
#include "util.h"
#include "tstamp.h"
#include "history.h"

/* Some systems don't define this */
#ifndef PI
#define PI 3.14159265358979323846
#endif

// color component extractors
#define ALPHA(color)    (((uint32_t) color & (uint32_t) 0x7F000000u) >> 24)   // GD uses a 7-bit alpha channel
#define RED(color)      (((uint32_t) color & (uint32_t) 0x00FF0000u) >> 16)
#define GREEN(color)    (((uint32_t) color & (uint32_t) 0x0000FF00u) >> 8)
#define BLUE(color)     (((uint32_t) color & (uint32_t) 0x000000FFu))

#define CX     156                           // center x (for pie)
#define CY     150                           // center y  (chart)
#define XRAD   241                           // X-axis radius
#define YRAD   201                           // Y-axis radius

#define MT     24                            // graph margin top
#define MR     20                            // graph margin right
#define MB     20                            // graph margin bottom
#define ML     19                            // graph margin left

#define GBW    2                             // graph border width
#define SBW    GBW                           // section border width

#define YSPL   5                             // yearly graph section padding left
#define YSPR   2                             // yearly graph section padding right
#define YSPT   2                             // yearly graph section padding top
#define YSPB   2                             // yearly graph section padding bottom
#define YSSW   23                            // yearly graph section slot width

#define YSBW   14                            // yearly graph section bar width

#define YGH    256                           // yearly graph height
#define YGHH   (MT+GBW+(YGH-MT-GBW*2-MB)/2)  // yearly graph half height
#define YSPH   (YGH-MT-MB-YSPT-YSPB-GBW*2)   // yearly graph section plot area height

#define MSPL   3                             // mini section padding left
#define MSPR   0                             // mini section padding right
#define MSPT   1                             // mini section padding top
#define MSPB   1                             // mini section padding bottom
#define MSSW   15                            // mini section slot width

#define MSTBW  8                             // mini section bar width (top)
#define MSBBW  11                            // mini section bar width (bottom)

#define MSH    ((YGH-MT-GBW*3-MB)/2)         // mini section height
#define MSPH   (MSH-MSPT-MSPB)               // mini section plot area height

#define HSPT   2                             // hourly graph section padding top
#define HSPR   9                             // hourly graph section padding right
#define HSPB   2                             // hourly graph section padding bottom
#define HSPL   9                             // hourly graph section padding left

#define HGW    512                           // hourly graph width
#define HGH    340                           // hourly graph height
#define HGSW   19                            // hourly graph slot width (bar+space)
#define HGSP(bw) ((HGSW-bw)>>1)              // hourly graph slot padding (bar width)

#define GD_FONT_SMALL         1
#define GD_FONT_MEDIUM        2
#define GD_FONT_MEDIUM_BOLD   3

//
//
//
const char *graph_t::numchar[] = {" 0"," 1"," 2"," 3"," 4"," 5"," 6"," 7"," 8"," 9","10",
                                  "11","12","13","14","15","16","17","18","19","20",
                                  "21","22","23","24","25","26","27","28","29","30","31"};

// use a non-const buffer to avoid compiler warnings (GD lacks const qualifiers)
static char uppercase_x[] = "X";

//
//
//
graph_t::graph_t(const config_t& config) : 
      config(config), 
      xfer_fmt_buf(128), 
      buffer_formatter(xfer_fmt_buf, xfer_fmt_buf.capacity(), buffer_formatter_t::overwrite)
{
   out = NULL;
   im = NULL;

   *maxvaltxt = 0;

   percent = 0.;
   font_size_small_px = 0;
   font_size_medium_px = 0;
   font_size_medium_bold_px = 0;

   graph_background = 0xE0E0E0;
   graph_title_color = 0x0000FF;
   graph_gridline = 0x808080;
   graph_shadow = 0x333333;
   graph_hits_color = 0x00805C;
   graph_files_color = 0x0000FF;
   graph_hosts_color = 0xFF8000;
   graph_pages_color = 0x00C0FF;
   graph_visits_color = 0xFFFF00;
   graph_xfer_color = 0xFF0000;
   graph_outline_color = 0x000000;
   graph_legend_color = 0x000000;
   graph_weekend_color = 0x00805C;
}

graph_t::~graph_t(void)
{
}

const char *graph_t::fmt_xfer(uint64_t xfer)
{
   auto fmt_kbyte = [](string_t::char_buffer_t& buffer, double xfer) -> size_t
   {
      // buffer_formatter is never in the append mode in graph_t, so we never go beyond its capacity 
      return snprintf(buffer, buffer.capacity(), "%.0f", xfer) + 1;
   };

   // check if we need to output classic transfer amounts without a human-readable suffix
   if(config.classic_kbytes)
      return buffer_formatter.format(fmt_kbyte, xfer / (config.decimal_kbytes ? 1000. : 1024.));

   // buffer_formatter_t::format always returns a holder buffer, so we can return a pointer to the buffer memory
   return buffer_formatter.format(fmt_hr_num, xfer, " ", config.lang.msg_unit_pfx, config.lang.msg_xfer_unit, config.decimal_kbytes);
}

//
//
//
void graph_t::_gdImageString(gdImagePtr im, int fonttype, int x, int y, const char *str, int color, bool xyhead, u_int *textsize)
{
   _gdImageStringEx(im, fonttype, x, y, (u_char*) str, color, false, xyhead, textsize);
}

void graph_t::_gdImageStringUp(gdImagePtr im, int fonttype, int x, int y, const char *str, int color, bool xyhead, u_int *textsize)
{
   _gdImageStringEx(im, fonttype, x, y, (u_char*) str, color, true, xyhead, textsize);
}

void graph_t::_gdImageStringEx(gdImagePtr im, int fonttype, int x, int y, u_char *str, int color, bool up, bool xyhead, u_int *textsize)
{
   int brect[8];
   double ptsize = 0;
   int pxsize = 0, charpx = 0;
   size_t strsize;
   gdFontPtr fontptr = NULL;
   const char *fontfile = NULL;
   double angle = up ? PI/2 : 0.;

   switch (fonttype) {
      case GD_FONT_SMALL:
         fontfile = config.font_file_normal;
         ptsize = config.font_size_small;
         pxsize = font_size_small_px;
         fontptr = gdFontGetSmall();
         charpx = fontptr ? fontptr->w : 6;
         break;
      case GD_FONT_MEDIUM:
         fontfile = config.font_file_normal;
         ptsize = config.font_size_medium;
         pxsize = font_size_medium_px;
         fontptr = gdFontGetMediumBold();
         charpx = fontptr ? fontptr->w : 7;
         break;
      case GD_FONT_MEDIUM_BOLD:
         fontfile = config.font_file_bold;
         ptsize = config.font_size_medium;
         pxsize = font_size_medium_bold_px;
         fontptr = gdFontGetMediumBold();
         charpx = fontptr ? fontptr->w : 7;
         break;
   }

   /* use raster fonts? */
   if(fontfile == NULL || *fontfile == 0 || pxsize == 0) {
      strsize = strlen((const char*) str) * charpx;
      if(up) {
         if(!xyhead)
            y += (int) strsize;
         gdImageStringUp(im, fontptr, x, y, str, color);
      }
      else {
         if(!xyhead)
            x -= (int) strsize;
         gdImageString(im, fontptr, x, y, str, color);
      }
      if(textsize)
         *textsize = (u_int) strsize + 1;   /* 1px padding */
      return;
   }

   /*
   // FT coordinates must point to the left bottom corner
   */
   if(!up) y += pxsize; else x += pxsize;

   /*
   // If the string is right-aligned, compute the bounding box first
   */
   if(!xyhead) {
      gdImageStringFT(NULL, brect, config.font_anti_aliasing ? color : -color, (char*) fontfile, ptsize, angle, x, y, (char*) str);
      if(textsize)
         *textsize = (up) ? brect[1] - brect[3] : brect[2] - brect[0];
      if(up) y += (brect[1] - brect[3]); else x -= (brect[2] - brect[0]);
   }

   gdImageStringFT(im, brect, config.font_anti_aliasing ? color : -color, (char*) fontfile, ptsize, angle, x, y, (char*) str);

   if(textsize && xyhead)
      *textsize = (up) ? brect[1] - brect[3] + 1 : brect[2] - brect[0] + 1;   /* 1px padding */
}


/*****************************************************************/
/*                                                               */
/* YEAR_GRAPH6x  - Year graph with six data sets                 */
/*                                                               */
/*****************************************************************/

int graph_t::year_graph6x(const history_t& history,
                          const char *fname,        /* file name use      */
                          const char *title,        /* title for graph    */
                          u_int& graph_width,
                          u_int& graph_height)
{
   u_int i;
   int x1,y1,x2;
   u_int s_mth;
   u_int textsize, offset;
   uint64_t maxval=1;
   uint64_t fmaxval=0;
   u_int sleft, sright, msleft, msright;
   const hist_month_t *hptr;
   history_t::const_iterator iter;

   graph_width = ML+GBW+YSPL + YSSW*history.disp_length() + YSPR + SBW+MSPL + MSSW*history.disp_length() + MSPR+GBW+MR;
   graph_height = YGH;

   /* initalize the graph */
   init_graph(title, graph_width, graph_height);

   sleft = ML+GBW;
   sright = ML+GBW+YSPL + YSSW*history.disp_length() + YSPR - 1;
   msleft = ML+GBW+YSPL + YSSW*history.disp_length() + YSPR;
   msright = msleft + SBW+MSPL + MSSW*history.disp_length() + MSPR;

   /* draw section lines */
   gdImageLine(im, msleft, MT+GBW, msleft, YGH-MB-GBW, white);
   gdImageLine(im, msleft+1, MT+GBW, msleft+1, YGH-MB-GBW, black);  
   gdImageLine(im, msleft, YGHH-1, msright, YGHH-1, white);
   gdImageLine(im, msleft+1, YGHH, msright, YGHH, black);

   msleft += SBW;

   /* index lines? */
   if (config.graph_lines)
   {
      y1 = YSPH / (config.graph_lines+1);
      for(i = 1; i <= config.graph_lines; i++)
         gdImageLine(im, sleft, MT+GBW+YSPT + i*y1, sright, MT+GBW+YSPT + i*y1, c_gridline);
      y1 = MSPH / (config.graph_lines+1);
      for(i = 1; i <= config.graph_lines; i++)
         gdImageLine(im, msleft, MT+GBW+MSPT + i*y1, msright, MT+GBW+MSPT + i*y1, c_gridline);
      for(i = 1; i <= config.graph_lines; i++)
         gdImageLine(im, msleft, YGH-MB-GBW-MSPB - i*y1 - 1, msright, YGH-MB-GBW-MSPB - i*y1 - 1, c_gridline);
   }

   /* x-axis legend */
   s_mth = history.first_month();
   for(i = 0; i < history.disp_length(); i++) {
      /* use language-specific array */
      _gdImageString(im, GD_FONT_SMALL, ML+GBW+YSPL+(i*YSSW), 238, lang_t::s_month[s_mth-1], c_legend, true, NULL);
      s_mth++;
      if (s_mth > 12) s_mth = 1;
   }

   // find maximum value
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      if(hptr->hits > maxval) maxval = hptr->hits;
      if(hptr->files > maxval) maxval = hptr->files;
      if(hptr->pages > maxval) maxval = hptr->pages;
   }

   // y-axis legend
   if (maxval <= 0) maxval = 1;
   sprintf(maxvaltxt, "%" PRIu64 "", maxval);
   _gdImageStringUp(im, GD_FONT_SMALL, ML-font_size_small_px-1, MT+GBW, maxvaltxt, c_legend, false, NULL);

   if (config.graph_legend)                          /* print color coded legends? */
   {
      /* Kbytes Legend */
      _gdImageString(im, GD_FONT_SMALL, msright+1, YGH-MB+2, config.lang.msg_h_xfer, c_shadow, false, NULL);
      _gdImageString(im, GD_FONT_SMALL, msright, YGH-MB+1, config.lang.msg_h_xfer, c_xfer, false, NULL);

      /* Hosts/Visits Legend */
      _gdImageString(im, GD_FONT_SMALL, msright+1, MT-font_size_small_px-2, config.lang.msg_h_hosts, c_shadow, false, NULL);
      _gdImageString(im, GD_FONT_SMALL, msright, MT-font_size_small_px-3, config.lang.msg_h_hosts, c_hosts, false, &textsize);
      offset = textsize;
      _gdImageString(im, GD_FONT_SMALL, msright-offset+1, MT-font_size_small_px-2, "/", c_shadow, false, NULL);
      _gdImageString(im, GD_FONT_SMALL, msright-offset, MT-font_size_small_px-3, "/", c_legend, false, &textsize);
      offset += textsize;
      _gdImageString(im, GD_FONT_SMALL, msright-offset+1, MT-font_size_small_px-2, config.lang.msg_h_visits, c_shadow, false, NULL);
      _gdImageString(im, GD_FONT_SMALL, msright-offset, MT-font_size_small_px-3, config.lang.msg_h_visits, c_visits, false, NULL);

      /* Hits/Files/Pages Legend */
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-3, YGH-MB-GBW+1, config.lang.msg_h_pages, c_shadow, true, NULL);
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-4, YGH-MB-GBW, config.lang.msg_h_pages, c_pages, true, &textsize);
      offset = textsize;
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-3, YGH-MB-GBW+1-offset, "/", c_shadow, true, NULL);
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-4, YGH-MB-GBW-offset, "/", c_legend, true, &textsize);
      offset += textsize;
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-3, YGH-MB-GBW+1-offset, config.lang.msg_h_files, c_shadow, true, NULL);
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-4, YGH-MB-GBW-offset, config.lang.msg_h_files, c_files, true, &textsize);
      offset += textsize;
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-3, YGH-MB-GBW+1-offset, "/",c_shadow, true, NULL);
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-4, YGH-MB-GBW-offset, "/",c_legend, true, &textsize);
      offset += textsize;
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-3, YGH-MB-GBW+1-offset, config.lang.msg_h_hits, c_shadow, true, NULL);
      _gdImageStringUp(im,GD_FONT_SMALL, ML-font_size_small_px-4, YGH-MB-GBW-offset, config.lang.msg_h_hits, c_hits, true, &textsize);
   }

   /* hits */
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      i = history.month_index(hptr->year, hptr->month);
      percent = ((double)hptr->hits / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = ML+GBW+YSPL + (i*YSSW);
      x2 = x1 + (YSBW-1);
      y1 = YGH-MB-GBW-YSPB - (int) ((percent * (double)YSPH)+.5);
      if(y1 > YGH-MB-GBW-YSPB-1) continue;
      draw_graph_bar(im, x1, y1, x2, YGH-MB-GBW-YSPB-1, c_hits);
   }

   /* files */
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      i = history.month_index(hptr->year, hptr->month);
      percent = ((double)hptr->files / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = ML+GBW+YSPL+3 + (i*YSSW);
      x2 = x1 + (YSBW-1);
      y1 = YGH-MB-GBW-YSPB - (int) ((percent * (double)YSPH)+.5);
      if(y1 > YGH-MB-GBW-YSPB-1) continue;
      draw_graph_bar(im, x1, y1, x2, YGH-MB-GBW-YSPB-1, c_files);
   }

   /* pages */
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      i = history.month_index(hptr->year, hptr->month);
      percent = ((double)hptr->pages / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = ML+GBW+YSPL+6 + (i*YSSW);
      x2 = x1 + (YSBW-1);
      y1 = YGH-MB-GBW-YSPB - (int) ((percent * (double)YSPH)+.5);
      if(y1 > YGH-MB-GBW-YSPB-1) continue;
      draw_graph_bar(im, x1, y1, x2, YGH-MB-GBW-YSPB-1, c_pages);
   }

   maxval=0;
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      if (hptr->hosts > maxval) maxval = hptr->hosts;           /* get max val    */
      if (hptr->visits > maxval) maxval = hptr->visits;
   }
   if (maxval <= 0) maxval = 1;
   sprintf(maxvaltxt, "%" PRIu64 "", maxval);
   _gdImageStringUp(im, GD_FONT_SMALL, graph_width-MR+1, MT, maxvaltxt, c_legend, false, NULL);

   /* visits */
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      i = history.month_index(hptr->year, hptr->month);
      percent = ((double)hptr->visits / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = msleft + MSPL + (i*MSSW);
      x2 = x1 + (MSTBW-1);
      y1 = MT+GBW+MSPT+MSPH - (int) ((percent * (double)MSPH)+.5);
      if(y1 > MT+GBW+MSPT+MSPH-1) continue;
      draw_graph_bar(im, x1, y1, x2, MT+GBW+MSPT+MSPH-1, c_visits);
   }

   /* hosts */
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      i = history.month_index(hptr->year, hptr->month);
      percent = ((double)hptr->hosts / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = msleft+4 + MSPL + (i*MSSW);
      x2 = x1 + (MSTBW-1);
      y1 = MT+GBW+MSPT+MSPH - (int) ((percent * (double)MSPH)+.5);
      if(y1 > MT+GBW+MSPT+MSPH-1) continue;
      draw_graph_bar(im, x1, y1, x2, MT+GBW+MSPT+MSPH-1, c_hosts);
   }

   fmaxval=0;
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      if(hptr->xfer > fmaxval) fmaxval = hptr->xfer;         /* get max val    */
   }
   if (fmaxval <= 0) fmaxval = 1;
   _gdImageStringUp(im, GD_FONT_SMALL, graph_width-MR+1, YGHH, fmt_xfer(fmaxval), c_legend, false, NULL);

   /* transfer */
   iter = history.begin();
   while(iter != history.end()) {
      hptr = &*iter++;
      i = history.month_index(hptr->year, hptr->month);
      percent = ((double)hptr->xfer / (double)fmaxval);
      if (percent <= 0.0) continue;
      x1 = msleft + MSPL + (i*MSSW);
      x2 = x1 + (MSBBW-1);
      y1 = YGH-MB-GBW-MSPT - (int) ((percent * (double)MSPH)+.5);
      if(y1 > YGH-MB-GBW-MSPT-1) continue;
      draw_graph_bar(im, x1, y1, x2, YGH-MB-GBW-MSPT-1, c_xfer);
   }

   /* save png image */
   if ((out = fopen(make_path(config.out_dir, fname), "wb")) != NULL)
   {
      gdImagePng(im, out);
      fclose(out);
   }
   /* deallocate memory */
   gdImageDestroy(im);

   return (0);
}

/*****************************************************************/
/*                                                               */
/* MONTH_GRAPH6  - Month graph with six data sets                */
/*                                                               */
/*****************************************************************/

int graph_t::month_graph6(const char *fname,          // filename
                          const char *title,          // graph title
                          int month,                  // graph month
                          int year,                   // graph year
                          const daily_t daily[31])    // daily data
{
   u_int i;
   int x1,y1,x2;
   uint64_t maxval=0;
   uint64_t fmaxval=0;
   u_int offset, textsize;
   uint64_t wday;

   // get the week day of the first day of the month
   wday = tstamp_t::wday(tstamp_t::jday(year, month, 1));

   /* initalize the graph */
   init_graph(title,512,400);

   draw_section_line(im, 512, 179);
   draw_section_line(im, 512, 279);

   /* index lines? */
   if (config.graph_lines)
   {
      y1=154/(config.graph_lines+1);
      for (i=0;i<config.graph_lines;i++)
         draw_grid_line(im, 512, ((i+1)*y1)+25);
      y1=100/(config.graph_lines+1);
      for (i=0;i<config.graph_lines;i++)
         draw_grid_line(im, 512, ((i+1)*y1)+180);
      for (i=0;i<config.graph_lines;i++)
         draw_grid_line(im, 512, ((i+1)*y1)+280);
   }

   /* x-axis legend */
   for (i=0;i<31;i++)
   {
      if((wday+i) % 7 == 6 || (wday+i) % 7 == 0)
         _gdImageString(im, GD_FONT_SMALL, 25+(i*15), 382, numchar[i+1], c_weekend, true, NULL);
      else
         _gdImageString(im, GD_FONT_SMALL, 25+(i*15), 382, numchar[i+1], c_legend, true, NULL);
   }

   /* y-axis legend */
   for (i=0; i<31; i++)
   {
       if (daily[i].tm_hits > maxval) maxval = daily[i].tm_hits;       /* get max val    */
       if (daily[i].tm_files > maxval) maxval = daily[i].tm_files;
       if (daily[i].tm_pages > maxval) maxval = daily[i].tm_pages;
   }
   if (maxval <= 0) maxval = 1;
   sprintf(maxvaltxt, "%" PRIu64 "", maxval);
   _gdImageStringUp(im, GD_FONT_SMALL, 8, 26, maxvaltxt, c_legend, false, NULL);

   if (config.graph_legend)                           /* Print color coded legends? */
   {
      /* Kbytes Legend */
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 376, config.lang.msg_h_xfer, c_shadow, true, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 375, config.lang.msg_h_xfer, c_xfer, true, NULL);

      /* Sites/Visits Legend */
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 276, config.lang.msg_h_hosts, c_shadow, true, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 275, config.lang.msg_h_hosts,c_hosts, true, &textsize);
      offset = textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 276-offset, "/", c_shadow, true, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 275-offset, "/", c_legend, true, &textsize);
      offset += textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 276-offset, config.lang.msg_h_visits, c_shadow, true, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 275-offset, config.lang.msg_h_visits, c_visits, true, &textsize);

      /* Pages/Files/Hits Legend */
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26, config.lang.msg_h_pages, c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25, config.lang.msg_h_pages, c_pages, false, &textsize);
      offset = textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, "/", c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, "/", c_legend, false, &textsize);
      offset += textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, config.lang.msg_h_files, c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, config.lang.msg_h_files, c_files, false, &textsize);
      offset += textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, "/", c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, "/", c_legend, false, &textsize);
      offset += textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, config.lang.msg_h_hits, c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, config.lang.msg_h_hits, c_hits, false, &textsize);
   }

   /* data1 */
   for (i=0; i<31; i++)
   {
      percent = ((double) daily[i].tm_hits / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = 25 + (i*15);
      x2 = x1 + 7;
      y1 = (int) (176. - (percent * 147.));
      draw_graph_bar(im, x1, y1, x2, 176, c_hits);
   }

   /* data2 */
   for (i=0; i<31; i++)
   {
      percent = ((double) daily[i].tm_files / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = 27 + (i*15);
      x2 = x1 + 7;
      y1 = (int) (176. - (percent * 147.));
      draw_graph_bar(im, x1, y1, x2, 176, c_files);
   }

   /* data5 */
   for (i=0; i<31; i++)
   {
      if (daily[i].tm_pages == 0) continue;
      percent = ((double) daily[i].tm_pages / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = 29 + (i*15);
      x2 = x1 + 7;
      y1 = (int) (176. - (percent * 147.));
      draw_graph_bar(im, x1, y1, x2, 176, c_pages);
   }

   /* sites / visits */
   maxval=0;
   for (i=0; i<31; i++)
   {
      if (daily[i].tm_hosts > maxval) maxval = daily[i].tm_hosts;
      if (daily[i].tm_visits > maxval) maxval = daily[i].tm_visits;
   }
   if (maxval <= 0) maxval = 1;
   sprintf(maxvaltxt, "%" PRIu64 "", maxval);
   _gdImageStringUp(im, GD_FONT_SMALL, 8, 180, maxvaltxt, c_legend, false, NULL);
   
   /* data 6 */
   for (i=0; i<31; i++)
   {
      percent = ((double) daily[i].tm_visits / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = 25 + (i*15);
      x2 = x1 + 8;
      y1 = (int) (276. - (percent * 92.));
      draw_graph_bar(im, x1, y1, x2, 276, c_visits);
   }

   /* data 3 */
   for (i=0; i<31; i++)
   {
      percent = ((double)daily[i].tm_hosts / (double)maxval);
      if (percent <= 0.0) continue;
      x1 = 29 + (i*15);
      x2 = x1 + 7;
      y1 = (int) (276. - (percent * 92.));
      draw_graph_bar(im, x1, y1, x2, 276, c_hosts);
   }

   /* data4 */
   fmaxval=0;
   for (i=0; i<31; i++)
      if (daily[i].tm_xfer > fmaxval) fmaxval = daily[i].tm_xfer;
   if (fmaxval <= 0) fmaxval = 1;
   _gdImageStringUp(im, GD_FONT_SMALL, 8, 280, fmt_xfer(fmaxval), c_legend, false, NULL);
   
   for (i=0; i<31; i++)
   {
      percent = (double) daily[i].tm_xfer / (double) fmaxval;
      if (percent <= 0.0) continue;
      x1 = 26 + (i*15);
      x2 = x1 + 10;
      y1 = (int) (375. - ( percent * 91.));
      draw_graph_bar(im, x1, y1, x2, 375, c_xfer);
   }

   /* open file for writing */
   if ((out = fopen(make_path(config.out_dir, fname), "wb")) != NULL)
   {
      gdImagePng(im, out);
      fclose(out);
   }
   /* deallocate memory */
   gdImageDestroy(im);

   return (0);
}

/*****************************************************************/
/*                                                               */
/* DAY_GRAPH3  - Day graph with three data sets                  */
/*                                                               */
/*****************************************************************/

int graph_t::day_graph3(const char *fname,
               const char *title,
               const hourly_t hourly[24])
{
   u_int i;
   int x1,y1,x2, baridx;
   uint64_t maxval=0, maxfer = 0;
   u_int offset, textsize;
   uint64_t xfer_ul[24];
   uint64_t data1[24], data2[24], data3[24];
   u_int colors[] = {c_hits, c_files, c_pages};
   const uint64_t *data[] = {data1, data2, data3};

   /* initalize the graph */
   init_graph(title, 512, 340);

   // copy data into local arrays to make it easier to address elements
   for(i = 0; i < 24; i++) {
      data1[i] = hourly[i].th_hits;
      data2[i] = hourly[i].th_files;
      data3[i] = hourly[i].th_pages;
      xfer_ul[i] = hourly[i].th_xfer;
   }

   draw_section_line(im, 512, 220);

   /* index lines? */
   if (config.graph_lines)
   {
      y1=194/(config.graph_lines+1);
      for (i=0;i<config.graph_lines;i++)
         draw_grid_line(im, 512, ((i+1)*y1)+25);

      y1=92/(config.graph_lines+1);
      for(i = 0; i < config.graph_lines; i++)
         draw_grid_line(im, 512, ((i+1)*y1)+223);
   }

   /* x-axis legend */
   for (i=0;i<24;i++)
   {
      _gdImageString(im, GD_FONT_SMALL, ML+SBW+HGSW+8+(i*HGSW), 324, numchar[i], c_legend, false, NULL);
      /* get max val    */
      if (hourly[i].th_hits > maxval) maxval = hourly[i].th_hits;
      if (hourly[i].th_files > maxval) maxval = hourly[i].th_files;
      if (hourly[i].th_pages > maxval) maxval = hourly[i].th_pages;
      if (xfer_ul[i] > maxfer) maxfer = xfer_ul[i];
   }

   if (maxval <= 0) maxval = 1;
   if (maxfer <= 0) maxfer = 1;

   sprintf(maxvaltxt, "%" PRIu64 "", maxval);
   _gdImageStringUp(im, GD_FONT_SMALL, 8, 26, maxvaltxt, c_legend, false, NULL);
   
   _gdImageStringUp(im, GD_FONT_SMALL, 8, 222, fmt_xfer(maxfer), c_legend, false, NULL);

   if (config.graph_legend)                          /* print color coded legends? */
   {
      /* Pages/Files/Hits Legend */
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26, config.lang.msg_h_pages, c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25, config.lang.msg_h_pages, c_pages, false, &textsize);
      offset = textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, "/", c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, "/", c_legend, false, &textsize);
      offset += textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, config.lang.msg_h_files, c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, config.lang.msg_h_files, c_files, false, &textsize);
      offset += textsize;                       
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, "/", c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, "/", c_legend, false, &textsize);
      offset += textsize;
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 26+offset, config.lang.msg_h_hits, c_shadow, false, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 25+offset, config.lang.msg_h_hits, c_hits, false, &textsize);

      // KBytes
      _gdImageStringUp(im, GD_FONT_SMALL, 494, 316, config.lang.msg_h_xfer, c_shadow, true, NULL);
      _gdImageStringUp(im, GD_FONT_SMALL, 493, 315, config.lang.msg_h_xfer, c_xfer, true, &textsize);
      offset = textsize;
   }

   // Hits/Files/Pages
   for(baridx = 0, offset = 0; baridx < sizeof(data)/sizeof(data[0]); baridx++, offset += 3) {
      for (i=0; i < 24; i++) {
         if(data[baridx][i] == 0) continue;
         percent = (double)maxval/(double)data[baridx][i];
         // 2 bars overlapping by 3 pixels = 6; bar width = 9; combined bar width = 15
         x1 = ML+SBW+HSPL + HGSP(15) + 6 + (i*HGSW) - offset;
         x2 = x1 + 8;
         y1 = (int) (217. - (189./percent));
         draw_graph_bar(im, x1, y1, x2, 217, colors[baridx]);
      }
   }

   // KBytes
   for(i = 0, offset = 0; i < 24; i++) {
      if(xfer_ul[i] == 0) continue;
      percent = (double)maxfer/(double)xfer_ul[i];
      x1 = ML+SBW+HSPL + HGSP(11) + (i*HGSW);
      x2 = x1 + 10;
      y1 = (int) (316. - (92./percent));
      draw_graph_bar(im, x1, y1, x2, 315, c_xfer);
   }

   /* save as png file */
   if ( (out = fopen(make_path(config.out_dir, fname), "wb")) != NULL)
   {
      gdImagePng(im, out);
      fclose(out);
   }
   /* deallocate memory */
   gdImageDestroy(im);

   return (0);
}

/*****************************************************************/
/*                                                               */
/* PIE_CHART  - draw a pie chart (10 data items max)             */
/*                                                               */
/*****************************************************************/

int graph_t::pie_chart(const char *fname, const char *title, uint64_t t_val, const uint64_t data1[], const char *legend[])
{
   u_int *pie_colors[] = {&green, &orange, &blue, &red, &cyan, &yellow, &purple, &ltgreen, &ltpurple, &brown};
   double percent, t_percent = 0.;
   u_int i, y;
   u_int s_arc = 0, e_arc = 0;
   u_int ps_arc = 0, pe_arc = 0;
   u_int maxslices = sizeof(pie_colors)/sizeof(pie_colors[0]);
   string_t str;

   if(font_size_medium_bold_px)
      y = 150 - (font_size_medium_bold_px * 10) - font_size_medium_bold_px + (font_size_medium_bold_px >> 1);
   else
      y = 47;

   /* init graph and colors */
   init_graph(title, 512, 300);
   purple  = gdImageColorAllocate(im, 128, 0, 128);
   ltgreen = gdImageColorAllocate(im, 128, 255, 192);
   ltpurple= gdImageColorAllocate(im, 255, 0, 255);
   brown   = gdImageColorAllocate(im, 255, 196, 128);

   gdImageSetAntiAliased(im, black);

   /* do the c_shadow... */
   gdImageFilledArc(im, CX, CY+7, XRAD, YRAD, 2, 178, gdAntiAliased, gdArc);

   /* slice the pie */
   for (i = 0; i < maxslices && t_percent < 100.; i++) {
      if (data1[i] == 0)           /* make sure valid slice       */
         continue;

      if((percent = ((double)data1[i] * 100.0)/(double)t_val) < 1.)
         break;

      t_percent += percent;

      /* draw the current slice */
      e_arc = s_arc + (u_int) ((percent * 360.)/100. + .5);
      gdImageFilledArc(im, CX, CY, XRAD, YRAD, s_arc, e_arc, *pie_colors[i], gdArc);

      /* outline the previous even slice */
      if(i > 0 && i & 1)
         gdImageFilledArc(im, CX, CY, XRAD, YRAD, ps_arc, pe_arc, gdAntiAliased, gdEdged | gdNoFill);

      ps_arc = s_arc; pe_arc = e_arc;
      s_arc = e_arc;

      /* print the legend */
      str.format("%s (%.0f%%)",legend[i], percent);
      _gdImageString(im, GD_FONT_MEDIUM_BOLD, 481, y+1, str.c_str(), c_shadow, false, NULL);
      _gdImageString(im, GD_FONT_MEDIUM_BOLD, 480, y, str.c_str(), *pie_colors[i], false, NULL);

      /* move y by two font heights */
      if(font_size_medium_bold_px)
         y += (font_size_medium_bold_px << 1);
      else
         y += 20;
   }

   /* outline the last even slice */
   if(i > 0 && i & 1)
      gdImageFilledArc(im, CX, CY, XRAD, YRAD, ps_arc, pe_arc, gdAntiAliased, gdEdged | gdNoFill);

   /* anything left over? */
   if(s_arc < 360) {
      gdImageFilledArc(im, CX, CY, XRAD, YRAD, s_arc, 360, white, gdArc);
      gdImageFilledArc(im, CX, CY, XRAD, YRAD, s_arc, 360, gdAntiAliased, gdEdged | gdNoFill);

      percent = 100. - t_percent;

      str.format("%s (%.*f%%)", config.lang.msg_h_other, (percent < 1 ? 2 : 0), percent);
      _gdImageString(im, GD_FONT_MEDIUM_BOLD, 481, y+1, str.c_str(), c_shadow, false, NULL);
      _gdImageString(im, GD_FONT_MEDIUM_BOLD, 480, y, str.c_str(), white, false, NULL);
   }
   else if(i > 0 && !(i & 1)) {
      gdImageFilledArc(im, CX, CY, XRAD, YRAD, ps_arc, 360, gdAntiAliased, gdEdged | gdNoFill);
   }

   /* outline the pie */
   gdImageArc(im, CX, CY, XRAD, YRAD, 0, 360, gdAntiAliased);

   /* save png image */
   if ((out = fopen(make_path(config.out_dir, fname), "wb")) != NULL)
   {
      gdImagePng(im, out);
      fclose(out);
   }

   /* deallocate memory */
   gdImageDestroy(im);

   return (0);
}

/*****************************************************************/
/*                                                               */
/* INIT_GRAPH  - initalize graph and draw borders                */
/*                                                               */
/*****************************************************************/

void graph_t::init_graph(const char *title, int xsize, int ysize)
{
   int c_title;

   if(config.graph_true_color)
      im = gdImageCreateTrueColor(xsize,ysize);
   else
      im = gdImageCreate(xsize,ysize);

   /* allocate color maps, background color first */
   c_background = gdImageColorAllocateAlpha(im, RED(graph_background), GREEN(graph_background), BLUE(graph_background), ALPHA(graph_background));
   c_shadow  = gdImageColorAllocate(im, RED(graph_shadow), GREEN(graph_shadow), BLUE(graph_shadow));
   c_gridline = gdImageColorAllocate(im, RED(graph_gridline), GREEN(graph_gridline), BLUE(graph_gridline));
   c_title = gdImageColorAllocate(im, RED(graph_title_color), GREEN(graph_title_color), BLUE(graph_title_color));
   c_hits = gdImageColorAllocate(im, RED(graph_hits_color), GREEN(graph_hits_color), BLUE(graph_hits_color));
   c_files = gdImageColorAllocate(im, RED(graph_files_color), GREEN(graph_files_color), BLUE(graph_files_color));
   c_hosts = gdImageColorAllocate(im, RED(graph_hosts_color), GREEN(graph_hosts_color), BLUE(graph_hosts_color));
   c_pages = gdImageColorAllocate(im, RED(graph_pages_color), GREEN(graph_pages_color), BLUE(graph_pages_color));
   c_visits = gdImageColorAllocate(im, RED(graph_visits_color), GREEN(graph_visits_color), BLUE(graph_visits_color));
   c_xfer = gdImageColorAllocate(im, RED(graph_xfer_color), GREEN(graph_xfer_color), BLUE(graph_xfer_color));
   c_outline = gdImageColorAllocate(im, RED(graph_outline_color), GREEN(graph_outline_color), BLUE(graph_outline_color));
   c_legend = gdImageColorAllocate(im, RED(graph_legend_color), GREEN(graph_legend_color), BLUE(graph_legend_color));
   c_weekend = gdImageColorAllocate(im, RED(graph_weekend_color), GREEN(graph_weekend_color), BLUE(graph_weekend_color));

   dkgrey  = gdImageColorAllocate(im, 128, 128, 128);
   black   = gdImageColorAllocate(im, 0, 0, 0);
   white   = gdImageColorAllocate(im, 255, 255, 255);
   green   = gdImageColorAllocate(im, 0, 128, 92);
   orange  = gdImageColorAllocate(im, 255, 128, 0);
   blue    = gdImageColorAllocate(im, 0, 0, 255);
   red     = gdImageColorAllocate(im, 255, 0, 0);
   cyan    = gdImageColorAllocate(im, 0, 192, 255);
   yellow  = gdImageColorAllocate(im, 255, 255, 0);

   if(config.graph_true_color) {
      if(config.graph_background_alpha)
         gdImageAlphaBlending(im, false);

      gdImageFilledRectangle(im, 0, 0, xsize-1, ysize-1, c_background);

      if(config.graph_background_alpha) 
         gdImageSaveAlpha(im, true);

      gdImageAlphaBlending(im, true);
   }

   /* make borders */

   for(u_int i = 0; i < config.graph_border_width; i++) {          /* do shadow effect */
      gdImageLine(im, i, i, xsize-i, i, white);
      gdImageLine(im, i, i, i, ysize-i, white);
      gdImageLine(im, i, ysize-i, xsize-i, ysize-i, dkgrey);
      gdImageLine(im, xsize-i, i, xsize-i, ysize-i, dkgrey);
   }

   gdImageRectangle(im, 20, 25, xsize-21, ysize-21, black);
   gdImageRectangle(im, 19, 24, xsize-22, ysize-22, white);

   /* display the graph title */
   _gdImageString(im, GD_FONT_MEDIUM_BOLD, 20, 8, title, c_title, true, NULL);

   return;
}

uint32_t graph_t::make_color(const char *str)
{
   return (str && *str) ? strtoul(str, NULL, 16) : 0;
}

//
// Draw a section line right of the left section border and to the  
// middle of the right section border
//
void graph_t::draw_section_line(gdImagePtr im, u_int gw, u_int y)
{
   gdImageLine(im, ML+SBW, y, gw-MR-SBW/2-1, y, white);
   gdImageLine(im, ML+SBW, y+1, gw-MR-SBW/2-1, y+1, black); 
}

void graph_t::draw_grid_line(gdImagePtr im, u_int gw, u_int y)
{
   gdImageLine(im, ML+SBW, y, gw-MR-SBW-1, y, c_gridline);
}

void graph_t::draw_graph_bar(gdImagePtr im, u_int x1, u_int y1, u_int x2, u_int y2, int color)
{
   gdImageFilledRectangle(im, x1+1, y1+1, x2-1, y2-1, color);
   gdImageRectangle(im, x1, y1, x2, y2, c_outline);
}

void graph_t::init_graph_engine(void)
{
   int brect[8];
   gdFontPtr fontptr;

   // determine the size of X for each font 
   if(!config.font_file_normal.isempty() && !gdImageStringFT(NULL, brect, 0, (char*) config.font_file_normal.c_str(), config.font_size_small, 0., 0, 0, uppercase_x))
      font_size_small_px = brect[1] - brect[7];
   else
      font_size_small_px = ((fontptr = gdFontGetSmall()) != NULL) ? fontptr->h-2 : 11;

   if(!config.font_file_normal.isempty() && !gdImageStringFT(NULL, brect, 0, (char*) config.font_file_normal.c_str(), config.font_size_medium, 0., 0, 0, uppercase_x))
      font_size_medium_px = brect[1] - brect[7];
   else
      font_size_medium_px = ((fontptr = gdFontGetMediumBold()) != NULL) ? fontptr->h-2 : 11;

   if(!config.font_file_bold.isempty() && !gdImageStringFT(NULL, brect, 0, (char*) config.font_file_bold.c_str(), config.font_size_medium, 0., 0, 0, uppercase_x))
      font_size_medium_bold_px = brect[1] - brect[7];
   else
      font_size_medium_bold_px = ((fontptr = gdFontGetMediumBold()) != NULL) ? fontptr->h-2 : 11;

   // assign configured colors
   if(!config.graph_background.isempty()) graph_background = make_color(config.graph_background);
   if(!config.graph_title_color.isempty()) graph_title_color = make_color(config.graph_title_color);
   if(!config.graph_gridline.isempty()) graph_gridline = make_color(config.graph_gridline);
   if(!config.graph_shadow.isempty()) graph_shadow = make_color(config.graph_shadow);
   if(!config.graph_hits_color.isempty()) graph_hits_color = make_color(config.graph_hits_color);
   if(!config.graph_files_color.isempty()) graph_files_color = make_color(config.graph_files_color);
   if(!config.graph_hosts_color.isempty()) graph_hosts_color = make_color(config.graph_hosts_color);
   if(!config.graph_pages_color.isempty()) graph_pages_color = make_color(config.graph_pages_color);
   if(!config.graph_visits_color.isempty()) graph_visits_color = make_color(config.graph_visits_color);
   if(!config.graph_xfer_color.isempty()) graph_xfer_color = make_color(config.graph_xfer_color);
   if(!config.graph_outline_color.isempty()) graph_outline_color = make_color(config.graph_outline_color);
   if(!config.graph_legend_color.isempty()) graph_legend_color = make_color(config.graph_legend_color);
   if(!config.graph_weekend_color.isempty()) graph_weekend_color = make_color(config.graph_weekend_color);

   // convert the percentage transparency to 0..127 and shift it into place
   if(config.graph_true_color)
      graph_background |= (127*config.graph_background_alpha)/100 << 24;
}

void graph_t::cleanup_graph_engine(void)
{
}

//
// Include templates
//
#include "formatter_tmpl.cpp"
