/**
 * @file class_worksheet_dataitem.cpp
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
 * the class WORKSHEET_DATAITEM (and derived) defines
 * a basic shape of a page layout ( frame references and title block )
 * Basic shapes are line, rect and texts
 * the WORKSHEET_DATAITEM coordinates units is the mm, and are relative to
 * one of 4 page corners.
 *
 * These items cannot be drawn or plot "as this". they should be converted
 * to a "draw list" (WS_DRAW_ITEM_BASE and derived items)

 * The list of these items is stored in a WORKSHEET_LAYOUT instance.
 *
 * When building the draw list:
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

#include <wx/chartype.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <drawtxt.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <worksheet_shape_builder.h>
#include "page_layout/worksheet_dataitem.h"
#include "sexpr/sexpr_syntax_exception.h"


// Static members of class WORKSHEET_DATAITEM:
double WORKSHEET_DATAITEM::m_WSunits2Iu = 1.0;
DPOINT WORKSHEET_DATAITEM::m_RB_Corner;
DPOINT WORKSHEET_DATAITEM::m_LT_Corner;
double WORKSHEET_DATAITEM::m_DefaultLineWidth = 0.0;
DSIZE  WORKSHEET_DATAITEM::m_DefaultTextSize( TB_DEFAULT_TEXTSIZE, TB_DEFAULT_TEXTSIZE );
double WORKSHEET_DATAITEM::m_DefaultTextThickness = 0.0;
bool WORKSHEET_DATAITEM::m_SpecialMode = false;
EDA_COLOR_T WORKSHEET_DATAITEM::m_Color = RED;              // the default color to draw items
EDA_COLOR_T WORKSHEET_DATAITEM::m_AltColor = RED;           // an alternate color to draw items
EDA_COLOR_T WORKSHEET_DATAITEM::m_SelectedColor = BROWN;   // the color to draw selected items

// The constructor:
WORKSHEET_DATAITEM::WORKSHEET_DATAITEM( WS_ItemType aType )
{
    m_type = aType;
    m_flags = 0;
    m_RepeatCount = 1;
    m_IncrementLabel = 1;
    m_LineWidth = 0;
}

// move item to aPosition
// starting point is moved to aPosition
// the Ending point is moved to a position which keeps the item size
// (if both coordinates have the same corner reference)
// MoveToUi and MoveTo takes the graphic position (i.e relative to the left top
// paper corner
void WORKSHEET_DATAITEM::MoveToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / m_WSunits2Iu;
    pos_mm.y = aPosition.y / m_WSunits2Iu;

    MoveTo( pos_mm );
}

void WORKSHEET_DATAITEM::MoveTo( DPOINT aPosition )
{
    DPOINT vector = aPosition - GetStartPos();
    DPOINT endpos = vector + GetEndPos();

    MoveStartPointTo( aPosition );
    MoveEndPointTo( endpos );
}

/* move the starting point of the item to a new position
 * aPosition = the new position of the starting point, in mm
 */
void WORKSHEET_DATAITEM::MoveStartPointTo( DPOINT aPosition )
{
    DPOINT position;

    // Calculate the position of the starting point
    // relative to the reference corner
    // aPosition is the position relative to the right top paper corner
    switch( m_Pos.m_Anchor )
    {
        case RB_CORNER:
            position = m_RB_Corner - aPosition;
            break;

        case RT_CORNER:
            position.x = m_RB_Corner.x - aPosition.x;
            position.y = aPosition.y - m_LT_Corner.y;
            break;

        case LB_CORNER:
            position.x = aPosition.x - m_LT_Corner.x;
            position.y = m_RB_Corner.y - aPosition.y;
            break;

        case LT_CORNER:
            position = aPosition - m_LT_Corner;
            break;
    }

    m_Pos.m_Pos = position;
}

/* move the starting point of the item to a new position
 * aPosition = the new position of the starting point in graphic units
 */
void WORKSHEET_DATAITEM::MoveStartPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / m_WSunits2Iu;
    pos_mm.y = aPosition.y / m_WSunits2Iu;

    MoveStartPointTo( pos_mm );
}

/**
 * move the ending point of the item to a new position
 * has meaning only for items defined by 2 points
 * (segments and rectangles)
 * aPosition = the new position of the ending point, in mm
 */
void WORKSHEET_DATAITEM::MoveEndPointTo( DPOINT aPosition )
{
    DPOINT position;

    // Calculate the position of the starting point
    // relative to the reference corner
    // aPosition is the position relative to the right top paper corner
    switch( m_End.m_Anchor )
    {
        case RB_CORNER:
            position = m_RB_Corner - aPosition;
            break;

        case RT_CORNER:
            position.x = m_RB_Corner.x - aPosition.x;
            position.y = aPosition.y - m_LT_Corner.y;
            break;

        case LB_CORNER:
            position.x = aPosition.x - m_LT_Corner.x;
            position.y = m_RB_Corner.y - aPosition.y;
            break;

        case LT_CORNER:
            position = aPosition - m_LT_Corner;
            break;
    }

    // Modify m_End only for items having 2 coordinates
    switch( GetType() )
    {
        case WS_SEGMENT:
        case WS_RECT:
            m_End.m_Pos = position;
            break;

        default:
            break;
    }
}

