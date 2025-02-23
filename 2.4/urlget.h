#define CONF_DEFAULT 0
#define CONF_PROXY   (1<<0)
#define CONF_PORT    (1<<1)
#define CONF_HTTP    (1<<2)
#define CONF_GOPHER  (1<<3)
#define CONF_FTP     (1<<4)
#define CONF_VERBOSE (1<<5)
#define CONF_MUTE    (1<<6) /* never ever output anything to stderr/stdout */

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
  URG_WRITE_ERROR,
  
  URL_LAST
} UrgError;


/*
 * urlget <url>
 * (result put on stdout)
 *
 * <url> ::= <proto> "://" <host> [ ":" <port> ] "/" <path>
 *
 * <proto> = "HTTP" | "GOPHER" | "FTP"
 *
 * When FTP:
 *
 * <host> ::= [ <user> ":" <password> "@" ] <host>
 */

UrgError urlget(FILE *out,   /* the fetched file goes here */
                char *url,   /* what to get */
                unsigned short port, /* which port to use (if non-protocol bind)
                                        set CONF_PORT to use this */
                char *proxy, /* if proxy, set it here, set CONF_PROXY to use
                                this */
                long conf);   /* configure flags */
