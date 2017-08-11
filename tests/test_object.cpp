#include "test_object_rust.h"
#include <QTest>

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
    Person person;
    person.setUserName("Konqi");
    QCOMPARE(person.userName(), QString("Konqi"));
}

QTEST_MAIN(TestRustObject)
#include "test_object.moc"
