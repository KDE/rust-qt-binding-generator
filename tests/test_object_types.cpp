#include "test_object_types_rust.h"
#include <QTest>
#include <QSignalSpy>
#include <QDebug>

class TestRustObjectTypes : public QObject
{
    Q_OBJECT
private slots:
    void testBool();
    void testOptionalBool();
    void testInt8();
    void testUint8();
    void testInt16();
    void testUint16();
    void testInt32();
    void testUint32();
    void testInt64();
    void testUint64();
    void testOptionalUint64();
    void testFloat();
    void testDouble();
    void testString();
    void testOptionalString();
    void testByteArray();
    void testOptionalByteArray();
};

template <typename V, typename Set, typename Get, typename Changed>
void testSetter(const V v, Set set, Get get, Changed changed)
{
    // GIVEN
    Object object;
    QSignalSpy spy(&object, changed);

    // WHEN
    (object.*set)((V)v);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE((V)(object.*get)(), (V)v);
}

void TestRustObjectTypes::testBool()
{
    testSetter(true, &Object::setBoolean,
        &Object::boolean, &Object::booleanChanged);
    testSetter(false, &Object::setBoolean,
        &Object::boolean, &Object::booleanChanged);
}

void TestRustObjectTypes::testOptionalBool()
{
    testSetter(QVariant(), &Object::setOptionalBoolean,
        &Object::optionalBoolean, &Object::optionalBooleanChanged);
    testSetter(QVariant::fromValue(true), &Object::setOptionalBoolean,
        &Object::optionalBoolean, &Object::optionalBooleanChanged);
    testSetter(QVariant::fromValue(false), &Object::setOptionalBoolean,
        &Object::optionalBoolean, &Object::optionalBooleanChanged);
}

void TestRustObjectTypes::testInt8()
{
    testSetter(0, &Object::setI8,
        &Object::i8, &Object::i8Changed);
    testSetter(1, &Object::setI8,
        &Object::i8, &Object::i8Changed);
    testSetter(std::numeric_limits<int8_t>::min(), &Object::setI8,
        &Object::i8, &Object::i8Changed);
    testSetter(std::numeric_limits<int8_t>::max(), &Object::setI8,
        &Object::i8, &Object::i8Changed);
}

void TestRustObjectTypes::testUint8()
{
    testSetter(0, &Object::setU8,
        &Object::u8, &Object::u8Changed);
    testSetter(1, &Object::setU8,
        &Object::u8, &Object::u8Changed);
    testSetter(std::numeric_limits<uint8_t>::min(), &Object::setU8,
        &Object::u8, &Object::u8Changed);
    testSetter(std::numeric_limits<uint8_t>::max(), &Object::setU8,
        &Object::u8, &Object::u8Changed);
}

void TestRustObjectTypes::testInt16()
{
    testSetter(0, &Object::setI16,
        &Object::i16, &Object::i16Changed);
    testSetter(1, &Object::setI16,
        &Object::i16, &Object::i16Changed);
    testSetter(std::numeric_limits<int16_t>::min(), &Object::setI16,
        &Object::i16, &Object::i16Changed);
    testSetter(std::numeric_limits<int16_t>::max(), &Object::setI16,
        &Object::i16, &Object::i16Changed);
}

void TestRustObjectTypes::testUint16()
{
    testSetter(0, &Object::setU16,
        &Object::u16, &Object::u16Changed);
    testSetter(1, &Object::setU16,
        &Object::u16, &Object::u16Changed);
    testSetter(std::numeric_limits<uint16_t>::min(), &Object::setU16,
        &Object::u16, &Object::u16Changed);
    testSetter(std::numeric_limits<uint16_t>::max(), &Object::setU16,
        &Object::u16, &Object::u16Changed);
}

void TestRustObjectTypes::testInt32()
{
    testSetter(0, &Object::setI32,
        &Object::i32, &Object::i32Changed);
    testSetter(1, &Object::setI32,
        &Object::i32, &Object::i32Changed);
    testSetter(std::numeric_limits<int32_t>::min(), &Object::setI32,
        &Object::i32, &Object::i32Changed);
    testSetter(std::numeric_limits<int32_t>::max(), &Object::setI32,
        &Object::i32, &Object::i32Changed);
}

void TestRustObjectTypes::testUint32()
{
    testSetter(0, &Object::setU32,
        &Object::u32, &Object::u32Changed);
    testSetter(1, &Object::setU32,
        &Object::u32, &Object::u32Changed);
    testSetter(std::numeric_limits<uint32_t>::min(), &Object::setU32,
        &Object::u32, &Object::u32Changed);
    testSetter(std::numeric_limits<uint32_t>::max(), &Object::setU32,
        &Object::u32, &Object::u32Changed);
}

