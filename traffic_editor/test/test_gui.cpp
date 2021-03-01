#include <QtWidgets>
#include <QTest>

#include "../gui/editor.h"

class TestGui : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase()
  {
    qInfo("initTestCase()");
    QApplication::setOrganizationName("open-robotics");
    QApplication::setOrganizationDomain("openrobotics.org");
    QApplication::setApplicationName("traffic-editor");
  }

  void ensureFileEquality(const QString& a_name, const QString& b_name)
  {
    QFile a(a_name), b(b_name);
    if (!a.open(QIODevice::ReadOnly | QIODevice::Text))
      QFAIL("couldn't open file");
    if (!b.open(QIODevice::ReadOnly | QIODevice::Text))
      QFAIL("couldn't open file");
    QString a_str(a.readAll()), b_str(b.readAll());
    QCOMPARE(a_str, b_str);
  }

  void testEmptyProject()
  {
    Editor editor;
    const QString app_path(QApplication::applicationDirPath());
    qInfo("creating empty project in %s", qUtf8Printable(app_path));
    editor.new_project(app_path + QString("/empty.project.yaml"));

    ensureFileEquality(
      app_path + "/empty.project.yaml",
      app_path + "/data/empty.project.yaml");

    ensureFileEquality(
      app_path + "/empty.building.yaml",
      app_path + "/data/empty.building.yaml");
  }

  void cleanupTestCase()
  {
    qInfo("cleanupTestCase()");
  }
};

QTEST_MAIN(TestGui)
#include "test_gui.moc"
