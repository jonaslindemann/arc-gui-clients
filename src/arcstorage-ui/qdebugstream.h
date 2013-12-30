#ifndef QDEBUGSTREAM_H
#define QDEBUGSTREAM_H

//################
//# qdebugstream.h  #
//################

#include <iostream>
#include <streambuf>
#include <string>
#include <QMutex>

#include "qtextedit.h"

class QDebugStream : public std::basic_streambuf<char>
{
public:
    QDebugStream(std::ostream &stream, QTextEdit* text_edit) : m_stream(stream)
    {
        log_window = text_edit;
        m_old_buf = stream.rdbuf();
        stream.rdbuf(this);
    }
    ~QDebugStream()
    {
        // output anything that is left
        if (!m_string.empty())
            log_window->append(m_string.c_str());

        m_stream.rdbuf(m_old_buf);
    }

protected:
    virtual int_type overflow(int_type v)
    {
        m_accessMutex.lock();

        if (v == '\n')
        {

            log_window->setTextColor(Qt::black);

            if (m_string.find("ERROR")!=std::string::npos)
                log_window->setTextColor(Qt::darkRed);
            if (m_string.find("INFO")!=std::string::npos)
                log_window->setTextColor(Qt::darkGreen);
            if (m_string.find("WARNING")!=std::string::npos)
                log_window->setTextColor(Qt::darkYellow);
            if (m_string.find("VERBOSE")!=std::string::npos)
                log_window->setTextColor(Qt::gray);

            log_window->append(m_string.c_str());

            // Scroll to bottom of window

            log_window->moveCursor(QTextCursor::End);
            log_window->moveCursor(QTextCursor::StartOfLine);

            m_string.erase(m_string.begin(), m_string.end());
        }
        else
            m_string += v;

        m_accessMutex.unlock();
        return v;
    }

    virtual std::streamsize xsputn(const char *p, std::streamsize n)
    {
        m_accessMutex.lock();
        m_string.append(p, p + n);

        std::string::size_type pos = 0;
        while (pos != std::string::npos)
        {
            pos = m_string.find('\n');
            if (pos != std::string::npos)
            {
                std::string tmp(m_string.begin(), m_string.begin() + pos);

                log_window->setTextColor(Qt::black);

                if (tmp.find("ERROR")!=std::string::npos)
                    log_window->setTextColor(Qt::darkRed);
                if (tmp.find("INFO")!=std::string::npos)
                    log_window->setTextColor(Qt::darkGreen);
                if (tmp.find("WARNING")!=std::string::npos)
                    log_window->setTextColor(Qt::darkYellow);
                if (tmp.find("VERBOSE")!=std::string::npos)
                    log_window->setTextColor(Qt::gray);

                log_window->append(tmp.c_str());

                // Scroll to bottom of window

                log_window->moveCursor(QTextCursor::End);
                log_window->moveCursor(QTextCursor::StartOfLine);

                m_string.erase(m_string.begin(), m_string.begin() + pos + 1);
            }
        }

        m_accessMutex.unlock();
        return n;
    }

private:
    std::ostream &m_stream;
    std::streambuf *m_old_buf;
    std::string m_string;
    QTextEdit* log_window;
    QMutex m_accessMutex;
};

#endif // QDEBUGSTREAM_H
