#include "TelescopeGL.hh"
#include <SFML/Window.hpp>

#include <chrono>
#include <thread>
#include <iostream>

int main(){
  TelescopeGL tel;
  tel.addTelLayer(0, 0, 0,   1, 0, 0, 0.028, 0.026, 1.0, 1024, 512);
  tel.addTelLayer(0, 0, 30,  0, 1, 0, 0.028, 0.026, 1.0, 1024, 512);
  tel.addTelLayer(0, 0, 60,  0, 0, 1, 0.028, 0.026, 1.0, 1024, 512);
  tel.addTelLayer(0, 0, 120, 1, 1, 0, 0.028, 0.026, 1.0, 1024, 512);
  tel.addTelLayer(0, 0, 150, 0, 1, 1, 0.028, 0.026, 1.0, 1024, 512);
  tel.addTelLayer(0, 0, 180, 1, 0, 1, 0.028, 0.026, 1.0, 1024, 512);
  
  tel.buildProgramTel();
  tel.buildProgramHit();
  
  bool running = true;
  while (running)
  {
    sf::Event windowEvent;
    while (tel.m_window->pollEvent(windowEvent))
    {
      switch (windowEvent.type)
      {
      case sf::Event::Closed:
        running = false;
        break;
      }  
    }
    tel.clearHit();
    tel.addHit(100, 100, 0);
    tel.addHit(200, 200, 1);
    tel.addHit(300, 300, 2);
    tel.addHit(500, 100, 3);
    tel.addHit(500, 250, 4);
    tel.addHit(500, 400, 5);
    
    tel.clearFrame();
    tel.drawTel();
    tel.drawHit();
    tel.flushFrame();    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
