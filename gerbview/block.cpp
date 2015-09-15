/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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
 * @file gerbview/block.cpp
 * @brief Block operations: displacement.
 */

#include <wx/dc.h>
#include <wx/debug.h>

#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gr_basic.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_draw_item.h>

#include <wx/debug.h>

#define BLOCK_COLOR BROWN


int GERBVIEW_FRAME::BlockCommand( int key )
{
    int cmd = 0;

    switch( key )
    {
    default:
        cmd = key & 0x255;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
    case GR_KB_CTRL:
    case GR_KB_SHIFTCTRL:
    case GR_KB_ALT:
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}

bool GERBVIEW_FRAME::HandleBlockEnd( wxDC* DC )
{
    bool nextcmd  = false;
    bool zoom_command = false;

    if( m_canvas->IsMouseCaptured() )

        switch( GetScreen()->m_BlockLocate.GetCommand() )
        {
        case BLOCK_MOVE:        // Move
        case BLOCK_ZOOM: /* Window Zoom */
            zoom_command = true;
            break;

        default:
            wxFAIL_MSG( wxT("HandleBlockEnd: Unexpected block command") );
            break;
        }

    if( ! nextcmd )
    {
        GetScreen()->ClearBlockCommand();
        m_canvas->EndMouseCapture( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString,
                                   false );
    }

    if( zoom_command )
        Window_Zoom( GetScreen()->m_BlockLocate );

    return nextcmd ;
}