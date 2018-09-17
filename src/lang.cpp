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
#include <stdexcept>
#include <locale>

#include "lang.h"

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

///
/// @brief  Performs a quick test whether `ch` is any of new line characters.
///
/// This function is intended for quick tests for new line characters where it is not 
/// necessary to know how many new line characters are there exactly. For example, it 
/// can be used to skip all new line characters, but it cannot be used to detect an 
/// empty line. 
///
inline bool iseolchar(char ch) 
{
   return ch == '\n' || ch == '\r';
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

   // register language variable names and their storage
   init_lang_htab();
}

lang_t::~lang_t(void)
{
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

   lang_buffer.resize(filelen + sizeof(char), 0);

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
   ln_htab.emplace("language", &language);
   ln_htab.emplace("language_code", &language_code);

   ln_htab.emplace("msg_processed", &msg_processed);
   ln_htab.emplace("msg_records", &msg_records);
   ln_htab.emplace("msg_addresses", &msg_addresses);
   ln_htab.emplace("msg_ignored", &msg_ignored);
   ln_htab.emplace("msg_bad", &msg_bad);
   ln_htab.emplace("msg_in", &msg_in);
   ln_htab.emplace("msg_seconds", &msg_seconds);
   ln_htab.emplace("msg_runtime", &msg_runtime);
   ln_htab.emplace("msg_dnstime", &msg_dnstime);
   ln_htab.emplace("msg_mnttime", &msg_mnttime);
   ln_htab.emplace("msg_rpttime", &msg_rpttime);
   ln_htab.emplace("msg_cmpctdb", &msg_cmpctdb);
   ln_htab.emplace("msg_nofile", &msg_nofile);
   ln_htab.emplace("msg_file_err", &msg_file_err);
   ln_htab.emplace("msg_use_help", &msg_use_help);
   ln_htab.emplace("msg_fpos_err", &msg_fpos_err);

   ln_htab.emplace("msg_log_err", &msg_log_err);
   ln_htab.emplace("msg_log_use", &msg_log_use);
   ln_htab.emplace("msg_log_done", &msg_log_done);
   ln_htab.emplace("msg_dir_err", &msg_dir_err);
   ln_htab.emplace("msg_dir_use", &msg_dir_use);
   ln_htab.emplace("msg_cur_dir", &msg_cur_dir);
   ln_htab.emplace("msg_hostname", &msg_hostname);
   ln_htab.emplace("msg_ign_hist", &msg_ign_hist);
   ln_htab.emplace("msg_no_hist", &msg_no_hist);
   ln_htab.emplace("msg_get_hist", &msg_get_hist);
   ln_htab.emplace("msg_put_hist", &msg_put_hist);
   ln_htab.emplace("msg_hist_err", &msg_hist_err);
   ln_htab.emplace("msg_bad_hist", &msg_bad_hist);
   ln_htab.emplace("msg_bad_conf", &msg_bad_conf);
   ln_htab.emplace("msg_bad_key", &msg_bad_key);
   ln_htab.emplace("msg_bad_date", &msg_bad_date);
   ln_htab.emplace("msg_ign_nscp", &msg_ign_nscp);
   ln_htab.emplace("msg_bad_rec", &msg_bad_rec);
   ln_htab.emplace("msg_no_vrec", &msg_no_vrec);
   ln_htab.emplace("msg_gen_rpt", &msg_gen_rpt);
   ln_htab.emplace("msg_gen_sum", &msg_gen_sum);
   ln_htab.emplace("msg_get_data", &msg_get_data);
   ln_htab.emplace("msg_put_data", &msg_put_data);
   ln_htab.emplace("msg_no_data", &msg_no_data);
   ln_htab.emplace("msg_bad_data", &msg_bad_data);
   ln_htab.emplace("msg_data_err", &msg_data_err);
   ln_htab.emplace("msg_dup_data", &msg_dup_data);
   ln_htab.emplace("msg_afm_err", &msg_afm_err);
   ln_htab.emplace("msg_ctrl_c", &msg_ctrl_c);
   ln_htab.emplace("msg_pars_err", &msg_pars_err);
   ln_htab.emplace("msg_use_conf", &msg_use_conf);
   ln_htab.emplace("msg_use_lang", &msg_use_lang);
   ln_htab.emplace("msg_use_db", &msg_use_db);

   ln_htab.emplace("msg_dns_nocf", &msg_dns_nocf);
   ln_htab.emplace("msg_dns_nodb", &msg_dns_nodb);
   ln_htab.emplace("msg_dns_nolk", &msg_dns_nolk);
   ln_htab.emplace("msg_dns_usec", &msg_dns_usec);
   ln_htab.emplace("msg_dns_rslv", &msg_dns_rslv);
   ln_htab.emplace("msg_dns_init", &msg_dns_init);
   ln_htab.emplace("msg_dns_htrt", &msg_dns_htrt);
   ln_htab.emplace("msg_dns_geoe", &msg_dns_geoe);
   ln_htab.emplace("msg_dns_useg", &msg_dns_useg);

   ln_htab.emplace("msg_big_rec", &msg_big_rec);
   ln_htab.emplace("msg_big_host", &msg_big_host);
   ln_htab.emplace("msg_big_date", &msg_big_date);
   ln_htab.emplace("msg_big_req", &msg_big_req);
   ln_htab.emplace("msg_big_ref", &msg_big_ref);
   ln_htab.emplace("msg_big_user", &msg_big_user);
   ln_htab.emplace("msg_big_url", &msg_big_url);
   ln_htab.emplace("msg_big_agnt", &msg_big_agnt);
   ln_htab.emplace("msg_big_srch", &msg_big_srch);
   ln_htab.emplace("msg_big_one", &msg_big_one);
   ln_htab.emplace("msg_no_open", &msg_no_open);

   ln_htab.emplace("h_usage1", &h_usage1);
   ln_htab.emplace("h_usage2", &h_usage2);

   ln_htab.emplace("h_msg", &h_msg);

   ln_htab.emplace("msg_hhdr_sp", &msg_hhdr_sp);
   ln_htab.emplace("msg_hhdr_gt", &msg_hhdr_gt);

   ln_htab.emplace("msg_main_us", &msg_main_us);
   ln_htab.emplace("msg_main_per", &msg_main_per);
   ln_htab.emplace("msg_main_sum", &msg_main_sum);
   ln_htab.emplace("msg_main_da", &msg_main_da);
   ln_htab.emplace("msg_main_mt", &msg_main_mt);

   ln_htab.emplace("msg_main_plst", &msg_main_plst);
   ln_htab.emplace("msg_main_pmns", &msg_main_pmns);

   ln_htab.emplace("msg_hmth_du", &msg_hmth_du);
   ln_htab.emplace("msg_hmth_hu", &msg_hmth_hu);

   ln_htab.emplace("msg_h_by", &msg_h_by);
   ln_htab.emplace("msg_h_avg", &msg_h_avg);
   ln_htab.emplace("msg_h_max", &msg_h_max);
   ln_htab.emplace("msg_h_total", &msg_h_total);
   ln_htab.emplace("msg_h_totals", &msg_h_totals);
   ln_htab.emplace("msg_h_day", &msg_h_day);
   ln_htab.emplace("msg_h_mth", &msg_h_mth);
   ln_htab.emplace("msg_h_hour", &msg_h_hour);
   ln_htab.emplace("msg_h_hits", &msg_h_hits);
   ln_htab.emplace("msg_h_pages", &msg_h_pages);
   ln_htab.emplace("msg_h_visits", &msg_h_visits);
   ln_htab.emplace("msg_h_files", &msg_h_files);
   ln_htab.emplace("msg_h_hosts", &msg_h_hosts);
   ln_htab.emplace("msg_h_xfer", &msg_h_xfer);
   ln_htab.emplace("msg_h_avgtime", &msg_h_avgtime);
   ln_htab.emplace("msg_h_maxtime", &msg_h_maxtime);
   ln_htab.emplace("msg_h_hname", &msg_h_hname);
   ln_htab.emplace("msg_h_host", &msg_h_host);
   ln_htab.emplace("msg_h_ipaddr", &msg_h_ipaddr);
   ln_htab.emplace("msg_h_url", &msg_h_url);
   ln_htab.emplace("msg_h_agent", &msg_h_agent);
   ln_htab.emplace("msg_h_ref", &msg_h_ref);
   ln_htab.emplace("msg_h_ctry", &msg_h_ctry);
   ln_htab.emplace("msg_h_ccode", &msg_h_ccode);
   ln_htab.emplace("msg_h_search", &msg_h_search);
   ln_htab.emplace("msg_h_uname", &msg_h_uname);
   ln_htab.emplace("msg_h_type", &msg_h_type);
   ln_htab.emplace("msg_h_status", &msg_h_status);
   ln_htab.emplace("msg_h_duration", &msg_h_duration);
   ln_htab.emplace("msg_h_method", &msg_h_method);
   ln_htab.emplace("msg_h_download", &msg_h_download);
   ln_htab.emplace("msg_h_count", &msg_h_count);
   ln_htab.emplace("msg_h_time", &msg_h_time);
   ln_htab.emplace("msg_h_spammer", &msg_h_spammer);
   ln_htab.emplace("msg_h_urls", &msg_h_urls);
   ln_htab.emplace("msg_h_refs", &msg_h_refs);
   ln_htab.emplace("msg_h_agents", &msg_h_agents);
   ln_htab.emplace("msg_h_chosts", &msg_h_chosts);
   ln_htab.emplace("msg_h_cvisits", &msg_h_cvisits);
   ln_htab.emplace("msg_h_latitude", &msg_h_latitude);
   ln_htab.emplace("msg_h_longitude", &msg_h_longitude);

   ln_htab.emplace("msg_hlnk_sum", &msg_hlnk_sum);
   ln_htab.emplace("msg_hlnk_ds", &msg_hlnk_ds);
   ln_htab.emplace("msg_hlnk_hs", &msg_hlnk_hs);
   ln_htab.emplace("msg_hlnk_u", &msg_hlnk_u);
   ln_htab.emplace("msg_hlnk_s", &msg_hlnk_s);
   ln_htab.emplace("msg_hlnk_a", &msg_hlnk_a);
   ln_htab.emplace("msg_hlnk_c", &msg_hlnk_c);
   ln_htab.emplace("msg_hlnk_r", &msg_hlnk_r);
   ln_htab.emplace("msg_hlnk_en", &msg_hlnk_en);
   ln_htab.emplace("msg_hlnk_ex", &msg_hlnk_ex);
   ln_htab.emplace("msg_hlnk_sr", &msg_hlnk_sr);
   ln_htab.emplace("msg_hlnk_i", &msg_hlnk_i);
   ln_htab.emplace("msg_hlnk_err", &msg_hlnk_err);
   ln_htab.emplace("msg_hlnk_dl", &msg_hlnk_dl);

   ln_htab.emplace("msg_mtot_ms", &msg_mtot_ms);
   ln_htab.emplace("msg_mtot_th", &msg_mtot_th);
   ln_htab.emplace("msg_mtot_tf", &msg_mtot_tf);
   ln_htab.emplace("msg_mtot_tp", &msg_mtot_tp);
   ln_htab.emplace("msg_mtot_tv", &msg_mtot_tv);
   ln_htab.emplace("msg_mtot_tx", &msg_mtot_tx);
   ln_htab.emplace("msg_mtot_us", &msg_mtot_us);
   ln_htab.emplace("msg_mtot_ur", &msg_mtot_ur);
   ln_htab.emplace("msg_mtot_ua", &msg_mtot_ua);
   ln_htab.emplace("msg_mtot_uu", &msg_mtot_uu);
   ln_htab.emplace("msg_mtot_ui", &msg_mtot_ui);
   ln_htab.emplace("msg_mtot_mhd", &msg_mtot_mhd);
   ln_htab.emplace("msg_mtot_mhh", &msg_mtot_mhh);
   ln_htab.emplace("msg_mtot_mfd", &msg_mtot_mfd);
   ln_htab.emplace("msg_mtot_mpd", &msg_mtot_mpd);
   ln_htab.emplace("msg_mtot_mvd", &msg_mtot_mvd);
   ln_htab.emplace("msg_mtot_mkd", &msg_mtot_mkd);
   ln_htab.emplace("msg_mtot_rc", &msg_mtot_rc);
   ln_htab.emplace("msg_mtot_dl", &msg_mtot_dl);
   ln_htab.emplace("msg_mtot_tcv", &msg_mtot_tcv);
   ln_htab.emplace("msg_mtot_tch", &msg_mtot_tch);
   ln_htab.emplace("msg_mtot_hcr", &msg_mtot_hcr);
   ln_htab.emplace("msg_mtot_cvd", &msg_mtot_cvd);
   ln_htab.emplace("msg_mtot_conv", &msg_mtot_conv);
   ln_htab.emplace("msg_mtot_rtot", &msg_mtot_rtot);
   ln_htab.emplace("msg_mtot_perf", &msg_mtot_perf);
   ln_htab.emplace("msg_mtot_hdt", &msg_mtot_hdt);
   ln_htab.emplace("msg_mtot_terr", &msg_mtot_terr);

   ln_htab.emplace("msg_mtot_htot", &msg_mtot_htot);
   ln_htab.emplace("msg_mtot_stot", &msg_mtot_stot);

   ln_htab.emplace("msg_mtot_sph", &msg_mtot_sph);
   ln_htab.emplace("msg_mtot_spf", &msg_mtot_spf);
   ln_htab.emplace("msg_mtot_spp", &msg_mtot_spp);

   ln_htab.emplace("msg_mtot_mhv", &msg_mtot_mhv);
   ln_htab.emplace("msg_mtot_mfv", &msg_mtot_mfv);
   ln_htab.emplace("msg_mtot_mpv", &msg_mtot_mpv);
   ln_htab.emplace("msg_mtot_mdv", &msg_mtot_mdv);
   ln_htab.emplace("msg_mtot_mkv", &msg_mtot_mkv);

   ln_htab.emplace("msg_dtot_ds", &msg_dtot_ds);

   ln_htab.emplace("msg_htot_hs", &msg_htot_hs);

   ln_htab.emplace("msg_ctry_use", &msg_ctry_use);

   ln_htab.emplace("msg_top_top", &msg_top_top);
   ln_htab.emplace("msg_top_of", &msg_top_of);
   ln_htab.emplace("msg_top_s", &msg_top_s);
   ln_htab.emplace("msg_top_u", &msg_top_u);
   ln_htab.emplace("msg_top_r", &msg_top_r);
   ln_htab.emplace("msg_top_a", &msg_top_a);
   ln_htab.emplace("msg_top_c", &msg_top_c);
   ln_htab.emplace("msg_top_en", &msg_top_en);
   ln_htab.emplace("msg_top_ex", &msg_top_ex);
   ln_htab.emplace("msg_top_sr", &msg_top_sr);
   ln_htab.emplace("msg_top_i", &msg_top_i);
   ln_htab.emplace("msg_v_hosts", &msg_v_hosts);
   ln_htab.emplace("msg_v_urls", &msg_v_urls);
   ln_htab.emplace("msg_v_refs", &msg_v_refs);
   ln_htab.emplace("msg_v_agents", &msg_v_agents);
   ln_htab.emplace("msg_v_search", &msg_v_search);
   ln_htab.emplace("msg_v_users", &msg_v_users);
   ln_htab.emplace("msg_h_errors", &msg_h_errors);
   ln_htab.emplace("msg_v_errors", &msg_v_errors);
   ln_htab.emplace("msg_h_downloads", &msg_h_downloads);
   ln_htab.emplace("msg_v_downloads", &msg_v_downloads);
   ln_htab.emplace("msg_ref_dreq", &msg_ref_dreq);
   ln_htab.emplace("msg_max_items", &msg_max_items);

   ln_htab.emplace("msg_misc_pages", &msg_misc_pages);
   ln_htab.emplace("msg_misc_visitors", &msg_misc_visitors);
   ln_htab.emplace("msg_misc_robots", &msg_misc_robots);

   ln_htab.emplace("s_month", &s_month);
   ln_htab.emplace("l_month", &l_month);

   ln_htab.emplace("msg_unit_pfx", &msg_unit_pfx);
   ln_htab.emplace("msg_xfer_unit", &msg_xfer_unit);

   ln_htab.emplace("response", &response);

   ln_htab.emplace("msg_title", &msg_title);
   ln_htab.emplace("msg_h_other", &msg_h_other);

   ln_htab.emplace("ctry", &ctry);

   ln_htab.emplace("msg_h_city", &msg_h_city);
}

