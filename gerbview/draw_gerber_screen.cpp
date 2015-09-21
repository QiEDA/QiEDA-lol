/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jpierre.charras at wanadoo
 * Copyright (C) 2013-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file draw_gerber_screen.cpp
 */


#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/debug.h>
#include <wx/utils.h>

#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <base_units.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_image.h>
#include <class_board_printout_controller.h>


void GERBVIEW_FRAME::PrintPage( wxDC* aDC, PRINT_PARAMETERS& aParams, int aPage )
{
    // Save current draw options, because print mode has specific options:
    GERBER_DISPLAY_OPTIONS imgDisplayOptions;

    // Set draw options for printing:
    imgDisplayOptions.m_DisplayFlashedItemsFill = true;
    imgDisplayOptions.m_DisplayLinesFill = true;
    imgDisplayOptions.m_DisplayPolygonsFill = true;
    imgDisplayOptions.m_DisplayDCodes = false;
    imgDisplayOptions.m_IsPrinting = true;
    imgDisplayOptions.m_NegativeObjectColor = GetVisibleElementColor( NEGATIVE_OBJECTS_VISIBLE );
    imgDisplayOptions.m_BackgroundColor = WHITE;

    m_canvas->SetPrintMirrored( aParams.m_PrintMirror );

    std::vector<GERBER_IMAGE*>::const_iterator first = aParams.m_LayerQueue.begin() + aPage -1;
    std::vector<GERBER_IMAGE*>::const_iterator last = aParams.m_LayerQueue.begin() + aPage;
    std::vector<GERBER_IMAGE*> printLayer(first, last);

    GetGerberLayout()->Draw( m_canvas,
                             aDC,
                             imgDisplayOptions,
                             printLayer,
                             NULL,
                             (GR_DRAWMODE) 0,
                             wxPoint( 0, 0 ),
                             WHITE,
                             aParams.m_Print_Black_and_White );

    m_canvas->SetPrintMirrored( false );
}


void GERBVIEW_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    GBR_SCREEN* screen = (GBR_SCREEN*) GetScreen();

    if( !GetGerberLayout() )
        return;

    wxBusyCursor dummy;

    GR_DRAWMODE drawMode = UNSPECIFIED_DRAWMODE;

    switch( GetDisplayMode() )
    {
    default:
    case 0:
        break;

    case 1:
        drawMode = GR_COPY;
        break;

    case 2:
        drawMode = GR_OR;
        break;
    }

    m_DisplayOptions.m_NegativeObjectColor = GetVisibleElementColor( NEGATIVE_OBJECTS_VISIBLE );
    m_DisplayOptions.m_BackgroundColor = GetDrawBgColor();

    // Draw according to the current setting.  This needs to be GR_COPY or GR_OR.
    GetGerberLayout()->Draw( m_canvas,
                             DC,
                             m_DisplayOptions,
                             GetGerberLayout()->GetGerbers(),
                             GetGerberLayout()->GetGerberByListIndex( getActiveLayer() ),
                             drawMode, wxPoint( 0, 0 ),GetDrawBgColor() );

    // Draw the "background" now, i.e. grid and axis after gerber layers
    // because most of time the actual background is erased by successive drawings of each gerber
    // layer mainly in COPY mode
    m_canvas->DrawBackGround( DC );

    if( IsElementVisible( DCODES_VISIBLE ) )
        DrawItemsDCodeID( DC, GR_COPY );

    DrawWorkSheet( DC, screen, 0, IU_PER_MILS, wxEmptyString );

#ifdef USE_WX_OVERLAY
    if( IsShown() )
    {
        m_overlay.Reset();
        wxDCOverlay overlaydc( m_overlay, (wxWindowDC*)DC );
        overlaydc.Clear();
    }
#endif

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    m_canvas->DrawCrossHair( DC );

    // Display the filename and the layer name (found in the gerber files, if any)
    // relative to the active layer
    UpdateTitleAndInfo();
}


void GERBVIEW_FRAME::DrawItemsDCodeID( wxDC* aDC, GR_DRAWMODE aDrawMode )
{
    wxPoint     pos;
    int         width;
    double      orient;
    wxString    Line;

    GRSetDrawMode( aDC, aDrawMode );

    for (std::vector<GERBER_IMAGE*>::const_reverse_iterator git = GetGerberLayout()->GetGerbers().rbegin() ; git != GetGerberLayout()->GetGerbers().rend(); ++git)
    {
        GERBER_IMAGE* gerber = *git;

        if( !gerber->m_Visible )
            continue;

        for (std::list<GERBER_DRAW_ITEM*>::iterator it=gerber->m_Drawings.begin(); it != gerber->m_Drawings.end(); ++it)
        {
            GERBER_DRAW_ITEM* item = *it;

            if (item->m_DCode <= 0)
                continue;

            if (item->m_Flashed || item->m_Shape == GBR_ARC) {
                pos = item->m_Start;
            }
            else {
                pos.x = (item->m_Start.x + item->m_End.x) / 2;
                pos.y = (item->m_Start.y + item->m_End.y) / 2;
            }

            pos = item->GetABPosition(pos);

            Line.Printf(wxT("D%d"), item->m_DCode);

            if (item->GetDcodeDescr())
                width = item->GetDcodeDescr()->GetShapeDim(item);
            else
                width = std::min(item->m_Size.x, item->m_Size.y);

            orient = TEXT_ORIENT_HORIZ;

            if (item->m_Flashed) {
                // A reasonable size for text is width/3 because most of time this text has 3 chars.
                width /= 3;
            }
            else        // this item is a line
            {
                wxPoint delta = item->m_Start - item->m_End;

                if (abs(delta.x) < abs(delta.y))
                    orient = TEXT_ORIENT_VERT;

                // A reasonable size for text is width/2 because text needs margin below and above it.
                // a margin = width/4 seems good
                width /= 2;
            }

            int color = GetVisibleElementColor(DCODES_VISIBLE);

            DrawGraphicText(m_canvas->GetClipBox(), aDC, pos, (EDA_COLOR_T) color, Line,
                            orient, wxSize(width, width),
                            GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                            0, false, false);
        }
    }
}
