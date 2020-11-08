# SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_test(parsertest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
ecm_add_test(evaluatetest.cpp LINK_LIBRARIES Qt5::Test KOpeningHours)