/* move the ending point of the item to a new position
 * has meaning only for items defined by 2 points
 * (segments and rectangles)
 * aPosition = the new position of the ending point in graphic units
 */
void WORKSHEET_DATAITEM::MoveEndPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / m_WSunits2Iu;
    pos_mm.y = aPosition.y / m_WSunits2Iu;

    MoveEndPointTo( pos_mm );
}

const DPOINT WORKSHEET_DATAITEM::GetStartPos( int ii ) const
{
    DPOINT pos;
    pos.x = m_Pos.m_Pos.x + ( m_IncrementVector.x * ii );
    pos.y = m_Pos.m_Pos.y + ( m_IncrementVector.y * ii );

    switch( m_Pos.m_Anchor )
    {
        case RB_CORNER:      // right bottom corner
            pos = m_RB_Corner - pos;
            break;

        case RT_CORNER:      // right top corner
            pos.x = m_RB_Corner.x - pos.x;
            pos.y = m_LT_Corner.y + pos.y;
            break;

        case LB_CORNER:      // left bottom corner
            pos.x = m_LT_Corner.x + pos.x;
            pos.y = m_RB_Corner.y - pos.y;
            break;

        case LT_CORNER:      // left top corner
            pos = m_LT_Corner + pos;
            break;
    }

    return pos;
}

const wxPoint WORKSHEET_DATAITEM::GetStartPosUi( int ii ) const
{
    DPOINT pos = GetStartPos( ii );
    pos = pos * m_WSunits2Iu;
    return wxPoint( KiROUND(pos.x), KiROUND(pos.y) );
}

const DPOINT WORKSHEET_DATAITEM::GetEndPos( int ii ) const
{
    DPOINT pos;
    pos.x = m_End.m_Pos.x + ( m_IncrementVector.x * ii );
    pos.y = m_End.m_Pos.y + ( m_IncrementVector.y * ii );
    switch( m_End.m_Anchor )
    {
        case RB_CORNER:      // right bottom corner
            pos = m_RB_Corner - pos;
            break;

        case RT_CORNER:      // right top corner
            pos.x = m_RB_Corner.x - pos.x;
            pos.y = m_LT_Corner.y + pos.y;
            break;

        case LB_CORNER:      // left bottom corner
            pos.x = m_LT_Corner.x + pos.x;
            pos.y = m_RB_Corner.y - pos.y;
            break;

        case LT_CORNER:      // left top corner
            pos = m_LT_Corner + pos;
            break;
    }

    return pos;
}

const wxPoint WORKSHEET_DATAITEM::GetEndPosUi( int ii ) const
{
    DPOINT pos = GetEndPos( ii );
    pos = pos * m_WSunits2Iu;
    return wxPoint( KiROUND(pos.x), KiROUND(pos.y) );
}


bool WORKSHEET_DATAITEM::IsInsidePage( int ii ) const
{
    DPOINT pos = GetStartPos( ii );

    for( int kk = 0; kk < 1; kk++ )
    {
        if( m_RB_Corner.x < pos.x || m_LT_Corner.x > pos.x )
            return false;

        if( m_RB_Corner.y < pos.y || m_LT_Corner.y > pos.y )
            return false;

        pos = GetEndPos( ii );
    }

    return true;
}

const wxString WORKSHEET_DATAITEM::GetClassName() const
{
    wxString name;
    switch( GetType() )
    {
        case WS_TEXT: name = wxT("Text"); break;
        case WS_SEGMENT: name = wxT("Line"); break;
        case WS_RECT: name = wxT("Rect"); break;
        case WS_POLYPOLYGON: name = wxT("Poly"); break;
        case WS_BITMAP: name = wxT("Bitmap"); break;
    }

    return name;
}

/* return 0 if the item has no specific option for page 1
 * 1  if the item is only on page 1
 * -1  if the item is not on page 1
 */
int WORKSHEET_DATAITEM::GetPage1Option() const
{
    if(( m_flags & PAGE1OPTION) == PAGE1OPTION_NOTONPAGE1 )
        return -1;

    if(( m_flags & PAGE1OPTION) == PAGE1OPTION_PAGE1ONLY )
        return 1;

    return 0;
}

/* Set the option for page 1
 * aChoice = 0 if the item has no specific option for page 1
 * > 0  if the item is only on page 1
 * < 0  if the item is not on page 1
 */
void WORKSHEET_DATAITEM::SetPage1Option( int aChoice )
{
    ClearFlags( PAGE1OPTION );

    if( aChoice > 0)
        SetFlags( PAGE1OPTION_PAGE1ONLY );

    else if( aChoice < 0)
        SetFlags( PAGE1OPTION_NOTONPAGE1 );

}


