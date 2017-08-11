#include "test_object_types_rust.h"
#include <QTest>
#include <QSignalSpy>

class TestRustObjectTypes : public QObject
{
    Q_OBJECT
private slots:
    void testInvalid();
    void testBool();
    void testString();
    void testByteArray();
};

void TestRustObjectTypes::testInvalid()
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

void TestRustObjectTypes::testBool()
{
    // GIVEN
    Person person;
    const QVariant userName(true);
    QSignalSpy spy(&person, &Person::userNameChanged);

    // WHEN
    person.setUserName(userName);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(person.userName().type(), userName.type());
    QCOMPARE(person.userName(), userName);
}

void TestRustObjectTypes::testString()
{
    // GIVEN
    Person person;
    const QVariant userName(QString("Konqi"));
    QSignalSpy spy(&person, &Person::userNameChanged);

    // WHEN
    person.setUserName(userName);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(person.userName().type(), userName.type());
    QCOMPARE(person.userName(), userName);
}

void TestRustObjectTypes::testByteArray()
{
    // GIVEN
    Person person;
    const char data[10] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9};
    const QVariant userName(QByteArray(data, 10));
    QSignalSpy spy(&person, &Person::userNameChanged);

    // WHEN
    person.setUserName(userName);

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(person.userName().type(), userName.type());
    QCOMPARE(person.userName(), userName);
}

QTEST_MAIN(TestRustObjectTypes)
#include "test_object_types.moc"
