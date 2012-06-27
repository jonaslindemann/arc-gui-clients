#ifndef ARCFILEELEMENT_H
#define ARCFILEELEMENT_H

#include <QString>
#include <QDateTime>

enum ARCFileType { ARCUndefined, ARCDir, ARCFile };

/** This class describes the information associated with a file
  * It's currently modelled after the information that QT provides about
  * local and ftp files. May be changed in the future to be more
  * focused towards other protocols like SRM
 */
class ARCFileElement
{
private:
    QString fileName;
    QString filePath;
    QString group;
    bool executable;
    bool readable;
    bool writable;
    QDateTime lastModified;
    QDateTime lastRead;
    QString owner;
    int permissions;
    qint64 size;

    enum ARCFileType fileType;

public:
    ARCFileElement();
    ARCFileElement(QString fn,
                   QString fp,
                   enum ARCFileType ft,
                   QString grp,
                   bool exec,
                   bool read,
                   bool write,
                   QDateTime lstModified,
                   QDateTime lstRead,
                   QString ownr,
                   int perm,
                   qint64 sze);
    QString          getFileName()    { return fileName;     }
    QString          getFilePath()    { return filePath;     }
    enum ARCFileType getFileType()    { return fileType;     }
    QString          getGroup()       { return group;        }
    bool             isExecutable()   { return executable;   }
    bool             isReadable()     { return readable;     }
    bool             isWritable()     { return writable;     }
    QDateTime        getLastModfied() { return lastModified; }
    QDateTime        getLastRead()    { return lastRead;     }
    QString          getOwner()       { return owner;        }
    int              getPermissions() { return permissions;  }
    qint64           getSize()        { return size;         }
};

#endif // ARCFILEELEMENT_H
