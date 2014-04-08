/*
 * Copyright (C) 2013 Jolla Ltd. <andrew.den.exter@jollamobile.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#ifndef QTCONTACTSSQLITE_CONTACTSDATABASE
#define QTCONTACTSSQLITE_CONTACTSDATABASE

#include "semaphore_p.h"

#include <QHash>
#include <QMutex>
#include <QScopedPointer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariantList>

class ContactsDatabase
{
public:
    enum Identity {
        SelfContactId
    };

    class ProcessMutex
    {
        Semaphore m_semaphore;
        bool m_initialProcess;

    public:
        ProcessMutex(const QString &path);

        bool lock();
        bool unlock();

        bool isLocked() const;

        bool isInitialProcess() const;
    };

    // This class is required to finish() each query at destruction
    class Query
    {
        friend class ContactsDatabase;

        QSqlQuery m_query;

        Query(const QSqlQuery &query);

    public:
        ~Query() { finish(); }

        void bindValue(const QString &id, const QVariant &value) { m_query.bindValue(id, value); }
        void bindValue(int pos, const QVariant &value) { m_query.bindValue(pos, value); }

        bool exec() { return m_query.exec(); }
        bool execBatch() { return m_query.execBatch(); }
        bool next() { return m_query.next(); }
        void finish() { return m_query.finish(); }

        QVariant lastInsertId() const { return m_query.lastInsertId(); }

        QVariant value(int index) { return m_query.value(index); }

        template<typename T>
        T value(int index) { return m_query.value(index).value<T>(); }

        void reportError(const QString &text) const;
        void reportError(const char *text) const;
    };

    ContactsDatabase();
    ~ContactsDatabase();

    QMutex *accessMutex() const;
    ProcessMutex *processMutex() const;

    bool open(const QString &databaseName, bool nonprivileged, bool secondaryConnection = false);

    operator QSqlDatabase &();
    operator QSqlDatabase const &() const;

    QSqlError lastError() const;

    bool nonprivileged() const;
    bool aggregating() const;

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    bool createTemporaryContactIdsTable(const QString &table, const QVariantList &boundIds);
    bool createTemporaryContactIdsTable(const QString &table, const QString &join, const QString &where, const QString &orderBy, const QVariantList &boundValues);
    bool createTemporaryContactIdsTable(const QString &table, const QString &join, const QString &where, const QString &orderBy, const QMap<QString, QVariant> &boundValues);

    void clearTemporaryContactIdsTable(const QString &table);

    bool createTemporaryValuesTable(const QString &table, const QVariantList &values);
    void clearTemporaryValuesTable(const QString &table);

    bool createTransientContactIdsTable(const QString &table, const QVariantList &ids, QString *transientTableName);

    void clearTransientContactIdsTable(const QString &table);

    Query prepare(const char *statement);
    Query prepare(const QString &statement);

    static QString expandQuery(const QString &queryString, const QVariantList &bindings);
    static QString expandQuery(const QString &queryString, const QMap<QString, QVariant> &bindings);
    static QString expandQuery(const QSqlQuery &query);

private:
    QSqlDatabase m_database;
    QMutex m_mutex;
    mutable QScopedPointer<ProcessMutex> m_processMutex;
    bool m_nonprivileged;
    QHash<QString, QSqlQuery> m_preparedQueries;
};

#endif
