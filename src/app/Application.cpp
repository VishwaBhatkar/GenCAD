#include "app/Application.h"
#include "ui/MainWindow.h"
#include <QIcon>
#include <QPalette>
#include <QColor>
#include <QTimer>

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setApplicationName(QStringLiteral("GenCAD"));
    setApplicationDisplayName(QStringLiteral("GenCAD 2D"));
    setOrganizationName(QStringLiteral("GenCAD"));
    setApplicationVersion(QStringLiteral("1.0.0"));

    setupAppearance();

    m_mainWindow = std::make_unique<MainWindow>();
}

Application::~Application() = default;

void Application::setupAppearance() {
    setWindowIcon(QIcon(QStringLiteral(":/icons/select.svg")));

    // Professional dark fusion style
    setStyle(QStringLiteral("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::WindowText, QColor(240, 240, 240));
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Text, QColor(240, 240, 240));
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ButtonText, QColor(240, 240, 240));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(0, 122, 204));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 122, 204));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    setPalette(darkPalette);
}

int Application::run() {
    m_mainWindow->show();
    QTimer::singleShot(100, m_mainWindow.get(), &MainWindow::showPageSetup);
    return exec();
}
