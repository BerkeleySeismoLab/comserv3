/*$Id: cfgutil.c,v 1.2 2005/03/30 22:27:56 isti Exp $*/
/*   Configuration file utility module.
     Copyright 1994-1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 15 Mar 94 WHO Translated from cfgutil.pas
    1 20 Mar 94 WHO split procedure added.
    2 30 May 94 WHO Extra semicolons in read_cfg removed. Incorrect
                    parameter to fclose in close_cfg fixed (DSN).
    3  9 Jun 94 WHO Cleanup to avoid warnings.
    4 27 Feb 95 WHO Start of conversion to run on OS9.
    5  9 Jul 96 WHO Fix read_cfg so that it notices end of file when
                    it really happens, rather than repeating the last
                    line read.
    6 16 Jul 96 WHO Put back in the initialization of "tries" in skipto,
                    it disapeared somehow.
    7  3 Nov 97 WHO Larger configuration sizes. Add VER_CFGUTIL.
    8  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
                        IGD  In version 7 which I have read_cfg () expression "if     (feof(cs->cfgfile))"
                                 was "if feof(cs->cfgfile)" causing a compiler parsing error. Fixed.
    9  4 Jan 2011 Paulf  - added @include directive handling
   10 17 Feb 2012 DSN Fixed read_cfg() to not close top level cfg file on EOF.
   11 29 Sep 2020 DSN Updated for comserv3.
   12 29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include "stuff.h"

/* type definitions needed to use this module */
#define CFGWIDTH 512
#define SECWIDTH 256

#define COMSERV_PARAMS_ENV "COMSERV_PARAMS"	/* the params dir where @ included file will be found */

short VER_CFGUTIL = 10 ;


#define MAX_INDIRECTION 10
typedef struct
{
    char lastread[CFGWIDTH];
    FILE *cfgfile[MAX_INDIRECTION];
    int current_file;
    char *current_section;
} config_struc;

int cfg_init = 0; /* used to detect new cfg struct, for initialization purposes */
config_struc *cfg_init_ptr = NULL; /* the cfg structure that cfg_init refers to */

/* declarations for funcs in this file used in by other funcs */
short open_cfg (config_struc *cs, pchar fname, pchar section); 
void close_cfg (config_struc *cs);

/* handles the @include file opening
   returns TRUE if there is a problem with include file

   input: in_section if NULL indicates we are in a section already skip'ed to
*/
int _include_cfg(config_struc *cs, char *in_section)
{                
    pchar path;
    pchar file;
    char filename[2*CFGWIDTH];

    file = &cs->lastread[1];
    filename[0]=0;
    /* open the new file  using the COMSERV_PARAMS env var */
    if (file[0] == '/')
    {
	strcat(filename, file);	/* we have an absolute path */
    }
    else if ((path = getenv(COMSERV_PARAMS_ENV)) != NULL)
    {
	strcat(filename, path);
	strcat(filename, "/");
	strcat(filename, file);
    }
    else
    {
	/* also just use the filename as is verbatim */
	strcat(filename, file);
    }
    return (open_cfg(cs, filename, in_section));
}
   
/* skip to the specified section 
 *   returns FALSE if section found!
 *   returns TRUE if section not found or problem with file */
short skipto (config_struc *cs, pchar section)
{
    char s[SECWIDTH] ;
    short tries ;
    pchar tmp ;

    if (section == NULL) 
    {
	/* this is an @include file inside a section already, return all is OK....FALSE */
	return FALSE;
    } 
    if (cs->current_file == -1) 
    {
	/* we closed this file out on a previous read of the config file */
	return TRUE;
    }
   
    /* repeat up to twice if looking for a specific section, or once
       if looking for any section "*" */
    tries = 0 ;
    do
    {
	/* build "[section]" string */
	strcpy(s, "[") ;
	strcat(s, section) ;
	strcat(s, "]") ;
	/* If the last line read is the section desired or if the section
	   is "*" and the line is any section start then return with FALSE
	   status */
	if ((strcasecmp((pchar)&cs->lastread, (pchar)&s) == 0) ||
	    ((strcmp(section, "*") == 0) && (cs->lastread[0] == '[')))
	    return FALSE ;
	/* read lines (removing trailing spaces) until either the desired
	   section is found, or if the section is "*" and the new line is
	   any section start. If found, return FALSE */
	do
	{
	    tmp = fgets(cs->lastread, CFGWIDTH-1, cs->cfgfile[cs->current_file]);
	    untrail (cs->lastread);
	    if (cs->lastread[0] == '@') 
	    {
		if (_include_cfg(cs, section) == TRUE) 
		{
		    continue; /* go back and read next line from this file, 
				 included one did not have desired section,  
				 or had fatal error opening */
		}
		else
		{
		    return FALSE; /* we found it in the included file */
		}
	    }
	}
	while ((tmp != NULL) && (strcasecmp((pchar)&cs->lastread, (pchar)&s) != 0) &&
	       ((strcmp(section, "*") != 0) || (cs->lastread[0] != '['))) ;
            
	/* if eof(cfgfile) */
	if (tmp == NULL)
	{
	    tries++ ;
	    rewind(cs->cfgfile[cs->current_file]) ;
	    cs->lastread[0] = '\0' ;
	}
	else
	    return FALSE ;
    }
    while ((tries <= 1) && (strcmp(section, "*") != 0)) ;
    /* if tmp == NULL then we went through the whole file, we should close it  */
    if (tmp == NULL) 
    {
	close_cfg(cs);
    }
    return TRUE ; /* no good */    
}

