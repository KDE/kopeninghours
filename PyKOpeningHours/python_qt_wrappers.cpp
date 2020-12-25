/*
    SPDX-FileCopyrightText: 2020 David Faure <faure@kde.org>

    SPDX-License-Identifier: AGPL-3.0-or-later
*/
#include <boost/python/converter/registry.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/object.hpp>
#include <QByteArray>

using namespace boost::python;

struct QByteArray_to_python_str
{
  static PyObject* convert(QByteArray const& s)
  {
      return incref(object(s.constData()).ptr());
  }
};

struct QByteArray_from_python_str
{
    QByteArray_from_python_str()
    {
        converter::registry::push_back(&convertible, &construct, type_id<QByteArray>());
    }

    // Determine if objPtr can be converted in a QByteArray
    static void* convertible(PyObject* obj_ptr)
    {
        if (!PyUnicode_Check(obj_ptr)) return nullptr;
        return obj_ptr;
    }

    // Convert objPtr into a QByteArray
    static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data)
    {
        // Extract the character data from the python string
        handle<> rawBytesHandle{PyUnicode_AsUTF8String(obj_ptr)};
        Q_ASSERT(rawBytesHandle);

        // Grab pointer to memory into which to construct the new QByteArray
        void* storage = ((converter::rvalue_from_python_storage<QByteArray>*) data)->storage.bytes;

        // in-place construct the new QByteArray using the character data extracted from the python object
        new (storage) QByteArray{PyBytes_AsString(rawBytesHandle.get()), int(PyBytes_Size(rawBytesHandle.get()))};
        data->convertible = storage;
    }
};

void register_qt_wrappers()
{
    to_python_converter<
            QByteArray,
            QByteArray_to_python_str>();
    QByteArray_from_python_str();
}
