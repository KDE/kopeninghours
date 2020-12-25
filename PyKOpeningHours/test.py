#!/usr/bin/env python3

from PyKOpeningHours import PyKOpeningHours

parser = PyKOpeningHours.OpeningHours()
parser.setExpression('Friday 11:00 am - 11:00 pm')
if parser.error() != PyKOpeningHours.Error.NoError:
    print("ERROR parsing expression:" + str(parser.error()))
else:
    print(parser.normalizedExpression())
