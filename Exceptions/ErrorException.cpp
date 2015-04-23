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
 * \author Zurab Khadikov, Jedox AG, Freiburg, Germany
 * \author Ladislav Morva, qBicon s.r.o., Prague, Czech Republic
 * \author Martin Dolezal, qBicon s.r.o., Hradec Kralove, Czech Republic
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */


#include "Exceptions/ErrorException.h"

#include <string>


string ErrorException::getDescriptionErrorType(ErrorType type) {
  switch (type) {
    case ERROR_INTERNAL:
      return "internal error";
    case ERROR_INVALID_COORDINATES:
      return "invalid coordinates";
    case ERROR_CONVERSION_FAILED:
      return "conversion failed";
    case ERROR_FILE_NOT_FOUND_ERROR:
      return "file not found";
    case ERROR_CORRUPT_FILE:
      return "corrupt file";
    case ERROR_OUT_OF_MEMORY:
      return "not enough memory";
    case ERROR_PARAMETER_MISSING:
      return "missing parameter";
    case ERROR_COPY_FAILED:
      return "copy operation failed";
    case ERROR_DIMENSION_NAME_IN_USE:
      return "dimension name in use";
    case ERROR_ELEMENT_NAME_IN_USE:
      return "element name in use";
    case ERROR_INVALID_ELEMENT_NAME:
      return "invalid element name";
    case ERROR_CUBE_EMPTY:
      return "cube empty";
    case ERROR_INVALID_AGGR_FUNCTION:
      return "invalid aggregation function";
    case ERROR_SPLASH_DISABLED:
      return "splash disabled";
  }

  return "internal error in ErrorException::getDescriptionErrorType";
}

ErrorException::ErrorException(ErrorType type)
    : type(type) {
}

ErrorException::ErrorException(ErrorType type, const string& message)
    : type(type),
      message(message) {
}

ErrorException::~ErrorException() {
}

const string& ErrorException::getMessage() const {
  return message;
}

const string& ErrorException::getDetails() const {
  return details;
}

ErrorException::ErrorType ErrorException::getErrorType() const {
  return type;
}
