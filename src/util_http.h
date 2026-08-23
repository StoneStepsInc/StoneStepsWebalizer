/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_http.h
*/
#ifndef UTIL_HTTP_H
#define UTIL_HTTP_H

#include "types.h"

#include <type_traits>

//
// HTTP response codes
//
// http://www.iana.org/assignments/http-status-codes/
// 
// Non-standard status codes are prefixed by their origin, as follows:
// 
//    * RC_NG - Nginx
//
enum rc_t : u_short {
    RC_CONTINUE         = 100,
    RC_SWITCHPROTO      = 101,
    RC_PROCESSING       = 102,        ///< Processing
    RC_EARLYHINTS       = 103,        ///< Early Hints
    RC_OK               = 200,
    RC_CREATED          = 201,
    RC_ACCEPTED         = 202,
    RC_NONAUTHINFO      = 203,
    RC_NOCONTENT        = 204,
    RC_RESETCONTENT     = 205,
    RC_PARTIALCONTENT   = 206,
    RC_MULTISTATUS      = 207,        ///< Multi-Status
    RC_ALREADYREPORTED  = 208,        ///< Already Reported
    RC_IMUSED           = 226,        ///< IM Used
    RC_MULTIPLECHOICES  = 300,
    RC_MOVEDPERM        = 301,
    RC_MOVEDTEMP        = 302,
    RC_SEEOTHER         = 303,
    RC_NOMOD            = 304,
    RC_USEPROXY         = 305,
    RC_UNUSED           = 306,        ///< (Unused)
    RC_MOVEDTEMPORARILY = 307,
    RC_PERMREDIRECT     = 308,        ///< Permanent Redirect
    RC_BAD              = 400,
    RC_UNAUTH           = 401,
    RC_PAYMENTREQ       = 402,
    RC_FORBIDDEN        = 403,
    RC_NOTFOUND         = 404,
    RC_METHODNOTALLOWED = 405,
    RC_NOTACCEPTABLE    = 406,
    RC_PROXYAUTHREQ     = 407,
    RC_TIMEOUT          = 408,
    RC_CONFLICT         = 409,
    RC_GONE             = 410,
    RC_LENGTHREQ        = 411,
    RC_PREFAILED        = 412,
    RC_REQENTTOOLARGE   = 413,
    RC_REQURITOOLARGE   = 414,
    RC_UNSUPMEDIATYPE   = 415,
    RC_RNGNOTSATISFIABLE= 416,
    RC_EXPECTATIONFAILED= 417,
    RC_MISDIRECTEDREQ   = 421,        ///< Misdirected Request
    RC_UNPROCESSABLE    = 422,        ///< Unprocessable Entity
    RC_LOCKED           = 423,        ///< Locked
    RC_FAILEDDEP        = 424,        ///< Failed Dependency
    RC_TOOEARLY         = 425,        ///< Too Early
    RC_UPGRADEREQUIRED  = 426,        ///< Upgrade Required
    RC_PRECONDREQUIRED  = 428,        ///< Precondition Required
    RC_TOOMANYREQS      = 429,        ///< Too Many Requests
    RC_REQFLDTOOLARGE   = 431,        ///< Request Header Fields Too Large
    RC_NG_NO_RESPONSE   = 444,        ///< No Response (Nginx-specific logged status code indicating that no response was sent back)
    RC_LEGALREASONS     = 451,        ///< Unavailable For Legal Reasons
    RC_SERVERERR        = 500,
    RC_NOTIMPLEMENTED   = 501,
    RC_BADGATEWAY       = 502,
    RC_UNAVAIL          = 503,
    RC_GATEWAYTIMEOUT   = 504,
    RC_BADHTTPVER       = 505,
    RC_VARIANTNEGOTIATES= 506,        ///< Variant Also Negotiates
    RC_INSUFFSTORAGE    = 507,        ///< Insufficient Storage
    RC_LOOPDETECTED     = 508,        ///< Loop Detected
    RC_NOTEXTENDED      = 510,        ///< Not Extended
    RC_NETAUTHREQUIRED  = 511,        ///< Network Authentication Required
};

bool is_http_redirect(std::underlying_type<rc_t>::type respcode);
bool is_http_error(std::underlying_type<rc_t>::type respcode);

#endif // UTIL_HTTP_H
