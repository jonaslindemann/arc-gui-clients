#include "arcfileelement.h"
#include <QString>

ARCFileElement::ARCFileElement()
{
}

ARCFileElement::ARCFileElement(QString fn,
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
                               qint64 sze)
{
    fileName     = fn;
    filePath     = fp;
    fileType     = ft;
    group        = grp;
    executable   = exec;
    readable     = read;
    writable     = write;
    lastModified = lstModified;
    lastRead     = lstRead;
    owner        = ownr;
    permissions  = perm;
    size         = sze;
}
