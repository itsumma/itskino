#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "its_api.hpp"

namespace its {

APIResponseStatus APIResponseStatus::create( intf_thread_t * p_intf, QString response )
{
    APIResponseStatus retStatus;
    retStatus.success = false;

    if ( response.isEmpty() ) {
        msg_Dbg( p_intf, "ITS_DBG: APIResponseStatus::create -> empty response" );
        retStatus.errorDescription = "empty response";
        return retStatus;
    }

    QJsonDocument jsonResponse = QJsonDocument::fromJson( response.toUtf8() );
    if (jsonResponse.isEmpty()) {
        msg_Dbg( p_intf, "ITS_DBG: APIResponseStatus::create -> empty json response" );
        retStatus.errorDescription = "empty json response";
        return retStatus;
    }

    retStatus.responseJsonObject = jsonResponse.object();
    if ( retStatus.responseJsonObject .isEmpty() ) {
        msg_Dbg( p_intf, "ITS_DBG: APIResponseStatus::create-> empty json object" );
        retStatus.errorDescription = "empty json object";
        return retStatus;
    }

    if ( !retStatus.responseJsonObject.contains("status") )
    {
        msg_Dbg( p_intf, "ITS_DBG: APIResponseStatus::create -> not contains \"status\" field" );
        retStatus.errorDescription = "not contains \"status\" field";
        return retStatus;
    }

    if ( retStatus.responseJsonObject .value( "status" ).toString().toLower() == QString( "Ok" ).toLower())
    {
        retStatus.success = true;
        retStatus.errorDescription = "";
        return retStatus;
    }

    if ( retStatus.responseJsonObject.contains("massage") )
    {
        retStatus.errorDescription = retStatus.responseJsonObject .value( "massage" ).toString();
    }
    else
    {
        msg_Dbg( p_intf, "ITS_DBG: APIResponseStatus::create -> not contains \"massage\" field" );
        retStatus.errorDescription = "not contains \"massage\" field";
    }

    if ( retStatus.responseJsonObject .contains("status_code") )
    {
        retStatus.statusCode =  retStatus.responseJsonObject .value( "status_code" ).toInt();
    }
    else
    {
        msg_Dbg( p_intf, "ITS_DBG: APIResponseStatus::create -> not contains \"status_code\" field" );
        retStatus.errorDescription = "not contains \"status_code\" field";
    }

    return retStatus;
}

API::API( intf_thread_t * p_intf, QString host)
: _p_intf( p_intf )
, _host( host )
{
}

API::~API()
{
}

void API::createSessionAsync( QUuid userUuid, QString source, int startTime, int length, Fn<void(QString link, QString hash, APIResponseStatus& status)> onRequestFinishedCallback)
{
    QJsonObject jsonObject;
    jsonObject.insert( "userGuid", userUuid.toString().replace( "{", "" ).replace( "}", "" ) );
    jsonObject.insert( "source", source );
    jsonObject.insert( "length", length );
    jsonObject.insert( "startTime", startTime );

    _httpRequest.asyncPost( _host + "api/stream/create",
    jsonObject,
    [onRequestFinishedCallback, this]( QString response ) {
        APIResponseStatus status = APIResponseStatus::create( _p_intf, response );

        if(!status.success) {
            onRequestFinishedCallback("", "", status);
            return;
        }

        if ( !status.responseJsonObject.contains( "link" ) ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::createSession -> not contains \"link\" field" );
            status.success = false;
            status.errorDescription = "not contains \"link\" field";
            onRequestFinishedCallback( "", "", status) ;
            return;
        }

        if ( status.responseJsonObject.value( "link" ).toString().isEmpty() ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::createSession -> \"link\" field fail or not valid" );
            status.success = false;
            status.errorDescription = "\"link\" field fail or not valid";
            onRequestFinishedCallback( "", "", status );
            return;
        }

        if ( !status.responseJsonObject.contains( "hash" ) ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::createSession -> not contains \"hash\" field" );
            status.success = false;
            status.errorDescription = "not contains \"hash\" field";
            onRequestFinishedCallback( "", "", status );
            return;
        }

        if ( status.responseJsonObject.value( "hash" ).toString().isEmpty() ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::createSession -> \"hash\" field fail or not valid" );
            status.success = false;
            status.errorDescription = "\"hash\" field fail or not valid";
            onRequestFinishedCallback( "", "", status );
            return;
        }

        onRequestFinishedCallback( status.responseJsonObject.value( "link" ).toString(), status.responseJsonObject.value( "hash" ).toString(), status );
    });
}

