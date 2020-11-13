/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_INTERVALMODEL_H
#define KOPENINGHOURS_INTERVALMODEL_H

#include "kopeninghours_export.h"

#include <KOpeningHours/OpeningHours>

#include <QAbstractListModel>
#include <QDate>

#include <memory>

namespace KOpeningHours {

class IntervalModelPrivate;

/** Model for showing opening intervals per day. */
class KOPENINGHOURS_EXPORT IntervalModel : public QAbstractListModel
{
    Q_OBJECT
    /** The opening hours expression shown in this model. */
    Q_PROPERTY(KOpeningHours::OpeningHours openingHours READ openingHours WRITE setOpeningHours NOTIFY setupChanged)
    /** Begin of the date range to show in this model. */
    Q_PROPERTY(QDate beginDate READ beginDate WRITE setBeginDate NOTIFY setupChanged)
    /** End of the date range to show in this model. */
    Q_PROPERTY(QDate endDate READ endDate WRITE setEndDate NOTIFY setupChanged)

public:
    explicit IntervalModel(QObject *parent = nullptr);
    ~IntervalModel();

    OpeningHours openingHours() const;
    void setOpeningHours(const OpeningHours &oh);

    QDate beginDate() const;
    void setBeginDate(const QDate &beginDate);
    QDate endDate() const;
    void setEndDate(const QDate &endDate);

    enum Roles {
        IntervalsRole = Qt::UserRole,
        DateRole,
        DayBeginTimeRole,
    };

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void setupChanged();

private:
    std::unique_ptr<IntervalModelPrivate> d;
};

}

#endif // KOPENINGHOURS_INTERVALMODEL_H
