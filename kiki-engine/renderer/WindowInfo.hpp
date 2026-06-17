#ifndef KIKI_RENDERER_WINDOWINFO
#define KIKI_RENDERER_WINDOWINFO

#include <filesystem>

namespace Kiki {
    /**
     * @brief Variables used to create GLFW window.
     * 
     * @param width width of the window, set to 0 to set to monitor size
     * @param height height of the window, set to 0 to set to monitor size
     * @param resizeable whether or not the window can be resized by the user
     * @param fullscreen create the window in fullscreen mode
     * @param monitor the monitor the application should appear on (fullscreen only)
     * @param title the application's title
     * @param icon path to the icon to use for the application
     * @param decorations enable/disable window decorations
     */
    struct WindowInfo {
        int width = 0;
        int height = 0;
        bool resizeable = false;
        bool fullscreen = true;
        int monitor = 0;
        const char *title = "kiki";
        std::filesystem::path icon;
        bool decorations = true; 
    };
}

#endif