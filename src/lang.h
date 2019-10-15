#ifndef LANG_H
#define LANG_H

#include "hashtab.h"
#include "tstring.h"
#include "types.h"

#include <vector>
#include <unordered_map>
#include <memory>

/// Unit test classes that need access to private members.
namespace sswtest {
   class LangTest_AssignCharLangVarSpace_Test;
   class LangTest_AssignCharLangVarLineEnding_Test;
   class LangTest_AssignCharArrLangVarOneLine_Test;
   class LangTest_AssignCharArrLangVarMultiLine_Test;
   class LangTest_AssignRespArrLangVarOneLine_Test;
   class LangTest_AssignRespArrLangVarMultiLine_Test;
   class LangTest_AssignCharArrLangVarQuoted_Test;
   class LangTest_AssignCharArrTrailingComma_Test;
   class LangTest_AssignCharArrTrailingCommaNoLineEnding_Test;
   class LangTest_AssignCountryArrLangVarMultiLine_Test;
   class LangTest_AssignCharArrLangVarUnbalancedQuotes_Test;
}

///
/// @class  lang_t
///
/// @brief  A class that reads a specific language file and makes all language strings 
///         available to the application
///
class lang_t {
   friend class sswtest::LangTest_AssignCharLangVarSpace_Test;
   friend class sswtest::LangTest_AssignCharLangVarLineEnding_Test;
   friend class sswtest::LangTest_AssignCharArrLangVarOneLine_Test;
   friend class sswtest::LangTest_AssignCharArrLangVarMultiLine_Test;
   friend class sswtest::LangTest_AssignRespArrLangVarOneLine_Test;
   friend class sswtest::LangTest_AssignRespArrLangVarMultiLine_Test;
   friend class sswtest::LangTest_AssignCharArrLangVarQuoted_Test;
   friend class sswtest::LangTest_AssignCharArrTrailingComma_Test;
   friend class sswtest::LangTest_AssignCharArrTrailingCommaNoLineEnding_Test;
   friend class sswtest::LangTest_AssignCountryArrLangVarMultiLine_Test;
   friend class sswtest::LangTest_AssignCharArrLangVarUnbalancedQuotes_Test;

   public:
      ///
      /// @struct resp_code_t
      ///
      /// #brief  A localized HTTP response code and its description
      ///
      struct resp_code_t {
         u_int         code;
         const char    *desc;
      };

      ///
      /// @struct country_t
      ///
      /// @brief  A localized country code and name
      ///
      struct country_t {
         const char    *ccode;
         const char    *desc;
      };

   private:
      ///
      /// @brief  Language variable type
      ///
      enum lang_vartype_t {
         LANG_VAR_CHAR,                   ///< Individual text message (char **)
         LANG_VAR_CHARR,                  ///< Array of text messages (char *[])
         LANG_VAR_RCARR,                  ///< Array of HTTP response codes (response_code[])
         LANG_VAR_CCARR                   ///< Array of country codes and names (country_code[])
      };

      ///
      /// @struct lang_node_t
      ///
      /// @brief  A localized language entry node
      ///
      struct lang_node_t {
            lang_vartype_t       vartype;             ///< Language variable type.

            union {
               const char                 **msg;      ///< A single message (`LANG_VAR_CHAR`)
               std::vector<const char*>   *msgs;      ///< An array of messages (`LANG_VAR_CHARR`)
               std::vector<resp_code_t>   *respcode;  ///< An array of HTTP response codes (`LANG_VAR_RCARR`)
               std::vector<country_t>     *country;   ///< An array of country descriptors (`LANG_VAR_CCARR`)
            };

            public:
               lang_node_t(const char **msg) :
                     vartype(LANG_VAR_CHAR), msg(msg)
               {
               }

               lang_node_t(std::vector<const char*> *msgs) :
                     vartype(LANG_VAR_CHARR), msgs(msgs)
               {
               }

               lang_node_t(std::vector<resp_code_t> *respcode) :
                     vartype(LANG_VAR_RCARR), respcode(respcode)
               {
               }

               lang_node_t(std::vector<country_t> *country) :
                     vartype(LANG_VAR_CCARR), country(country)
               {
               }

               void reset(void)
               {
                  switch (vartype) {
                     case LANG_VAR_CHAR:
                        *msg = nullptr;
                        break;
                     case LANG_VAR_CHARR:
                        msgs->clear();
                        break;
                     case LANG_VAR_RCARR:
                        respcode->clear();
                        break;
                     case LANG_VAR_CCARR:
                        country->clear();
                        break;
                  }
               }
      };

