/*
 * Copyright (c) 2000-2001 Silicon Graphics, Inc.  All Rights Reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Further, this software is distributed without any warranty that it is
 * free of the rightful claim of any third person regarding infringement
 * or the like.  Any license provided herein, whether implied or
 * otherwise, applies only to this software file.  Patent licenses, if
 * any, provided herein do not apply to combinations of this program with
 * other software, or any other product whatsoever.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston MA 02111-1307, USA.
 * 
 * Contact information: Silicon Graphics, Inc., 1600 Amphitheatre Pkwy,
 * Mountain View, CA  94043, or:
 * 
 * http://www.sgi.com 
 * 
 * For further information regarding this notice, see: 
 * 
 * http://oss.sgi.com/projects/GenInfo/SGIGPLNoticeExplan/
 */

#include <libxfs.h>
#include <jdm.h>

#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "types.h"
#include "util.h"
#include "mlog.h"
#include "dlog.h"
#include "global.h"
#include "getopt.h"


/* declarations of externally defined global symbols *************************/

extern void usage( void );
extern bool_t pipeline;


/* forward declarations of locally defined static functions ******************/

#ifdef DUMP
static char * prompt_label( char *bufp, size_t bufsz );
#endif /* DUMP */

/* definition of locally defined global variables ****************************/


/* definition of locally defined static variables *****************************/


/* definition of locally defined global functions ****************************/

global_hdr_t *
global_hdr_alloc( intgen_t argc, char *argv[ ] )
{
	global_hdr_t *ghdrp;
	int c;
	char *dumplabel;
#ifdef DUMP
	char labelbuf[ GLOBAL_HDR_STRING_SZ ];
#endif /* DUMP */

	intgen_t rval;

	/* sanity checks
	 */
	ASSERT( sizeof( time32_t ) == GLOBAL_HDR_TIME_SZ );
	ASSERT( sizeof( uuid_t ) == GLOBAL_HDR_UUID_SZ );

	/* allocate a global hdr
	 */
	ghdrp = ( global_hdr_t * )calloc( 1, sizeof( global_hdr_t ));
	ASSERT( ghdrp );

	/* fill in the magic number
	 */
	strncpy( ghdrp->gh_magic, GLOBAL_HDR_MAGIC, GLOBAL_HDR_MAGIC_SZ );

	/* fill in the hdr version
	 */
	ghdrp->gh_version = GLOBAL_HDR_VERSION;

	/* fill in the timestamp: all changes made at or after this moment
	 * will be included in increments on this base.
	 */
	ghdrp->gh_timestamp = (time32_t) time( 0 );

	/* fill in the host id: typecast to fit into a 64 bit field
	 */
	ghdrp->gh_ipaddr = ( u_int64_t )( unsigned long )gethostid( );

#ifdef DUMP
	uuid_generate( ghdrp->gh_dumpid );
#endif /* DUMP */
#ifdef RESTORE
	uuid_clear( ghdrp->gh_dumpid );
#endif /* RESTORE */

	/* fill in the hostname
	 */
	rval = gethostname( ghdrp->gh_hostname, GLOBAL_HDR_STRING_SZ );
	if ( rval ) {
		mlog( MLOG_NORMAL | MLOG_ERROR,
		      "unable to determine hostname: %s\n",
		      strerror( errno ));
		return 0;
	}
	if ( ! strlen( ghdrp->gh_hostname )) {
		mlog( MLOG_NORMAL | MLOG_ERROR,
		      "hostname length is zero\n" );
		return 0;
	}

	/* scan the command line for the dump session label
	 */
	dumplabel = 0;
	optind = 1;
	opterr = 0;
	while ( ( c = getopt( argc, argv, GETOPT_CMDSTRING )) != EOF ) {
		switch ( c ) {
                case GETOPT_DUMPLABEL:
                        if ( dumplabel ) {
                                mlog( MLOG_NORMAL,
                                      "too many -%c arguments: "
                                      "\"-%c %s\" already given\n",
                                      optopt,
                                      optopt,
                                      dumplabel );
                                usage( );
                                return 0;
                        }
                        if ( ! optarg || optarg[ 0 ] == '-' ) {
                                mlog( MLOG_NORMAL | MLOG_ERROR,
                                      "-%c argument missing\n",
                                      optopt );
                                usage( );
                                return 0;
                        }
                        dumplabel = optarg;
                        break;
#ifdef RESTORE
		case GETOPT_SESSIONID:
                        if ( ! uuid_is_null( ghdrp->gh_dumpid )) {
                                mlog( MLOG_NORMAL | MLOG_ERROR,
                                      "too many -%c arguments\n",
                                      optopt );
                                usage( );
                                return 0;
                        }
                        if ( ! optarg || optarg[ 0 ] == '-' ) {
                                mlog( MLOG_NORMAL | MLOG_ERROR,
                                      "-%c argument missing\n",
                                      optopt );
                                usage( );
                                return 0;
                        }
			
			if ( ! uuid_parse( optarg, ghdrp->gh_dumpid ) ) {
				mlog( MLOG_NORMAL | MLOG_ERROR,
				      "-%c argument not a valid uuid\n",
				      optopt );
                                usage( );
                                return 0;
                        }
                        break;
#endif /* RESTORE */
#ifdef DUMP
#ifdef EXTATTR
		case GETOPT_NOEXTATTR:
			/* if this is the version which dumps extended
			 * file attributes and the option to not do so
			 * has been specified, then we can regress the
			 * header version number.
			 */
			ghdrp->gh_version = GLOBAL_HDR_VERSION_0;
			break;
#endif /* EXTATTR */
#endif /* DUMP */
		}
	}

#ifdef DUMP
	/* if no dump label specified, no pipes in use, and dialogs
	 * are allowed, prompt for one
	 */
	if ( ! dumplabel && dlog_allowed( )) {
		dumplabel = prompt_label( labelbuf, sizeof( labelbuf ));
	}
#endif /* DUMP */

	if ( ! dumplabel || ! strlen( dumplabel )) {
#ifdef DUMP
		if ( ! pipeline ) {
			mlog( MLOG_VERBOSE | MLOG_WARNING,
			      "no session label specified\n" );
		}
#endif /* DUMP */
		dumplabel = "";
	}

	strncpyterm( ghdrp->gh_dumplabel,
		     dumplabel,
		     sizeof( ghdrp->gh_dumplabel ));

	return ghdrp;
}


