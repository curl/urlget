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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "urlget.h"

extern void hugehelp(void);

static void help(void)
{
  puts("urlget v" URLGET_VERSION "\n"
       " usage: urlget [options...] <url>\n"
       " options: (H) means HTTP only (F) means FTP only\n"
       "  -d/--data          POST data. (H)\n"
       "  -f/--fail          Fail silently (no output at all) on errors. (H)\n"
       "  -h/--help          Large help text\n"
       "  -i/--include       Include the HTTP-header in the output (H)\n"
       "  -I/--head          Fetch the HTTP-header only (HEAD)! (H)\n"
       "  -k/--keep-alive    Use Keep-Alive connection (H)\n"
       "  -l/--list-only     List only names of an FTP directory (F)\n"
       "  -m/--max-time <seconds> Maximum time allowed for the download\n"
       "  -o/--output <file> Write output to <file> instead of stdout\n"
       "  -O/--remote-name   Write output to a file named as the remote file\n"
       "  -p/--port <port>   Use port other than default for current protocol.\n"
       "  -r/--range <range> Retrieve a byte range from a HTTP/1.1 server (H)\n"
       "  -s/--silent        Silent mode. Don't show progress info\n"
       "  -t/--upload        Transfer/upload stdin to remote site. (F)\n"
       "  -T/--upload-file <file> Transfer/upload <file> to remote site. (F)\n"
       "  -u/--user <user:password> Specify user and password to use when fetching\n"
       "  -v/--verbose       Makes the fetching more talkative\n"
       "  -V/--version       Outputs version number then quits\n"
       "  -x/--proxy <host>  Use proxy. (Default port is 1080)"
       /* puts add a terminating newline by itself */
       );
}

struct LongShort {
  char letter;
  char *lname;
};

static int argcheck(char option, int i, int argc)
{
  if(i >= argc -1 ) {
    printf("Option '%c' is missing a parameter!\n", option);
    help();
    return 1; 
  }
  else
    return 0;
}

