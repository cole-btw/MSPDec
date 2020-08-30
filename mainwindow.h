#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Windows.h>

#include <QMainWindow>

#include <string>
#include <cctype>
#include <regex>
#include <chrono>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // sends text to console, with format string
    void console_log(const char* format, ...);

private slots:

    void on_pushButton_clicked();

    void on_textEdit_cursorPositionChanged();

    void on_textEdit_textChanged();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