      ///
      /// All language variable names are string literals in `lang.cpp` wrapped in an
      /// instance of `string_t` and used as a key in the hash map below.
      ///
      typedef std::unordered_map<string_t, lang_node_t, hash_string> lang_hash_table;

   public:
      const char *msg_ctrl_c;

      const char *msg_dns_nocf;
      const char *msg_dns_nodb;
      const char *msg_dns_nolk;
      const char *msg_dns_usec;
      const char *msg_dns_rslv;
      const char *msg_dns_init;
      const char *msg_dns_htrt;
      const char *msg_dns_geoe;
      const char *msg_dns_asne;
      const char *msg_dns_useg;
      const char *msg_dns_usea;

      const char *h_usage1;
      const char *h_usage2;
      std::vector<const char*> h_msg;

      std::vector<const char*> s_month;
      std::vector<const char*> l_month;

      std::vector<const char*> msg_unit_pfx;
      const char *msg_xfer_unit;

      std::vector<resp_code_t> response;

      std::vector<country_t> ctry;

      const char *language;
      const char *language_code;

      const char *msg_processed;
      const char *msg_records ;
      const char *msg_addresses;
      const char *msg_ignored ;
      const char *msg_bad     ;
      const char *msg_in      ;
      const char *msg_seconds ;
      const char *msg_runtime ;
      const char *msg_dnstime ;
      const char *msg_mnttime ;
      const char *msg_rpttime ;
      const char *msg_cmpctdb ;
      const char *msg_nofile  ;
      const char *msg_file_err;
      const char *msg_use_help;
      const char *msg_fpos_err;

      const char *msg_log_err ;
      const char *msg_log_use ;
      const char *msg_log_done;
      const char *msg_dir_err ;
      const char *msg_dir_use ;
      const char *msg_cur_dir ;
      const char *msg_hostname;
      const char *msg_ign_hist;
      const char *msg_no_hist ;
      const char *msg_get_hist;
      const char *msg_put_hist;
      const char *msg_hist_err;
      const char *msg_bad_hist;
      const char *msg_bad_conf;
      const char *msg_bad_key ;
      const char *msg_bad_date;
      const char *msg_ign_nscp;
      const char *msg_bad_rec ;
      const char *msg_no_vrec ;
      const char *msg_gen_rpt ;
      const char *msg_gen_sum ;
      const char *msg_get_data;
      const char *msg_put_data;
      const char *msg_no_data ;
      const char *msg_bad_data;
      const char *msg_data_err;
      const char *msg_dup_data;
      const char *msg_afm_err;
      const char *msg_pars_err;
      const char *msg_use_conf;
      const char *msg_use_lang;
      const char *msg_use_db;

      const char *msg_big_rec ;
      const char *msg_big_host;
      const char *msg_big_date;
      const char *msg_big_req ;
      const char *msg_big_ref ;
      const char *msg_big_user;
      const char *msg_big_url;
      const char *msg_big_agnt;
      const char *msg_big_srch;
      const char *msg_big_one ;

      const char *msg_no_open ;

      /* HTML Strings */

      const char *msg_hhdr_sp ;
      const char *msg_hhdr_gt ;

      const char *msg_main_us ;
      const char *msg_main_per;
      const char *msg_main_sum;
      const char *msg_main_da ;
      const char *msg_main_mt ;
      const char *msg_main_plst;
      const char *msg_main_pmns;

      const char *msg_hmth_du ;
      const char *msg_hmth_hu ;

      const char *msg_h_by    ;
      const char *msg_h_avg   ;
      const char *msg_h_max   ;
      const char *msg_h_total ;
      const char *msg_h_totals;
      const char *msg_h_day   ;
      const char *msg_h_mth   ;
      const char *msg_h_hour  ;
      const char *msg_h_hits  ;
      const char *msg_h_pages ;
      const char *msg_h_visits;
      const char *msg_h_cvisits;
      const char *msg_h_files ;
      const char *msg_h_hosts ;
      const char *msg_h_chosts;
      const char *msg_h_xfer  ;
      const char *msg_h_downloads;
      const char *msg_h_avgtime;
      const char *msg_h_maxtime;
      const char *msg_h_hname ;
      const char *msg_h_host;
      const char *msg_h_ipaddr;
      const char *msg_h_url   ;
      const char *msg_h_urls  ;
      const char *msg_h_agent ;
      const char *msg_h_agents;
      const char *msg_h_ref   ;
      const char *msg_h_refs  ;
      const char *msg_h_ctry  ;
      const char *msg_h_ccode ;
      const char *msg_h_search;
      const char *msg_h_uname ;
      const char *msg_h_type  ;
      const char *msg_h_status;
      const char *msg_h_duration;
      const char *msg_h_method;
      const char *msg_h_download;
      const char *msg_h_count ;
      const char *msg_h_time  ;
      const char *msg_h_spammer;
      const char *msg_h_errors;
      const char *msg_h_latitude;
      const char *msg_h_longitude;

