/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2018, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   lang.cpp    
*/

/*
   Some of the English initialization text appearing in this file has 
   been copied from webalizer_lang.english. Original change history:

   Webalizer V2.0x Language Support file for English.
   15-May-1998 by Bradford L. Barrett (brad@mrunix.net)
   31-May-1998 Modified for level 1.1 support (brad@mrunix.net)
   23-Jul-1998 Modified for level 1.2 support (brad@mrunix.net)
   08-Mar-1999 Updated HTTP 1.1 response codes by Yves Lafon (ylafon@w3.org)
   28-Jun-1999 Modified for level 1.3 support (brad@mrunix.net)
   16-Feb-2000 Modified for level 2.0 support (brad@mrunix.net)
*/
#include "pch.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "hashtab.h"
#include "lang.h"

//
//
//
#define LANG_VAR_CHAR   0               /* char **                           */
#define LANG_VAR_CHARR   1               /* char *[]                           */
#define LANG_VAR_RCARR   2               /* response_code[]                  */
#define LANG_VAR_CCARR   3               /* country_code[]                     */

//
// Language array literals are defined here to make lang_t constructor more readable 
// because language arrays have to be initialized using member initialization syntax. 
//

/* Help display... */
#define H_MSG_INIT \
         "-h -?     = print this help message"             , \
         "-v -V     = print version information"           , \
         "-w -W     = print warranty information"          , \
         "-d        = print additional debug info"         , \
         "-F type   = Log type.  type= (clf | squid | iis | apache | w3c)", \
         "-i        = ignore history file"                 , \
         "-p        = preserve state (incremental)"        , \
         "-q        = suppress informational messages"     , \
         "-Q        = suppress _ALL_ messages"             , \
         "-Y        = suppress country graph"              , \
         "-G        = suppress hourly graph"               , \
         "-H        = suppress hourly stats"               , \
         "-L        = suppress color coded graph legends"  , \
         "-l num    = use num background lines on graph"   , \
         "-m num    = Visit timout value (seconds)"        , \
         "-T        = print timing information"            , \
         "-c file   = use configuration file 'file'"       , \
         "-n name   = hostname to use"                     , \
         "-o dir    = output directory to use"             , \
         "-t name   = report title 'name'"                 , \
         "-a name   = hide user agent 'name'"              , \
         "-r name   = hide referrer 'name'"                , \
         "-s name   = hide site 'name'"                    , \
         "-u name   = hide URL 'name'"                     , \
         "-x name   = Use filename extension 'name'"       , \
         "-P name   = Page type extension 'name'"          , \
         "-I name   = Index alias 'name'"                  , \
         "-A num    = Display num top agents"              , \
         "-C num    = Display num top countries"           , \
         "-R num    = Display num top referrers"           , \
         "-S num    = Display num top hosts"               , \
         "-U num    = Display num top URLs"                , \
         "-e num    = Display num top Entry Pages"         , \
         "-E num    = Display num top Exit Pages"          , \
         "-g num    = Group Domains to 'num' levels"       , \
         "-X        = Hide individual hosts"               , \
         "-D name   = Use DNS Cache file 'name'"           , \
         "-N num    = Number of DNS processes (0=disable)" , \
         "\n"                                              , \
         "Long options:"                                   , \
         "--help              print this help message"     , \
         "--last-log          last log file for the current month", \
         "--prepare-report    prepare a report using a state database", \
         "--warranty          print warranty information"  , \
         "--version           print version information"   , \
         "--batch             just update the database (no report)"   , \
         "--end-month         end active visits and close the database", \
         "--compact-db        compact the database", \
         "--db-info           print database information", \
         "--pipe-log-names    read log file names from standard input", \
         NULL

#define HELPMSG_ARRAY_SIZE (sizeof(h_msg)/sizeof(h_msg[0])-1)

/* short month names MUST BE 3 CHARS in size... pad if needed*/
#define S_MONTH_INIT \
   "Jan", "Feb", "Mar", \
   "Apr", "May", "Jun", \
   "Jul", "Aug", "Sep", \
   "Oct", "Nov", "Dec"

#define SMONTH_ARRAY_SIZE sizeof(s_month)/sizeof(s_month[0])

/* long month names - can be any length */
#define L_MONTH_INIT \
   "January",  "February", "March",   "April",    \
   "May",      "June",     "July",    "August",   \
   "September","October",  "November","December"

#define LMONTH_ARRAY_SIZE sizeof(l_month)/sizeof(l_month[0])

// unit prefix characters (i.e. kilo, mega, etc)
#define UNIT_PFX_INIT "K", "M", "G", "T", "P", "E", "Z"

#define UNIT_PREFIX_ARRAY_SIZE sizeof(msg_unit_pfx)/sizeof(msg_unit_pfx[0])

/* response code descriptions... order IS important!      */
#define HTTP_RESP_INIT \
         {  0, "Undefined response code"                    }, \
         {100, "Code 100 - Continue"                        }, \
         {101, "Code 101 - Switching Protocols"             }, \
         {200, "Code 200 - OK"                              }, \
         {201, "Code 201 - Created"                         }, \
         {202, "Code 202 - Accepted"                        }, \
         {203, "Code 203 - Non-Authoritative Information"   }, \
         {204, "Code 204 - No Content"                      }, \
         {205, "Code 205 - Reset Content"                   }, \
         {206, "Code 206 - Partial Content"                 }, \
         {300, "Code 300 - Multiple Choices"                }, \
         {301, "Code 301 - Moved Permanently"               }, \
         {302, "Code 302 - Found"                           }, \
         {303, "Code 303 - See Other"                       }, \
         {304, "Code 304 - Not Modified"                    }, \
         {305, "Code 305 - Use Proxy"                       }, \
         {307, "Code 307 - Moved Temporarily"               }, \
         {400, "Code 400 - Bad Request"                     }, \
         {401, "Code 401 - Unauthorized"                    }, \
         {402, "Code 402 - Payment Required"                }, \
         {403, "Code 403 - Forbidden"                       }, \
         {404, "Code 404 - Not Found"                       }, \
         {405, "Code 405 - Method Not Allowed"              }, \
         {406, "Code 406 - Not Acceptable"                  }, \
         {407, "Code 407 - Proxy Authentication Required"   }, \
         {408, "Code 408 - Request Timeout"                 }, \
         {409, "Code 409 - Conflict"                        }, \
         {410, "Code 410 - Gone"                            }, \
         {411, "Code 411 - Length Required"                 }, \
         {412, "Code 412 - Precondition Failed"             }, \
         {413, "Code 413 - Request Entity Too Large"        }, \
         {414, "Code 414 - Request-URI Too Long"            }, \
         {415, "Code 415 - Unsupported Media Type"          }, \
         {416, "Code 416 - Requested Range Not Satisfiable" }, \
         {417, "Code 417 - Expectation Failed"              }, \
         {500, "Code 500 - Internal Server Error"           }, \
         {501, "Code 501 - Not Implemented"                 }, \
         {502, "Code 502 - Bad Gateway"                     }, \
         {503, "Code 503 - Service Unavailable"             }, \
         {504, "Code 504 - Gateway Timeout"                 }, \
         {505, "Code 505 - HTTP Version Not Supported"      } 

#define RESPCODE_ARRAY_SIZE sizeof(response)/sizeof(response[0])

/* Country codes */

