#include "ClientMessage.h"
#include "LoginPage.h"
#include "ServerMessage.h"
#include <QtCore/QScopedPointer>
#include <QtNetwork/QTcpServer>
#include <QtTest/QtTest>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>

class ClientTests final : public QObject
{
    Q_OBJECT

private slots:
    void cleanup()
    {
        ClientConnection::Instance().DisconnectFromServer();
    }

    void fragmentedAndCoalescedResponses()
    {
        const QByteArray registration = QByteArray::fromHex("000000010001800401");
        const QByteArray login = QByteArray::fromHex("000000010001800100");
        ServerResponse response;
        QString error;
        for (int split = 0; split < registration.size(); ++split)
        {
            QByteArray buffer = registration.left(split);
            QCOMPARE(TakeServerResponse(buffer, response, error), ResponseReadResult::Incomplete);
            QCOMPARE(buffer, registration.left(split));
            buffer.append(registration.mid(split));
            buffer.append(login);
            QCOMPARE(TakeServerResponse(buffer, response, error), ResponseReadResult::Complete);
            QCOMPARE(response.type, CreateUserResponseType);
            QVERIFY(response.succeeded);
            QCOMPARE(buffer, login);
            QCOMPARE(TakeServerResponse(buffer, response, error), ResponseReadResult::Complete);
            QCOMPARE(response.type, LoginResponseType);
            QVERIFY(!response.succeeded);
            QVERIFY(buffer.isEmpty());
        }
    }

    void invalidResponse_data()
    {
        QTest::addColumn<QByteArray>("frame");
        QTest::newRow("version") << QByteArray::fromHex("000000010002800401");
        QTest::newRow("length") << QByteArray::fromHex("ffffffff00018004");
        QTest::newRow("type") << QByteArray::fromHex("000000010001800201");
        QTest::newRow("result") << QByteArray::fromHex("000000010001800402");
    }

    void invalidResponse()
    {
        QFETCH(QByteArray, frame);
        ServerResponse response;
        QString error;
        QCOMPARE(TakeServerResponse(frame, response, error), ResponseReadResult::Invalid);
        QVERIFY(!error.isEmpty());
    }

    void registrationAndLoginThroughUi()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        LoginPage page;
        page.show();
        auto* tabs = page.findChild<QTabWidget*>("authTabs");
        auto* port = page.findChild<QSpinBox*>("serverPort");
        auto* status = page.findChild<QLabel*>("requestStatus");
        auto* loginAccount = page.findChild<QLineEdit*>("loginAccount");
        auto* loginPassword = page.findChild<QLineEdit*>("loginPassword");
        auto* registerPassword = page.findChild<QLineEdit*>("registerPassword");
        if (qEnvironmentVariableIsSet("CHAT_CLIENT_PREVIEW"))
            QVERIFY(page.grab().save("login-preview.png"));
        port->setValue(server.serverPort());
        tabs->setCurrentIndex(1);
        if (qEnvironmentVariableIsSet("CHAT_CLIENT_PREVIEW"))
            QVERIFY(page.grab().save("register-preview.png"));
        page.findChild<QLineEdit*>("registerAccount")->setText("alice");
        page.findChild<QLineEdit*>("registerNickname")->setText(QStringLiteral("测试用户"));
        registerPassword->setText("pass1234");
        page.findChild<QLineEdit*>("confirmPassword")->setText("different");
        page.findChild<QPushButton*>("registerButton")->click();
        QVERIFY(status->text().contains(QStringLiteral("不一致")));
        QVERIFY(!ClientConnection::Instance().IsBusy());

        page.findChild<QLineEdit*>("confirmPassword")->setText("pass1234");
        page.findChild<QPushButton*>("registerButton")->click();
        QVERIFY(!tabs->isEnabled());
        QTRY_VERIFY(server.hasPendingConnections());
        QScopedPointer<QTcpSocket> peer(server.nextPendingConnection());
        QByteArray received;
        connect(peer.data(), &QTcpSocket::readyRead, &page,
            [&]() { received.append(peer->readAll()); });
        received.append(peer->readAll());
        const QByteArray expectedRegistration = QByteArray::fromHex(
            "0000001f000100040005616c69636500087061737331323334000ce6b58be8af95e794a8e688b7");
        QTRY_COMPARE(received, expectedRegistration);
        SendCreateUserRequest();
        QTest::qWait(25);
        QCOMPARE(received, expectedRegistration);

        // Deliberately split the header and body across separate TCP writes.
        peer->write(QByteArray::fromHex("0000000100"));
        peer->flush();
        QTest::qWait(25);
        QVERIFY(ClientConnection::Instance().IsBusy());
        peer->write(QByteArray::fromHex("01800401"));
        peer->flush();
        QTRY_VERIFY(!ClientConnection::Instance().IsBusy());
        QCOMPARE(tabs->currentIndex(), 0);
        QCOMPARE(loginAccount->text(), QStringLiteral("alice"));
        QVERIFY(registerPassword->text().isEmpty());

