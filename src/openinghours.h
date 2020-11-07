/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOPENINGHOURS_OPENINGHOURS_H
#define KOPENINGHOURS_OPENINGHOURS_H

#include "kopeninghours_export.h"

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

class QByteArray;
class QDateTime;
class QString;

/** OSM opening hours parsing and evaluation. */
namespace KOpeningHours {

class Interval;
class OpeningHoursPrivate;

/** An OSM opening hours specification.
 *  This is the main entry point into this library, providing both a way to parse opening hours expressions
 *  and to evaluate them.
 *  @see https://wiki.openstreetmap.org/wiki/Key:opening_hours
 */
class KOPENINGHOURS_EXPORT OpeningHours
{
    Q_GADGET
public:
    explicit OpeningHours();
    /** Parse OSM opening hours expression @p openingHours. */
    explicit OpeningHours(const QByteArray &openingHours);
    OpeningHours(const OpeningHours&);
    OpeningHours(OpeningHours&&);
    ~OpeningHours();

    OpeningHours& operator=(const OpeningHours&);
    OpeningHours& operator=(OpeningHours&&);

    // TODO
    //setRegion(ISO 3166-2)
    //setCountry(ISO 3166-1)
    //setLocation(lat, lon)

    /** Error codes. */
    enum Error {
        NoError,
        SyntaxError, ///< syntax error in the opening hours expression
        MissingRegion, ///< expression refers to public or school holidays, but that information is not available
        MissingLocation, ///< evaluation requires location information and those aren't set
    };
    Q_ENUM(Error)

    /** Error status of this expression. */
    Error error() const;
    // TODO
    // errorString() const

    // TODO human readable, translated description of the opening hours
    // QString description() const

    // TODO
    // interval(QDateTime) const
    // nextInterval(Interval) const

    // TODO point-in-time mode iteration API?
    // nextPointInTime(QDateTime) const

private:
    QExplicitlySharedDataPointer<OpeningHoursPrivate> d;
};

}

#endif // KOPENINGHOURS_OPENINGHOURS_H
