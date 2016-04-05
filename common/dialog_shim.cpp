
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <dialog_shim.h>
#include <kiway_player.h>
#include <wx/evtloop.h>
#include <pgm_base.h>


/// Toggle a window's "enable" status to disabled, then enabled on destruction.
class WDO_ENABLE_DISABLE
{
    wxWindow* m_win;

public:

    WDO_ENABLE_DISABLE( wxWindow* aWindow ) :
        m_win( aWindow )
    {
        if( m_win )
            m_win->Disable();
    }

    ~WDO_ENABLE_DISABLE()
    {
        if( m_win )
        {
            m_win->Enable();
            m_win->SetFocus(); // let's focus back on the parent window
        }
    }
};


DIALOG_SHIM::DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style, const wxString& name ) :
    wxDialog( aParent, id, title, pos, size, style, name ),
    KIWAY_HOLDER( 0 ),
    m_qmodal_loop( 0 ),
    m_qmodal_showing( false ),
    m_qmodal_parent_disabler( 0 )
{
    // pray that aParent is either a KIWAY_PLAYER or DIALOG_SHIM derivation.
    KIWAY_HOLDER* h = dynamic_cast<KIWAY_HOLDER*>( aParent );

    // wxASSERT_MSG( h, wxT( "DIALOG_SHIM's parent is NULL or not derived from KIWAY_PLAYER nor DIALOG_SHIM" ) );

    if( h )
        SetKiway( this, &h->Kiway() );

#ifdef __WINDOWS__
    // On Windows, the app top windows can be brought to the foreground
    // (at least temporary) in certain circumstances,
    // for instance when calling an external tool in Eeschema boom generation.
    // So set the parent KIWAY_PLAYER kicad frame (if exists) to top window
    // to avoid this annoying behavior
    KIWAY_PLAYER* parent_kiwayplayer = dynamic_cast<KIWAY_PLAYER*>( aParent );

    if( parent_kiwayplayer )
        Pgm().App().SetTopWindow( parent_kiwayplayer );
#endif

#if DLGSHIM_USE_SETFOCUS
    Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_SHIM::onInit ) );
#endif
}


DIALOG_SHIM::~DIALOG_SHIM()
{
    // if the dialog is quasi-modal, this will end its event loop
    if( IsQuasiModal() )
        EndQuasiModal( wxID_CANCEL );

    delete m_qmodal_parent_disabler;    // usually NULL by now
}


// our hashtable is an implementation secret, don't need or want it in a header file
#include <hashtables.h>
#include <base_struct.h>        // EDA_RECT
#include <typeinfo>

static RECT_MAP class_map;

bool DIALOG_SHIM::Show( bool show )
{
    bool        ret;
    const char* hash_key;

    if( m_hash_key.size() )
    {
        // a special case like EDA_LIST_DIALOG, which has multiple uses.
        hash_key = m_hash_key.c_str();
    }
    else
    {
        hash_key = typeid(*this).name();
    }

    // Show or hide the window.  If hiding, save current position and size.
    // If showing, use previous position and size.
    if( show )
    {
        wxDialog::Raise();  // Needed on OS X and some other window managers (i.e. Unity)
        ret = wxDialog::Show( show );

        // classname is key, returns a zeroed out default EDA_RECT if none existed before.
        EDA_RECT r = class_map[ hash_key ];

        if( r.GetSize().x != 0 && r.GetSize().y != 0 )
            SetSize( r.GetPosition().x, r.GetPosition().y, r.GetSize().x, r.GetSize().y, 0 );
    }
    else
    {
        // Save the dialog's position & size before hiding, using classname as key
        EDA_RECT  r( wxDialog::GetPosition(), wxDialog::GetSize() );
        class_map[ hash_key ] = r;

        ret = wxDialog::Show( show );
    }
    return ret;
}


bool DIALOG_SHIM::Enable( bool enable )
{
    // so we can do logging of this state change:

#if defined(DEBUG)
    const char* type_id = typeid( *this ).name();
    printf( "wxDialog %s: %s\n", type_id, enable ? "enabled" : "disabled" );
#endif

    return wxDialog::Enable( enable );
}


#if DLGSHIM_USE_SETFOCUS

static bool findWindowRecursively( const wxWindowList& children, const wxWindow* wanted )
{
    for( wxWindowList::const_iterator it = children.begin();  it != children.end();  ++it )
    {
        const wxWindow* child = *it;

        if( wanted == child )
            return true;
        else
        {
            if( findWindowRecursively( child->GetChildren(), wanted ) )
                return true;
        }
    }

    return false;
}


static bool findWindowRecursively( const wxWindow* topmost, const wxWindow* wanted )
{
    // wanted may be NULL and that is ok.

    if( wanted == topmost )
        return true;

    return findWindowRecursively( topmost->GetChildren(), wanted );
}


