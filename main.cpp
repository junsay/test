// Justin Junsay, 171144
// Patricia Vianne Lee, 171234
// May X, 2022 |||CHANGE THIS|||

// I/we certify that this submission complies with the DISCS Academic Integrity
// Policy.

// If I/we have discussed my/our C++ language code with anyone other than
// my/our instructor(s), my/our groupmate(s), the teaching assistant(s),
// the extent of each discussion has been clearly noted along with a proper
// citation in the comments of my/our program.

// If any C++ language code or documentation used in my/our program
// was obtained from another source, either modified or unmodified, such as a
// textbook, website, or another individual, the extent of its use has been
// clearly noted along with a proper citation in the comments of my/our program.

#include <SFML/Graphics.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#define FPS 60

using namespace std;

// create AABBs
sf::RectangleShape getBoundingBox(sf::RectangleShape polygon)
{
	sf::Vector2f minimum, maximum;

	for (int i = 0; i < polygon.getPointCount(); i++) 
	{
		sf::Vector2f globalCoords = polygon.getPosition() + polygon.getPoint(i); // use global coordinates

		if (i == 0) // initial values
		{ 
			minimum = globalCoords;
			maximum = globalCoords;
		}
		else 
		{
			minimum.x = min(minimum.x, globalCoords.x);
			minimum.y = min(minimum.y, globalCoords.y);
			maximum.x = max(maximum.x, globalCoords.x);
			maximum.y = max(maximum.y, globalCoords.y);
		}
	}

	// build the bounding box and return it
	sf::RectangleShape result;
	result.setPosition(minimum.x, minimum.y);
	result.setSize(sf::Vector2f(maximum.x - minimum.x, maximum.y - minimum.y));
	result.setOrigin(result.getSize() / 2.0f);

	return result;
}

bool aabbCheck(sf::RectangleShape boundingBoxA, sf::RectangleShape boundingBoxB)
{
	sf::Vector2f minA, maxA, minB, maxB;

	// get the min max of A
	minA.x = boundingBoxA.getPosition().x - (boundingBoxA.getSize().x / 2); // subtract 'size / 2' from the position to compensate for the centered origin of the bounding boxes
	minA.y = boundingBoxA.getPosition().y - (boundingBoxA.getSize().y / 2);
	maxA.x = minA.x + boundingBoxA.getSize().x;
	maxA.y = minA.y + boundingBoxA.getSize().y;

	// get the min and max of B
	minB.x = boundingBoxB.getPosition().x - (boundingBoxB.getSize().x / 2);
	minB.y = boundingBoxB.getPosition().y - (boundingBoxB.getSize().y / 2);
	maxB.x = minB.x + boundingBoxB.getSize().x;
	maxB.y = minB.y + boundingBoxB.getSize().y;

	// check if they overlap
	// should be false if the min coord of one is greater than the max coord of the other (no intersection)
	return (minA.x <= maxB.x && maxA.x >= minB.x) && (minA.y <= maxB.y && maxA.y >= minB.y);
}

