from skbuild import setup

setup(
    name="PyKOpeningHours",
    version="1.4.0",
    description="Validator for OSM opening_hours expressions",
    author='David Faure',
    license="AGPL",
    package_data={'PyKOpeningHours': ['py.typed', 'PyKOpeningHours.pyi']},
    packages=['PyKOpeningHours'],
    cmake_args=['-DVALIDATOR_ONLY=TRUE', '-DBUILD_SHARED_LIBS=FALSE', '-DCMAKE_BUILD_TYPE=Release']
)
