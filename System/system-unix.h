/* 
 *
 * Copyright (C) 2006-2015 Jedox AG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as published
 * by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * If you are developing and distributing open source applications under the
 * GPL License, then you are free to use Palo under the GPL License.  For OEMs,
 * ISVs, and VARs who distribute Palo with their products, and do not license
 * and distribute their source code under the GPL, Jedox provides a flexible
 * OEM Commercial License.
 *
 * \author Frank Celler, triagens GmbH, Cologne, Germany
 * \author Achim Brandt, triagens GmbH, Cologne, Germany
 * \author Martin Jakl, qBicon s.r.o., Prague, Czech Republic
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */

#ifndef STOAP_SYSTEM_SYSTEM_UNIX_H_
#define STOAP_SYSTEM_SYSTEM_UNIX_H_ 1

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief type of signal
////////////////////////////////////////////////////////////////////////////////

typedef int signal_t;

////////////////////////////////////////////////////////////////////////////////
/// @brief noinline keyword for GCC
////////////////////////////////////////////////////////////////////////////////
#define __noinline __attribute__ ((noinline))

////////////////////////////////////////////////////////////////////////////////
/// @brief not a number
////////////////////////////////////////////////////////////////////////////////

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5) || defined(__clang__)
#define isnanLocal std::isnan
#else
#define isnanLocal isnan
#endif /* __GNUC__> 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5) */

////////////////////////////////////////////////////////////////////////////////
/// @brief deprecated keyword for GCC
////////////////////////////////////////////////////////////////////////////////
#define __deprecated __attribute__ ((deprecated))
#define __deprecated_msg(msg) __attribute__ ((deprecated (msg)))

#endif  // STOAP_SYSTEM_SYSTEM_UNIX_H_
