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
    Q_PROPERTY(KOpeningHours::OpeningHours openingHours READ openingHours WRITE setOpeningHours NOTIFY openingHoursChanged)
    /** Begin of the date range to show in this model. */
    Q_PROPERTY(QDate beginDate READ beginDate WRITE setBeginDate NOTIFY beginDateChanged)
    /** End of the date range to show in this model. */
    Q_PROPERTY(QDate endDate READ endDate WRITE setEndDate NOTIFY endDateChanged)

    /** Description of the current status as a translated human-readable string.
     *  See Display::currentState.
     */
    Q_PROPERTY(QString currentState READ currentState NOTIFY openingHoursChanged)

public:
    explicit IntervalModel(QObject *parent = nullptr);
    ~IntervalModel() override;

    OpeningHours openingHours() const;
    void setOpeningHours(const OpeningHours &oh);

    QDate beginDate() const;
    void setBeginDate(QDate beginDate);
    QDate endDate() const;
    void setEndDate(QDate endDate);

    enum Roles {
        IntervalsRole = Qt::UserRole, ///< All intervals in the current row.
        DateRole, ///< The date represented by the current row.
        DayBeginTimeRole, ///< Same as @c DateRole, but as a date/time object.
        ShortDayNameRole, ///< Localized short day name for the current row.
        IsTodayRole, ///< @c true if the row represents the current day.
    };

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /** Returns the day the week containing @p dt begins, based on the current locale.
     *  This is useful to align the content of this model to a week.
     */
    Q_INVOKABLE QDate beginOfWeek(const QDateTime &dt) const;

    /** Localized formatting for time column headers. */
    Q_INVOKABLE QString formatTimeColumnHeader(int hour, int minute) const;

Q_SIGNALS:
    void openingHoursChanged();
    void beginDateChanged();
    void endDateChanged();

private:
    QString currentState() const;
    std::unique_ptr<IntervalModelPrivate> d;
};

}

#endif // KOPENINGHOURS_INTERVALMODEL_H
