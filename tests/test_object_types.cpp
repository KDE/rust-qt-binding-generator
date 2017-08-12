#include "test_object_types_rust.h"
#include <QTest>
#include <QSignalSpy>
#include <QDebug>

class TestRustObjectTypes : public QObject
{
    Q_OBJECT
private slots:
// no support for QVariant: it's too hard to implement all of it
//    void testSetter();
//    void testSetter_data();
    void testBool();
    void testInteger();
    void testUinteger();
    void testString();
    void testByteArray();
};

template <typename V, typename Set, typename Get, typename Changed>
void testSetter(V v, Set set, Get get, Changed changed)
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

void TestRustObjectTypes::testInteger()
{
    testSetter(0, &Object::setInteger,
        &Object::integer, &Object::integerChanged);
    testSetter(1, &Object::setInteger,
        &Object::integer, &Object::integerChanged);
    testSetter(std::numeric_limits<int>::min(), &Object::setInteger,
        &Object::integer, &Object::integerChanged);
    testSetter(std::numeric_limits<int>::max(), &Object::setInteger,
        &Object::integer, &Object::integerChanged);
}

void TestRustObjectTypes::testUinteger()
{
    testSetter(0, &Object::setUinteger,
        &Object::uinteger, &Object::uintegerChanged);
    testSetter(1, &Object::setUinteger,
        &Object::uinteger, &Object::uintegerChanged);
    testSetter(std::numeric_limits<uint>::min(), &Object::setUinteger,
        &Object::uinteger, &Object::uintegerChanged);
    testSetter(std::numeric_limits<uint>::max(), &Object::setUinteger,
        &Object::uinteger, &Object::uintegerChanged);
}

void TestRustObjectTypes::testString()
{
    testSetter(QString(), &Object::setString,
        &Object::string, &Object::stringChanged);
    testSetter(QString("Konqi"), &Object::setString,
        &Object::string, &Object::stringChanged);
    testSetter(QString("$𐐷𤭢"), &Object::setString,
        &Object::string, &Object::stringChanged);
}

void TestRustObjectTypes::testByteArray()
{
    testSetter(QByteArray(), &Object::setBytearray,
        &Object::bytearray, &Object::bytearrayChanged);
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    testSetter(QByteArray(data, 10), &Object::setBytearray,
        &Object::bytearray, &Object::bytearrayChanged);
}

QTEST_MAIN(TestRustObjectTypes)
#include "test_object_types.moc"
