#ifndef ARCTOOLS_H
#define ARCTOOLS_H

#include <QObject>
#include <QMutex>

#include <arc/UserConfig.h>

#include "arcproxy-utils.h"
#include "helpwindow.h"


class ARCTools : public QObject
{
    Q_OBJECT
private:
    Arc::UserConfig* m_userConfig;
    ArcProxyController* m_proxyController;
    QString m_jobListFile;
    HelpWindow* m_helpWindow;
public:
    static ARCTools* instance()
    {
        static QMutex mutex;
        if (!m_instance)
        {
            mutex.lock();

            if (!m_instance)
                m_instance = new ARCTools;

            mutex.unlock();
        }

        return m_instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete m_instance;
        m_instance = 0;
        mutex.unlock();
    }

    bool initUserConfig();
    Arc::UserConfig* currentUserConfig();
    bool hasValidProxy();

    void setJobListFile(QString filename);
    QString jobListFile();

    void proxyCertificateTool();
    void certConversionTool();
    void jobManagerTool();
    void submissionTool();
    void storageTool();

    void showHelpWindow(QMainWindow* window);
    void closeHelpWindow();

private:

    ARCTools();

    /*
    ARCTools(const ARCTools &); // hide copy constructor
    ARCTools& operator=(const ARCTools &); // hide assign op
    // we leave just the declarations, so the compiler will warn us
    // if we try to use those two functions by accident
    */

    static ARCTools* m_instance;
};

#endif // ARCTOOLS_H