      const char *msg_hlnk_sum ;
      const char *msg_hlnk_ds ;
      const char *msg_hlnk_hs ;
      const char *msg_hlnk_u  ;
      const char *msg_hlnk_s  ;
      const char *msg_hlnk_a  ;
      const char *msg_hlnk_c  ;
      const char *msg_hlnk_ct ;
      const char *msg_hlnk_r  ;
      const char *msg_hlnk_en ;
      const char *msg_hlnk_ex ;
      const char *msg_hlnk_sr ;
      const char *msg_hlnk_i  ;
      const char *msg_hlnk_err;
      const char *msg_hlnk_dl ;

      const char *msg_mtot_ms ;
      const char *msg_mtot_th ;
      const char *msg_mtot_tf ;
      const char *msg_mtot_tp ;
      const char *msg_mtot_tv ;
      const char *msg_mtot_tx ;
      const char *msg_mtot_terr;
      const char *msg_mtot_us ;
      const char *msg_mtot_ur ;
      const char *msg_mtot_ua ;
      const char *msg_mtot_uu ;
      const char *msg_mtot_ui ;
      const char *msg_mtot_mhd;
      const char *msg_mtot_mhh;
      const char *msg_mtot_mfd;
      const char *msg_mtot_mpd;
      const char *msg_mtot_mvd;
      const char *msg_mtot_mkd;
      const char *msg_mtot_rc ;
      const char *msg_mtot_sph;
      const char *msg_mtot_spf;
      const char *msg_mtot_spp;
      const char *msg_mtot_mhv;
      const char *msg_mtot_mfv;
      const char *msg_mtot_mpv;
      const char *msg_mtot_mdv;
      const char *msg_mtot_mkv;
      const char *msg_mtot_dl ;
      const char *msg_mtot_tcv;
      const char *msg_mtot_tch;
      const char *msg_mtot_hcr;
      const char *msg_mtot_cvd;
      const char *msg_mtot_conv;
      const char *msg_mtot_rtot;
      const char *msg_mtot_perf;
      const char *msg_mtot_hdt;
      const char *msg_mtot_htot;
      const char *msg_mtot_stot;
      
      const char *msg_dtot_ds ;

      const char *msg_htot_hs ;

      const char *msg_ctry_use;

      const char *msg_top_top ;
      const char *msg_top_of  ;
      const char *msg_top_s   ;
      const char *msg_top_u   ;
      const char *msg_top_r   ;
      const char *msg_top_a   ;
      const char *msg_top_c   ;
      const char *msg_top_ct  ;
      const char *msg_top_en  ;
      const char *msg_top_ex  ;
      const char *msg_top_sr  ;
      const char *msg_top_i   ;
      const char *msg_v_hosts ;
      const char *msg_v_urls  ;
      const char *msg_v_refs  ;
      const char *msg_v_agents;
      const char *msg_v_search;
      const char *msg_v_users ;
      const char *msg_misc_pages;
      const char *msg_misc_visitors;
      const char *msg_misc_robots;
      const char *msg_v_errors;
      const char *msg_v_downloads;
      const char *msg_ref_dreq;
      const char *msg_max_items;

      const char *msg_title   ;
      const char *msg_h_other ;

      const char *msg_h_city;
      
   private:
      string_t lang_fname;

      lang_hash_table ln_htab;

      string_t::char_buffer_t lang_buffer;

   private:
      void init_lang_htab(void);

      bool read_lang_file(const char *fname, std::vector<string_t>& errors);

      void parse_lang_file(const char *fname, char *buffer, std::vector<string_t>& errors);

   public:
      lang_t(void);
      ~lang_t(void);

      void report_lang_fname(void) const;

      void proc_lang_file(const char *fname, std::vector<string_t>& errors);
      void cleanup_lang_data(void);

      const resp_code_t& get_resp_code(u_int respcode) const;

      static bool check_language(const char *lc1, const char *lc2);
      static bool check_language(const char *lc1, const char *lc2, size_t slen);
};

#endif  // _LANG_H
