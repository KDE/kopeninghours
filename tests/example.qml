/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.14
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.0 as Kirigami
import org.kde.kholidays 1.0 as KHolidays
import org.kde.kopeninghours 1.0

Kirigami.ApplicationWindow {
    title: "Opening Hours Demo"
    reachableModeEnabled: false

    width: 640
    height: 480

    pageStack.initialPage: mainPage

    Component {
        id: openingHoursDelegate
        Item {
            id: delegateRoot
            property var dayData: model
            implicitHeight: row.implicitHeight
            Row {
                id: row
                QQC2.Label {
                    text: dayData.display
                    width: delegateRoot.ListView.view.labelWidth + Kirigami.Units.smallSpacing
                    Component.onCompleted: delegateRoot.ListView.view.labelWidth = Math.max(delegateRoot.ListView.view.labelWidth, implicitWidth)
                }
                Repeater {
                    model: dayData.intervals
                    Rectangle {
                        id: intervalBox
                        property var interval: modelData
                        color: {
                            switch (interval.state) {
                                case Interval.Open: return Kirigami.Theme.positiveBackgroundColor;
                                case Interval.Closed: return Kirigami.Theme.negativeBackgroundColor;
                                case Interval.Unknown: return Kirigami.Theme.neutralBackgroundColor;
                            }
                            return "transparent";
                        }
                        width: {
                            var ratio = (interval.end - interval.begin) / (24 * 60 * 60 * 1000);
                            return ratio * (delegateRoot.ListView.view.width - delegateRoot.ListView.view.labelWidth - Kirigami.Units.smallSpacing);
                        }
                        height: Kirigami.Units.gridUnit

                        QQC2.Label {
                            id: commentLabel
                            text: interval.comment
                            anchors.centerIn: parent
                            visible: commentLabel.implicitWidth < intervalBox.width
                            font.italic: true
                        }
                    }
                }
            }
            Rectangle {
                id: nowMarker
                property double position: (Date.now() - dayData.dayBegin) / (24 * 60 * 60 * 1000)
                visible: position >= 0.0 && position < 1.0
                color: Kirigami.Theme.textColor
                width: 2
                height: Kirigami.Units.gridUnit
                x: position * (delegateRoot.ListView.view.width - delegateRoot.ListView.view.labelWidth - Kirigami.Units.smallSpacing)
                    + delegateRoot.ListView.view.labelWidth + Kirigami.Units.smallSpacing
            }
        }
    }

    Component {
        id: mainPage
        Kirigami.Page {
            id: page
            title: "OSM Opening Hours Expression Evaluator Demo"
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
                    text: "Mo-Fr 08:00-12:00,13:00-17:30; Sa 08:00-12:00; Su unknown \"on appointment\""
                    onTextChanged: {
                        var oh = OpeningHoursParser.parse(text);
                        oh.setLocation(latitude.text, longitude.text);
                        oh.region = region.currentText;
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

                KHolidays.HolidayRegionsModel { id: regionModel }
                QQC2.ComboBox {
                    id: region
                    model: regionModel
                    textRole: "region"
                    onCurrentIndexChanged: {
                        page.oh.region = currentText;
                        evaluateCurrentState();
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

                IntervalModel {
                    id: intervalModel
                    openingHours: page.oh
                }

                ListView {
                    id: intervalView
                    model: intervalModel
                    delegate: openingHoursDelegate
                    property int labelWidth: 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Kirigami.Units.smallSpacing
                    clip: true
                    header: Row {
                        id: intervalHeader
                        property int itemWidth: (intervalHeader.ListView.view.width -  intervalHeader.ListView.view.labelWidth - Kirigami.Units.smallSpacing) / 8
                        x: intervalHeader.ListView.view.labelWidth + Kirigami.Units.smallSpacing + intervalHeader.itemWidth/2
                        Repeater {
                            model: [3, 6, 9, 12, 15, 18, 21]
                            QQC2.Label {
                                text: modelData + ":00" // TODO
                                width: intervalHeader.itemWidth
                                horizontalAlignment: Qt.AlignHCenter
                            }
                        }
                    }
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

