/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 2013-2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file footprint_info.cpp
 */

/*
 * Functions to read footprint libraries and fill m_footprints by available footprints names
 * and their documentation (comments and keywords)
 */

#include <wx/wx.h>
#include <common.h>
#include <confirm.h>
#include <macros.h>
#include <pgm_base.h>
#include <wildcards_and_files_ext.h>
#include <footprint_info.h>
#include <io_mgr.h>
#include <fp_lib_table.h>
#include <fpid.h>
#include <class_module.h>
#include <boost/thread.hpp>
#include <html_messagebox.h>


void FOOTPRINT_INFO::load()
{
    FP_LIB_TABLE*   fptable = m_owner->GetTable();

    wxASSERT( fptable );

    std::auto_ptr<MODULE> m( fptable->FootprintLoad( m_nickname, m_fpname ) );

    if( m.get() == NULL )    // Should happen only with malformed/broken libraries
    {
        m_pad_count = 0;
        m_unique_pad_count = 0;
    }
    else
    {
        m_pad_count = m->GetPadCount( DO_NOT_INCLUDE_NPTH );
        m_unique_pad_count = m->GetUniquePadCount( DO_NOT_INCLUDE_NPTH );
        m_keywords  = m->GetKeywords();
        m_doc       = m->GetDescription();

        // tell ensure_loaded() I'm loaded.
        m_loaded = true;
    }
}


void FOOTPRINT_LIST::loader_job( const wxString* aNicknameList, int aJobZ )
{
    for( int i=0; i<aJobZ; ++i )
    {
        const wxString& nickname = aNicknameList[i];

        try
        {
            wxArrayString fpnames = m_lib_table->FootprintEnumerate( nickname );

            for( unsigned ni=0;  ni<fpnames.GetCount();  ++ni )
            {
                FOOTPRINT_INFO* fpinfo = new FOOTPRINT_INFO( this, nickname, fpnames[ni] );

                addItem( fpinfo );
            }
        }
        catch( const PARSE_ERROR& pe )
        {
            // m_errors.push_back is not thread safe, lock its MUTEX.
            MUTLOCK lock( m_errors_lock );

            ++m_error_count;        // modify only under lock
            m_errors.push_back( new IO_ERROR( pe ) );
        }
        catch( const IO_ERROR& ioe )
        {
            MUTLOCK lock( m_errors_lock );

            ++m_error_count;
            m_errors.push_back( new IO_ERROR( ioe ) );
        }

        // Catch anything unexpected and map it into the expected.
        // Likely even more important since this function runs on GUI-less
        // worker threads.
        catch( const std::exception& se )
        {
            // This is a round about way to do this, but who knows what THROW_IO_ERROR()
            // may be tricked out to do someday, keep it in the game.
            try
            {
                THROW_IO_ERROR( se.what() );
            }
            catch( const IO_ERROR& ioe )
            {
                MUTLOCK lock( m_errors_lock );

                ++m_error_count;
                m_errors.push_back( new IO_ERROR( ioe ) );
            }
        }
    }
}


bool FOOTPRINT_LIST::ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname )
{
    bool retv = true;

    m_lib_table = aTable;

    // Clear data before reading files
    m_error_count = 0;
    m_errors.clear();
    m_list.clear();

    if( aNickname )
    {
        // single footprint
        loader_job( aNickname, 1 );
    }
    else
    {
        std::vector< wxString > nicknames;

        // do all of them
        nicknames = aTable->GetLogicalLibs();

        loader_job( &nicknames[0], nicknames.size() );

        m_list.sort();
    }

    // The result of this function can be a blend of successes and failures, whose
    // mix is given by the Count()s of the two lists.  The return value indicates whether
    // an abort occurred, even true does not necessarily mean full success, although
    // false definitely means failure.

    return retv;
}


FOOTPRINT_INFO* FOOTPRINT_LIST::GetModuleInfo( const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
        return NULL;

    for( FOOTPRINT_INFO& fp : m_list )
    {
        FPID fpid;

        wxCHECK_MSG( fpid.Parse( aFootprintName ) < 0, NULL,
                     wxString::Format( wxT( "'%s' is not a valid FPID." ),
                                       GetChars( aFootprintName ) ) );

        wxString libNickname   = fpid.GetLibNickname();
        wxString footprintName = fpid.GetFootprintName();

        if( libNickname == fp.GetNickname() && footprintName == fp.GetFootprintName() )
            return &fp;
    }

    return NULL;
}


bool FOOTPRINT_INFO::InLibrary( const wxString& aLibrary ) const
{
    return aLibrary == m_nickname;
}


void FOOTPRINT_LIST::DisplayErrors( wxTopLevelWindow* aWindow )
{
    // @todo: go to a more HTML !<table>! ? centric output, possibly with
    // recommendations for remedy of errors.  Add numeric error codes
    // to PARSE_ERROR, and switch on them for remedies, etc.  Full
    // access is provided to everything in every exception!

    HTML_MESSAGE_BOX dlg( aWindow, _( "Load Error" ) );

    dlg.MessageSet( _( "Errors were encountered loading footprints:" ) );

    wxString msg;

    for( unsigned i = 0; i<m_errors.size();  ++i )
    {
        msg += wxT( "<p>" ) + m_errors[i].errorText + wxT( "</p>" );
    }

    dlg.AddHTML_Text( msg );

    dlg.ShowModal();
}