void API::getSessionAsync( QUuid userUuid, QString sessionHash, Fn<void( QString source, APIResponseStatus& status )> onRequestFinishedCallback )
{
    QJsonObject jsonObject;
    jsonObject.insert( "userGuid", userUuid.toString().replace( "{", "" ).replace( "}", "" ) );
    jsonObject.insert( "hash", sessionHash );

    _httpRequest.asyncPost( _host + "api/session/get",
    jsonObject,
    [onRequestFinishedCallback, this]( QString response ) {
        APIResponseStatus status = APIResponseStatus::create( _p_intf, response) ;

        if( !status.success ) {
            onRequestFinishedCallback( "", status );
            return;
        }

        if ( !status.responseJsonObject.contains( "source" ) ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::getSession -> not contains \"source\" field" );
            status.success = false;
            status.errorDescription = "not contains \"session\" field";
            onRequestFinishedCallback( "", status );
            return;
        }

        if ( status.responseJsonObject.value( "source" ).toString().isEmpty() ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::getSession -> \"session\" field fail or not valid" );
            status.success = false;
            status.errorDescription = "\"source\" field fail or not valid";
            onRequestFinishedCallback( "", status );
            return;
        }

        onRequestFinishedCallback( status.responseJsonObject.value( "source" ).toString(), status );
    });
}

void API::playAsync( QUuid userUuid, QString sessionHash, Fn<void( int time, APIResponseStatus& status )> onRequestFinishedCallback )
{
    QJsonObject jsonObject;
    jsonObject.insert( "userGuid", userUuid.toString().replace( "{", "" ).replace( "}", "" ) );
    jsonObject.insert( "hash", sessionHash );

    _httpRequest.asyncPost(_host + "api/stream/play",
    jsonObject,
    [onRequestFinishedCallback, this](QString response) {
        APIResponseStatus status = APIResponseStatus::create( _p_intf, response);

        if(!status.success) {
            onRequestFinishedCallback(-1, status);
            return;
        }

        if ( !status.responseJsonObject.contains( "time" ) ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::play -> not contains \"time\" field" );
            status.success = false;
            status.errorDescription = "not contains \"time\" field";
            onRequestFinishedCallback(-1, status);
            return;
        }

        if ( status.responseJsonObject.value( "time" ).toInt() < 0 ) {
            msg_Dbg( _p_intf, "ITS_DBG: API::play -> \"time\" field fail or not valid" );
            status.success = false;
            status.errorDescription = "\"time\" field fail or not valid";
            onRequestFinishedCallback(-1, status);
            return;
        }

        onRequestFinishedCallback( status.responseJsonObject.value( "time" ).toInt(), status );
    });
}

void API::pauseAsync( QUuid userUuid, QString sessionHash, Fn<void( APIResponseStatus& status )> onRequestFinishedCallback )
{
    msg_Dbg( _p_intf, "ITS_DBG: API::pause -> userUuid: (%s) sessionHash: (%s)", userUuid.toString().toStdString().c_str(), sessionHash.toStdString().c_str() );

    QJsonObject jsonObject;
    jsonObject.insert( "userGuid", userUuid.toString().replace( "{", "" ).replace( "}", "" ));
    jsonObject.insert( "hash", sessionHash );

    _httpRequest.asyncPost(_host + "api/session/pause",
    jsonObject,
    [onRequestFinishedCallback, this](QString response) {
        APIResponseStatus status = APIResponseStatus::create( _p_intf, response);

        onRequestFinishedCallback(status);
    });
}

void API::stopAsync( QUuid userUuid, QString sessionHash, Fn<void( APIResponseStatus& status )> onRequestFinishedCallback )
{
    QJsonObject jsonObject;
    jsonObject.insert( "userGuid", userUuid.toString().replace( "{", "" ).replace( "}", "") );
    jsonObject.insert( "hash", sessionHash);

    _httpRequest.asyncPost( _host + "api/session/stop",
    jsonObject,
    [onRequestFinishedCallback, this]( QString response ) {
        APIResponseStatus status = APIResponseStatus::create( _p_intf, response);

        onRequestFinishedCallback(status);
    });
}

APIResponseStatus API::stopSync( QUuid userUuid, QString sessionHash )
{
    QJsonObject jsonObject;
    jsonObject.insert( "userGuid", userUuid.toString().replace( "{", "" ).replace( "}", "" ) );
    jsonObject.insert( "hash", sessionHash );

    QString response =_httpRequest.syncPost( _host + "api/session/stop", jsonObject );
    APIResponseStatus status = APIResponseStatus::create( _p_intf, response );

    return status;
}

} //its