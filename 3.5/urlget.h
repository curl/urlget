#ifndef __URLGET_H
#define __URLGET_H

/***********************************************************************
 *              _            _   
 *   _   _ _ __| | __ _  ___| |_ 
 *  | | | | '__| |/ _` |/ _ \ __|
 *  | |_| | |  | | (_| |  __/ |_ 
 *   \__,_|_|  |_|\__, |\___|\__| - Gets your URL!
 *                |___/          
 * NAME
 *   UrlGet
 *
 * DESCRIPTION
 *   Get a file using HTTP, GOPHER or FTP.
 *
 * COMPILATION
 *   Compile with: gcc urlget.c -o urlget
 *	          or: cc urlget.c -o urlget
 *               (add -lnsl -lsocket on Solaris)
 *   Win32: include 'wsock32.lib' in the list of libs to link with
 *
 * USAGE
 *   (invoke without argument to get help text)
 *
 *   Example: urlget -p 8080 -x my-proxy.my.site http://www.from.here/
 *
 * PROJECT
 *   Initial author and project maintainer (which started as HttpGet)
 *   Rafael Sagula <sagula@inf.ufrgs.br>
 *
 * HOMEPAGE
 *   http://www.inf.ufrgs.br/~sagula/urlget.html
 *
 *************************************************************************/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* mute is obsolete from 2.10 */
#define CONF_MUTE    0 /* never ever output anything to stderr/stdout */

#define CONF_DEFAULT 0
#define CONF_PROXY   (1<<0) /* set if proxy is in use */
#define CONF_PORT    (1<<1) /* set if different port than protcol-defines is
                               used */
#define CONF_HTTP    (1<<2) /* http get */
#define CONF_GOPHER  (1<<3) /* gopher get */
#define CONF_FTP     (1<<4) /* ftp get (binary mode) */
#define CONF_VERBOSE (1<<5) /* talk a lot */

/* this bit (1<<6) was previously used by the MUTE flag */

#define CONF_KEEPALIVE (1<<7) /* If set, the http GET command will use the
                                 'Keep-Alive' option */
#define CONF_HEADER  (1<<8) /* throw the header out too */
#define CONF_USERPWD (1<<9) /* user+passwd has been specified */
#define CONF_NOPROGRESS (1<<10) /* shut off the progress meter by force */
#define CONF_NOBODY  (1<<11) /* use HEAD to get http document */
#define CONF_FAILONERROR (1<<12) /* Makes urlget() fail with a return code
                                    WITHOUT writing anything to the output if
                                    a return code >=300 is returned from the
                                    server. */
#define CONF_RANGE (1<<13) /* Byte-range request, specified parameter is set */
#define CONF_UPLOAD (1<<14) /* this is an upload, only supported for ftp
                               currently */

#define CONF_POST (1<<15) /* HTTP POST method implementation - UrlGet 3.5 */

/* When getting an FTP directory, this switch makes the listing only show file
   names and nothing else. Makes machine parsing of the output possible. This
   enforces the NLST command to the ftp server, compared to the otherwise
   used: LIST. */
#define CONF_FTPLISTONLY (1<<15)

/* All possible error codes from this version of urlget(). Future versions
   may return other values, stay prepared. */

