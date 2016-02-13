/**
 * @file class_worksheet_layout.cpp
 * @brief description of graphic items and texts to build a title block
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
 *
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


/*
 * the class WORKSHEET_DATAITEM (and derived ) defines
 * a basic shape of a page layout ( frame references and title block )
 * The list of these items is stored in a WORKSHEET_LAYOUT instance.
 *
 *
 * These items cannot be drawn or plot "as this". they should be converted
 * to a "draw list". When building the draw list:
 * the WORKSHEET_LAYOUT is used to create a WS_DRAW_ITEM_LIST
 *  coordinates are converted to draw/plot coordinates.
 *  texts are expanded if they contain format symbols.
 *  Items with m_RepeatCount > 1 are created m_RepeatCount times
 *
 * the WORKSHEET_LAYOUT is created only once.
 * the WS_DRAW_ITEM_LIST is created each time the page layout is plot/drawn
 *
 * the WORKSHEET_LAYOUT instance is created from a S expression which
 * describes the page layout (can be the default page layout or a custom file).
 */

#include <memory>

#include <wx/filename.h>
#include <wx/string.h>
#include <kiface_i.h>
#include <drawtxt.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <worksheet_shape_builder.h>
#include "page_layout/worksheet_layout.h"
#include "common/file_writer.h"
#include "sexpr/sexpr_parser.h"
#include "sexpr/sexpr_syntax_exception.h"


// The layout shape used in the application
// It is accessible by WORKSHEET_LAYOUT::GetTheInstance()
static WORKSHEET_LAYOUT wksTheInstance;
static WORKSHEET_LAYOUT* wksAltInstance;

WORKSHEET_LAYOUT::WORKSHEET_LAYOUT()
{
    m_allowVoidList = false;
    m_leftMargin = 10.0;    // the left page margin in mm
    m_rightMargin = 10.0;   // the right page margin in mm
    m_topMargin = 10.0;     // the top page margin in mm
    m_bottomMargin = 10.0;  // the bottom page margin in mm
}

/* static function: returns the instance of WORKSHEET_LAYOUT
 * used in the application
 */
WORKSHEET_LAYOUT& WORKSHEET_LAYOUT::GetTheInstance()
{
    if( wksAltInstance )
        return *wksAltInstance;
    else
        return wksTheInstance;
}

SEXPR::SEXPR* WORKSHEET_LAYOUT::SerializeSEXPR() const
{
    SEXPR::SEXPR_LIST* root = new SEXPR::SEXPR_LIST();
    *root << SEXPR::AsSymbol("page_layout");
    
    
    SEXPR::SEXPR_LIST* setup = new SEXPR::SEXPR_LIST();
    *setup << SEXPR::AsSymbol( "setup" );
    
    SEXPR::SEXPR_LIST* textsize = new SEXPR::SEXPR_LIST();
    *textsize << SEXPR::AsSymbol( "textsize" );
    *textsize << WORKSHEET_DATAITEM::m_DefaultTextSize.x;
    *textsize << WORKSHEET_DATAITEM::m_DefaultTextSize.y;
    
    SEXPR::SEXPR_LIST* linewidth = new SEXPR::SEXPR_LIST();
    *linewidth << SEXPR::AsSymbol( "linewidth" );
    *linewidth << WORKSHEET_DATAITEM::m_DefaultLineWidth;
    
    SEXPR::SEXPR_LIST* textlinewidth = new SEXPR::SEXPR_LIST();
    *textlinewidth << SEXPR::AsSymbol( "textlinewidth" );
    *textlinewidth << WORKSHEET_DATAITEM::m_DefaultTextThickness;
    
    SEXPR::SEXPR_LIST* left = new SEXPR::SEXPR_LIST();
    *left << SEXPR::AsSymbol( "left_margin" );
    *left << GetLeftMargin();
    
    SEXPR::SEXPR_LIST* right = new SEXPR::SEXPR_LIST();
    *right << SEXPR::AsSymbol( "right_margin" );
    *right << GetRightMargin();
    
    SEXPR::SEXPR_LIST* top = new SEXPR::SEXPR_LIST();
    *top << SEXPR::AsSymbol( "top_margin" );
    *top << GetTopMargin();
    
    SEXPR::SEXPR_LIST* bottom = new SEXPR::SEXPR_LIST();
    *bottom << SEXPR::AsSymbol( "bottom_margin" );
    *bottom << GetBottomMargin();
    
    *setup << textsize;
    *setup << linewidth;
    *setup << textlinewidth;
    *setup << left;
    *setup << right;
    *setup << top;
    *setup << bottom;
    
    *root << setup;
        // Save the graphical items on the page layout
    for( unsigned ii = 0; ii < GetCount(); ii++ )
    {
        WORKSHEET_DATAITEM* item = GetItem( ii );
        *root << *item;
    }
    
    return root;
}

/**
 * static function: Set an alternate instance of WORKSHEET_LAYOUT
 * mainly used in page setting dialog
 * @param aLayout = the alternate page layout.
 * if null, restore the basic page layout
 */
void WORKSHEET_LAYOUT::SetAltInstance( WORKSHEET_LAYOUT* aLayout )
{
    wksAltInstance = aLayout;
}


