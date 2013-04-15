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

#include "contactwriter.h"

#include "contactnotifier.h"

#include <QContactFavorite>
#include <QContactGender>
#include <QContactGlobalPresence>
#include <QContactName>
#include <QContactTimestamp>

#include <QSqlError>
#include <QVector>

#include <QtDebug>

static const char *checkContactExists =
        "\n SELECT COUNT(contactId) FROM Contacts WHERE contactId = :contactId;";

static const char *existingContactIds =
        "\n SELECT DISTINCT contactId FROM Contacts;";

static const char *selfContactId =
        "\n SELECT DISTINCT contactId FROM Identities WHERE identity = :identity;";

static const char *insertContact =
        "\n INSERT INTO Contacts ("
        "\n  displayLabel,"
        "\n  firstName,"
        "\n  lastName,"
        "\n  middleName,"
        "\n  prefix,"
        "\n  suffix,"
        "\n  customLabel,"
        "\n  created,"
        "\n  modified,"
        "\n  gender,"
        "\n  isFavorite)"
        "\n VALUES ("
        "\n  :displayLabel,"
        "\n  :firstName,"
        "\n  :lastName,"
        "\n  :middleName,"
        "\n  :prefix,"
        "\n  :suffix,"
        "\n  :customLabel,"
        "\n  :created,"
        "\n  :modified,"
        "\n  :gender,"
        "\n  :isFavorite);";

static const char *updateContact =
        "\n UPDATE Contacts SET"
        "\n  displayLabel = :displayLabel,"
        "\n  firstName = :firstName,"
        "\n  lastName = :lastName,"
        "\n  middleName = :middleName,"
        "\n  prefix = :prefix,"
        "\n  suffix = :suffix,"
        "\n  customLabel = :customLabel,"
        "\n  created = :created,"
        "\n  modified = :modified,"
        "\n  gender = :gender,"
        "\n  isFavorite = :isFavorite"
        "\n WHERE contactId = :contactId;";

static const char *removeContact =
        "\n DELETE FROM Contacts WHERE contactId = :contactId;";

static const char *existingRelationships =
        "\n SELECT firstId, secondId, type FROM Relationships;";

static const char *insertRelationship =
        "\n INSERT INTO Relationships ("
        "\n  firstId,"
        "\n  secondId,"
        "\n  type)"
        "\n VALUES ("
        "\n  :firstId,"
        "\n  :secondId,"
        "\n  :type);";

static const char *removeRelationship =
        "\n DELETE FROM Relationships"
        "\n WHERE firstId = :firstId AND secondId = :secondId AND type = :type;";

static const char *insertAddress =
        "\n INSERT INTO Addresses ("
        "\n  contactId,"
        "\n  street,"
        "\n  postOfficeBox,"
        "\n  region,"
        "\n  locality,"
        "\n  postCode,"
        "\n  country)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :street,"
        "\n  :postOfficeBox,"
        "\n  :region,"
        "\n  :locality,"
        "\n  :postCode,"
        "\n  :country)";

static const char *insertAnniversary =
        "\n INSERT INTO Anniversaries ("
        "\n  contactId,"
        "\n  originalDateTime,"
        "\n  calendarId,"
        "\n  subType)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :originalDateTime,"
        "\n  :calendarId,"
        "\n  :subType)";

static const char *insertAvatar =
        "\n INSERT INTO Avatars ("
        "\n  contactId,"
        "\n  imageUrl,"
        "\n  videoUrl)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :imageUrl,"
        "\n  :videoUrl)";

static const char *insertBirthday =
        "\n INSERT INTO Birthdays ("
        "\n  contactId,"
        "\n  birthday,"
        "\n  calendarId)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :birthday,"
        "\n  :calendarId)";

static const char *insertEmailAddress =
        "\n INSERT INTO EmailAddresses ("
        "\n  contactId,"
        "\n  emailAddress)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :emailAddress)";

static const char *insertGlobalPresence =
        "\n INSERT INTO GlobalPresences ("
        "\n  contactId,"
        "\n  presenceState,"
        "\n  timestamp,"
        "\n  nickname,"
        "\n  customMessage)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :presenceState,"
        "\n  :timestamp,"
        "\n  :nickname,"
        "\n  :customMessage)";

static const char *insertGuid =
        "\n INSERT INTO Guids ("
        "\n  contactId,"
        "\n  guid)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :guid)";

static const char *insertHobby =
        "\n INSERT INTO Hobbies ("
        "\n  contactId,"
        "\n  hobby)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :hobby)";

static const char *insertNickname =
        "\n INSERT INTO Nicknames ("
        "\n  contactId,"
        "\n  nickname)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :nickname)";

static const char *insertNote =
        "\n INSERT INTO Notes ("
        "\n  contactId,"
        "\n  note)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :note)";

static const char *insertOnlineAccount =
        "\n INSERT INTO OnlineAccounts ("
        "\n  contactId,"
        "\n  accountUri,"
        "\n  protocol,"
        "\n  serviceProvider,"
        "\n  capabilities,"
        "\n  subTypes,"
        "\n  accountPath,"
        "\n  accountIconPath,"
        "\n  enabled)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :accountUri,"
        "\n  :protocol,"
        "\n  :serviceProvider,"
        "\n  :capabilities,"
        "\n  :subTypes,"
        "\n  :accountPath,"
        "\n  :accountIconPath,"
        "\n  :enabled)";

static const char *insertOrganization =
        "\n INSERT INTO Organizations ("
        "\n  contactId,"
        "\n  name,"
        "\n  role,"
        "\n  title,"
        "\n  location,"
        "\n  department,"
        "\n  logoUrl)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :name,"
        "\n  :role,"
        "\n  :title,"
        "\n  :location,"
        "\n  :department,"
        "\n  :logoUrl)";

static const char *insertPhoneNumber =
        "\n INSERT INTO PhoneNumbers ("
        "\n  contactId,"
        "\n  phoneNumber,"
        "\n  subTypes,"
        "\n  normalizedNumber)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :phoneNumber,"
        "\n  :subTypes,"
        "\n  :normalizedNumber)";

