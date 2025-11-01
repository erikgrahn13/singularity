#include "../StandaloneHelper.h"
#include "singularity_Webview.h"
#include <sys/wait.h>

int main(int argc, char **argv)
{
    auto controller = createControllerInstance();

    // Pass nullptr for standalone mode - WebViewLinux will create its own GtkWindow
    StandaloneHelper::initializeView(controller.get(), nullptr);

    // Wait for child process to exit (blocks here until window is closed)
    int status;
    wait(&status);

    return 0;
}