int main() 
{
	// slides set states that for best results, use 800 x 600. So that is what we used
	sf::RenderWindow window(sf::VideoMode(800, 600), "Platformer Camera"); // initialize the game window

	vector<string> propertyVars = { "H_ACCEL", "H_COEFF", "H_OPPOSITE", "H_AIR", "MIN_H_VEL", "MAX_H_VEL",
	"GRAVITY", "V_ACCEL", "V_HOLD", "V_SAFE", "CUT_V_VEL", "MAX_V_VEL", "GAP", "CAM_EDGES", "CAM_DRIFT" };

	map<string, float> properties; // a map which holds all of the properties and their values
	vector<float> cam_edges; // vector to store cam edge values

	// parse properties file
	ifstream propertiesFile("properties.txt"); 

	if (propertiesFile.fail()) 
		cout << "Cannot locate properties.txt, program will not work properly" << endl;
	else
	{
		cout << "properties.txt loaded successfully" << endl;
		string line;
		while (getline(propertiesFile, line))
		{
			vector<string> split; // vector which would hold the property name and its value
			istringstream iss(line);
			string word;
			while (iss >> word)
				split.push_back(word); // splits each line of the properties file based on the whitespace

			if (split[0] == "CAM_EDGES") 
			{
				for (int i = 1; i < split.size(); i++)
					cam_edges.push_back(stof(split[i]));
			}

			properties.insert(pair<string, float>(split[0], stof(split[1]))); // insert the splitted property into the map
		}

		// check if the map contains all of the required property variables based on propertyVars
		for (int i = 0; i < propertyVars.size(); i++)
		{
			if (properties.find(propertyVars[i]) == properties.end())
				throw runtime_error("Missing " + propertyVars[i] + " variable from properties file");
		}
		propertiesFile.close();
	}
	
	// parse level data 
	ifstream levelFile("stage.txt"); 

	// player 
	sf::Vector2f playerPos;
	sf::RectangleShape player(sf::Vector2f(24, 32));

	// walls
	int numWalls;
	std::vector<sf::RectangleShape> walls;

	if (levelFile.fail()) 
		cout << "Cannot locate stage.txt, program will not work properly" << endl;

	else 
	{
		cout << "stage.txt loaded successfully" << endl;
		// parse and initialize the player position
		// specs: first line of input from the second file contains the player position
		levelFile >> playerPos.x >> playerPos.y;
		player.setPosition(playerPos);
		player.setFillColor(sf::Color::Cyan);
		player.setOrigin(12, 16);

		// parse and initialize the walls of the stage
		// specs: second line containts the number of walls
		levelFile >> numWalls;

		if (numWalls < 0) // check if the input is valid
			throw range_error("Number of Walls is less than 0");

		walls.resize(numWalls);
		for (int i = 0; i < numWalls; i++)
		{
			sf::Vector2f position;
			sf::Vector2f size;
			// N lines are the walls: position, width, and height
			levelFile >> position.x >> position.y >> size.x >> size.y;
			walls[i].setPosition(position);
			walls[i].setSize(size);
			walls[i].setOrigin(size / 2.0f);
			walls[i].setFillColor(sf::Color(128, 128, 128));
		}
		levelFile.close();
	}

	// floats for "current" velocity and acceleration of the player
	float player_vel_h = 0, player_vel_v = 0, player_accel_h = 0, player_accel_v = 0;

	// create the bounding boxes of the player and the walls
	std::vector<sf::RectangleShape> bounds;
	bounds.resize(numWalls + 1);
	bounds[numWalls] = getBoundingBox(player); // last element contains the bounding box of the player
	// remaining elements contain the bounding boxes of the walls
	for (int i = 0; i < numWalls; i++)
		bounds[i] = getBoundingBox(walls[i]);

	// variables for jumping
	bool grounded = false; // boolean if a player is on the ground
	int hold_counter = 0;  // counter to determine the number of frames a jump has elapsed
	int safe_counter = 0;  // counter to determine the number of frames that a player has been falling for

	window.setFramerateLimit(FPS);

	sf::View view = window.getDefaultView(); // use the default view of the windows as a base for the camera

	while (window.isOpen()) 
	{
		sf::Event event;
		while (window.pollEvent(event)) 
		{
			// check triggered window's events since the last iteration of the loop
			switch (event.type) 
			{ 
			case sf::Event::Closed: // "close requested" event
				window.close();
				break;
			}
		}

		// detecting what movement keys are pressed along with applying the respective acceleration
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			player_accel_v = properties["V_ACCEL"];
		else
			player_accel_v = 0; // set acceleration to 0 if no key is pressed

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			player_accel_h = -properties["H_ACCEL"];
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			player_accel_h = properties["H_ACCEL"];
		else
			player_accel_h = 0;

		// applying horizontal movement
		float baseAccel = player_accel_h / FPS;

		// modify baseAccel if the player is... 
		// ... moving in the opposite direction (Apply base acceleration times an adjustment factor greater than one)
		if ((sf::Keyboard::isKeyPressed(sf::Keyboard::A) && player_vel_h > 0) || (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && player_vel_h < 0))
			baseAccel *= properties["H_OPPOSITE"];
		// ... or if the player is in the air
		if (!grounded)
			baseAccel *= properties["H_AIR"];

		// apply horizontal acceleration to horizontal velocity
		player_vel_h += baseAccel; // apply acceleration first before clamping velocity (the code for capping the horizontal speed found below)

		// cap the horizontal speed of the player as movement within a single frame should not exceed a % of the player character size
		float maxSpeed = properties["MAX_H_VEL"] / FPS;
		// if moving forward
		if (player_vel_h >= maxSpeed) 
			player_vel_h = maxSpeed;
		// if moving backward
		else if (player_vel_h <= -maxSpeed)
			player_vel_h = -maxSpeed;

		// slow the player down if there is no input (not pressing A or D)
		if (!sf::Keyboard::isKeyPressed(sf::Keyboard::A) && !sf::Keyboard::isKeyPressed(sf::Keyboard::D)) 
		{
			// multiply current velocity with slowdown coefficient
			player_vel_h *= properties["H_COEFF"]; 
			// if the absolute value of the player's velocity is less than the provided "minimum horizontal velocity", set the player velocity to 0 
			if (abs(player_vel_h) < properties["MIN_H_VEL"])
				player_vel_h = 0; 
		}

		player.move(player_vel_h, 0); // move the player horizontally

		// check for collision from the sides
		bounds[numWalls] = getBoundingBox(player); // update the player bounding box after horizontal movement
		for (int i = 0; i < numWalls; i++) 
		{
			// check for collision between the player and all walls
			if (aabbCheck(bounds[numWalls], bounds[i])) 
			{ 
				// collision from the left of the player
				if (player_vel_h < 0) 
				{ 
					float min_x_player = bounds[numWalls].getPosition().x - (bounds[numWalls].getSize().x / 2); 
					float max_x_wall = bounds[i].getPosition().x + (bounds[i].getSize().x / 2);
					// reset the player position after a collision based on how much the player and wall overlap plus a small gap/allowance
					player.move((max_x_wall - min_x_player) + properties["GAP"], 0); 
				}
				// collision from the right of the player
				else if (player_vel_h > 0) 
				{ 
					float max_x_player = bounds[numWalls].getPosition().x + (bounds[numWalls].getSize().x / 2);
					float min_x_wall = bounds[i].getPosition().x - (bounds[i].getSize().x / 2);
					player.move((min_x_wall - max_x_player) - properties["GAP"], 0);
				}
				player_vel_h = 0; // set horizontal velocity to 0 after a collision from the sides
			}
		}

		// apply vertical movement
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && ((grounded || safe_counter < properties["V_SAFE"]) && hold_counter < properties["V_HOLD"])) 
		{
			baseAccel = player_accel_v / FPS; // only apply jump acceleration if (1) there is jump input, (2) the player is grounded or has not exceeded the V_SAFE limit, and (3) the V_HOLD limit has not yet been exceeded
			hold_counter++; // increment the hold counter when a jump has been registered
		}
		else
			baseAccel = properties["GRAVITY"] / FPS; // apply gravity instead if there is no jump input

		player_vel_v += baseAccel; // apply vertical acceleration to vertical velocity

		// cap the falling speed of the player
		maxSpeed = properties["MAX_V_VEL"] / FPS;
		if (player_vel_v >= maxSpeed)
			player_vel_v = maxSpeed;

		// allow the player to cut their jumps short
		if (!sf::Keyboard::isKeyPressed(sf::Keyboard::W) && player_vel_v < properties["CUT_V_VEL"] / FPS)
			player_vel_v = properties["CUT_V_VEL"] / FPS; // cut the jump if there is no jump input and the player's upward velocity exceeds the cut threshold

		player.move(0, player_vel_v); // move the player vertically

		// check collisions from the top and bottom
		bounds[numWalls] = getBoundingBox(player); // update the player bounding box after vertical movement
		grounded = false; // reset grounded flag to false before checking for vertical collision
		for (int i = 0; i < numWalls; i++) 
		{
			if (aabbCheck(bounds[numWalls], bounds[i])) 
			{
				// collision from the top of the player
				if (player_vel_v < 0) 
				{ 
					float min_y_player = bounds[numWalls].getPosition().y - (bounds[numWalls].getSize().y / 2);
					float max_y_wall = bounds[i].getPosition().y + (bounds[i].getSize().y / 2);
					player.move(0, (max_y_wall - min_y_player) + properties["GAP"]);
				}
				// collision from the bottom of the player
				else if (player_vel_v > 0) 
				{ 
					float max_y_player = bounds[numWalls].getPosition().y + (bounds[numWalls].getSize().y / 2);
					float min_y_wall = bounds[i].getPosition().y - (bounds[i].getSize().y / 2);
					player.move(0, (min_y_wall - max_y_player) - properties["GAP"]);
					grounded = true; // set grounded flag to true once a collision on the bottom has been registered
				}
				player_vel_v = 0; // set horizontal velocity to 0 after a collision from the top or bottom
			}
		}
		// reset the jump counters when the player has hit the ground
		if (grounded) 
		{ 
			safe_counter = 0;
			hold_counter = 0;
		}
		else
			safe_counter++; // else, increment the safe counter when the player is falling

		// draw the current frame
		window.clear(sf::Color::Black);

		sf::Vector2f center = view.getCenter();
		playerPos = player.getPosition();

		// draw the camera edges 
		sf::RectangleShape border(sf::Vector2f(abs(cam_edges[2] - cam_edges[0]), abs(cam_edges[3] - cam_edges[1])));
		border.setPosition(center.x + cam_edges[0], center.y + cam_edges[1]);
		border.setFillColor(sf::Color::Transparent);
		border.setOutlineColor(sf::Color::White);
		border.setOutlineThickness(-1.0f);
		window.draw(border);

		// camera-window implementation
		if (playerPos.x - (player.getSize().x / 2) < border.getPosition().x) // checks if the left side of the player has gone past the left camera edge
			center.x -= border.getPosition().x - (playerPos.x - (player.getSize().x / 2)); // moves the camera center to the left by how much the left side of the player has gone past the left camera edge

		else if (playerPos.x + (player.getSize().x / 2) > border.getPosition().x + border.getSize().x) // checks if the right side of the player has gone past the right camera edge
			center.x += (playerPos.x + (player.getSize().x / 2)) - (border.getPosition().x + border.getSize().x); // moves the camera center to the right by how much the right side of the player has gone past right camera edge

		// same concept above but now checks the top and bottom edges
		if (playerPos.y - (player.getSize().y / 2) < border.getPosition().y) 
			center.y -= border.getPosition().y - (playerPos.y - (player.getSize().y / 2));

		else if (playerPos.y + (player.getSize().y / 2) > border.getPosition().y + border.getSize().y)
			center.y += (playerPos.y + (player.getSize().y / 2)) - (border.getPosition().y + border.getSize().y);

		///////////////////////// INSERT "position-snapping (CAM_TYPE 3) implementation, cam drift stuff" HERE////////////////////////////

		view.setCenter(center); // apply the modified center to the view

		window.setView(view); // update the view of the window

		window.draw(player);
		for (int i = 0; i < numWalls; i++)
			window.draw(walls[i]);

		window.display();
	}
	return 0;
}