static const char *insertPresence =
        "\n INSERT INTO Presences ("
        "\n  contactId,"
        "\n  presenceState,"
        "\n  timestamp,"
        "\n  nickname,"
        "\n  customMessage)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :presenceState,"
        "\n  :timestamp,"
        "\n  :nickname,"
        "\n  :customMessage)";

static const char *insertRingtone =
        "\n INSERT INTO Ringtones ("
        "\n  contactId,"
        "\n  audioRingtone,"
        "\n  videoRingtone)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :audioRingtone,"
        "\n  :videoRingtone)";


static const char *insertSyncTarget =
        "\n INSERT INTO SyncTargets ("
        "\n  contactId,"
        "\n  syncTarget)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :syncTarget)";

static const char *insertTag =
        "\n INSERT INTO Tags ("
        "\n  contactId,"
        "\n  tag)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :tag)";

static const char *insertUrl =
        "\n INSERT INTO Urls ("
        "\n  contactId,"
        "\n  url,"
        "\n  subTypes)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :url,"
        "\n  :subTypes)";

static const char *insertTpMetadata =
        "\n INSERT INTO TpMetadata ("
        "\n  contactId,"
        "\n  telepathyId,"
        "\n  accountId,"
        "\n  accountEnabled)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :telepathyId,"
        "\n  :accountId,"
        "\n  :accountEnabled)";

static const char *insertDetail =
        "\n INSERT INTO Details ("
        "\n  contactId,"
        "\n  detailId,"
        "\n  detail,"
        "\n  detailUri,"
        "\n  linkedDetailUris,"
        "\n  contexts)"
        "\n VALUES ("
        "\n  :contactId,"
        "\n  :detailId,"
        "\n  :detail,"
        "\n  :detailUri,"
        "\n  :linkedDetailUris,"
        "\n  :contexts);";

static const char *insertIdentity =
        "\n INSERT OR REPLACE INTO Identities ("
        "\n  identity,"
        "\n  contactId)"
        "\n VALUES ("
        "\n  :identity,"
        "\n  :contactId);";


static QSqlQuery prepare(const char *statement, const QSqlDatabase &database)
{
    return ContactsDatabase::prepare(statement, database);
}

ContactWriter::ContactWriter(const QSqlDatabase &database)
    : m_database(database)
    , m_checkContactExists(prepare(checkContactExists, database))
    , m_existingContactIds(prepare(existingContactIds, database))
    , m_selfContactId(prepare(selfContactId, database))
    , m_insertContact(prepare(insertContact, database))
    , m_updateContact(prepare(updateContact, database))
    , m_removeContact(prepare(removeContact, database))
    , m_existingRelationships(prepare(existingRelationships, database))
    , m_insertRelationship(prepare(insertRelationship, database))
    , m_removeRelationship(prepare(removeRelationship, database))
    , m_insertAddress(prepare(insertAddress, database))
    , m_insertAnniversary(prepare(insertAnniversary, database))
    , m_insertAvatar(prepare(insertAvatar, database))
    , m_insertBirthday(prepare(insertBirthday, database))
    , m_insertEmailAddress(prepare(insertEmailAddress, database))
    , m_insertGlobalPresence(prepare(insertGlobalPresence, database))
    , m_insertGuid(prepare(insertGuid, database))
    , m_insertHobby(prepare(insertHobby, database))
    , m_insertNickname(prepare(insertNickname, database))
    , m_insertNote(prepare(insertNote, database))
    , m_insertOnlineAccount(prepare(insertOnlineAccount, database))
    , m_insertOrganization(prepare(insertOrganization, database))
    , m_insertPhoneNumber(prepare(insertPhoneNumber, database))
    , m_insertPresence(prepare(insertPresence, database))
    , m_insertRingtone(prepare(insertRingtone, database))
    , m_insertSyncTarget(prepare(insertSyncTarget, database))
    , m_insertTag(prepare(insertTag, database))
    , m_insertUrl(prepare(insertUrl, database))
    , m_insertTpMetadata(prepare(insertTpMetadata, database))
    , m_insertDetail(prepare(insertDetail, database))
    , m_insertIdentity(prepare(insertIdentity, database))
    , m_removeAddress(prepare("DELETE FROM Addresses WHERE contactId = :contactId;", database))
    , m_removeAnniversary(prepare("DELETE FROM Anniversaries WHERE contactId = :contactId;", database))
    , m_removeAvatar(prepare("DELETE FROM Avatars WHERE contactId = :contactId;", database))
    , m_removeBirthday(prepare("DELETE FROM Birthdays WHERE contactId = :contactId;", database))
    , m_removeEmailAddress(prepare("DELETE FROM EmailAddresses WHERE contactId = :contactId;", database))
    , m_removeGlobalPresence(prepare("DELETE FROM GlobalPresences WHERE contactId = :contactId;", database))
    , m_removeGuid(prepare("DELETE FROM Guids WHERE contactId = :contactId;", database))
    , m_removeHobby(prepare("DELETE FROM Hobbies WHERE contactId = :contactId;", database))
    , m_removeNickname(prepare("DELETE FROM Nicknames WHERE contactId = :contactId;", database))
    , m_removeNote(prepare("DELETE FROM Notes WHERE contactId = :contactId;", database))
    , m_removeOnlineAccount(prepare("DELETE FROM OnlineAccounts WHERE contactId = :contactId;", database))
    , m_removeOrganization(prepare("DELETE FROM Organizations WHERE contactId = :contactId;", database))
    , m_removePhoneNumber(prepare("DELETE FROM PhoneNumbers WHERE contactId = :contactId;", database))
    , m_removePresence(prepare("DELETE FROM Presences WHERE contactId = :contactId;", database))
    , m_removeRingtone(prepare("DELETE FROM Ringtones WHERE contactId = :contactId;", database))
    , m_removeSyncTarget(prepare("DELETE FROM SyncTargets WHERE contactId = :contactId;", database))
    , m_removeTag(prepare("DELETE FROM Tags WHERE contactId = :contactId;", database))
    , m_removeUrl(prepare("DELETE FROM Urls WHERE contactId = :contactId;", database))
    , m_removeTpMetadata(prepare("DELETE FROM TpMetadata WHERE contactId = :contactId;", database))
    , m_removeDetail(prepare("DELETE FROM Details WHERE contactId = :contactId AND detail = :detail;", database))
    , m_removeIdentity(prepare("DELETE FROM Identities WHERE identity = :identity;", database))
{
}

