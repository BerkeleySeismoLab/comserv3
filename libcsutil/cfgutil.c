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
    8  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins.
                    Changed "if feof(cs->cfgfile)" to "if (feof(cs->cfgfile))"
		    to avoid compiler parsing error.
    9  4 Jan 2011 Paulf  - added @include directive handling
   10 17 Feb 2012 DSN Fixed read_cfg() to not close top level cfg file on EOF.
   11 29 Sep 2020 DSN Updated for comserv3.
   12 30 Apr 2021 DSN Check for max depth of include files.
		      Add cstypes.h and cfgutil.h include files.
		      Remove duplicate definition of config_struc.
		      Fixed include file handling to allow include file at
		      any place in the configuration files.
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include "cstypes.h"
#include "cfgutil.h"
#include "stuff.h"

/* type definitions needed to use this module */
#define CFGWIDTH 512
#define SECWIDTH 256

#define COMSERV_PARAMS_ENV "COMSERV_PARAMS"	/* the params dir where @ included file will be found */

short VER_CFGUTIL = 12 ;

#define SUCCESS 0
#define FAILURE 1

/* Open the specified configuration file.  
   The file may be either a primary or include configuration file.
   Return SUCCESS on success, FAILURE otherwise. */
static short _open_cfg (config_struc *cs, pchar fname)
{
    if (cs->cfg_init == 0)
    {
 	/* We have uninitialized cfg struct and this is first open call. */
        cs->cfg_init = 1;
        cs->current_file = -1;
    } 
    cs->current_file++; /* new file */

    /* Ensure we do not exceed the maximum deptch of include files. */
    if (cs->current_file >= MAX_INDIRECTION) {
	fprintf (stderr, "util/open_cfg(): Fatal Warning: include file level exceeds %d, skipping %s\n",
		 MAX_INDIRECTION, fname);
	cs->current_file--;
	return FAILURE;
    }

    cs->cfgfile[cs->current_file] = fopen(fname, "r") ;
    if (cs->cfgfile[cs->current_file] == NULL)
    {
	fprintf(stderr, "util/open_cfg(): Fatal Warning: file at path %s could not be opened, skipping\n", fname);
	cs->current_file--; /* did not open file successfully! */
	if (cs->current_file < 0) cs->cfg_init = 0;
	return FAILURE ;
    }
    cs->lastread[0] = '\0' ; /* there is no next section */
    return SUCCESS;
}

/* Open an include configuration file.
   Return SUCCESS if successful, FAILURE otherwise. */
static int _open_include_cfg(config_struc *cs)
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
    return (_open_cfg(cs, filename));
}
   
/* Close an include configuration file. */
static void _close_include_cfg (config_struc *cs)
{
    if (cs->cfg_init == 0 || cs->current_file == -1 || cs->cfgfile[cs->current_file] == NULL) 
    { 
	cs->cfg_init = 0;
	return;
    }
    /* Close the current include files. */
    if (cs->current_file > 0) {
	fclose(cs->cfgfile[cs->current_file]) ;
	cs->cfgfile[cs->current_file] = NULL;
	cs->current_file--;
    }
}

/* Skip to the specified section.
 * Do NOT close the main config file if section is not found.
 * Return SUCCESS if desired section is found.
 * Return FAILURE if section not found or problem with file. */
short skipto (config_struc *cs, pchar section)
{
    char s[SECWIDTH] ;
    short tries ;
    pchar tmp ;

    /* Check for initial error conditions. */
    if (section == NULL) 
    {
	/* No section is specified. */
	return FAILURE;
    } 
    if (cs->current_file == -1) 
    {
	/* No configuration file open. */
	return FAILURE;
    }
   

    /* Process the main configuration file up to twice if looking for a */
    /* specific section, or until EOF if looking for a "*" (any) section. */
    tries = 0 ;
    do
    {
	/* build "[section]" string */
	strcpy(s, "[") ;
	strcat(s, section) ;
	strcat(s, "]") ;

	/* If the last line read is the section desired or if the section
	   is "*" and the line is any section start, then return SUCCESS. */
	if ((strcasecmp((pchar)&cs->lastread, (pchar)&s) == 0) ||
	    ((strcmp(section, "*") == 0) && (cs->lastread[0] == '[')))
	    return SUCCESS ;

	/* Read lines (removing trailing spaces) until either the desired
	   section is found, or if the section is "*" and the new line is
	   any section start. If found, return SUCCESS.
	   Process any include_file lines encountered. */
	do
	{
	    /* Read the next line in the current config file. */
	    tmp = fgets(cs->lastread, CFGWIDTH-1, cs->cfgfile[cs->current_file]);
	    untrail (cs->lastread);
	    if (tmp == NULL && cs->current_file > 0)
	    {
		/* Close include file and continue reading open config file. */
		_close_include_cfg (cs);
		/* Force the loop to continue with the open config file. */
		tmp = strcpy(cs->lastread,"");
		continue;
	    }
	    if (tmp != NULL && cs->lastread[0] == '@') 
	    {
		/* Open include file, and continue reading include file. */
		if (_open_include_cfg(cs) == SUCCESS) 
		{
		    continue;	/* Continue reading from include file. */
		}
		else
		{
		    return FAILURE ; /* Error opening the include file. */
		}
	    }
	}
	while ((tmp != NULL) && (strcasecmp((pchar)&cs->lastread, (pchar)&s) != 0) &&
	       ((strcmp(section, "*") != 0) || (cs->lastread[0] != '['))) ;
            
	/* If we found the specified section, return SUCCESS. */
	if (tmp != NULL) {
	    return SUCCESS;
	}

	/* We are at EOF on the top level config file. */
	/* Rewind the file and increment the number of tries through */
	/* at least a portion of the file looking for the specified section. */
	rewind(cs->cfgfile[cs->current_file]) ;
	cs->lastread[0] = '\0' ;
	tries++ ;
    }
    while ((tries <= 1) && (strcmp(section, "*") != 0)) ;
    return FAILURE ; /* Failure to find specified section. */    
}

