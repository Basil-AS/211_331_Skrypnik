#include "mainwindow.h"
#include "ui_mainwindow.h"

// создание главного окна
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    setWindowTitle("Система контроля транзакций - 211_331_Skrypnik");

    if (!checkPin()) {
        QMessageBox::critical(this, "Ошибка", "Неверный пин-код!");
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        return;
    }

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    textEdit_transactions = new QTextEdit(this);
    textEdit_transactions->setReadOnly(true);
    layout->addWidget(textEdit_transactions);

    changePinButton = new QPushButton("Сменить пин-код", this);
    layout->addWidget(changePinButton);

    setCentralWidget(centralWidget);

    connect(changePinButton, &QPushButton::clicked, this, &MainWindow::changePin);

    loadTransactions();
}

// очистка 
MainWindow::~MainWindow()
{
    delete ui;
}

// чтение транзакций 
void MainWindow::loadTransactions()
{
    QFile file("transactions.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Внимание", "Файл с транзакциями не найден!");
        return;
    }

    QTextStream in(&file);
    QString displayText = "=== ТРАНЗАКЦИИ ===\n\n";
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        
        QStringList fields = line.split(',');
        if (fields.size() >= 4) {
            QString timeStr = QDateTime::fromSecsSinceEpoch(fields[2].toLongLong()).toString("dd.MM.yyyy hh:mm");
            displayText += QString("Номер карты: %1\n").arg(fields[0]);
            displayText += QString("Маршрут: %2\n").arg(fields[1]);
            displayText += QString("Время: %3\n").arg(timeStr);
            displayText += QString("Хеш MD5: %4\n").arg(fields[3]);
            displayText += "---\n";
        }
    }
    
    textEdit_transactions->setText(displayText);
    file.close();
}

// проверка пинкода
bool MainWindow::checkPin()
{
    storedPinHash = loadPinHash();

    if (storedPinHash.isEmpty()) {
        bool ok;
        QString pin = QInputDialog::getText(this, "Создание пин-кода", "Придумайте новый пин-код:", QLineEdit::Password, "", &ok);
        if (ok && !pin.isEmpty()) {
            storedPinHash = QCryptographicHash::hash(pin.toUtf8(), QCryptographicHash::Md5);
            savePinHash(storedPinHash);
            return true;
        }
        return false;
    }

    bool ok;
    QString pin = QInputDialog::getText(this, "Ввод пин-кода", "Введите пин-код:", QLineEdit::Password, "", &ok);
    if (ok && !pin.isEmpty()) {
        return QCryptographicHash::hash(pin.toUtf8(), QCryptographicHash::Md5) == storedPinHash;
    }
    return false;
}

// смена пинкода
void MainWindow::changePin()
{
    bool ok;
    QString oldPin = QInputDialog::getText(this, "Смена пин-кода", "Введите старый пин-код:", QLineEdit::Password, "", &ok);
    if (!ok || oldPin.isEmpty())
        return;

    if (QCryptographicHash::hash(oldPin.toUtf8(), QCryptographicHash::Md5) != storedPinHash) {
        QMessageBox::critical(this, "Ошибка", "Старый пин-код неверный!");
        return;
    }

    QString newPin = QInputDialog::getText(this, "Смена пин-кода", "Введите новый пин-код:", QLineEdit::Password, "", &ok);
    if (ok && !newPin.isEmpty()) {
        storedPinHash = QCryptographicHash::hash(newPin.toUtf8(), QCryptographicHash::Md5);
        savePinHash(storedPinHash);
        QMessageBox::information(this, "Успех", "Пин-код успешно изменен.");
    }
}

void MainWindow::savePinHash(const QByteArray &hash)
{
    QFile file("pin.bin");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(hash);
        file.close();
    }
}

QByteArray MainWindow::loadPinHash()
{
    QFile file("pin.bin");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray hash = file.readAll();
        file.close();
        return hash;
    }
    return QByteArray();
}
