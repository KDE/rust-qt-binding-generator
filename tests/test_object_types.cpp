#include "test_object_types_rust.h"
#include <QTest>
#include <QSignalSpy>

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
    Person person;
    const QVariant userName;
    QSignalSpy spy(&person, &Person::userNameChanged);

    // WHEN
    person.setUserName(userName);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(person.userName().type(), userName.type());
    QCOMPARE(person.userName(), userName);
}

void TestRustObjectTypes::testSetter_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::newRow("invalid") << QVariant();
    QTest::newRow("true") << QVariant(true);
    QTest::newRow("false") << QVariant(false);
    QTest::newRow("QString()") << QVariant(QString());
    QTest::newRow("QString(Konqi)") << QVariant("Konqi");
    QTest::newRow("QString($€𐐷𤭢)") << QVariant("$€𐐷𤭢");
    QTest::newRow("QByteArray()") << QVariant(QByteArray());
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    QTest::newRow("QByteArray(data)") << QVariant(QByteArray(data, 10));
}

QTEST_MAIN(TestRustObjectTypes)
#include "test_object_types.moc"
