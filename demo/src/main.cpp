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

#include "Bindings.h"
#include "SortedModel.h"

#ifdef QT_CHARTS_LIB
#include <QtCharts>
#endif

#ifdef QT_QUICK_LIB
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickView>
#include <QtQml/qqml.h>
#endif

#include <QApplication>
#include <QComboBox>
#include <QCommandLineParser>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QStringListModel>
#include <QStyleFactory>
#include <QSvgRenderer>
#include <QSvgWidget>
#include <QTabWidget>
#include <QTableView>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWindow>

#include <cstdlib>

struct Model {
    QStringListModel styles;
    Demo demo;
    SortedModel sortedFileSystem;
    SortedModel sortedProcesses;
};

void setStyle(QWidget* w, QStyle* style) {
    for (QObject* o: w->children()) {
        QWidget* c = dynamic_cast<QWidget*>(o);
        if (c) {
            setStyle(c, style);
        }
    }
    w->setStyle(style);
}

QWindow* getWindow(QWidget* w) {
    QWidget* top = w;
    while (top && top->parentWidget()) {
        top = top->parentWidget();
    }
    return top->windowHandle();
}

#ifdef QT_QUICK_LIB

void copyWindowGeometry(const QRect& rect, QObject* c) {
    if (rect.width() && rect.height()) {
        c->setProperty("x", rect.x());
        c->setProperty("y", rect.y());
        c->setProperty("width", rect.width());
        c->setProperty("heigth", rect.height());
    }
}

bool isQQC2Style(const QString& name) {
    return name == "Kirigami 2" || name.startsWith("QtQuick Controls 2");
}

QString qqc2Style(const QString& name) {
    if (name.startsWith("QtQuick Controls 2")) {
        return name.mid(19);
    }
    return QString("Default");
}

void createQtQuick(const QString& name, const QString& qml, Model* model,
            QWidget* widgets, const QString& initialTab) {
    static QString qqc2style;
    static bool blocked = false;
    if (blocked) {
        return;
    }
    blocked = true;
    if (qqc2style.isNull() && isQQC2Style(name)) {
        qqc2style = qqc2Style(name);
        QQuickStyle::setStyle(qqc2style);
        QStringListModel& styles = model->styles;
        int i = 0;
        while (i < styles.rowCount()) {
            QString style = styles.data(styles.index(i)).toString();
            if (isQQC2Style(style) && style != name) {
                styles.removeRows(i, 1);
            } else {
                ++i;
            }
        }
    }
    blocked = false;
    QQmlApplicationEngine* engine = new QQmlApplicationEngine();
    QQmlContext* c = engine->rootContext();
    c->setContextProperty("styles", &model->styles);
    c->setContextProperty("demo", &model->demo);
    c->setContextProperty("sortedFileSystem", &model->sortedFileSystem);
    c->setContextProperty("widgets", widgets);
    QRect geometry;
    QWindow* window = getWindow(widgets);
    if (window) {
        geometry = window->geometry();
    } else {
        geometry = QRect(0, 0, 800, 640);
    }
    engine->load(QUrl(qml));
    QObject* root = engine->rootObjects().first();
    copyWindowGeometry(geometry, root);
    root->setProperty("initialTab", initialTab);
    root->setProperty("qtquickIndex",
            QVariant(model->styles.stringList().indexOf(name)));
    root->setProperty("processes",
            QVariant::fromValue(&model->sortedProcesses));
}

#endif

QComboBox* createStyleComboBox(Model* model) {
    QComboBox* box = new QComboBox();
    box->setModel(&model->styles);
    auto styles = QStyleFactory::keys();
    QString currentStyle = QApplication::style()->objectName().toLower();
    for (auto v: styles) {
        box->addItem("QWidgets " + v);
        if (v.toLower() == currentStyle) {
            box->setCurrentText(v);
        }
    }
#ifdef QT_QUICK_LIB
    box->addItem("QtQuick");
#endif
#ifdef QTQUICKCONTROLS2
    for (auto style: QQuickStyle::availableStyles()) {
        if (style != "Plasma") { // Plasma style is buggy
            box->addItem("QtQuick Controls 2 " + style);
        }
    }
#endif
#ifdef KIRIGAMI2
    box->addItem("Kirigami 2");
#endif
    return box;
}

