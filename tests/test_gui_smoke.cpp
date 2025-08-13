
#include <QtTest/QtTest>
#include <QApplication>
#include "gui/MainWindow.h"

class GuiSmoke : public QObject {
  Q_OBJECT
private slots:
  void createAndShow(){
    int argc=0; char** argv=nullptr;
    // Note: in ctest add env QT_QPA_PLATFORM=offscreen
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    QVERIFY(w.findChild<QTableWidget*>("tableResults") != nullptr);
  }
};
QTEST_MAIN(GuiSmoke)
#include "test_gui_smoke.moc"