ContactWriter::~ContactWriter()
{
}

QContactManager::Error ContactWriter::setIdentity(
        ContactsDatabase::Identity identity, QContactLocalId contactId)
{
    QSqlQuery *query = 0;
    if (contactId != 0) {
        m_insertIdentity.bindValue(0, identity);
        m_insertIdentity.bindValue(1, contactId - 1);
        query = &m_insertIdentity;
    } else {
        m_removeIdentity.bindValue(0, identity);
        query = &m_removeIdentity;
    }

    if (query->exec()) {
        // Notify..

        return QContactManager::NoError;
    } else {
        return QContactManager::UnspecifiedError;
    }
}

static QContactManager::Error bindRelationships(
        QSqlQuery *query,
        const QList<QContactRelationship> &relationships,
        QMap<int, QContactManager::Error> *errorMap,
        QSet<QContactLocalId> *contactIds,
        QMultiMap<QContactLocalId, QPair<QString, QContactLocalId> > *bucketedRelationships,
        int *removedDuplicatesCount)
{
    QVariantList firstIds;
    QVariantList secondIds;
    QVariantList types;
    *removedDuplicatesCount = 0;

    for (int i = 0; i < relationships.count(); ++i) {
        const QContactRelationship &relationship = relationships.at(i);
        const QContactLocalId firstId = relationship.first().localId();
        const QContactLocalId secondId = relationship.second().localId();
        const QString &type = relationship.relationshipType();

        if (firstId == 0 || secondId == 0) {
            if (errorMap)
                errorMap->insert(i, QContactManager::UnspecifiedError);
        } else if (type.isEmpty()) {
            if (errorMap)
                errorMap->insert(i, QContactManager::UnspecifiedError);
        } else {
            if (bucketedRelationships->find(firstId, QPair<QString, QContactLocalId>(type, secondId)) != bucketedRelationships->end()) {
                // this relationship is already represented in our database.
                // according to the semantics defined in tst_qcontactmanager,
                // we allow saving duplicates by "overwriting" (with identical values)
                // which means that we simply "drop" this one from the list
                // of relationships to add to the database.
                *removedDuplicatesCount += 1;
            } else {
                // this relationships has not yet been represented in our database.
                firstIds.append(firstId - 1);
                secondIds.append(secondId - 1);
                types.append(type);

                contactIds->insert(firstId);
                contactIds->insert(secondId);

                bucketedRelationships->insert(firstId, QPair<QString, QContactLocalId>(type, secondId));
            }
        }
    }

    if (firstIds.isEmpty() && *removedDuplicatesCount == 0) {
        // if we "successfully overwrote" some duplicates, it's not an error.
        return QContactManager::UnspecifiedError;
    }

    if (firstIds.size() == 1) {
        query->bindValue(0, firstIds.at(0).toUInt());
        query->bindValue(1, secondIds.at(0).toUInt());
        query->bindValue(2, types.at(0).toString());
    } else if (firstIds.size() > 1) {
        query->bindValue(0, firstIds);
        query->bindValue(1, secondIds);
        query->bindValue(2, types);
    }

    return QContactManager::NoError;
}

