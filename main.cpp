#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>

void change_theme(QApplication& ctx, const char* filename)
{
    QFile file(filename);
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    ctx.setStyleSheet(stream.readAll());
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    change_theme(a, "style.qss");

    MainWindow w;
    w.show();
    return a.exec();
}
