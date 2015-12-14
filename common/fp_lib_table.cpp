/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012-2015 KiCad Developers, see change_log.txt for contributors.
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

#include <wx/chartype.h>
#include <wx/config.h>      // wxExpandEnvVars()
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/stdpaths.h>

#include <set>

//#include <pgm_base.h>
#include <kiface_i.h>
#include <search_stack.h>
#include <pcb_netlist.h>
#include <reporter.h>
#include <footprint_info.h>
#include <wildcards_and_files_ext.h>
#include <fpid.h>
#include <fp_lib_table.h>
#include <class_module.h>
#include "sexpr/sexpr_syntax_exception.h"

static const wxChar global_tbl_name[] = wxT( "fp-lib-table" );

void FP_LIB_TABLE::ROW::SetType( const wxString& aType )
{
    type = IO_MGR::EnumFromStr( aType );

    if( IO_MGR::PCB_FILE_T( -1 ) == type )
        type = IO_MGR::KICAD;
}


void FP_LIB_TABLE::ROW::SetFullURI( const wxString& aFullURI )
{
    uri_user = aFullURI;

#if !FP_LATE_ENVVAR
    uri_expanded = FP_LIB_TABLE::ExpandSubstitutions( aFullURI );
#endif
}


const wxString FP_LIB_TABLE::ROW::GetFullURI( bool aSubstituted ) const
{
    if( aSubstituted )
    {
#if !FP_LATE_ENVVAR         // early expansion
        return uri_expanded;

#else   // late expansion
        return FP_LIB_TABLE::ExpandSubstitutions( uri_user );
#endif
    }
    else
        return uri_user;
}


FP_LIB_TABLE::ROW::ROW( const ROW& a ) :
    nickName( a.nickName ),
    type( a.type ),
    options( a.options ),
    description( a.description ),
    properties( 0 )
{
    // may call ExpandSubstitutions()
    SetFullURI( a.uri_user );

    if( a.properties )
        properties = new PROPERTIES( *a.properties );
}


FP_LIB_TABLE::ROW& FP_LIB_TABLE::ROW::operator=( const ROW& r )
{
    nickName     = r.nickName;
    type         = r.type;
    options      = r.options;
    description  = r.description;
    properties   = r.properties ? new PROPERTIES( *r.properties ) : NULL;

    // may call ExpandSubstitutions()
    SetFullURI( r.uri_user );

    // Do not copy the PLUGIN, it is lazily created.  Delete any existing
    // destination plugin.
    setPlugin( NULL );

    return *this;
}


bool FP_LIB_TABLE::ROW::operator==( const ROW& r ) const
{
    return nickName == r.nickName
        && uri_user == r.uri_user
        && type == r.type
        && options == r.options
        && description == r.description
        ;
}

FP_LIB_TABLE::FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable ) :
    fallBack( aFallBackTable )
{
    // not copying fall back, simply search aFallBackTable separately
    // if "nickName not found".
}


FP_LIB_TABLE::~FP_LIB_TABLE()
{
    // *fallBack is not owned here.
}


wxArrayString FP_LIB_TABLE::FootprintEnumerate( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->FootprintEnumerate( row->GetFullURI( true ), row->GetProperties() );
}


MODULE* FP_LIB_TABLE::FootprintLoad( const wxString& aNickname, const wxString& aFootprintName )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );

    MODULE* ret = row->plugin->FootprintLoad( row->GetFullURI( true ), aFootprintName, row->GetProperties() );

    // The library cannot know its own name, because it might have been renamed or moved.
    // Therefore footprints cannot know their own library nickname when residing in
    // a footprint library.
    // Only at this API layer can we tell the footprint about its actual library nickname.
    if( ret )
    {
        // remove "const"-ness, I really do want to set nickname without
        // having to copy the FPID and its two strings, twice each.
        FPID& fpid = (FPID&) ret->GetFPID();

        // Catch any misbehaving plugin, which should be setting internal footprint name properly:
        wxASSERT( aFootprintName == (wxString) fpid.GetFootprintName() );

        // and clearing nickname
        wxASSERT( !fpid.GetLibNickname().size() );

        fpid.SetLibNickname( row->GetNickName() );
    }

    return ret;
}