QContactManager::Error ContactWriter::save(
        const QList<QContactRelationship> &relationships, QMap<int, QContactManager::Error> *errorMap)
{
    if (relationships.isEmpty())
        return QContactManager::NoError;

    // in order to perform duplicate detection we build up the following datastructure.
    QMultiMap<QContactLocalId, QPair<QString, QContactLocalId> > bucketedRelationships; // first id to <type, second id>.
    {
        if (!m_existingRelationships.exec()) {
            qWarning() << "Failed to fetch existing relationships for duplicate detection during insert:";
            qWarning() << m_existingRelationships.lastError();
            return QContactManager::UnspecifiedError;
        }

        while (m_existingRelationships.next()) {
            QContactLocalId fid = (m_existingRelationships.value(0).toUInt() + 1);
            QContactLocalId sid = (m_existingRelationships.value(1).toUInt() + 1);
            QString rt = m_existingRelationships.value(2).toString();
            bucketedRelationships.insert(fid, QPair<QString, QContactLocalId>(rt, sid));
        }
    }

    // in order to perform validity detection we build up the following set.
    // XXX TODO: use foreign key constraint or similar in Relationships table?
    QSet<QContactLocalId> validContactIds;
    {
        if (!m_existingContactIds.exec()) {
            qWarning() << "Failed to fetch existing contacts for validity detection during insert:";
            qWarning() << m_existingContactIds.lastError();
            return QContactManager::UnspecifiedError;
        }

        while (m_existingContactIds.next()) {
            validContactIds.insert(m_existingContactIds.value(0).toUInt() + 1);
        }
    }

    QList<QContactLocalId> firstIdsToBind;
    QList<QContactLocalId> secondIdsToBind;
    QList<QString> typesToBind;

    QSqlQuery multiInsertQuery(m_database);
    QString queryString = QLatin1String("INSERT INTO Relationships");
    int realInsertions = 0;
    int invalidInsertions = 0;
    for (int i = 0; i < relationships.size(); ++i) {
        const QContactRelationship &relationship = relationships.at(i);
        const QContactLocalId firstId = relationship.first().localId();
        const QContactLocalId secondId = relationship.second().localId();
        const QString &type = relationship.relationshipType();

        if ((firstId == secondId)
                || (!relationship.first().managerUri().isEmpty()
                    && relationship.first().managerUri() != QLatin1String("org.nemomobile.contacts.sqlite"))
                || (!relationship.second().managerUri().isEmpty()
                    && relationship.second().managerUri() != QLatin1String("org.nemomobile.contacts.sqlite"))
                || (!validContactIds.contains(firstId) || !validContactIds.contains(secondId))) {
            // invalid contact specified in relationship, don't insert.
            invalidInsertions += 1;
            if (errorMap)
                errorMap->insert(i, QContactManager::InvalidRelationshipError);
            continue;
        }

        if (bucketedRelationships.find(firstId, QPair<QString, QContactLocalId>(type, secondId)) != bucketedRelationships.end()) {
            // duplicate, don't insert.
            continue;
        } else {
            if (realInsertions == 0) {
                queryString += QString(QLatin1String("\n SELECT :firstId%1 as firstId, :secondId%1 as secondId, :type%1 as type"))
                                      .arg(QString::number(realInsertions));
            } else {
                queryString += QString(QLatin1String("\n UNION SELECT :firstId%1, :secondId%1, :type%1"))
                                      .arg(QString::number(realInsertions));
            }
            firstIdsToBind.append(firstId);
            secondIdsToBind.append(secondId);
            typesToBind.append(type);
            bucketedRelationships.insert(firstId, QPair<QString, QContactLocalId>(type, secondId));
            realInsertions += 1;
        }
    }

    if (realInsertions > 0 && !multiInsertQuery.prepare(queryString)) {
        qWarning() << "Failed to prepare multiple insert relationships query";
        qWarning() << multiInsertQuery.lastError();
        qWarning() << "Query:\n" << queryString;
        return QContactManager::UnspecifiedError;
    }

    for (int i = 0; i < realInsertions; ++i) {
        multiInsertQuery.bindValue(QString(QLatin1String(":firstId%1")).arg(QString::number(i)), (firstIdsToBind.at(i) - 1));
        multiInsertQuery.bindValue(QString(QLatin1String(":secondId%1")).arg(QString::number(i)), (secondIdsToBind.at(i) - 1));
        multiInsertQuery.bindValue(QString(QLatin1String(":type%1")).arg(QString::number(i)), typesToBind.at(i));
    }

    if (realInsertions > 0 && !multiInsertQuery.exec()) {
        qWarning() << "Failed to insert relationships";
        qWarning() << multiInsertQuery.lastError();
        qWarning() << "Query:\n" << queryString;
        return QContactManager::UnspecifiedError;
    }

    // Notify

    if (invalidInsertions > 0) {
        return QContactManager::InvalidRelationshipError;
    }

    return QContactManager::NoError;
}

QContactManager::Error ContactWriter::remove(
        const QList<QContactRelationship> &relationships, QMap<int, QContactManager::Error> *errorMap)
{
    if (relationships.isEmpty())
        return QContactManager::NoError;

    // in order to perform existence detection we build up the following datastructure.
    QMultiMap<QContactLocalId, QPair<QString, QContactLocalId> > bucketedRelationships; // first id to <type, second id>.
    {
        if (!m_existingRelationships.exec()) {
            qWarning() << "Failed to fetch existing relationships for duplicate detection during insert:";
            qWarning() << m_existingRelationships.lastError();
            return QContactManager::UnspecifiedError;
        }

        while (m_existingRelationships.next()) {
            QContactLocalId fid = (m_existingRelationships.value(0).toUInt() + 1);
            QContactLocalId sid = (m_existingRelationships.value(1).toUInt() + 1);
            QString rt = m_existingRelationships.value(2).toString();
            bucketedRelationships.insert(fid, QPair<QString, QContactLocalId>(rt, sid));
        }
    }

    QContactManager::Error worstError = QContactManager::NoError;
    QSet<QContactRelationship> alreadyRemoved;
    bool removeInvalid = false;
    for (int i = 0; i < relationships.size(); ++i) {
        QContactRelationship curr = relationships.at(i);
        if (alreadyRemoved.contains(curr)) {
            continue;
        }

        if (bucketedRelationships.find(curr.first().localId(), QPair<QString, QContactLocalId>(curr.relationshipType(), curr.second().localId())) == bucketedRelationships.end()) {
            removeInvalid = true;
            if (errorMap)
                errorMap->insert(i, QContactManager::DoesNotExistError);
            continue;
        }

        QSqlQuery removeRelationship(m_database);
        if (!removeRelationship.prepare("DELETE FROM Relationships WHERE firstId = :firstId AND secondId = :secondId AND type = :type;")) {
            qWarning() << "Failed to prepare remove relationship";
            qWarning() << removeRelationship.lastError();
            worstError = QContactManager::UnspecifiedError;
            if (errorMap)
                errorMap->insert(i, worstError);
            continue;
        }

        removeRelationship.bindValue(":firstId", curr.first().localId() - 1);
        removeRelationship.bindValue(":secondId", curr.second().localId() - 1);
        removeRelationship.bindValue(":type", curr.relationshipType());

        if (!removeRelationship.exec()) {
            qWarning() << "Failed to remove relationship";
            qWarning() << removeRelationship.lastError();
            worstError = QContactManager::UnspecifiedError;
            if (errorMap)
                errorMap->insert(i, worstError);
            continue;
        }

        alreadyRemoved.insert(curr);
    }

    // Notify

    if (removeInvalid) {
        return QContactManager::DoesNotExistError;
    }

    return QContactManager::NoError;
}