/// Set the focus if it is not already set in a derived constructor to a specific control.
void DIALOG_SHIM::onInit( wxInitDialogEvent& aEvent )
{
    wxWindow* focusWnd = wxWindow::FindFocus();

    // If focusWnd is not already this window or a child of it, then SetFocus().
    // Otherwise the derived class's constructor SetFocus() already to a specific
    // child control.

    if( !findWindowRecursively( this, focusWnd ) )
    {
        // Linux wxGTK needs this to allow the ESCAPE key to close a wxDialog window.
        SetFocus();
    }

    aEvent.Skip();     // derived class's handler should be called too
}
#endif


/*
    Quasi-Modal Mode Explained:

    The gtk calls in wxDialog::ShowModal() cause event routing problems if that
    modal dialog then tries to use KIWAY_PLAYER::ShowModal().  The latter shows up
    and mostly works but does not respond to the window decoration close button.
    There is no way to get around this without reversing the gtk calls temporarily.

    Quasi-Modal mode is our own almost modal mode which disables only the parent
    of the DIALOG_SHIM, leaving other frames operable and while staying captured in the
    nested event loop.  This avoids the gtk calls and leaves event routing pure
    and sufficient to operate the KIWAY_PLAYER::ShowModal() properly.  When using
    ShowQuasiModal() you have to use EndQuasiModal() in your dialogs and not
    EndModal().  There is also IsQuasiModal() but its value can only be true
    when the nested event loop is active.  Do not mix the modal and quasi-modal
    functions.  Use one set or the other.

    You might find this behavior preferable over a pure modal mode, and it was said
    that only the Mac has this natively, but now other platforms have something
    similar.  You CAN use it anywhere for any dialog.  But you MUST use it when
    you want to use KIWAY_PLAYER::ShowModal() from a dialog event.
*/



/*
/// wxEventLoopActivator but with a friend so it
/// has access to m_evtLoopOld, and it does not SetActive() as that is
/// done inside base class Run().
class ELOOP_ACTIVATOR
{
    friend class EVENT_LOOP;

public:
    ELOOP_ACTIVATOR( WX_EVENT_LOOP* evtLoop )
    {
        m_evtLoopOld = wxEventLoopBase::GetActive();
        // wxEventLoopBase::SetActive( evtLoop );
    }

    ~ELOOP_ACTIVATOR()
    {
        // restore the previously active event loop
        wxEventLoopBase::SetActive( m_evtLoopOld );
    }

private:
    WX_EVENT_LOOP* m_evtLoopOld;
};
*/


class EVENT_LOOP : public WX_EVENT_LOOP
{
public:

    EVENT_LOOP()
    {
        ;
    }

    ~EVENT_LOOP()
    {
    }
};


int DIALOG_SHIM::ShowQuasiModal()
{
    // This is an exception safe way to zero a pointer before returning.
    // Yes, even though DismissModal() clears this first normally, this is
    // here in case there's an exception before the dialog is dismissed.
    struct NULLER
    {
        void*&  m_what;
        NULLER( void*& aPtr ) : m_what( aPtr ) {}
        ~NULLER() { m_what = 0; }   // indeed, set it to NULL on destruction
    } clear_this( (void*&) m_qmodal_loop );

    // release the mouse if it's currently captured as the window having it
    // will be disabled when this dialog is shown -- but will still keep the
    // capture making it impossible to do anything in the modal dialog itself
    wxWindow* win = wxWindow::GetCapture();
    if( win )
        win->ReleaseMouse();

    // Get the optimal parent
    wxWindow* parent = GetParentForModalDialog( GetParent(), GetWindowStyle() );

    // Show the optimal parent
#ifdef DEBUG
    if( parent ) 
        printf( "%s: optimal parent: %s\n", __func__, typeid(*parent).name() );
#endif

    wxASSERT_MSG( !m_qmodal_parent_disabler,
            wxT( "Caller using ShowQuasiModal() twice on same window?" ) );

    // quasi-modal: disable only my "optimal" parent
    m_qmodal_parent_disabler = new WDO_ENABLE_DISABLE( parent );

    Show( true );

    m_qmodal_showing = true;

    EVENT_LOOP event_loop;

    m_qmodal_loop = &event_loop;

    event_loop.Run();

    return GetReturnCode();
}


void DIALOG_SHIM::EndQuasiModal( int retCode )
{
    // Hook up validator and transfer data from controls handling so quasi-modal dialogs
    // handle validation in the same way as other dialogs.
    if( ( retCode == wxID_OK ) && ( !Validate() || !TransferDataFromWindow() ) )
        return;

    SetReturnCode( retCode );

    if( !IsQuasiModal() )
    {
        wxFAIL_MSG( wxT( "either DIALOG_SHIM::EndQuasiModal called twice or ShowQuasiModal wasn't called" ) );
        return;
    }

    m_qmodal_showing = false;

    if( m_qmodal_loop )
    {
        if( m_qmodal_loop->IsRunning() )
            m_qmodal_loop->Exit( 0 );
        else
            m_qmodal_loop->ScheduleExit( 0 );

        m_qmodal_loop = NULL;
    }

    delete m_qmodal_parent_disabler;
    m_qmodal_parent_disabler = 0;

    Show( false );
}
