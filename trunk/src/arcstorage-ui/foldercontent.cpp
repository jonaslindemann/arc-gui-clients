#include "foldercontent.h"
#include <QDir>
#include <QVector>

FolderContent::FolderContent()
{
    folderListAbsolutePaths = new QVector <QString>;
    folderListFolderNames = new QVector <QString>;
    fileListAbsolutePaths = new QVector <QString>;
    fileListFileNames = new QVector <QString>;
}

void FolderContent::updateFolderContent(QString path)
{
    QDir dir(path);

    QFileInfoList fileInfoList = dir.entryInfoList();

    folderListAbsolutePaths->clear();
    folderListFolderNames->clear();
    fileListAbsolutePaths->clear();
    fileListFileNames->clear();

    for (int i = 0; i < fileInfoList.size(); ++i)
    {
        if (fileInfoList.at(i).isDir())
        {
            (*folderListAbsolutePaths) << (QString)(fileInfoList.at(i).filePath());
            (*folderListFolderNames) << (QString)fileInfoList.at(i).fileName();
        }
        else if (fileInfoList.at(i).isFile())
        {
            (*fileListAbsolutePaths) << (QString)fileInfoList.at(i).filePath();
            (*fileListFileNames) << (QString)fileInfoList.at(i).fileName();

        }
    }


}

QVector<QString> &FolderContent::getFolderListAbsolutePaths()
{
    return *folderListAbsolutePaths;
}

QVector<QString> &FolderContent::getFolderListFolderNames()
{
    return *folderListFolderNames;
}

QVector<QString> &FolderContent::getFileListAbsolutePaths()
{
    return *fileListAbsolutePaths;
}

QVector<QString> &FolderContent::getFileListFileNames()
{
    return *fileListFileNames;
}

QString FolderContent::getFolderAbsolutePathAt(int index)
{
    return folderListAbsolutePaths->at(index);
}

