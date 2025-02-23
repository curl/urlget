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
 *   Compile with: make
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
 *    <All history is moved to CHANGES>
 *
 *  TODO:
 *   - (HTTP) Add support for PUT, TRACE, DELETE and conditional GETs.
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

/* you might need to edit the config.h */
#include "config.h"

#include "urlget.h"

#ifdef WIN32
#include <winsock.h>
#include <time.h>
#include <io.h>
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

/* Don't show progress for sizes smaller than: */
#define LEAST_SIZE_PROGRESS BUFSIZE

#define BUFSIZE (2048*5)

/* We don't support header lengths larger than this, in cases where we need to
   append parts to build full lines (i.e longer lines may be fine, but they
   may also not be fine). */
#define HEADERSIZE 256

/***********************************************************************
 *        config struct
 **********************************************************************/

struct UrlData {
  FILE *out;   /* the fetched file goes here */
  FILE *in;    /* the uploaded file is read from here */
  char *url;   /* what to get */
  unsigned short port; /* which port to use (if non-protocol bind) set
                          CONF_PORT to use this */
  char *proxy; /* if proxy, set it here, set CONF_PROXY to use this */
  long conf;   /* configure flags */
  char *userpwd;  /* <user:password>, if used */
  char *proxyuserpwd;  /* Proxy <user:password>, if used */
  char *range; /* range, if used. See README for detailed specification on
                  this syntax. */
  char *postfields; /* if POST, set the fields' values here */
  char *referer;
  char *errorbuffer; /* store failure messages in here */

 /* function that stores the output:*/
  size_t (*fwrite)(char *buffer,
                   int size,
                   int nitems,
                   FILE *outstream);

  /* function that reads the input:*/
  size_t (*fread)(char *buffer,
                  int size,
                  int nitems,
                  FILE *outstream);

  long timeout; /* in seconds, 0 means no timeout */
  long infilesize; /* size of file to upload, -1 means unknown */

  /* fields only set and used within _urlget() */
  int firstsocket;     /* the main socket to use */
  int secondarysocket; /* for i.e ftp transfers */

  char buffer[BUFSIZE+1]; /* buffer with size BUFSIZE */
};

static int sendf(int fd, struct UrlData *, char *fmt, ...);
static void infof(struct UrlData *, char *fmt, ...);
static void failf(struct UrlData *, char *fmt, ...);


/***********************************************************************
 * Start with some silly functions to make win32-systems survive
 ***********************************************************************/
#ifdef WIN32
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
#endif

static UrgError _urlget(struct UrlData *data);

void urlfree(struct UrlData *data)
{
  /* close possibly still open sockets */
  if(-1 != data->secondarysocket)
    sclose(data->secondarysocket);
  if(-1 != data->firstsocket)
    sclose(data->firstsocket);

  free(data);

  /* winsock crap cleanup */
  cleanup();
}