SEXPR::SEXPR* WORKSHEET_DATAITEM::SerializeSEXPR() const
{
    SEXPR::SEXPR_LIST* root = new SEXPR::SEXPR_LIST();
    
    if( GetType() == WORKSHEET_DATAITEM::WS_RECT )
        *root << SEXPR::AsSymbol("rect");
    else
        *root << SEXPR::AsSymbol("line");

    
    SEXPR::SEXPR_LIST* name = new SEXPR::SEXPR_LIST();
    *name << SEXPR::AsSymbol( "name" );
    *name << m_Name.ToStdString();
    
    *root << name;
    
    *root << serializeSEXPRCoordinate( "start", m_Pos );
    *root << serializeSEXPRCoordinate( "end", m_End );
    

    if( m_LineWidth && m_LineWidth != m_DefaultLineWidth )
    {
        SEXPR::SEXPR_LIST* linewidth = new SEXPR::SEXPR_LIST();
        *linewidth << SEXPR::AsSymbol( "linewidth" );
        *linewidth << m_LineWidth;
        *root << linewidth;
    }
    
    serializeSEXPRRepeatParameters(root);
    
    return root;
}

SEXPR::SEXPR* WORKSHEET_DATAITEM::serializeSEXPRCoordinate( const std::string aToken, const POINT_COORD & aCoord ) const
{
    SEXPR::SEXPR_LIST* coord = new SEXPR::SEXPR_LIST();
    *coord << SEXPR::AsSymbol( aToken );
    *coord << aCoord.m_Pos.x;
    *coord << aCoord.m_Pos.y;

    switch( aCoord.m_Anchor )
    {
        case RB_CORNER:
            break;

        case LT_CORNER:
            *coord << SEXPR::AsSymbol("ltcorner");
            break;

        case LB_CORNER:
            *coord << SEXPR::AsSymbol("lbcorner");
            break;

        case RT_CORNER:
            *coord << SEXPR::AsSymbol("rtcorner");
            break;
    }

    return coord;
}

void WORKSHEET_DATAITEM::serializeSEXPRRepeatParameters( SEXPR::SEXPR_LIST* root ) const
{
    if( m_RepeatCount <= 1 )
        return;
    
    SEXPR::SEXPR_LIST* repeat = new SEXPR::SEXPR_LIST();
    *repeat << SEXPR::AsSymbol("repeat");
    *repeat << m_RepeatCount;
    
    *root << repeat;
    
    if( m_IncrementVector.x )
    {
        SEXPR::SEXPR_LIST* incrx = new SEXPR::SEXPR_LIST();
        *incrx << SEXPR::AsSymbol("incrx");
        *incrx << m_IncrementVector.x;
        *root << incrx;
    }
    
    if( m_IncrementVector.y )
    {
        SEXPR::SEXPR_LIST* incry = new SEXPR::SEXPR_LIST();
        *incry << SEXPR::AsSymbol("incry");
        *incry << m_IncrementVector.y;
        *root << incry;
    }

    if( m_IncrementLabel != 1 && GetType() == WORKSHEET_DATAITEM::WS_TEXT )
    {
        SEXPR::SEXPR_LIST* incrlabel = new SEXPR::SEXPR_LIST();
        *incrlabel << SEXPR::AsSymbol("incrlabel");
        *incrlabel << m_IncrementLabel;
        *root << incrlabel;
    }
}

void WORKSHEET_DATAITEM::serializeSEXPROptions( SEXPR::SEXPR_LIST* root ) const
{   
    switch( GetPage1Option() )
    {
        default:
        case 0:
            break;

        case 1:
            {
                SEXPR::SEXPR_LIST* option = new SEXPR::SEXPR_LIST();
                *option << SEXPR::AsSymbol("option");
                *option << SEXPR::AsSymbol("page1only");
                *root << option;
            }
            break;

        case -1:
            {
                SEXPR::SEXPR_LIST* option = new SEXPR::SEXPR_LIST();
                *option << SEXPR::AsSymbol("option");
                *option << SEXPR::AsSymbol("notonpage1");
                *root << option;
            }
            break;
    }
}

void WORKSHEET_DATAITEM::DeserializeSEXPR( SEXPR::SEXPR& root )
{
    if( !root.GetChild(0)->IsSymbol() )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("page_layout symbol not found"), root.GetChild(0)->GetLineNumber() );
    } 
    
    std::string rootSym = root.GetChild(0)->GetSymbol();
    if( rootSym != "rect" && rootSym != "line" )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("rect or line symbol not found"), root.GetChild(0)->GetLineNumber() );
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

        if( sym == "comment" )
        {
            //kicad compat
            if( childList->GetChild(1)->IsSymbol() )
            {
                m_Info = childList->GetChild(1)->GetSymbol();
            }
            else if( childList->GetChild(1)->IsString() )
            {
                m_Info = childList->GetChild(1)->GetString();
            }
        }
        else if( sym == "name" )
        {
            //kicad compat
            if( childList->GetChild(1)->IsSymbol() )
            {
                m_Name = childList->GetChild(1)->GetSymbol();
            }
            else if( childList->GetChild(1)->IsString() )
            {
                m_Name = childList->GetChild(1)->GetString();
            }
        }
        else if( sym == "option" )
        {
            deserializeSEXPROption( childList );
        }
        else if( sym == "start" )
        {
            deserializeSEXPRCoordinate( childList, m_Pos );
        }
        else if( sym == "end" )
        {
            deserializeSEXPRCoordinate( childList, m_End );
        }
        else if( sym == "repeat" )
        {
            m_RepeatCount = childList->GetChild(1)->GetInteger();
        }
        else if( sym == "incrx" )
        {
            m_IncrementVector.x = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "incry" )
        {
            m_IncrementVector.y = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "linewidth" )
        {
            m_LineWidth = childList->GetChild(1)->GetDouble();
        }
    }
}