QStatusBar* createStatusBar(Model* model, QWidget* main, QComboBox* box,
        const QString& initialTab) {
    QRect windowRect;
    auto f = [windowRect, box, main, model, initialTab](const QString &text) mutable {
        QWindow* window = getWindow(main);
        bool visible = main->isVisible();
        if (text.startsWith("QWidgets ")) {
            main->setVisible(true);
            if (window && !visible) {
                window->setX(windowRect.x());
                window->setY(windowRect.y());
                window->setWidth(windowRect.width());
                window->setHeight(windowRect.height());
            }
            setStyle(main, QStyleFactory::create(text.mid(9)));
#ifdef QT_QUICK_LIB
        } else {
            if (window) {
                windowRect.setX(window->x());
                windowRect.setY(window->y());
                windowRect.setWidth(window->width());
                windowRect.setHeight(window->height());
            }
            main->setVisible(false);
#ifdef QTQUICKCONTROLS2
            if (text.startsWith("QtQuick Controls 2")) {
                createQtQuick(text, "qrc:///qml/demo-qtquick2.qml",
                    model, box, initialTab);
            } else
#endif
#ifdef KIRIGAMI2
            if (text == "Kirigami 2") {
                createQtQuick("Kirigami 2", "qrc:///qml/demo-kirigami2.qml",
                    model, box, initialTab);
            } else
#endif
            createQtQuick("QtQuick", "qrc:///qml/demo.qml", model, box, initialTab);
#endif
        }
    };
    box->connect(box, &QComboBox::currentTextChanged, box, f);

    QSvgWidget* rust_qt_binding_generator = new QSvgWidget(":/rust_qt_binding_generator.svg");
    rust_qt_binding_generator->setFixedSize(rust_qt_binding_generator->renderer()->defaultSize() / 4);

    QStatusBar* statusBar = new QStatusBar;
    statusBar->addPermanentWidget(rust_qt_binding_generator);
    statusBar->addPermanentWidget(box);
    return statusBar;
}

