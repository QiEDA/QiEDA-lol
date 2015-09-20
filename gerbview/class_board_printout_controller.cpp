/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file printout_control.cpp
 * @brief Board print handler implementation file.
 */


// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include <wx/dc.h>
#include <wx/print.h>
#include <wx/string.h>
#include <wx/utils.h>

#include <pgm_base.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <base_units.h>
#include <wxstruct.h>
#include <class_base_screen.h>

#include <gerbview_frame.h>

#include <class_board_printout_controller.h>



PRINT_PARAMETERS::PRINT_PARAMETERS()
{
    m_PenDefaultSize        = Millimeter2iu( 0.2 ); // A reasonable default value to draw items
                                      // which do not have a specified line width
    m_PrintScale            = 1.0;
    m_XScaleAdjust          = 1.0;
    m_YScaleAdjust          = 1.0;
    m_LayerQueue.clear();
    m_PrintMirror           = false;
    m_Print_Black_and_White = true;
    m_PageSetupData         = NULL;
}


BOARD_PRINTOUT_CONTROLLER::BOARD_PRINTOUT_CONTROLLER( const PRINT_PARAMETERS& aParams,
                                                      GERBVIEW_FRAME*         aParent,
                                                      const wxString&         aTitle ) :
    wxPrintout( aTitle )
{
    m_Parent = aParent;
    m_PrintParams =  aParams;
}


bool BOARD_PRINTOUT_CONTROLLER::OnPrintPage( int aPage )
{
    // in gerbview, draw layers are always printed on separate pages
    // because handling negative objects when using only one page is tricky
    DrawPage( aPage );

    return true;
}


void BOARD_PRINTOUT_CONTROLLER::GetPageInfo( int* minPage, int* maxPage,
                                             int* selPageFrom, int* selPageTo )
{
    *minPage     = 1;
    *selPageFrom = 1;

    *maxPage   = m_PrintParams.m_LayerQueue.size();
    *selPageTo  = m_PrintParams.m_LayerQueue.size();
}


void BOARD_PRINTOUT_CONTROLLER::DrawPage( int aPage )
{
    wxPoint       offset;
    double        userscale;
    EDA_RECT      boardBoundingBox;
    EDA_RECT      drawRect;
    wxDC*         dc = GetDC();
    BASE_SCREEN*  screen = m_Parent->GetScreen();
    bool          printMirror = m_PrintParams.m_PrintMirror;
    wxSize        pageSizeIU = m_Parent->GetPageSizeIU();

    wxBusyCursor  dummy;

    boardBoundingBox = ((GERBVIEW_FRAME*) m_Parent)->GetGerberLayoutBoundingBox();

    // Compute the PCB size in internal units
    userscale = m_PrintParams.m_PrintScale;

    if( m_PrintParams.m_PrintScale == 0 )   //  fit in page option
    {
        if(boardBoundingBox.GetWidth() && boardBoundingBox.GetHeight())
        {
            int margin = Millimeter2iu( 10.0 ); // add a margin around the drawings
            double scaleX = (double)(pageSizeIU.x - (2 * margin)) /
                            boardBoundingBox.GetWidth();
            double scaleY = (double)(pageSizeIU.y - (2 * margin)) /
                            boardBoundingBox.GetHeight();
            userscale = (scaleX < scaleY) ? scaleX : scaleY;
        }
        else
        {
            userscale = 1.0;
        }
    }

    wxSize scaledPageSize = pageSizeIU;
    drawRect.SetSize( scaledPageSize );
    scaledPageSize.x = wxRound( scaledPageSize.x / userscale );
    scaledPageSize.y = wxRound( scaledPageSize.y / userscale );


    if( m_PrintParams.m_PageSetupData )
    {
        // Always scale to the size of the paper.
        FitThisSizeToPageMargins( scaledPageSize, *m_PrintParams.m_PageSetupData );
    }

    // Compute Accurate scale 1
    if( m_PrintParams.m_PrintScale == 1.0 )
    {
        // We want a 1:1 scale, regardless the page setup
        // like page size, margin ...
        MapScreenSizeToPaper(); // set best scale and offset (scale is not used)
        int w, h;
        GetPPIPrinter( &w, &h );
        double accurate_Xscale = (double) w / (IU_PER_MILS*1000);
        double accurate_Yscale = (double) h / (IU_PER_MILS*1000);

        if( IsPreview() )  // Scale must take in account the DC size in Preview
        {
            // Get the size of the DC in pixels
            wxSize       PlotAreaSize;
            dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );
            GetPageSizePixels( &w, &h );
            accurate_Xscale *= (double)PlotAreaSize.x / w;
            accurate_Yscale *= (double)PlotAreaSize.y / h;
        }
        // Fine scale adjust
        accurate_Xscale *= m_PrintParams.m_XScaleAdjust;
        accurate_Yscale *= m_PrintParams.m_YScaleAdjust;

        // Set print scale for 1:1 exact scale
        dc->SetUserScale( accurate_Xscale, accurate_Yscale );
    }

    // Get the final size of the DC in pixels
    wxSize       PlotAreaSizeInPixels;
    dc->GetSize( &PlotAreaSizeInPixels.x, &PlotAreaSizeInPixels.y );

    // In some cases the plot origin is the centre of the board outline rather than the center
    // of the selected paper size.
    if( m_PrintParams.CenterOnBoardOutline() )
    {
        // Here we are only drawing the board and it's contents.
        drawRect = boardBoundingBox;
        offset.x += wxRound( (double) -scaledPageSize.x / 2.0 );
        offset.y += wxRound( (double) -scaledPageSize.y / 2.0 );

        wxPoint center = boardBoundingBox.Centre();

        if( printMirror )
        {
            // Calculate the mirrored center of the board.
            center.x = m_Parent->GetPageSizeIU().x - boardBoundingBox.Centre().x;
        }

        offset += center;
    }

    GRResetPenAndBrush( dc );

    screen->m_IsPrinting = true;

    // Print frame reference, if requested, before printing draw layers
    if( m_PrintParams.m_Print_Black_and_White )
        GRForceBlackPen( true );

    if( printMirror )
    {
        // To plot mirror, we reverse the x axis, and modify the plot x origin
        dc->SetAxisOrientation( false, false );

        /* Plot offset x is moved by the x plot area size in order to have
         * the old draw area in the new draw area, because the draw origin has not moved
         * (this is the upper left corner) but the X axis is reversed, therefore the plotting area
         * is the x coordinate values from  - PlotAreaSize.x to 0 */
        int x_dc_offset = PlotAreaSizeInPixels.x;
        x_dc_offset = KiROUND( x_dc_offset  * userscale );
        dc->SetDeviceOrigin( x_dc_offset, 0 );
    }

    dc->SetLogicalOrigin( offset.x, offset.y );

    // Never force black pen to print draw layers
    // because negative objects need a white pen, not a black pen
    // B&W mode is handled in print page function
    GRForceBlackPen( false );

    m_Parent->PrintPage( dc, m_PrintParams, aPage );

    screen->m_IsPrinting = false;
}