void WORKSHEET_DATAITEM::deserializeSEXPRCoordinate( SEXPR::SEXPR_LIST* list, POINT_COORD & aCoord )
{
    //kicad compat
    aCoord.m_Pos.x = list->GetChild(1)->GetDouble();
    aCoord.m_Pos.y = list->GetChild(2)->GetDouble();

    if( list->GetNumberOfChildren() > 3 )
    {
        std::string token = list->GetChild(3)->GetSymbol();
        if( token == "ltcorner" )
        {
            aCoord.m_Anchor = LT_CORNER;   // left top corner
        }
        else if( token == "lbcorner" )
        {
            aCoord.m_Anchor = LB_CORNER;      // left bottom corner
        }
        else if( token == "rbcorner" )
        {
            aCoord.m_Anchor = RB_CORNER;      // right bottom corner
        }
        else if( token == "rtcorner" )
        {
            aCoord.m_Anchor = RT_CORNER;      // right top corner
        }
        else
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("unsupported symbol"), list->GetChild(3)->GetLineNumber() );
        }
    }
}


void WORKSHEET_DATAITEM::deserializeSEXPROption( SEXPR::SEXPR_LIST* list )
{
    for(size_t i = 1; i < list->GetNumberOfChildren(); i++ )
    {
        std::string token = list->GetChild(i)->GetSymbol();
        if( token == "page1only" )
        {
            SetPage1Option( 1 );
        }
        else if( token == "lbcorner" )
        {
            SetPage1Option( -1 );
        }
        else
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("unsupported symbol"), list->GetChild(i)->GetLineNumber() );
        }
    }
}


WORKSHEET_DATAITEM_POLYPOLYGON::WORKSHEET_DATAITEM_POLYPOLYGON() :
    WORKSHEET_DATAITEM( WS_POLYPOLYGON )
{
    m_Orient = 0.0;
}

const DPOINT WORKSHEET_DATAITEM_POLYPOLYGON::GetCornerPosition( unsigned aIdx,
                                                         int aRepeat ) const
{
    DPOINT pos = m_Corners[aIdx];

    // Rotation:
    RotatePoint( &pos.x, &pos.y, m_Orient * 10 );
    pos += GetStartPos( aRepeat );
    return pos;
}

void WORKSHEET_DATAITEM_POLYPOLYGON::SetBoundingBox()
{
    if( m_Corners.size() == 0 )
    {
        m_minCoord.x = m_maxCoord.x = 0.0;
        m_minCoord.y = m_maxCoord.y = 0.0;
        return;
    }

    DPOINT pos;
    pos = m_Corners[0];
    RotatePoint( &pos.x, &pos.y, m_Orient * 10 );
    m_minCoord = m_maxCoord = pos;

    for( unsigned ii = 1; ii < m_Corners.size(); ii++ )
    {
        pos = m_Corners[ii];
        RotatePoint( &pos.x, &pos.y, m_Orient * 10 );

        if( m_minCoord.x > pos.x )
            m_minCoord.x = pos.x;

        if( m_minCoord.y > pos.y )
            m_minCoord.y = pos.y;

        if( m_maxCoord.x < pos.x )
            m_maxCoord.x = pos.x;

        if( m_maxCoord.y < pos.y )
            m_maxCoord.y = pos.y;
    }
}

bool WORKSHEET_DATAITEM_POLYPOLYGON::IsInsidePage( int ii ) const
{
    DPOINT pos = GetStartPos( ii );
    pos += m_minCoord;  // left top pos of bounding box

    if( m_LT_Corner.x > pos.x || m_LT_Corner.y > pos.y )
        return false;

    pos = GetStartPos( ii );
    pos += m_maxCoord;  // rignt bottom pos of bounding box

    if( m_RB_Corner.x < pos.x || m_RB_Corner.y < pos.y )
        return false;

    return true;
}

const wxPoint WORKSHEET_DATAITEM_POLYPOLYGON::GetCornerPositionUi( unsigned aIdx,
                                                            int aRepeat ) const
{
    DPOINT pos = GetCornerPosition( aIdx, aRepeat );
    pos = pos * m_WSunits2Iu;
    return wxPoint( int(pos.x), int(pos.y) );
}

