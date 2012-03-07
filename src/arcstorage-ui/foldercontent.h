#ifndef FOLDERCONTENT_H
#define FOLDERCONTENT_H

#include <QVector>
#include <QString>

class FolderContent
{
private:
    QVector<QString> *folderListAbsolutePaths;
    QVector<QString> *folderListFolderNames;
    QVector<QString> *fileListAbsolutePaths;
    QVector<QString> *fileListFileNames;

public:
    FolderContent();

    void updateFolderContent(QString path);
    QVector<QString> &getFolderListAbsolutePaths();
    QVector<QString> &getFolderListFolderNames();
    QVector<QString> &getFileListAbsolutePaths();
    QVector<QString> &getFileListFileNames();
    QString getFolderAbsolutePathAt(int index);
};

#endif // FOLDERCONTENT_H
