// ============================================================
// BuildLens — C++ 编译性能观测工具
//
// main.cpp
// 功能：程序入口，初始化 QApplication 并启动主窗口。
// ============================================================

#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  // 设置应用元信息，影响 QSettings 存储路径等。
  QApplication::setApplicationName(QStringLiteral("BuildLens"));
  QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
  QApplication::setOrganizationName(QStringLiteral("BuildLens"));

  MainWindow window;
  window.show();

  return app.exec();
}
