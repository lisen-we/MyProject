#pragma once

#include <QtWidgets/QWidget>
#include <memory>

class ClientConnection;

namespace Ui
{
    class LoginPage;
}

class LoginPage final : public QWidget
{
public:
    explicit LoginPage(QWidget* parent = nullptr);
    ~LoginPage() override;

private:
    void HandleSignIn();
    void HandleSignUp();
    void RefreshConnectionState();
    void ShowStatus(const QString& message, bool isError);

    std::unique_ptr<Ui::LoginPage> ui_;
    ClientConnection& connection_;
    QString pendingAccount_;
};
