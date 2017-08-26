#include "test_objects_rust.h"
#include <QTest>
#include <QSignalSpy>

class TestRustObjects : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testStringGetter();
    void testStringSetter();
};

void TestRustObjects::testConstructor()
{
    Person person;
}

void TestRustObjects::testStringGetter()
{
    Person person;
    person.object()->setDescription("Konqi");
}

void TestRustObjects::testStringSetter()
{
    // GIVEN
    Person person;
    QSignalSpy spy(person.object(), &InnerObject::descriptionChanged);

    // WHEN
    person.object()->setDescription("Konqi");

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(person.object()->description(), QString("Konqi"));
}

QTEST_MAIN(TestRustObjects)
#include "test_objects.moc"
