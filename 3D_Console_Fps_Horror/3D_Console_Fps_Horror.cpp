#include <iostream>
#include <Windows.h>
#include <chrono> 
#include <string>
#include <vector>
#include <algorithm>

struct Vector
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	Vector()
	{
		this->x = 0.f;
		this->y = 0.f;
		this->z = 0.f;
	}

	Vector(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	double length()
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	Vector operator - (Vector v)
	{
		return Vector(this->x - v.x, this->y - v.y, this->z - v.z);
	}

	std::string str()
	{
		return "[" + std::to_string(x) + ", " + std::to_string(y) + "]";
	}
};

struct globals
{
	
	int screen_width = 120;
	int screen_height = 40;
	// player vars
	Vector loc = Vector(0,0,0);	// xyz location of player (origin)
	Vector cam; // xyz location of camera (eye position)
	Vector ang; // for now we only use x/y, no roll involved (x is pitch, y is yaw, z is roll)
	float fov = 3.1415926 / 2;
}g;

int main()
{
	const auto hout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hout, &csbi);

	// Bind x and y
	float wx = csbi.dwMaximumWindowSize.X;
	float wy = csbi.dwMaximumWindowSize.Y;

	g.screen_width = wx;
	g.screen_height = wy;

	wchar_t* screen = new wchar_t[g.screen_width * g.screen_height];
	HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(console);
	DWORD bytes_written = 0;

	std::string map =
			// N
		"##########################################"
		"#   #                                    #"
		"#   #     ############################   #"
		"#   #     #                              #"
		"#   #     #                              #"
		"#   #     #                              #"
		"#   #     #                              #"
  /*W*/	"#   #     #         #######              #"	// E
		"#   #     #         #######              #"
		"#   #     #         #######              #"
		"#   #     #                              #"
		"#   #     #                              #"
		"#   #     #                              #"
		"#         #                              #"
		"#         #                              #"
		"##########################################";
			//  S

	// player starts by looking straight south
	int map_height = 16;
	int map_width = 42;

	g.loc = Vector(3, 1, 0);

	// Using time point and system_clock
	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	end = std::chrono::system_clock::now();
	

	while (1)
	{
		start = std::chrono::system_clock::now();
		
		std::chrono::duration<float> elapsed = end - start;
		end = std::chrono::system_clock::now();
		
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			g.ang.y -= 5 * 0.75 * (float)elapsed.count();
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			g.ang.y += 5 * 0.75 * (float)elapsed.count();

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			g.loc.x += sinf(g.ang.y) * 5.f * (float)elapsed.count();;
			g.loc.y += cosf(g.ang.y) * 5.f * (float)elapsed.count();;
			if (map.c_str()[(int)g.loc.x * map_width + (int)g.loc.y] == '#')
			{
				g.loc.x -= sinf(g.ang.y) * 5.f * (float)elapsed.count();;
				g.loc.y -= cosf(g.ang.y) * 5.f * (float)elapsed.count();;
			}
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			g.loc.x -= sinf(g.ang.y) * 5.f * (float)elapsed.count();;
			g.loc.y -= cosf(g.ang.y) * 5.f * (float)elapsed.count();;
			if (map.c_str()[(int)g.loc.x * map_width + (int)g.loc.y] == '#')
			{
				g.loc.x += sinf(g.ang.y) * 5.f * (float)elapsed.count();;
				g.loc.y += cosf(g.ang.y) * 5.f * (float)elapsed.count();;
			}
		}

		for (int y = 0; y < g.screen_height; y++)
		{
			for (int x = 0; x < g.screen_width; x++)
			{
				// calculate eye position
				g.cam = g.loc;
				g.cam.z += 10;

				// plan is to shoot rays out from orign, and determine distance, initially just using horizontal, eventually will use vertical
				float rayAngYaw = (g.ang.y - g.fov / 2.f) + ((float)x / (float)g.screen_width * g.fov);
				float rayAngPitch = (g.ang.x - g.fov / 2.f) + ((float)y / (float)g.screen_height * g.fov); // still calculate it though

				// the horizontal scanning 
				Vector EyeYaw = Vector(sinf(rayAngYaw), cosf(rayAngYaw), 0);

				bool hitWall = false;
				bool hitEdge = false;
				float rayDist = 0.f;
				while (!hitWall && rayDist < max(map_height, map_width)) // at most we can see end of map so y render farther
				{
					rayDist += 0.1f;

					// map is currently in integer space
					int testX = (int)(g.cam.y + EyeYaw.y * rayDist);
					int testY = (int)(g.cam.x + EyeYaw.x * rayDist);

					// hit edge of map
					if (testX < 0 || testY < 0 || testX >= map_width || testY >= map_height)
					{
						hitWall = true;
						rayDist = max(map_width, map_height);
					}
					else
					{
						// checks and sees if we have hit a wall
						if (map[testY * map_width + testX] == '#')
						{
							hitWall = true;
							// okay we have hit the wall, lets detect if it's the "edge" of a wall

							std::vector<std::pair<float, float>> p; // distance, dot
							for (int tx = 0; tx < 2; tx++)
							{
								for (int ty = 0; ty < 2; ty++)
								{
									float vx = (float)testX + tx - g.loc.x;
									float vy = (float)testY + ty - g.loc.y;
									float dist = sqrt(vx * vx + vy * vy);
									// dot producte between ray casted unitvector, and unit vector of perfect corner 
									float dot = (EyeYaw.x * vx / dist) + (EyeYaw.y * vy / dist);
									p.push_back(std::make_pair(dist, dot));
								}
							}

							// do big dumb sorting
							std::sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) {return left.first < right.first; });

							float fBound = 0.08;
							if (acos(p.at(0).second) < fBound) hitEdge = true;
							if (acos(p.at(1).second) < fBound) hitEdge = true;
							//if (acos(p.at(2).second) < fBound) hitEdge = true;
						}
					}
				}

				int ceiling = (float)(g.screen_height / 2.f) - g.screen_height / ((float)rayDist);
				int floor = g.screen_height - ceiling;

				float max = max(map_height, map_width);
				if (y < ceiling)
					screen[y * g.screen_width + x] = ' ';
				else if (y > ceiling && y <= floor)
				{
					if (rayDist < max / 4.f)
						screen[y * g.screen_width + x] = '#';
					else if (rayDist < max / 3.f)
						screen[y * g.screen_width + x] = '$';
					else if (rayDist < max / 2.f)
						screen[y * g.screen_width + x] = '%';
					else if (rayDist < max)
						screen[y * g.screen_width + x] = '^';

					if (hitEdge)
						screen[y * g.screen_width + x] = ' ';
				}
				else 
					screen[y * g.screen_width + x] = '-';

				// removing top line of screen to put ton of bad debug code
				/*if (y == 0)
				{
					float fang = g.ang.y;
					fang *= 180 / 3.1415926;
					while (fang > 360.f)
						fang -= 360;
					while (fang < 0.f)
						fang += 360;
					std::string ang = std::to_string(fang) + " " + g.loc.str();
					int len = ang.length();
					const char* c_str = ang.c_str();
					if (x < len)
						screen[y * g.screen_width + x] = c_str[x];
					else
						screen[y * g.screen_width + x] = ' ';
						
				}*/
			}
		}
		// Display Map
		for (int nx = 0; nx < map_width; nx++)
			for (int ny = 0; ny < map_height; ny++)
			{
				screen[(ny + 1) * g.screen_width + nx] = map[ny * map_width + nx];
			}
		screen[((int)g.loc.x + 1) * g.screen_width + (int)g.loc.y] = 'P';

		screen[g.screen_width * g.screen_height - 1] = '\0';
		WriteConsoleOutputCharacter(console, screen, g.screen_width * g.screen_height, { 0,0 }, &bytes_written);
	}
	int x = 1;
	int y = 1;
	
}