QWidget* createObjectTab(Model* model) {
    QWidget* view = new QWidget;
    Fibonacci* fibonacci = model->demo.fibonacci();

    QLabel* label = new QLabel;
    label->setText(QCoreApplication::translate("main", "Calculate the <i>nth</i> Fibonacci number"));

    QLineEdit* input = new QLineEdit;
    input->setPlaceholderText("Your number");
    input->setValidator(new QIntValidator(0, 100));
    input->connect(input, &QLineEdit::textChanged, fibonacci,
        [fibonacci](const QString& text) {
            fibonacci->setInput(text.toInt());
    });
    fibonacci->connect(fibonacci, &Fibonacci::inputChanged, input,
        [input, fibonacci]() {
            input->setText(QString::number(fibonacci->input()));
    });

    QLabel* result = new QLabel;
    fibonacci->connect(fibonacci, &Fibonacci::resultChanged, result,
        [result, fibonacci]() {
            result->setText(QCoreApplication::translate("main", "The Fibonacci number: ")
                + QString::number(fibonacci->result()));
    });
    input->setText(QString::number(model->demo.fibonacci()->input()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(input);
    layout->addWidget(result);
    view->setLayout(layout);
    return view;
}

QWidget* createListTab(Model* model) {
    QTableView* view = new QTableView();
    model->demo.fibonacciList()->setHeaderData(0, Qt::Horizontal,
        QCoreApplication::translate("main", "Row"), Qt::DisplayRole);
    model->demo.fibonacciList()->setHeaderData(1, Qt::Horizontal,
        QCoreApplication::translate("main", "Fibonacci number"), Qt::DisplayRole);
    view->setModel(model->demo.fibonacciList());
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return view;
}

QWidget* createTreeTab(Model* model) {
    QTreeView* view = new QTreeView();
    model->demo.fileSystemTree()->setHeaderData(0, Qt::Horizontal,
        QCoreApplication::translate("main", "Name"), Qt::DisplayRole);
    model->demo.fileSystemTree()->setHeaderData(1, Qt::Horizontal,
        QCoreApplication::translate("main", "Size"), Qt::DisplayRole);
    view->setUniformRowHeights(true);
    view->setSortingEnabled(true);
    view->setModel(&model->sortedFileSystem);
    view->header()->setSectionHidden(2, true);
    view->header()->setSectionHidden(3, true);
    view->header()->setSectionHidden(4, true);
    auto root = model->sortedFileSystem.index(0, 0);
    view->expand(root);
    view->sortByColumn(0, Qt::AscendingOrder);
    view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return view;
}

QWidget* createProcessesTab(Model* model) {
    QTreeView* view = new QTreeView();
    view->setUniformRowHeights(true);
    view->setSortingEnabled(true);
    view->setModel(&model->sortedProcesses);
    // expand when the model is populated
    view->connect(&model->sortedProcesses, &QAbstractItemModel::rowsInserted,
        view, [view](const QModelIndex& index) {
        if (!index.isValid()) {
            view->expandAll();
        }
    });
    view->expandAll();
    view->sortByColumn(0, Qt::AscendingOrder);
    return view;
}

#ifdef QT_CHARTS_LIB

using namespace QtCharts;

QWidget* createChartTab(Model* model) {
    QLineSeries *sin = new QLineSeries();
    sin->setName("sin");
    QVXYModelMapper *sinMapper = new QVXYModelMapper(sin);
    sinMapper->setXColumn(0);
    sinMapper->setYColumn(1);
    sinMapper->setSeries(sin);
    sinMapper->setModel(model->demo.timeSeries());

    QLineSeries *cos = new QLineSeries();
    cos->setName("cos");
    QVXYModelMapper *cosMapper = new QVXYModelMapper(cos);
    cosMapper->setXColumn(0);
    cosMapper->setYColumn(2);
    cosMapper->setSeries(cos);
    cosMapper->setModel(model->demo.timeSeries());

    QChart* chart = new QChart;
    chart->addSeries(sin);
    chart->addSeries(cos);

    QValueAxis *axisX = new QValueAxis;
    axisX->setLabelFormat("%.1f");
    axisX->setTitleText("time [s]");
    chart->addAxis(axisX, Qt::AlignBottom);
    sin->attachAxis(axisX);
    cos->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%.1f");
    axisY->setTitleText("electric potential [V]");
    chart->addAxis(axisY, Qt::AlignLeft);
    sin->attachAxis(axisY);
    cos->attachAxis(axisY);

    QWidget* view = new QWidget;
    QTableView* data = new QTableView;
    data->setModel(model->demo.timeSeries());
    QChartView *chartView = new QChartView(chart);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(data);
    layout->addWidget(chartView);
    view->setLayout(layout);
    return view;
}
#endif

QTabWidget* createTabs(Model* model) {
    QTabWidget* tabs = new QTabWidget();
    tabs->addTab(createObjectTab(model), "object");
    tabs->addTab(createListTab(model), "list");
    tabs->addTab(createTreeTab(model), "tree");
    const int procTab = tabs->addTab(createProcessesTab(model), "processes");
    tabs->connect(tabs, &QTabWidget::currentChanged, model->demo.processes(),
        [model, procTab](int current) {
            model->demo.processes()->setActive(current == procTab);
        });
#ifdef QT_CHARTS_LIB
    tabs->addTab(createChartTab(model), "chart");
#endif
    return tabs;
}

void createMainWindow(Model* model, const QString& initialStyle,
                      const QString& initialTab) {
    QMainWindow* main = new QMainWindow();
    main->setMinimumSize(QSize(800, 640));

    QComboBox* box = createStyleComboBox(model);
    QStatusBar* statusBar = createStatusBar(model, main, box, initialTab);
    main->setStatusBar(statusBar);
    QTabWidget* tabs = createTabs(model);
    main->setCentralWidget(tabs);
    main->show();
    box->setCurrentText(initialStyle);
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i) == initialTab) {
            tabs->setCurrentIndex(i);
        }
    }
}

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/rust_qt_binding_generator.svg"));

#ifdef QT_QUICK_LIB
    qmlRegisterType<QSortFilterProxyModel>("org.qtproject.example", 1, 0, "SortFilterProxyModel");
    qmlRegisterType<Fibonacci>("rust", 1, 0, "Fibonacci");
    qmlRegisterType<FibonacciList>("rust", 1, 0, "FibonacciList");
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription("Demo application for Qt and RUst");
    parser.addHelpOption();
    parser.addVersionOption();
    // --initial-style
    QCommandLineOption initialStyleOption(QStringList()
            << "initial-style",
            QCoreApplication::translate("main", "Initial widget style."),
            QCoreApplication::translate("main", "style"));
    parser.addOption(initialStyleOption);
    // --initial-tab
    QCommandLineOption initialTabOption(QStringList()
            << "initial-tab",
            QCoreApplication::translate("main", "Initial tab."),
            QCoreApplication::translate("main", "tab"));
    parser.addOption(initialTabOption);
    parser.process(app);

    Model model;
    model.demo.fibonacci()->setInput(1);
    model.demo.fileSystemTree()->setPath("/");
    model.sortedFileSystem.setSourceModel(model.demo.fileSystemTree());
    model.sortedFileSystem.setDynamicSortFilter(true);
    model.sortedProcesses.setSourceModel(model.demo.processes());
    model.sortedProcesses.setDynamicSortFilter(true);

    createMainWindow(&model, parser.value(initialStyleOption),
                             parser.value(initialTabOption));

    return app.exec();
}
