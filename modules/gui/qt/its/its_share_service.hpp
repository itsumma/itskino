#ifndef VLC_QT_ITS_SHARE_SERVICE_HPP_
#define VLC_QT_ITS_SHARE_SERVICE_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <vlc_input.h>
#include <vlc_common.h>    /* VLC_COMMON_MEMBERS for vlc_interface.h */
#include <vlc_interface.h> /* intf_thread_t */
#include <vlc_playlist.h>  /* playlist_t */

#include "qt.hpp"
#include "input_manager.hpp"

#include <QObject>
#include <QEvent>
#include <QUuid>
#include <QSettings>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QTimer>
#include <QMutex>

#include "its_api.hpp"

namespace its {

#define ITSDBGMARK(x) msg_Dbg(its::ShareService::getInterfaceThread(), "ITS_DBG: (%s) [%s:%d: %s]\n", #x, __FILE__, __LINE__, __func__);

class ITSEvent : public QEvent
{
public:
    enum event_types
	{
        GetSession = QEvent::User + 50,
		ForceStop,
    };

    ITSEvent( event_types type ): QEvent( ( QEvent::Type )( type ) )
    {}

    virtual ~ITSEvent()
    {}

	QString shareURL;
	QString sessionHash;
};

class ShareService : public QObject
{
	Q_OBJECT

public:
	static ShareService *getInstance()
	{
		assert( _instance );
		return _instance;
	}

	static ShareService *getInstance( intf_thread_t *p_intf )
	{
		if( !_instance )
			_instance = new ShareService( p_intf );
		return _instance;
	}

	static void killInstance()
	{
		delete _instance;
		_instance = NULL;
		_p_intf = NULL;
	}

	static intf_thread_t *getInterfaceThread() {
		return _p_intf;
	}

	static MainInputManager *getMIM() {
		return MainInputManager::getInstance( _p_intf );
	}

	static InputManager *getIM() {
		return MainInputManager::getInstance( _p_intf )->getIM();
	}

	static playlist_t *getPL()
	{
		return its::ShareService::getInterfaceThread()->p_sys->p_playlist;
	}

	void handleItemChanged( input_item_t *p_item );
	void handleUpdateStatus( int state );
	void handleSetSyncDirectory();
	void handlePlayTutorialVideo();
	void handleNeedPlayTutorialVideo( bool emptyPlaylist );
	void handleShare();
	void handleDebug();

	void openShareURL( QString shareUrl );
	void handleSessionSourceUri( QString source, QString sessionHash );
	QString pathAppend( const QString& path1, const QString& path2 );

	void loadSettings();
	void saveSettins();

	bool isShareUri( QString uri );
	QString getCurrentItemShareHash();
	void registerShareItem( QString uri, QString hash );
	void unregisterShareItem( QString uri );
	void unregisterShareItemByHash( QString hash );
	bool hasShareItemHash( QString hash );
	void sendForceStopEvent( QString sessionHash );
	void forceStopSession( QString sessionHash );

	bool _initializeCrutch = false;

private:
	ShareService( intf_thread_t * p_intf);
	virtual ~ShareService();

	void customEvent( QEvent * );

	void registerProtocol();

	static ShareService *_instance;
	static intf_thread_t *_p_intf;

	bool
		_its_random_user = false,
		_is_new_user = false;

	QUuid _user_uuid;

	QString
		_api_host,
		_sync_dir;

	API *_api;

	QMap<QString, QString> _sharedItems;

	QMutex _forceStopMutex;
};

} // its

#endif // include-guard