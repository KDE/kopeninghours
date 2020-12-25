/*
    SPDX-FileCopyrightText: 2020 David Faure <faure@kde.org>

    SPDX-License-Identifier: AGPL-3.0-or-later
*/
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/enum.hpp>
#include "python_qt_wrappers.h"
#include <KOpeningHours/OpeningHours>

using namespace boost::python;
using namespace KOpeningHours;

BOOST_PYTHON_MODULE(PyKOpeningHours)
{
    register_qt_wrappers();

    enum_<OpeningHours::Mode>("Mode")
            .value("IntervalMode", OpeningHours::IntervalMode)
            .value("PointInTimeMode", OpeningHours::PointInTimeMode);
    class_<OpeningHours::Modes>("Modes", init<OpeningHours::Mode>());

    // select the right overload of setExpression
    using SetExpressionFunc = void(OpeningHours::*)(const QByteArray&,OpeningHours::Modes);
    class_<OpeningHours>("OpeningHours", init<>())
        .def("setExpression", static_cast<SetExpressionFunc>(&OpeningHours::setExpression),
             (arg("expression"), arg("modes")=OpeningHours::Modes{OpeningHours::IntervalMode}))
        .def("error", &OpeningHours::error)
        .def("normalizedExpression", &OpeningHours::normalizedExpression);

    enum_<OpeningHours::Error>("Error")
            .value("Null", OpeningHours::Null) ///< no expression is set
            .value("NoError", OpeningHours::NoError) ///< there is no error, the expression is valid and can be used
            .value("SyntaxError", OpeningHours::SyntaxError) ///< syntax error in the opening hours expression
            .value("MissingRegion", OpeningHours::MissingRegion) ///< expression refers to public or school holidays, but that information is not available
            .value("MissingLocation", OpeningHours::MissingLocation) ///< evaluation requires location information and those aren't set
            .value("IncompatibleMode", OpeningHours::IncompatibleMode) ///< expression mode doesn't match the expected mode
            .value("UnsupportedFeature", OpeningHours::UnsupportedFeature) ///< expression uses a feature that isn't implemented/supported (yet)
            .value("EvaluationError", OpeningHours::EvaluationError) ///< runtime error during evaluating the expression
            ;
}