SEXPR::SEXPR* WORKSHEET_DATAITEM_POLYPOLYGON::SerializeSEXPR() const
{
    SEXPR::SEXPR_LIST* root = new SEXPR::SEXPR_LIST();

    *root << SEXPR::AsSymbol("polygon");

    SEXPR::SEXPR_LIST* name = new SEXPR::SEXPR_LIST();
    *name << SEXPR::AsSymbol( "name" );
    *name << m_Name.ToStdString();

    *root << name;

    *root << serializeSEXPRCoordinate( "pos", m_Pos );


    serializeSEXPROptions(root);
    serializeSEXPRRepeatParameters(root);
    
    if( m_Orient )
    {
        SEXPR::SEXPR_LIST* rotate = new SEXPR::SEXPR_LIST();
        *rotate << SEXPR::AsSymbol( "rotate" );
        *rotate << m_Orient;
        *root << rotate;
    }

    if( m_LineWidth )
    {
        SEXPR::SEXPR_LIST* linewidth = new SEXPR::SEXPR_LIST();
        *linewidth << SEXPR::AsSymbol( "linewidth" );
        *linewidth << m_LineWidth;
        *root << linewidth;
    }

    for( int kk = 0; kk < GetPolyCount(); kk++ )
    {
        SEXPR::SEXPR_LIST* polylist = new SEXPR::SEXPR_LIST();
        *polylist << SEXPR::AsSymbol( "pts" );
        
        // Create current polygon corners list
        unsigned ist = GetPolyIndexStart( kk );
        unsigned iend = GetPolyIndexEnd( kk );
        
        while( ist <= iend )
        {
            DPOINT pos = m_Corners[ist++];
            
            SEXPR::SEXPR_LIST* xy = new SEXPR::SEXPR_LIST();
            *xy << SEXPR::AsSymbol( "xy" );
            *xy << pos.x;
            *xy << pos.y;
            *polylist << xy;
        }
        
        *root << polylist;
    }
    
    return root;
}

void WORKSHEET_DATAITEM_POLYPOLYGON::DeserializeSEXPR( SEXPR::SEXPR& root )
{
    if( !root.GetChild(0)->IsSymbol() )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("polygon symbol not found"), root.GetChild(0)->GetLineNumber() );
    } 
    
    std::string rootSym = root.GetChild(0)->GetSymbol();
    if( rootSym != "polygon" )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("polygon symbol not found"), root.GetChild(0)->GetLineNumber() );
    }
    
    for(size_t i = 2; i < root.GetNumberOfChildren(); i++ )
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

        if( sym == "comment" )
        {
            //kicad compat
            if( childList->GetChild(1)->IsSymbol() )
            {
                m_Info = childList->GetChild(1)->GetSymbol();
            }
            else if( childList->GetChild(1)->IsString() )
            {
                m_Info = childList->GetChild(1)->GetString();
            }
        }
        else if( sym == "name" )
        {
            //kicad compat
            if( childList->GetChild(1)->IsSymbol() )
            {
                m_Name = childList->GetChild(1)->GetSymbol();
            }
            else if( childList->GetChild(1)->IsString() )
            {
                m_Name = childList->GetChild(1)->GetString();
            }
        }
        else if( sym == "option" )
        {
            deserializeSEXPROption( childList );
        }
        else if( sym == "pos" )
        {
            deserializeSEXPRCoordinate( childList, m_Pos );
        }
        else if( sym == "repeat" )
        {
            m_RepeatCount = childList->GetChild(1)->GetInteger();
        }
        else if( sym == "incrx" )
        {
            m_IncrementVector.x = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "incry" )
        {
            m_IncrementVector.y = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "incrlabel" )
        {
            m_IncrementLabel = childList->GetChild(1)->GetInteger();
        }
        else if( sym == "linewidth" )
        {
            m_LineWidth = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "rotate" )
        {
            m_Orient = childList->GetChild(1)->GetDouble();
        }
    }
}

WORKSHEET_DATAITEM_TEXT::WORKSHEET_DATAITEM_TEXT( const wxString& aTextBase ) :
    WORKSHEET_DATAITEM( WS_TEXT )
{
    m_TextBase = aTextBase;
    m_IncrementLabel = 1;
    m_Hjustify = GR_TEXT_HJUSTIFY_LEFT;
    m_Vjustify = GR_TEXT_VJUSTIFY_CENTER;
    m_Orient = 0.0;
    m_LineWidth = 0.0;      // 0.0 means use default value
}

void WORKSHEET_DATAITEM_TEXT::TransfertSetupToGraphicText( WS_DRAW_ITEM_TEXT* aGText )
{
    aGText->SetHorizJustify( m_Hjustify ) ;
    aGText->SetVertJustify( m_Vjustify );
    aGText->SetOrientation( m_Orient * 10 );    // graphic text orient unit = 0.1 degree
}

void WORKSHEET_DATAITEM_TEXT::IncrementLabel( int aIncr )
{
    int last = m_TextBase.Len() -1;

    wxChar lbchar = m_TextBase[last];
    m_FullText = m_TextBase;
    m_FullText.RemoveLast();

    if( lbchar >= '0' &&  lbchar <= '9' )
        // A number is expected:
        m_FullText << (int)( aIncr + lbchar - '0' );
    else
        m_FullText << (wxChar) ( aIncr + lbchar );
}

