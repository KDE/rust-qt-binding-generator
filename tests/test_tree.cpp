#include "test_list_rust.h"
#include <QTest>
#include <QSignalSpy>

class TestRustTree : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testStringGetter();
    void testStringSetter();
};

void TestRustTree::testConstructor()
{
    Persons persons;
}

void TestRustTree::testStringGetter()
{
    Persons persons;
    QCOMPARE(persons.rowCount(), 10);
    QVariant value = persons.data(persons.index(0,0));
    // value should be empty string in default implementation
    QVERIFY(value.isValid());
    QCOMPARE(value.type(), QVariant::String);
    QCOMPARE(value.toString(), QString());
}

void TestRustTree::testStringSetter()
{
    // GIVEN
    Persons persons;
    QSignalSpy spy(&persons, &Persons::dataChanged);

    // WHEN
    const QModelIndex index(persons.index(0,0));
    const bool set = persons.setData(index, "Konqi");

    // THEN
    QVERIFY(set);
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QVariant value = persons.data(persons.index(0,0));
    QCOMPARE(value.toString(), QString("Konqi"));
}

QTEST_MAIN(TestRustTree)
#include "test_tree.moc"