/* open the configuration file, return TRUE if the file cannot be opened
   or the section is not found */
short open_cfg (config_struc *cs, pchar fname, pchar section)
{
    if (cfg_init == 0 && cs != cfg_init_ptr) 
    {
 	/* we have uninitialized cfg struct and this is first open call */
        cfg_init = 1;
        cfg_init_ptr = cs;
        cs->current_file = -1;
    } 
    else if (cfg_init == 1 && cs != cfg_init_ptr) 
    {
        /* we have a second config opened before a prior one closed...could be problematic! degenerate case: init anyway */
        cfg_init_ptr = cs;
        cs->current_file = -1;
    }

    cs->current_file++; /* new file */

    cs->cfgfile[cs->current_file] = fopen(fname, "r") ;
    if (cs->cfgfile[cs->current_file] == NULL)
    {
	fprintf(stderr, "util/open_cfg(): Fatal Warning: file at path %s could not be opened, skipping\n", fname);
	cs->current_file--; /* did not open file successfully! */
	return TRUE ;
    }
    cs->lastread[0] = '\0' ; /* there is no next section */
    cs->current_section = section;
    return skipto(cs, section) ; /* skip to section */
}


/* Returns the part to the left of the "=" in s1, upshifted. Returns the
   part to the right of the "=" in s2, not upshifted. Returns with s1
   a null string if no more strings in the section */
void read_cfg (config_struc *cs, pchar s1, pchar s2)
{
    pchar tmp;

/* start with two null strings, in case things don't go well */
    *s1 = '\0' ;
    *s2 = '\0' ;
    do
    {
	if (cs->current_file == -1) 
	{
	    return; /* something closed the file */
	}
	if  (feof(cs->cfgfile[cs->current_file]))
	{
	    if (cs->current_file > 0)	/* only close include files on EOF */
	    {
		close_cfg(cs);	/* close the include file */
		continue;
	    }
	    return; /* we have reached the last file in the stack */
	}
/* read a line and remove trailing spaces */
	tmp = fgets(cs->lastread, CFGWIDTH-1, cs->cfgfile[cs->current_file]) ;
	untrail (cs->lastread) ;
	if ((tmp != NULL) && (cs->lastread[0] != '\0'))
	{
/* if starts with a '@', it is file to include and start reading instead */
	    if (cs->lastread[0] == '@')
	    {
		if (_include_cfg(cs, NULL) == TRUE) {
		    continue; /* section was not found in included file */
		}
	    }
/* if starts with a '[', it is start of new section, return */
	    if (cs->lastread[0] == '[')
		return ;
/* if first character in a line is A-Z or 0-9, then process line */
	    if (isalnum(cs->lastread[0]))
		break ;
	}
    }
    while (1) ;
    strcpy(s1, cs->lastread) ;
    tmp = strchr(s1, '=') ;
    if (tmp)
    {
	str_right (s2, tmp) ; /* copy from tmp+1 to terminator */
	*tmp = '\0' ; /* replace "=" with terminator */
    }
    upshift(s1) ;
}

void close_cfg (config_struc *cs)
{
    if (cs->current_file == -1 || cs->cfgfile[cs->current_file] == NULL) 
    { 
	return;
    }
    fclose(cs->cfgfile[cs->current_file]) ;
    cs->current_file--;
}

/* Find the separator character in the source string, the portion to the
   right is moved into "right". The portion to the left remains in "src".
   The separator itself is removed. If the separator is not found then
   "src" is unchanged and "right" is a null string.
*/
void comserv_split (pchar src, pchar right, char sep)
{
    pchar tmp ;
      
    tmp = strchr (src, sep) ;
    if (tmp)
    {
	str_right (right, tmp) ;
	*tmp = '\0' ;
    }
    else
	right[0] = '\0' ;
}
