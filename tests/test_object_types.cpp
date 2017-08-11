#include "test_object_types_rust.h"
#include <QTest>
#include <QSignalSpy>
#include <QDebug>

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

QTEST_MAIN(TestRustObject)
#include "test_object_types.moc"
