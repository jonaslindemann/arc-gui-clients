#include "settings.h"
#include <qhash.h>
#include <qvariant.h>
#include <qdatastream.h>
#include <qfile.h>
#include <qiodevice.h>

QHash<QString,QVariant> Settings::settings;
const QString Settings::SETTINGS_FILENAME = QString("ArcFTPClientSettings.dat");

Settings::Settings()
{
}

QString Settings::getStringValue(QString key)
{
    QString value = "";

    if (settings.contains(key))
    {
        QVariant qValue;
        qValue = settings[key];
        value = qValue.toString();
    }

    return value;
}

QVariant Settings::getValue(QString key)
{
    QVariant value;

    if (settings.contains(key))
    {
        value = settings[key];
    }

    return value;
}

bool Settings::setValue(QString key, QVariant value)
{
    bool success = true;

    Settings::settings[key] = value;

    return success;
}

bool Settings::saveToDisk()
{
    bool success = true;

    QFile settingsFile(SETTINGS_FILENAME);
    settingsFile.open(QIODevice::WriteOnly);
    QDataStream out(&settingsFile);
    out << settings;

    return success;
}

bool Settings::loadFromDisk()
{
    bool success = true;

    QFile settingsFile(SETTINGS_FILENAME);
    settingsFile.open(QIODevice::ReadOnly);
    QDataStream in(&settingsFile);
    in >> settings;

    return success;
}

