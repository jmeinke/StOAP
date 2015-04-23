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
 * \author Martin Dolezal, qBicon s.r.o., Hradec Kralove, Czech Republic
 * \author Jerome Meinke, University of Freiburg, Germany
 *
 */


#ifndef STOAP_EXCEPTIONS_ERROREXCEPTION_H_
#define STOAP_EXCEPTIONS_ERROREXCEPTION_H_ 1

#include <string>

#include "Olap.h"


////////////////////////////////////////////////////////////////////////////////
/// @brief base class for error exceptions
////////////////////////////////////////////////////////////////////////////////

class ErrorException {
 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief error number
  ////////////////////////////////////////////////////////////////////////////////

  enum ErrorType {
    ERROR_INTERNAL = 2,

    // general error messages
    ERROR_INVALID_COORDINATES = 1006,
    ERROR_CONVERSION_FAILED = 1007,
    ERROR_FILE_NOT_FOUND_ERROR = 1008,
    ERROR_CORRUPT_FILE = 1010,
    ERROR_PARAMETER_MISSING = 1016,
    ERROR_OUT_OF_MEMORY = 1023,
    ERROR_INVALID_AGGR_FUNCTION = 1028,
    ERROR_COPY_FAILED = 1033,

    // dimension related errors
    ERROR_DIMENSION_NAME_IN_USE = 3005,

    // dimension element related errors
    ERROR_ELEMENT_NAME_IN_USE = 4002,
    ERROR_INVALID_ELEMENT_NAME = 4006,

    // cube related errors
    ERROR_CUBE_EMPTY = 5003,
    ERROR_SPLASH_DISABLED = 5005
  };

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief descriptive text for error number
  ////////////////////////////////////////////////////////////////////////////////

  static string getDescriptionErrorType(ErrorType type);

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief constructor
  ////////////////////////////////////////////////////////////////////////////////

  explicit ErrorException(ErrorType type);

  ErrorException(ErrorType type, const string& message);

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief destructor
  ////////////////////////////////////////////////////////////////////////////////

  virtual ~ErrorException();

 public:
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief get error message
  ////////////////////////////////////////////////////////////////////////////////

  virtual const string& getMessage() const;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief get error details
  ////////////////////////////////////////////////////////////////////////////////

  virtual const string& getDetails() const;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief get error type
  ////////////////////////////////////////////////////////////////////////////////

  virtual ErrorType getErrorType() const;

 protected:
  ErrorType type;
  string message;
  string details;
};

#endif  // STOAP_EXCEPTIONS_ERROREXCEPTION_H_