        received.clear();
        loginPassword->setText("wrong123");
        page.findChild<QPushButton*>("loginButton")->click();
        QTRY_COMPARE(received, QByteArray::fromHex(
            "00000011000100010005616c696365000877726f6e67313233"));
        peer->write(QByteArray::fromHex("000000010001800100"));
        peer->flush();
        QTRY_VERIFY(!ClientConnection::Instance().IsBusy());
        QVERIFY(status->text().contains(QStringLiteral("登录失败")));
        QVERIFY(tabs->isVisible());

        received.clear();
        loginPassword->setText("pass1234");
        page.findChild<QPushButton*>("loginButton")->click();
        QTRY_COMPARE(received, QByteArray::fromHex(
            "00000011000100010005616c69636500087061737331323334"));
        peer->write(QByteArray::fromHex("000000010001800101"));
        peer->flush();
        QTRY_VERIFY(page.findChild<QWidget*>("loggedInPage")->isVisible());
        if (qEnvironmentVariableIsSet("CHAT_CLIENT_PREVIEW"))
            QVERIFY(page.grab().save("success-preview.png"));
        QVERIFY(page.findChild<QLabel*>("currentAccount")->text().contains("alice"));
        QVERIFY(loginPassword->text().isEmpty());
        QVERIFY(ClientConnection::Instance().IsConnected());
        page.findChild<QPushButton*>("returnToLoginButton")->click();
        QVERIFY(!ClientConnection::Instance().IsConnected());
        QVERIFY(tabs->isVisible());
        QVERIFY(port->isEnabled());
    }

    void invalidInput_data()
    {
        QTest::addColumn<QString>("account");
        QTest::addColumn<QString>("password");
        QTest::addColumn<QString>("nickname");
        QTest::newRow("empty-account") << QString() << QString("pass1234") << QString("tester");
        QTest::newRow("account-characters") << QString("bad account") << QString("pass1234") << QString("tester");
        QTest::newRow("short-password") << QString("alice") << QString("1234567") << QString("tester");
        QTest::newRow("long-password") << QString("alice") << QString(1025, QChar('x')) << QString("tester");
        QTest::newRow("empty-nickname") << QString("alice") << QString("pass1234") << QString();
        QTest::newRow("utf8-nickname-length") << QString("alice") << QString("pass1234") << QString(43, QChar(0x4e2d));
    }

    void invalidInput()
    {
        QFETCH(QString, account);
        QFETCH(QString, password);
        QFETCH(QString, nickname);
        auto& client = ClientConnection::Instance();
        QSignalSpy messages(&client, &ClientConnection::StatusMessage);
        client.SetUserInput(account, password, nickname);
        SendCreateUserRequest();
        QVERIFY(!client.IsBusy());
        QVERIFY(!client.IsConnected());
        QCOMPARE(messages.size(), 1);
        QVERIFY(messages.last().at(1).toBool());
    }

    void utf8LoginAndMismatchedResponse()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        auto& client = ClientConnection::Instance();
        QVERIFY(client.SetServer("127.0.0.1", server.serverPort()));
        client.SetUserInput("test", QStringLiteral("密码1234"));
        SendLoginRequest();
        QTRY_VERIFY(server.hasPendingConnections());
        QScopedPointer<QTcpSocket> peer(server.nextPendingConnection());
        QTRY_VERIFY(peer->bytesAvailable() > 0);
        QCOMPARE(peer->readAll(), QByteArray::fromHex(
            "0000001200010001000474657374000ae5af86e7a08131323334"));
        peer->write(QByteArray::fromHex("000000010001800401"));
        peer->flush();
        QTRY_VERIFY(!client.IsConnected());
        QVERIFY(!client.IsBusy());
    }

    void disconnectAndReconnect()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        auto& client = ClientConnection::Instance();
        QVERIFY(client.SetServer("127.0.0.1", server.serverPort()));
        client.SetUserInput("alice", "pass1234");
        SendLoginRequest();
        QTRY_VERIFY(server.hasPendingConnections());
        QScopedPointer<QTcpSocket> peer(server.nextPendingConnection());
        peer->abort();
        QTRY_VERIFY(!client.IsBusy());
        QVERIFY(!client.IsConnected());
        client.SetUserInput("alice", "pass1234");
        SendLoginRequest();
        QTRY_VERIFY(server.hasPendingConnections());
        peer.reset(server.nextPendingConnection());
        QTRY_VERIFY(peer->bytesAvailable() > 0);
        peer->readAll();
        peer->write(QByteArray::fromHex("000000010001800100"));
        peer->flush();
        QTRY_VERIFY(!client.IsBusy());
        QVERIFY(client.IsConnected());
    }

    void responseTimeout()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        auto& client = ClientConnection::Instance();
        QVERIFY(client.SetServer("127.0.0.1", server.serverPort()));
        QSignalSpy messages(&client, &ClientConnection::StatusMessage);
        client.SetUserInput("alice", "pass1234");
        SendLoginRequest();
        QTRY_VERIFY(server.hasPendingConnections());
        QScopedPointer<QTcpSocket> peer(server.nextPendingConnection());
        QTest::qWait(10500);
        QVERIFY(!client.IsBusy());
        QVERIFY(!client.IsConnected());
        QVERIFY(messages.last().at(0).toString().contains(QStringLiteral("超时")));
        QVERIFY(messages.last().at(1).toBool());
    }
};

QTEST_MAIN(ClientTests)
#include "ClientTests.moc"
