#include <SFML/Window.hpp>

int main()
{
    // Create a window with a size of 800x600 and a title "SFML Window"
    sf::Window window(sf::VideoMode(800, 600), "SFML Window");

    // Main loop that continues until the window is closed
    while (window.isOpen())
    {
        // Create an event object to handle window events
        sf::Event event;

        // Poll for events (such as closing the window)
        while (window.pollEvent(event))
        {
            // Check if the window close button was pressed
            if (event.type == sf::Event::Closed)
            {
                window.close(); // Close the window
            }
        }

        // No rendering required since we are using sf::Window, not sf::RenderWindow
    }

    return 0;
}
