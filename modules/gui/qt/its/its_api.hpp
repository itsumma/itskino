#ifndef VLC_QT_ITS_DATA_HPP_
#define VLC_QT_ITS_DATA_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <vlc_common.h>    /* VLC_COMMON_MEMBERS for vlc_interface.h */
#include <vlc_interface.h> /* intf_thread_t */

#include <QObject>
#include <QUuid>

#include "its_request.hpp"

namespace its {

class APIResponseStatus
{
public:
	bool success;
	QString
		errorDescription,
		statusString;
	int statusCode;

	QJsonObject responseJsonObject;

	static APIResponseStatus create( intf_thread_t * p_intf, QString response );
};

class API : public QObject
{
	Q_OBJECT

public:
	API( intf_thread_t * p_intf, QString host );
	virtual ~API();

	void createSessionAsync( QUuid userUuid, QString source, int startTime, int length, Fn<void( QString link, QString hash, APIResponseStatus& status )> onRequestFinishedCallback = Fn<void( QString, QString, APIResponseStatus& )>() );
	void getSessionAsync( QUuid userUuid, QString sessionHash, Fn<void( QString source, APIResponseStatus& status )> onRequestFinishedCallback = Fn<void( QString, APIResponseStatus& )>() );
	void playAsync( QUuid userUuid, QString sessionHash, Fn<void( int time, APIResponseStatus& status )> onRequestFinishedCallback = Fn<void( int, APIResponseStatus& )>() );
	void pauseAsync( QUuid userUuid, QString sessionHash, Fn<void( APIResponseStatus& status )> onRequestFinishedCallback = Fn<void( APIResponseStatus& )>() );
	void stopAsync( QUuid userUuid, QString sessionHash, Fn<void( APIResponseStatus& status )> onRequestFinishedCallback = Fn<void( APIResponseStatus& )>() );
	APIResponseStatus stopSync( QUuid userUuid, QString sessionHash );

private:
	intf_thread_t *_p_intf;
	QString _host;
	HttpRequest _httpRequest;
};

} // its

#endif // include-guard
