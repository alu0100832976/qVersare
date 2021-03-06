#include <QCommandLineParser>
#include <QList>
#include <QSettings>

#include "serversettings.h"


ServerSettings::ServerSettings(QCoreApplication *a)
{
    QCommandLineParser parser;
    QSettings settings("/etc/qVersareServer/qVersareServer.conf",QSettings::IniFormat);

    QList<QCommandLineOption> options(
    {
        {{"ip","i"},"Set ip to listen to", "ip","127.0.0.1"},
        {{"port","p"},"Set port to listen to","port","9000"},
        {"database","Set dbname","db","qversare.sqlite"},
        {{"daemon","d"},"Set daemon mode"},
        {{"stats","s"}, "Set interval time", "interval", "900000"}
    });

    parser.addOptions(options);


    parser.parse(a->arguments());
    if( !settings.contains("ip") || !settings.contains("port") ||
        !settings.contains("db") || !settings.contains("daemon") ||
        !settings.contains("stats"))
            setDefaultSettings(&settings);

    ( parser.isSet("ip") ) ? ipAddress_ = parser.value("ip") :
        ipAddress_ = settings.value("ip","127.0.0.1").toString();

    (parser.isSet("port") ) ? port_ = parser.value("port").toShort() :
        port_ = settings.value("port", 8000).toString().toUInt();

    (parser.isSet("database") ) ? dbName_ = parser.value("database") :
            dbName_ = settings.value("database", "qversare.sqlite").toString();

    (parser.isSet("daemon") ) ? daemon_ = true :
            daemon_ = settings.value("daemon", false).toBool();

    (parser.isSet("stats") ) ? interval_ = parser.value("stats").toInt() :
            interval_ = settings.value("stats", 900000).toInt();
}

bool ServerSettings::getDaemon() const
{
    return daemon_;
}

QString ServerSettings::getDbName() const
{
    return dbName_;
}

QString ServerSettings::getIpAddress() const
{
    return ipAddress_;
}

quint16 ServerSettings::getPort() const
{
    return port_;
}

void ServerSettings::setDefaultSettings(QSettings *settings)
{
    settings->setValue("ip", "127.0.0.1");
    settings->setValue("port",8000);
    settings->setValue("db","qversare.sqlite");
    settings->setValue("daemon",false);
    settings->setValue("stats",900000);
}

quint32 ServerSettings::getInterval() const
{
    return interval_;
}
