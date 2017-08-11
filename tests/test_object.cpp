#include "test_object_rust.h"
#include <QTest>
#include <QSignalSpy>

class TestRustObject : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testStringGetter();
    void testStringSetter();
};

void TestRustObject::testConstructor()
{
    Person person;
}

void TestRustObject::testStringGetter()
{
    Person person;
    person.setUserName("Konqi");
}

void TestRustObject::testStringSetter()
{
    // GIVEN
    Person person;
    QSignalSpy spy(&person, &Person::userNameChanged);

    // WHEN
    person.setUserName("Konqi");

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(person.userName(), QString("Konqi"));
}

QTEST_MAIN(TestRustObject)
#include "test_object.moc"
