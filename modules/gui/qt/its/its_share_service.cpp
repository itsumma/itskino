#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "its_share_service.hpp"
#include "its_dialogs_service.hpp"

#include "input_manager.hpp"

namespace its {

ShareService* ShareService::_instance = NULL;
intf_thread_t* ShareService::_p_intf = NULL;

ShareService::ShareService( intf_thread_t *p_intf )
{
    ShareService::_p_intf = p_intf;

    _its_random_user = var_InheritBool( p_intf, "qt-its-random-user" );

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::ShareService()" );
    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::ShareService() -> qt-its-random-user: (%i)", _its_random_user );

    loadSettings();

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::ShareService() -> user uuid: (%s)", _user_uuid.toString().toStdString().c_str() );
    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::ShareService() -> api host: (%s)", _api_host.toStdString().c_str() );
    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::ShareService() -> sync directory: (%s)", _sync_dir.toStdString().c_str() );

    _api = new API( _p_intf, _api_host );

    registerProtocol();

    saveSettins();
}

ShareService::~ShareService()
{
}

void ShareService::sendForceStopEvent( QString sessionHash )
{
    ITSEvent *event = new ITSEvent( ITSEvent::ForceStop);
    event->sessionHash = sessionHash;
    QApplication::postEvent( this, event );
}

void ShareService::forceStopSession( QString sessionHash )
{
    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::forceStopSession()" );

    sessionHash = sessionHash.isEmpty() ? getCurrentItemShareHash() : sessionHash;

    if( sessionHash.isEmpty() )
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::forceStopSession() -> sessionHash is empty" );
        return;
    }

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::forceStopSession()) -> stop item hash: (%s)", sessionHash.toStdString().c_str());

    DialogsService::getInstance()->showBusy();

    APIResponseStatus status = _api->stopSync( _user_uuid, sessionHash );

    DialogsService::getInstance()->hideBusy();

    if(status.success)
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::forceStopSession() -> stop api call success.");
        unregisterShareItemByHash(sessionHash);
    }
    else
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService:forceStopSession() -> stop api call failed. (%s)", status.errorDescription.toStdString().c_str() );
    }
}

