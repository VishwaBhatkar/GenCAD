#pragma once

#include <QApplication>
#include <memory>

class MainWindow;

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application() override;

    int run();

private:
    void setupAppearance();

    std::unique_ptr<MainWindow> m_mainWindow;
};
