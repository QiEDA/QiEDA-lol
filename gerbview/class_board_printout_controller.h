/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_board_printout_controler.h
 * @brief Board print handler definition file.
 */

#ifndef CLASS_BOARD_PRINTOUT_CONTROLLER_H
#define CLASS_BOARD_PRINTOUT_CONTROLLER_H


#include <wx/dcps.h>
#include <layers_id_colors_and_visibility.h>
#include <wx/print.h>
#include <class_gerber_image.h>

#define DEFAULT_ORIENTATION_PAPER wxLANDSCAPE   // other option is wxPORTRAIT


/**
 * Class PRINT_PARAMETERS
 * handles the parameters used to print a board drawing.
 */

class PRINT_PARAMETERS
{
public:
    int    m_PenDefaultSize;                 // The default value pen size to plot/print items
    // that have no defined pen size
    double m_PrintScale;                     // general scale when printing
    double m_XScaleAdjust;                   // fine scale adjust for X axis
    double m_YScaleAdjust;                   // fine scale adjust for Y axis
    bool   m_Print_Sheet_Ref;                // Option: print page references
    std::vector<GERBER_IMAGE*> m_LayerQueue;    // layers to print
    bool   m_PrintMirror;                    // Option: Print mirrored
    bool   m_Print_Black_and_White;          // Option: Print in B&W or Color
    wxPageSetupDialogData* m_PageSetupData;  // A wxPageSetupDialogData for page options (margins)

public:
    PRINT_PARAMETERS();

    /**
     * Function CenterOnBoardOutline
     * returns true if the print should be centered by the board outline instead of the
     * paper size.
     */
    bool CenterOnBoardOutline() const
    {
        return ( (m_PrintScale > 1.0) || (m_PrintScale == 0) );
    }
};


/**
 * Class BOARD_PRINTOUT_CONTROLLER
 * is a class derived from wxPrintout to handle the necessary information to control a printer
 * when printing a board
 */
class BOARD_PRINTOUT_CONTROLLER : public wxPrintout
{
private:
    GERBVIEW_FRAME*     m_Parent;
    PRINT_PARAMETERS    m_PrintParams;

public:
    BOARD_PRINTOUT_CONTROLLER( const PRINT_PARAMETERS& aParams,
                               GERBVIEW_FRAME*         aParent,
                               const wxString&         aTitle );

    bool OnPrintPage( int aPage );

    bool HasPage( int aPage )       // do not test page num
    {
        if( aPage <= (int)m_PrintParams.m_LayerQueue.size() )
            return true;
        else
            return false;
    }

    void GetPageInfo( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo );

    void DrawPage( int aPage );
};
#endif //CLASS_BOARD_PRINTOUT_CONTROLLER_H