#define COUNTRY_INIT \
         {"*",         "Unresolved/Unknown"                }, \
         {"ac",        "Ascension Island"                  }, \
         {"ad",        "Andorra"                           }, \
         {"ae",        "United Arab Emirates"              }, \
         {"af",        "Afghanistan"                       }, \
         {"ag",        "Antigua and Barbuda"               }, \
         {"ai",        "Anguilla"                          }, \
         {"al",        "Albania"                           }, \
         {"am",        "Armenia"                           }, \
         {"ao",        "Angola"                            }, \
         {"aq",        "Antarctica"                        }, \
         {"ar",        "Argentina"                         }, \
         {"as",        "American Samoa"                    }, \
         {"at",        "Austria"                           }, \
         {"au",        "Australia"                         }, \
         {"aw",        "Aruba"                             }, \
         {"az",        "Azerbaijan"                        }, \
         {"ax",        "Aland Islands"                     }, \
         {"bl",        "Saint Barthelemy"                  }, \
         {"ba",        "Bosnia and Herzegovina"            }, \
         {"bb",        "Barbados"                          }, \
         {"bd",        "Bangladesh"                        }, \
         {"be",        "Belgium"                           }, \
         {"bf",        "Burkina Faso"                      }, \
         {"bg",        "Bulgaria"                          }, \
         {"bh",        "Bahrain"                           }, \
         {"bi",        "Burundi"                           }, \
         {"bj",        "Benin"                             }, \
         {"bm",        "Bermuda"                           }, \
         {"bn",        "Brunei Darussalam"                 }, \
         {"bo",        "Bolivia"                           }, \
         {"br",        "Brazil"                            }, \
         {"bs",        "Bahamas"                           }, \
         {"bt",        "Bhutan"                            }, \
         {"bv",        "Bouvet Island"                     }, \
         {"bw",        "Botswana"                          }, \
         {"by",        "Belarus"                           }, \
         {"bz",        "Belize"                            }, \
         {"ca",        "Canada"                            }, \
         {"cc",        "Cocos (Keeling) Islands"           }, \
         {"cd",        "Congo"                             }, \
         {"cf",        "Central African Republic"          }, \
         {"cg",        "Congo"                             }, \
         {"ch",        "Switzerland"                       }, \
         {"ci",        "Cote D'Ivoire (Ivory Coast)"       }, \
         {"ck",        "Cook Islands"                      }, \
         {"cl",        "Chile"                             }, \
         {"cm",        "Cameroon"                          }, \
         {"cn",        "China"                             }, \
         {"co",        "Colombia"                          }, \
         {"cr",        "Costa Rica"                        }, \
         {"cu",        "Cuba"                              }, \
         {"cv",        "Cape Verde"                        }, \
         {"cw",      u8"Cura\u00E7ao"                      }, \
         {"cx",        "Christmas Island"                  }, \
         {"cy",        "Cyprus"                            }, \
         {"cz",        "Czech Republic"                    }, \
         {"de",        "Germany"                           }, \
         {"dj",        "Djibouti"                          }, \
         {"dk",        "Denmark"                           }, \
         {"dm",        "Dominica"                          }, \
         {"do",        "Dominican Republic"                }, \
         {"dz",        "Algeria"                           }, \
         {"ec",        "Ecuador"                           }, \
         {"ee",        "Estonia"                           }, \
         {"eg",        "Egypt"                             }, \
         {"eh",        "Western Sahara"                    }, \
         {"er",        "Eritrea"                           }, \
         {"es",        "Spain"                             }, \
         {"et",        "Ethiopia"                          }, \
         {"eu",        "European Union"                    }, \
         {"fi",        "Finland"                           }, \
         {"fj",        "Fiji"                              }, \
         {"fk",        "Falkland Islands (Malvinas)"       }, \
         {"fm",        "Micronesia"                        }, \
         {"fo",        "Faroe Islands"                     }, \
         {"fr",        "France"                            }, \
         {"ga",        "Gabon"                             }, \
         {"gb",        "United Kingdom"                    }, \
         {"gd",        "Grenada"                           }, \
         {"ge",        "Georgia"                           }, \
         {"gf",        "French Guiana"                     }, \
         {"gg",        "Guernsey"                          }, \
         {"gh",        "Ghana"                             }, \
         {"gi",        "Gibraltar"                         }, \
         {"gl",        "Greenland"                         }, \
         {"gm",        "Gambia"                            }, \
         {"gn",        "Guinea"                            }, \
         {"gp",        "Guadeloupe"                        }, \
         {"gq",        "Equatorial Guinea"                 }, \
         {"gr",        "Greece"                            }, \
         {"gs",        "S. Georgia and S. Sandwich Islands"}, \
         {"gt",        "Guatemala"                         }, \
         {"gu",        "Guam"                              }, \
         {"gw",        "Guinea-Bissau"                     }, \
         {"gy",        "Guyana"                            }, \
         {"hk",        "Hong Kong"                         }, \
         {"hm",        "Heard and McDonald Islands"        }, \
         {"hn",        "Honduras"                          }, \
         {"hr",        "Croatia"                           }, \
         {"ht",        "Haiti"                             }, \
         {"hu",        "Hungary"                           }, \
         {"id",        "Indonesia"                         }, \
         {"ie",        "Ireland"                           }, \
         {"il",        "Israel"                            }, \
         {"im",        "Isle of Man"                       }, \
         {"in",        "India"                             }, \
         {"io",        "British Indian Ocean Territory"    }, \
         {"iq",        "Iraq"                              }, \
         {"ir",        "Iran"                              }, \
         {"is",        "Iceland"                           }, \
         {"it",        "Italy"                             }, \
         {"je",        "Jersey"                            }, \
         {"jm",        "Jamaica"                           }, \
         {"jo",        "Jordan"                            }, \
         {"jp",        "Japan"                             }, \
         {"ke",        "Kenya"                             }, \
         {"kg",        "Kyrgyzstan"                        }, \
         {"kh",        "Cambodia"                          }, \
         {"ki",        "Kiribati"                          }, \
         {"km",        "Comoros"                           }, \
         {"kn",        "Saint Kitts and Nevis"             }, \
         {"kp",        "Korea (North)"                     }, \
         {"kr",        "Korea (South)"                     }, \
         {"kw",        "Kuwait"                            }, \
         {"ky",        "Cayman Islands"                    }, \
         {"kz",        "Kazakhstan"                        }, \
         {"la",        "Lao"                               }, \
         {"lb",        "Lebanon"                           }, \
         {"lc",        "Saint Lucia"                       }, \
         {"li",        "Liechtenstein"                     }, \
         {"lk",        "Sri Lanka"                         }, \
         {"lr",        "Liberia"                           }, \
         {"ls",        "Lesotho"                           }, \
         {"lt",        "Lithuania"                         }, \
         {"lu",        "Luxembourg"                        }, \
         {"lv",        "Latvia"                            }, \
         {"ly",        "Libya"                             }, \
         {"ma",        "Morocco"                           }, \
         {"mc",        "Monaco"                            }, \
         {"md",        "Moldova"                           }, \
         {"me",        "Montenegro"                        }, \
         {"mf",        "Saint Martin"                      }, \
         {"mg",        "Madagascar"                        }, \
         {"mh",        "Marshall Islands"                  }, \
         {"mk",        "Macedonia"                         }, \
         {"ml",        "Mali"                              }, \
         {"mm",        "Myanmar"                           }, \
         {"mn",        "Mongolia"                          }, \
         {"mo",        "Macao"                             }, \
         {"mp",        "Northern Mariana Islands"          }, \
         {"mq",        "Martinique"                        }, \
         {"mr",        "Mauritania"                        }, \
         {"ms",        "Montserrat"                        }, \
         {"mt",        "Malta"                             }, \
         {"mu",        "Mauritius"                         }, \
         {"mv",        "Maldives"                          }, \
         {"mw",        "Malawi"                            }, \
         {"mx",        "Mexico"                            }, \
         {"my",        "Malaysia"                          }, \
         {"mz",        "Mozambique"                        }, \
         {"na",        "Namibia"                           }, \
         {"nc",        "New Caledonia"                     }, \
         {"ne",        "Niger"                             }, \
         {"nf",        "Norfolk Island"                    }, \
         {"ng",        "Nigeria"                           }, \
         {"ni",        "Nicaragua"                         }, \
         {"nl",        "Netherlands"                       }, \
         {"no",        "Norway"                            }, \
         {"np",        "Nepal"                             }, \
         {"nr",        "Nauru"                             }, \
         {"nu",        "Niue"                              }, \
         {"nz",        "New Zealand"                       }, \
         {"om",        "Oman"                              }, \
         {"pa",        "Panama"                            }, \
         {"pe",        "Peru"                              }, \
         {"pf",        "French Polynesia"                  }, \
         {"pg",        "Papua New Guinea"                  }, \
         {"ph",        "Philippines"                       }, \
         {"pk",        "Pakistan"                          }, \
         {"pl",        "Poland"                            }, \
         {"pm",        "St. Pierre and Miquelon"           }, \
         {"pn",        "Pitcairn"                          }, \
         {"pr",        "Puerto Rico"                       }, \
         {"ps",        "Palestinian Territories"           }, \
         {"pt",        "Portugal"                          }, \
         {"pw",        "Palau"                             }, \
         {"py",        "Paraguay"                          }, \
         {"qa",        "Qatar"                             }, \
         {"re",        "Reunion"                           }, \
         {"ro",        "Romania"                           }, \
         {"rs",        "Serbia"                            }, \
         {"ru",        "Russian Federation"                }, \
         {"rw",        "Rwanda"                            }, \
         {"sa",        "Saudi Arabia"                      }, \
         {"sb",        "Solomon Islands"                   }, \
         {"sc",        "Seychelles"                        }, \
         {"sd",        "Sudan"                             }, \
         {"se",        "Sweden"                            }, \
         {"sg",        "Singapore"                         }, \
         {"sh",        "St. Helena"                        }, \
         {"si",        "Slovenia"                          }, \
         {"sj",        "Svalbard and Jan Mayen"            }, \
         {"sk",        "Slovakia"                          }, \
         {"sl",        "Sierra Leone"                      }, \
         {"sm",        "San Marino"                        }, \
         {"sn",        "Senegal"                           }, \
         {"so",        "Somalia"                           }, \
         {"sr",        "Suriname"                          }, \
         {"st",        "Sao Tome and Principe"             }, \
         {"sv",        "El Salvador"                       }, \
         {"sy",        "Syria"                             }, \
         {"sz",        "Swaziland"                         }, \
         {"tc",        "Turks and Caicos Islands"          }, \
         {"td",        "Chad"                              }, \
         {"tf",        "French Southern Territories"       }, \
         {"tg",        "Togo"                              }, \
         {"th",        "Thailand"                          }, \
         {"tj",        "Tajikistan"                        }, \
         {"tk",        "Tokelau"                           }, \
         {"tl",        "Timor-Leste"                       }, \
         {"tm",        "Turkmenistan"                      }, \
         {"tn",        "Tunisia"                           }, \
         {"to",        "Tonga"                             }, \
         {"tr",        "Turkey"                            }, \
         {"tt",        "Trinidad and Tobago"               }, \
         {"tv",        "Tuvalu"                            }, \
         {"tw",        "Taiwan"                            }, \
         {"tz",        "Tanzania"                          }, \
         {"ua",        "Ukraine"                           }, \
         {"ug",        "Uganda"                            }, \
         {"uk",        "United Kingdom"                    }, \
         {"um",        "US Minor Outlying Islands"         }, \
         {"us",        "United States"                     }, \
         {"uy",        "Uruguay"                           }, \
         {"uz",        "Uzbekistan"                        }, \
         {"va",        "Vatican City State (Holy See)"     }, \
         {"vc",        "Saint Vincent and the Grenadines"  }, \
         {"ve",        "Venezuela"                         }, \
         {"vg",        "Virgin Islands (British)"          }, \
         {"vi",        "Virgin Islands (U.S.)"             }, \
         {"vn",        "Viet Nam"                          }, \
         {"vu",        "Vanuatu"                           }, \
         {"wf",        "Wallis and Futuna"                 }, \
         {"ws",        "Samoa"                             }, \
         {"ye",        "Yemen"                             }, \
         {"yt",        "Mayotte"                           }, \
         {"za",        "South Africa"                      }, \
         {"zm",        "Zambia"                            }, \
         {"zw",        "Zimbabwe"                          }, \
         {NULL,        NULL                                }