void WORKSHEET_LAYOUT::SetLeftMargin( double aMargin )
{
    m_leftMargin = aMargin;    // the left page margin in mm
}


void WORKSHEET_LAYOUT::SetRightMargin( double aMargin )
{
    m_rightMargin = aMargin;   // the right page margin in mm
}


void WORKSHEET_LAYOUT::SetTopMargin( double aMargin )
{
    m_topMargin = aMargin;     // the top page margin in mm
}


void WORKSHEET_LAYOUT::SetBottomMargin( double aMargin )
{
    m_bottomMargin = aMargin;  // the bottom page margin in mm
}


void WORKSHEET_LAYOUT::ClearList()
{
    for( unsigned ii = 0; ii < m_list.size(); ii++ )
        delete m_list[ii];
    m_list.clear();
}


void WORKSHEET_LAYOUT::Insert( WORKSHEET_DATAITEM* aItem, unsigned aIdx )
{
    if ( aIdx >= GetCount() )
        Append( aItem );
    else
        m_list.insert(  m_list.begin() + aIdx, aItem );
}


bool WORKSHEET_LAYOUT::Remove( unsigned aIdx )
{
    if ( aIdx >= GetCount() )
        return false;
    m_list.erase( m_list.begin() + aIdx );
    return true;
}


bool WORKSHEET_LAYOUT::Remove( WORKSHEET_DATAITEM* aItem )
{
    unsigned idx = 0;

    while( idx < m_list.size() )
    {
        if( m_list[idx] == aItem )
            break;

        idx++;
    }

    return Remove( idx );
}


int WORKSHEET_LAYOUT::GetItemIndex( WORKSHEET_DATAITEM* aItem ) const
{
    unsigned idx = 0;
    while( idx < m_list.size() )
    {
        if( m_list[idx] == aItem )
            return (int) idx;

        idx++;
    }

    return -1;
}


/* return the item from its index aIdx, or NULL if does not exist
 */
WORKSHEET_DATAITEM* WORKSHEET_LAYOUT::GetItem( unsigned aIdx ) const
{
    if( aIdx < m_list.size() )
        return m_list[aIdx];
    else
        return NULL;
}


const wxString WORKSHEET_LAYOUT::MakeShortFileName( const wxString& aFullFileName,
                                                    const wxString& aProjectPath  )
{
    wxString    shortFileName = aFullFileName;
    wxFileName  fn = aFullFileName;

    if( fn.IsRelative() )
        return shortFileName;

    if( ! aProjectPath.IsEmpty() && aFullFileName.StartsWith( aProjectPath ) )
    {
        fn.MakeRelativeTo( aProjectPath );
        shortFileName = fn.GetFullPath();
        return shortFileName;
    }

    wxString    fileName = Kiface().KifaceSearch().FindValidPath( fn.GetFullName() );

    if( !fileName.IsEmpty() )
    {
        fn = fileName;
        shortFileName = fn.GetFullName();
        return shortFileName;
    }

    return shortFileName;
}


const wxString WORKSHEET_LAYOUT::MakeFullFileName( const wxString& aShortFileName,
                                                   const wxString& aProjectPath )
{
    wxString    fullFileName = ExpandEnvVarSubstitutions( aShortFileName );

    if( fullFileName.IsEmpty() )
        return fullFileName;

    wxFileName fn = fullFileName;

    if( fn.IsAbsolute() )
        return fullFileName;

    // the path is not absolute: search it in project path, and then in
    // kicad valid paths
    if( !aProjectPath.IsEmpty() )
    {
        fn.MakeAbsolute( aProjectPath );

        if( wxFileExists( fn.GetFullPath() ) )
            return fn.GetFullPath();
    }

    fn = fullFileName;
    wxString name = Kiface().KifaceSearch().FindValidPath( fn.GetFullName() );

    if( !name.IsEmpty() )
        fullFileName = name;

    return fullFileName;
}


/* Save the description in a buffer
 */
void WORKSHEET_LAYOUT::SaveInString( wxString& aOutputString )
{
    std::unique_ptr<SEXPR::SEXPR> sexpr( SerializeSEXPR() );
    aOutputString = wxString( sexpr->AsString() );
}

/*
 * Save the description in a file
 */
void WORKSHEET_LAYOUT::Save( const wxString& aFullFileName )
{
    std::unique_ptr<SEXPR::SEXPR> sexpr( SerializeSEXPR() );
    
    try
    {
        FILE_WRITER f( aFullFileName );
        std::string sexprString = sexpr->AsString();
        f.Write( sexprString.c_str(), sexprString.length() );
    }
    catch( const IO_ERROR& ioe )
    {
        wxMessageBox( ioe.errorText, _("Error writing page layout descr file" ) );
    }
}

// defaultPageLayout is the default page layout description
// using the S expr.
// see page_layout_default_shape.cpp
extern const char defaultPageLayout[];

void WORKSHEET_LAYOUT::SetDefaultLayout()
{
    ClearList();
    
    try
    {
        Parse( defaultPageLayout );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogMessage( ioe.errorText );
    }
}

