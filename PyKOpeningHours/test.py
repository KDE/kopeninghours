#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2020 David Faure <faure@kde.org>
# SPDX-License-Identifier: LGPL-2.0-or-later

from PyKOpeningHours import PyKOpeningHours

parser = PyKOpeningHours.OpeningHours()
parser.setExpression('Friday 11:00 am - 11:00 pm')
if parser.error() != PyKOpeningHours.Error.NoError:
    print("ERROR parsing expression:" + str(parser.error()))
else:
    print(parser.normalizedExpression())
