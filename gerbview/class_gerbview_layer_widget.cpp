/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_gerbview_layer_widget.cpp
 * @brief  GerbView layers manager.
 */

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/window.h>

#include <common.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <class_gbr_layer_box_selector.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_image.h>
#include <layer_widget.h>
#include <class_gerbview_layer_widget.h>
#include <class_gerber_image_list.h>


/*
 * Class GERBER_LAYER_WIDGET
 * is here to implement the abtract functions of LAYER_WIDGET so they
 * may be tied into the GERBVIEW_FRAME's data and so we can add a popup
 * menu which is specific to Pcbnew's needs.
 */


GERBER_LAYER_WIDGET::GERBER_LAYER_WIDGET( GERBVIEW_FRAME* aParent, wxWindow* aFocusOwner,
                                          int aPointSize ) :
    LAYER_WIDGET( aParent, aFocusOwner, aPointSize ),
    myframe( aParent )
{
    m_alwaysShowActiveLayer = false;

    ReFillRender();

    // Update default tabs labels for GerbView
    SetLayersManagerTabsText( );

    //-----<Popup menu>-------------------------------------------------
    // handle the poawpup menu over the layer window.
    m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN,
        wxMouseEventHandler( GERBER_LAYER_WIDGET::onRightDownLayers ), NULL, this );

    // since Popupmenu() calls this->ProcessEvent() we must call this->Connect()
    // and not m_LayerScrolledWindow->Connect()
    Connect( ID_LAYER_MANAGER_START, ID_LAYER_MANAGER_END,
        wxEVT_COMMAND_MENU_SELECTED,
        wxCommandEventHandler( GERBER_LAYER_WIDGET::onPopupSelection ), NULL, this );

    // install the right click handler into each control at end of ReFill()
    // using installRightLayerClickHandler
}

/**
 * Function SetLayersManagerTabsText
 * Update the layer manager tabs labels
 * Useful when changing Language or to set labels to a non default value
 */
void GERBER_LAYER_WIDGET::SetLayersManagerTabsText( )
{
    m_notebook->SetPageText(0, _("Layer") );
    m_notebook->SetPageText(1, _("Render") );
}

/**
 * Function ReFillRender
 * Rebuild Render for instance after the config is read
 */
void GERBER_LAYER_WIDGET::ReFillRender()
{
    ClearRenderRows();

    // Fixed "Rendering" tab rows within the LAYER_WIDGET, only the initial color
    // is changed before appending to the LAYER_WIDGET.  This is an automatic variable
    // not a static variable, change the color & state after copying from code to renderRows
    // on the stack.
    LAYER_WIDGET::ROW renderRows[3] = {

#define RR  LAYER_WIDGET::ROW   // Render Row abreviation to reduce source width

             // text                id                      color       tooltip                 checked
        RR( _( "Grid" ),            GERBER_GRID_VISIBLE,    WHITE,      _( "Show the (x,y) grid dots" ) ),
        RR( _( "DCodes" ),          DCODES_VISIBLE,         WHITE,      _( "Show DCodes identification" ) ),
        RR( _( "Neg. Obj." ),       NEGATIVE_OBJECTS_VISIBLE,  DARKGRAY,
                                    _( "Show negative objects in this color" ) ),
    };

    for( unsigned row=0;  row<DIM(renderRows);  ++row )
    {
        if( renderRows[row].color != -1 )       // does this row show a color?
        {
            renderRows[row].color = myframe->GetVisibleElementColor(
                                    (GERBER_VISIBLE_ID)renderRows[row].id );
        }
        renderRows[row].state = myframe->IsElementVisible(
                                (GERBER_VISIBLE_ID)renderRows[row].id );
    }

    AppendRenderRows( renderRows, DIM(renderRows) );
}

void GERBER_LAYER_WIDGET::installRightLayerClickHandler()
{
    int rowCount = GetLayerRowCount();
    for( int row=0;  row<rowCount;  ++row )
    {
        for( int col=0; col<LYR_COLUMN_COUNT;  ++col )
        {
            wxWindow* w = getLayerComp( row, col );

            w->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler(
                GERBER_LAYER_WIDGET::onRightDownLayerRow ), NULL, this );
        }
    }
}


void GERBER_LAYER_WIDGET::onRightDownLayers( wxMouseEvent& event )
{
    wxMenu          menu;

    baseRightClickMenu(menu);

    PopupMenu( &menu );

    passOnFocus();
}


