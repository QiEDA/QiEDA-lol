/*
 * This program source code file is part of QiEDA
 *
 * Copyright (C) 2016 QiEDA Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SEXPR_SYNTAX_EXCEPTION_H_
#define SEXPR_SYNTAX_EXCEPTION_H_

#include "richio.h"

#define SEXPR_SYNTAX_EXCEPTION_FORMAT    _( "PARSE_ERROR: %s on line %d" )

/**
 * Struct FILE_SYNTAX_EXCEPTION
 */
struct SEXPR_SYNTAX_EXCEPTION : public IO_ERROR
{
    int         lineNumber;     ///< at which line number, 1 based index.

    /**
     * Constructor
     * which is normally called via the macro THROW_PARSE_ERROR so that
     * __FILE__ and __LOC__ can be captured from the call site.
     */
    SEXPR_SYNTAX_EXCEPTION( const char* aThrowersFile, const char* aThrowersLoc,
                 const wxString& aMsg, int aLineNumber ) : IO_ERROR()
    {
        init( aThrowersFile, aThrowersLoc, aMsg, aLineNumber );
    }

    void init( const char* aThrowersFile, const char* aThrowersLoc,
               const wxString& aMsg, int aLineNumber )
    {
        // save inpuLine, lineNumber, and offset for UI (.e.g. Sweet text editor)
        lineNumber = aLineNumber;

        errorText.Printf( SEXPR_SYNTAX_EXCEPTION_FORMAT, aMsg.GetData(), aLineNumber );
    }

    ~SEXPR_SYNTAX_EXCEPTION() throw ( /*none*/ ){}
};


#define THROW_SEXPR_SYNTAX_EXCEPTION( aMsg, aLineNumber )  \
        throw SEXPR_SYNTAX_EXCEPTION( __FILE__, __LOC__, aMsg, aLineNumber )
        
#endif // SEXPR_SYNTAX_EXCEPTION_H_