void ShareService::loadSettings()
{
    msg_Dbg(its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::loadSettings()");

    QSettings settings(
#ifdef _WIN32
        QSettings::IniFormat,
#else
        QSettings::NativeFormat,
#endif
        QSettings::UserScope, "vlc", "vlc-qt-its" );

    settings.beginGroup( "Account" );

    //userUuid
    _user_uuid = settings.value( "Uuid").toUuid();
    if ( _user_uuid.variant() == -1 )
    {
        _is_new_user = true;
        _user_uuid = QUuid::createUuid();
    }

    if( _its_random_user )
    {
        _is_new_user = true;
        _user_uuid = QUuid::createUuid();
    }

    settings.endGroup();

    settings.beginGroup( "General" );

    //apiHost
    _api_host = settings.value( "ApiHost").toString();

    if(_api_host.isEmpty())
        _api_host = "https://itskino.ru/";

    //syncDir
    _sync_dir = settings.value( "SyncDir").toString();

    if(_sync_dir.isEmpty())
        _sync_dir = "";

    settings.endGroup();
}

void ShareService::saveSettins()
{
    msg_Dbg(its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::saveSettins()");

    QSettings settings(
#ifdef _WIN32
        QSettings::IniFormat,
#else
        QSettings::NativeFormat,
#endif
        QSettings::UserScope, "vlc", "vlc-qt-its" );

    settings.beginGroup( "Account" );

    //userUuid
    if( !_its_random_user )
         settings.setValue( "Uuid",  _user_uuid);

    settings.endGroup();

    settings.beginGroup( "General" );

    //apiHost
    settings.setValue( "ApiHost", _api_host);

    //syncDir
    settings.setValue( "SyncDir", _sync_dir);

    settings.endGroup();

    settings.sync();
}

void ShareService::registerProtocol()
{
    QString applicationPath = pathAppend( config_GetLibDir(), "vlc.exe" );

    applicationPath = applicationPath.replace("/", QDir::separator());

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::registerProtocol() -> app path: (%s)",
        applicationPath.toStdString().c_str() );

    QSettings itsshareRootKey( "HKEY_CURRENT_USER\\Software\\Classes\\itsshare", QSettings::NativeFormat );
    itsshareRootKey.setValue( ".", "URL:ITSSHARE" );
    itsshareRootKey.setValue( "URL Protocol", "" );
    QSettings itsshareOpenKey( "HKEY_CURRENT_USER\\Software\\Classes\\itsshare\\shell\\open\\command", QSettings::NativeFormat );
    itsshareOpenKey.setValue( ".", "\"" + applicationPath + "\" \"%1\"" );
}

void ShareService::handleItemChanged( input_item_t *p_item )
{
    playlist_item_t *pl_item = playlist_ItemGetByInput( its::ShareService::getPL(), p_item );
    QString uri( input_item_GetURI( p_item ) );
    if( pl_item != NULL && uri.startsWith( "itsshare://" ) )
    {
        msg_Dbg(  its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleItemChanged() -> start with itsshare://" );
        playlist_NodeDelete( its::ShareService::getPL(), pl_item );

        ITSEvent *event = new ITSEvent( ITSEvent::GetSession );
        event->shareURL = uri;
        QApplication::postEvent( this, event );
    }
}

void ShareService::customEvent( QEvent *event )
{
    int i_type = event->type();
    ITSEvent *its_event = static_cast<ITSEvent *>( event );
    QString shareURL = its_event->shareURL;
    QString sessionHash = its_event->sessionHash;

    /* Actions */
    switch( i_type )
    {
        case ITSEvent::GetSession:
            openShareURL( shareURL );
            break;
        case ITSEvent::ForceStop:
            forceStopSession( sessionHash );
            break;
        default:
            //TODO: handle error
            break;
    }
}

void ShareService::openShareURL( QString shareUrl )
{
    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::openShareURL() -> shareURL: (%s)", shareUrl.toStdString().c_str() );

    QString sessionHash = shareUrl.replace( "itsshare://", "" ).replace( "/", "" );
    if( sessionHash.isEmpty() )
    {
        msg_Dbg(its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::openShareURL() -> session hash is empty" );
        DialogsService::getInstance()->displayError( "ITS", _("Something went wrong. Please try again later" ) );
        return;
    }

    if( hasShareItemHash( sessionHash ) )
    {
        its::DialogsService::getInstance()->displayQuestion( "ITS", _("You are trying to restart playing stream"), DIALOG_QUESTION_NORMAL, "Ok", "", "", [=](int){} );
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::openShareURL() -> session already exists" );
        return;
    }

    DialogsService::getInstance()->showBusy();

    _api->getSessionAsync(_user_uuid, sessionHash, [this, sessionHash]( QString source, APIResponseStatus status )
    {
        DialogsService::getInstance()->hideBusy();

        if( status.success )
        {
            handleSessionSourceUri( source, sessionHash );
        }
        else
        {
            msg_Dbg(its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::openShareURL() -> getSession failed.");
            DialogsService::getInstance()->displayError( "ITS", _("Something went wrong. Please try again later") );
        }
    });
}

void ShareService::handleSessionSourceUri( QString source, QString sessionHash )
{
    QString cutSource = source;
    if(cutSource.length() > 30)
        cutSource = cutSource.left(10) + "..." + cutSource.right(15);

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleSessionSourceUri() -> source: (%s)", cutSource.toStdString().c_str() );

    if( source.isEmpty() )
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleSessionSourceUri() -> source is empty" );
        DialogsService::getInstance()->displayError( "ITS", _("Something went wrong. Please try again later") );
        return;
    }

    //http
    if( !source.startsWith( "file://" ) )
    {
        registerShareItem( source, sessionHash );

        QString cutSource = source;
        if( cutSource.length() > 30 )
            cutSource = cutSource.left( 10 ) + "..." + cutSource.right( 15 );

        QString cutSessionHash = sessionHash;
        if(cutSessionHash.length() > 15)
            cutSessionHash = cutSessionHash.left(5) + "..." + cutSessionHash.right(5);

        QString name = QString( "Session: %1" ).arg( cutSessionHash );

        playlist_AddExt( getPL(), source.toStdString().c_str(), name.toStdString().c_str() , true, 0, NULL, 0, true );
        return;
    }

    //local file
    if( !_sync_dir.isEmpty() && QDir( _sync_dir ).exists() )
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleSessionSourceUri() -> is local file. file name: (%s)",
            QUrl( source ).fileName().toStdString().c_str() );

        QString fullPath = QUrl::fromLocalFile( pathAppend(_sync_dir, QUrl(source).fileName()) ).toString();

        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleSessionSourceUri() -> is local file. full path: (%s)",
            fullPath.toStdString().c_str() );

        registerShareItem( fullPath, sessionHash );
        playlist_Add( its::ShareService::getPL(), fullPath.toStdString().c_str(), true ) ;
        return;
    }

    its::DialogsService::getInstance()->displayQuestion( "ITS", _("Sync directory not set"), DIALOG_QUESTION_NORMAL, "Cancel", _("Set sync directory"), "", [=]( int actionNumber )
    {
        if(actionNumber == 0)
            return;

        handleSetSyncDirectory();

        if ( _sync_dir.isEmpty() )
            return;

        QString fullPath = QUrl::fromLocalFile(pathAppend( _sync_dir, QUrl(source).fileName()) ).toString();

        registerShareItem( fullPath, sessionHash );
        playlist_Add( its::ShareService::getPL(), fullPath.toStdString().c_str(), true );
    });
}