void GERBER_LAYER_WIDGET::onRightDownLayerRow( wxMouseEvent& event )
{
    wxMenu          menu;

    wxWindow* w = (wxWindow*) event.GetEventObject();
    int rowId = getDecodedId( w->GetId() );

    if( g_GERBER_List.GetImageCount() > 1) //are there more than 1 layer?
    {
        if( rowId != 0 ) // layer id 0 is "top"
        {
            menu.Append( new wxMenuItem( &menu, ID_LAYER_MOVE_UP,
                                         _("Move up") ) );

        }

        if( rowId != g_GERBER_List.GetImageCount()-1 ) // not end of layer list
        {
            menu.Append( new wxMenuItem( &menu, ID_LAYER_MOVE_DOWN,
                                         _("Move down") ) );
        }

        menu.AppendSeparator();
    }

    menu.SetRefData(new LAYER_WIDGT_ROW(rowId));

    menu.Append( new wxMenuItem( &menu, ID_LAYER_DELETE,
                                 _("Delete layer") ) );
    menu.AppendSeparator();

    baseRightClickMenu(menu);

    PopupMenu( &menu );

    passOnFocus();
}


void GERBER_LAYER_WIDGET::baseRightClickMenu( wxMenu& aMenu )
{
    aMenu.Append( new wxMenuItem( &aMenu, ID_SHOW_ALL_LAYERS,
                                 _("Show All Layers") ) );

    aMenu.Append( new wxMenuItem( &aMenu, ID_SHOW_NO_LAYERS_BUT_ACTIVE,
                                 _( "Hide All Layers But Active" ) ) );

    aMenu.Append( new wxMenuItem( &aMenu, ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE,
                                 _( "Always Hide All Layers But Active" ) ) );

    aMenu.Append( new wxMenuItem( &aMenu, ID_SHOW_NO_LAYERS,
                                 _( "Hide All Layers" ) ) );

    aMenu.AppendSeparator();
    aMenu.Append( new wxMenuItem( &aMenu, ID_SORT_GBR_LAYERS,
                                 _( "Sort Layers if X2 Mode" ) ) );
}


void GERBER_LAYER_WIDGET::onPopupSelection( wxCommandEvent& event )
{
    int  rowCount;
    int  menuId = event.GetId();
    bool visible = (menuId == ID_SHOW_ALL_LAYERS);
    bool force_active_layer_visible;

    m_alwaysShowActiveLayer = ( menuId == ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE );
    force_active_layer_visible = ( menuId == ID_SHOW_NO_LAYERS_BUT_ACTIVE ||
                                   menuId == ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE );

    switch( menuId )
    {
    case ID_SHOW_ALL_LAYERS:
    case ID_SHOW_NO_LAYERS:
    case ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE:
    case ID_SHOW_NO_LAYERS_BUT_ACTIVE:
        rowCount = GetLayerRowCount();
        for( int row=0; row < rowCount; ++row )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
            int layer = getDecodedId( cb->GetId() );
            bool loc_visible = visible;

            if( force_active_layer_visible && (layer == myframe->getActiveLayer() ) )
                loc_visible = true;

            cb->SetValue( loc_visible );
        }

        myframe->GetCanvas()->Refresh();
        break;
    case ID_LAYER_MOVE_UP:
    case ID_LAYER_MOVE_DOWN:
        {
            wxMenu* menu = (wxMenu*) event.GetEventObject();
            LAYER_WIDGT_ROW* const layerRowData = (LAYER_WIDGT_ROW*)menu->GetRefData();

            if( menuId == ID_LAYER_MOVE_UP )
                g_GERBER_List.MoveLayerUp(layerRowData->m_row);
            else
                g_GERBER_List.MoveLayerDown(layerRowData->m_row);

            myframe->ReFillLayerWidget();
            myframe->syncLayerBox();
            myframe->GetCanvas()->Refresh();
        }
        break;
    case ID_LAYER_DELETE:
        {
            wxMenu* menu = (wxMenu*) event.GetEventObject();
            LAYER_WIDGT_ROW* const layerRowData = (LAYER_WIDGT_ROW*)menu->GetRefData();
            g_GERBER_List.RemoveImage(layerRowData->m_row);

            myframe->ReFillLayerWidget();
            myframe->syncLayerBox();
            myframe->GetCanvas()->Refresh();
        }
            break;
    case ID_SORT_GBR_LAYERS:
        g_GERBER_List.SortImagesByZOrder();
        myframe->ReFillLayerWidget();
        myframe->syncLayerBox();
        myframe->GetCanvas()->Refresh();
        break;
    }
}