#define CCODE_ARRAY_SIZE (sizeof(ctry)/sizeof(ctry[0])-1)

//
//
//
inline bool iseolchar(char ch) {return (ch == '\n' || ch == '\r') ? true : false;}

//
//
//

lang_t::lang_node_t::lang_node_t(const char *varname) : varname(varname) 
{
   vartype = LANG_VAR_CHAR; 
   varptr = NULL; 
   maxcount = 0; 
   elemsize = 0; 
}

lang_t::lang_node_t::~lang_node_t(void)
{
   memset(varptr, 0, maxcount * elemsize);
}

void lang_t::lang_hash_table::put_lang_var(const char *varname, int vartype, void *varptr, int maxcount, size_t elemsize)
{
   lang_node_t *lnode;

   if(varname == NULL || *varname == 0)
      return;

   lnode = new lang_node_t(varname);

   lnode->vartype = vartype;
   lnode->varptr = (u_char*) varptr;
   lnode->maxcount = maxcount;
   lnode->elemsize = elemsize;

   put_node(lnode);
}

const lang_t::lang_node_t *lang_t::lang_hash_table::find_lang_var(const string_t& varname) const
{
   if(varname == NULL || *varname == 0)
      return NULL;

   return find_node(varname, OBJ_REG);
}

// -----------------------------------------------------------------------
//
// lang_t
//
// -----------------------------------------------------------------------