FP_LIB_TABLE::SAVE_T FP_LIB_TABLE::FootprintSave( const wxString& aNickname, const MODULE* aFootprint, bool aOverwrite )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );

    if( !aOverwrite )
    {
        // Try loading the footprint to see if it already exists, caller wants overwrite
        // protection, which is atypical, not the default.

        wxString fpname = aFootprint->GetFPID().GetFootprintName();

        std::auto_ptr<MODULE>   m( row->plugin->FootprintLoad( row->GetFullURI( true ), fpname, row->GetProperties() ) );

        if( m.get() )
            return SAVE_SKIPPED;
    }

    row->plugin->FootprintSave( row->GetFullURI( true ), aFootprint, row->GetProperties() );

    return SAVE_OK;
}


void FP_LIB_TABLE::FootprintDelete( const wxString& aNickname, const wxString& aFootprintName )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->FootprintDelete( row->GetFullURI( true ), aFootprintName, row->GetProperties() );
}


bool FP_LIB_TABLE::IsFootprintLibWritable( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    return row->plugin->IsFootprintLibWritable( row->GetFullURI( true ) );
}


void FP_LIB_TABLE::FootprintLibDelete( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->FootprintLibDelete( row->GetFullURI( true ), row->GetProperties() );
}


void FP_LIB_TABLE::FootprintLibCreate( const wxString& aNickname )
{
    const ROW* row = FindRow( aNickname );
    wxASSERT( (PLUGIN*) row->plugin );
    row->plugin->FootprintLibCreate( row->GetFullURI( true ), row->GetProperties() );
}


const wxString FP_LIB_TABLE::GetDescription( const wxString& aNickname )
{
    // use "no exception" form of find row:
    const ROW* row = findRow( aNickname );
    if( row )
        return row->description;
    else
        return wxEmptyString;
}

void FP_LIB_TABLE::Parse( const std::string& sexpr )
{
    SEXPR::PARSER parser;
    std::unique_ptr<SEXPR::SEXPR> fplibroot( parser.Parse( sexpr ) );
    
    if( !fplibroot->IsList() )
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), fplibroot->GetLineNumber() );
    }
    
    if( !fplibroot->GetChild(0)->IsSymbol() ||  fplibroot->GetChild(0)->GetSymbol() != "fp_lib_table")
    {
        THROW_SEXPR_SYNTAX_EXCEPTION( _T("fp_lib_table symbol not found"), fplibroot->GetChild(0)->GetLineNumber() );
    }
    
    for(size_t i = 1; i < fplibroot->GetNumberOfChildren(); i++ )
    {
        SEXPR::SEXPR* libList = fplibroot->GetChild(i);
        
        if( !libList->IsList() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), libList->GetChild(0)->GetLineNumber() );
        }
        
        if( !libList->GetChild(0)->IsSymbol() ||  libList->GetChild(0)->GetSymbol() != "lib")
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("lib symbol not found"), libList->GetChild(0)->GetLineNumber() );
        }
        
        parseLibList( libList );
    }
}


