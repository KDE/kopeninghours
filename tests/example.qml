/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.14
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.0 as Kirigami
import org.kde.kopeninghours 1.0

Kirigami.ApplicationWindow {
    title: "Opening Hours Demo"
    reachableModeEnabled: false

    width: 640
    height: 240

    pageStack.initialPage: mainPage

    Component {
        id: mainPage
        Kirigami.Page {
            id: page
            title: "OSM Opening Hours Expression Evaluator"
            property var oh: {
                var v = OpeningHoursParser.parse(expression.text);
                v.setLocation(52.5, 13.0);
                return v;
            }

            function evaluateCurrentState() {
                if (oh.error != OpeningHours.NoError) {
                    currentState.text = "";
                    return;
                }

                var s = "";
                var i = oh.interval(new Date());
                switch (i.state) {
                    case Interval.Open:
                        s += "Currently open";
                        break;
                    case Interval.Closed:
                        s += "Currently closed";
                        break;
                    case Interval.Unknown:
                        s += "Currently unknown";
                        break;
                    case Interval.Invalid:
                        s += "evaluation error";
                }

                if (i.comment.length > 0) {
                    s += " (" + i.comment + ")";
                }

                if (i.end) {
                    s += ", for " + Math.round((i.end - Date.now())/60000) + " more minutes.";
                }

                currentState.text = s;
            }

            ColumnLayout {
                anchors.fill: parent

                QQC2.TextField {
                    id: expression
                    Layout.fillWidth: true
                    text: "Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00"
                    onTextChanged: {
                        var oh = OpeningHoursParser.parse(text);
                        oh.setLocation(latitude.text, longitude.text);
                        page.oh = oh;
                        evaluateCurrentState();
                    }
                }

                RowLayout {
                    QQC2.Label { text: "Latitude:" }
                    QQC2.TextField {
                        id: latitude
                        text: "52.5"
                        onTextChanged: {
                            oh.latitude = text;
                            evaluateCurrentState();
                        }
                    }
                    QQC2.Label { text: "Longitude:" }
                    QQC2.TextField {
                        id: longitude
                        text: "13.0"
                        onTextChanged: {
                            oh.longitude = text;
                            evaluateCurrentState();
                        }
                    }
                }

                QQC2.Label {
                    text: {
                        switch (oh.error) {
                            case OpeningHours.NoError:
                                return "Expression is valid.";
                            case OpeningHours.SyntaxError:
                                return "Syntax error!";
                            case OpeningHours.MissingRegion:
                                return "Expression needs to know the ISO 3166-2 region it is evaluated for.";
                            case OpeningHours.MissingLocation:
                                return "Expression needs to know the geo coordinates of the location it is evaluated for.";
                            case OpeningHours.UnsupportedFeature:
                                return "Expression uses a feature that is not supported/implemented yet.";
                        }
                    }
                }

                QQC2.Label {
                    id: currentState
                }

                Timer {
                    interval: 60000
                    repeat: true
                    running: true
                    onTriggered: evaluateCurrentState()
                }

                Component.onCompleted: evaluateCurrentState()
            }
        }
    }
}