bool GERBER_LAYER_WIDGET::OnLayerSelected()
{
    if( !m_alwaysShowActiveLayer )
        return false;

    // postprocess after active layer selection
    // ensure active layer visible
    wxCommandEvent event;
    event.SetId( ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE );
    onPopupSelection( event );
    return true;
}


void GERBER_LAYER_WIDGET::ReFill()
{
    Freeze();

    ClearLayerRows();

    for (std::vector<GERBER_IMAGE*>::iterator git = g_GERBER_List.m_GERBER_List.begin(); git != g_GERBER_List.m_GERBER_List.end(); ++git)
    {
        GERBER_IMAGE* gerber = *git;
        wxString msg = gerber->GetDisplayName();

        int layer = git-g_GERBER_List.m_GERBER_List.begin();

        AppendLayerRow( LAYER_WIDGET::ROW( msg, layer,
                        myframe->GetLayerColor( layer ), wxEmptyString, gerber->m_Visible ) );
    }

    Thaw();

    installRightLayerClickHandler();
}

//-----<LAYER_WIDGET callbacks>-------------------------------------------

void GERBER_LAYER_WIDGET::OnLayerColorChange( int aLayer, EDA_COLOR_T aColor )
{
    myframe->SetLayerColor( aLayer, aColor );
    myframe->m_SelLayerBox->ResyncBitmapOnly();
    myframe->GetCanvas()->Refresh();
}

bool GERBER_LAYER_WIDGET::OnLayerSelect( int aLayer )
{
    // the layer change from the GERBER_LAYER_WIDGET can be denied by returning
    // false from this function.
    int layer = myframe->getActiveLayer( );
    myframe->setActiveLayer( aLayer, false );
    myframe->syncLayerBox();

    if( layer != myframe->getActiveLayer( ) )
    {
        if( ! OnLayerSelected() )
            myframe->GetCanvas()->Refresh();
    }

    return true;
}

void GERBER_LAYER_WIDGET::OnLayerVisible( int aLayer, bool isVisible, bool isFinal )
{
    GERBER_IMAGE* gerber = g_GERBER_List.GetGerberByListIndex(aLayer);

    gerber->m_Visible = isVisible;

    if( isFinal )
        myframe->GetCanvas()->Refresh();
}

void GERBER_LAYER_WIDGET::OnRenderColorChange( int aId, EDA_COLOR_T aColor )
{
    myframe->SetVisibleElementColor( (GERBER_VISIBLE_ID)aId, aColor );
    myframe->GetCanvas()->Refresh();
}

void GERBER_LAYER_WIDGET::OnRenderEnable( int aId, bool isEnabled )
{
    myframe->SetElementVisibility( (GERBER_VISIBLE_ID)aId, isEnabled );
    myframe->GetCanvas()->Refresh();
}

//-----</LAYER_WIDGET callbacks>------------------------------------------

/*
 * Virtual Function useAlternateBitmap
 * return true if bitmaps shown in Render layer list
 * must be alternate bitmaps, or false to use "normal" bitmaps
 */
bool GERBER_LAYER_WIDGET::useAlternateBitmap(int aRow)
{
    return g_GERBER_List.IsUsed( aRow );
}

/*
 * Update the layer manager icons (layers only)
 * Useful when loading a file or clearing a layer because they change
 */
void GERBER_LAYER_WIDGET::UpdateLayerIcons()
{
    int row_count = GetLayerRowCount();
    for( int row = 0; row < row_count ; row++ )
    {
        wxStaticBitmap* bm = (wxStaticBitmap*) getLayerComp( row, COLUMN_ICON_ACTIVE );
        if( bm == NULL)
            continue;

        if( row == m_CurrentRow )
            bm->SetBitmap( useAlternateBitmap(row) ? *m_RightArrowAlternateBitmap :
                           *m_RightArrowBitmap );
        else
            bm->SetBitmap( useAlternateBitmap(row) ? *m_BlankAlternateBitmap : *m_BlankBitmap );
    }
}
