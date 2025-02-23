#ifndef __CONFIG_H
#define __CONFIG_H
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
 * BIBLIOGRAPHY
 *   - "UNIX Network Programming" - Richard Stevens (rstevens@noao.edu)
 *   - "Advanced Programming in UNIX Environment" Richard Stevens (the same)
 *   - RFC 1738 T. Berners-Lee, L. Masinter, M. McCahill,
 *     "Uniform Resource Locators (URL)", 12/20/1994
 *   - RFC 2068 R. Fielding, J. Gettys, J. Mogul, H. Frystyk, T. Berners-Lee,
 *     "Hypertext Transfer Protocol -- HTTP/1.1", 01/03/1997.
 *   - RFC 959 J. Postel, J. Reynolds, "File Transfer Protocol", 10/01/1985.
 *
 ***************************************************************************/

#if !defined(WIN32) && defined(_WIN32)
/* This _might_ be a good Borland fix. Please report whether this works or
   not! */
#define WIN32
#endif

#if 0 /* used in 3.1, not anymore */
#ifndef HAVE_ALARM
/* remove all alarm() calls neatly */
#define alarm(x)
#endif
#endif

#ifdef HAVE_STRCASECMP
#define strequal(x,y) strcasecmp(x,y)
#else
#define strequal(x,y) stricmp(x,y)
#endif

/* Below we define three functions. They should
   1. close a socket
   2. read from a socket
   3. write to a socket
   Hopefully, only win32-crap do this weird name changing 
   */

#ifdef WIN32
#define sclose(x) closesocket(x)
#define sread(x,y,z) recv(x,y,z,0)
#define swrite(x,y,z) (size_t)send(x,y,z,0)
#else
#define sclose(x) close(x)
#define sread(x,y,z) read(x,y,z)
#define swrite(x,y,z) write(x,y,z)

#ifdef HAVE_STRCASECMP
/* this is for "-ansi -Wall -pedantic" to stop complaining! */
extern int strcasecmp(const char *s1, const char *s2);
int fileno( FILE *stream);
#endif

#endif

#endif /* __CONFIG_H */