UrgError urlget(UrgTag tag, ...)
{
  va_list arg;
  void *param;
  UrgError res;

  struct UrlData *data;

  /* this is for the lame win32 socket crap */
  if(init())
    return URG_FAILED_INIT;

  data = malloc(sizeof(struct UrlData));
  if(data) {

    memset(data, 0, sizeof(struct UrlData));

    /* Let's set some default values: */
    data->out = stdout; /* default output to stdout */
    data->in  = stdin;  /* default input from stdin */
    data->firstsocket = -1; /* no file descriptor */
    data->secondarysocket = -1; /* no file descriptor */

    /* use fwrite as default function to store output */
    data->fwrite = (size_t (*)(char *, int, int, FILE *))fwrite;

    /* use fread as default function to read input */
    data->fread = (size_t (*)(char *, int, int, FILE *))fread;

    data->infilesize = -1; /* we don't know any size */

    va_start(arg, tag);

    while(tag != URGTAG_DONE) {
      /*      printf("tag: %d\n", tag); */
      param = va_arg(arg, void *);
      
      switch(tag) {
      case URGTAG_ERRORBUFFER:
        data->errorbuffer = (char *)param;
        break;
      case URGTAG_FILE:
        data->out = (FILE *)param;
        break;
      case URGTAG_INFILE:
        data->in = (FILE *)param;
        break;
      case URGTAG_INFILESIZE:
        data->infilesize = (long)param;
        break;
      case URGTAG_URL:
        data->url = (char *)param;
        break;
      case URGTAG_PORT:
        /* this typecast is used to fool the compiler to NOT warn for a
           "cast from pointer to integer of different size" */
        data->port = (unsigned short)((unsigned int)param);
        break;
      case URGTAG_POSTFIELDS:
        data->postfields = (char *)param;
        break;
      case URGTAG_REFERER:
        data->referer = (char *)param;
        break;
      case URGTAG_PROXY:
        data->proxy = (char *)param;
        break;
      case URGTAG_FLAGS:
        data->conf = (long)param;
        break;
      case URGTAG_TIMEOUT:
        data->timeout = (long)param;
        break;
      case URGTAG_USERPWD:
        data->userpwd = (char *)param;
        break;
      case URGTAG_PROXYUSERPWD:
        data->proxyuserpwd = (char *)param;
        break;
      case URGTAG_RANGE:
        data->range = (char *)param;
        break;
      case URGTAG_WRITEFUNCTION:
        data->fwrite = (size_t (*)(char *, int, int, FILE *))param;
        break;
      case URGTAG_READFUNCTION:
        data->fread = (size_t (*)(char *, int, int, FILE *))param;
        break;
      case URGTAG_DONE: /* done with the parsing, fall through */
        continue;
      default:
        /* unknown tag and its companion, just ignore: */
        break;
      }
      tag = va_arg(arg, UrgTag);
    }

    va_end(arg);

    res = _urlget(data); /* fetch the URL please */

  }
  else
    res = URG_FAILED_INIT; /* failed */

  /* cleanup */
  urlfree(data);

  return res;
}


