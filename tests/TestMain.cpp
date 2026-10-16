#include <QTest>
#include <QApplication>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    int status = 0;

    auto runSuite = [&status, argc, argv](QObject* testObject, const char* name) {
        std::cout << "\n========================================\n";
        std::cout << " Running Suite: " << name << "\n";
        std::cout << "========================================\n";
        QString logFile = QStringLiteral("%1_report.txt").arg(name);
        QStringList args = { QString::fromUtf8(argv[0]), QStringLiteral("-o"), QStringLiteral("%1,txt").arg(logFile), QStringLiteral("-v1") };
        int result = QTest::qExec(testObject, args);
        std::cout << " Finished Suite: " << name << " with code: " << result << "\n";
        if (result != 0) {
            status = result;
        }
        delete testObject;
    };

    // Instantiate and run each test suite
    extern QObject* createGeometryTests();
    extern QObject* createDocumentTests();
    extern QObject* createCommandTests();
    extern QObject* createFileIoTests();
    extern QObject* createFunctionalE2ETest();
    extern QObject* createPhase2Tests();
    extern QObject* createPhase3Tests();
    extern QObject* createPhase4Tests();
    extern QObject* createCadDimensionWorkflowTests();
    extern QObject* createAiFloorPlanTests();

    runSuite(createGeometryTests(), "GeometryTests");
    runSuite(createDocumentTests(), "DocumentTests");
    runSuite(createCommandTests(), "CommandTests");
    runSuite(createFileIoTests(), "FileIoTests");
    runSuite(createFunctionalE2ETest(), "FunctionalE2ETest");
    runSuite(createPhase2Tests(), "Phase2Tests");
    runSuite(createPhase3Tests(), "Phase3Tests");
    runSuite(createPhase4Tests(), "Phase4Tests");
    runSuite(createCadDimensionWorkflowTests(), "CadDimensionWorkflowTests");
    runSuite(createAiFloorPlanTests(), "AiFloorPlanTests");

    return status;
}