void FP_LIB_TABLE::parseLibList( SEXPR::SEXPR* aLibList )
{
    ROW     row;
    
    for(size_t i = 1; i < aLibList->GetNumberOfChildren(); i++ )
    {
        SEXPR::SEXPR* pairList = aLibList->GetChild(i);
        
        if( !pairList->IsList() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("Expected list"), pairList->GetLineNumber() );
        }
        
        if( !pairList->GetChild(0)->IsSymbol() )
        {
            THROW_SEXPR_SYNTAX_EXCEPTION( _T("lib symbol not found"), pairList->GetLineNumber() );
        }
        
        std::string key = pairList->GetChild(0)->GetSymbol();
        
        if( key == "name" )
        {
            if( !pairList->GetChild(1)->IsSymbol() )
            {
                THROW_SEXPR_SYNTAX_EXCEPTION( _T("expected symbol"), pairList->GetLineNumber() );
            }
        
            row.SetNickName( pairList->GetChild(1)->GetSymbol() );
        }
        else if ( key == "uri" )
        {
            if( !pairList->GetChild(1)->IsSymbol() )
            {
                THROW_SEXPR_SYNTAX_EXCEPTION( _T("expected symbol"), pairList->GetLineNumber() );
            }
        
            row.SetFullURI( pairList->GetChild(1)->GetSymbol() );
        }
        else if ( key == "type" )
        {
            if( !pairList->GetChild(1)->IsSymbol() )
            {
                THROW_SEXPR_SYNTAX_EXCEPTION( _T("expected symbol"), pairList->GetLineNumber() );
            }
        
            row.SetType( pairList->GetChild(1)->GetSymbol() );
        }
        else if ( key == "options" )
        {
            if( !pairList->GetChild(1)->IsString() )
            {
                THROW_SEXPR_SYNTAX_EXCEPTION( _T("expected string"), pairList->GetLineNumber() );
            }
        
            row.SetOptions( pairList->GetChild(1)->GetString() );
        }
        else if ( key == "descr" )
        {
            /* STUPID MODE FALLBACK...
            why? some cases of fpliblist created a keyword isntead of quoted string....
            seriously */
            if( pairList->GetChild(1)->IsString() )
            {
                row.SetDescr( pairList->GetChild(1)->GetString() );
            }
            else if( pairList->GetChild(1)->IsSymbol() )
            {
                row.SetDescr( pairList->GetChild(1)->GetSymbol() );
            }
            else
            {
                THROW_SEXPR_SYNTAX_EXCEPTION( _T("expected string or symbol"), pairList->GetLineNumber() );
            }
        }
    }

    if( !InsertRow( row ) )
    {
        wxString msg = wxString::Format(
                            _( "'%s' is a duplicate footprint library nickName" ),
                            GetChars( row.nickName ) );
        THROW_SEXPR_SYNTAX_EXCEPTION( msg, aLibList->GetLineNumber() );
    }
}

