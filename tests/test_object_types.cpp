#include "test_object_types_rust.h"
#include <QTest>
#include <QSignalSpy>
#include <QDebug>

class TestRustObjectTypes : public QObject
{
    Q_OBJECT
private slots:
    void testSetter();
    void testSetter_data();
};

void TestRustObjectTypes::testSetter()
{
    // GIVEN
    QFETCH(QVariant, value);
    Object object;
    QSignalSpy spy(&object, &Object::valueChanged);

    // WHEN
    object.setValue(value);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    auto resultType = object.value().type();
    QCOMPARE(resultType, value.type());
    QCOMPARE(object.value(), value);
}

void TestRustObjectTypes::testSetter_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::newRow("invalid") << QVariant();
    QTest::newRow("true") << QVariant(true);
    QTest::newRow("false") << QVariant(false);
    QTest::newRow("0") << QVariant((int)0);
    QTest::newRow("1") << QVariant((int)1);
    int min_int = std::numeric_limits<int>::min();
    QTest::newRow("min_int") << QVariant(min_int);
    int max_int = std::numeric_limits<int>::max();
    QTest::newRow("max_int") << QVariant(max_int);
    QTest::newRow("QString()") << QVariant(QString());
    QTest::newRow("QString(Konqi)") << QVariant("Konqi");
    QTest::newRow("QString($€𐐷𤭢)") << QVariant("$€𐐷𤭢");
    QTest::newRow("QByteArray()") << QVariant(QByteArray());
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    QTest::newRow("QByteArray(data)") << QVariant(QByteArray(data, 10));
}

QTEST_MAIN(TestRustObjectTypes)
#include "test_object_types.moc"
