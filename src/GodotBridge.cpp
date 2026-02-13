#include "GodotBridge.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QDebug>

GodotBridge::GodotBridge(QObject* parent)
    : QObject(parent)
{
}

GodotBridge::~GodotBridge() {
    shutdown();
}

void GodotBridge::launch(const QString& projectPath) {
    if (m_status != Idle) {
        qWarning() << "GodotBridge: Already launched";
        return;
    }

    // --- Step 1: Start the TCP server BEFORE launching Godot ---
    // This way Godot can connect immediately on startup.

    m_server = new QTcpServer(this);

    if (!m_server->listen(QHostAddress::LocalHost, IPC_PORT)) {
        emit errorOccurred("Failed to start IPC server: "
                           + m_server->errorString());
        setStatus(Error);
        return;
    }

    connect(m_server, &QTcpServer::newConnection,
            this, &GodotBridge::onNewConnection);

    // --- Step 2: Launch Godot ---

    m_process = new QProcess(this);

    connect(m_process, &QProcess::finished,
            this, &GodotBridge::onProcessFinished);

    // Arguments for Godot 4
    QStringList args = {
        "--path", projectPath,
        // Pass IPC port so Godot knows where to connect
        // "--" separates Godot's args from your custom args
        // Your Godot project reads these from OS.get_cmdline_user_args()
        "--", QString("--ipc-port=%1").arg(IPC_PORT)
    };

    m_process->start("godot", args);
    setStatus(Launching);

    if (!m_process->waitForStarted(5000)) {
        emit errorOccurred("Failed to start Godot: "
                           + m_process->errorString());
        setStatus(Error);
    } else {
        setStatus(WaitingForConnection);
    }
}

void GodotBridge::shutdown() {
    if (m_socket) {
        // Tell Godot to close gracefully
        sendCommand("shutdown", {});
        m_socket->flush();
        m_socket->disconnectFromHost();
        m_socket = nullptr;
    }

    if (m_process) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();  // Force kill if it doesn't respond
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }

    if (m_godotWindow) {
        m_godotWindow->deleteLater();
        m_godotWindow = nullptr;
        emit godotWindowChanged();
    }

    setStatus(Idle);
}

void GodotBridge::sendCommand(const QString& cmd, const QJsonObject& params) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "GodotBridge: Not connected";
        return;
    }

    QJsonObject msg;
    msg["cmd"] = cmd;
    msg["params"] = params;

    // Newline-delimited JSON: one message per line
    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    data.append('\n');  // Message delimiter

    m_socket->write(data);
}

void GodotBridge::onNewConnection() {
    m_socket = m_server->nextPendingConnection();

    if (!m_socket) return;

    connect(m_socket, &QTcpSocket::readyRead,
            this, &GodotBridge::onDataReady);

    qDebug() << "Godot connected to IPC";
    // Don't set Connected yet -- wait for the "ready" message
    // with the window handle
}

void GodotBridge::onDataReady() {
    // Accumulate data in buffer
    m_readBuffer.append(m_socket->readAll());

    // Process complete lines (newline-delimited JSON)
    while (true) {
        int newlineIdx = m_readBuffer.indexOf('\n');
        if (newlineIdx < 0) break;  // No complete message yet

        QByteArray line = m_readBuffer.left(newlineIdx);
        m_readBuffer.remove(0, newlineIdx + 1);

        if (line.isEmpty()) continue;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "GodotBridge: Invalid JSON:" << parseError.errorString();
            continue;
        }

        handleMessage(doc.object());
    }
}

void GodotBridge::handleMessage(const QJsonObject& msg) {
    QString event = msg["event"].toString();

    if (event == "ready") {
        // Godot is ready and sending us its window handle
        qint64 handle = msg["window_handle"].toInteger();
        embedWindow(handle);
        setStatus(Connected);
        emit connectedChanged();
    }

    // Forward all messages to QML
    emit messageReceived(event, msg);
}

void GodotBridge::embedWindow(qint64 windowHandle) {
    // QWindow::fromWinId wraps a platform-native window handle
    // On Windows: HWND, on X11: Window (XID), on Wayland: not supported
    m_godotWindow = QWindow::fromWinId(static_cast<WId>(windowHandle));

    if (!m_godotWindow) {
        emit errorOccurred("Failed to create QWindow from handle");
        return;
    }

    qDebug() << "Embedded Godot window:" << windowHandle;
    emit godotWindowChanged();
}

void GodotBridge::onProcessFinished(int exitCode,
                                     QProcess::ExitStatus exitStatus) {
    qDebug() << "Godot process exited:" << exitCode << exitStatus;

    if (m_status == Connected) {
        // Unexpected exit
        emit errorOccurred("Godot process terminated unexpectedly");
    }

    // Clean up
    m_godotWindow = nullptr;
    emit godotWindowChanged();
    setStatus(Idle);
}

void GodotBridge::setStatus(Status s) {
    if (m_status != s) {
        m_status = s;
        emit statusChanged();
    }
}