lang_t::lang_t(void) :
   h_msg {H_MSG_INIT},
   s_month {S_MONTH_INIT},
   l_month {L_MONTH_INIT},
   msg_unit_pfx {UNIT_PFX_INIT},
   response {HTTP_RESP_INIT},
   ctry {COUNTRY_INIT}
{
   lang_buffer = NULL;

   /***********************************************************************/
   /* DEFINE LANGUAGE NAME here                                           */
   /***********************************************************************/

   language  = "English";
   language_code = "en";

   /***********************************************************************/
   /*                                                                     */
   /* Informational messages                                              */
   /*                                                                     */
   /* These messages are only displayed while The Webalizer is being run, */
   /* usually to the screen, and are not part of the HTML output.         */
   /*                                                                     */
   /***********************************************************************/

   /* these are only used in timing totals */
   /* Format:   Processed XXX records (XXX ignored, XXX bad) in X.XX seconds */
   msg_processed="Processed";
   msg_records = "records";
   msg_addresses="addresses";
   msg_ignored = "ignored";
   msg_bad     = "bad";
   msg_in      = "in";
   msg_seconds = "seconds";

   /* progress and setup error messages */
   msg_log_err = "Error: Can't open log file";
   msg_log_use = "Using logfile";
   msg_log_done = "Finished log file";
   msg_dir_err = "Error: Can't change directory to";
   msg_dir_use = "Creating output in";
   msg_cur_dir = "current directory";
   msg_hostname= "Host name for reports is";
   msg_ign_hist= "Ignoring previous history...";
   msg_no_hist = "History file not found...";
   msg_get_hist= "Reading history file...";
   msg_put_hist= "Saving history information...";
   msg_hist_err= "Error: Unable to write history file";
   msg_bad_hist= "Error: Ignoring invalid history record";
   msg_bad_conf= "Error: Unable to open configuration file";
   msg_bad_key = "Warning: Invalid keyword";
   msg_bad_date= "Error: Skipping record (bad date)";
   msg_ign_nscp= "Skipping Netscape header record";
   msg_bad_rec = "Skipping bad record";
   msg_no_vrec = "No valid records found!";
   msg_gen_rpt = "Generating report for";
   msg_gen_sum = "Generating summary report";
   msg_get_data= "Reading previous run data..";
   msg_put_data= "Saving current run data...";
   msg_no_data = "Previous run data not found...";
   msg_bad_data= "Error: Unable to restore run data";
   msg_data_err= "Error: Unable to save current run data";
   msg_dup_data= "Warning: Possible duplicate data found";
   msg_afm_err = "Error: Invalid or missing ApacheLogFormat";
   msg_pars_err= "Cannot initialize log file parser";
   msg_use_conf= "Processed configuration file";
   msg_use_lang= "Processed language file";
   msg_use_db  = "Using database";
   msg_runtime = "Total run time is";
   msg_dnstime = "DNS wait time is";
   msg_mnttime = "Maintenance time is";
   msg_rpttime = "Generated reports in";
   msg_cmpctdb = "Finished compacting the database";
   msg_nofile  = "File not found";
   msg_file_err= "Cannot read file";
   msg_use_help= "Using XML help file";
   msg_fpos_err= "Cannot retrieve current file position for";

   /* log record errors */
   msg_big_rec = "Error: Skipping oversized log record";
   msg_big_host= "Warning: Truncating oversized hostname";
   msg_big_date= "Warning: Truncating oversized date field";
   msg_big_req = "Warning: Truncating oversized request field";
   msg_big_ref = "Warning: Truncating oversized referrer field";
   msg_big_user= "Warning: Truncating oversized username";
   msg_big_url = "Warning: Truncating oversized URL field";
   msg_big_agnt= "Warning: Truncating oversized user agent field";
   msg_big_srch= "Warning: Truncating oversized search arguments field";
   msg_big_one = "Warning: String exceeds storage size";

   /* misc errors */
   msg_no_open = "Error: Unable to open file";

   /***********************************************************************/
   /*                                                                     */
   /* HTML strings                                                        */
   /*                                                                     */
   /* These strings are used as part of the HTML output generated by The  */
   /* Webalizer.                                                          */ 
   /*                                                                     */
   /***********************************************************************/

   /* header strings */
   msg_hhdr_sp = "Summary Period";
   msg_hhdr_gt = "Generated";

   /* main index strings */
   msg_main_us = "Usage summary for";
   msg_main_per= "Last 12 Months";
   msg_main_sum= "Summary by Month";
   msg_main_da = "Daily Avg";
   msg_main_mt = "Monthly Totals";
   msg_main_plst = "Last";
   msg_main_pmns = "Months";

   /* month HTML page strings */
   msg_hmth_du = "Daily usage for";
   msg_hmth_hu = "Hourly usage for";

   /* table header strings */
   msg_h_by    = "By";
   msg_h_avg   = "Avg";
   msg_h_max   = "Max";
   msg_h_total = "Total";
   msg_h_totals= "Totals";
   msg_h_day   = "Day";
   msg_h_mth   = "Month";
   msg_h_hour  = "Hour";
   msg_h_hits  = "Hits";
   msg_h_pages = "Pages";
   msg_h_visits= "Visits";
   msg_h_cvisits = "Converted Visits";
   msg_h_files = "Files";
   msg_h_hosts = "Hosts";
   msg_h_chosts= "Converted Hosts";
   msg_h_xfer  = "Transfer";
   msg_h_avgtime = "AvgTime";
   msg_h_maxtime = "MaxTime";
   msg_h_hname = "Host Name";
   msg_h_host = "Host";
   msg_h_ipaddr= "IP Address";
   msg_h_url   = "URL";
   msg_h_urls  = "URLs";
   msg_h_agent = "User Agent";
   msg_h_agents= "User Agents";
   msg_h_ref   = "Referrer";
   msg_h_refs  = "Referrers";
   msg_h_ctry  = "Country";
   msg_h_ccode  = "Country Code";
   msg_h_search= "Search String";
   msg_h_uname = "User Name";
   msg_h_type  = "Type";
   msg_h_status= "Status";
   msg_h_duration = "Duration";
   msg_h_method= "Method";
   msg_h_download = "Download";
   msg_h_count = "Count";
   msg_h_time = "Time";
   msg_h_spammer = "Spammer";
   msg_h_latitude = "Latitude";
   msg_h_longitude = "Longitude";

   /* links along top of page */
   msg_hlnk_sum= "Summary";
   msg_hlnk_ds = "Daily Statistics";
   msg_hlnk_hs = "Hourly Statistics";
   msg_hlnk_u  = "URLs";
   msg_hlnk_s  = "Hosts";
   msg_hlnk_a  = "Agents";
   msg_hlnk_c  = "Countries";
   msg_hlnk_r  = "Referrers";
   msg_hlnk_en = "Entry";
   msg_hlnk_ex = "Exit";
   msg_hlnk_sr = "Search";
   msg_hlnk_i  = "Users";
   msg_hlnk_err= "Errors";
   msg_hlnk_dl = "Downloads";

   /* monthly total table */
   msg_mtot_ms = "Monthly Statistics for";
   msg_mtot_th = "Total Hits";
   msg_mtot_tf = "Total Files";
   msg_mtot_tp = "Total Pages";
   msg_mtot_tv = "Total Visits";
   msg_mtot_tx = "Total Transfer";
   msg_mtot_us = "Total Unique Hosts";
   msg_mtot_ur = "Total Unique Referrers";
   msg_mtot_ua = "Total Unique User Agents";
   msg_mtot_uu = "Total Unique URLs";
   msg_mtot_ui = "Total Unique User Names";
   msg_mtot_mhd= "Hits per Day";
   msg_mtot_mhh= "Hits per Hour";
   msg_mtot_mfd= "Files per Day";
   msg_mtot_mpd= "Pages per Day";
   msg_mtot_mvd= "Visits per Day";
   msg_mtot_mkd= "Transfer per Day";
   msg_mtot_rc = "Hits by Response Code";
   msg_mtot_sph = "Seconds per Hit";
   msg_mtot_spf = "Seconds per File";
   msg_mtot_spp = "Seconds per Page";
   msg_mtot_mhv = "Hits per Visit";
   msg_mtot_mfv = "Files per Visit";
   msg_mtot_mpv = "Pages per Visit";
   msg_mtot_mdv = "Visit Duration";
   msg_mtot_mkv = "Transfer per Visit";
   msg_mtot_dl = "Total Downloads";
   msg_mtot_tcv = "Total Converted Visits";
   msg_mtot_tch = "Total Converted Hosts";
   msg_mtot_hcr = "Host Converstion Rate";
   msg_mtot_cvd = "Visit Duration (Converted)";
   msg_mtot_conv = "Conversion";
   msg_mtot_rtot = "Robot Totals";
   msg_mtot_perf = "Performance";
   msg_mtot_hdt = "Hourly/Daily Totals";
   msg_mtot_htot = "Human Totals";
   msg_mtot_stot = "Spammer Totals";
   msg_mtot_terr = "Total Errors";

   /* daily total table */
   msg_dtot_ds = "Daily Statistics for";

   /* hourly total table */
   msg_htot_hs = "Hourly Statistics for";

   /* country pie chart */
   msg_ctry_use= "Usage by Country for";

   /* top tables */
   /* Formatted as "Top xxx of xxx Total something" */
   msg_top_top = "Top";
   msg_top_of  = "of";
   msg_top_s   = "Total Hosts";
   msg_top_u   = "Total URLs";
   msg_top_r   = "Total Referrers";
   msg_top_a   = "Total User Agents";
   msg_top_c   = "Total Countries";
   msg_top_en  = "Total Entry Pages";
   msg_top_ex  = "Total Exit Pages";
   msg_top_sr  = "Total Search Strings";
   msg_top_i   = "Total User Names";
   msg_v_hosts = "View All Hosts";
   msg_v_urls  = "View All URLs";
   msg_v_refs  = "View All Referrers";
   msg_v_agents= "View All User Agents";
   msg_v_search= "View All Search Strings";
   msg_v_users = "View All User Names";
   msg_misc_pages = "Pages - document requests (e.g. html, asp, txt); Files - successful requests of all types (e.g. html, gif, css); Hits - all requests, including errors;";
   msg_misc_visitors = "Visitors are identified by IP addresses. Two or more visitors sharing the same IP address (e.g. firewall address) will be counted as a single visitor.";
   msg_misc_robots = "Robot activity is excluded from the Country, Entry and Exit URL reports";
   msg_h_errors = "Errors";
   msg_v_errors= "View All Errors";
   msg_h_downloads = "Downloads";
   msg_v_downloads = "View All Downloads";
   msg_ref_dreq = "- (Direct Request)";
   msg_max_items = "Maximum number of items is displayed";

   msg_xfer_unit = "B";

   msg_title   = "Usage Statistics for";
   msg_h_other = "Other";

   msg_h_city = "City";

   msg_ctrl_c  = "Ctrl-C detected. Aborting...";

   /* DNS Stuff */
   msg_dns_nocf= "No cache file specified, aborting...";
   msg_dns_nodb= "Error: Unable to open DNS cache file";
   msg_dns_nolk= "Error: Unable to lock DNS cache file";
   msg_dns_usec= "Using DNS cache file";
   msg_dns_rslv= "DNS workers";
   msg_dns_init= "Error: Cannot initialize DNS resolver";
   msg_dns_htrt= "DNS cache hit ratio";
   msg_dns_geoe= "Cannot open GeoIP database";
   msg_dns_useg= "Using GeoIP database";

   h_usage1 = "Usage";
   h_usage2 = "[options] [log file [[ log file] ...] | report database]";
}

lang_t::~lang_t(void)
{
   if(lang_buffer)
      delete [] lang_buffer;
}

bool lang_t::read_lang_file(const char *fname, std::vector<string_t>& errors)
{
   long filelen;
   size_t lang_len;
   FILE *lang_file = nullptr;

   if(fname == NULL || *fname == 0) {
      errors.emplace_back("The name of the language file name is not valid.");
      return false;
   }

   if((lang_file = fopen(fname, "r")) == NULL)
      goto errexit;

   if(fseek(lang_file, 0, SEEK_END) == -1 || (filelen = ftell(lang_file)) == -1L)
      goto errexit;

   lang_buffer = new u_char[filelen+1];

   if(fseek(lang_file, 0, SEEK_SET) == -1 || (lang_len = fread(lang_buffer, 1, filelen, lang_file)) == 0)
      goto errexit;

   fclose(lang_file);

   lang_buffer[lang_len] = 0;

   return true;

errexit:
   errors.push_back(string_t::_format("Cannot open the language file (%s). ", fname));

   if(lang_file)
      fclose(lang_file);

   return false;
}

