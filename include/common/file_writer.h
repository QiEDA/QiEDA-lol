/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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

#ifndef FILE_WRITER_H_
#define FILE_WRITER_H_

#include <wx/string.h>

class FILE_WRITER
{
public:

    /**
     * Constructor
     * @param aFileName is the full filename to open and save to as a text file.
     * @param aMode is what you would pass to wxFopen()'s mode, defaults to wxT( "wt" )
     *      for text files that are to be created here and now.
     * @param aQuoteChar is a char used for quoting problematic strings
            (with whitespace or special characters in them).
     * @throw IO_ERROR if the file cannot be opened.
     */
    FILE_WRITER( const wxString& aFileName, const wxChar* aMode = wxT( "wt" ) );
    ~FILE_WRITER();

    void Write( const char* aOutBuf, int aCount );
private:
    FILE*       m_fp;               ///< takes ownership
    wxString    m_filename;
};


#endif