void WORKSHEET_LAYOUT::Parse( std::string layout )
{
    SEXPR::PARSER parser;
    SEXPR::SEXPR* parsedRoot = parser.Parse( layout );
    
    if( !parsedRoot->IsList() )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), parsedRoot->GetLineNumber() );
    }

    std::unique_ptr<SEXPR::SEXPR_LIST> list( parsedRoot->GetList() );

    DeserializeSEXPR( *list );
}

void WORKSHEET_LAYOUT::deserializeSetup( SEXPR::SEXPR& root )
{
    if( !root.IsList() )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), root.GetLineNumber() );
    }

    for(size_t i = 1; i < root.GetNumberOfChildren(); i++ )
    {
        SEXPR::SEXPR* child = root.GetChild(i);

        if( !child->IsList() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), child->GetLineNumber() );
        }
        
        SEXPR::SEXPR_LIST* childList = child->GetList();
        
        if( !childList->GetChild(0)->IsSymbol() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("symbol not found"), childList->GetChild(0)->GetLineNumber() );
        }

        std::string sym = childList->GetChild(0)->GetSymbol();
        
        if( sym == "textsize" )
        {
            WORKSHEET_DATAITEM::m_DefaultTextSize.x = childList->GetChild(1)->GetDouble();
            WORKSHEET_DATAITEM::m_DefaultTextSize.y = childList->GetChild(2)->GetDouble();
        }
        else if( sym == "linewidth" )
        {
            WORKSHEET_DATAITEM::m_DefaultLineWidth = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "textlinewidth" )
        {
            WORKSHEET_DATAITEM::m_DefaultTextThickness = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "left_margin" )
        {
            SetLeftMargin( childList->GetChild(1)->GetDouble() );
        }
        else if( sym == "right_margin" )
        {
            SetRightMargin( childList->GetChild(1)->GetDouble() );
        }
        else if( sym == "top_margin" )
        {
            SetTopMargin( childList->GetChild(1)->GetDouble() );
        }
        else if( sym == "bottom_margin" )
        {
            SetBottomMargin( childList->GetChild(1)->GetDouble() );
        }
    }
}



void WORKSHEET_LAYOUT::DeserializeSEXPR( SEXPR::SEXPR& root )
{
    if( !root.GetChild(0)->IsSymbol() ||  root.GetChild(0)->GetSymbol() != "page_layout")
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("page_layout symbol not found"), root.GetChild(0)->GetLineNumber() );
    }

    for(size_t i = 1; i < root.GetNumberOfChildren(); i++ )
    {
        SEXPR::SEXPR* child = root.GetChild(i);

        if( !child->IsList() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), child->GetLineNumber() );
        }

        SEXPR::SEXPR_LIST* childList = child->GetList();
        
        if( !childList->GetChild(0)->IsSymbol() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("symbol not found"), childList->GetChild(0)->GetLineNumber() );
        }

        std::string sym = childList->GetChild(0)->GetSymbol();

        WORKSHEET_DATAITEM * item = nullptr;
        if( sym == "setup" )
        {
            
        }
        else if( sym == "rect" )
        {
            item = new WORKSHEET_DATAITEM( WORKSHEET_DATAITEM::WS_RECT );
        }
        else if( sym == "line" )
        {
            item = new WORKSHEET_DATAITEM( WORKSHEET_DATAITEM::WS_SEGMENT );
        }
        else if( sym == "tbtext" )
        {
            item = new WORKSHEET_DATAITEM_TEXT( "" );
        }
        else if( sym == "bitmap" )
        {
            item = new WORKSHEET_DATAITEM_BITMAP( NULL );
        }
        else if( sym == "polygon" )
        {
            item = new WORKSHEET_DATAITEM_POLYPOLYGON();
        }

        if( item != nullptr )
        {
            item->DeserializeSEXPR( *childList );
            Append( item );
        }
    }
}

/**
 * Populates the list from a S expr description stored in a string
 * @param aPageLayout = the S expr string
 */
void WORKSHEET_LAYOUT::SetPageLayout( const char* aPageLayout, bool Append )
{
    if( ! Append )
        ClearList();

    try
    {
        Parse( std::string( aPageLayout ) );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogMessage( ioe.errorText );
    }
}

// SetLayout() try to load the aFullFileName custom layout file,
// if aFullFileName is empty, try the filename defined by the
// environment variable KICAD_WKSFILE (a *.kicad_wks filename).
// if does not exists, loads the default page layout.
void WORKSHEET_LAYOUT::SetPageLayout( const wxString& aFullFileName, bool Append )
{
    if( !Append )
    {
        if( aFullFileName.IsEmpty() || !wxFileExists( aFullFileName ) )
        {
            SetDefaultLayout();
            return;
        }
    }
#if 0
    wxFile wksFile( aFullFileName );

    if( ! wksFile.IsOpened() )
    {
        if( !Append )
            SetDefaultLayout();
        return;
    }
#endif
    std::string layout = SEXPR::PARSER::GetFileContents( aFullFileName.ToStdString() );

    if( ! Append )
        ClearList();

    try
    {
        Parse( layout );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogMessage( ioe.errorText );
    }
}