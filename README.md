# KOpeningHours

A library for parsing and evaluating OSM opening hours expressions.

## Introduction

OSM opening hours expressions are used to describe when a feature is open/available or closed. This format
is not only used in OpenStreetMap itself, but in various other data sources or APIs needing such a description
as well.

See:
* https://wiki.openstreetmap.org/wiki/Key:opening_hours
* https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification

## Supported Features

So far only the time interval mode is supported, not the point in time mode.

Supported opening hours features are:
* rule sequences
* fallback rules
* all rule modifiers and rule comments
* time span, weekday, week, month, month day and year selectors
* solar position based variable time events
* public holiday selectors (based on KF5::Holidays)
* variable date selectors

Still missing features:
* additional rules (treated as normal rules atm)
* comment preservation on intersecting close rules
* wide range selector placeholder comments
* open-ended time selectors
* offsets to variable time based events
* AND/OR distinction in weekday selectors
* school holiday selectors
* n-th entry weekday selectors
* holiday offset selectors
* week intervals
* open ended month ranges

## Other Formats

Opening hours in the schema.org format can be read as well, via KOpeningHours::OpeningHours::fromJsonLd().
