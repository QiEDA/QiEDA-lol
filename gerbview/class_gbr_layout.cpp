/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_gbr_layout.cpp
 * @brief  GBR_LAYOUT class functions.
 */

#include <limits.h>
#include <algorithm>

#include <common.h>

#include <class_gbr_layout.h>
#include <class_drawpanel.h>
#include <class_gerber_image.h>
#include <class_X2_gerber_attributes.h>

static bool sortZorder( const GERBER_IMAGE* const& ref, const GERBER_IMAGE* const& test );

GBR_LAYOUT::GBR_LAYOUT( GERBVIEW_FRAME* aParent ) :
        m_Parent( aParent ),
        m_nextLayerId( 0 )
{
}


GBR_LAYOUT::~GBR_LAYOUT()
{
}


EDA_RECT GBR_LAYOUT::ComputeBoundingBox()
{
    EDA_RECT bbox;

    for (std::vector<GERBER_IMAGE*>::iterator it=m_Gerbers.begin(); it != m_Gerbers.end(); ++it)
    {
        GERBER_IMAGE *gerber = *it;

        for (std::list<GERBER_DRAW_ITEM *>::iterator jt = gerber->m_Drawings.begin();
             jt != gerber->m_Drawings.end(); ++jt)
        {
            GERBER_DRAW_ITEM* item = *jt;
            bbox.Merge( item->GetBoundingBox() );
        }
    }

    SetBoundingBox( bbox );
    return bbox;
}



/*
 * Redraw All GerbView layers, using a buffered mode or not
 */
