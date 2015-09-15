
#ifndef _CLASS_GERBER_IMAGE_LIST_H_
#define _CLASS_GERBER_IMAGE_LIST_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <vector>
#include <class_GERBER.h>

/**
 * @brief GERBER_IMAGE_LIST is a helper class to handle a list of GERBER_IMAGE files
 * which are loaded and can be displayed
 */
class GERBER_IMAGE_LIST
{

    unsigned m_nextLayerId;

public:
    GERBER_IMAGE_LIST();
    ~GERBER_IMAGE_LIST();

    // the list of loaded images (1 image = 1 gerber file)
    std::vector<GERBER_IMAGE*> m_GERBER_List;

    //Accessor
    GERBER_IMAGE* GetGerberByListIndex( int aIdx );

    /**
     * Add a GERBER_IMAGE* at index aIdx
     * or at the first free location if aIdx < 0
     * @param aGbrImage = the image to add
     * @param aIdx = the location to use ( 0 ... GERBER_DRAWLAYERS_COUNT-1 )
     * @return true if the index used, or -1 if no room to add image
     */
    int AddGbrImage( GERBER_IMAGE* aGbrImage );

    /**
     * Add a GERBER_IMAGE* at index aIdx
     * or at the first free location if aIdx < 0
     * @param aGbrImage = the image to add
     * @param aIdx = the location to use ( 0 ... GERBER_DRAWLAYERS_COUNT-1 )
     * @return true if the index used, or -1 if no room to add image
     */
    int ReplaceGbrImage( int aIdx, GERBER_IMAGE* aGbrImage );

    /**
     * @return Number of images in list
     */
    size_t GetImageCount()
    {
        return m_GERBER_List.size();
    }

    /**
     * remove all loaded data in list
     */
    void ClearList();

    /**
     * remove the loaded data of image aIdx
     * @param aIdx = the index ( 0 ... GERBER_DRAWLAYERS_COUNT-1 )
     */
    void ClearImage( int aIdx );

    /**
     * remove the loaded data of image aIdx
     * @param aIdx = the index ( 0 ... GERBER_DRAWLAYERS_COUNT-1 )
     */
    void RemoveImage( int aIdx );

    /**
     * @return true if image is used (loaded and with items)
     * @param aIdx = the index ( 0 ... GERBER_DRAWLAYERS_COUNT-1 )
     */
    bool IsUsed( int aIdx );

    /**
     * Sort loaded images by Z order priority, if they have the X2 FileFormat info
     * @param aDrawList: the draw list associated to the gerber images
     * (SortImagesByZOrder updates the graphic layer of these items)
     */
    void SortImagesByZOrder();

    void MoveLayerUp( int aIdx );
    void MoveLayerDown( int aIdx );

    GERBER_IMAGE* GetGerberById( int layerID );

    int GetGerberIndexByLayer( int layerID );
};


extern GERBER_IMAGE_LIST g_GERBER_List;

#endif  // ifndef _CLASS_GERBER_IMAGE_LIST_H_