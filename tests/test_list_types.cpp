/*
 *   Copyright 2018  Jos van den Oever <jos@vandenoever.info>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test_list_types_rust.h"
#include <QTest>
#include <QSignalSpy>

class TestRustListTypes : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testStringGetter();
    void testStringSetter();
    void testBool();
    void testInt8();
    void testUint8();
    void testInt16();
    void testUint16();
    void testInt32();
    void testUint32();
    void testInt64();
    void testUint64();
    void testFloat();
    void testDouble();
    void testString();
    void testOptionalString();
    void testByteArray();
    void testOptionalByteArray();
};

template <typename V, typename Set, typename Get>
void testSetter(const V v, Set set, Get get)
{
    // GIVEN
    List list;
    QSignalSpy spy(&list, &List::dataChanged);

    // WHEN
    QVariant vv = QVariant::fromValue(v);
    if (vv.isNull()) {
        vv = QVariant();
    }
    bool ok = (list.*set)(0, vv);
    QVERIFY(ok);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE((list.*get)(0), vv);
}

int getRoleFromName(const QAbstractItemModel& model, const char* name)
{
    auto names = model.roleNames();
    auto i = names.constBegin();
    while (i != names.constEnd()) {
        if (i.value() == name) {
            return i.key();
        }
        ++i;
    }
    return -1;
}

template <typename V>
void testDataSetter(const char* roleName, const V v)
{
    // GIVEN
    List list;
    QSignalSpy spy(&list, &List::dataChanged);

    // WHEN
    int role = getRoleFromName(list, roleName);
    auto index = list.index(1, 0);
    QVariant vv = QVariant::fromValue(v);
    if (vv.isNull()) {
        vv = QVariant();
    }
    bool ok = list.setData(index, vv, role);
    QVERIFY(ok);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(list.data(index, role), vv);
}

template <typename V, typename Set, typename Get>
void test(const V v, Set set, Get get, const char* roleName)
{
    testSetter(v, set, get);
    //testDataSetter(roleName, v);
}

void TestRustListTypes::testConstructor()
{
    List list;
}

void TestRustListTypes::testBool()
{
    test(true, &List::setBoolean, &List::boolean, "boolean");
    test(false, &List::setBoolean, &List::boolean, "boolean");
}

void TestRustListTypes::testInt8()
{
    test(0, &List::setI8, &List::i8, "i8");
    test(1, &List::setI8, &List::i8, "i8");
    test(std::numeric_limits<int8_t>::min(), &List::setI8, &List::i8, "i8");
    test(std::numeric_limits<int8_t>::max(), &List::setI8, &List::i8, "i8");
}

void TestRustListTypes::testUint8()
{
    test(0, &List::setU8, &List::u8, "u8");
    test(1, &List::setU8, &List::u8, "u8");
    test(std::numeric_limits<uint8_t>::min(), &List::setU8, &List::u8, "u8");
    test(std::numeric_limits<uint8_t>::max(), &List::setU8, &List::u8, "u8");
}

void TestRustListTypes::testInt16()
{
    test(0, &List::setI16, &List::i16, "i16");
    test(1, &List::setI16, &List::i16, "i16");
    test(std::numeric_limits<int16_t>::min(), &List::setI16, &List::i16, "i16");
    test(std::numeric_limits<int16_t>::max(), &List::setI16, &List::i16, "i16");
}

void TestRustListTypes::testUint16()
{
    test(0, &List::setU16, &List::u16, "u16");
    test(1, &List::setU16, &List::u16, "u16");
    test(std::numeric_limits<uint16_t>::min(), &List::setU16, &List::u16, "u16");
    test(std::numeric_limits<uint16_t>::max(), &List::setU16, &List::u16, "u16");
}

void TestRustListTypes::testInt32()
{
    test(0, &List::setI32, &List::i32, "i32");
    test(1, &List::setI32, &List::i32, "i32");
    test(std::numeric_limits<int32_t>::min(), &List::setI32, &List::i32, "i32");
    test(std::numeric_limits<int32_t>::max(), &List::setI32, &List::i32, "i32");
}

void TestRustListTypes::testUint32()
{
    test(0, &List::setU32, &List::u32, "u32");
    test(1, &List::setU32, &List::u32, "u32");
    test(std::numeric_limits<uint32_t>::min(), &List::setU32, &List::u32, "u32");
    test(std::numeric_limits<uint32_t>::max(), &List::setU32, &List::u32, "u32");
}

void TestRustListTypes::testInt64()
{
    test(0, &List::setI64, &List::i64, "i64");
    test(1, &List::setI64, &List::i64, "i64");
    test(std::numeric_limits<int64_t>::min(), &List::setI64, &List::i64, "i64");
    test(std::numeric_limits<int64_t>::max(), &List::setI64, &List::i64, "i64");
}

void TestRustListTypes::testUint64()
{
    test(0, &List::setU64, &List::u64, "u64");
    test(1, &List::setU64, &List::u64, "u64");
    test(std::numeric_limits<uint64_t>::min(), &List::setU64, &List::u64, "u64");
    test(std::numeric_limits<uint64_t>::max(), &List::setU64, &List::u64, "u64");
}

void TestRustListTypes::testFloat()
{
    test(0, &List::setF32, &List::f32, "f32");
    test(1, &List::setF32, &List::f32, "f32");
    test(std::numeric_limits<float>::min(), &List::setF32, &List::f32, "f32");
    test(std::numeric_limits<float>::max(), &List::setF32, &List::f32, "f32");
}

void TestRustListTypes::testDouble()
{
    test(0, &List::setF64, &List::f64, "f64");
    test(1, &List::setF64, &List::f64, "f64");
    test(std::numeric_limits<double>::min(), &List::setF64, &List::f64, "f64");
    test(std::numeric_limits<double>::max(), &List::setF64, &List::f64, "f64");
}

void TestRustListTypes::testString()
{
    test(QString(""), &List::setString, &List::string, "string");
    test(QString("Konqi"), &List::setString, &List::string, "string");
    test(QString("$𐐷𤭢"), &List::setString, &List::string,  "string");
}

void TestRustListTypes::testOptionalString()
{
    test(QString(), &List::setOptionalString, &List::optionalString,
            "optionalString");
    test(QString(""), &List::setOptionalString, &List::optionalString,
            "optionalString");
    test(QString("Konqi"), &List::setOptionalString, &List::optionalString,
            "optionalString");
    test(QString("$𐐷𤭢"), &List::setOptionalString, &List::optionalString,
            "optionalString");
}

void TestRustListTypes::testByteArray()
{
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    test(QByteArray(data, 0), &List::setBytearray,
        &List::bytearray, "bytearray");
    test(QByteArray(data, 10), &List::setBytearray,
        &List::bytearray, "bytearray");
}

void TestRustListTypes::testOptionalByteArray()
{
    test(QByteArray(), &List::setOptionalBytearray, &List::optionalBytearray,
            "optionalBytearray");
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    test(QByteArray(data, 0), &List::setOptionalBytearray,
        &List::optionalBytearray, "optionalBytearray");
    test(QByteArray(data, 10), &List::setOptionalBytearray,
        &List::optionalBytearray, "optionalBytearray");
}


void TestRustListTypes::testStringGetter()
{
    List list;
    QCOMPARE(list.rowCount(), 10);
    QVariant value = list.data(list.index(0,0));
    // value should be empty string in default implementation
    QVERIFY(value.isValid());
    QCOMPARE(value.type(), QVariant::String);
    QCOMPARE(value.toString(), QString());
}

void TestRustListTypes::testStringSetter()
{
    // GIVEN
    List list;
    QSignalSpy spy(&list, &List::dataChanged);

    // WHEN
    const QModelIndex index(list.index(0,0));
    const bool set = list.setData(index, "Konqi");

    // THEN
    QVERIFY(set);
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QVariant value = list.data(list.index(0,0));
    QCOMPARE(value.toString(), QString("Konqi"));
}

QTEST_MAIN(TestRustListTypes)
#include "test_list_types.moc"