int main(argc,argv)
    int argc;
    char *argv[];
{
  long conf=CONF_DEFAULT;
  unsigned short porttouse=0;

  char errorbuffer[URLGET_ERROR_SIZE];

  char *proxy = NULL;

  char *outfile = NULL;
  char *infile = NULL;
  char *url = NULL;
  char *userpwd=NULL;
  char *range=NULL;
  char remotefile=FALSE;
  char *postfields=NULL;
  
  FILE *outfd = stdout;
  FILE *infd = stdin;
  char *urlbuffer=NULL;
  bool showerror=TRUE;
  long timeout=0;
  int infilesize=-1; /* -1 means unknown */

  int res;
  int i;

  struct LongShort aliases[]= {
    {'d', "date"},
    {'f', "fail"},
    {'h', "help"},
    {'i', "include"},
    {'I', "head"},
    {'k', "keep-alive"},
    {'l', "list-only"},
    {'m', "max-time"},
    {'o', "output"},
    {'O', "remote-name"},
    {'p', "port"},
    {'r', "range"},
    {'s', "silent"},
    {'t', "upload"},
    {'T', "upload-file"},
    {'u', "user"},
    {'v', "verbose"},
    {'V', "version"},
    {'x', "proxy"}
  };
  if (argc < 2) {
    help();
    return URG_FAILED_INIT;
  }

  /* Parse options */
  for (i = 1; i < argc; i++) {
    if('-' == argv[i][0]) {
      char letter=0;
      if('-' == argv[i][1]) {
        /* try a long name */
        if(argv[i][2]) {
          int j;
          for(j=0; j< sizeof(aliases)/sizeof(aliases[0]); j++)
            if(!strequal(aliases[j].lname, &argv[i][2])) {
              letter = aliases[j].letter;
              break;
            }
        }
      }
      else
        letter = argv[i][1];
      switch(letter) {
      case 'p':
        /* specified port */
        if(argcheck(letter, i, argc)) /* check we have another argument */
          return URG_FAILED_INIT;
        porttouse = atoi(argv[++i]);
        conf |= CONF_PORT; /* changed port */
        break;
      case 'm':
        /* specified port */
        if(argcheck(letter, i, argc)) /* check we have another argument */
          return URG_FAILED_INIT;
        timeout = atoi(argv[++i]);
        break;
      case 'x':
        /* proxy */
        if(argcheck(letter, i, argc)) /* check we have another argument */
          return URG_FAILED_INIT;
        proxy = argv[++i];
        conf |= CONF_PROXY;
        /* Sagula:
         * If port defined, don't change it. */
        if (!(conf & (CONF_PORT))) {
          porttouse = 1080;
          conf |= CONF_PORT;
        }
        break;
      case 'v':
        conf |= CONF_VERBOSE; /* talk a lot */
        break;
      case 'V':
        puts ("urlget version " URLGET_VERSION );
        return URG_FAILED_INIT;
      case 's':
        conf |= CONF_NOPROGRESS; /* don't show progress meter */
        showerror=FALSE;
        break;
      case 'i':
        conf |= CONF_HEADER; /* include the HTTP header aswell */
        break;
      case 'l':
        conf |= CONF_FTPLISTONLY; /* only list the names of the FTP dir */
        break;
      case 'I':
        conf |= CONF_HEADER; /* include the HTTP header in the output */
        conf |= CONF_NOBODY; /* don't fetch the body at all */
        break;
      case 'k':
        /* tell the server to keep the connection alive */
        conf |= CONF_KEEPALIVE;
        break;
      case 't':
        /* we are uploading */
        conf |= CONF_UPLOAD;
        break;
      case 'T':
        /* we are uploading */
        if(argcheck(letter, i, argc)) /* check we have another argument */
          return URG_FAILED_INIT;
        conf |= CONF_UPLOAD;
        infile = argv[++i]; /* read from this file */
        break;
      case 'o':
        /* output file */
        if(argcheck(letter, i, argc)) /* check we have another argument */
          return URG_FAILED_INIT;
        outfile = argv[++i]; /* write to this file */
        break;
      case 'O':
        /* output file */
        remotefile = TRUE;
        break;
      case 'f':
        /* fail hard on errors  */
        conf |= CONF_FAILONERROR;
        break;
      case 'u':
        /* user:password  */
        if(argcheck(letter, i, argc)) /* check we have another argument */
          return URG_FAILED_INIT;
        userpwd = argv[++i];
        conf |= CONF_USERPWD;
      case 'r':
        /* byte range requested */
        if(argcheck(letter, i, argc)) /* check we have another argument */
          return URG_FAILED_INIT;
        range = argv[++i];
        conf |= CONF_RANGE;
        break;
      case 'h': /* h for HUGE help */
        hugehelp();
        return URG_FAILED_INIT;
      case 'd':
        /* postfield data */
        postfields = argv[++i];
        /* printf("%s\n",postfields); fflush(stdout); */
        conf |= CONF_POST;
        break;
      default: /* unknown flag */
        if(letter)
          printf("Unknow option '%c'\n", letter);
        help(); /* show usage blurb */
        return URG_FAILED_INIT;
      }
    }
    else {
      url = argv[i];
      break;
    }
  }
#if 0
  /* previous, "friendly" style :-) */
  /* we consider the last argument being the URL */
  url = argv[argc - 1];
#else
  if(!url) {
    fprintf(stderr, "%s: consider specifying an URL too! :-)\n", argv[0]);
    return URG_FAILED_INIT;
  }
#endif

  if(outfile && infile) {
    fprintf(stderr, "%s: you can't both upload and download!\n", argv[0]);
    return URG_FAILED_INIT;
  }
 
  if (outfile || remotefile) {
    /* 
     * We have specified a file name to store the result in, or we have
     * decided we want to use the remote file name.
     */

    if(remotefile) {
      /* Find and get the remote file name */
      outfile=strstr(url, "://");
      if(outfile)
        outfile+=3;
      else
        outfile=url;
      outfile = strrchr(outfile, '/');
      if(!outfile || !strlen(++outfile)) {
        fprintf(stderr, "%s: Remote file name has no length!\n", argv[0]);
        return URG_WRITE_ERROR;
      }
    }

    /* open file for output: */
    outfd=(FILE *) fopen(outfile,"w");
    if (!outfd) {
      fprintf(stderr, "%s: Can't open '%s'!\n", argv[0], outfile);
      return URG_WRITE_ERROR;
    }
  }
  if (infile) {
    /*
     * We have specified a file to upload
     */
    struct stat fileinfo;

    /* If no file name part is given in the URL, we add this file name */
    char *ptr=strstr(url, "://");
    if(ptr)
      ptr+=3;
    else
      ptr=url;
    ptr = strrchr(ptr, '/');
    if(!ptr || !strlen(++ptr)) {
      /* The URL has no file name part, add the local file name. In order
         to be able to do so, we have to create a new URL in another buffer.*/
      urlbuffer=malloc(strlen(url) + strlen(infile) + 3);
      if(!urlbuffer) {
        fprintf(stderr, "%s: out of memory\n", argv[0]);
        return URG_OUT_OF_MEMORY;
      }
      if(ptr)
        /* there is a trailing slash on the URL */
        sprintf(urlbuffer, "%s%s", url, infile);
      else
        /* thers is no trailing slash on the URL */
        sprintf(urlbuffer, "%s/%s", url, infile);

      url = urlbuffer; /* use our new URL instead! */
    }
    

    infd=(FILE *) fopen(infile, "r");
    if (!infd) {
      fprintf(stderr, "%s: Can't open '%s'!\n", argv[0], infile);
      return URG_READ_ERROR;
    }
    else if(stat(infile, &fileinfo)) {
      fprintf(stderr, "%s: Can't stat '%s'!\n", argv[0], infile);
      return URG_READ_ERROR;
    }
    infilesize=fileinfo.st_size;
  }

  /* This was previously done in urlget, but that was wrong place to do it */
  if(isatty(fileno(outfd)))
    /* we send the output to a tty, and therefor we switch off the progress
       meter right away */
    conf |= CONF_NOPROGRESS;

  res = urlget(URGTAG_FILE, outfd,  /* where to store */
               URGTAG_INFILE, infd, /* for uploads */
               URGTAG_INFILESIZE, infilesize, /* size of uploaded file */
               URGTAG_URL, url,     /* what to fetch */
               URGTAG_PORT, porttouse, /* from which port */
               URGTAG_PROXY, proxy, /* proxy to use */
               URGTAG_FLAGS, conf, /* flags */
               URGTAG_USERPWD, userpwd, /* user + passwd */
               URGTAG_RANGE, range, /* range of document */
               URGTAG_ERRORBUFFER, errorbuffer,
               URGTAG_TIMEOUT, timeout,
               URGTAG_POSTFIELDS, postfields,
               URGTAG_DONE); /* always terminate the list of tags */

  if((res!=URG_OK) && showerror)
    fprintf(stderr, "%s: %s\n", argv[0], errorbuffer);

  if(urlbuffer)
    free(urlbuffer);
  if (outfile)
    fclose(outfd);
  if (infile)
    fclose(infd);

  return(res);
}

