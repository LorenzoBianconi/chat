#include <QtGui/QApplication>
#include "qchat.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qChat w;
    w.show();
    
    return a.exec();
}
