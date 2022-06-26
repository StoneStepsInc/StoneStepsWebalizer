/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2022, Stone Steps Inc. (www.stonesteps.ca)
   Copyright (C) 1997-2001  Bradford L. Barrett (brad@mrunix.net)

   See COPYING and Copyright files for additional licensing and copyright information 
   
   util_http.h
*/
#ifndef UTIL_HTTP_H
#define UTIL_HTTP_H

//
// HTTP response codes
//
// http://www.iana.org/assignments/http-status-codes/
//
#define RC_CONTINUE           100
#define RC_SWITCHPROTO        101
#define RC_PROCESSING         102        ///< Processing
#define RC_EARLYHINTS         103        ///< Early Hints
#define RC_OK                 200
#define RC_CREATED            201
#define RC_ACCEPTED           202
#define RC_NONAUTHINFO        203
#define RC_NOCONTENT          204
#define RC_RESETCONTENT       205
#define RC_PARTIALCONTENT     206
#define RC_MULTISTATUS        207        ///< Multi-Status
#define RC_ALREADYREPORTED    208        ///< Already Reported
#define RC_IMUSED             226        ///< IM Used
#define RC_MULTIPLECHOICES    300
#define RC_MOVEDPERM          301
#define RC_MOVEDTEMP          302
#define RC_SEEOTHER           303
#define RC_NOMOD              304
#define RC_USEPROXY           305
#define RC_UNUSED             306        ///< (Unused)
#define RC_MOVEDTEMPORARILY   307
#define RC_PERMREDIRECT       308        ///< Permanent Redirect
#define RC_BAD                400
#define RC_UNAUTH             401
#define RC_PAYMENTREQ         402
#define RC_FORBIDDEN          403
#define RC_NOTFOUND           404
#define RC_METHODNOTALLOWED   405
#define RC_NOTACCEPTABLE      406
#define RC_PROXYAUTHREQ       407
#define RC_TIMEOUT            408
#define RC_CONFLICT           409
#define RC_GONE               410
#define RC_LENGTHREQ          411
#define RC_PREFAILED          412
#define RC_REQENTTOOLARGE     413
#define RC_REQURITOOLARGE     414
#define RC_UNSUPMEDIATYPE     415
#define RC_RNGNOTSATISFIABLE  416
#define RC_EXPECTATIONFAILED  417
#define RC_MISDIRECTEDREQ     421        ///< Misdirected Request
#define RC_UNPROCESSABLE      422        ///< Unprocessable Entity
#define RC_LOCKED             423        ///< Locked
#define RC_FAILEDDEP          424        ///< Failed Dependency
#define RC_TOOEARLY           425        ///< Too Early
#define RC_UPGRADEREQUIRED    426        ///< Upgrade Required
#define RC_PRECONDREQUIRED    428        ///< Precondition Required
#define RC_TOOMANYREQS        429        ///< Too Many Requests
#define RC_REQFLDTOOLARGE     431        ///< Request Header Fields Too Large
#define RC_LEGALREASONS       451        ///< Unavailable For Legal Reasons
#define RC_SERVERERR          500
#define RC_NOTIMPLEMENTED     501
#define RC_BADGATEWAY         502
#define RC_UNAVAIL            503
#define RC_GATEWAYTIMEOUT     504
#define RC_BADHTTPVER         505
#define RC_VARIANTNEGOTIATES  506        ///< Variant Also Negotiates
#define RC_INSUFFSTORAGE      507        ///< Insufficient Storage
#define RC_LOOPDETECTED       508        ///< Loop Detected
#define RC_NOTEXTENDED        510        ///< Not Extended
#define RC_NETAUTHREQUIRED    511        ///< Network Authentication Required

bool is_http_redirect(size_t respcode);
bool is_http_error(size_t respcode);

#endif // UTIL_HTTP_H