void
global_hdr_free( global_hdr_t *ghdrp )
{
    free( ( void * )ghdrp );
}

/* global_hdr_checksum_set - fill in the global media file header checksum.
 * utility function for use by drive-specific strategies.
 */
void
global_hdr_checksum_set( global_hdr_t *hdrp )
{
	u_int32_t *beginp = ( u_int32_t * )&hdrp[ 0 ];
	u_int32_t *endp = ( u_int32_t * )&hdrp[ 1 ];
	u_int32_t *p;
	u_int32_t accum;

	hdrp->gh_checksum = 0;
	accum = 0;
	for ( p = beginp ; p < endp ; p++ ) {
		accum += INT_GET(*p, ARCH_CONVERT);
	}
	INT_SET(hdrp->gh_checksum, ARCH_CONVERT, (int32_t)(~accum + 1));
}

/* global_hdr_checksum_check - check the global media file header checksum.
 * utility function for use by drive-specific strategies.
 * returns BOOL_TRUE if ok, BOOL_FALSE if bad
 */
bool_t
global_hdr_checksum_check( global_hdr_t *hdrp )
{
	u_int32_t *beginp = ( u_int32_t * )&hdrp[ 0 ];
	u_int32_t *endp = ( u_int32_t * )&hdrp[ 1 ];
	u_int32_t *p;
	u_int32_t accum;

	accum = 0;
	for ( p = beginp ; p < endp ; p++ ) {
		accum += INT_GET(*p, ARCH_CONVERT);
	}
	return accum == 0 ? BOOL_TRUE : BOOL_FALSE;
}

/* global_version_check - if we know this version number, return BOOL_TRUE 
 * else return BOOL_FALSE
 */
bool_t 
global_version_check( u_int32_t version )
{
	switch (version) {
		case GLOBAL_HDR_VERSION_0:
		case GLOBAL_HDR_VERSION_1:
		case GLOBAL_HDR_VERSION_2:
			return BOOL_TRUE;
		default:
			return BOOL_FALSE;
	}
}

/* definition of locally defined static functions ****************************/

#ifdef DUMP
#define PREAMBLEMAX	3
#define QUERYMAX	1
#define CHOICEMAX	1
#define ACKMAX		3
#define POSTAMBLEMAX	3
#define DLOG_TIMEOUT	300

/* ARGSUSED */
static void
prompt_label_cb( void *uctxp, dlog_pcbp_t pcb, void *pctxp )
{
	/* query: ask for a dump label
	 */
	( * pcb )( pctxp,
		   "please enter label for this dump session" );
}

static char *
prompt_label( char *bufp, size_t bufsz )
{
	fold_t fold;
	char *preamblestr[ PREAMBLEMAX ];
	size_t preamblecnt;
	char *ackstr[ ACKMAX ];
	size_t ackcnt;
	char *postamblestr[ POSTAMBLEMAX ];
	size_t postamblecnt;
	const ix_t abortix = 1;
	const ix_t okix = 2;
	ix_t responseix;

	preamblecnt = 0;
	fold_init( fold, "dump label dialog", '=' );
	preamblestr[ preamblecnt++ ] = "\n";
	preamblestr[ preamblecnt++ ] = fold;
	preamblestr[ preamblecnt++ ] = "\n\n";
	ASSERT( preamblecnt <= PREAMBLEMAX );
	dlog_begin( preamblestr, preamblecnt );

	responseix = dlog_string_query( prompt_label_cb,
					0,
					bufp,
					bufsz,
					DLOG_TIMEOUT,
					abortix,/* timeout ix */
					IXMAX, /* sigint ix */
					IXMAX,  /* sighup ix */
					IXMAX,  /* sigquit ix */
					okix );   /* ok ix */
	ackcnt = 0;
	if ( responseix == okix ) {
		ackstr[ ackcnt++ ] = "session label entered: \"";
		ackstr[ ackcnt++ ] = bufp;
		ackstr[ ackcnt++ ] = "\"\n";
	} else {
		ackstr[ ackcnt++ ] = "session label left blank\n";
	}

	ASSERT( ackcnt <= ACKMAX );
	dlog_string_ack( ackstr,
			 ackcnt );

	postamblecnt = 0;
	fold_init( fold, "end dialog", '-' );
	postamblestr[ postamblecnt++ ] = "\n";
	postamblestr[ postamblecnt++ ] = fold;
	postamblestr[ postamblecnt++ ] = "\n\n";
	ASSERT( postamblecnt <= POSTAMBLEMAX );
	dlog_end( postamblestr,
		  postamblecnt );

	if ( responseix == okix ) {
		return bufp;
	} else {
		return 0;
	}
}
#endif /* DUMP */
