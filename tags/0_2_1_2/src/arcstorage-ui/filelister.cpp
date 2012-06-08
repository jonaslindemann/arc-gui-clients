#include <QDir>
#include <QLinkedList>
#include <iostream>
#include "filelister.h"

using namespace std;

FileLister::FileLister()
{
}

QLinkedList<QString> FileLister::getFoldersAsStrings(QString path)
{
    QLinkedList<QString> list;

    QDir dir(path);
    dir.setFilter(QDir::Dirs);

    QFileInfoList fileInfoList = dir.entryInfoList();

    for (int i = 0; i < fileInfoList.size(); ++i)
    {
        QFileInfo fileInfo = fileInfoList.at(i);
//        QString qs = fileInfo.fileName();
        QString qs = fileInfo.absolutePath();
        list << qs;
    }

    return list;
}

QLinkedList<QString> FileLister::getFilesAsStrings(QString path)
{
    QLinkedList<QString> list;

    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
//    dir.setSorting(QDir::Size | QDir::Reversed);

    QFileInfoList fileInfoList = dir.entryInfoList();

    for (int i = 0; i < fileInfoList.size(); ++i)
    {
        QFileInfo fileInfo = fileInfoList.at(i);
        QString qs = fileInfo.fileName();
        list << qs;
    }

    return list;
}

void FileLister::traverseFileStructure(QString path)
{
    QDir dir(path);
    recursivelyTraverseFileStructure(dir);

}

void FileLister::recursivelyTraverseFileStructure(QDir dir)
{
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList fileInfoList = dir.entryInfoList();

    for (int i = 0; i < fileInfoList.size(); ++i)
    {
        if (fileInfoList.at(i).isDir())
        {
            QString qs = fileInfoList.at(i).fileName();
            std::cout << qs.toStdString() << std::endl;

            QDir sd(fileInfoList.at(i).filePath());
            recursivelyTraverseFileStructure(sd);
        }
        else
        {
            QString qs = fileInfoList.at(i).fileName();
            std::cout << qs.toStdString() << std::endl;

        }
    }
}