// Replace the '\''n' sequence by EOL
// and the sequence  '\''\' by only one '\' in m_FullText
// if m_FullText is a multiline text (i.e.contains '\n') return true
bool WORKSHEET_DATAITEM_TEXT::ReplaceAntiSlashSequence()
{
    bool multiline = false;

    for( unsigned ii = 0; ii < m_FullText.Len(); ii++ )
    {
        if( m_FullText[ii] == '\n' )
            multiline = true;

        else if( m_FullText[ii] == '\\' )
        {
            if( ++ii >= m_FullText.Len() )
                break;

            if( m_FullText[ii] == '\\' )
            {
                // a double \\ sequence is replaced by a single \ char
                m_FullText.Remove(ii, 1);
                ii--;
            }
            else if( m_FullText[ii] == 'n' )
            {
                // Replace the "\n" sequence by a EOL char
                multiline = true;
                m_FullText[ii] = '\n';
                m_FullText.Remove(ii-1, 1);
                ii--;
            }
        }
    }

    return multiline;
}

void WORKSHEET_DATAITEM_TEXT::SetConstrainedTextSize()
{
    m_ConstrainedTextSize = m_TextSize;

    if( m_ConstrainedTextSize.x == 0  )
        m_ConstrainedTextSize.x = m_DefaultTextSize.x;

    if( m_ConstrainedTextSize.y == 0 )
        m_ConstrainedTextSize.y = m_DefaultTextSize.y;

    if( m_BoundingBoxSize.x || m_BoundingBoxSize.y )
    {
        int linewidth = 0;
        // to know the X and Y size of the line, we should use
        // EDA_TEXT::GetTextBox()
        // but this function uses integers
        // So, to avoid truncations with our unit in mm, use microns.
        wxSize size_micron;
        size_micron.x = KiROUND( m_ConstrainedTextSize.x * 1000.0 );
        size_micron.y = KiROUND( m_ConstrainedTextSize.y * 1000.0 );
        WS_DRAW_ITEM_TEXT dummy( WS_DRAW_ITEM_TEXT( this, this->m_FullText,
                                               wxPoint(0,0),
                                               size_micron,
                                               linewidth, BLACK,
                                               IsItalic(), IsBold() ) );
        dummy.SetMultilineAllowed( true );
        TransfertSetupToGraphicText( &dummy );
        EDA_RECT rect = dummy.GetTextBox();
        DSIZE size;
        size.x = rect.GetWidth() / 1000.0;
        size.y = rect.GetHeight() / 1000.0;

        if( m_BoundingBoxSize.x && size.x > m_BoundingBoxSize.x )
            m_ConstrainedTextSize.x *= m_BoundingBoxSize.x / size.x;

        if( m_BoundingBoxSize.y &&  size.y > m_BoundingBoxSize.y )
            m_ConstrainedTextSize.y *= m_BoundingBoxSize.y / size.y;
    }
}


SEXPR::SEXPR* WORKSHEET_DATAITEM_TEXT::SerializeSEXPR() const
{
    SEXPR::SEXPR_LIST* root = new SEXPR::SEXPR_LIST();

    *root << SEXPR::AsSymbol("tbtext");
    *root << m_TextBase.ToStdString();

    SEXPR::SEXPR_LIST* name = new SEXPR::SEXPR_LIST();
    *name << SEXPR::AsSymbol( "name" );
    *name << m_Name.ToStdString();
    *root << name;

    *root << serializeSEXPRCoordinate( "pos", m_Pos );

    serializeSEXPROptions(root);
    
    if( m_Orient )
    {
        SEXPR::SEXPR_LIST* rotate = new SEXPR::SEXPR_LIST();
        *rotate << SEXPR::AsSymbol( "rotate" );
        *rotate << m_Orient;
        *root << rotate;
    }
    
    // Write font info
    bool write_size = m_TextSize.x != 0.0 && m_TextSize.y != 0.0;
    if( write_size || IsBold() || IsItalic() )
    {
        SEXPR::SEXPR_LIST* font = new SEXPR::SEXPR_LIST();
        *font << SEXPR::AsSymbol( "font" );

        if( write_size )
        {
            SEXPR::SEXPR_LIST* size = new SEXPR::SEXPR_LIST();
            *size << SEXPR::AsSymbol( "size" );
            *size << m_TextSize.x << m_TextSize.y;
            *font << size;
        }

        if( IsBold() )
            *font << SEXPR::AsSymbol( "bold" );

        if( IsItalic() )
            *font << SEXPR::AsSymbol( "italic" );

        *root << font;
    }

    // Write text justification
    if( m_Hjustify != GR_TEXT_HJUSTIFY_LEFT ||
        m_Vjustify != GR_TEXT_VJUSTIFY_CENTER )
    {
        SEXPR::SEXPR_LIST* justify = new SEXPR::SEXPR_LIST();
        *justify << SEXPR::AsSymbol( "justify" );

        // Write T_center opt first, because it is
        // also a center for both m_Hjustify and m_Vjustify
        if( m_Hjustify == GR_TEXT_HJUSTIFY_CENTER )
            *justify << SEXPR::AsSymbol( "center" );

        if( m_Hjustify == GR_TEXT_HJUSTIFY_RIGHT )
            *justify << SEXPR::AsSymbol( "right" );

        if( m_Vjustify == GR_TEXT_VJUSTIFY_TOP )
            *justify << SEXPR::AsSymbol( "top" );

        if( m_Vjustify == GR_TEXT_VJUSTIFY_BOTTOM )
            *justify << SEXPR::AsSymbol( "bottom" );

        *root << justify;
    }
    
    // write constraints
    if( m_BoundingBoxSize.x )
    {
        SEXPR::SEXPR_LIST* maxlen = new SEXPR::SEXPR_LIST();
        *maxlen << SEXPR::AsSymbol( "maxlen" ) << m_BoundingBoxSize.x;
        *root << maxlen;
    }

    if( m_BoundingBoxSize.y )
    {
        SEXPR::SEXPR_LIST* maxheight = new SEXPR::SEXPR_LIST();
        *maxheight << SEXPR::AsSymbol( "maxheight" ) << m_BoundingBoxSize.y;
        *root << maxheight;
    }
                      
    serializeSEXPRRepeatParameters(root);
    
    return root;
}

