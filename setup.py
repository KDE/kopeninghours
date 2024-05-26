# SPDX-FileCopyrightText: 2020 David Faure <faure@kde.org>
# SPDX-License-Identifier: LGPL-2.0-or-later

from skbuild import setup

setup(
    name="PyKOpeningHours",
    version="1.6.0",
    description="Validator for OSM opening_hours expressions",
    author='David Faure',
    license="AGPL",
    package_data={'PyKOpeningHours': ['py.typed', 'PyKOpeningHours.pyi']},
    packages=['PyKOpeningHours'],
    cmake_args=['-DVALIDATOR_ONLY=TRUE', '-DBUILD_SHARED_LIBS=FALSE', '-DCMAKE_BUILD_TYPE=Release']
)
