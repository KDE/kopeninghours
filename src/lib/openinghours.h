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
    Q_PROPERTY(Error error READ error)
    Q_PROPERTY(float latitude READ latitude WRITE setLatitude)
    Q_PROPERTY(float longitude READ longitude WRITE setLongitude)
    Q_PROPERTY(QString region READ region WRITE setRegion)
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
    //setCountry(ISO 3166-1)

    /** Geographic coordinate at which this expression should be evaluated.
     *  This is needed for expressions containing location-based variable time references,
     *  such as "sunset". If the expression requires a location, error() returns @c MissingLocation
     *  if no location has been specified.
     */
    Q_INVOKABLE void setLocation(float latitude, float longitude);

    float latitude() const;
    void setLatitude(float latitude);
    float longitude() const;
    void setLongitude(float longitude);

    /** ISO 3166-2 Region in which this expression should be evaluated.
     *  This is needed for expressions containing public holiday references ("PH").
     *  If the expression references a public holiday, error() returns @c MissingRegion
     *  if no region has been specified, or if no holiday data is available for the specified region.
     */
    void setRegion(const QString &region);
    QString region() const;

    /** Error codes. */
    enum Error {
        NoError,
        SyntaxError, ///< syntax error in the opening hours expression
        MissingRegion, ///< expression refers to public or school holidays, but that information is not available
        MissingLocation, ///< evaluation requires location information and those aren't set
        UnsupportedFeature, ///< expression uses a feature that isn't implemented/supported (yet)
    };
    Q_ENUM(Error)

    /** Error status of this expression. */
    Error error() const;
    // TODO
    // errorString() const

    // TODO human readable, translated description of the opening hours
    // QString description() const

    /** Returns the interval containing @p dt. */
    Q_INVOKABLE KOpeningHours::Interval interval(const QDateTime &dt) const;
    /** Returns the interval immediately following @p interval. */
    Q_INVOKABLE Interval nextInterval(const KOpeningHours::Interval &interval) const;

    // TODO point-in-time mode iteration API?
    // nextPointInTime(QDateTime) const

private:
    QExplicitlySharedDataPointer<OpeningHoursPrivate> d;
};

}

Q_DECLARE_METATYPE(KOpeningHours::OpeningHours)

#endif // KOPENINGHOURS_OPENINGHOURS_H