void WORKSHEET_DATAITEM_TEXT::DeserializeSEXPR( SEXPR::SEXPR& root )
{
    if( !root.GetChild(0)->IsSymbol() )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("page_layout symbol not found"), root.GetChild(0)->GetLineNumber() );
    } 
    
    std::string rootSym = root.GetChild(0)->GetSymbol();
    if( rootSym != "tbtext" )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("rect or line symbol not found"), root.GetChild(0)->GetLineNumber() );
    }
    
    if( root.GetChild(1)->IsSymbol() )
    {
        m_TextBase = root.GetChild(1)->GetSymbol();
    }
    else if( root.GetChild(1)->IsString() )
    {
        m_TextBase = root.GetChild(1)->GetString();
    }
    
    for(size_t i = 2; i < root.GetNumberOfChildren(); i++ )
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

        if( sym == "comment" )
        {
            //kicad compat
            if( childList->GetChild(1)->IsSymbol() )
            {
                m_Info = childList->GetChild(1)->GetSymbol();
            }
            else if( childList->GetChild(1)->IsString() )
            {
                m_Info = childList->GetChild(1)->GetString();
            }
        }
        else if( sym == "name" )
        {
            //kicad compat
            if( childList->GetChild(1)->IsSymbol() )
            {
                m_Name = childList->GetChild(1)->GetSymbol();
            }
            else if( childList->GetChild(1)->IsString() )
            {
                m_Name = childList->GetChild(1)->GetString();
            }
        }
        else if( sym == "option" )
        {
            deserializeSEXPROption( childList );
        }
        else if( sym == "pos" )
        {
            deserializeSEXPRCoordinate( childList, m_Pos );
        }
        else if( sym == "repeat" )
        {
            m_RepeatCount = childList->GetChild(1)->GetInteger();
        }
        else if( sym == "incrx" )
        {
            m_IncrementVector.x = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "incry" )
        {
            m_IncrementVector.y = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "incrlabel" )
        {
            m_IncrementLabel = childList->GetChild(1)->GetInteger();
        }
        else if( sym == "maxlen" )
        {
            m_BoundingBoxSize.x = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "maxheight" )
        {
            m_BoundingBoxSize.y = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "font" )
        {
            deserializeSEXPRFont( childList );
        }
        else if( sym == "justify" )
        {
            deserializeSEXPRJustify( childList );
        }
        else if( sym == "rotate" )
        {
            m_Orient = childList->GetChild(1)->GetDouble();
        }
    }
}

void WORKSHEET_DATAITEM_TEXT::deserializeSEXPRFont( SEXPR::SEXPR_LIST* list )
{
    for(size_t i = 1; i < list->GetNumberOfChildren(); i++ )
    {
        if(list->GetChild(i)->IsSymbol())
        {
            std::string token = list->GetChild(i)->GetSymbol();
            if( token == "bold" )
            {
                SetBold( true );
            }
            else if( token == "italic" )
            {
                SetItalic( true );
            }
        }
        else if(list->GetChild(i)->IsList())
        {
            SEXPR::SEXPR_LIST* childList = list->GetChild(i)->GetList();
            std::string token = childList->GetChild(0)->GetSymbol();
            
            if( token == "size" )
            {
                m_TextSize.x = childList->GetChild(1)->GetDouble();
                m_TextSize.y = childList->GetChild(2)->GetDouble();
            }
        }
        else
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("unsupported"), list->GetChild(i)->GetLineNumber() );
        }
    }
}

void WORKSHEET_DATAITEM_TEXT::deserializeSEXPRJustify( SEXPR::SEXPR_LIST* list )
{
    for(size_t i = 1; i < list->GetNumberOfChildren(); i++ )
    {
        std::string token = list->GetChild(i)->GetSymbol();
        if( token == "center" )
        {
            m_Hjustify = GR_TEXT_HJUSTIFY_CENTER;
            m_Vjustify = GR_TEXT_VJUSTIFY_CENTER;
        }
        else if( token == "left" )
        {
            m_Hjustify = GR_TEXT_HJUSTIFY_LEFT;
        }
        else if( token == "right" )
        {
            m_Hjustify = GR_TEXT_HJUSTIFY_RIGHT;
        }
        else if (token == "top")
        {
            m_Vjustify = GR_TEXT_VJUSTIFY_TOP;
        }
        else if (token == "bottom")
        {
            m_Vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
        }
        else
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("unsupported"), list->GetChild(i)->GetLineNumber() );
        }
    }
}

