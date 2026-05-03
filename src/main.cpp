#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    
    // Modern dark theme
    QPalette p = app.palette();
    p.setColor(QPalette::Window, QColor(15, 18, 26));
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Base, QColor(24, 28, 42));
    p.setColor(QPalette::AlternateBase, QColor(30, 35, 50));
    p.setColor(QPalette::ToolTipBase, Qt::white);
    p.setColor(QPalette::ToolTipText, Qt::white);
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Button, QColor(30, 35, 50));
    p.setColor(QPalette::ButtonText, Qt::white);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(42, 130, 218));
    p.setColor(QPalette::Highlight, QColor(42, 130, 218));
    p.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(p);
    
    MainWindow w;
    w.show();
    
    return app.exec();
}
