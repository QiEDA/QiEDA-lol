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
                             drawMode,
                             wxPoint( 0, 0 )
                            );

    // Draw the "background" now, i.e. grid and axis after gerber layers
    // because most of time the actual background is erased by successive drawings of each gerber
    // layer mainly in COPY mode
    m_canvas->DrawBackGround( DC );

    if( IsElementVisible( DCODES_VISIBLE ) )
    {
        GetGerberLayout()->DrawItemsDCodeID( m_canvas, DC, GR_COPY, GetVisibleElementColor(DCODES_VISIBLE) );
    }

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