/* Open the configuration file and search for the specified section.
   Return SUCCESS if the file is opened AND the specified section if found.
   Return FAILURE otherwise. */
short open_cfg (config_struc *cs, pchar fname, pchar section)
{
    if (_open_cfg (cs, fname) != SUCCESS) {
	return FAILURE; /* Error opening config file. */
    }
    else {
	cs->lastread[0] = '\0' ; /* there is no next section */
	cs->current_section = section;
	return skipto(cs, section) ; /* Skip to specified section. */
    }
}

/* Read and parse the next configuration directive.
   Returns the part to the left of the "=" in s1, upshifted. 
   Returns the part to the right of the "=" in s2, not upshifted. 
   Returns s1=NULL if no more directives in the section. */
void read_cfg (config_struc *cs, pchar s1, pchar s2)
{
    pchar tmp;

/* start with two null strings, in case things don't go well */
    *s1 = '\0' ;
    *s2 = '\0' ;
    do
    {
	/* Ensure we have an open configuration file. */
	if (cs->current_file == -1) 
	{
	    return; /* No configuration file is open */
	}

	/* Handle EOF on both include and main configuration files. */
	if  (feof(cs->cfgfile[cs->current_file]))
	{
	    if (cs->current_file > 0)	    {
		/* Close this include file and continue with outer file. */
		_close_include_cfg(cs);
		continue;
	    }
	    else {
		/* We are at EOF of the main configuration file. */
		return; 
	    }
	}

	/* Read the next line and remove trailing spaces */
	tmp = fgets(cs->lastread, CFGWIDTH-1, cs->cfgfile[cs->current_file]) ;
	untrail (cs->lastread) ;

	/* If we read a non-empty line, examine the line. */
	if ((tmp != NULL) && (cs->lastread[0] != '\0'))
	{
	    /* If line starts with a '@', it specified an include file. */
	    if (cs->lastread[0] == '@')
	    {
		if (_open_include_cfg(cs) == SUCCESS ) {
		    continue; /* Continue reading in the include file. */
		}
	    }
	    /* If line starts with a '[', it is start of new section, */
	    /* which means the no more directives in the current section. */
	    if (cs->lastread[0] == '[')
		return ;
	    /* If first character in the line is A-Z or 0-9, then process the line */
	    if (isalnum(cs->lastread[0]))
		break ;
	}
    }
    while (1) ;

    /* We have a presumed configuration directive line. */
    /* Parse the line into keyword (s1) and value (s2) fields. */
    strcpy(s1, cs->lastread) ;
    tmp = strchr(s1, '=') ;
    if (tmp)
    {
	str_right (s2, tmp) ; /* copy from tmp+1 to terminator */
	*tmp = '\0' ; /* replace "=" with terminator */
    }
    upshift(s1) ;
}

/* Close all files (including any included files) opend with this config_struct. */
void close_cfg (config_struc *cs)
{
    if (cs->cfg_init == 0 || cs->current_file == -1 || cs->cfgfile[cs->current_file] == NULL) 
    { 
	cs->cfg_init = 0;
	return;
    }
    /* Close all include files. */
    while (cs->current_file > 0) {
	_close_include_cfg(cs);
    }
    /* Close initial config file. */
    if (cs -> current_file == 0) {
	fclose(cs->cfgfile[cs->current_file]) ;
	cs->cfgfile[cs->current_file] = NULL;
	cs->current_file--;
	cs->cfg_init = 0;
    }
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
