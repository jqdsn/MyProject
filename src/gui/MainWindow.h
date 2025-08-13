#pragma once
#include <QMainWindow>
#include <QTableWidget>
class RoiView;

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent=nullptr);
  ~MainWindow();
private slots:
  void onOpen();
  void onRun();
  void onModeRect();
  void onModeRing();
  void onModePoly();
  void onClearRoi();
private:
  void appendResultRow(const QString& name, const QString& value, const QString& spec, bool ok);
  void clearResults();
  Ui::MainWindow* ui;
  RoiView* roiView_;
};
