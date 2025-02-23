/***********************************************************************
 *              _            _   
 *   _   _ _ __| | __ _  ___| |_ 
 *  | | | | '__| |/ _` |/ _ \ __|
 *  | |_| | |  | | (_| |  __/ |_ 
 *   \__,_|_|  |_|\__, |\___|\__| - Gets your URL for you!
 *                |___/          
 * NAME
 *   UrlGet - version 2.4
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
 * BIBLIOGRAPHY
 *   - "UNIX Network Programming" - Richard Stevens (rstevens@noao.edu)
 *   - "Advanced Programming in UNIX Environment" Richard Stevens (the same)
 *   - RFC 1738 T. Berners-Lee, L. Masinter, M. McCahill,
 *     "Uniform Resource Locators (URL)", 12/20/1994
 *   - RFC 2068 R. Fielding, J. Gettys, J. Mogul, H. Frystyk, T. Berners-Lee,
 *     "Hypertext Transfer Protocol -- HTTP/1.1", 01/03/1997.
 *   - RFC 959 J. Postel, J. Reynolds, "File Transfer Protocol", 10/01/1985.
 *
 * HISTORY
 *   Daniel Stenberg <Daniel.Stenberg@sth.frontec.se>
 *   - Adjusted it slightly to accept named hosts on the command line. We
 *     wouldn't wanna use IP numbers for the rest of our lifes, would we?
 *
 *   Bjorn Reese <breese@imada.ou.dk>
 *   - Implemented URLs (and skipped the old syntax).
 *   - Output is written to stdout, so to achieve the above example, do:
 *       httpget http://143.54.10.6/info_logo.gif > test.gif
 *
 *   Johan Andersson <johan@homemail.com>
 *   - Implemented HTTP proxy support.
 *   - Receive byte counter added.
 *
 *   Daniel Stenberg <Daniel.Stenberg@sth.frontec.se>
 *   - Bugfixed the proxy usage. It should *NOT* use nor strip the port number
 *     from the URL but simply pass that information to the proxy. This also
 *     made the user/password fields possible to use in proxy [ftp-] URLs.
 *     (like in ftp://user:password@ftp.my.site:8021/README)
 *
 *   Rafael Sagula <sagula@inf.ufrgs.br>
 *   - Let "-p" before "-x".
 *
 *   Johan Andersson
 *   - Discovered and fixed the problem with getting binary files. puts() is
 *     now replaced with fwrite(). (Daniel's note: this also fixed the buffer
 *     overwrite problem I found in the previous version.)
 *
 *   1.3 Daniel Stenberg
 *   - Well, I added a lame text about the time it took to get the data. I also
 *     fought against Johan to prevent his -f option (to specify a file name
 *     that should be written instead of stdout)! =)
 *   - Made it write 'connection refused' for that particular connect()
 *     problem.
 *   - Renumbered the version. Let's not make silly 1.0.X versions, this is
 *     a plain 1.3 instead.
 *
 *   1.4 Daniel Stenberg
 *   - The gopher source used the ppath variable instead of path which could
 *     lead to disaster.
 *
 *   1.5 Daniel Stenberg
 *   - The skip_header() crap messed it up big-time. By simply removing that
 *     one we can all of a sudden download anything ;)
 *   - No longer requires a trailing slash on the URLs.
 *   - If the given URL isn't prefixed with 'http://', HTTP is assumed and
 *     given a try!
 *   - 'void main()' is history.
 *
 *   2.0 Daniel Stenberg
 *   - Introducing ftp GET support. The FTP URL type is recognized and used.
 *   - Renamed the project to 'urlget'.
 *   - Supports the user+passwd in the FTP URL (otherwise it tries anonymous
 *     login with a weird email address as password).
 *
 *   2.1 Daniel Stenberg and Kjell Ericsson
 *   - Win32-compilable
 *   - No more global variables
 *   - Mute option (no output at all to stderr)
 *   - Full range of return codes from urlget(), which is now written to be a
 *     function for easy-to-use in [other] programs.
 *   - Define STAND_ALONE to compile the stand alone urlget program
 *   - Now compiles with gcc options -ansi -Wall -pedantic ;)
 *
 *   2.2
 *     Johan Andersson
 *   - The urlget function didn't set the path to url when using proxy.
 *   - Fixed bug with IMC proxy. Now using (almost) complete GET command.
 *
 *     Daniel Stenberg
 *   - Made it compile on Solaris. Had to reorganize the includes a bit.
 *     (so Win32, Linux, SunOS 4 and Solaris 2 compile fine.)
 *   - Made Johan's keepalive keyword optional with the -k flag (since it
 *     makes a lot of urlgets take a lot longer time).
 *   - Made a '-h' switch in case you want the HTTP-header in the output.
 *
 *   2.3
 *     Rafael Sagula
 *   - Added "-o" option (output file)
 *   - Added URG_HTTP_NOT_FOUND return code.
 *     (Daniel's note:)
 *     Perhaps we should detect all kinds of errors and instead of writing that
 *     custom string for the particular 404-error, use the error text we
 *     actually get from the server. See further details in RFC2068 (HTTP 1.1
 *     definition). The current way also relies on a HTTP/1.0 reply, which
 *     newer servers might not do.
 *   - Looping mode ("-l" option). It's easier to get various split files.
 *     (Daniel's note:)
 *     Use it like 'urlget -l 1 http://from.this.site/file%d.html', which will
 *     make urlget to attempt to fetch all files named file1.html, file2.html
 *     etc until no more files are found. This is only a modification of the
 *     STAND_ALONE part, nothing in the urlget() function was modfified for
 *     this.
 *
 *     Daniel Stenberg
 *   - Changed the -h to be -i instead. -h should be preserved to help use.
 *   - Bjorn Reese indicated that Borland _might_ use '_WIN32' instead of the
 *     VC++ WIN32 define and therefore I added a little fix for that.
 *
 *   2.4
 *     Daniel Stenberg
 *   - Added progress meter. It appears if downloading > BUFFER SIZE and
 *     mute is not selected. I found out that when downloading large files
 *     from really really slow sites, it is desirable to know the status of
 *     the download. Do note that some downloads are done unawaring of the
 *     size, which makes the progress meter less thrilling ;)
 *   - Increased buffer size used for reading.
 *   - Added length checks in the user+passwd parsing.
 *   - Made it grok user+passwd for HTTP fetches. The trick is to base64
 *     encode the user+passwd and send an extra header line. Read chapter 11.1
 *     in RFC2068 for details. I added it to be used just like the ftp one.
 *     To get a http document from a place that requires user and password,
 *     use an URL like:
 *           http://user:passwd@www.site.to.leach/doc.html
 *     I also added the -u flag, since WHEN USING A PROXY YOU CAN'T SPECIFY THE
 *     USER AND PASSWORD WITH HTTP LIKE THAT. The -u flag works for ftp too,
 *     but not if used with proxy. To do the same as the above one, you can
 *     invoke:
 *           urlget -u user:passwd http://www.site.to.leach/doc.html
 *   
 */