void lang_t::init_lang_htab(void)
{
   put_lang_var("language", LANG_VAR_CHAR, &language, 1, sizeof(char*));
   put_lang_var("language_code", LANG_VAR_CHAR, &language_code, 1, sizeof(char*));

   put_lang_var("msg_processed", LANG_VAR_CHAR, &msg_processed, 1, sizeof(char*));
   put_lang_var("msg_records", LANG_VAR_CHAR, &msg_records, 1, sizeof(char*));
   put_lang_var("msg_addresses", LANG_VAR_CHAR, &msg_addresses, 1, sizeof(char*));
   put_lang_var("msg_ignored", LANG_VAR_CHAR, &msg_ignored, 1, sizeof(char*));
   put_lang_var("msg_bad", LANG_VAR_CHAR, &msg_bad, 1, sizeof(char*));
   put_lang_var("msg_in", LANG_VAR_CHAR, &msg_in, 1, sizeof(char*));
   put_lang_var("msg_seconds", LANG_VAR_CHAR, &msg_seconds, 1, sizeof(char*));
   put_lang_var("msg_runtime", LANG_VAR_CHAR, &msg_runtime, 1, sizeof(char*));
   put_lang_var("msg_dnstime", LANG_VAR_CHAR, &msg_dnstime, 1, sizeof(char*));
   put_lang_var("msg_mnttime", LANG_VAR_CHAR, &msg_mnttime, 1, sizeof(char*));
   put_lang_var("msg_rpttime", LANG_VAR_CHAR, &msg_rpttime, 1, sizeof(char*));
   put_lang_var("msg_cmpctdb", LANG_VAR_CHAR, &msg_cmpctdb, 1, sizeof(char*));
   put_lang_var("msg_nofile", LANG_VAR_CHAR, &msg_nofile, 1, sizeof(char*));
   put_lang_var("msg_file_err", LANG_VAR_CHAR, &msg_file_err, 1, sizeof(char*));
   put_lang_var("msg_use_help", LANG_VAR_CHAR, &msg_use_help, 1, sizeof(char*));
   put_lang_var("msg_fpos_err", LANG_VAR_CHAR, &msg_fpos_err, 1, sizeof(char*));

   put_lang_var("msg_log_err", LANG_VAR_CHAR, &msg_log_err, 1, sizeof(char*));
   put_lang_var("msg_log_use", LANG_VAR_CHAR, &msg_log_use, 1, sizeof(char*));
   put_lang_var("msg_log_done", LANG_VAR_CHAR, &msg_log_done, 1, sizeof(char*));
   put_lang_var("msg_dir_err", LANG_VAR_CHAR, &msg_dir_err, 1, sizeof(char*));
   put_lang_var("msg_dir_use", LANG_VAR_CHAR, &msg_dir_use, 1, sizeof(char*));
   put_lang_var("msg_cur_dir", LANG_VAR_CHAR, &msg_cur_dir, 1, sizeof(char*));
   put_lang_var("msg_hostname", LANG_VAR_CHAR, &msg_hostname, 1, sizeof(char*));
   put_lang_var("msg_ign_hist", LANG_VAR_CHAR, &msg_ign_hist, 1, sizeof(char*));
   put_lang_var("msg_no_hist", LANG_VAR_CHAR, &msg_no_hist, 1, sizeof(char*));
   put_lang_var("msg_get_hist", LANG_VAR_CHAR, &msg_get_hist, 1, sizeof(char*));
   put_lang_var("msg_put_hist", LANG_VAR_CHAR, &msg_put_hist, 1, sizeof(char*));
   put_lang_var("msg_hist_err", LANG_VAR_CHAR, &msg_hist_err, 1, sizeof(char*));
   put_lang_var("msg_bad_hist", LANG_VAR_CHAR, &msg_bad_hist, 1, sizeof(char*));
   put_lang_var("msg_bad_conf", LANG_VAR_CHAR, &msg_bad_conf, 1, sizeof(char*));
   put_lang_var("msg_bad_key", LANG_VAR_CHAR, &msg_bad_key, 1, sizeof(char*));
   put_lang_var("msg_bad_date", LANG_VAR_CHAR, &msg_bad_date, 1, sizeof(char*));
   put_lang_var("msg_ign_nscp", LANG_VAR_CHAR, &msg_ign_nscp, 1, sizeof(char*));
   put_lang_var("msg_bad_rec", LANG_VAR_CHAR, &msg_bad_rec, 1, sizeof(char*));
   put_lang_var("msg_no_vrec", LANG_VAR_CHAR, &msg_no_vrec, 1, sizeof(char*));
   put_lang_var("msg_gen_rpt", LANG_VAR_CHAR, &msg_gen_rpt, 1, sizeof(char*));
   put_lang_var("msg_gen_sum", LANG_VAR_CHAR, &msg_gen_sum, 1, sizeof(char*));
   put_lang_var("msg_get_data", LANG_VAR_CHAR, &msg_get_data, 1, sizeof(char*));
   put_lang_var("msg_put_data", LANG_VAR_CHAR, &msg_put_data, 1, sizeof(char*));
   put_lang_var("msg_no_data", LANG_VAR_CHAR, &msg_no_data, 1, sizeof(char*));
   put_lang_var("msg_bad_data", LANG_VAR_CHAR, &msg_bad_data, 1, sizeof(char*));
   put_lang_var("msg_data_err", LANG_VAR_CHAR, &msg_data_err, 1, sizeof(char*));
   put_lang_var("msg_dup_data", LANG_VAR_CHAR, &msg_dup_data, 1, sizeof(char*));
   put_lang_var("msg_afm_err", LANG_VAR_CHAR, &msg_afm_err, 1, sizeof(char*));
   put_lang_var("msg_ctrl_c", LANG_VAR_CHAR, &msg_ctrl_c, 1, sizeof(char*));
   put_lang_var("msg_pars_err", LANG_VAR_CHAR, &msg_pars_err, 1, sizeof(char*));
   put_lang_var("msg_use_conf", LANG_VAR_CHAR, &msg_use_conf, 1, sizeof(char*));
   put_lang_var("msg_use_lang", LANG_VAR_CHAR, &msg_use_lang, 1, sizeof(char*));
   put_lang_var("msg_use_db", LANG_VAR_CHAR, &msg_use_db, 1, sizeof(char*));

   put_lang_var("msg_dns_nocf", LANG_VAR_CHAR, &msg_dns_nocf, 1, sizeof(char*));
   put_lang_var("msg_dns_nodb", LANG_VAR_CHAR, &msg_dns_nodb, 1, sizeof(char*));
   put_lang_var("msg_dns_nolk", LANG_VAR_CHAR, &msg_dns_nolk, 1, sizeof(char*));
   put_lang_var("msg_dns_usec", LANG_VAR_CHAR, &msg_dns_usec, 1, sizeof(char*));
   put_lang_var("msg_dns_rslv", LANG_VAR_CHAR, &msg_dns_rslv, 1, sizeof(char*));
   put_lang_var("msg_dns_init", LANG_VAR_CHAR, &msg_dns_init, 1, sizeof(char*));
   put_lang_var("msg_dns_htrt", LANG_VAR_CHAR, &msg_dns_htrt, 1, sizeof(char*));
   put_lang_var("msg_dns_geoe", LANG_VAR_CHAR, &msg_dns_geoe, 1, sizeof(char*));
   put_lang_var("msg_dns_useg", LANG_VAR_CHAR, &msg_dns_useg, 1, sizeof(char*));

   put_lang_var("msg_big_rec", LANG_VAR_CHAR, &msg_big_rec, 1, sizeof(char*));
   put_lang_var("msg_big_host", LANG_VAR_CHAR, &msg_big_host, 1, sizeof(char*));
   put_lang_var("msg_big_date", LANG_VAR_CHAR, &msg_big_date, 1, sizeof(char*));
   put_lang_var("msg_big_req", LANG_VAR_CHAR, &msg_big_req, 1, sizeof(char*));
   put_lang_var("msg_big_ref", LANG_VAR_CHAR, &msg_big_ref, 1, sizeof(char*));
   put_lang_var("msg_big_user", LANG_VAR_CHAR, &msg_big_user, 1, sizeof(char*));
   put_lang_var("msg_big_url", LANG_VAR_CHAR, &msg_big_url, 1, sizeof(char*));
   put_lang_var("msg_big_agnt", LANG_VAR_CHAR, &msg_big_agnt, 1, sizeof(char*));
   put_lang_var("msg_big_srch", LANG_VAR_CHAR, &msg_big_srch, 1, sizeof(char*));
   put_lang_var("msg_big_one", LANG_VAR_CHAR, &msg_big_one, 1, sizeof(char*));
   put_lang_var("msg_no_open", LANG_VAR_CHAR, &msg_no_open, 1, sizeof(char*));

   put_lang_var("h_usage1", LANG_VAR_CHAR, &h_usage1, 1, sizeof(char*));
   put_lang_var("h_usage2", LANG_VAR_CHAR, &h_usage2, 1, sizeof(char*));

   put_lang_var("h_msg", LANG_VAR_CHARR, h_msg, HELPMSG_ARRAY_SIZE, sizeof(h_msg[0]));

   put_lang_var("msg_hhdr_sp", LANG_VAR_CHAR, &msg_hhdr_sp, 1, sizeof(char*));
   put_lang_var("msg_hhdr_gt", LANG_VAR_CHAR, &msg_hhdr_gt, 1, sizeof(char*));

   put_lang_var("msg_main_us", LANG_VAR_CHAR, &msg_main_us, 1, sizeof(char*));
   put_lang_var("msg_main_per", LANG_VAR_CHAR, &msg_main_per, 1, sizeof(char*));
   put_lang_var("msg_main_sum", LANG_VAR_CHAR, &msg_main_sum, 1, sizeof(char*));
   put_lang_var("msg_main_da", LANG_VAR_CHAR, &msg_main_da, 1, sizeof(char*));
   put_lang_var("msg_main_mt", LANG_VAR_CHAR, &msg_main_mt, 1, sizeof(char*));

   put_lang_var("msg_main_plst", LANG_VAR_CHAR, &msg_main_plst, 1, sizeof(char*));
   put_lang_var("msg_main_pmns", LANG_VAR_CHAR, &msg_main_pmns, 1, sizeof(char*));

   put_lang_var("msg_hmth_du", LANG_VAR_CHAR, &msg_hmth_du, 1, sizeof(char*));
   put_lang_var("msg_hmth_hu", LANG_VAR_CHAR, &msg_hmth_hu, 1, sizeof(char*));

   put_lang_var("msg_h_by", LANG_VAR_CHAR, &msg_h_by, 1, sizeof(char*));
   put_lang_var("msg_h_avg", LANG_VAR_CHAR, &msg_h_avg, 1, sizeof(char*));
   put_lang_var("msg_h_max", LANG_VAR_CHAR, &msg_h_max, 1, sizeof(char*));
   put_lang_var("msg_h_total", LANG_VAR_CHAR, &msg_h_total, 1, sizeof(char*));
   put_lang_var("msg_h_totals", LANG_VAR_CHAR, &msg_h_totals, 1, sizeof(char*));
   put_lang_var("msg_h_day", LANG_VAR_CHAR, &msg_h_day, 1, sizeof(char*));
   put_lang_var("msg_h_mth", LANG_VAR_CHAR, &msg_h_mth, 1, sizeof(char*));
   put_lang_var("msg_h_hour", LANG_VAR_CHAR, &msg_h_hour, 1, sizeof(char*));
   put_lang_var("msg_h_hits", LANG_VAR_CHAR, &msg_h_hits, 1, sizeof(char*));
   put_lang_var("msg_h_pages", LANG_VAR_CHAR, &msg_h_pages, 1, sizeof(char*));
   put_lang_var("msg_h_visits", LANG_VAR_CHAR, &msg_h_visits, 1, sizeof(char*));
   put_lang_var("msg_h_files", LANG_VAR_CHAR, &msg_h_files, 1, sizeof(char*));
   put_lang_var("msg_h_hosts", LANG_VAR_CHAR, &msg_h_hosts, 1, sizeof(char*));
   put_lang_var("msg_h_xfer", LANG_VAR_CHAR, &msg_h_xfer, 1, sizeof(char*));
   put_lang_var("msg_h_avgtime", LANG_VAR_CHAR, &msg_h_avgtime, 1, sizeof(char*));
   put_lang_var("msg_h_maxtime", LANG_VAR_CHAR, &msg_h_maxtime, 1, sizeof(char*));
   put_lang_var("msg_h_hname", LANG_VAR_CHAR, &msg_h_hname, 1, sizeof(char*));
   put_lang_var("msg_h_host", LANG_VAR_CHAR, &msg_h_host, 1, sizeof(char*));
   put_lang_var("msg_h_ipaddr", LANG_VAR_CHAR, &msg_h_ipaddr, 1, sizeof(char*));
   put_lang_var("msg_h_url", LANG_VAR_CHAR, &msg_h_url, 1, sizeof(char*));
   put_lang_var("msg_h_agent", LANG_VAR_CHAR, &msg_h_agent, 1, sizeof(char*));
   put_lang_var("msg_h_ref", LANG_VAR_CHAR, &msg_h_ref, 1, sizeof(char*));
   put_lang_var("msg_h_ctry", LANG_VAR_CHAR, &msg_h_ctry, 1, sizeof(char*));
   put_lang_var("msg_h_ccode", LANG_VAR_CHAR, &msg_h_ccode, 1, sizeof(char*));
   put_lang_var("msg_h_search", LANG_VAR_CHAR, &msg_h_search, 1, sizeof(char*));
   put_lang_var("msg_h_uname", LANG_VAR_CHAR, &msg_h_uname, 1, sizeof(char*));
   put_lang_var("msg_h_type", LANG_VAR_CHAR, &msg_h_type, 1, sizeof(char*));
   put_lang_var("msg_h_status", LANG_VAR_CHAR, &msg_h_status, 1, sizeof(char*));
   put_lang_var("msg_h_duration", LANG_VAR_CHAR, &msg_h_duration, 1, sizeof(char*));
   put_lang_var("msg_h_method", LANG_VAR_CHAR, &msg_h_method, 1, sizeof(char*));
   put_lang_var("msg_h_download", LANG_VAR_CHAR, &msg_h_download, 1, sizeof(char*));
   put_lang_var("msg_h_count", LANG_VAR_CHAR, &msg_h_count, 1, sizeof(char*));
   put_lang_var("msg_h_time", LANG_VAR_CHAR, &msg_h_time, 1, sizeof(char*));
   put_lang_var("msg_h_spammer", LANG_VAR_CHAR, &msg_h_spammer, 1, sizeof(char*));
   put_lang_var("msg_h_urls", LANG_VAR_CHAR, &msg_h_urls, 1, sizeof(char*));
   put_lang_var("msg_h_refs", LANG_VAR_CHAR, &msg_h_refs, 1, sizeof(char*));
   put_lang_var("msg_h_agents", LANG_VAR_CHAR, &msg_h_agents, 1, sizeof(char*));
   put_lang_var("msg_h_chosts", LANG_VAR_CHAR, &msg_h_chosts, 1, sizeof(char*));
   put_lang_var("msg_h_cvisits", LANG_VAR_CHAR, &msg_h_cvisits, 1, sizeof(char*));
   put_lang_var("msg_h_latitude", LANG_VAR_CHAR, &msg_h_latitude, 1, sizeof(char*));
   put_lang_var("msg_h_longitude", LANG_VAR_CHAR, &msg_h_longitude, 1, sizeof(char*));

   put_lang_var("msg_hlnk_sum", LANG_VAR_CHAR, &msg_hlnk_sum, 1, sizeof(char*));
   put_lang_var("msg_hlnk_ds", LANG_VAR_CHAR, &msg_hlnk_ds, 1, sizeof(char*));
   put_lang_var("msg_hlnk_hs", LANG_VAR_CHAR, &msg_hlnk_hs, 1, sizeof(char*));
   put_lang_var("msg_hlnk_u", LANG_VAR_CHAR, &msg_hlnk_u, 1, sizeof(char*));
   put_lang_var("msg_hlnk_s", LANG_VAR_CHAR, &msg_hlnk_s, 1, sizeof(char*));
   put_lang_var("msg_hlnk_a", LANG_VAR_CHAR, &msg_hlnk_a, 1, sizeof(char*));
   put_lang_var("msg_hlnk_c", LANG_VAR_CHAR, &msg_hlnk_c, 1, sizeof(char*));
   put_lang_var("msg_hlnk_r", LANG_VAR_CHAR, &msg_hlnk_r, 1, sizeof(char*));
   put_lang_var("msg_hlnk_en", LANG_VAR_CHAR, &msg_hlnk_en, 1, sizeof(char*));
   put_lang_var("msg_hlnk_ex", LANG_VAR_CHAR, &msg_hlnk_ex, 1, sizeof(char*));
   put_lang_var("msg_hlnk_sr", LANG_VAR_CHAR, &msg_hlnk_sr, 1, sizeof(char*));
   put_lang_var("msg_hlnk_i", LANG_VAR_CHAR, &msg_hlnk_i, 1, sizeof(char*));
   put_lang_var("msg_hlnk_err", LANG_VAR_CHAR, &msg_hlnk_err, 1, sizeof(char*));
   put_lang_var("msg_hlnk_dl", LANG_VAR_CHAR, &msg_hlnk_dl, 1, sizeof(char*));

   put_lang_var("msg_mtot_ms", LANG_VAR_CHAR, &msg_mtot_ms, 1, sizeof(char*));
   put_lang_var("msg_mtot_th", LANG_VAR_CHAR, &msg_mtot_th, 1, sizeof(char*));
   put_lang_var("msg_mtot_tf", LANG_VAR_CHAR, &msg_mtot_tf, 1, sizeof(char*));
   put_lang_var("msg_mtot_tp", LANG_VAR_CHAR, &msg_mtot_tp, 1, sizeof(char*));
   put_lang_var("msg_mtot_tv", LANG_VAR_CHAR, &msg_mtot_tv, 1, sizeof(char*));
   put_lang_var("msg_mtot_tx", LANG_VAR_CHAR, &msg_mtot_tx, 1, sizeof(char*));
   put_lang_var("msg_mtot_us", LANG_VAR_CHAR, &msg_mtot_us, 1, sizeof(char*));
   put_lang_var("msg_mtot_ur", LANG_VAR_CHAR, &msg_mtot_ur, 1, sizeof(char*));
   put_lang_var("msg_mtot_ua", LANG_VAR_CHAR, &msg_mtot_ua, 1, sizeof(char*));
   put_lang_var("msg_mtot_uu", LANG_VAR_CHAR, &msg_mtot_uu, 1, sizeof(char*));
   put_lang_var("msg_mtot_ui", LANG_VAR_CHAR, &msg_mtot_ui, 1, sizeof(char*));
   put_lang_var("msg_mtot_mhd", LANG_VAR_CHAR, &msg_mtot_mhd, 1, sizeof(char*));
   put_lang_var("msg_mtot_mhh", LANG_VAR_CHAR, &msg_mtot_mhh, 1, sizeof(char*));
   put_lang_var("msg_mtot_mfd", LANG_VAR_CHAR, &msg_mtot_mfd, 1, sizeof(char*));
   put_lang_var("msg_mtot_mpd", LANG_VAR_CHAR, &msg_mtot_mpd, 1, sizeof(char*));
   put_lang_var("msg_mtot_mvd", LANG_VAR_CHAR, &msg_mtot_mvd, 1, sizeof(char*));
   put_lang_var("msg_mtot_mkd", LANG_VAR_CHAR, &msg_mtot_mkd, 1, sizeof(char*));
   put_lang_var("msg_mtot_rc", LANG_VAR_CHAR, &msg_mtot_rc, 1, sizeof(char*));
   put_lang_var("msg_mtot_dl", LANG_VAR_CHAR, &msg_mtot_dl, 1, sizeof(char*));
   put_lang_var("msg_mtot_tcv", LANG_VAR_CHAR, &msg_mtot_tcv, 1, sizeof(char*));
   put_lang_var("msg_mtot_tch", LANG_VAR_CHAR, &msg_mtot_tch, 1, sizeof(char*));
   put_lang_var("msg_mtot_hcr", LANG_VAR_CHAR, &msg_mtot_hcr, 1, sizeof(char*));
   put_lang_var("msg_mtot_cvd", LANG_VAR_CHAR, &msg_mtot_cvd, 1, sizeof(char*));
   put_lang_var("msg_mtot_conv", LANG_VAR_CHAR, &msg_mtot_conv, 1, sizeof(char*));
   put_lang_var("msg_mtot_rtot", LANG_VAR_CHAR, &msg_mtot_rtot, 1, sizeof(char*));
   put_lang_var("msg_mtot_perf", LANG_VAR_CHAR, &msg_mtot_perf, 1, sizeof(char*));
   put_lang_var("msg_mtot_hdt", LANG_VAR_CHAR, &msg_mtot_hdt, 1, sizeof(char*));
   put_lang_var("msg_mtot_terr", LANG_VAR_CHAR, &msg_mtot_terr, 1, sizeof(char*));

   put_lang_var("msg_mtot_htot", LANG_VAR_CHAR, &msg_mtot_htot, 1, sizeof(char*));
   put_lang_var("msg_mtot_stot", LANG_VAR_CHAR, &msg_mtot_stot, 1, sizeof(char*));

   put_lang_var("msg_mtot_sph", LANG_VAR_CHAR, &msg_mtot_sph, 1, sizeof(char*));
   put_lang_var("msg_mtot_spf", LANG_VAR_CHAR, &msg_mtot_spf, 1, sizeof(char*));
   put_lang_var("msg_mtot_spp", LANG_VAR_CHAR, &msg_mtot_spp, 1, sizeof(char*));

   put_lang_var("msg_mtot_mhv", LANG_VAR_CHAR, &msg_mtot_mhv, 1, sizeof(char*));
   put_lang_var("msg_mtot_mfv", LANG_VAR_CHAR, &msg_mtot_mfv, 1, sizeof(char*));
   put_lang_var("msg_mtot_mpv", LANG_VAR_CHAR, &msg_mtot_mpv, 1, sizeof(char*));
   put_lang_var("msg_mtot_mdv", LANG_VAR_CHAR, &msg_mtot_mdv, 1, sizeof(char*));
   put_lang_var("msg_mtot_mkv", LANG_VAR_CHAR, &msg_mtot_mkv, 1, sizeof(char*));

   put_lang_var("msg_dtot_ds", LANG_VAR_CHAR, &msg_dtot_ds, 1, sizeof(char*));

   put_lang_var("msg_htot_hs", LANG_VAR_CHAR, &msg_htot_hs, 1, sizeof(char*));

   put_lang_var("msg_ctry_use", LANG_VAR_CHAR, &msg_ctry_use, 1, sizeof(char*));

   put_lang_var("msg_top_top", LANG_VAR_CHAR, &msg_top_top, 1, sizeof(char*));
   put_lang_var("msg_top_of", LANG_VAR_CHAR, &msg_top_of, 1, sizeof(char*));
   put_lang_var("msg_top_s", LANG_VAR_CHAR, &msg_top_s, 1, sizeof(char*));
   put_lang_var("msg_top_u", LANG_VAR_CHAR, &msg_top_u, 1, sizeof(char*));
   put_lang_var("msg_top_r", LANG_VAR_CHAR, &msg_top_r, 1, sizeof(char*));
   put_lang_var("msg_top_a", LANG_VAR_CHAR, &msg_top_a, 1, sizeof(char*));
   put_lang_var("msg_top_c", LANG_VAR_CHAR, &msg_top_c, 1, sizeof(char*));
   put_lang_var("msg_top_en", LANG_VAR_CHAR, &msg_top_en, 1, sizeof(char*));
   put_lang_var("msg_top_ex", LANG_VAR_CHAR, &msg_top_ex, 1, sizeof(char*));
   put_lang_var("msg_top_sr", LANG_VAR_CHAR, &msg_top_sr, 1, sizeof(char*));
   put_lang_var("msg_top_i", LANG_VAR_CHAR, &msg_top_i, 1, sizeof(char*));
   put_lang_var("msg_v_hosts", LANG_VAR_CHAR, &msg_v_hosts, 1, sizeof(char*));
   put_lang_var("msg_v_urls", LANG_VAR_CHAR, &msg_v_urls, 1, sizeof(char*));
   put_lang_var("msg_v_refs", LANG_VAR_CHAR, &msg_v_refs, 1, sizeof(char*));
   put_lang_var("msg_v_agents", LANG_VAR_CHAR, &msg_v_agents, 1, sizeof(char*));
   put_lang_var("msg_v_search", LANG_VAR_CHAR, &msg_v_search, 1, sizeof(char*));
   put_lang_var("msg_v_users", LANG_VAR_CHAR, &msg_v_users, 1, sizeof(char*));
   put_lang_var("msg_h_errors", LANG_VAR_CHAR, &msg_h_errors, 1, sizeof(char*));
   put_lang_var("msg_v_errors", LANG_VAR_CHAR, &msg_v_errors, 1, sizeof(char*));
   put_lang_var("msg_h_downloads", LANG_VAR_CHAR, &msg_h_downloads, 1, sizeof(char*));
   put_lang_var("msg_v_downloads", LANG_VAR_CHAR, &msg_v_downloads, 1, sizeof(char*));
   put_lang_var("msg_ref_dreq", LANG_VAR_CHAR, &msg_ref_dreq, 1, sizeof(char*));
   put_lang_var("msg_max_items", LANG_VAR_CHAR, &msg_max_items, 1, sizeof(char*));

   put_lang_var("msg_misc_pages", LANG_VAR_CHAR, &msg_misc_pages, 1, sizeof(char*));
   put_lang_var("msg_misc_visitors", LANG_VAR_CHAR, &msg_misc_visitors, 1, sizeof(char*));
   put_lang_var("msg_misc_robots", LANG_VAR_CHAR, &msg_misc_robots, 1, sizeof(char*));

   put_lang_var("s_month", LANG_VAR_CHARR, s_month, SMONTH_ARRAY_SIZE, sizeof(s_month[0]));
   put_lang_var("l_month", LANG_VAR_CHARR, l_month, LMONTH_ARRAY_SIZE, sizeof(l_month[0]));

   put_lang_var("msg_unit_pfx", LANG_VAR_CHARR, msg_unit_pfx, UNIT_PREFIX_ARRAY_SIZE, sizeof(msg_unit_pfx[0]));
   put_lang_var("msg_xfer_unit", LANG_VAR_CHAR, &msg_xfer_unit, 1, sizeof(char*));

   put_lang_var("response", LANG_VAR_RCARR, response, RESPCODE_ARRAY_SIZE, sizeof(response[0]));

   put_lang_var("msg_title", LANG_VAR_CHAR, &msg_title, 1, sizeof(char*));
   put_lang_var("msg_h_other", LANG_VAR_CHAR, &msg_h_other, 1, sizeof(char*));

   put_lang_var("ctry", LANG_VAR_CCARR, ctry, CCODE_ARRAY_SIZE, sizeof(ctry[0]));

   put_lang_var("msg_h_city", LANG_VAR_CHAR, &msg_h_city, 1, sizeof(char*));
}

