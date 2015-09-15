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


#include <class_gerber_image_list.h>
#include <class_X2_gerber_attributes.h>

// GERBER_IMAGE_LIST is a helper class to handle a list of GERBER_IMAGE files
GERBER_IMAGE_LIST::GERBER_IMAGE_LIST() :
        m_nextLayerId(0)
{
}

GERBER_IMAGE_LIST::~GERBER_IMAGE_LIST()
{
    ClearList();

    for( unsigned layer = 0; layer < m_Gerbers.size(); ++layer )
    {
        delete m_Gerbers[layer];
        m_Gerbers[layer] = NULL;
    }
}

GERBER_IMAGE* GERBER_IMAGE_LIST::GetGerberByListIndex( int aIdx )
{
    if( aIdx < m_Gerbers.size() && aIdx >= 0 )
        return m_Gerbers[aIdx];

    return NULL;
}

GERBER_IMAGE* GERBER_IMAGE_LIST::GetGerberById( int layerID )
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


int GERBER_IMAGE_LIST::GetGerberIndexByLayer( int layerID )
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

int GERBER_IMAGE_LIST::AddGbrImage( GERBER_IMAGE* aGbrImage )
{
    m_Gerbers.push_back(aGbrImage);
    int idx = m_nextLayerId++;

    return idx;
}


int GERBER_IMAGE_LIST::ReplaceGbrImage( int aIdx, GERBER_IMAGE* aGbrImage )
{
    int idx = aIdx;

    if( IsUsed( idx ) )
    {
        m_Gerbers[idx] = aGbrImage;
    }

    return idx;
}


// remove all loaded data in list, but do not delete empty images
// (can be reused)
void GERBER_IMAGE_LIST::ClearList()
{
    for (std::vector<GERBER_IMAGE*>::iterator it = m_Gerbers.begin() ; it != m_Gerbers.end(); ++it)
    {
        delete (*it);
    }

    m_Gerbers.clear();

    m_nextLayerId = 0;
}

// remove the loaded data of image aIdx, but do not delete it
void GERBER_IMAGE_LIST::ClearImage( int aIdx )
{
    if( aIdx >= 0 && aIdx < (int)m_Gerbers.size() && m_Gerbers[aIdx] )
    {
        m_Gerbers[aIdx]->ClearDrawingItems();
        m_Gerbers[aIdx]->InitToolTable();
        m_Gerbers[aIdx]->ResetDefaultValues();
        m_Gerbers[aIdx]->m_InUse = false;
    }
}

// remove the loaded data of image aIdx, but do not delete it
void GERBER_IMAGE_LIST::RemoveImage( int aIdx )
{
    if( aIdx >= 0 && aIdx < (int)m_Gerbers.size() )
    {
        delete m_Gerbers[aIdx];
        m_Gerbers.erase (m_Gerbers.begin()+aIdx);
    }
}

// return true if image is used (loaded and not cleared)
bool GERBER_IMAGE_LIST::IsUsed( int aIdx )
{
    if( aIdx >= 0 && aIdx < (int)m_Gerbers.size() )
        return m_Gerbers[aIdx] != NULL && m_Gerbers[aIdx]->m_InUse;

    return false;
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

void GERBER_IMAGE_LIST::SortImagesByZOrder()
{
    std::sort( m_Gerbers.begin(), m_Gerbers.end(), sortZorder );
}


void GERBER_IMAGE_LIST::MoveLayerUp( int aIdx )
{
    if( aIdx > 0 )
    {
        std::iter_swap(m_Gerbers.begin() + aIdx-1, m_Gerbers.begin() + aIdx);
    }
}

void GERBER_IMAGE_LIST::MoveLayerDown( int aIdx )
{
    if( aIdx < m_Gerbers.size()-1 )
    {
        std::iter_swap(m_Gerbers.begin() + aIdx, m_Gerbers.begin() + aIdx+1);
    }
}
