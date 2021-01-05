/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "intervalmodel.h"

#include <KOpeningHours/Display>
#include <KOpeningHours/Interval>

#include <QLocale>

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

static void clipIntervalEnd(Interval &i, const QDateTime &endDt)
{
    i.setEnd(i.hasOpenEnd() ? endDt : std::min(i.end(), endDt));
    if (i.end() == endDt && i.hasOpenEndTime()) { // spans in the next day, so this sub-interval isn't open-ended
        i.setOpenEndTime(false);
    }
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
        clipIntervalEnd(i, QDateTime(dt, {0, 0}));

        dayData.intervals.push_back(i);
        while (i.isValid() && i.end() < QDateTime(dt, {0, 0})) {
            i = oh.nextInterval(i);
            clipIntervalEnd(i, QDateTime(dt, {0, 0}));
            dayData.intervals.push_back(i);
        }

        // fill open end time estimates
        for (auto it = dayData.intervals.begin(); it != std::prev(dayData.intervals.end()); ++it) {
            auto nextStartDt = QDateTime(dt, {0, 0});
            if (!(*it).hasOpenEndTime() || (*it).state() == Interval::Closed) {
                continue;
            }
            for (auto nextIt = std::next(it); nextIt != dayData.intervals.end(); ++nextIt) {
                if ((*nextIt).state() != Interval::Closed) {
                    nextStartDt = (*nextIt).begin();
                    break;
                }
            }

            auto estimatedEnd = nextStartDt == QDateTime(dt, {0, 0}) ? nextStartDt : (*it).end().addSecs((*it).end().secsTo(nextStartDt) / 2);
            estimatedEnd = std::min(estimatedEnd, (*it).end().addSecs(4 * 3600));
            (*it).setEstimatedEnd(estimatedEnd);
            (*std::next(it)).setBegin(estimatedEnd);
        }
    }
}

IntervalModel::IntervalModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new IntervalModelPrivate)
{
}

IntervalModel::~IntervalModel() = default;

OpeningHours IntervalModel::openingHours() const
{
    return d->oh;
}

void IntervalModel::setOpeningHours(const OpeningHours &oh)
{
    d->oh = oh;
    emit openingHoursChanged();

    beginResetModel();
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

    d->beginDt = beginDate;
    emit beginDateChanged();

    beginResetModel();
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

    d->endDt = endDate;
    emit endDateChanged();

    beginResetModel();
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
        case ShortDayNameRole:
             return QLocale().standaloneDayName(d->m_intervals[index.row()].day.dayOfWeek(), QLocale::ShortFormat);
        case IsTodayRole:
            return d->m_intervals[index.row()].day == QDate::currentDate();
    }

    return {};
}

QHash<int, QByteArray> IntervalModel::roleNames() const
{
    auto n = QAbstractListModel::roleNames();
    n.insert(IntervalsRole, "intervals");
    n.insert(DateRole, "date");
    n.insert(DayBeginTimeRole, "dayBegin");
    n.insert(ShortDayNameRole, "shortDayName");
    n.insert(IsTodayRole, "isToday");
    return n;
}

QDate IntervalModel::beginOfWeek(const QDateTime& dt) const
{
    auto d = dt.date();
    const auto start = QLocale().firstDayOfWeek();
    if (start < d.dayOfWeek()) {
        d = d.addDays(start - d.dayOfWeek());
    } else {
        d = d.addDays(start - d.dayOfWeek() - 7);
    }
    return d;
}

QString IntervalModel::formatTimeColumnHeader(int hour, int minute) const
{
    return QLocale().toString(QTime(hour, minute), QLocale::NarrowFormat);
}

QString IntervalModel::currentState() const
{
    return Display::currentState(d->oh);
}

#include "moc_intervalmodel.cpp"
