#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QInputDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// основное окно
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void changePin();

private:
    void loadTransactions();
    bool checkPin();
    void savePinHash(const QByteArray &hash);
    QByteArray loadPinHash();

    Ui::MainWindow *ui;
    QTextEdit *textEdit_transactions;
    QLineEdit *pinLineEdit;
    QPushButton *changePinButton;
    QByteArray storedPinHash;
};

#endif // MAINWINDOW_H
