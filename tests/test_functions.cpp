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

#include "test_functions_rust.h"
#include <QTest>
#include <QSignalSpy>

class TestRustObject : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testStringFunction();
    void testSimpleFunction();
    void testVoidFunction();
};

void TestRustObject::testConstructor()
{
    Person person;
}

void TestRustObject::testSimpleFunction()
{
    // GIVEN
    Person person;
    person.setUserName("Konqi");

    // THEN
    QCOMPARE(person.userName(), QString("Konqi"));
    QCOMPARE((int)person.vowelsInName(), 2);
}

void TestRustObject::testVoidFunction()
{
    // GIVEN
    Person person;
    person.setUserName("Konqi");

    // THEN
    person.doubleName();
    QCOMPARE(person.userName(), QString("KonqiKonqi"));
}

void TestRustObject::testStringFunction()
{
    // GIVEN
    Person person;
    person.setUserName("Konqi");

    // THEN
    QCOMPARE(person.greet("John"), QString("Hello John, my name is Konqi, how is it going?"));
}

QTEST_MAIN(TestRustObject)
#include "test_functions.moc"
