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


#ifndef CLASS_GERBER_DISPLAY_OPTIONS_H
#define CLASS_GERBER_DISPLAY_OPTIONS_H

/**
 * Class GBR_DISPLAY_OPTIONS
 * A helper class to handle display options.
 */
class GERBER_DISPLAY_OPTIONS
{
public:
    bool    m_DisplayFlashedItemsFill;
    bool    m_DisplayLinesFill;
    bool    m_DisplayPolygonsFill;
    bool    m_DisplayPolarCood;
    bool    m_DisplayDCodes;
    bool    m_DisplayNegativeObjects;
    bool    m_IsPrinting;
    EDA_COLOR_T m_NegativeObjectColor;
    EDA_COLOR_T m_BackgroundColor;
    bool    m_DrawBlackAndWhite;

public:
    GERBER_DISPLAY_OPTIONS()
    {
        m_DisplayFlashedItemsFill = true;
        m_DisplayLinesFill      = true;
        m_DisplayPolygonsFill   = true;
        m_DisplayPolarCood      = false;
        m_DisplayDCodes = false;
        m_IsPrinting = false;
        m_DisplayNegativeObjects = false;
        m_DrawBlackAndWhite = false;
    }
};

#endif //CLASS_GERBER_DISPLAY_OPTIONS_H