QString ShareService::pathAppend( const QString& path1, const QString& path2 )
{
    return path1 + QDir::separator() + path2;
}

bool ShareService::isShareUri( QString uri )
{
    return _sharedItems.contains( uri );
}

QString ShareService::getCurrentItemShareHash()
{
    auto mim = ShareService::getMIM();
    Q_ASSERT( mim != nullptr );

    auto input_item = mim->currentInputItem();
    if( input_item == nullptr )
        return "";

    if( input_item->i_type != ITEM_TYPE_FILE )
        return "";

    QString uri = QString( input_item_GetURI( input_item ) );
    QString cutURI = uri;
    if( cutURI.length() > 30 )
        cutURI = cutURI.left( 10 ) + "..." + cutURI.right( 15 );

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::currentItemIsShare() -> uri: (%s) - hash (%s) - shared: (%i)", cutURI.toStdString().c_str(), _sharedItems.value( uri ).toStdString().c_str(), isShareUri( uri ));

    if( !_sharedItems.contains( uri ) )
        return "";

    return _sharedItems.value( uri );
}

void ShareService::registerShareItem( QString uri, QString hash )
{
    QString cutURI = uri;
    if( cutURI.length() > 30 )
        cutURI = cutURI.left( 10 ) + "..." + cutURI.right( 15 );

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::registerShareItem() ->  uri: (%s) - hash (%s)", cutURI.toStdString().c_str(), hash.toStdString().c_str() );
    if( _sharedItems.contains( uri ) )
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::registerShareItem() ->  already contained" );
        return;
    }

    _initializeCrutch = true;
    _sharedItems.insert( uri, hash );
    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::registerShareItem() ->  register success" );
}

void ShareService::unregisterShareItem( QString uri )
{
    QString cutURI = uri;
    if( cutURI.length() > 30 )
        cutURI = cutURI.left( 10 ) + "..." + cutURI.right( 15 );

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::unregisterShareItem() ->  uri: (%s)", cutURI.toStdString().c_str() );
    if( _sharedItems.contains( uri ) )
    {
        _sharedItems.remove( uri );
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::unregisterShareItem() ->  success" );
        return;
    }

    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::unregisterShareItem() ->  not contained" );
}

