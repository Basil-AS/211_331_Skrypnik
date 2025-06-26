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

    openFileButton = new QPushButton("Открыть", this);
    layout->addWidget(openFileButton);

    setCentralWidget(centralWidget);

    connect(changePinButton, &QPushButton::clicked, this, &MainWindow::changePin);
    connect(openFileButton, &QPushButton::clicked, this, &MainWindow::openFile);

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
    QStringList transactions;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            transactions.append(line);
        }
    }
    file.close();

    // проверка целостности данных
    QVector<bool> validityFlags;
    verifyTransactionIntegrity(transactions, validityFlags);
    
    // формирование простого текста для отображения
    QString displayText = "=== ТРАНЗАКЦИИ ===\n\n";
    
    for (int i = 0; i < transactions.size(); ++i) {
        QStringList fields = transactions[i].split(',');
        if (fields.size() >= 4) {
            QString timeStr = QDateTime::fromSecsSinceEpoch(fields[2].toLongLong()).toString("dd.MM.yyyy hh:mm");
            
            bool isValid = (i < validityFlags.size()) ? validityFlags[i] : false;
            
            displayText += QString("Номер карты: %1\n").arg(fields[0]);
            displayText += QString("Маршрут: %1\n").arg(fields[1]);
            displayText += QString("Время: %1\n").arg(timeStr);
            displayText += QString("Хеш MD5: %1\n").arg(fields[3]);
            if (!isValid) {
                displayText += "⚠ НАРУШЕНА ЦЕЛОСТНОСТЬ ДАННЫХ!\n";
            }
            displayText += "-----------------------------------\n";
        }
    }
    
    textEdit_transactions->setPlainText(displayText);
    
    // выделение красным цветом нарушенных записей
    QTextCursor cursor = textEdit_transactions->textCursor();
    cursor.movePosition(QTextCursor::Start);
    
    for (int i = 0; i < transactions.size(); ++i) {
        bool isValid = (i < validityFlags.size()) ? validityFlags[i] : false;
        
        if (!isValid) {
            // поиск блока транзакции и выделение красным
            cursor = textEdit_transactions->document()->find("Номер карты:", cursor);
            if (!cursor.isNull()) {
                cursor.movePosition(QTextCursor::StartOfLine);
                for (int j = 0; j < 5; ++j) {
                    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                    cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
                }
                
                QTextCharFormat format;
                format.setForeground(QColor(Qt::red));
                cursor.mergeCharFormat(format);
            }
        } else {
            cursor = textEdit_transactions->document()->find("Номер карты:", cursor);
            if (!cursor.isNull()) {
                cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 5);
            }
        }
    }
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

// проверка целостности данных транзакций
bool MainWindow::verifyTransactionIntegrity(const QStringList &transactions, QVector<bool> &validityFlags)
{
    validityFlags.clear();
    validityFlags.reserve(transactions.size());
    
    QString previousHash = "";
    bool allValid = true;
    bool foundInvalid = false;
    
    for (int i = 0; i < transactions.size(); ++i) {
        QStringList fields = transactions[i].split(',');
        if (fields.size() < 4) {
            validityFlags.append(false);
            allValid = false;
            foundInvalid = true;
            continue;
        }
        
        QString cardNumber = fields[0];
        QString route = fields[1];
        QString timestamp = fields[2];
        QString storedHash = fields[3];
        
        bool isValid = true;
        
        // если уже найдена нарушенная запись, все последующие тоже считаются нарушенными
        if (!foundInvalid) {
            // формируем строку для хеширования согласно формуле
            QString dataToHash = cardNumber + route + timestamp + previousHash;
            
            // вычисляем MD5 хеш
            QByteArray hash = QCryptographicHash::hash(dataToHash.toUtf8(), QCryptographicHash::Md5);
            QString calculatedHash = hash.toHex();
            
            // проверяем соответствие
            isValid = (calculatedHash.toLower() == storedHash.toLower());
            
            if (!isValid) {
                foundInvalid = true;
                allValid = false;
            }
        } else {
            isValid = false;
        }
        
        validityFlags.append(isValid);
        previousHash = storedHash;
    }
    
    return allValid;
}

// открытие файла через диалог
void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Выберите файл транзакций", "", "CSV файлы (*.csv)");
    
    if (fileName.isEmpty())
        return;
        
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл!");
        return;
    }

    QTextStream in(&file);
    QStringList transactions;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            transactions.append(line);
        }
    }
    file.close();

    // проверка целостности данных
    QVector<bool> validityFlags;
    verifyTransactionIntegrity(transactions, validityFlags);
    
    // формирование простого текста для отображения
    QString displayText = "=== ТРАНЗАКЦИИ ===\n\n";
    
    for (int i = 0; i < transactions.size(); ++i) {
        QStringList fields = transactions[i].split(',');
        if (fields.size() >= 4) {
            QString timeStr = QDateTime::fromSecsSinceEpoch(fields[2].toLongLong()).toString("dd.MM.yyyy hh:mm");
            
            bool isValid = (i < validityFlags.size()) ? validityFlags[i] : false;
            
            displayText += QString("Номер карты: %1\n").arg(fields[0]);
            displayText += QString("Маршрут: %1\n").arg(fields[1]);
            displayText += QString("Время: %1\n").arg(timeStr);
            displayText += QString("Хеш MD5: %1\n").arg(fields[3]);
            if (!isValid) {
                displayText += "⚠ НАРУШЕНА ЦЕЛОСТНОСТЬ ДАННЫХ!\n";
            }
            displayText += "-----------------------------------\n";
        }
    }
    
    textEdit_transactions->setPlainText(displayText);
    
    // выделение красным цветом нарушенных записей
    QTextCursor cursor = textEdit_transactions->textCursor();
    cursor.movePosition(QTextCursor::Start);
    
    for (int i = 0; i < transactions.size(); ++i) {
        bool isValid = (i < validityFlags.size()) ? validityFlags[i] : false;
        
        if (!isValid) {
            // поиск блока транзакции и выделение красным
            cursor = textEdit_transactions->document()->find("Номер карты:", cursor);
            if (!cursor.isNull()) {
                cursor.movePosition(QTextCursor::StartOfLine);
                for (int j = 0; j < 5; ++j) {
                    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                    cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
                }
                
                QTextCharFormat format;
                format.setForeground(QColor(Qt::red));
                cursor.mergeCharFormat(format);
            }
        } else {
            cursor = textEdit_transactions->document()->find("Номер карты:", cursor);
            if (!cursor.isNull()) {
                cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 5);
            }
        }
    }
}
