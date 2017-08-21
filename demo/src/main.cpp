#include "Tree.h"
#include "Fibonacci.h"
#include "TimeSeries.h"

#ifdef QT_CHARTS_LIB
#include <QtCharts>
#endif

#ifdef QT_QUICK_LIB
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickItem>
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
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWindow>

#include <cstdlib>

struct Models {
    QStringListModel styles;
    Fibonacci fibonacci;
    FibonacciList fibonacciList;
    Tree tree;
    QSortFilterProxyModel sortedTree;
    TimeSeries timeSeries;
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

void copyWindowGeometry(QWidget* w, QQmlContext* c) {
    QWindow* window = getWindow(w);
    if (window) {
        c->setContextProperty("windowX", window->x());
        c->setContextProperty("windowY", window->y());
        c->setContextProperty("windowWidth", window->width());
        c->setContextProperty("windowHeight", window->height());
    }
}

void createQtQuick(const QString& name, const QString& qml, Models* models, QWidget* widgets) {
    QQmlApplicationEngine* engine = new QQmlApplicationEngine();
    QQmlContext* c = engine->rootContext();
    c->setContextProperty("fsModel", &models->tree);
    c->setContextProperty("sortedFsModel", &models->sortedTree);
    c->setContextProperty("fibonacci", &models->fibonacci);
    c->setContextProperty("fibonacciList", &models->fibonacciList);
    c->setContextProperty("styles", &models->styles);
    c->setContextProperty("widgets", widgets);
    c->setContextProperty("qtquickIndex",
        QVariant(models->styles.stringList().indexOf(name)));
    copyWindowGeometry(widgets, engine->rootContext());
    engine->load(QUrl(qml));
}

#endif

QComboBox* createStyleComboBox(Models* models) {
    QComboBox* box = new QComboBox();
    box->setModel(&models->styles);
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
    box->addItem("QtQuick Controls 2");
#endif
    return box;
}

QWidget* createStyleTab(Models* models, QWidget* tabs) {
    QComboBox* box = createStyleComboBox(models);
    QRect windowRect;
    box->connect(box, &QComboBox::currentTextChanged, box, [windowRect, box, tabs, models](const QString &text) mutable {
        QWindow* window = getWindow(tabs);
        bool visible = tabs->isVisible();
        if (text.startsWith("QWidgets ")) {
            tabs->setVisible(true);
            if (window && !visible) {
                window->setX(windowRect.x());
                window->setY(windowRect.y());
                window->setWidth(windowRect.width());
                window->setHeight(windowRect.height());
            }
            setStyle(tabs, QStyleFactory::create(text.mid(9)));
#ifdef QT_QUICK_LIB
        } else {
            if (window) {
                windowRect.setX(window->x());
                windowRect.setY(window->y());
                windowRect.setWidth(window->width());
                windowRect.setHeight(window->height());
            }
            tabs->setVisible(false);
#ifdef QTQUICKCONTROLS2
            if (text == "QtQuick Controls 2") {
                createQtQuick("QtQuick Controls 2", "qrc:///demo-qtquick2.qml",
                    models, box);
            } else
#endif
            createQtQuick("QtQuick", "qrc:///demo.qml", models, box);
#endif
        }
    });
    return box;
}

QWidget* createObjectTab(Models* models) {
    QWidget* view = new QWidget;
    Fibonacci* fibonacci = &models->fibonacci;

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

    QLabel* label = new QLabel;
    fibonacci->connect(fibonacci, &Fibonacci::resultChanged, label,
        [label, fibonacci]() {
            label->setText("The Fibonacci number: "
                + QString::number(fibonacci->result()));
    });

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(input);
    layout->addWidget(label);
    view->setLayout(layout);
    return view;
}

QWidget* createListTab(Models* models) {
    QListView* view = new QListView();
    view->setModel(&models->fibonacciList);
    return view;
}

QWidget* createTreeTab(Models* models) {
    QTreeView* view = new QTreeView();
    view->setUniformRowHeights(true);
    view->setSortingEnabled(true);
    view->setModel(&models->sortedTree);
    auto root = models->sortedTree.index(0, 0);
    view->expand(root);
    view->sortByColumn(0, Qt::AscendingOrder);
    view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return view;
}

#ifdef QT_CHARTS_LIB

using namespace QtCharts;

QWidget* createChartTab(Models* models) {
    QLineSeries *series = new QLineSeries();
    series->setName("Line 1");
    QVXYModelMapper *mapper = new QVXYModelMapper(series);
    mapper->setXColumn(0);
    mapper->setYColumn(1);
    mapper->setSeries(series);
    mapper->setModel(&models->timeSeries);

    QChart* chart = new QChart;
    chart->addSeries(series);
    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("MMM yyyy");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Sunspots count");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QWidget* view = new QWidget;
    QTableView* data = new QTableView;
    data->setModel(&models->timeSeries);
    QChartView *chartView = new QChartView(chart);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(data);
    layout->addWidget(chartView);
    view->setLayout(layout);
    return view;
}
#endif

void createWidgets(Models* models) {
    QTabWidget* tabs = new QTabWidget();

    tabs->addTab(createStyleTab(models, tabs), "style");
    tabs->addTab(createObjectTab(models), "object");
    tabs->addTab(createListTab(models), "list");
    tabs->addTab(createTreeTab(models), "tree");
#ifdef QT_CHARTS_LIB
    tabs->addTab(createChartTab(models), "chart");
#endif
    tabs->setMinimumSize(QSize(500, 500));
    tabs->show();
}

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef QT_QUICK_LIB

    qmlRegisterType<QSortFilterProxyModel>("org.qtproject.example", 1, 0, "SortFilterProxyModel");
    qmlRegisterType<Fibonacci>("rust", 1, 0, "Fibonacci");
    qmlRegisterType<FibonacciList>("rust", 1, 0, "FibonacciList");

#endif

    Models models;
    Tree& model = models.tree;
    QSortFilterProxyModel& sortedModel = models.sortedTree;
    model.setPath("/");
    sortedModel.setSourceModel(&model);
    sortedModel.setDynamicSortFilter(true);

    createWidgets(&models);
    return app.exec();
}
