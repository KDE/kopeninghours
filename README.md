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

The parser and validator is fairly complete. However the evaluation feature has the following limitations.

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
* comment preservation on intersecting close rules
* wide range selector placeholder comments
* open-ended time selectors
* school holiday selectors
* open ended month ranges
* weekday-based offsets

## Other Formats

Opening hours in the schema.org format can be read as well, via KOpeningHours::OpeningHours::fromJsonLd().
