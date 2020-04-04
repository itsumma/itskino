#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "its_request.hpp"
#include "its_share_service.hpp"
#include <QEventLoop>

namespace its {

HttpRequest::HttpRequest() {
}

HttpRequest::~HttpRequest() {
}

void HttpRequest::asyncPost(QString url, QJsonObject jsonObject, Fn<void(QString response)> onRequestFinishedCallback )
{
	QNetworkRequest request;
	QNetworkReply* requestReply = nullptr;
	QUrlQuery postData;

	request.setUrl(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
	requestReply = _networkAccessManager.post(request, QJsonDocument(jsonObject).toJson());

	auto onFinished = [onRequestFinishedCallback](QNetworkReply* reply) {
		QString response = "";
		if (reply->error() == QNetworkReply::NoError)
			response = reply->readAll();
		else
			response = "code: " + QString::number(reply->error()) + " descr: " + reply->errorString() + " content: " + reply->readAll();

		reply->deleteLater();
		onRequestFinishedCallback(response);
	};

	connect(requestReply, &QNetworkReply::finished, std::bind(onFinished, requestReply));
	connect(requestReply, SIGNAL(sslErrors(QList<QSslError>)), requestReply, SLOT(ignoreSslErrors()));
}

QString HttpRequest::syncPost( QString url, QJsonObject jsonObject )
{
	QEventLoop waitLoop;
	QNetworkRequest request;
	QNetworkReply* requestReply = nullptr;
	QString response;
	QUrlQuery postData;

	request.setUrl( url );
	request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );
	request.setAttribute( QNetworkRequest::FollowRedirectsAttribute, true );
	requestReply = _networkAccessManager.post( request, QJsonDocument(jsonObject).toJson() );

	auto onFinished = [&response, &waitLoop]( QNetworkReply* reply )
	{
		if ( reply->error() == QNetworkReply::NoError )
			response = reply->readAll();
		else
		{
			response = "code: " + QString::number(reply->error()) + " descr: " + reply->errorString() + " content: " + reply->readAll();
		}

		reply->deleteLater();
		emit waitLoop.quit();
	};

	connect( requestReply, &QNetworkReply::finished, std::bind( onFinished, requestReply ));
	connect( requestReply, SIGNAL( sslErrors(QList<QSslError>) ), requestReply, SLOT( ignoreSslErrors() ) );

	waitLoop.exec();

	return response;
}

} // its