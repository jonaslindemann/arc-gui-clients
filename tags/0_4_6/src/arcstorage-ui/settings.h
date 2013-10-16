#ifndef SETTINGS_H
#define SETTINGS_H

#include <qhash.h>
#include <qvariant.h>

class Settings
{
private:
    static QHash<QString,QVariant> settings;
    static const QString SETTINGS_FILENAME;

private:
    Settings();

public:
    static QString getStringValue(QString key);
    static QVariant getValue(QString key);
    static bool setValue(QString key, QVariant value);
    static bool saveToDisk();
    static bool loadFromDisk();
};

#endif // SETTINGS_H