//
//   See webalizer_lang.english for file format description
//
void lang_t::proc_lang_file(const char *fname, std::vector<string_t>& errors)
{
   u_char *name, *cptr, *cctld;
   const lang_node_t *lnode;
   int index;
   bool endofinput, quoted;
   string_t str;

   if(!read_lang_file(fname, errors))
      return;

   //
   //   Initialize the language hash table
   //
   init_lang_htab();
   
   //
   //   Parse the file content and re-assign existing language variables
   //
   cptr = lang_buffer;

   //
   // Check for Unicode BOM sequences
   //
   if(cptr[0] == 0xEF && cptr[1] == 0xBB && cptr[2] == 0xBF) {
      cptr += 3;
   }
   else if(cptr[0] == 0xFF && cptr[1] == 0xFE || cptr[0] == 0xFE && cptr[1] == 0xFF) {
      errors.push_back(string_t::_format("Unicode language files must be stored in the UTF-8 format (%s)", fname));
      return;
   }

   while(*cptr) {
      //
      //   Skip comments, orphaned elements and empty lines
      //
      if(*cptr == '#') {
         while(*cptr && !iseolchar(*cptr++));
         continue;
      }

      if(*cptr == ' ' || *cptr == '\t') {                     
         while(*cptr && !iseolchar(*cptr++));
         continue;
      }

      if(iseolchar(*cptr)) {
         cptr++;
         continue;
      }

      //
      //   Extract the variable name and find its descriptor
      //
      name = cptr++;
      while(*cptr && *cptr != ' ' && *cptr != '\t' && *cptr != '=') cptr++;

      if(*cptr == 0) {
         errors.push_back(string_t::_format("The format of the language file is not valid (%s)", fname));
         break;
      }

      *cptr++ = 0;

      // skip unknown language variables
      if((lnode = ln_htab.find_lang_var(str.assign((const char*) name, cptr-name-1))) == NULL) {
         while(!iseolchar(*cptr++));
         continue;
      }

      //
      //   Skip to the first character of the value
      //
      while(*cptr == ' ' || *cptr == '\t' || *cptr == '=') cptr++;

      // ignore language variables with empty values
      if(iseolchar(*cptr))
         continue;

      //
      //   Assign new values
      //
      switch(lnode->vartype) {
         case LANG_VAR_CHAR:
            *(u_char**)lnode->varptr = cptr;
            while(!iseolchar(*cptr)) cptr++;
            *cptr++ = 0;
            break;

         case LANG_VAR_CHARR:
         case LANG_VAR_RCARR:
         case LANG_VAR_CCARR:
            //
            //   Loop through comma-separated array elements. The last element
            // is the one that doesn't have a comma at the end of the line. 
            // If element text contains commas, the entire element must be 
            // enclosed in quotes. For example:
            //
            //      name = element, "elem-part, elem-part", element
            //
            endofinput = false;
            cctld = NULL;

            for(index = 0; index < lnode->maxcount && !endofinput; index++) {
               while(*cptr == ' ' || *cptr == '\t' || iseolchar(*cptr)) cptr++;

               if(*cptr == 0)
                  break;

               if((quoted = (*cptr == '"')))
                  cptr++;

               //
               // Interpret the input based on the variable type.
               // 
               if(lnode->vartype == LANG_VAR_CHARR)
                  ((u_char**)lnode->varptr)[index] = cptr;
               else if(lnode->vartype == LANG_VAR_RCARR) 
                  (((resp_code_t*) lnode->varptr)[index]).desc = (char*) cptr;
               else if(lnode->vartype == LANG_VAR_CCARR) {
                  if(cctld == NULL) {
                     cctld = cptr;
                     index--;                        // adjust index
                  }
                  else {
                     (((country_t*) lnode->varptr)[index]).ccode = (char*) cctld;
                     (((country_t*) lnode->varptr)[index]).desc = (char*) cptr;
                     cctld = NULL;
                  }
               }

               //
               // Skip to the end of the element (ignore commas inside quotes)
               //
               while(*cptr && !iseolchar(*cptr) && (*cptr != ',' || quoted)) {
                  if(quoted && *cptr == '"') {
                     *cptr = 0;
                     quoted = false;
                  }
                  cptr++;
               }

               if(*cptr == 0) {
                  endofinput = true;               // end of file
                  continue;
               }

               if(iseolchar(*cptr)) {
                  endofinput = true;               // end of line
                  *cptr++ = 0;
                  continue;
               }

               //
               // Replace a comma with a zero terminator and skip to the next
               // non-whitespace character. If there's an optional new line
               // character, skip it too.
               //
               *cptr++ = 0;
            }

            //
            //   If the number of elements is less than the size of the destination 
            // array, set the rest of the array to zeros.
            //
            if(index < lnode->maxcount)
               memset(&lnode->varptr[index * lnode->elemsize], 0, lnode->elemsize * (lnode->maxcount - index));

            break;

         default:
            break;
      }
   }

   lang_fname = fname;     // keep the name of the file for reference
}

