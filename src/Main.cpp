#include "Application.h"

int main(int argc, char** argv)
{
    Application app;
    app.setFPS(60);
    app.start();
    return 0;
}