/* -- WIN32 approved -- */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#if !defined(WIN32) && defined(_WIN32)
/* This _might_ be a good Borland fix */
#define WIN32
#endif

#ifdef WIN32
#include <winsock.h>
#include <time.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

/* -- -- */

/* This define let's this source get compiled into a stand-alone urlget program
   rather than just offering the urlget() function */
//#define STAND_ALONE

/* This is the version number of *this* urlget */
#define VERSION "2.4"

/* Don't show progress for sizes smaller than: */
#define LEAST_SIZE_PROGRESS BUFSIZE

#define BUFSIZE (2048*5)

static int readline(int fd,char *ptr,int maxlen);
static int sendf(int fd, long conf, char *fmt, ...);
static void infof(long conf, char *fmt, ...);

#define CONF_DEFAULT 0
#define CONF_PROXY   (1<<0) /* set if proxy is in use */
#define CONF_PORT    (1<<1) /* set if different port than protcol-defines is
                               used */
/* Hm, the following three are for function internals only */ 
#define CONF_HTTP    (1<<2) /* http get */
#define CONF_GOPHER  (1<<3) /* gopher get */
#define CONF_FTP     (1<<4) /* ftp get (binary mode) */
/* end of internals only */

#define CONF_VERBOSE (1<<5) /* talk a lot */
#define CONF_MUTE    (1<<6) /* never ever output anything to stderr/stdout */
#define CONF_KEEPALIVE (1<<7) /* If set, the http GET command will use the
                                 'Keep-Alive' option */