void GBR_LAYOUT::Draw( EDA_DRAW_PANEL* aPanel,
                       wxDC* aDC,
                       const std::vector<GERBER_IMAGE*>& aLayers,
                       GERBER_IMAGE* aSelectedLayer,
                       GR_DRAWMODE aDrawMode,
                       const wxPoint& aOffset,
                       EDA_COLOR_T aBGColor,
                       bool aPrintBlackAndWhite )
{
    GERBVIEW_FRAME* gerbFrame = (GERBVIEW_FRAME*) aPanel->GetParent();

    // Because Images can be negative (i.e with background filled in color) items are drawn
    // graphic layer per graphic layer, after the background is filled
    // to a temporary bitmap
    // at least when aDrawMode = GR_COPY or aDrawMode = GR_OR
    // If aDrawMode = UNSPECIFIED_DRAWMODE, items are drawn to the main screen, and therefore
    // artifacts can happen with negative items or negative images

    wxColour bgColor = MakeColour( aBGColor );
    wxBrush  bgBrush( bgColor, wxBRUSHSTYLE_SOLID );

    int      bitmapWidth, bitmapHeight;
    wxDC*    plotDC = aDC;

    aPanel->GetClientSize( &bitmapWidth, &bitmapHeight );

    wxBitmap*  layerBitmap  = NULL;
    wxBitmap*  screenBitmap = NULL;
    wxMemoryDC layerDC;         // used sequentially for each gerber layer
    wxMemoryDC screenDC;

    // When each image must be drawn using GR_OR (transparency mode)
    // or GR_COPY (stacked mode) we must use a temporary bitmap
    // to draw gerber images.
    // this is due to negative objects (drawn using background color) that create artifacts
    // on other images when drawn on screen
    bool useBufferBitmap = false;

#ifndef __WXMAC__
    // Can't work with MAC
    // Don't try this with retina display
    if( (aDrawMode == GR_COPY) || ( aDrawMode == GR_OR ) )
        useBufferBitmap = true;
#endif

    // these parameters are saved here, because they are modified
    // and restored later
    EDA_RECT drawBox = *aPanel->GetClipBox();
    double scale;
    aDC->GetUserScale(&scale, &scale);
    wxPoint dev_org = aDC->GetDeviceOrigin();
    wxPoint logical_org = aDC->GetLogicalOrigin( );


    if( useBufferBitmap )
    {
        layerBitmap  = new wxBitmap( bitmapWidth, bitmapHeight );
        screenBitmap = new wxBitmap( bitmapWidth, bitmapHeight );
        layerDC.SelectObject( *layerBitmap );
        aPanel->DoPrepareDC( layerDC );
        aPanel->SetClipBox( drawBox );
        layerDC.SetBackground( bgBrush );
        layerDC.SetBackgroundMode( wxSOLID );
        layerDC.Clear();

        screenDC.SelectObject( *screenBitmap );
        screenDC.SetBackground( bgBrush );
        screenDC.SetBackgroundMode( wxSOLID );
        screenDC.Clear();

        plotDC = &layerDC;
    }

    bool doBlit = false; // this flag requests an image transfer to actual screen when true.

    bool end = false;

    // Draw layers from bottom to top, and active layer last
    // in non transparent modes, the last layer drawn mask mask previously drawn layer
    for( int layer = aLayers.size()-1; !end; --layer )
    {
        int dcode_highlight = 0;
        GERBER_IMAGE* gerber = NULL;
        if( layer >= 0 )
        {
            gerber = aLayers[layer];

            if( aSelectedLayer != NULL
                && gerber == aSelectedLayer ) // active layer will be drawn after other layers
            {
                continue;
            }
        }
        else
        {
            end   = true;
            gerber = aSelectedLayer;

            if( gerber != NULL )
                dcode_highlight = gerber->m_Selected_Tool;
        }

        if( gerber == NULL )
            continue;

        if( !gerber->m_Visible )
            continue;

        EDA_COLOR_T color = gerbFrame->GetLayerColor( layer );

        // Force black and white draw mode on request:
        if( aPrintBlackAndWhite )
            gerbFrame->SetLayerColor( layer, aBGColor == BLACK ? WHITE : BLACK );

        if( useBufferBitmap )
        {
            // Draw each layer into a bitmap first. Negative Gerber
            // layers are drawn in background color.
            if( gerber->HasNegativeItems() &&  doBlit )
            {
                // Set Device origin, logical origin and scale to default values
                // This is needed by Blit function when using a mask.
                // Beside, for Blit call, both layerDC and screenDc must have the same settings
                layerDC.SetDeviceOrigin(0,0);
                layerDC.SetLogicalOrigin( 0, 0 );
                layerDC.SetUserScale( 1, 1 );

                if( aDrawMode == GR_COPY )
                {
                    // Use the layer bitmap itself as a mask when blitting.  The bitmap
                    // cannot be referenced by a device context when setting the mask.
                    layerDC.SelectObject( wxNullBitmap );
                    layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor ) );
                    layerDC.SelectObject( *layerBitmap );
                    screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxCOPY, true );
                }
                else if( aDrawMode == GR_OR )
                {
                    // On Linux with a large screen, this version is much faster and without
                    // flicker, but gives a Pcbnew look where layer colors blend together.
                    // Plus it works only because the background color is black.  But it may
                    // be more usable for some.  The difference is due in part because of
                    // the cpu cycles needed to create the monochromatic bitmap above, and
                    // the extra time needed to do bit indexing into the monochromatic bitmap
                    // on the blit above.
                    screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxOR );
                }
                // Restore actual values and clear bitmap for next drawing
                layerDC.SetDeviceOrigin( dev_org.x, dev_org.y );
                layerDC.SetLogicalOrigin( logical_org.x, logical_org.y );
                layerDC.SetUserScale( scale, scale );
                layerDC.SetBackground( bgBrush );
                layerDC.SetBackgroundMode( wxSOLID );
                layerDC.Clear();

                doBlit = false;
            }

        }

        if( gerber->m_ImageNegative )
        {
            // Draw background negative (i.e. in graphic layer color) for negative images.
            EDA_COLOR_T color = gerbFrame->GetLayerColor( layer );

            GRSetDrawMode( &layerDC, GR_COPY );
            GRFilledRect( &drawBox, plotDC, drawBox.GetX(), drawBox.GetY(),
                          drawBox.GetRight(), drawBox.GetBottom(),
                          0, color, color );

            GRSetDrawMode( plotDC, GR_COPY );
            doBlit = true;
        }


        GR_DRAWMODE layerdrawMode = GR_COPY;

        if( aDrawMode == GR_OR && !gerber->HasNegativeItems() )
            layerdrawMode = GR_OR;

        // Now we can draw the current layer to the bitmap buffer
        // When needed, the previous bitmap is already copied to the screen buffer.
        for (std::list<GERBER_DRAW_ITEM*>::iterator it=gerber->m_Drawings.begin(); it != gerber->m_Drawings.end(); ++it)
        {
            GERBER_DRAW_ITEM* item = *it;
            GR_DRAWMODE drawMode = layerdrawMode;

            if( dcode_highlight && dcode_highlight == item->m_DCode )
                DrawModeAddHighlight( &drawMode);

            item->Draw( aPanel, plotDC, drawMode, wxPoint(0,0) );
            doBlit = true;
        }

        if( aPrintBlackAndWhite )
            gerbFrame->SetLayerColor( layer, color );
    }

    if( doBlit && useBufferBitmap )     // Blit is used only if aDrawMode >= 0
    {
        // For this Blit call, layerDC and screenDC must have the same settings
        // So we set device origin, logical origin and scale to default values
        // in layerDC
        layerDC.SetDeviceOrigin(0,0);
        layerDC.SetLogicalOrigin( 0, 0 );
        layerDC.SetUserScale( 1, 1 );

        // this is the last transfer to screenDC.  If there are no negative items, this is
        // the only one
        if( aDrawMode == GR_COPY )
        {
            layerDC.SelectObject( wxNullBitmap );
            layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor ) );
            layerDC.SelectObject( *layerBitmap );
            screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxCOPY, true );

        }
        else if( aDrawMode == GR_OR )
        {
            screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxOR );
        }
    }

    if( useBufferBitmap )
    {
        // For this Blit call, aDC and screenDC must have the same settings
        // So we set device origin, logical origin and scale to default values
        // in aDC
        aDC->SetDeviceOrigin( 0, 0);
        aDC->SetLogicalOrigin( 0, 0 );
        aDC->SetUserScale( 1, 1 );

        aDC->Blit( 0, 0, bitmapWidth, bitmapHeight, &screenDC, 0, 0, wxCOPY );

        // Restore aDC values
        aDC->SetDeviceOrigin(dev_org.x, dev_org.y);
        aDC->SetLogicalOrigin( logical_org.x, logical_org.y );
        aDC->SetUserScale( scale, scale );

        layerDC.SelectObject( wxNullBitmap );
        screenDC.SelectObject( wxNullBitmap );
        delete layerBitmap;
        delete screenBitmap;
    }
}