QContactManager::Error ContactWriter::remove(const QList<QContactLocalId> &contactIds, QMap<int, QContactManager::Error> *errorMap)
{
    if (contactIds.isEmpty())
        return QContactManager::NoError;

    QContactLocalId selfContactId = 0;
    m_selfContactId.bindValue(":identity", ContactsDatabase::SelfContactId);
    if (!m_selfContactId.exec()) {
        qWarning() << "Failed to fetch self contact id during remove";
        qWarning() << m_selfContactId.lastError();
        return QContactManager::UnspecifiedError;
    }

    if (m_selfContactId.next()) {
        selfContactId = m_selfContactId.value(0).toUInt() + 1;
    }

    QSet<QContactLocalId> existingContactIds;
    if (!m_existingContactIds.exec()) {
        qWarning() << "Failed to fetch existing contact ids during remove";
        qWarning() << m_existingContactIds.lastError();
        return QContactManager::UnspecifiedError;
    }

    while (m_existingContactIds.next()) {
        existingContactIds.insert(m_existingContactIds.value(0).toUInt() + 1);
    }

    QContactManager::Error error = QContactManager::NoError;
    QVariantList boundValues;
    QList<QContactLocalId> realRemoveIds;
    for (int i = 0; i < contactIds.size(); ++i) {
        QContactLocalId currLId = contactIds.at(i);
        if (selfContactId > 0 && currLId == selfContactId) {
            if (errorMap)
                errorMap->insert(i, QContactManager::BadArgumentError);
            error = QContactManager::BadArgumentError;
        } else if (existingContactIds.contains(currLId)) {
            boundValues.append(currLId - 1);
            realRemoveIds.append(currLId);
        } else {
            if (errorMap)
                errorMap->insert(i, QContactManager::DoesNotExistError);
            error = QContactManager::DoesNotExistError;
        }
    }

    m_removeContact.bindValue(QLatin1String(":contactId"), boundValues);
    if (realRemoveIds.size() > 0) {
        if (!m_removeContact.execBatch()) {
            qWarning() << "Failed to removed contacts";
            qWarning() << m_removeContact.lastError();
            return QContactManager::UnspecifiedError;
        }

        ContactNotifier::contactsRemoved(realRemoveIds);
    }

    return error;
}

template <typename T> bool ContactWriter::removeCommonDetails(
            QContactLocalId contactId, QContactManager::Error *error)
{
    m_removeDetail.bindValue(0, contactId);
    m_removeDetail.bindValue(1, T::DefinitionName);

    if (!m_removeDetail.exec()) {
        qWarning() << "Failed to remove common detail for" << QLatin1String(T::DefinitionName);
        qWarning() << m_removeDetail.lastError();
        *error = QContactManager::UnspecifiedError;
        return false;
    }
    return true;
}

template <typename T> bool ContactWriter::writeCommonDetails(
            QContactLocalId contactId, const QSqlQuery &query, const T &detail, QContactManager::Error *error)
{
    const QVariant detailUri = detail.variantValue(QContactDetail::FieldDetailUri);
    const QVariant linkedDetailUris = detail.variantValue(QContactDetail::FieldLinkedDetailUris);
    const QVariant contexts = detail.variantValue(QContactDetail::FieldContext);

    if (detailUri.isValid() || linkedDetailUris.isValid() || contexts.isValid()) {
        m_insertDetail.bindValue(0, contactId);
        m_insertDetail.bindValue(1, query.lastInsertId());
        m_insertDetail.bindValue(2, T::DefinitionName);
        m_insertDetail.bindValue(3, detailUri);
        m_insertDetail.bindValue(4, linkedDetailUris.toStringList().join(QLatin1String(";")));
        m_insertDetail.bindValue(5, contexts.toStringList().join(QLatin1String(";")));

        if (!m_insertDetail.exec()) {
            qWarning() << "Failed to write common details for" << QLatin1String(T::DefinitionName);
            qWarning() << m_insertDetail.lastError();
            *error = QContactManager::UnspecifiedError;
            return false;
        }
    }
    return true;
}

template <typename T> bool ContactWriter::writeDetails(
        QContactLocalId contactId,
        QContact *contact,
        QSqlQuery &removeQuery,
        const QStringList &definitionMask,
        QContactManager::Error *error)
{
    if (!definitionMask.isEmpty() && !definitionMask.contains(T::DefinitionName))
        return true;

    if (!removeCommonDetails<T>(contactId, error))
        return false;

    removeQuery.bindValue(0, contactId);
    if (!removeQuery.exec()) {
        qWarning() << "Failed to remove existing details for" << QLatin1String(T::DefinitionName);
        qWarning() << removeQuery.lastError();
        *error = QContactManager::UnspecifiedError;
        return false;
    }

    foreach (const T &detail, contact->details<T>()) {
        QSqlQuery &query = bindDetail(contactId, detail);
        if (!query.exec()) {
            qWarning() << "Failed to write details for" << QLatin1String(T::DefinitionName);
            qWarning() << query.lastError();
            *error = QContactManager::UnspecifiedError;
            return false;
        }

        if (!writeCommonDetails(contactId, query, detail, error))
            return false;
    }
    return true;
}

static bool betterPresence(const QContactPresence &detail, const QContactPresence &best)
{
    if (best.isEmpty())
        return true;

    if (best.presenceState() == QContactPresence::PresenceUnknown) {
        return (detail.presenceState() != QContactPresence::PresenceUnknown);
    }

    return (detail.presenceState() < best.presenceState());
}

