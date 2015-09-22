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
 * @file class_gbr_layout.h
 * @brief Class CLASS_GBR_LAYOUT to handle info to draw/print loaded Gerber images
 * and page frame reference
 */

#ifndef CLASS_GBR_LAYOUT_H
#define CLASS_GBR_LAYOUT_H


#include <dlist.h>

#include <class_colors_design_settings.h>
#include <common.h>                         // PAGE_INFO
#include <gerbview.h>                       // GERBER_DRAWLAYERS_COUNT
#include "gerbview_frame.h"
#include <class_title_block.h>
#include <class_gerber_draw_item.h>
#include <class_gerber_display_options.h>

#include <gerbview_frame.h>
#include <gr_basic.h>

class GERBVIEW_FRAME;

/**
 * Class GBR_LAYOUT
 * holds list of GERBER_DRAW_ITEM currently loaded.
 */
class GBR_LAYOUT
{
private:
    EDA_RECT            m_BoundingBox;
    TITLE_BLOCK         m_titles;
    wxPoint             m_originAxisPosition;
    GERBVIEW_FRAME*     m_Parent;

    // the list of loaded images (1 image = 1 gerber file)
    std::vector<GERBER_IMAGE*> m_Gerbers;
    int m_nextLayerId;
public:
    GERBVIEW_FRAME* GetParent() const { return m_Parent; }

    GBR_LAYOUT(GERBVIEW_FRAME* aParent);
    ~GBR_LAYOUT();

    const wxPoint&      GetAuxOrigin() const
    {
        return m_originAxisPosition;
    }

    void SetAuxOrigin( const wxPoint& aPosition )
    {
        m_originAxisPosition = aPosition;
    }

    TITLE_BLOCK& GetTitleBlock()
    {
        return m_titles;
    }

    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
    {
        m_titles = aTitleBlock;
    }

    /**
     * Function ComputeBoundingBox
     * calculates the bounding box containing all Gerber items.
     * @return EDA_RECT - the full item list bounding box
     */
    EDA_RECT ComputeBoundingBox();

    /**
     * Function GetBoundingBox
     * may be called soon after ComputeBoundingBox() to return the same EDA_RECT,
     * as long as the CLASS_GBR_LAYOUT has not changed.
     */
    EDA_RECT GetBoundingBox() const { return m_BoundingBox; }    // override

    void SetBoundingBox( const EDA_RECT& aBox ) { m_BoundingBox = aBox; }

    /**
     * Function Draw.
     * Redraw the CLASS_GBR_LAYOUT items but not cursors, axis or grid.
     * @param aPanel = the panel relative to the board
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ... (not always used)
     * @param aOffset = an draw offset value
     * @param aPrintBlackAndWhite = true to force black and white insdeat of color
     *        useful only to print/plot gebview layers
     */
    void Draw( EDA_DRAW_PANEL* aPanel,
               wxDC* aDC,
               GERBER_DISPLAY_OPTIONS& aDisplayOptions,
               const std::vector<GERBER_IMAGE*>& aLayers,
               GERBER_IMAGE* selectedLayer,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset,
               bool aPrintBlackAndWhite = false );

    void DrawItemsDCodeID( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor );

#if defined(DEBUG)
    void    Show( int nestLevel, std::ostream& os ) const;  // overload
#endif

    int AddGerber( GERBER_IMAGE* aGbrImage );
    void MoveLayerUp( int aIdx );
    void MoveLayerDown( int aIdx );
    void SortGerbersByZOrder();
    void DeleteGerber( int aIdx );
    GERBER_IMAGE* GetGerberByListIndex( int aIdx );

    const std::vector<GERBER_IMAGE*>& GetGerbers()
    {
        return m_Gerbers;
    }

    GERBER_IMAGE* GetGerberById( int layerID );
    int GetGerberIndexByLayer( int layerID );
    void DeleteAllGerbers( void );
};

#endif      // #ifndef CLASS_GBR_LAYOUT_H