/* set the pixel scale factor of the bitmap
 * this factor depend on the application internal unit
 * and the PPI bitmap factor
 * the pixel scale factor should be initialized before drawing the bitmap
 */
void WORKSHEET_DATAITEM_BITMAP::SetPixelScaleFactor()
{
    if( m_ImageBitmap )
    {
        // m_WSunits2Iu is the page layout unit to application internal unit
        // i.e. the mm to to application internal unit
        // however the bitmap definition is always known in pixels per inches
        double scale = m_WSunits2Iu * 25.4 / m_ImageBitmap->GetPPI();
        m_ImageBitmap->SetPixelScaleFactor( scale );
    }
}

/* return the PPI of the bitmap
 */
int WORKSHEET_DATAITEM_BITMAP::GetPPI() const
{
    if( m_ImageBitmap )
        return m_ImageBitmap->GetPPI() / m_ImageBitmap->m_Scale;

    return 300;
}

/*adjust the PPI of the bitmap
 */
void WORKSHEET_DATAITEM_BITMAP::SetPPI( int aBitmapPPI )
{
    if( m_ImageBitmap )
        m_ImageBitmap->m_Scale = (double) m_ImageBitmap->GetPPI() / aBitmapPPI;
}


SEXPR::SEXPR* WORKSHEET_DATAITEM_BITMAP::SerializeSEXPR() const
{
    SEXPR::SEXPR_LIST* root = new SEXPR::SEXPR_LIST();

    *root << SEXPR::AsSymbol("bitmap");

    SEXPR::SEXPR_LIST* name = new SEXPR::SEXPR_LIST();
    *name << SEXPR::AsSymbol( "name" );
    *name << m_Name.ToStdString();
    *root << name;

    *root << serializeSEXPRCoordinate( "pos", m_Pos );

    serializeSEXPROptions(root);
    
    SEXPR::SEXPR_LIST* scale = new SEXPR::SEXPR_LIST();
    *scale << SEXPR::AsSymbol( "scale" ) << m_ImageBitmap->m_Scale;
    *root << scale;
        
    serializeSEXPRRepeatParameters(root);
    
    SEXPR::SEXPR_LIST* pngdata = new SEXPR::SEXPR_LIST();
    *pngdata << SEXPR::AsSymbol( "pngdata" );

    wxArrayString pngStrings;
    m_ImageBitmap->SaveData( pngStrings );

    for( unsigned ii = 0; ii < pngStrings.GetCount(); ii++ )
    {
        SEXPR::SEXPR_LIST* data = new SEXPR::SEXPR_LIST();
        *data << SEXPR::AsSymbol( "data" ) << pngStrings[ii].ToStdString();
        *pngdata << data;
    }
    
    *root << pngdata;

    return root;
}


void WORKSHEET_DATAITEM_BITMAP::DeserializeSEXPR( SEXPR::SEXPR& root )
{
    if( !root.GetChild(0)->IsSymbol() )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("bitmap symbol not found"), root.GetChild(0)->GetLineNumber() );
    } 
    
    std::string rootSym = root.GetChild(0)->GetSymbol();
    if( rootSym != "bitmap" )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("bitmap symbol not found"), root.GetChild(0)->GetLineNumber() );
    }
    
    for(size_t i = 2; i < root.GetNumberOfChildren(); i++ )
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

        if( sym == "name" )
        {
            //kicad compat
            if( childList->GetChild(1)->IsSymbol() )
            {
                m_Name = childList->GetChild(1)->GetSymbol();
            }
            else if( childList->GetChild(1)->IsString() )
            {
                m_Name = childList->GetChild(1)->GetString();
            }
        }
        else if( sym == "pos" )
        {
            deserializeSEXPRCoordinate( childList, m_Pos );
        }
        else if( sym == "scale" )
        {
            m_RepeatCount = childList->GetChild(1)->GetDouble();
        }
        else if( sym == "pngdata" )
        {
            deserializeSEXPRPNGData( childList );
        }
    }
}


void WORKSHEET_DATAITEM_BITMAP::deserializeSEXPRPNGData( SEXPR::SEXPR* root )
{
    std::string tmp;
    
    for(size_t i = 1; i < root->GetNumberOfChildren(); i++ )
    {
        SEXPR::SEXPR* child = root->GetChild(i);

        if( !child->IsList() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), child->GetLineNumber() );
        }
        
        SEXPR::SEXPR_LIST* childList = child->GetList();
        
        tmp += childList->GetChild(1)->GetString();
    }
    

    tmp += "EndData";

    wxString msg;
    STRING_LINE_READER reader( tmp, wxT("Png kicad_wks data") );
    if( !m_ImageBitmap->LoadData( reader, msg ) )
    {
        wxLogMessage(msg);
    }
}