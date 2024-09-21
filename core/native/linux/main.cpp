#include <gtkmm.h>

int main()
{
    auto app = Gtk::Application::create();
    Gtk::Window window;

    window.set_default_size(800, 600);
    window.show_all(); // for some widgets (I don't remember which) show() is not enough
    return app->run(window);
}