/* ---- Base64 Encoding --- */
static char table64[]=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
void base64Encode(char *intext, char *output)
{
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


/* --- start of progress routines --- */
int progressmax=-1;

void ProgressInit(struct UrlData *data, int max)
{
  if(data->conf&CONF_NOPROGRESS)
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

void time2str(char *r, int t)
{
  int h = (t/3600);
  int m = (t-(h*3600))/60;
  int s = (t-(h*3600)-(m*60));
  sprintf(r,"%3d:%02d:%02d",h,m,s);
}

void ProgressShow(struct UrlData *data,
                  int point, int start, int now)
{
  static time_t lastshow;
  int spent;
  int speed;
  if(data->conf&CONF_NOPROGRESS)
    return;

  if((point != progressmax) && (lastshow == now))
    return; /* never update this more than once a second if the end isn't 
               reached */

  spent = now-start;
  speed = point/(spent?spent:1);
  if(!speed)
    speed=1;

  if(-1 != progressmax) {
    char left[20],estim[20];
    int estimate = progressmax/speed;
    
    time2str(left,estimate-spent); 
    time2str(estim,estimate);

    fprintf(stderr, "\r%3d %8d  %8d %6d %s %s",
            point*100/progressmax, point, progressmax,
            speed, left, estim);
  }
  else
    fprintf(stderr, "\r%d bytes received in %d seconds (%d bytes/sec)",
            point, spent, speed);

  lastshow = now;
}

void ProgressEnd(struct UrlData *data)
{
  if(data->conf&CONF_NOPROGRESS)
    return;
  fputs("\n", stderr);
}

/* --- end of progress routines --- */


/* --- resolve name or IP-number --- */

/* Originally stolen from Dancer source code: */
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
static struct hostent *GetHost(struct UrlData *data, char *hostname)
{
  struct hostent *h = NULL;
  unsigned long in;

  if ( (in=inet_addr(hostname)) != INADDR_NONE ) {
    if ( (h=gethostbyaddr((char *)&in, sizeof(in), AF_INET)) == NULL )
      infof(data, "gethostbyaddr(2) failed for %s\n", hostname);
  } else if ( (h=gethostbyname(hostname)) == NULL ) {
    infof(data, "gethostbyname(2) failed for %s\n", hostname);
  }
  return (h);
}

/* --- parse FTP server responses --- */

static int GetLastResponse(int sockfd, char *buf, struct UrlData *data)
{
  int nread;
  char *ptr;
  do {
    ptr=buf;

    /* get us a full line, terminated with a newline */
    for(nread=0;
        nread<BUFSIZE && (sread(sockfd, ptr, 1)==1);
        nread++, ptr++) {
      if (*ptr == '\n')
        break;
    }
    *ptr=0; /* zero terminate */

    if(data->conf & CONF_VERBOSE) {
      fputs("< ", stderr);
      fwrite(buf, 1, nread, stderr);
      fputs("\n", stderr);
    }
  } while((nread>3) && ('-'==buf[3]));
  return nread;
}

/* --- upload a stream to a socket --- */

static UrgError Upload(struct UrlData *data,
                       int sockfd,
                       int size,
                       long *bytecountp)
{
  fd_set writefd;
  fd_set keepfd;
  struct timeval interval;
  bool keepon=TRUE;
  char *buf = data->buffer;
  size_t nread;
  long bytecount=0;
  time_t start = time(NULL);
  time_t now;
      
  /* timeout every X second
     - makes a better progressmeter (i.e even when no data is sent, the
       meter can be updated and reflect reality)
     - allows removal of the alarm() crap
     - variable timeout is easier
     */
            
  FD_ZERO(&writefd); /* clear it */
  FD_SET(sockfd, &writefd);
      
  keepfd = writefd;

  while(keepon) {
    writefd = keepfd; /* set this every lap in the loop */
    interval.tv_sec = 2;
    interval.tv_usec = 0;

    switch(select(sockfd+1, NULL, &writefd, NULL, &interval)) {
    case -1: /* error, stop writing */
      keepon=FALSE;
      continue;
    case 0: /* timeout */
      break;
    default: /* write! */
      nread = data->fread(buf, 1, BUFSIZE, data->in);
      bytecount += nread;

      if (nread==0) {
        /* done */
        keepon = FALSE; 
        break;
      }

      /* write to socket */
      if(nread != swrite(sockfd, buf, nread)) {
        failf(data, "Failed uploading file");
        return URG_FTP_WRITE_ERROR;
      }
    }
    now = time(NULL);
    ProgressShow(data, bytecount, start, now);
    if(data->timeout && ((now-start)>data->timeout)) {
      failf(data, "Upload timed out with %d bytes sent", bytecount);
      return URG_OPERATION_TIMEOUTED;
    }

  }
  *bytecountp = bytecount;

  return URG_OK;
}

/* --- download a stream from a socket --- */

/* This features optional (HTTP) header parsing to
   - show the header
   - find the document size
   */

static
UrgError Download(struct UrlData *data,
                  int sockfd, /* socket to read from */
                  int size, /* -1 if unknown at this point */
                  bool getheader, /* TRUE if header parsing is wanted */
                  long *bytecountp /* return number of bytes read */
                  )
{
  char *buf = data->buffer;
  size_t nread;
  int bytecount=0;
  time_t start=time(NULL);
  time_t now;
  bool header=TRUE;

  char headerbuff[HEADERSIZE];
  char *hbufp=headerbuff;
  int hbuflen=0;
  char *str;

  if(!getheader) {
    header=FALSE;
    ProgressInit(data, size);
  }
  {
    fd_set readfd;
    fd_set keepfd;
    struct timeval interval;
    bool keepon=TRUE;
      
    /* timeout every X second
       - makes a better progressmeter (i.e even when no data is read, the
         meter can be updated and reflect reality)
       - allows removal of the alarm() crap
       - variable timeout is easier
       */
            
    FD_ZERO(&readfd); /* clear it */
    FD_SET(sockfd, &readfd);
      
    keepfd = readfd;

    while(keepon) {
      readfd = keepfd; /* set this every lap in the loop */
      interval.tv_sec = 2;
      interval.tv_usec = 0;

      switch(select(sockfd+1, &readfd, NULL, NULL, &interval)) {
      case -1: /* error, stop reading */
        keepon=FALSE;
        continue;
      case 0: /* timeout */
        break;
      default: /* read! */
        nread = sread(sockfd, buf, BUFSIZE);

        /* if we receive 0 here, the server closed the connection and we
           bail out from this! */
        if (nread==0) {
          keepon=FALSE;
          break;
        }

        str = buf; /* Default buffer to use when we write the buffer, it may
                      be changed in the flow below before the actual storing
                      is done. */

        /* Since this is a two-state thing, we check if we are parsing
           headers at the moment or not. */

        if(header) {
          /* we are in parse-the-header-mode */
          int code;

          /* Search for end of line */
          /* str = strchr(buf, '\n'); */

          if(!str) {
            /* We didn't find end of line, make next read buffer get appended
               to the end of this buffer and continue. */
            strcpy(hbufp, buf);
            hbufp += strlen(buf);
            hbuflen += strlen(buf);
            break;
          }
          else {
            do {
              /* Each \r should be followed by a \n, but we do it nicely
                 and check for it anyway */
              if('\n' == *str)
                str++;

              /* in case we got a buffer to append */
              if(hbufp != headerbuff) {
                /* check that we can append this without overflow */
                if(hbuflen + strlen(str) > HEADERSIZE)
                  return URG_READ_ERROR;

                /* reset the variables */
                strcpy(hbufp, str); /* copy the end of the line we want */
                hbufp = headerbuff; /* reset */
                hbuflen=0;
              }
              else
                hbufp=str; /* point out our line to scan */

              /* if we want written headers, write the headers: */
              if(data->conf & CONF_HEADER) {
		char aux[127];
		sscanf(hbufp, "%127[^\n]", aux);
                fprintf(data->out, "%s\n", aux);
                bytecount += strlen(hbufp)+2; /* count the \r\n too */
              }

              /* we now have a full line that hbufp points to */
              if (('\n' == *hbufp)||('\r' == *hbufp)) {
                /* Zero-length line means end of header! */
                if(-1 != size) /* if known */
                  size += bytecount; /* we append the already read size */

                str++; /* pass the \n byte */

                /* Each \r should be followed by a \n, but we do it nicely
                   and check for it anyway */
                if('\n' == *str)
                  str++;

                ProgressInit(data, size); /* init progress meter */
                header=FALSE; /* no more header to parse! */
                break;
              } else if(data->conf & CONF_FAILONERROR) {
                /* If we have been told to fail hard on HTTP-errors,
                   here is the check for that: */
                if (sscanf(hbufp, "HTTP/1.%*c %3d", &code)) {
                  /* 404 -> URL not found! */
                  if(code >= 300) {
                    /* serious error, go home! */
                    failf(data, "The requested file was not found");
                    return URG_HTTP_NOT_FOUND;
                  }
                }
              }
              /* check for Content-Length: header lines to get size */
              sscanf(hbufp, "Content-%*cength: %d", &size);

              /* find the next line in our buffer: */
              str=strchr(str, '\n');

              /* reset hbufp pointer */
              hbufp = headerbuff;
            } while (str);

            /* We might have reached the end of the header part here, but
               there might be a non-header part left in the end of the read
               buffer. */

            if(!header) { /* the next token and forward is not part of
                             the header! */

              /* we subtract the header size from the buffer */
              nread -= (str-buf);
            }
          }
        }

        /* This is not an 'else if' since it may be a rest from the header
           parsing, where the beginning of the buffer is headers and the end
           is non-headers. */
        if(!header && (nread>0)) {
          bytecount += nread;
          data->fwrite(str, 1, nread, data->out);
#ifdef CHECK_THIS_OUT
	  if(nread != data->fwrite(str, 1, nread, data->out)) {
            failf(data, "Failed writing output");
            return URG_WRITE_ERROR;
          }
#endif
        }
        break;
      }
      now = time(NULL);
      if(!header)
        ProgressShow(data, bytecount, start, now);
      if(data->timeout && ((now-start)>data->timeout)) {
        failf(data, "Operation timed out with %d out of %d bytes received",
              bytecount, size);
        return URG_OPERATION_TIMEOUTED;
      }
    }
  }
  *bytecountp = bytecount;
  return URG_OK;
}


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

static UrgError _urlget(struct UrlData *data)
{
  struct hostent *hp=NULL;
  struct sockaddr_in serv_addr;
  char *buf;
  char proto[64];
  char gname[256]="default.com";
  char *name;
  char path[512]="/";
  char *ppath, *tmp;
  long bytecount;
  time_t now;
  char ftpuser[128]="";
  char ftppasswd[128]="";
  char proxyuser[128]="";
  char proxypasswd[128]="";
  size_t nread;
  UrgError result;

  /* Temporary kludgey fix until I replace properly in the source: */
  char *proxy = data->proxy; /* if proxy, set it here, set CONF_PROXY to use
                                this */
  long conf = data->conf;   /* configure flags */

  buf = data->buffer; /* this is our buffer */

  /* Don't even try to parse <url> when using proxy, just pass it to the
     proxy. */
  if (conf & CONF_PROXY) {
    strncpy(path, data->url, 511);
  }
  else {
    /* Parse <url> */
    if (2 > sscanf(data->url, "%64[^\n:]://%256[^\n/]%512[^\n]",
		   proto, gname, path)) {
      
      /* badly formatted, let's try the browser-style _without_ 'http://' */
	if((1 > sscanf(data->url, "%256[^\n/]%512[^\n]", gname, path)) ) {
	  failf(data, "<url> malformed");
	  return URG_URL_MALFORMAT;
	}
	if(strnequal(gname, "FTP", 3))
	  strcpy(proto, "FTP");
	else if(strnequal(gname, "GOPHER", 6))
	  strcpy(proto, "GOPHER");
	else
	  strcpy(proto, "HTTP");
    }
  }

  if(conf & CONF_USERPWD) {
    if(':' != *data->userpwd) {
      if((1 <= sscanf(data->userpwd, "%127[^:]:%127s",
                      ftpuser, ftppasswd))) {
        ; /* just continue */
      }
    }
    if(!ftpuser[0]) {
      failf(data, "USER malformat: user name can't be zero length");
      return URG_MALFORMAT_USER;
    }
  }

  if(conf & CONF_PROXYUSERPWD) {
    if(':' != *data->proxyuserpwd) {
      if((1 <= sscanf(data->proxyuserpwd, "%127[^:]:%127s",
                      proxyuser, proxypasswd))) {
        ; /* just continue */
      }
    }
    if(!proxyuser[0]) {
      failf(data, " Proxy USER malformat: user name can't be zero length");
      return URG_MALFORMAT_USER;
    }
  }

  name = gname;
  ppath = path;

  if(!(conf & CONF_PROXY)) {
    /*
     * Only do this when not using proxy, as when using proxy all that is up
     * to that one to deal with.
     */

    if (strequal(proto, "HTTP")) {
      if(!(conf & CONF_PORT))
        data->port = 80;
      conf |= CONF_HTTP;
    }
    else if (strequal(proto, "GOPHER")) {
      if(!(conf & CONF_PORT))
        data->port = 70;
      /* Skip /<item-type>/ in path if present */
      if (isdigit(path[1])) {
        ppath = strchr(&path[1], '/');
        if (ppath == NULL)
          ppath = path;
      }
      conf |= CONF_GOPHER;
    }
    else if(strequal(proto, "FTP")) {

      if(!(conf & CONF_PORT))
        data->port = 21;
      conf |= CONF_FTP;
#if 1
      /* 3.2-fix, why did we remove this? */
      ppath++; /* don't include the initial slash */
#endif
    }
    else {
      failf(data, "Unsupported protocol: %s", proto);
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
        failf(data, "URL malformat: user can't be zero length");
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

  if (conf & CONF_PROXY) {
    /* When using a proxy, we shouldn't extract the port number from the URL
     * since that would destroy it. */
    
    /* When using proxy and FTP, we can't get the user+passwd in the 'userpwd'
       parameter. Since we can't easily check for it, we won't. I have written
       this in the README as well as I write it here. I know it will cause
       problems one day, but what the heck... */

    if(!(hp = GetHost(data, proxy))) {
      failf(data, "Couldn't resolv '%s'", proxy);
      return URG_COULDNT_RESOLVE_PROXY;
    }
    if(!(conf & CONF_PORT))
      data->port = 1080; /* default proxy port */
  }
  else {
    tmp = strchr(name, ':');
    if (tmp) {
      *tmp++ = '\0';
      data->port = atoi(tmp);
    }

    if(!(hp = GetHost(data, name))) {
      failf(data, "Couldn't resolv '%s'", name);
      return URG_COULDNT_RESOLVE_HOST;
    }
  }

  data->firstsocket = socket(AF_INET, SOCK_STREAM, 0);

  memset((char *) &serv_addr, '\0', sizeof(serv_addr));
  memcpy((char *)&(serv_addr.sin_addr), hp->h_addr, hp->h_length);
  serv_addr.sin_family = hp->h_addrtype;
  serv_addr.sin_port = htons(data->port);
  
  if (connect(data->firstsocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    switch(errno) {
#ifndef WIN32
      /* this should be made nicer */
    case ECONNREFUSED:
      failf(data, "Connection refused");
      break;
#endif
    default:
      failf(data, "Can't connect to server");
      break;
    }
    return URG_COULDNT_CONNECT;
  }
  now = time(NULL); /* time this *after* the connect is done */
  bytecount = 0;
  

  /* Figure out the ip-number and the first host name it shows: */
  {
    struct in_addr in;
    (void) memcpy(&in.s_addr, *hp->h_addr_list, sizeof (in.s_addr));
    infof(data, "Connected to %s (%s)\n", hp->h_name, inet_ntoa(in));
  }

  if((conf&(CONF_FTP|CONF_PROXY)) == CONF_FTP) {
    /* this is FTP and no proxy, we don't do the usual crap then */

    /* The first thing we do is wait for the "220*" line: */
    nread = GetLastResponse(data->firstsocket, buf, data);
    if(strncmp(buf, "220", 3)) {
      failf(data, "This doesn't seem like a nice ftp-server response");
      return URG_FTP_WEIRD_SERVER_REPLY;
    }

    /* send USER */
    sendf(data->firstsocket, data, "USER %s\n", ftpuser);

    /* wait for feedback */
    nread = GetLastResponse(data->firstsocket, buf, data);

    if(!strncmp(buf, "530", 3)) {
      /* 530 User ... access denied
         (the server denies to log the specified user) */
      failf(data, "Access denied: %s", &buf[4]);
      return URG_FTP_ACCESS_DENIED;
    }
    else if(!strncmp(buf, "331", 3)) {
      /* 331 Password required for ...
         (the server requires to send the user's password too) */
      sendf(data->firstsocket, data, "PASS %s\n", ftppasswd);
      nread = GetLastResponse(data->firstsocket, buf, data);

      if(!strncmp(buf, "530", 3)) {
        /* 530 Login incorrect.
           (the username and/or the password are incorrect) */
        failf(data, "the username and/or the password are incorrect");
        return URG_FTP_USER_PASSWORD_INCORRECT;
      }
      else if(!strncmp(buf, "230", 3)) {
        /* 230 User ... logged in.
           (user successfully logged in) */
        
        infof(data, "We have successfully logged in\n");
      }
      else {
        failf(data, "Odd return code after PASS");
        return URG_FTP_WEIRD_PASS_REPLY;
      }
    }
    else if(! strncmp(buf, "230", 3)) {
      /* 230 User ... logged in.
	(the user logged in without password) */
      infof(data, "We have successfully logged in\n");
    }
    else {
      failf(data, "Odd return code after USER");
      return URG_FTP_WEIRD_USER_REPLY;
    }
    sendf(data->firstsocket, data, "PASV\n");

    nread = GetLastResponse(data->firstsocket, buf, data);

    if(strncmp(buf, "227", 3)) {
      failf(data, "Odd return code after PASV");
      return URG_FTP_WEIRD_PASV_REPLY;
    }
    else {
      int ip[4];
      int port[2];
      unsigned short newport;
      char newhost[32];
      struct hostent *he;
      /* 227 Entering Passive Mode (127,0,0,1,4,51) */
      if(6 != sscanf(buf, "%*[^(](%d,%d,%d,%d,%d,%d)",
                     &ip[0], &ip[1], &ip[2], &ip[3],
                     &port[0], &port[1])) {
        failf(data, "Oddly formatted 227-reply");
        return URG_FTP_WEIRD_227_FORMAT;
      }
      sprintf(newhost, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      he = GetHost(data, newhost);
      if(!he) {
        failf(data, "Can't resolve new host %s", newhost);
        return URG_FTP_CANT_GET_HOST;
      }

      newport = port[0]*256 + port[1];
      data->secondarysocket = socket(AF_INET, SOCK_STREAM, 0);

      memset((char *) &serv_addr, '\0', sizeof(serv_addr));
      memcpy((char *)&(serv_addr.sin_addr), he->h_addr, he->h_length);
      serv_addr.sin_family = he->h_addrtype;
      serv_addr.sin_port = htons(newport);
  
      if (connect(data->secondarysocket, (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) {
        switch(errno) {
#ifndef WIN32
          /* this should be made nicer */
        case ECONNREFUSED:
          failf(data, "Connection refused");
          break;
#endif
        default:
          failf(data, "Can't connect to server");
          break;
        }
        return URG_FTP_CANT_RECONNECT;
      }
      /* we have the data connection ready */


      if(conf & CONF_UPLOAD) {
        /* Set type to binary */
        sendf(data->firstsocket, data, "TYPE I\n");

        nread = GetLastResponse(data->firstsocket, buf, data);

        if(strncmp(buf, "200", 3)) {
          failf(data, "Couldn't set binary mode");
          return URG_FTP_COULDNT_SET_BINARY;
        }

        /* Send everything on data->in to the socket */
        sendf(data->firstsocket, data, "STOR %s\n", ppath);

        nread = GetLastResponse(data->firstsocket, buf, data);

        if(atoi(buf)>=400) {
          if(conf & CONF_VERBOSE)
            failf(data, "Failed FTP upload:%s", buf+3);
          /* oops, we never close the sockets! */
          return URG_FTP_COULDNT_STOR_FILE;
        }
        bytecount=0;

        /* When we know we're uploading a specified file, we can get the file
           size prior to the actual upload. */

        ProgressInit(data, data->infilesize);
        result = Upload(data, data->secondarysocket, data->infilesize,
                        &bytecount);
        if(result)
          return result;

        if((-1 != data->infilesize) && (data->infilesize != bytecount)) {
          failf(data, "Wrote only partial file (%d out of %d bytes)",
                bytecount, data->infilesize);
          return URG_FTP_PARTIAL_FILE;
        }

      }
      else {
        /* Retrieve file or directory */

        if(!ppath[0])
          /* make sure this becomes a valid name */
          ppath="/";

        if('/' == ppath[strlen(ppath)-1]) {
          /* The specified path ends with a slash, and therefore we think this
             is a directory that is requested, use LIST. But before that we
             need to set ASCII transfer mode. */

          /* Set type to ASCII */
          sendf(data->firstsocket, data, "TYPE A\n");

          nread = GetLastResponse(data->firstsocket, buf, data);

          if(strncmp(buf, "200", 3)) {
            failf(data, "Couldn't set ascii mode");
            return URG_FTP_COULDNT_SET_ASCII;
          }

          /* if this output is to be machine-parsed, the NLST command will be
             better used since the LIST command output is not specified or
             standard in any way */

          sendf(data->firstsocket, data, "%s %s\n",
                data->conf&CONF_FTPLISTONLY?"NLST":"LIST",
                ppath);
        }
        else {
          /* Set type to binary */
          sendf(data->firstsocket, data, "TYPE I\n");

          nread = GetLastResponse(data->firstsocket, buf, data);

          if(strncmp(buf, "200", 3)) {
            failf(data, "Couldn't set binary mode");
            return URG_FTP_COULDNT_SET_BINARY;
          }

          sendf(data->firstsocket, data, "RETR %s\n", ppath);
        }
        nread = GetLastResponse(data->firstsocket, buf, data);

        if(!strncmp(buf, "150", 3)) {
          /* 150 Opening BINARY mode data connection for /etc/passwd (2241
           bytes).  (ok, the file is being transfered) */

          /* 150 Opening ASCII mode data connection for /bin/ls */

          int size=-1; /* default unknown size */

          sscanf(buf, "%*[^(](%d", &size);

          ProgressInit(data, size);

          infof(data, "Getting file with size: %d\n", size);

          result=Download(data, data->secondarysocket, size, FALSE,
                          &bytecount);
          if(result)
            return result;

          if((-1 != size) && (size != bytecount)) {
            failf(data, "Received only partial file");
            return URG_FTP_PARTIAL_FILE;
          }
        }
        else {
          failf(data, "%s", buf+4);
          return URG_FTP_COULDNT_RETR_FILE;
        }
	
      }
      /* end of transfer */
      ProgressEnd(data);

      /* shut down the socket to inform the server we're done */
      sclose(data->secondarysocket);
      data->secondarysocket = -1;

      /* now let's see what the server says about the transfer we
	 just performed: */
      nread = GetLastResponse(data->firstsocket, buf, data);

      /* 226 Transfer complete */
      if(strncmp(buf, "226", 3)) {
	failf(data, "%s", buf+4);
	return URG_FTP_WRITE_ERROR;
      }
    }
  }

  else {
    /* Send the GET line to the HTTP server */

    char authorization[512];
    char userpwd[512];
    char proxyuserpwd[512];
    char rangeline[512];
    char content[80];
    char ref[512];

    if(conf & CONF_USERPWD) {
      sprintf(userpwd, "%s:%s", ftpuser, ftppasswd);
      base64Encode(userpwd, authorization);
      sprintf(userpwd,
              "Authorization: Basic %s\015\012",
              authorization);
    }
    if(conf & CONF_PROXYUSERPWD) {
      sprintf(proxyuserpwd, "%s:%s", proxyuser, proxypasswd);
      base64Encode(proxyuserpwd, authorization);
      sprintf(proxyuserpwd,
              "Proxy-authorization: Basic %s\015\012",
              authorization);
    }
    if(conf & CONF_RANGE) {
      sprintf(rangeline, "Range: bytes=%s\015\012", data->range);
    }
    if(conf & CONF_POST) {
      sprintf(content, "Content-length: %d\015\012",
              strlen(data->postfields)) ;
    }
    if(conf & CONF_REFERER) {
      sprintf(ref, "Referer: %s\015\012", data->referer);
    }
    sendf(data->firstsocket, data,
          "%s %s HTTP/1.0\015\012"
          "%s"
          "%s"
          "%s"
          "%s"
          "Host: %s\015\012"
          "User-Agent: urlget/" URLGET_VERSION "\015\012"
          "Pragma: no-cache\015\012"
          "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\015\012"
          "%s"
          "%s%s"
          "%s\015\012",

          conf&CONF_NOBODY?"HEAD":(conf&CONF_POST?"POST":"GET"),
          ppath,
          (conf&CONF_PROXYUSERPWD)?proxyuserpwd:"",
          (conf&CONF_USERPWD)?userpwd:"",
          (conf&CONF_KEEPALIVE)?"Connection: Keep-Alive\015\012":"",
          (conf&CONF_RANGE)?rangeline:"",
          name, /* host */
	  (conf&CONF_REFERER)?ref:"",
          (conf&CONF_POST)?content:"",
          (conf&CONF_POST)?"Content-type: application/x-www-form-urlencoded\015\012\015\012":"",
          (conf&CONF_POST)?data->postfields:""
          );

    result = Download(data, data->firstsocket, -1, TRUE, &bytecount);
    if(result)
      return result;

    ProgressEnd(data);

  }
  if(bytecount) {
    time_t end=time(NULL);
    infof(data, "%i bytes transfered in %d seconds (%d bytes/sec).\n",
          bytecount, end-now, bytecount/(end-now?end-now:1));
  }
  return URG_OK;
}

/* infof() is for info message along the way */

static void infof(struct UrlData *data, char *fmt, ...)
{
  va_list ap;
  if(data->conf & CONF_VERBOSE) {
    va_start(ap, fmt);
    fputs("* ", stderr);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
  }
}

/* failf() is for messages stating why we failed, the LAST one will be
   returned for the user (if requested) */

static void failf(struct UrlData *data, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if(data->errorbuffer)
    vsprintf(data->errorbuffer, fmt, ap);
  else /* no errorbuffer receives this, write to stderr instead */
    vfprintf(stderr, fmt, ap);
  va_end(ap);
}

/* sendf() sends the formated data to the server */

static int sendf(int fd, struct UrlData *data, char *fmt, ...)
{
  char s[1024];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(s, fmt, ap);
  va_end(ap);
  if(data->conf & CONF_VERBOSE)
    fprintf(stderr, "> %s", s);
  return(swrite(fd, s, strlen(s)));
}