void lang_t::cleanup_lang_data(void)
{
   ln_htab.clear();

   if(lang_buffer) {
      free(lang_buffer);
      lang_buffer = NULL;
   }
}

void lang_t::put_lang_var(const char *varname, int vartype, void *varptr, int maxcount, size_t elemsize)
{
   ln_htab.put_lang_var(varname, vartype, varptr, maxcount, elemsize);
}

u_int lang_t::resp_code_count(void) const
{
   return RESPCODE_ARRAY_SIZE;
}

const lang_t::resp_code_t& lang_t::get_resp_code(u_int respcode) const
{
   u_int lbound = 1, rbound = RESPCODE_ARRAY_SIZE-1, index = 3;

   while(rbound >= lbound) {
      if(respcode == response[index].code)
         return response[index];

      if(respcode > response[index].code)
         lbound = index + 1;
      else
         rbound = index - 1;

      index = lbound + (rbound - lbound) / 2;
   }

   return response[0];
}

const lang_t::resp_code_t& lang_t::get_resp_code_by_index(u_int index) const
{
   if(index > RESPCODE_ARRAY_SIZE-1)
      return response[0];

   return response[index];
}

void lang_t::report_lang_fname(void) const
{
   if(!lang_fname.isempty())
      printf("%s %s\n", msg_use_lang, lang_fname.c_str());
}

bool lang_t::check_language(const char *lc1, const char *lc2)
{
   while(*lc1 && *lc2 && 
         ((*lc1 == '-' || *lc1 == '_') && (*lc2 == '-' || *lc2 == '_') ||
            string_t::tolower(*lc1) == string_t::tolower(*lc2))) {
      lc1++;
      lc2++;
   }

   return *lc1 == *lc2;
}

bool lang_t::check_language(const char *lc1, const char *lc2, size_t slen)
{
   while(*lc1 && *lc2 && slen &&
         ((*lc1 == '-' || *lc1 == '_') && (*lc2 == '-' || *lc2 == '_') ||
            string_t::tolower(*lc1) == string_t::tolower(*lc2))) {
      slen--;
      lc1++;
      lc2++;
   }

   return !slen || *lc1 == *lc2;
}

//
// instantiate language node and hash table
//
#include "hashtab_tmpl.cpp"

template struct htab_node_t<lang_t::lang_node_t>;
template class hash_table<lang_t::lang_node_t>;