void FP_LIB_TABLE::Format( OUTPUTFORMATTER* out, int nestLevel ) const
    throw( IO_ERROR, boost::interprocess::lock_exception )
{
    SEXPR::SEXPR_LIST list;
    
    list << SEXPR::AsSymbol("fp_lib_table");
    
    for( ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
    {
        list << *it;
    }
    
    std::string sexprString = list.AsString();
    out->Print( 0, sexprString.c_str() );

}

SEXPR::SEXPR* FP_LIB_TABLE::ROW::SerializeSEXPR() const
{
    SEXPR::SEXPR_LIST* nameList = new SEXPR::SEXPR_LIST();
    *nameList << SEXPR::AsSymbol("name");
    *nameList << SEXPR::AsSymbol( GetNickName().ToStdString() );
    
    SEXPR::SEXPR_LIST* typeList = new SEXPR::SEXPR_LIST();
    *typeList << SEXPR::AsSymbol("type");
    *typeList << SEXPR::AsSymbol( GetType().ToStdString() );
    
    SEXPR::SEXPR_LIST* uriList = new SEXPR::SEXPR_LIST();
    *uriList << SEXPR::AsSymbol("uri");
    *uriList << SEXPR::AsSymbol( GetFullURI().ToStdString() );
    
    SEXPR::SEXPR_LIST* optionsList = new SEXPR::SEXPR_LIST();
    *optionsList << SEXPR::AsSymbol("options");
    *optionsList << SEXPR::AsString( GetOptions().ToStdString() );
    
    SEXPR::SEXPR_LIST* descrList = new SEXPR::SEXPR_LIST();
    *descrList << SEXPR::AsSymbol("descr");
    *descrList << SEXPR::AsString( GetDescr().ToStdString() );
    
    SEXPR::SEXPR_LIST* libList = new SEXPR::SEXPR_LIST();
    *libList << SEXPR::AsSymbol("lib");
    *libList << nameList;
    *libList << typeList;
    *libList << uriList;
    *libList << optionsList;
    *libList << descrList;
    
    return libList;
}

#define OPT_SEP     '|'         ///< options separator character

PROPERTIES* FP_LIB_TABLE::ParseOptions( const std::string& aOptionsList )
{
    if( aOptionsList.size() )
    {
        const char* cp  = &aOptionsList[0];
        const char* end = cp + aOptionsList.size();

        PROPERTIES  props;
        std::string pair;

        // Parse all name=value pairs
        while( cp < end )
        {
            pair.clear();

            // Skip leading white space.
            while( cp < end && isspace( *cp )  )
                ++cp;

            // Find the end of pair/field
            while( cp < end )
            {
                if( *cp=='\\'  &&  cp+1<end  &&  cp[1]==OPT_SEP  )
                {
                    ++cp;           // skip the escape
                    pair += *cp++;  // add the separator
                }
                else if( *cp==OPT_SEP )
                {
                    ++cp;           // skip the separator
                    break;          // process the pair
                }
                else
                    pair += *cp++;
            }

            // stash the pair
            if( pair.size() )
            {
                // first equals sign separates 'name' and 'value'.
                size_t  eqNdx = pair.find( '=' );
                if( eqNdx != pair.npos )
                {
                    std::string name  = pair.substr( 0, eqNdx );
                    std::string value = pair.substr( eqNdx + 1 );
                    props[name] = value;
                }
                else
                    props[pair] = "";       // property is present, but with no value.
            }
        }

        if( props.size() )
            return new PROPERTIES( props );
    }
    return NULL;
}


UTF8 FP_LIB_TABLE::FormatOptions( const PROPERTIES* aProperties )
{
    UTF8 ret;

    if( aProperties )
    {
        for( PROPERTIES::const_iterator it = aProperties->begin();  it != aProperties->end();  ++it )
        {
            const std::string& name  = it->first;

            const UTF8& value = it->second;

            if( ret.size() )
                ret += OPT_SEP;

            ret += name;

            // the separation between name and value is '='
            if( value.size() )
            {
                ret += '=';

                for( std::string::const_iterator si = value.begin();  si != value.end();  ++si )
                {
                    // escape any separator in the value.
                    if( *si == OPT_SEP )
                        ret += '\\';

                    ret += *si;
                }
            }
        }
    }

    return ret;
}


std::vector<wxString> FP_LIB_TABLE::GetLogicalLibs()
{
    // Only return unique logical library names.  Use std::set::insert() to
    // quietly reject any duplicates, which can happen when encountering a duplicate
    // nickname from one of the fall back table(s).

    std::set<wxString>          unique;
    std::vector<wxString>       ret;
    const FP_LIB_TABLE*         cur = this;

    do
    {
        for( ROWS_CITER it = cur->rows.begin();  it!=cur->rows.end();  ++it )
        {
            unique.insert( it->nickName );
        }

    } while( ( cur = cur->fallBack ) != 0 );

    ret.reserve( unique.size() );

    // return a sorted, unique set of nicknames in a std::vector<wxString> to caller
    for( std::set<wxString>::const_iterator it = unique.begin();  it!=unique.end();  ++it )
    {
        ret.push_back( *it );
    }

    return ret;
}


FP_LIB_TABLE::ROW* FP_LIB_TABLE::findRow( const wxString& aNickName ) const
{
    FP_LIB_TABLE* cur = (FP_LIB_TABLE*) this;

    do
    {
        cur->ensureIndex();

        INDEX_CITER  it = cur->nickIndex.find( aNickName );

        if( it != cur->nickIndex.end() )
        {
            return &cur->rows[it->second];  // found
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


const FP_LIB_TABLE::ROW* FP_LIB_TABLE::FindRowByURI( const wxString& aURI )
{
    FP_LIB_TABLE* cur = this;

    do
    {
        cur->ensureIndex();

        for( unsigned i = 0;  i < cur->rows.size();  i++ )
        {
            wxString uri = cur->rows[i].GetFullURI( true );

            if( wxFileName::GetPathSeparator() == wxChar( '\\' ) && uri.Find( wxChar( '/' ) ) >= 0 )
                uri.Replace( wxT( "/" ), wxT( "\\" ) );

            if( (wxFileName::IsCaseSensitive() && uri == aURI)
              || (!wxFileName::IsCaseSensitive() && uri.Upper() == aURI.Upper() ) )
            {
                return &cur->rows[i];  // found
            }
        }

        // not found, search fall back table(s), if any
    } while( ( cur = cur->fallBack ) != 0 );

    return 0;   // not found
}


bool FP_LIB_TABLE::InsertRow( const ROW& aRow, bool doReplace )
{
    ensureIndex();

    INDEX_CITER it = nickIndex.find( aRow.nickName );

    if( it == nickIndex.end() )
    {
        rows.push_back( aRow );
        nickIndex.insert( INDEX_VALUE( aRow.nickName, rows.size() - 1 ) );
        return true;
    }

    if( doReplace )
    {
        rows[it->second] = aRow;
        return true;
    }

    return false;
}


const FP_LIB_TABLE::ROW* FP_LIB_TABLE::FindRow( const wxString& aNickname )
    throw( IO_ERROR )
{
    ROW* row = findRow( aNickname );

    if( !row )
    {
        wxString msg = wxString::Format(
            _( "fp-lib-table files contain no lib with nickname '%s'" ),
            GetChars( aNickname ) );

        THROW_IO_ERROR( msg );
    }

    // We've been 'lazy' up until now, but it cannot be deferred any longer,
    // instantiate a PLUGIN of the proper kind if it is not already in this ROW.
    if( !row->plugin )
        row->setPlugin( IO_MGR::PluginFind( row->type ) );

    return row;
}


// wxGetenv( wchar_t* ) is not re-entrant on linux.
// Put a lock on multithreaded use of wxGetenv( wchar_t* ), called from wxEpandEnvVars(),
// needed by bool ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname = NULL );
#include <ki_mutex.h>

const wxString FP_LIB_TABLE::ExpandSubstitutions( const wxString& aString )
{
    return ExpandEnvVarSubstitutions( aString );
}


bool FP_LIB_TABLE::IsEmpty( bool aIncludeFallback )
{
    if( !aIncludeFallback || !fallBack )
        return rows.empty();

    return rows.empty() && fallBack->IsEmpty( true );
}


MODULE* FP_LIB_TABLE::FootprintLoadWithOptionalNickname( const FPID& aFootprintId )
    throw( IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception )
{
    wxString   nickname = aFootprintId.GetLibNickname();
    wxString   fpname   = aFootprintId.GetFootprintName();

    if( nickname.size() )
    {
        return FootprintLoad( nickname, fpname );
    }

    // nickname is empty, sequentially search (alphabetically) all libs/nicks for first match:
    else
    {
        std::vector<wxString> nicks = GetLogicalLibs();

        // Search each library going through libraries alphabetically.
        for( unsigned i = 0;  i<nicks.size();  ++i )
        {
            // FootprintLoad() returns NULL on not found, does not throw exception
            // unless there's an IO_ERROR.
            MODULE* ret = FootprintLoad( nicks[i], fpname );
            if( ret )
                return ret;
        }

        return NULL;
    }
}


const wxString FP_LIB_TABLE::GlobalPathEnvVariableName()
{
    return  wxT( "KISYSMOD" );
}


bool FP_LIB_TABLE::LoadGlobalTable( FP_LIB_TABLE& aTable )
    throw (IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception )
{
    bool        tableExists = true;
    wxFileName  fn = GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        tableExists = false;

        if( !fn.DirExists() && !fn.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Cannot create global library table path '%s'." ),
                                              GetChars( fn.GetPath() ) ) );
        }

        // Attempt to copy the default global file table from the KiCad
        // template folder to the user's home configuration path.
        wxString fileName = Kiface().KifaceSearch().FindValidPath( global_tbl_name );

        // The fallback is to create an empty global footprint table for the user to populate.
        if( fileName.IsEmpty() || !::wxCopyFile( fileName, fn.GetFullPath(), false ) )
        {
            FP_LIB_TABLE    emptyTable;

            emptyTable.Save( fn.GetFullPath() );
        }
    }

    aTable.Load( fn.GetFullPath() );

    return tableExists;
}


wxString FP_LIB_TABLE::GetGlobalTableFileName()
{
    wxFileName fn;

    fn.SetPath( GetKicadConfigPath() );
    fn.SetName( global_tbl_name );

    return fn.GetFullPath();
}

// prefer wxString filename so it can be seen in a debugger easier than wxFileName.

void FP_LIB_TABLE::Load( const wxString& aFileName )
    throw( IO_ERROR )
{
    // It's OK if footprint library tables are missing.
    if( wxFileName::IsFileReadable( aFileName ) )
    {
        std::string sexpr = SEXPR::PARSER::GetFileContents( aFileName.ToStdString() );
        Parse( sexpr );
    }
}


void FP_LIB_TABLE::Save( const wxString& aFileName )
    const throw( IO_ERROR, boost::interprocess::lock_exception )
{
    FILE_OUTPUTFORMATTER sf( aFileName );
    Format( &sf, 0 );
}

