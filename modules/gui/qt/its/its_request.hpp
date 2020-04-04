#ifndef VLC_QT_ITS_REQUEST_HPP_
#define VLC_QT_ITS_REQUEST_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <memory>
#include <functional>

#include <QObject>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QUrlQuery>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>

#include "its/its_base.hpp"

namespace its {

class HttpRequest : public QObject
{
	Q_OBJECT

public:
	HttpRequest();
	virtual ~HttpRequest();

	void asyncPost( QString url, QJsonObject jsonObject, Fn<void(QString response)> onRequestFinishedCallback = Fn<void(QString)>() );
	QString syncPost( QString url, QJsonObject jsonObject );

private:
	QNetworkAccessManager _networkAccessManager;
};

} // its

#endif // include-guard