template <> bool ContactWriter::writeDetails<QContactPresence>(
        QContactLocalId contactId,
        QContact *contact,
        QSqlQuery &removeQuery,
        const QStringList &definitionMask,
        QContactManager::Error *error)
{
    if (!definitionMask.isEmpty() && !definitionMask.contains(QContactPresence::DefinitionName))
        return true;

    if (!removeCommonDetails<QContactPresence>(contactId, error))
        return false;

    removeQuery.bindValue(0, contactId);
    if (!removeQuery.exec()) {
        qWarning() << "Failed to remove existing details for" << QLatin1String(QContactPresence::DefinitionName);
        qWarning() << removeQuery.lastError();
        *error = QContactManager::UnspecifiedError;
        return false;
    }

    m_removeGlobalPresence.bindValue(0, contactId);
    if (!m_removeGlobalPresence.exec()) {
        qWarning() << "Failed to remove existing details for" << QLatin1String(QContactGlobalPresence::DefinitionName);
        qWarning() << removeQuery.lastError();
        *error = QContactManager::UnspecifiedError;
        return false;
    }

    QContactGlobalPresence globalPresence = contact->detail<QContactGlobalPresence>();
    const QList<QContactPresence> details = contact->details<QContactPresence>();
    if (details.isEmpty()) {
        // No presence - remove global presence if present
        if (!globalPresence.isEmpty()) {
            contact->removeDetail(&globalPresence);
        }
        return true;
    }

    QContactPresence bestPresence;

    foreach (const QContactPresence &detail, details) {
        if (betterPresence(detail, bestPresence)) {
            bestPresence = detail;
        }

        QSqlQuery &query = bindDetail(contactId, detail);
        if (!query.exec()) {
            qWarning() << "Failed to write details for" << QLatin1String(QContactPresence::DefinitionName);
            qWarning() << query.lastError();
            *error = QContactManager::UnspecifiedError;
            return false;
        }

        if (!writeCommonDetails(contactId, query, detail, error))
            return false;
    }

    m_insertGlobalPresence.bindValue(0, contactId);
    m_insertGlobalPresence.bindValue(1, bestPresence.presenceState());
    m_insertGlobalPresence.bindValue(2, bestPresence.timestamp());
    m_insertGlobalPresence.bindValue(3, bestPresence.nickname());
    m_insertGlobalPresence.bindValue(4, bestPresence.customMessage());
    if (!m_insertGlobalPresence.exec()) {
        qWarning() << "Failed to write details for" << QLatin1String(QContactGlobalPresence::DefinitionName);
        qWarning() << m_insertGlobalPresence.lastError();
        *error = QContactManager::UnspecifiedError;
        return false;
    }

    globalPresence.setPresenceState(bestPresence.presenceState());
    globalPresence.setTimestamp(bestPresence.timestamp());
    globalPresence.setNickname(bestPresence.nickname());
    globalPresence.setCustomMessage(bestPresence.customMessage());

    contact->saveDetail(&globalPresence);

    return true;
}

QContactManager::Error ContactWriter::save(
            QList<QContact> *contacts,
            const QStringList &definitionMask,
            QMap<int, QContactManager::Error> *errorMap)
{
    if (contacts->isEmpty())
        return QContactManager::NoError;

    if (!m_database.transaction())
        return QContactManager::UnspecifiedError;

    QVector<QContactLocalId> createdContactIds;
    QVector<QContactLocalId> updatedContactIds;

    QContactManager::Error worstError = QContactManager::NoError;
    QContactManager::Error err = QContactManager::NoError;
    for (int i = 0; i < contacts->count(); ++i) {
        QContact &contact = (*contacts)[i];
        const QContactLocalId contactId = contact.localId();
        if (contactId == 0) {
            err = create(&contact, definitionMask);
            createdContactIds.append(contact.localId());
        } else {
            err = update(&contact, definitionMask);
            updatedContactIds.append(contactId);
        }
        if (err != QContactManager::NoError) {
            worstError = err;
            if (errorMap) {
                errorMap->insert(i, err);
            }
        }
    }

    if (!m_database.commit()) {
        qWarning() << "Failed to commit contacts";
        qWarning() << m_database.lastError();
        return QContactManager::UnspecifiedError;
    }

    if (!createdContactIds.isEmpty())
        ContactNotifier::contactsAdded(createdContactIds);
    if (!updatedContactIds.isEmpty())
        ContactNotifier::contactsChanged(updatedContactIds);

    return worstError;
}

QContactManager::Error ContactWriter::create(QContact *contact, const QStringList &definitionMask)
{
    bindContactDetails(*contact, m_insertContact);
    if (!m_insertContact.exec()) {
        qWarning() << "Failed to create contact";
        qWarning() << m_insertContact.lastError();
        return QContactManager::UnspecifiedError;
    } else {
        QContactLocalId contactId = m_insertContact.lastInsertId().toUInt();
        QContactManager::Error writeErr = write(contactId, contact, definitionMask);
        if (writeErr == QContactManager::NoError) {
            // successfully saved all data.  Update id.
            QContactId id;
            id.setLocalId(contactId + 1);
            id.setManagerUri(QLatin1String("org.nemomobile.contacts.sqlite"));
            contact->setId(id);
        } else {
            // error occurred.  Remove the failed entry.
            m_removeContact.bindValue(":contactId", contactId);
            if (!m_removeContact.exec()) {
                qWarning() << "Unable to remove stale contact after failed save";
                qWarning() << m_removeContact.lastError().text();
            }
        }

        return writeErr;
    }
}

QContactManager::Error ContactWriter::update(QContact *contact, const QStringList &definitionMask)
{
    // 0 is an invalid QContactLocalId but a valid sqlite row id, so ids are all offset by one.
    QContactLocalId contactId = contact->localId() - 1;

    m_checkContactExists.bindValue(0, contactId);
    if (!m_checkContactExists.exec()) {
        qWarning() << "Failed to check contact existence";
        qWarning() << m_checkContactExists.lastError();
        return QContactManager::UnspecifiedError;
    }
    m_checkContactExists.next();
    int exists = m_checkContactExists.value(0).toInt();
    if (!exists) {
        return QContactManager::DoesNotExistError;
    }

    bindContactDetails(*contact, m_updateContact);
    m_updateContact.bindValue(11, contactId);

    if (!m_updateContact.exec()) {
        qWarning() << "Failed to update contact";
        qWarning() << m_updateContact.lastError();
        return QContactManager::UnspecifiedError;
    }

    return write(contactId, contact, definitionMask);
}

