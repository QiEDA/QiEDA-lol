/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#ifndef  WORKSHEET_LAYOUT_H
#define  WORKSHEET_LAYOUT_H

#include <math/vector2d.h>
#include <eda_text.h>
#include <class_bitmap_base.h>
#include "page_layout/worksheet_dataitem.h"

/**
 * WORKSHEET_LAYOUT handles the graphic items list to draw/plot
 * the title block and other items (page references ...
 */
class WORKSHEET_LAYOUT
{
    std::vector <WORKSHEET_DATAITEM*> m_list;
    bool m_allowVoidList;   // If false, the default page layout
                            // will be loaded the first time
                            // WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList
                            // is run (useful mainly for page layout editor)
    double m_leftMargin;    // the left page margin in mm
    double m_rightMargin;   // the right page margin in mm
    double m_topMargin;     // the top page margin in mm
    double m_bottomMargin;  // the bottom page margin in mm

public:
    WORKSHEET_LAYOUT();
    ~WORKSHEET_LAYOUT() {ClearList(); }

    /**
     * static function: returns the instance of WORKSHEET_LAYOUT
     * used in the application
     */
    static WORKSHEET_LAYOUT& GetTheInstance();

    /**
     * static function: Set an alternate instance of WORKSHEET_LAYOUT
     * mainly used in page setting dialog
     * @param aLayout = the alternate page layout.
     * if null, restore the basic page layout
     */
    static void SetAltInstance( WORKSHEET_LAYOUT* aLayout = NULL );

    // Accessors:
    double GetLeftMargin() { return m_leftMargin; }
    double GetRightMargin() { return m_rightMargin; }
    double GetTopMargin() { return m_topMargin; }
    double GetBottomMargin() { return m_bottomMargin; }

    void SetLeftMargin( double aMargin );
    void SetRightMargin( double aMargin );
    void SetTopMargin( double aMargin );
    void SetBottomMargin( double aMargin );

    /**
     * In Kicad applications, a page layout description is needed
     * So if the list is empty, a default description is loaded,
     * the first time a page layout is drawn.
     * However, in page layout editor, an empty list is acceptable.
     * AllowVoidList allows or not the empty list
     */
    void AllowVoidList( bool Allow ) { m_allowVoidList = Allow; }

    /**
     * @return true if an empty list is allowed
     * (mainly allowed for page layout editor).
     */
    bool VoidListAllowed() { return m_allowVoidList; }

    /**
     * erase the list of items
     */
    void ClearList();

    /**
     * Save the description in a file
     * @param aFullFileName the filename of the file to created
     */
    void Save( const wxString& aFullFileName );

    /**
     * Save the description in a buffer
     * @param aOutputString = a wxString to store the S expr string
     */
    void SaveInString( wxString& aOutputString );

    /**
     * Add an item to the list of items
     */
    void Append( WORKSHEET_DATAITEM* aItem )
    {
        m_list.push_back( aItem );
    }

    /**
     *Insert an item to the list of items at position aIdx
     */
    void Insert( WORKSHEET_DATAITEM* aItem, unsigned aIdx );

    /**
     *Remove the item to the list of items at position aIdx
     */
    bool  Remove( unsigned aIdx );

    /**
     *Remove the item to the list of items at position aIdx
     */
    bool  Remove( WORKSHEET_DATAITEM* aItem );

    /**
     * @return the index of aItem, or -1 if does not exist
     */
    int GetItemIndex( WORKSHEET_DATAITEM* aItem ) const;

    /**
     * @return the item from its index aIdx, or NULL if does not exist
     */
    WORKSHEET_DATAITEM* GetItem( unsigned aIdx ) const;

    /**
     * @return the item count
     */
    unsigned GetCount() const { return m_list.size(); }

    /**
     * Fills the list with the default layout shape
     */
    void SetDefaultLayout();

    /**
     * Populates the list with a custom layout, or
     * the default layout, if no custom layout available
     * @param aFullFileName = the custom page layout description file.
     * if empty, uses the default internal description
     * @param Append = if true: do not delete old layout, and load only
       aFullFileName.
     */
    void SetPageLayout( const wxString& aFullFileName = wxEmptyString,
                        bool Append = false );

    /**
     * Populates the list from a S expr description stored in a string
     * @param aPageLayout = the S expr string
     * @param Append Do not delete old layout if true and append \a aPageLayout
     *               the existing one.
     */
    void SetPageLayout( const char* aPageLayout, bool Append = false );

    /**
     * @return a short filename  from a full filename:
     * if the path is the current project path, or if the path
     * is the same as kicad.pro (in template), returns the shortname
     * else do nothing and returns a full filename
     * @param aFullFileName = the full filename, which can be a relative
     * @param aProjectPath = the curr project absolute path (can be empty)
     */
    static const wxString MakeShortFileName( const wxString& aFullFileName,
                                             const wxString& aProjectPath );

    /**
     * Static function
     * @return a full filename from a short filename.
     * @param aShortFileName = the short filename, which can be a relative
     * @param aProjectPath = the curr project absolute path (can be empty)
     * or absolute path, and can include env variable reference ( ${envvar} expression )
     * if the short filename path is relative, it is expected relative to the project path
     * or (if aProjectPath is empty or if the file does not exist)
     * relative to kicad.pro (in template)
     * If aShortFileName is absolute return aShortFileName
     */
    static const wxString MakeFullFileName( const wxString& aShortFileName,
                                            const wxString& aProjectPath );
};

#endif