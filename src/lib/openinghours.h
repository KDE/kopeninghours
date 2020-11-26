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
class QJsonObject;
class QString;
class QTimeZone;

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
    Q_PROPERTY(QString timezone READ timeZoneId WRITE setTimeZoneId)
public:
    /** Evaluation modes for opening hours expressions. */
    enum Mode {
        IntervalMode = 1, ///< Expressions that describe time ranges
        PointInTimeMode = 2, ///< Expressions that describe points in time
    };
    Q_DECLARE_FLAGS(Modes, Mode)
    Q_FLAG(Modes)

    /** Create an empty/invalid instance. */
    explicit OpeningHours();

    /** Parse OSM opening hours expression @p openingHours.
     *  @param modes Specify whether time interval and/or point in time expressions are expected.
     *  If @p openingHours doesn't match @p modes, error() return IncompatibleMode.
     */
    explicit OpeningHours(const QByteArray &openingHours, Modes modes = IntervalMode);

    OpeningHours(const OpeningHours&);
    OpeningHours(OpeningHours&&);
    ~OpeningHours();

    OpeningHours& operator=(const OpeningHours&);
    OpeningHours& operator=(OpeningHours&&);

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

    /** ISO 3166-2 region or ISO 316-1 country in which this expression should be evaluated.
     *  This is needed for expressions containing public holiday references ("PH").
     *  If the expression references a public holiday, error() returns @c MissingRegion
     *  if no region has been specified, or if no holiday data is available for the specified region.
     */
    void setRegion(QStringView region);
    QString region() const;

    /** Timezone in which this expression should be evaluated.
     *  If not specified, this assumes local time.
     */
    void setTimeZone(const QTimeZone &tz);
    QTimeZone timeZone() const;

    /** Error codes. */
    enum Error {
        Null, ///< no expression is set
        NoError, ///< there is no error, the expression is valid and can be used
        SyntaxError, ///< syntax error in the opening hours expression
        MissingRegion, ///< expression refers to public or school holidays, but that information is not available
        MissingLocation, ///< evaluation requires location information and those aren't set
        IncompatibleMode, ///< expression mode doesn't match the expected mode
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
    Q_INVOKABLE KOpeningHours::Interval nextInterval(const KOpeningHours::Interval &interval) const;

    // TODO point-in-time mode iteration API?
    // nextPointInTime(QDateTime) const

    /** Convert opening hours in schema.org JSON-LD format.
     *  This supports the following formats:
     *  - https://schema.org/openingHours
     *  - https://schema.org/openingHoursSpecification
     *  - https://schema.org/specialOpeningHoursSpecification
     */
    static OpeningHours fromJsonLd(const QJsonObject &obj);

private:
    // for QML bindings
    QString timeZoneId() const;
    void setTimeZoneId(const QString &tzId);

    QExplicitlySharedDataPointer<OpeningHoursPrivate> d;
};

}

Q_DECLARE_METATYPE(KOpeningHours::OpeningHours)

#endif // KOPENINGHOURS_OPENINGHOURS_H
