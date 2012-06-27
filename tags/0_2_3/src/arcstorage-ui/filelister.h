#ifndef FILELISTER_H
#define FILELISTER_H

#include <QDir>

class FileLister
{
public:
    FileLister();
    QLinkedList<QString> getFilesAsStrings(QString path);
    QLinkedList<QString> getFoldersAsStrings(QString path);
    void traverseFileStructure(QString path);
    void recursivelyTraverseFileStructure(QDir dir);
};

#endif // FILELISTER_H
