/*
 *   Copyright 2017  Jos van den Oever <jos@vandenoever.info>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