QContactManager::Error ContactWriter::write(QContactLocalId contactId, QContact *contact, const QStringList &definitionMask)
{
    QContactManager::Error error = QContactManager::NoError;

    // look for unsupported detail data.  XXX TODO: this is really slow, due to string comparison.
    // We could simply ignore all unsupported data during save, which would save quite some time.
    QList<QContactDetail> allDets = contact->details();
    foreach (const QContactDetail &det, allDets) {
        if (det.definitionName() != QContactType::DefinitionName
                && det.definitionName() != QContactDisplayLabel::DefinitionName
                && det.definitionName() != QContactName::DefinitionName
                && det.definitionName() != QContactSyncTarget::DefinitionName
                && det.definitionName() != QContactGuid::DefinitionName
                && det.definitionName() != QContactNickname::DefinitionName
                && det.definitionName() != QContactFavorite::DefinitionName
                && det.definitionName() != QContactGender::DefinitionName
                && det.definitionName() != QContactTimestamp::DefinitionName
                && det.definitionName() != QContactPhoneNumber::DefinitionName
                && det.definitionName() != QContactEmailAddress::DefinitionName
                && det.definitionName() != QContactBirthday::DefinitionName
                && det.definitionName() != QContactAvatar::DefinitionName
                && det.definitionName() != QContactOnlineAccount::DefinitionName
                && det.definitionName() != QContactPresence::DefinitionName
                && det.definitionName() != QContactGlobalPresence::DefinitionName
                && det.definitionName() != QContactTpMetadata::DefinitionName
                && det.definitionName() != QContactAddress::DefinitionName
                && det.definitionName() != QContactTag::DefinitionName
                && det.definitionName() != QContactUrl::DefinitionName
                && det.definitionName() != QContactAnniversary::DefinitionName
                && det.definitionName() != QContactHobby::DefinitionName
                && det.definitionName() != QContactNote::DefinitionName
                && det.definitionName() != QContactOrganization::DefinitionName
                && det.definitionName() != QContactRingtone::DefinitionName) {
            return QContactManager::InvalidDetailError;
        }
    }

    if (writeDetails<QContactAddress>(contactId, contact, m_removeAddress, definitionMask, &error)
            && writeDetails<QContactAnniversary>(contactId, contact, m_removeAnniversary, definitionMask, &error)
            && writeDetails<QContactAvatar>(contactId, contact, m_removeAvatar, definitionMask, &error)
            && writeDetails<QContactBirthday>(contactId, contact, m_removeBirthday, definitionMask, &error)
            && writeDetails<QContactEmailAddress>(contactId, contact, m_removeEmailAddress, definitionMask, &error)
            && writeDetails<QContactGuid>(contactId, contact, m_removeGuid, definitionMask, &error)
            && writeDetails<QContactHobby>(contactId, contact, m_removeHobby, definitionMask, &error)
            && writeDetails<QContactNickname>(contactId, contact, m_removeNickname, definitionMask, &error)
            && writeDetails<QContactNote>(contactId, contact, m_removeNote, definitionMask, &error)
            && writeDetails<QContactOnlineAccount>(contactId, contact, m_removeOnlineAccount, definitionMask, &error)
            && writeDetails<QContactOrganization>(contactId, contact, m_removeOrganization, definitionMask, &error)
            && writeDetails<QContactPhoneNumber>(contactId, contact, m_removePhoneNumber, definitionMask, &error)
            && writeDetails<QContactPresence>(contactId, contact, m_removePresence, definitionMask, &error)
            && writeDetails<QContactRingtone>(contactId, contact, m_removeRingtone, definitionMask, &error)
            && writeDetails<QContactSyncTarget>(contactId, contact, m_removeSyncTarget, definitionMask, &error)
            && writeDetails<QContactTag>(contactId, contact, m_removeTag, definitionMask, &error)
            && writeDetails<QContactUrl>(contactId, contact, m_removeUrl, definitionMask, &error)
            && writeDetails<QContactTpMetadata>(contactId, contact, m_removeTpMetadata, definitionMask, &error)) {
        return QContactManager::NoError;
    }
    return error;
}

void ContactWriter::bindContactDetails(const QContact &contact, QSqlQuery &query)
{
    query.bindValue(0, contact.displayLabel());

    QContactName name = contact.detail<QContactName>();
    query.bindValue(1, name.variantValue(QContactName::FieldFirstName));
    query.bindValue(2, name.variantValue(QContactName::FieldLastName));
    query.bindValue(3, name.variantValue(QContactName::FieldMiddleName));
    query.bindValue(4, name.variantValue(QContactName::FieldPrefix));
    query.bindValue(5, name.variantValue(QContactName::FieldSuffix));
    query.bindValue(6, name.variantValue(QContactName::FieldCustomLabel));

    QContactTimestamp timestamp = contact.detail<QContactTimestamp>();
    query.bindValue(7, timestamp.variantValue(QContactTimestamp::FieldModificationTimestamp));
    query.bindValue(8, timestamp.variantValue(QContactTimestamp::FieldCreationTimestamp));

    QContactGender gender = contact.detail<QContactGender>();
    query.bindValue(9, gender.variantValue(QContactGender::FieldGender));

    QContactFavorite favorite = contact.detail<QContactFavorite>();
    query.bindValue(10, favorite.isFavorite());
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactAddress &detail)
{
    typedef QContactAddress T;
    m_insertAddress.bindValue(0, contactId);
    m_insertAddress.bindValue(1, detail.variantValue(T::FieldStreet));
    m_insertAddress.bindValue(2, detail.variantValue(T::FieldPostOfficeBox));
    m_insertAddress.bindValue(3, detail.variantValue(T::FieldRegion));
    m_insertAddress.bindValue(4, detail.variantValue(T::FieldLocality));
    m_insertAddress.bindValue(5, detail.variantValue(T::FieldPostcode));
    m_insertAddress.bindValue(6, detail.variantValue(T::FieldCountry));
    return m_insertAddress;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactAnniversary &detail)
{
    typedef QContactAnniversary T;
    m_insertAnniversary.bindValue(0, contactId);
    m_insertAnniversary.bindValue(1, detail.variantValue(T::FieldOriginalDate));
    m_insertAnniversary.bindValue(2, detail.variantValue(T::FieldCalendarId));
    m_insertAnniversary.bindValue(3, detail.variantValue(T::FieldSubType));
    return m_insertAnniversary;
}


QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactAvatar &detail)
{
    typedef QContactAvatar T;
    m_insertAvatar.bindValue(0, contactId);
    m_insertAvatar.bindValue(1, detail.variantValue(T::FieldImageUrl));
    m_insertAvatar.bindValue(2, detail.variantValue(T::FieldVideoUrl));
    return m_insertAvatar;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactBirthday &detail)
{
    typedef QContactBirthday T;
    m_insertBirthday.bindValue(0, contactId);
    m_insertBirthday.bindValue(1, detail.variantValue(T::FieldBirthday));
    m_insertBirthday.bindValue(2, detail.variantValue(T::FieldCalendarId));
    return m_insertBirthday;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactEmailAddress &detail)
{
    typedef QContactEmailAddress T;
    m_insertEmailAddress.bindValue(0, contactId);
    m_insertEmailAddress.bindValue(1, detail.variantValue(T::FieldEmailAddress));
    return m_insertEmailAddress;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactGuid &detail)
{
    typedef QContactGuid T;
    m_insertGuid.bindValue(0, contactId);
    m_insertGuid.bindValue(1, detail.variantValue(T::FieldGuid));
    return m_insertGuid;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactHobby &detail)
{
    typedef QContactHobby T;
    m_insertHobby.bindValue(0, contactId);
    m_insertHobby.bindValue(1, detail.variantValue(T::FieldHobby));
    return m_insertHobby;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactNickname &detail)
{
    typedef QContactNickname T;
    m_insertNickname.bindValue(0, contactId);
    m_insertNickname.bindValue(1, detail.variantValue(T::FieldNickname));
    return m_insertNickname;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactNote &detail)
{
    typedef QContactNote T;
    m_insertNote.bindValue(0, contactId);
    m_insertNote.bindValue(1, detail.variantValue(T::FieldNote));
    return m_insertNote;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactOnlineAccount &detail)
{
    typedef QContactOnlineAccount T;
    m_insertOnlineAccount.bindValue(0, contactId);
    m_insertOnlineAccount.bindValue(1, detail.variantValue(T::FieldAccountUri));
    m_insertOnlineAccount.bindValue(2, detail.variantValue(T::FieldProtocol));
    m_insertOnlineAccount.bindValue(3, detail.variantValue(T::FieldServiceProvider));
    m_insertOnlineAccount.bindValue(4, detail.variantValue(T::FieldCapabilities));
    m_insertOnlineAccount.bindValue(5, detail.subTypes().join(QLatin1String(";")));
    m_insertOnlineAccount.bindValue(6, detail.variantValue("AccountPath"));
    m_insertOnlineAccount.bindValue(7, detail.variantValue("AccountIconPath"));
    m_insertOnlineAccount.bindValue(8, detail.variantValue("Enabled"));
    return m_insertOnlineAccount;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactOrganization &detail)
{
    typedef QContactOrganization T;
    m_insertOrganization.bindValue(0, contactId);
    m_insertOrganization.bindValue(1, detail.variantValue(T::FieldName));
    m_insertOrganization.bindValue(2, detail.variantValue(T::FieldRole));
    m_insertOrganization.bindValue(3, detail.variantValue(T::FieldTitle));
    m_insertOrganization.bindValue(4, detail.variantValue(T::FieldLocation));
    m_insertOrganization.bindValue(5, detail.department().join(QLatin1String(";")));
    m_insertOrganization.bindValue(6, detail.variantValue(T::FieldLogoUrl));
    return m_insertOrganization;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactPhoneNumber &detail)
{
    typedef QContactPhoneNumber T;
    m_insertPhoneNumber.bindValue(0, contactId);
    m_insertPhoneNumber.bindValue(1, detail.variantValue(T::FieldNumber));
    m_insertPhoneNumber.bindValue(2, detail.subTypes().join(QLatin1String(";")));
    m_insertPhoneNumber.bindValue(3, detail.variantValue("NormalizedNumber"));
    return m_insertPhoneNumber;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactPresence &detail)
{
    typedef QContactPresence T;
    m_insertPresence.bindValue(0, contactId);
    m_insertPresence.bindValue(1, detail.variantValue(T::FieldPresenceState));
    m_insertPresence.bindValue(2, detail.variantValue(T::FieldTimestamp));
    m_insertPresence.bindValue(3, detail.variantValue(T::FieldNickname));
    m_insertPresence.bindValue(4, detail.variantValue(T::FieldCustomMessage));
    return m_insertPresence;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactRingtone &detail)
{
    typedef QContactRingtone T;
    m_insertRingtone.bindValue(0, contactId);
    m_insertRingtone.bindValue(1, detail.variantValue(T::FieldAudioRingtoneUrl));
    m_insertRingtone.bindValue(2, detail.variantValue(T::FieldVideoRingtoneUrl));
    return m_insertRingtone;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactSyncTarget &detail)
{
    typedef QContactSyncTarget T;
    m_insertSyncTarget.bindValue(0, contactId);
    m_insertSyncTarget.bindValue(1, detail.variantValue(T::FieldSyncTarget));
    return m_insertSyncTarget;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactTag &detail)
{
    typedef QContactTag T;
    m_insertTag.bindValue(0, contactId);
    m_insertTag.bindValue(1, detail.variantValue(T::FieldTag));
    return m_insertTag;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactUrl &detail)
{
    typedef QContactUrl T;
    m_insertUrl.bindValue(0, contactId);
    m_insertUrl.bindValue(1, detail.variantValue(T::FieldUrl));
    m_insertUrl.bindValue(2, detail.variantValue(T::FieldSubType));
    return m_insertUrl;
}

QSqlQuery &ContactWriter::bindDetail(QContactLocalId contactId, const QContactTpMetadata &detail)
{
    m_insertTpMetadata.bindValue(0, contactId);
    m_insertTpMetadata.bindValue(1, detail.variantValue("ContactId"));
    m_insertTpMetadata.bindValue(2, detail.variantValue("AccountId"));
    m_insertTpMetadata.bindValue(3, detail.variantValue("AccountEnabled"));
    return m_insertTpMetadata;
}