void TestRustObjectTypes::testInt64()
{
    testSetter(0, &Object::setI64,
        &Object::i64, &Object::i64Changed);
    testSetter(1, &Object::setI64,
        &Object::i64, &Object::i64Changed);
    testSetter(std::numeric_limits<int64_t>::min(), &Object::setI64,
        &Object::i64, &Object::i64Changed);
    testSetter(std::numeric_limits<int64_t>::max(), &Object::setI64,
        &Object::i64, &Object::i64Changed);
}

void TestRustObjectTypes::testUint64()
{
    testSetter(0, &Object::setU64,
        &Object::u64, &Object::u64Changed);
    testSetter(1, &Object::setU64,
        &Object::u64, &Object::u64Changed);
    testSetter(std::numeric_limits<uint64_t>::min(), &Object::setU64,
        &Object::u64, &Object::u64Changed);
    testSetter(std::numeric_limits<uint64_t>::max(), &Object::setU64,
        &Object::u64, &Object::u64Changed);
}

void TestRustObjectTypes::testOptionalUint64()
{
    testSetter(QVariant(), &Object::setOptionalU64,
        &Object::optionalU64, &Object::optionalU64Changed);
    testSetter(QVariant::fromValue(0), &Object::setOptionalU64,
        &Object::optionalU64, &Object::optionalU64Changed);
    testSetter(QVariant::fromValue(1), &Object::setOptionalU64,
        &Object::optionalU64, &Object::optionalU64Changed);
    testSetter(QVariant::fromValue(std::numeric_limits<uint64_t>::min()),
        &Object::setOptionalU64, &Object::optionalU64,
        &Object::optionalU64Changed);
    testSetter(QVariant::fromValue(std::numeric_limits<uint64_t>::max()),
        &Object::setOptionalU64, &Object::optionalU64,
        &Object::optionalU64Changed);
}

void TestRustObjectTypes::testFloat()
{
    testSetter(0, &Object::setF32,
        &Object::f32, &Object::f32Changed);
    testSetter(1, &Object::setF32,
        &Object::f32, &Object::f32Changed);
    testSetter(std::numeric_limits<float>::min(), &Object::setF32,
        &Object::f32, &Object::f32Changed);
    testSetter(std::numeric_limits<float>::max(), &Object::setF32,
        &Object::f32, &Object::f32Changed);
}

void TestRustObjectTypes::testDouble()
{
    testSetter(0, &Object::setF64,
        &Object::f64, &Object::f64Changed);
    testSetter(1, &Object::setF64,
        &Object::f64, &Object::f64Changed);
    testSetter(std::numeric_limits<double>::min(), &Object::setF64,
        &Object::f64, &Object::f64Changed);
    testSetter(std::numeric_limits<double>::max(), &Object::setF64,
        &Object::f64, &Object::f64Changed);
}

void TestRustObjectTypes::testString()
{
    testSetter(QString(), &Object::setString,
        &Object::string, &Object::stringChanged);
    testSetter(QString(""), &Object::setString,
        &Object::string, &Object::stringChanged);
    testSetter(QString("Konqi"), &Object::setString,
        &Object::string, &Object::stringChanged);
    testSetter(QString("$𐐷𤭢"), &Object::setString,
        &Object::string, &Object::stringChanged);
}

void TestRustObjectTypes::testOptionalString()
{
    testSetter(QString(), &Object::setOptionalString,
        &Object::optionalString, &Object::optionalStringChanged);
    testSetter(QString(""), &Object::setOptionalString,
        &Object::optionalString, &Object::optionalStringChanged);
    testSetter(QString("Konqi"), &Object::setOptionalString,
        &Object::optionalString, &Object::optionalStringChanged);
    testSetter(QString("$𐐷𤭢"), &Object::setOptionalString,
        &Object::optionalString, &Object::optionalStringChanged);
}

void TestRustObjectTypes::testByteArray()
{
    testSetter(QByteArray(), &Object::setBytearray,
        &Object::bytearray, &Object::bytearrayChanged);
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    testSetter(QByteArray(data, 0), &Object::setBytearray,
        &Object::bytearray, &Object::bytearrayChanged);
    testSetter(QByteArray(data, 10), &Object::setBytearray,
        &Object::bytearray, &Object::bytearrayChanged);
}

void TestRustObjectTypes::testOptionalByteArray()
{
    testSetter(QByteArray(), &Object::setOptionalBytearray,
        &Object::optionalBytearray, &Object::optionalBytearrayChanged);
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    testSetter(QByteArray(data, 0), &Object::setOptionalBytearray,
        &Object::optionalBytearray, &Object::optionalBytearrayChanged);
    testSetter(QByteArray(data, 10), &Object::setOptionalBytearray,
        &Object::optionalBytearray, &Object::optionalBytearrayChanged);
}

QTEST_MAIN(TestRustObjectTypes)
#include "test_object_types.moc"