#define CONF_HEADER  (1<<8) /* throw the header out too */
#define CONF_USERPWD (1<<9) /* user+passwd has been specified */

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
  
  URL_LAST
} UrgError;


/* ---- Base64 Encoding --- */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

static char table64[]=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
void base64Encode(char *intext, char *output)
{
  unsigned long twentyfourbits=0;
  unsigned char ibuf[3];
  unsigned char obuf[4];
  int i;
  int inputparts;

  while(*intext) {
    for (i = inputparts = 0; i < 3; i++) { 
      if(*intext) {
        inputparts++;
        ibuf[i] = *intext;
        intext++;
      }
      else
        ibuf[i] = 0;
    }
                       
    obuf [0] = (ibuf [0] & 0xFC) >> 2;
    obuf [1] = ((ibuf [0] & 0x03) << 4) | ((ibuf [1] & 0xF0) >> 4);
    obuf [2] = ((ibuf [1] & 0x0F) << 2) | ((ibuf [2] & 0xC0) >> 6);
    obuf [3] = ibuf [2] & 0x3F;

    switch(inputparts) {
    case 1: /* only one byte read */
      sprintf(output, "%c%c==", 
              table64[obuf[0]],
              table64[obuf[1]]);
      break;
    case 2: /* two bytes read */
      sprintf(output, "%c%c%c=", 
              table64[obuf[0]],
              table64[obuf[1]],
              table64[obuf[2]]);
      break;
    default:
      sprintf(output, "%c%c%c%c", 
              table64[obuf[0]],
              table64[obuf[1]],
              table64[obuf[2]],
              table64[obuf[3]] );
      break;
    }
    output += 4;
  }
  *output=0;
}
/* ---- End of Base64 Encoding ---- */

int progressmax=-1;

void ProgressInit(int conf, int max)
{
  if(conf&CONF_MUTE)
    return;
  progressmax = max;
  if(-1 == max)
    return;
  if(progressmax <= LEAST_SIZE_PROGRESS) {
    progressmax = -1; /* disable */
    return;
  }

  fprintf(stderr, "  %%   Received   Total  Speed   Time left  Total estimated time\n");
}

void ProgressShow(int conf, int point, time_t start, time_t now)
{
  static time_t lastshow;
  time_t spent;
  int speed;
  if(conf&CONF_MUTE)
    return;

  if((point != progressmax) && (lastshow == now))
    return; /* never update this more than once a second if the end isn't 
               reached */

  spent = now-start;
  speed = point/(spent?spent:1);

  if(-1 != progressmax) {
        
    time_t estimate = progressmax/speed;
    
    fprintf(stderr, "\r%3d %8d  %8d %6d %6d %8d",
            point*100/progressmax, point, progressmax,
            speed, estimate-spent, estimate);
  }
  else
    fprintf(stderr, "\r%d bytes received in %d seconds (%d bytes/sec)",
            point, spent, speed);

  lastshow = now;
}


/* Stolen from Dancer source code: */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
static struct hostent *GetHost(long conf, char *hostname)
{
  struct hostent *h = NULL;
  unsigned long in;

  if ( (in=inet_addr(hostname)) != INADDR_NONE ) {
    if ( (h=gethostbyaddr((char *)&in, sizeof(in), AF_INET)) == NULL )
      infof(conf, "gethostbyaddr(2) failed for %s\n", hostname);
  } else if ( (h=gethostbyname(hostname)) == NULL ) {
    infof(conf, "gethostbyname(2) failed for %s\n", hostname);
  }
  return (h);
}

static int GetLastResponse(int sockfd, char *buf, long conf)
{
  int nread;
  do {
    nread = readline(sockfd, buf, BUFSIZE);
    if(conf & CONF_VERBOSE)
      fwrite(buf, 1, nread, stderr);
  } while((nread>3) && ('-'==buf[3]));
  return nread;
}

