/*
    SPDX-FileCopyrightText: 2020 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <functional>
#include <QByteArray>

// Input: 1,2,4,5,6,9
// Output: 1-2,4-6,9
class ConsecutiveAccumulator
{
public:
    // The std::function is usually QByteArray::number
    // but it could be also 1->Mo, 2->Tu etc. if we need that one day.
    explicit ConsecutiveAccumulator(std::function<QByteArray(int)> f)
        : func(f) {}

    void add(int value)
    {
        if (!first && value == cur+1) {
            ++cur;
        } else {
            flush();
            cur = value;
            start = value;
            first = false;
        }
    }
    QByteArray result()
    {
        // Finalize
        flush();
        return expr.isEmpty() ? expr : expr.chopped(1);
    }

private:
    void flush()
    {
        if (!first) {
            if (start < cur) {
                expr += func(start) + '-' + func(cur) + ',';
            } else {
                expr += func(cur) + ',';
            }
        }
    }

    bool first = true;
    int cur = 0;
    int start = 0;
    QByteArray expr;
    std::function<QByteArray(int)> func;
};