void ShareService::unregisterShareItemByHash( QString hash )
{
    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::unregisterShareItemByHash() ->  hash: (%s)", hash.toStdString().c_str() );

    for( auto it = _sharedItems.begin(); it != _sharedItems.end(); )
    if( it.value() == hash )
    {
        QString cutURI = it.key().toStdString().c_str();
        if( cutURI.length() > 30 )
            cutURI = cutURI.left( 10 ) + "..." + cutURI.right( 15 );

        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::unregisterShareItemByHash() ->  erase uri: (%s)", cutURI.toStdString().c_str() );
        it = _sharedItems.erase( it );
    }
    else
    {
        ++it;
    }
}

bool ShareService::hasShareItemHash( QString hash )
{
    for( auto it = _sharedItems.begin(); it != _sharedItems.end(); )
    if( it.value() == hash )
    {
        return true;
    }
    else
    {
        ++it;
    }

    return false;
}

void ShareService::handleUpdateStatus( int state )
{
    QString stateString = "";
    switch ( state )
    {
    case input_state_e::INIT_S:
            stateString = "INIT";
            break;
        case input_state_e::OPENING_S:
            stateString = "OPENING";
            break;
        case input_state_e::PLAYING_S:
            stateString = "PLAYING";
            break;
        case input_state_e::PAUSE_S:
            stateString = "PAUSE";
            break;
        case input_state_e::END_S:
            stateString = "END";
            break;
        case input_state_e::ERROR_S:
            stateString = "ERROR";
            break;

    default:
            stateString = "UNDEFINED";
            break;
    }

    msg_Dbg( getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus() -> state: (%s)", stateString.toStdString().c_str() );

    QString shareHash = getCurrentItemShareHash();

    if( shareHash.isEmpty() )
        return;

    //=> initialize crutch
    if( this->_initializeCrutch && state == input_state_e::PLAYING_S )
    {
        playlist_Pause( getPL() );
        return;
    } else if ( this->_initializeCrutch && state == input_state_e::PAUSE_S )
    {
        _initializeCrutch = false;
        return;
    }
    //<= initialize crutch

    if( state == input_state_e::PLAYING_S )
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus() -> play api call start");

        DialogsService::getInstance()->showBusy();

        _api->playAsync(_user_uuid, shareHash, [this]( int time, APIResponseStatus status ) {

            DialogsService::getInstance()->hideBusy();

            if(status.success)
            {
                msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus() -> play api call finish success. time: (%i)", time );
                int64_t length = var_GetInteger( ShareService::getMIM()->getInput(), "length" ) / CLOCK_FREQ ;
                float pos = static_cast<float>( time ) / static_cast<float>( length );

                if( pos >= 0 && pos <= 1 )
                {
                    getMIM()->getIM()->sliderUpdate( pos );
                    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus() -> rewind success . time/length/pos: (%i/%f/%f)", time, length, pos );
                }
                else
                {
                    msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus() -> rewind failed. time/length/pos: (%i/%f/%f)", time, length, pos );
                }
            }
            else
            {
                msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus() -> play api call finish failed. (%s)", status.errorDescription.toStdString().c_str() );
                DialogsService::getInstance()->displayError( "ITS", _("Something went wrong. Please try again later") );
            }
        });
    }
    else if( state == input_state_e::PAUSE_S )
    {
        msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus() -> pause api call start" );

        DialogsService::getInstance()->showBusy();

        _api->pauseAsync(_user_uuid, shareHash, [this]( APIResponseStatus status ) {

            DialogsService::getInstance()->hideBusy();

            if(status.success)
            {
                msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus -> pause api call finish success.");
            }
            else
            {
                msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleUpdateStatus -> pause api call finish failed. (%s)", status.errorDescription.toStdString().c_str() );
                DialogsService::getInstance()->displayError( "ITS", _("Something went wrong. Please try again later") );
            }
        });
    }
    else if( state == input_state_e::END_S)
    {
        sendForceStopEvent( shareHash );
    }
}

