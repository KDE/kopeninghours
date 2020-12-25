# Without this, any build libraries automatically have names "lib{x}.so"
set(CMAKE_SHARED_MODULE_PREFIX "")

# Silence warning "zero as null pointer constant" due to the BOOST_PYTHON_MODULE macro
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0.0")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-zero-as-null-pointer-constant" )
   endif()
endif()

add_library(PyKOpeningHours MODULE
    pykopeninghours.cpp
    python_qt_wrappers.cpp
    test.py # just so it appears in QtCreator
)
kde_target_enable_exceptions(PyKOpeningHours PRIVATE)

target_compile_definitions(PyKOpeningHours PRIVATE KOPENINGHOURS_VALIDATOR_ONLY)

# Set up the libraries and header search paths for this target
target_link_libraries(PyKOpeningHours PUBLIC ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} KOpeningHours)
target_include_directories(PyKOpeningHours PRIVATE ${PYTHON_INCLUDE_DIRS})

set(_install_dir lib/python${PYTHONLIBS_VERSION_MAJOR}.${PYTHONLIBS_VERSION_MINOR}/site-packages)

install(TARGETS PyKOpeningHours DESTINATION ${_install_dir}/PyKOpeningHours)
install(FILES PyKOpeningHours.pyi __init__.py DESTINATION ${_install_dir}/PyKOpeningHours)

