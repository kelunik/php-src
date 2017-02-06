/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2017 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Niklas Keller <kelunik@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#include "hrtime.h"

/* - timer.h ------------------------------------------------------------------------------------ */
/* The following code is based on:
   timer.h - Cross-platform timer library - Public Domain - 2011 Mattias Jansson / Rampant Pixels */

#define TIMER_PLATFORM_WINDOWS 0
#define TIMER_PLATFORM_APPLE   0
#define TIMER_PLATFORM_POSIX   0

#if defined( _WIN32 ) || defined( _WIN64 )
#  undef  TIMER_PLATFORM_WINDOWS
#  define TIMER_PLATFORM_WINDOWS 1
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif defined( __APPLE__ )
#  undef  TIMER_PLATFORM_APPLE
#  define TIMER_PLATFORM_APPLE 1
#  include <mach/mach_time.h>
#  include <string.h>
static mach_timebase_info_data_t _timerlib_info;
static void absolutetime_to_nanoseconds (uint64_t mach_time, uint64_t* clock ) { *clock = mach_time * _timerlib_info.numer / _timerlib_info.denom; }
#else
#  undef  TIMER_PLATFORM_POSIX
#  define TIMER_PLATFORM_POSIX 1
#  include <unistd.h>
#  include <time.h>
#  include <string.h>
#endif

static uint64_t _timer_freq = 0;

static int _timer_init()
{
#if TIMER_PLATFORM_WINDOWS
	uint64_t unused;
	if( !QueryPerformanceFrequency( (LARGE_INTEGER*)&_timer_freq ) ||
	    !QueryPerformanceCounter( (LARGE_INTEGER*)&unused ) )
		return -1;
#elif TIMER_PLATFORM_APPLE
	if( mach_timebase_info( &_timerlib_info ) )
		return -1;
	_timer_freq = 1000000000ULL;
#elif TIMER_PLATFORM_POSIX
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
	if( clock_gettime( CLOCK_MONOTONIC, &ts ) )
		return -1;
	_timer_freq = 1000000000ULL;
#endif

	return 0;
}

static uint64_t _timer_current()
{
#if TIMER_PLATFORM_WINDOWS

	uint64_t curclock;
	QueryPerformanceCounter( (LARGE_INTEGER*)&curclock );
	return curclock;

#elif TIMER_PLATFORM_APPLE

	uint64_t curclock = 0;
	absolutetime_to_nanoseconds( mach_absolute_time(), &curclock );
	return curclock;

#elif TIMER_PLATFORM_POSIX

	struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
	clock_gettime( CLOCK_MONOTONIC, &ts );
	return ( (uint64_t)ts.tv_sec * 1000000000ULL ) + ts.tv_nsec;

#endif
}

/* - end of timer.h ----------------------------------------------------------------------------- */

/* {{{ */
PHP_MINIT_FUNCTION(hrtime)
{
	if (_timer_init()) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed to initialize internal timer");
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ */
PHP_MSHUTDOWN_FUNCTION(hrtime)
{
	return SUCCESS;
}
/* }}} */

/* {{{ proto int hrtime()
   Returns an integer containing the current high-resolution time in nanoseconds
   counted from an arbitrary point in time */
PHP_FUNCTION(hrtime)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	// Use double to avoid integer overflows as we don't know the order of magnitude
	RETURN_LONG((uint64_t) ((double) 1000000000 * _timer_current() / _timer_freq));
}
/* }}} */
