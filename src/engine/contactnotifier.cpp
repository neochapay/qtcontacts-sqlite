/*
 * Copyright (C) 2013 Jolla Ltd.
 * Copyright (C) 2019 - 2020 Open Mobile Platform LLC.
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

#include "contactnotifier.h"
#include "trace_p.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QVector>
#include <QUuid>

#include <QDebug>

#define NOTIFIER_NAME "org.nemomobile.contacts.sqlite.uuid_%1"
#define NOTIFIER_PATH "/org/nemomobile/contacts/sqlite"
#define NOTIFIER_INTERFACE "org.nemomobile.contacts.sqlite"

Q_DECLARE_METATYPE(QVector<quint32>)

namespace {

bool initialized = false;
void initialize()
{
    if (!initialized) {
        initialized = true;
        qDBusRegisterMetaType<QVector<quint32> >();
    }
}

QString pathName()
{
    return QString::fromLatin1(NOTIFIER_PATH);
}

QString interfaceName(bool nonprivileged)
{
    return QString::fromLatin1(NOTIFIER_INTERFACE) + QString::fromLatin1(nonprivileged ? ".np" : "");
}

QDBusMessage createSignal(const char *name, bool nonprivileged)
{
    return QDBusMessage::createSignal(pathName(), interfaceName(nonprivileged), QString::fromLatin1(name));
}

QVector<quint32> idVector(const QList<QContactId> &contactIds)
{
    QVector<quint32> ids;
    ids.reserve(contactIds.size());
    foreach (const QContactId &id, contactIds) {
        ids.append(ContactId::databaseId(id));
    }
    return ids;
}

QVector<quint32> idVector(const QList<QContactCollectionId> &collectionIds)
{
    QVector<quint32> ids;
    ids.reserve(collectionIds.size());
    foreach (const QContactCollectionId &id, collectionIds) {
        ids.append(ContactCollectionId::databaseId(id));
    }
    return ids;
}

}

ContactNotifier::ContactNotifier(bool nonprivileged)
    : m_nonprivileged(nonprivileged)
{
    initialize();
}

ContactNotifier::~ContactNotifier()
{
    if (QDBusConnection::sessionBus().isConnected() && !m_serviceName.isEmpty()) {
        QDBusConnection::sessionBus().unregisterService(m_serviceName);
    }
}

void ContactNotifier::collectionsAdded(const QList<QContactCollectionId> &collectionIds)
{
    if (!collectionIds.isEmpty()) {
        QDBusMessage message = createSignal("collectionsAdded", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(collectionIds)));
        sendMessage(message);
    }
}

void ContactNotifier::collectionsChanged(const QList<QContactCollectionId> &collectionIds)
{
    if (!collectionIds.isEmpty()) {
        QDBusMessage message = createSignal("collectionsChanged", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(collectionIds)));
        sendMessage(message);
    }
}

void ContactNotifier::collectionsRemoved(const QList<QContactCollectionId> &collectionIds)
{
    if (!collectionIds.isEmpty()) {
        QDBusMessage message = createSignal("collectionsRemoved", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(collectionIds)));
        sendMessage(message);
    }
}

void ContactNotifier::contactsAdded(const QList<QContactId> &contactIds)
{
    if (!contactIds.isEmpty()) {
        QDBusMessage message = createSignal("contactsAdded", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds)));
        sendMessage(message);
    }
}

void ContactNotifier::contactsChanged(const QList<QContactId> &contactIds)
{
    if (!contactIds.isEmpty()) {
        QDBusMessage message = createSignal("contactsChanged", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds)));
        sendMessage(message);
    }
}

void ContactNotifier::contactsPresenceChanged(const QList<QContactId> &contactIds)
{
    if (!contactIds.isEmpty()) {
        QDBusMessage message = createSignal("contactsPresenceChanged", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds)));
        sendMessage(message);
    }
}

// notify that synced contacts have changed in the given collections
void ContactNotifier::collectionContactsChanged(const QList<QContactCollectionId> &collectionIds)
{
    if (!collectionIds.isEmpty()) {
        QDBusMessage message = createSignal("collectionContactsChanged", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(collectionIds)));
        sendMessage(message);
    }
}

void ContactNotifier::contactsRemoved(const QList<QContactId> &contactIds)
{
    if (!contactIds.isEmpty()) {
        QDBusMessage message = createSignal("contactsRemoved", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds)));
        sendMessage(message);
    }
}

void ContactNotifier::selfContactIdChanged(QContactId oldId, QContactId newId)
{
    if (oldId != newId) {
        QDBusMessage message = createSignal("selfContactIdChanged", m_nonprivileged);
        message.setArguments(QVariantList() << QVariant::fromValue(ContactId::databaseId(oldId)) << QVariant::fromValue(ContactId::databaseId(newId)));
        sendMessage(message);
    }
}

void ContactNotifier::relationshipsAdded(const QSet<QContactId> &contactIds)
{
    if (!contactIds.isEmpty()) {
        QDBusMessage message = createSignal("relationshipsAdded", m_nonprivileged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds.values())));
#else
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds.toList())));
#endif
        sendMessage(message);
    }
}

void ContactNotifier::relationshipsRemoved(const QSet<QContactId> &contactIds)
{
    if (!contactIds.isEmpty()) {
        QDBusMessage message = createSignal("relationshipsRemoved", m_nonprivileged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds.values())));
#else
        message.setArguments(QVariantList() << QVariant::fromValue(idVector(contactIds.toList())));
#endif
        sendMessage(message);
    }
}

void ContactNotifier::displayLabelGroupsChanged()
{
    QDBusMessage message = createSignal("displayLabelGroupsChanged", m_nonprivileged);
    sendMessage(message);
}

bool ContactNotifier::connect(const char *name, const char *signature, QObject *receiver, const char *slot)
{
    static QDBusConnection connection(QDBusConnection::sessionBus());

    if (!connection.isConnected()) {
        QTCONTACTS_SQLITE_DEBUG(QString::fromLatin1("Session Bus is not connected"));
        return false;
    }

    if (!connection.connect(QString(),
                            pathName(),
                            interfaceName(m_nonprivileged),
                            QLatin1String(name),
                            QLatin1String(signature),
                            receiver,
                            slot)) {
        QTCONTACTS_SQLITE_DEBUG(QString::fromLatin1("Unable to connect DBUS signal: %1").arg(name));
        return false;
    }

    return true;
}

void ContactNotifier::sendMessage(const QDBusMessage &message)
{
    static QDBusConnection connection(QDBusConnection::sessionBus());

    if (!connection.isConnected()) {
        QTCONTACTS_SQLITE_DEBUG(QString::fromLatin1("Session Bus is not connected"));
        return;
    }

    if (m_serviceName.isEmpty()) {
        // Register a unique name for this signal source on the session bus.
        // Remove surrounding braces and hyphens from the generated uuid.
        const QString uuid = QUuid::createUuid().toString();
        const QString serviceName = QString(NOTIFIER_NAME)
                .arg(uuid.mid(1, uuid.length() - 2).replace('-', QString()));
        if (connection.registerService(serviceName)) {
            m_serviceName = serviceName;
        } else {
            QTCONTACTS_SQLITE_DEBUG(
                    QString::fromLatin1("Failed to register D-Bus service name %1 for contact change notifications: %2 %3")
                        .arg(serviceName)
                        .arg(connection.lastError().name())
                        .arg(connection.lastError().message()));
            return;
        }
    }

    connection.send(message);
}
