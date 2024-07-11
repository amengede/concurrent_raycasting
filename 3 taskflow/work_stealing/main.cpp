#include "config.h"
#include "game_app.h"

int main() {
	GameApp* myApp = new GameApp(800,600);
	delete myApp;
	return 0;
}