#ifdef WIN32
/* remove all alarm() calls neatly */
#define alarm(x)

#define sread(x,y,z) recv(x,y,z,0)
#define swrite(x,y,z) send(x,y,z,0)

#define strcasecmp(x,y) stricmp(x,y)

static void cleanup(void)
{
  WSACleanup();
}

static int init(void)
{
  WORD wVersionRequested;  
  WSADATA wsaData; 
  int err; 
  wVersionRequested = MAKEWORD(1, 1); 
    
  err = WSAStartup(wVersionRequested, &wsaData); 
    
  if (err != 0) 
    /* Tell the user that we couldn't find a useable */ 
    /* winsock.dll.     */ 
    return 1; 
    
  /* Confirm that the Windows Sockets DLL supports 1.1.*/ 
  /* Note that if the DLL supports versions greater */ 
  /* than 1.1 in addition to 1.1, it will still return */ 
  /* 1.1 in wVersion since that is the version we */ 
  /* requested. */ 
    
  if ( LOBYTE( wsaData.wVersion ) != 1 || 
       HIBYTE( wsaData.wVersion ) != 1 ) { 
    /* Tell the user that we couldn't find a useable */ 

    /* winsock.dll. */ 
    WSACleanup(); 
    return 1; 
  }
  return 0;
}
/* The Windows Sockets DLL is acceptable. Proceed. */ 
#else
static int init(void) { return 0; }
static void cleanup(void) {}

/* this is for "-ansi -Wall -pedantic" to stop complaining! */
extern int strcasecmp(const char *s1, const char *s2);

#define closesocket(x) close(x)
#define sread(x,y,z) read(x,y,z)
#define swrite(x,y,z) write(x,y,z)

