#ifndef QDEBUGSTREAM_H
#define QDEBUGSTREAM_H

//################
//# qdebugstream.h  #
//################

#include <iostream>
#include <streambuf>
#include <string>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QMainWindow>
#include <QApplication>

#include "qtextedit.h"

#include <QCustomEvent>

// Define your custom event identifier
const QEvent::Type DEBUG_STREAM_EVENT = static_cast<QEvent::Type>(QEvent::User + 1);

// Define your custom event subclass
class DebugStreamEvent : public QEvent
{
    public:
        DebugStreamEvent(const QString& outputText):
            QEvent(DEBUG_STREAM_EVENT),
            m_outputText(outputText)
        {
        }

        QString getOutputText() const
        {
            return m_outputText;
        }

    private:
        QString m_outputText;
};

class QDebugStream : public std::basic_streambuf<char>
{
public:
    QDebugStream(std::ostream &stream, QMainWindow* mainWindow) : m_stream(stream)
    {
        m_mainWindow = mainWindow;
        m_old_buf = stream.rdbuf();
        stream.rdbuf(this);
    }
    ~QDebugStream()
    {
        // output anything that is left
        QMutexLocker locker(&m_mutex);
        if (!m_string.empty()) {

            QString appendString = m_string.c_str();
            QApplication::postEvent(m_mainWindow, new DebugStreamEvent(appendString));
            //log_window->append(appendString);
        }

        m_stream.rdbuf(m_old_buf);
    }

protected:
    virtual int_type overflow(int_type v)
    {
        QMutexLocker locker(&m_mutex);
        if (v == '\n')
        {
            QString appendString = m_string.c_str();
            //log_window->append(appendString);
            QApplication::postEvent(m_mainWindow, new DebugStreamEvent(appendString));
            // Scroll to bottom of window

            //log_window->moveCursor(QTextCursor::End);
            //log_window->moveCursor(QTextCursor::StartOfLine);

            m_string.erase(m_string.begin(), m_string.end());
        }
        else
            m_string += v;

        return v;
    }

    virtual std::streamsize xsputn(const char *p, std::streamsize n)
    {
        QMutexLocker locker(&m_mutex);

        m_string.append(p, p + n);

        std::string::size_type pos = 0;
        while (pos != std::string::npos)
        {
            pos = m_string.find('\n');
            if (pos != std::string::npos)
            {
                std::string tmp(m_string.begin(), m_string.begin() + pos);
                QString appendString = tmp.c_str();
                QApplication::postEvent(m_mainWindow, new DebugStreamEvent(appendString));
                //log_window->append(appendString);
                qDebug() << appendString;

                // Scroll to bottom of window

                //log_window->moveCursor(QTextCursor::End);
                //log_window->moveCursor(QTextCursor::StartOfLine);

                m_string.erase(m_string.begin(), m_string.begin() + pos + 1);
            }
        }

        return n;
    }

private:
    std::ostream &m_stream;
    std::streambuf *m_old_buf;
    std::string m_string;
    QMainWindow* m_mainWindow;
    QMutex m_mutex;
};

#endif // QDEBUGSTREAM_H