void ShareService::handleSetSyncDirectory()
{
    msg_Dbg( ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleSetSyncDirectory()" );

    const QStringList schemes = QStringList( QStringLiteral( "file" ) );
    QUrl dirurl = QFileDialog::getExistingDirectoryUrl( NULL,
            qtr( "Set sync directory" ), its::ShareService::getInterfaceThread()->p_sys->filepath,
            QFileDialog::ShowDirsOnly, schemes );

    msg_Dbg( ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleSetSyncDirectory() -> dirurl: (%s)",
        dirurl.toLocalFile().replace("/", QDir::separator()).toStdString().c_str() );

    if( dirurl.isEmpty() )
        return;

    _sync_dir = dirurl.toLocalFile().replace( "/", QDir::separator() );
    saveSettins();
}

void ShareService::handlePlayTutorialVideo()
{
    msg_Dbg( ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handlePlayTutorialVideo()" );

    QString toturialURL = "https://youtu.be/41wV3ddFDAg";
    playlist_Add( its::ShareService::getPL(), toturialURL.toStdString().c_str(), true );
}

void ShareService::handleNeedPlayTutorialVideo( bool emptyPlaylist )
{
    msg_Dbg( ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleNeedPlayTutorialVideo() -> isNewUser: (%i) emptyPlayList: (%i)", _is_new_user), emptyPlaylist;

    if( _is_new_user && emptyPlaylist )
        handlePlayTutorialVideo();

    _is_new_user = false;
}

void ShareService::handleShare()
{
    msg_Dbg( ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleShare() start" );
    QString shareHash = getCurrentItemShareHash();

    if( !shareHash.isEmpty() )
    {
        its::DialogsService::getInstance()->displayQuestion( "ITS", _("You are trying to restart playing stream"), DIALOG_QUESTION_NORMAL, "Ok", "", "", [=](int){} );
        return;
    }

    auto p_input = ShareService::getMIM()->getInput();

    if( p_input == NULL )
    {
        msg_Dbg( ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleShare()-> p_input == NULL");
        its::DialogsService::getInstance()->displayQuestion("ITS", _("No video to share"), DIALOG_QUESTION_NORMAL, "Ok", "", "", [=](int){});
        return;
    }

    input_item_t *p_item = input_GetItem( p_input );
    if( p_item == NULL )
    {
        msg_Dbg( ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleShare()-> p_item == NULL");
        its::DialogsService::getInstance()->displayQuestion( "ITS", _("No video to share"), DIALOG_QUESTION_NORMAL, "Ok", "", "" );
        return;
    }

    QString uri = QString( input_item_GetURI( p_item ) );
    int64_t length = var_GetInteger(  p_input , "length" ) / CLOCK_FREQ ;
    int64_t time = var_GetInteger(  p_input , "time") / CLOCK_FREQ ;

    if( ShareService::getMIM()->getIM()->playingStatus() == PLAYING_S )
        playlist_TogglePause( its::ShareService::getPL() );

    DialogsService::getInstance()->showBusy();

    _api->createSessionAsync(_user_uuid, uri, time, length, [this, uri](QString link, QString hash, APIResponseStatus status )
    {
        DialogsService::getInstance()->hideBusy();

        if( status.success )
        {
            msg_Dbg(its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleShare() -> createSession success. link: (%s) hash: (%s)", link.toStdString().c_str(), hash.toStdString().c_str());
            QClipboard *clipboard = QApplication::clipboard();
            assert( clipboard != NULL );
            clipboard->setText( link, QClipboard::Clipboard );

            its::DialogsService::getInstance()->displayQuestion( "ITS", _("Share link copied to clipboard"), DIALOG_QUESTION_NORMAL, "Ok", "", "", [=](int){} );

            this->registerShareItem( uri, hash );
        }
        else
        {
            msg_Dbg( its::ShareService::getInterfaceThread(), "ITS_DBG: ShareService::handleShare() -> createSession failed" );
            DialogsService::getInstance()->displayError( "ITS", _("Something went wrong. Please try again later") );
        }
    });
}

void ShareService::handleDebug()
{
}

} // its