#endif


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
                unsigned short port, /* which port to use (if non-protocol
                                        bind) set CONF_PORT to use this */
                char *proxy, /* if proxy, set it here, set CONF_PROXY to use
                                this */
                long conf,   /* configure flags */
                char *userpwd) /* <user:password>, if used */
{
  struct hostent *hp=NULL;
  int sockfd;
  struct sockaddr_in serv_addr;
  char buf[BUFSIZE+1];
  char proto[64];
  char gname[256];
  char *name;
  char path[512]="/";
  char *ppath, *tmp;
  long bytecount;
  time_t now;
  char ftpuser[128]="";
  char ftppasswd[128]="";
  size_t nread;

  /* Don't even try to parse <url> when using proxy, just pass it to the proxy. */
  if (conf & CONF_PROXY) {
    strncpy(path, url, 511);
  }
  else {
    /* Parse <url> */
    if (2 > sscanf(url, "%64[^\n:]://%256[^\n/]%512[^\n]",
		   proto, gname, path)) {
      
      /* badly formatted, let's try the browser-style _without_ 'http://' */
	if((1 > sscanf(url, "%256[^\n/]%512[^\n]", gname, path)) ) {
	  infof(conf, "<url> malformed.\n");
	  return URG_URL_MALFORMAT;
	}
	strcpy(proto, "HTTP");
    }
  }

  if(conf & CONF_USERPWD) {
    if(':' != *userpwd) {
      if((1 <= sscanf(userpwd, "%127[^:]:%127s",
                      ftpuser, ftppasswd))) {
        ; /* just continue */
      }
    }
    if(!ftpuser[0]) {
      infof(conf, "USER malformat: user name can't be zero length\n");
      cleanup();
      return URG_MALFORMAT_USER;
    }
  }


  name = gname;
  ppath = path;

  /* this is for the lame win32 socket crap */
  if(init())
    return URG_FAILED_INIT;

  if(!(conf & CONF_PROXY)) {
    /*
     * Only do this when not using proxy, as when using proxy all that is up
     * to that one to deal with.
     */

    if (!strcasecmp(proto, "HTTP")) {
      if(!(conf & CONF_PORT))
        port = 80;
      conf |= CONF_HTTP;
    }
    else if (!strcasecmp(proto, "GOPHER")) {
      if(!(conf & CONF_PORT))
        port = 70;
      /* Skip /<item-type>/ in path if present */
      if (isdigit(path[1])) {
        ppath = strchr(&path[1], '/');
        if (ppath == NULL)
          ppath = path;
      }
      conf |= CONF_GOPHER;
    }
    else if(!strcasecmp(proto, "FTP")) {

      if(!(conf & CONF_PORT))
        port = 21;
      conf |= CONF_FTP;
      ppath++; /* don't include the slash */
    }
    else {
      infof(conf, "Unsupported protocol: %s\n", proto);
      cleanup();
      return URG_UNSUPPORTED_PROTOCOL;
    }

    if(!(conf & CONF_USERPWD) &&
       (conf & (CONF_FTP|CONF_HTTP)) ) {
      /* This is a FTP or HTTP URL, and we haven't got the user+password in
         the extra parameter, we will now try to extract the possible
         user+password pair in a string like:
         ftp://user:password@ftp.my.site:8021/README */
      char *ptr;
      if(':' == *name) {
        infof(conf, "URL malformat: user can't be zero length\n");
        cleanup();
        return URG_URL_MALFORMAT_USER;
      }
      if((1 <= sscanf(name, "%127[^:]:%127[^@]",
                      ftpuser, ftppasswd)) && (ptr=strchr(name, '@'))) {
        name = ++ptr;
        conf |= CONF_USERPWD;
      }
      else {
        strcpy(ftpuser, "anonymous");
        strcpy(ftppasswd, "urlget_by_Daniel.Stenberg@sth.frontec.se");
      }
    }
  }
  alarm(60*30); /* 30 minutes */

  if (conf & CONF_PROXY) {
    /* When using a proxy, we shouldn't extract the port number from the URL
     * since that would destroy it. */
    
    /* When using proxy and FTP, we can't get the user+passwd in the 'userpwd'
       parameter. Since we can't easily check for it, we won't. I have written
       this is the "history" at the top as well as I write it here. I know it
       will cause problems one day, but what the heck... */

    if(!(hp = GetHost(conf, proxy))) {
      infof(conf, "Couldn't resolv '%s', exiting.\n", proxy);
      cleanup();
      return URG_COULDNT_RESOLVE_PROXY;
    }
    if(!(conf & CONF_PORT))
      port = 1080; /* default proxy port */
  }
  else {
    tmp = strchr(name, ':');
    if (tmp) {
      *tmp++ = '\0';
      port = atoi(tmp);
    }

    if(!(hp = GetHost(conf, name))) {
      infof(conf, "Couldn't resolv '%s', exiting.\n", name);
      cleanup();
      return URG_COULDNT_RESOLVE_HOST;
    }
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  memset((char *) &serv_addr, '\0', sizeof(serv_addr));
  memcpy((char *)&(serv_addr.sin_addr), hp->h_addr, hp->h_length);
  serv_addr.sin_family = hp->h_addrtype;
  serv_addr.sin_port = htons(port);
  
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    switch(errno) {
#ifndef WIN32
      /* this should be made nicer */
    case ECONNREFUSED:
      infof(conf, "Connection refused.\n");
      break;
#endif
    default:
      infof(conf, "Can't connect to server.\n");
      break;
    }
    cleanup();
    return URG_COULDNT_CONNECT;
  }
  now = time(NULL); /* time this *after* the connect is done */
  bytecount = 0;
  
  if(conf & CONF_VERBOSE)
    infof(conf, "Connected!\n");

  if((conf&(CONF_FTP|CONF_PROXY)) == CONF_FTP) {
    /* this is FTP and no proxy, we don't do the usual crap then */

    /* The first thing we do is wait for the "220*" line: */
    nread = GetLastResponse(sockfd, buf, conf);
    if(strncmp(buf, "220", 3)) {
      infof(conf, "This doesn't seem like a nice ftp-server response!\n");
      cleanup();
      return URG_FTP_WEIRD_SERVER_REPLY;
    }
    /* for debugging */
    if(conf & CONF_VERBOSE)
      fwrite(buf, 1, nread, stderr);

    /* send USER */
    sendf(sockfd, conf, "USER %s\n", ftpuser);

    /* wait for feedback */
    nread = GetLastResponse(sockfd, buf, conf);

    if(!strncmp(buf, "530", 3)) {
      /* 530 User ... access denied
         (the server denies to log the specified user) */
      infof(conf, "Access denied\n");
      cleanup();
      return URG_FTP_ACCESS_DENIED;
    }
    else if(!strncmp(buf, "331", 3)) {
      /* 331 Password required for ...
         (the server requires to send the user's password too) */
      sendf(sockfd, conf, "PASS %s\n", ftppasswd);
      nread = GetLastResponse(sockfd, buf, conf);

      if(!strncmp(buf, "530", 3)) {
        /* 530 Login incorrect.
           (the username and/or the password are incorrect) */
        infof(conf, "the username and/or the password are incorrect\n");
        cleanup();
        return URG_FTP_USER_PASSWORD_INCORRECT;
      }
      else if(!strncmp(buf, "230", 3)) {
        /* 230 User ... logged in.
           (user successfully logged in) */
        if(conf & CONF_VERBOSE)
          infof(conf, "we're in\n");
      }
      else {
        infof(conf, "Odd return code after PASS, exiting\n");
        cleanup();
        return URG_FTP_WEIRD_PASS_REPLY;
      }
    }
    else if(! strncmp(buf, "230", 3)) {
      /* 230 User ... logged in.
	(the user logged in without password) */
      if(conf & CONF_VERBOSE)
        infof(conf, "we're in\n");
    }
    else {
      infof(conf, "Odd return code after USER, exiting\n");
      cleanup();
      return URG_FTP_WEIRD_USER_REPLY;
    }
    sendf(sockfd, conf, "PASV\n");

    nread = GetLastResponse(sockfd, buf, conf);

    if(strncmp(buf, "227", 3)) {
      infof(conf, "Odd return code after PASV, exiting\n");
      cleanup();
      return URG_FTP_WEIRD_PASV_REPLY;
    }
    else {
      int ip[4];
      int port[2];
      unsigned short newport;
      char newhost[32];
      struct hostent *he;
      int newsock;
      /* 227 Entering Passive Mode (127,0,0,1,4,51) */
      if(6 != sscanf(buf, "%*[^(](%d,%d,%d,%d,%d,%d)",
                     &ip[0], &ip[1], &ip[2], &ip[3],
                     &port[0], &port[1])) {
        infof(conf, "Oddly formatted 227-reply, exiting\n");
        cleanup();
        return URG_FTP_WEIRD_227_FORMAT;
      }
      sprintf(newhost, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      he = GetHost(conf, newhost);
      if(!he) {
        infof(conf, "Can't get new host\n");
        cleanup();
        return URG_FTP_CANT_GET_HOST;
      }

      newport = port[0]*256 + port[1];
      newsock = socket(AF_INET, SOCK_STREAM, 0);

      memset((char *) &serv_addr, '\0', sizeof(serv_addr));
      memcpy((char *)&(serv_addr.sin_addr), he->h_addr, he->h_length);
      serv_addr.sin_family = he->h_addrtype;
      serv_addr.sin_port = htons(newport);
  
      if (connect(newsock, (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) {
        switch(errno) {
#ifndef WIN32
          /* this should be made nicer */
        case ECONNREFUSED:
          infof(conf, "Connection refused.\n");
          break;
#endif
        default:
          infof(conf, "Can't connect to server.\n");
          break;
        }
        cleanup();
        return URG_FTP_CANT_RECONNECT;
      }
      /* we have the data connection ready */

      /* Set type to binary */
      sendf(sockfd, conf, "TYPE I\n");

      nread = GetLastResponse(sockfd, buf, conf);

      if(strncmp(buf, "200", 3)) {
        infof(conf, "Couldn't set binary mode\n");
        cleanup();
        return URG_FTP_COULDNT_SET_BINARY;
      }

      /* Retrieve file */
      sendf(sockfd, conf, "RETR %s\n", ppath);

      nread = GetLastResponse(sockfd, buf, conf);

      if(!strncmp(buf, "150", 3)) {
        /* 150 Opening BINARY mode data connection for /etc/passwd (2241 bytes).
           (ok, the file is being transfered) */
        int size;
        if(!sscanf(buf, "%*[^(](%d", &size)) {
          size=-1;
        }
        ProgressInit(conf, size);
        if(conf & CONF_VERBOSE)
          infof(conf, "Getting file with size: %d\n", size);

        bytecount=0;
        for (;;) {
          nread = sread(newsock, buf, BUFSIZE);
          bytecount += nread;
          ProgressShow(conf, bytecount, now, time(NULL));
          if (nread==0) {
            closesocket(newsock);
            closesocket(sockfd);
            break;
          }
          alarm(60*30); /* 30 minutes */
          if(nread != fwrite(buf, 1, nread, out)) {
            infof(conf, "Failed writing output!\n");
            cleanup();
            return URG_FTP_WRITE_ERROR;
          }
        }
        if((-1 != size) && (size != bytecount)) {
          infof(conf, "Received only partial file!\n");
          cleanup();
          return URG_FTP_PARTIAL_FILE;
        }
      }
      else {
        infof(conf, "Couldn't RETR file %s\n", ppath);
        cleanup();
        return URG_FTP_COULDNT_RETR_FILE;
      }
    }
    
  }
  else {
    /* Send the GET line to the HTTP server */
    int size=-1;
    char authorization[512];
    char userpwd[512];
    if(conf & CONF_USERPWD) {
      sprintf(userpwd, "%s:%s", ftpuser, ftppasswd);
      base64Encode(userpwd, authorization);
      sprintf(userpwd,
              "Authorization: Basic %s\015\012",
              authorization);
    }
    sendf(sockfd, conf,
          "GET %s HTTP/1.0\015\012"
          "%s"
          "%s"
          "User-Agent: urlget/" VERSION "\015\012"
          "Pragma: no-cache\015\012"
          "Accept: image/gif, image/x-xbitmap, image/jpeg,image/pjpeg, */*\015\012"
          "\015\012",
          ppath,
          (conf&CONF_USERPWD)?userpwd:"",
          (conf&CONF_KEEPALIVE)?"Connection: Keep-Alive\015\012":""
          );

    {
      char ch;
      int nr = 3;
      int hdrline = 0;
      while ((nr>2)) {
        hdrline++;
        nr=readline(sockfd, buf, BUFSIZE-2);
        if(conf & CONF_HEADER)
          fprintf(out, "%s\n", buf);
        /* 404 -> URL not found! */
        if (!strncmp(buf, "HTTP/1.0 404",12)) {
          infof(conf,
                "The requested URL %s was not found on this server.\n", url);
          cleanup();
          return URG_HTTP_NOT_FOUND;
        }
        else
          sscanf(buf, "Content-%cength: %d", &ch, &size);
      }
    }
    ProgressInit(conf, size);
    for (;;) {
      nread = sread(sockfd, buf, BUFSIZE);
      bytecount += nread;
      ProgressShow(conf, bytecount, now, time(NULL));
      if (nread==0) {
        closesocket(sockfd);
        break;
      }
      alarm(60*30); /* 30 minutes */
      if(nread != fwrite(buf, 1, nread, out)) {
        infof(conf, "Failed writing output!\n");
        cleanup();
        return URG_WRITE_ERROR;
      }
    }
  }
  {
    time_t end=time(NULL);
    infof(conf, "\n*DONE* %i bytes received in %d seconds (%d bytes/sec).\n",
          bytecount, end-now, bytecount/(end-now?end-now:1));
  }
  cleanup();
  return URG_OK;
}

static int readline(int fd, char *ptr, int maxlen)
{
  int n, rc;
  char c;
        
  for (n = 1; n < maxlen; n++) {
    rc = sread(fd, &c, 1);
    if ( 1 == rc ) {
      if (c == '\n')
        break;
      *ptr++ = c;
    } 
    else 
      if (rc == 0) {
        if (n == 1)
          return 0;
        else
          break;
      } 
      else
        return -1;
  }
  *ptr = 0;
  return n;
}

static void infof(long conf, char *fmt, ...)
{
  va_list ap;
  if(conf & CONF_MUTE)
    return;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

static int sendf(int fd, long conf, char *fmt, ...)
{
  char s[512];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(s, fmt, ap);
  va_end(ap);
  if(conf & CONF_VERBOSE)
    fprintf(stderr, "SENDS: %s", s);
  return(swrite(fd, s, strlen(s)));
}

#ifdef STAND_ALONE
static void help(void)
{
  fprintf(stderr,
          "urlget v" VERSION " (1996 - 1997)\n"
          "- Sagula/Stenberg/Reese/Andersson/Ericson\n"
          "usage: urlget [options...] <url>\n"
          "options:\n"
          "  -i          Include the HTTP-header in the output\n"
          "  -k          Use Keep-Alive connection\n"
          "  -l n        Initial looping value.\n"
          "  -o <file>   Write output to <file> instead of stdout\n"
          "  -p <port>   Use port other than default for current protocol.\n"
          "  -s          Silent mode. Don't output anything to stderr\n"
          "  -u <user:password> Specify user and password to use when fetching\n"
          "  -v          Makes the fetching more talkative\n"
          "  -x <host>   Use proxy. (Default port is 1080)\n"
          " NOTE:\n"
          " To ftp files using name+passwd, include them in the URL like:\n"
          "   ftp://name:passwd@this.machine.domain:port/full/path/to/file\n"

          );
  exit(-1);
}

int main(argc,argv)
    int argc;
    char *argv[];
{
  long conf=CONF_DEFAULT;
  unsigned short porttouse=0;

  char *proxy = NULL;

  char *url_orig = NULL;
  char *outfile_orig = NULL;
  char url[512];
  char outfile[512];
  char *userpwd=NULL;

  FILE *outfd = stdout;

  int linit = -1;
  int res;
  int i;

  if (argc < 2)
    help();

  /* Parse options */
  for (i = 1; i < argc-1; i++) {
    if (!strcmp(argv[i], "-p")) {
      porttouse = atoi(argv[++i]);
      conf |= CONF_PORT; /* changed port */
    }
    else if (!strcmp(argv[i], "-x")) {
      proxy = argv[++i];
      conf |= CONF_PROXY;
      /* Sagula:
       * If port defined, don't change it. */
      if (!(conf & (CONF_PORT))) {
        porttouse = 1080;
        conf |= CONF_PORT;
      }
    }
    else if(!strcmp(argv[i], "-v")) {
      conf |= CONF_VERBOSE; /* talk a lot */
    }
    else if(!strcmp(argv[i], "-s")) {
      conf |= CONF_MUTE; /* don't talk at all*/
    }
    else if(!strcmp(argv[i], "-i")) {
      conf |= CONF_HEADER; /* include the HTTP header aswell */
    }
    else if(!strcmp(argv[i], "-k")) {
      /* tell the server to keep the connection alive */
      conf |= CONF_KEEPALIVE;
    }
    else if(!strcmp(argv[i], "-o")) {
      /* output file */
      outfile_orig = argv[++i];
    }
    else if(!strcmp(argv[i], "-l")) {
      /* loop init  */
      linit = atoi(argv[++i]);
    }
    else if(!strcmp(argv[i], "-u")) {
      /* user:password  */
      userpwd = argv[++i];
      conf |= CONF_USERPWD;
    }
    else
      /* unknown flag, show usage blurb */
      help();
  }
  /* we consider the last argument being the URL */
  url_orig = argv[argc - 1];
 
  if (linit>-1) { 
    res=URG_OK;

    i=linit;
    while(res==URG_OK) {
 
      sprintf(url,url_orig,i);

      if (outfile_orig!=NULL) {
        sprintf(outfile,outfile_orig,i);
        if ((outfd=(FILE *) fopen(outfile,"w"))==NULL) {
          return URG_WRITE_ERROR;}
      }
      res = urlget(outfd, url, porttouse, proxy, conf, userpwd);

      if (outfile_orig!=NULL) {
        fclose(outfd);
      }
      i++;
    }
  } else {
      if (outfile_orig!=NULL) {
        if ((outfd=(FILE *) fopen(outfile_orig,"w"))==NULL) {
          return URG_WRITE_ERROR;}
      }
      res = urlget(outfd, url_orig, porttouse, proxy, conf, userpwd);
    
      if (outfile_orig!=NULL) {
        fclose(outfd);}
  }
  return(res);
}
#endif /* STAND_ALONE */
