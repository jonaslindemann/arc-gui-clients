#include "arctools.h"

#include <QProcess>

ARCTools* ARCTools::m_instance = 0;

ARCTools::ARCTools()
    :QObject()
{

}

void ARCTools::proxyCertificateTool()
{
    QProcess process;
    process.startDetached("arcproxy-ui");
}

void ARCTools::certConversionTool()
{
    QProcess process;
    process.startDetached("arccert-ui");
}

void ARCTools::jobManagerTool()
{
    QProcess process;
    process.startDetached("arcstat-ui");
}

void ARCTools::submissionTool()
{
    QProcess process;
    process.startDetached("arcsub-ui");
}

void ARCTools::storageTool()
{
    QProcess process;
    process.startDetached("arcstorage-ui");
}
