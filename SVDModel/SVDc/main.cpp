#include <QCoreApplication>

#include <QTimer>
#include "../SVDUI/version.h"
#include "consoleshell.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("SVD console (%s - #%s)\n", currentVersion(), gitVersion());
    printf("This is the console version of SVD, the Scaling Vegetation Dynamics model.\n");
    printf("More at: https://svdmodel.github.io/SVD \n");
    printf("(c) Werner Rammer, Rupert Seidl, 2019- \n");
    printf("version: %s\n", verboseVersion().toLocal8Bit().data());
    printf("****************************************\n\n");
    if (a.arguments().count()<3) {
        printf("Usage: \n");
        printf("SVDc.exe <project-file> <years> <...other options>\n");
        printf("Options:\n");
        printf("you specify a number key=value pairs, and *after* loading of the project\n");
        printf("the 'key' settings are set to 'value'. E.g.: ilandc project.xml 100 output.stand.enabled=false output.stand.landscape=false\n");
        printf("See also http://iland.boku.ac.at/iLand+console\n.");
        return 0;
    }
    ConsoleShell svd_shell;

    QTimer::singleShot(0, &svd_shell, SLOT(run()));
    //a.installEventFilter(&iland_shell);
    return a.exec();

}
