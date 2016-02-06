/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#include "common/file_writer.h"
#include "richio.h"

FILE_WRITER::FILE_WRITER( const wxString& aFileName, const wxChar* aMode) :
    m_filename( aFileName )
{
    m_fp = wxFopen( aFileName, aMode );

    if( !m_fp )
    {
        wxString msg = wxString::Format(
                            _( "cannot open or save file '%s'" ),
                            m_filename.GetData() );
        THROW_IO_ERROR( msg );
    }
}


FILE_WRITER::~FILE_WRITER()
{
    if( m_fp )
        fclose( m_fp );
}


void FILE_WRITER::Write( const char* aOutBuf, int aCount )
{
    if( 1 != fwrite( aOutBuf, aCount, 1, m_fp ) )
    {
        wxString msg = wxString::Format(
                            _( "error writing to file '%s'" ),
                            m_filename.GetData() );
        THROW_IO_ERROR( msg );
    }
}