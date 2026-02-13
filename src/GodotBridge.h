#ifndef GODOTBRIDGE_H
#define GODOTBRIDGE_H

#include <QObject>
#include <QProcess>
#include <QWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>

class GodotBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QWindow* godotWindow READ godotWindow NOTIFY godotWindowChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Status {
        Idle,
        Launching,
        WaitingForConnection,
        Connected,
        Error
    };
    Q_ENUM(Status)

    explicit GodotBridge(QObject* parent = nullptr);
    ~GodotBridge();

    QWindow* godotWindow() const { return m_godotWindow; }
    bool isConnected() const { return m_status == Connected; }
    Status status() const { return m_status; }

    // Called from QML
    Q_INVOKABLE void launch(const QString& projectPath);
    Q_INVOKABLE void shutdown();
    Q_INVOKABLE void sendCommand(const QString& cmd, const QJsonObject& params);

signals:
    void godotWindowChanged();
    void connectedChanged();
    void statusChanged();
    void messageReceived(const QString& event, const QJsonObject& data);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onDataReady();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setStatus(Status s);
    void handleMessage(const QJsonObject& msg);
    void embedWindow(qint64 windowHandle);

    QProcess* m_process = nullptr;
    QTcpServer* m_server = nullptr;
    QTcpSocket* m_socket = nullptr;
    QWindow* m_godotWindow = nullptr;
    Status m_status = Idle;
    QByteArray m_readBuffer;

    static constexpr quint16 IPC_PORT = 9876;
};

#endif // GODOTBRIDGE_H