//
//   See webalizer_lang.english for file format description
//
void lang_t::proc_lang_file(const char *fname, std::vector<string_t>& errors)
{
   if(!read_lang_file(fname, errors))
      return;

   // hold onto the name of the file
   lang_fname = fname;

   parse_lang_file(fname, lang_buffer, errors);
}

void lang_t::parse_lang_file(const char *fname, char *buffer, std::vector<string_t>& errors)
{
   char *cptr;
   const char *name;
   lang_hash_table::iterator lit;
   lang_node_t *lnode;

   cptr = buffer;

   //
   // Check for Unicode BOM sequences
   //
   if(cptr[0] == '\xEF' && cptr[1] == '\xBB' && cptr[2] == '\xBF') {
      cptr += 3;
   }
   else if(cptr[0] == '\xFF' && cptr[1] == '\xFE' || cptr[0] == '\xFE' && cptr[1] == '\xFF') {
      errors.push_back(string_t::_format("Unicode language files must be stored in the UTF-8 format (%s)", fname));
      return;
   }

   while(*cptr) {
      // skip comments
      if(*cptr == '#') {
         while(*cptr && !iseolchar(*cptr++));
         continue;
      }

      // error out if a line starts with a space or a tab
      if(*cptr == ' ' || *cptr == '\t') {                     
         errors.push_back(string_t::_format("Language variable cannot begin with a space or a tab (%s)", fname));
         break;
      }

      // skip empty lines
      if(iseolchar(*cptr)) {
         cptr++;
         continue;
      }

      if(*cptr == 0) {
         errors.push_back(string_t::_format("Unexpected end of the language file (%s)", fname));
         break;
      }

      //
      // Extract the variable name and find its descriptor
      //
      name = cptr++;
      while(*cptr && *cptr != ' ' && *cptr != '\t' && *cptr != '=') cptr++;

      if(*cptr == 0) {
         errors.push_back(string_t::_format("Unexpected end of the language file (%s)", fname));
         break;
      }

      // look up the language variable by name (name is not null-terminated)
      if((lit = ln_htab.find(std::string_view(name, cptr-name))) == ln_htab.end()) {
         // skip unknown language variables
         errors.push_back(string_t::_format("Unknown language variable (%s)", std::string(name, cptr-name).c_str()));
         while(*cptr && !iseolchar(*cptr++));
         continue;
      }

      lnode = &lit->second;
      lnode->reset();

      // skip any whitespace after the name
      while(*cptr && *cptr == ' ' || *cptr == '\t') cptr++;

      // make sure there's an equal sign
      if(*cptr != '=') {
         while(*cptr && !iseolchar(*cptr++));
         errors.push_back(string_t::_format("Language variable %s must be followed by an equal sign", std::string(lit->first).c_str()));
         break;
      }

      cptr++;

      // skip the whitespace before the value
      while(*cptr == ' ' || *cptr == '\t') cptr++;

      // ignore language variables with empty values
      if(!*cptr || iseolchar(*cptr))
         continue;

      //
      // Assign new values
      //
      switch(lnode->vartype) {
         case LANG_VAR_CHAR:
            *lnode->msg = cptr;
            while(*cptr && !iseolchar(*cptr)) cptr++;
            if(*cptr)
               *cptr++ = 0;
            break;

         case LANG_VAR_CHARR:
         case LANG_VAR_RCARR:
         case LANG_VAR_CCARR:
            bool comma = false;           // trailing comma after the previous element?
            bool quoted = false;          // inside a quoted value?

            const char *cctld = nullptr;  // country code

            //
            // Loop through comma-separated array elements. The array may end 
            // with a new line without a trailing comma or with an empty line. 
            // If the element text contains commas, the entire element must be 
            // enclosed in quotes. For example:
            //
            //    name = element, element
            //    name = element, 
            //           element,
            //
            //    name = element, "elem-part, elem-part", element
            //
            // Note that we must evaluate full end-of-line sequences here in
            // order to detect an empty line reliably.
            //
            do {
               quoted = comma = false;

               // skip leading whitespace
               while(*cptr && *cptr == ' ' || *cptr == '\t') cptr++;

               // an empty line indicates there are no more array elements
               if(*cptr == '\r' && *(cptr + 1) == '\n')
                  break;
               else if(*cptr == '\r' || *cptr == '\n')
                  break;

               // check if it's a quoted value
               if(*cptr == '"') {
                  quoted = true;
                  cptr++;
               }

               // check for a missing value
               if(*cptr == 0) {
                  errors.push_back(string_t::_format("Missing value for language variable %s", std::string(lit->first).c_str()));
                  break;
               }

               const char *value = cptr;

               //
               // Skip to the end of the element (ignore commas inside quotes)
               //
               while(*cptr && !iseolchar(*cptr) && (*cptr != ',' || quoted)) {
                  // terminate a quoted value and keep going
                  if(quoted && *cptr == '"') {
                     *cptr = 0;
                     quoted = false;
                  }
                  cptr++;
               }

               // report unbalanced quotes
               if(quoted) {
                  errors.push_back(string_t::_format("Unbalanced quotes for language variable %s", std::string(lit->first).c_str()));
                  return;
               }

               // terminate non-quoted values
               if(*cptr == ',') {
                  *cptr++ = 0;
                  comma = true;
               
                  // skip trailing whitespace
                  while(*cptr && (*cptr == ' ' || *cptr == '\t')) cptr++;
               }
               
               if(*cptr == '\r' && *(cptr + 1) == '\n') {
                  *cptr++ = 0;
                  *cptr++;
               }
               else if(*cptr == '\r' || *cptr == '\n')
                  *cptr++ = 0;

               // push the value into its vector by type
               if(lnode->vartype == LANG_VAR_CHARR)
                  lnode->msgs->push_back(value);
               else if(lnode->vartype == LANG_VAR_RCARR) {
                  const char *cp = value;

                  // find where the HTTP code starts in the text
                  while(*cp && !std::isdigit(*cp, std::locale::classic())) cp++;

                  lnode->respcode->push_back({*cp ? (u_int) atoi(cp) : 0, value});
               }
               else if(lnode->vartype == LANG_VAR_CCARR) {
                  // country entries consist of two comma-separated values
                  if(cctld == nullptr)
                     cctld = value;
                  else {
                     lnode->country->push_back({cctld, value});
                     cctld = nullptr;
                  }
               }
            } while(*cptr && comma);

            break;
      }
   }
}

void lang_t::cleanup_lang_data(void)
{
   ln_htab.clear();
}

u_int lang_t::resp_code_count(void) const
{
   return (u_int) response.size();
}

const lang_t::resp_code_t& lang_t::get_resp_code(u_int respcode) const
{
   u_int lbound = 1, rbound = (u_int) response.size()-1, index = 3;

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
   if(index > response.size()-1)
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
