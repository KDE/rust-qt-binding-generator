#include "test_objects_rust.h"
#include <QTest>
#include <QSignalSpy>

class TestRustObjects : public QObject
{
    Q_OBJECT
private slots:
    void testOneLevelConstructor();
    void testOneLevelStringGetter();
    void testOneLevelStringSetter();
    void testTwoLevelsConstructor();
    void testTwoLevelsStringGetter();
    void testTwoLevelsStringSetter();
};

void TestRustObjects::testOneLevelConstructor()
{
    Person person;
}

void TestRustObjects::testOneLevelStringGetter()
{
    Person person;
    person.object()->setDescription("Konqi");
}

void TestRustObjects::testOneLevelStringSetter()
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

void TestRustObjects::testTwoLevelsConstructor()
{
    Group group;
}

void TestRustObjects::testTwoLevelsStringGetter()
{
    Group group;
    group.person()->object()->setDescription("Konqi");
}

void TestRustObjects::testTwoLevelsStringSetter()
{
    // GIVEN
    Group group;
    QSignalSpy spy(group.person()->object(), &InnerObject::descriptionChanged);

    // WHEN
    group.person()->object()->setDescription("Konqi");

    // THEN
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(group.person()->object()->description(), QString("Konqi"));
}

QTEST_MAIN(TestRustObjects)
#include "test_objects.moc"