typedef enum {
  URG_OK = 0,
  URG_UNSUPPORTED_PROTOCOL,
  URG_FAILED_INIT,
  URG_URL_MALFORMAT,
  URG_URL_MALFORMAT_USER,
  URG_COULDNT_RESOLVE_PROXY,
  URG_COULDNT_RESOLVE_HOST,
  URG_COULDNT_CONNECT,
  URG_FTP_WEIRD_SERVER_REPLY,
  URG_FTP_ACCESS_DENIED,
  URG_FTP_USER_PASSWORD_INCORRECT,
  URG_FTP_WEIRD_PASS_REPLY,
  URG_FTP_WEIRD_USER_REPLY,
  URG_FTP_WEIRD_PASV_REPLY,
  URG_FTP_WEIRD_227_FORMAT,
  URG_FTP_CANT_GET_HOST,
  URG_FTP_CANT_RECONNECT,
  URG_FTP_COULDNT_SET_BINARY,
  URG_FTP_PARTIAL_FILE,
  URG_FTP_COULDNT_RETR_FILE,
  URG_FTP_WRITE_ERROR,
  URG_HTTP_NOT_FOUND,
  URG_WRITE_ERROR,

  URG_MALFORMAT_USER, /* the user name is illegally specified */
  URG_FTP_COULDNT_STOR_FILE, /* failed FTP upload */
  URG_READ_ERROR, /* could open/read from file */

  URG_OUT_OF_MEMORY,
  URG_OPERATION_TIMEOUTED, /* the timeout time was reached */
  URG_FTP_COULDNT_SET_ASCII, /* TYPE A failed */

  URL_LAST
} UrgError;

#define URGTAG_DONE -1
#define URGTAG_LAST -1
#define URGTAG_END -1

#define URLGET_ERROR_SIZE 256

typedef enum {
  URGTAG_NOTHING, /* the first unused */
  
  /* This is the FILE * the regular output should be written to. */
  URGTAG_FILE,

  /* The full URL to get/put */
  URGTAG_URL,

  /* Port number to connect to, if other than default. Specify the CONF_PORT
     flag in the URGTAG_FLAGS to activate this */
  URGTAG_PORT,

  /* Name of proxy to use. Specify the CONF_PROXY flag in the URGTAG_FLAGS to
     activate this */
  URGTAG_PROXY,
  
  /* Name and password to use when fetching. Specify the CONF_USERPWD flag in
     the URGTAG_FLAGS to activate this */
  URGTAG_USERPWD,

  /* Range to get, specified as an ASCII string. Specify the CONF_RANGE flag
     in the URGTAG_FLAGS to activate this */
  URGTAG_RANGE,

  /* Configuration flags */
  URGTAG_FLAGS,

  /* Specified file stream to upload from (use as input): */
  URGTAG_INFILE,

  /* Buffer to receive error messages in, must be at least URLGET_ERROR_SIZE
     bytes big. If this is not used, error messages go to stderr instead: */
  URGTAG_ERRORBUFFER,

  /* Function that will be called to store the output (instead of fwrite). The
     parameters will use fwrite() syntax, make sure to follow them. */
  URGTAG_WRITEFUNCTION, 

  /* Function that will be called to read the input (instead of fread). The
     parameters will use fread() syntax, make sure to follow them. */
  URGTAG_READFUNCTION, 

  /* Time-out the read operation after this amount of seconds */
  URGTAG_TIMEOUT,

  /* If the URGTAG_INFILE is used, this can be used to inform urlget about how
     large the file being sent really is. That allows better error checking
     and better verifies that the upload was succcessful. -1 means unknown
     size. */
  URGTAG_INFILESIZE,

  /* POST input fields. */
  URGTAG_POSTFIELDS,

  URGTAG_LASTENTRY /* the last unusued */
} UrgTag;

typedef char bool;

/**********************************************************************
 *
 * >>> urlget() interface (from 3.0) <<<
 *
 * You enter parameters as tags. Tags are specified as a pair of parameters.
 * The first parameter in a pair is the tag identifier, telling urlget what
 * kind of tag it is, and the second is the data. The tags may come in any
 * order but MUST ALWAYS BE TERMINATED with an ending URGTAG_DONE (which
 * needs no data).
 *
 * _Very_ simple example:
 *
 * urlget(URGTAG_URL, "www.fts.frontec.se/~dast", URGTAG_DONE);
 *
 ***********************************************************************/

UrgError urlget(UrgTag, ...);

/* This is the version number of *this* urlget */
#define URLGET_VERSION "3.5"

#endif /* __URLGET_H */
