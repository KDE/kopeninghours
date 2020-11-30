/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "intervalmodel.h"

#include <KOpeningHours/Interval>

#include <vector>

using namespace KOpeningHours;

namespace KOpeningHours {

struct DayData {
    QDate day;
    std::vector<Interval> intervals;
};

class IntervalModelPrivate {
public:
    void repopulateModel();

    OpeningHours oh;
    std::vector<DayData> m_intervals;
    QDate beginDt = QDate::currentDate();
    QDate endDt = QDate::currentDate().addDays(7);
};
}

void IntervalModelPrivate::repopulateModel()
{
    m_intervals.clear();
    if (endDt < beginDt || oh.error() != OpeningHours::NoError) {
        return;
    }

    QDate dt = beginDt;
    m_intervals.resize(beginDt.daysTo(endDt));
    for (auto &dayData : m_intervals) {
        dayData.day = dt;
        auto i = oh.interval(QDateTime(dt, {0, 0}));

        // clip intervals to the current day, makes displaying much easier
        i.setBegin(i.hasOpenBegin() ? QDateTime(dt, {0, 0}) : std::max(i.begin(), QDateTime(dt, {0, 0})));
        dt = dt.addDays(1);
        i.setEnd(i.hasOpenEnd() ? QDateTime(dt, {0, 0}) : std::min(i.end(), QDateTime(dt, {0, 0})));

        dayData.intervals.push_back(i);
        while (i.isValid() && i.end() < QDateTime(dt, {0, 0})) {
            i = oh.nextInterval(i);
            i.setEnd(i.hasOpenEnd() ? QDateTime(dt, {0, 0}) : std::min(i.end(), QDateTime(dt, {0, 0})));
            dayData.intervals.push_back(i);
        }

    }
}

IntervalModel::IntervalModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new IntervalModelPrivate)
{
    connect(this, &QAbstractItemModel::modelReset, this, &IntervalModel::setupChanged);
}

IntervalModel::~IntervalModel() = default;

OpeningHours IntervalModel::openingHours() const
{
    return d->oh;
}

void IntervalModel::setOpeningHours(const OpeningHours &oh)
{
    beginResetModel();
    d->oh = oh;
    d->repopulateModel();
    endResetModel();
}

QDate IntervalModel::beginDate() const
{
    return d->beginDt;
}

void IntervalModel::setBeginDate(QDate beginDate)
{
    if (d->beginDt == beginDate) {
        return;
    }

    beginResetModel();
    d->beginDt = beginDate;
    d->repopulateModel();
    endResetModel();
}

QDate IntervalModel::endDate() const
{
    return d->endDt;
}


void IntervalModel::setEndDate(QDate endDate)
{
    if (d->endDt == endDate) {
        return;
    }

    beginResetModel();
    d->endDt = endDate;
    d->repopulateModel();
    endResetModel();
}

int IntervalModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return d->m_intervals.size();
}

QVariant IntervalModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(d->m_intervals[index.row()].day, QLocale::ShortFormat);
        case IntervalsRole:
            return QVariant::fromValue(d->m_intervals[index.row()].intervals);
        case DateRole:
            return d->m_intervals[index.row()].day;
        case DayBeginTimeRole:
            return QDateTime(d->m_intervals[index.row()].day, {0, 0});
    }

    return {};
}

QHash<int, QByteArray> IntervalModel::roleNames() const
{
    auto n = QAbstractListModel::roleNames();
    n.insert(IntervalsRole, "intervals");
    n.insert(DateRole, "date");
    n.insert(DayBeginTimeRole, "dayBegin");
    return n;
}

#include "moc_intervalmodel.cpp"
