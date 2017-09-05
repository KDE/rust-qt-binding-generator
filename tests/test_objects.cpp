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