void GBR_LAYOUT::MoveLayerUp( int aIdx )
{
    if( aIdx > 0 )
    {
        std::iter_swap(m_Gerbers.begin() + aIdx-1, m_Gerbers.begin() + aIdx);
    }
}

void GBR_LAYOUT::MoveLayerDown( int aIdx )
{
    if( aIdx < (int)m_Gerbers.size()-1 )
    {
        std::iter_swap(m_Gerbers.begin() + aIdx, m_Gerbers.begin() + aIdx+1);
    }
}
void GBR_LAYOUT::SortGerbersByZOrder()
{
    std::sort( m_Gerbers.begin(), m_Gerbers.end(), sortZorder );
}

// Helper function, for std::sort.
// Sort loaded images by Z order priority, if they have the X2 FileFormat info
// returns true if the first argument (ref) is ordered before the second (test).
static bool sortZorder( const GERBER_IMAGE* const& ref, const GERBER_IMAGE* const& test )
{
    if( !ref && !test )
        return false;        // do not change order: no criteria to sort items

    if( !ref || !ref->m_InUse )
        return false;       // Not used: ref ordered after

    if( !test || !test->m_InUse )
        return true;        // Not used: ref ordered before

    if( !ref->m_FileFunction && !test->m_FileFunction )
        return false;        // do not change order: no criteria to sort items

    if( !ref->m_FileFunction )
        return false;

    if( !test->m_FileFunction )
        return true;

    if( ref->m_FileFunction->GetZOrder() != test->m_FileFunction->GetZOrder() )
        return ref->m_FileFunction->GetZOrder() > test->m_FileFunction->GetZOrder();

    return ref->m_FileFunction->GetZSubOrder() > test->m_FileFunction->GetZSubOrder();
}



// remove the loaded data of image aIdx, but do not delete it
void GBR_LAYOUT::DeleteGerber( int aIdx )
{
    if( aIdx >= 0 && aIdx < (int)m_Gerbers.size() )
    {
        delete m_Gerbers[aIdx];
        m_Gerbers.erase (m_Gerbers.begin()+aIdx);
    }
}

GERBER_IMAGE* GBR_LAYOUT::GetGerberByListIndex( int aIdx )
{
    if( aIdx < m_Gerbers.size() && aIdx >= 0 )
        return m_Gerbers[aIdx];

    return NULL;
}

GERBER_IMAGE* GBR_LAYOUT::GetGerberById( int layerID )
{
    for (std::vector<GERBER_IMAGE*>::iterator it = m_Gerbers.begin() ; it != m_Gerbers.end(); ++it)
    {
        if( (*it)->m_GraphicLayer == layerID )
        {
            return *it;
        }
    }

    return NULL;
}

int GBR_LAYOUT::GetGerberIndexByLayer( int layerID )
{
    for (std::vector<GERBER_IMAGE*>::iterator it = m_Gerbers.begin() ; it != m_Gerbers.end(); ++it)
    {
        if( (*it)->m_GraphicLayer == layerID )
        {
            int result = it - m_Gerbers.begin();
            return result;
        }
    }

    return 0;
}

int GBR_LAYOUT::AddGerber( GERBER_IMAGE* aGbrImage )
{
    m_Gerbers.push_back(aGbrImage);
    int idx = m_nextLayerId++;

    return idx;
}


void GBR_LAYOUT::DeleteAllGerbers( void )
{
    for (std::vector<GERBER_IMAGE*>::iterator it = m_Gerbers.begin() ; it != m_Gerbers.end(); ++it)
    {
        delete (*it);
    }

    m_Gerbers.clear();

    m_nextLayerId = 0;
}
