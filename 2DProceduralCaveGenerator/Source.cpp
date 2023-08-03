#include "SFML/Graphics.hpp"
#include "FastNoiseLite.h"
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

#pragma region TWEAKING AREA   //Change the values inside here to experiment

#define WALL_SYMBOL "#"
#define AIR_SYMBOL " "
#define SEED rand()                     //Set random seed
#define WALKER_COUNT 100                
#define WALKER_MIN_LIFE 300             
#define WALKER_MAX_LIFE 1000            
#define PERLIN_LIMIT 0.1                //If perlin noise is less than this value the cell turns into air
#define PERLIN_ZOOM 6                   //Perlin noise zoom level
#define PERLIN_FREQ .2                  //Perlin noise frequency
#define CELLULAR_CHANCE 45              //%45
#define CELLULAR_DEATH_LIMIT 3          //If an alive cells neighbor count is more than this, the cell dies
#define CELLULAR_BIRTH_LIMIT 5          //If a dead cells neighbor count is more than this, the cell comes alive
#define CELLULAR_ITERATION 100          //This is how many times the rules will be applied to the map array
#define WINDOW_SIZE 900                 
#define NOISE_WINDOW_TEXTURE_SCALE 1    /*How much the drawn sprite will be scaled to draw on the window
                                        (sometimes the noise texture is so small to see so we can manually change this to see better)
                                        */

#pragma endregion TWEAKING AREA

enum gen_type
{
    walker = 1,     //random walking
    perlin = 2,     //perlin noise
    cellular = 3,   //cellular automation
};

int* map_array;
int map_size;
int grid_cell_size;
gen_type genTechnique;

void displayMapConsole()
{
    for (int i = 0;i < map_size;i++)
    {
        for (int j = 0;j < map_size;j++)
        {
            if (*(map_array + (i * map_size) + j) == 1)
            {
                //Wall
                std::cout << WALL_SYMBOL << " ";
            }
            else
            {
                //AIR
                std::cout << AIR_SYMBOL << " ";
            }
        }
        std::cout << endl;
    }
    std::cout << endl;
}

void resetMapArray()
{
#ifdef map_array
    delete[] map_array;
#endif
    map_array = new int[map_size * map_size];
    for (int i = 0;i < map_size;i++)
    {
        for (int j = 0;j < map_size;j++)
        {
            *(map_array + (i * map_size) + j) = 1;  //Fill the map with walls (1 = wall, 0 = air)
        }
    }
}

int getMapSize()
{
    //Console input for Map Size
    std::cout << "Map Size: ";
    int _size{};
    std::cout << endl;
    std::cin >> _size;
    return _size;
}

int getGenTechnique()
{
    //Console input for Generation Type
    int _gen_index;
    std::cout << "Select Generation Type:\n"
        "(1) - Random Walker\n"
        "(2) - Perlin Noise\n"
        "(3) - Cellular Automata";
    std::cout << endl;
    std::cin >> _gen_index;
    return _gen_index;
}

class Walker
{
public:
    int x;
    int y;
    int life;
};
Walker walker_object[5000]; //Maximum of "5000" walkers;

void applyWalker(int _walker_count, int min_life, int max_life)
{
    //Setup walkers
    for (int w = 0;w < _walker_count;w++)
    {
        walker_object[w].x = map_size / 2;    //Set x position of walker in map
        walker_object[w].y = map_size / 2;    //Set y position of walker in map
        walker_object[w].life = min_life + (rand() % (max_life - min_life));  //Set life of the walker between min life and max life

        for (int l = 0;l < walker_object[w].life;l++)
        {
            int _dir = round(rand() % 4);
            switch (_dir)
            {
                case 0:
                    walker_object[w].x += 1;
                break;
                case 1:
                    walker_object[w].x -= 1;
                    break;
                case 2:
                    walker_object[w].y += 1;
                    break;
                case 3:
                    walker_object[w].y -= 1;
                    break;
            }
            //Stay walker inside map
            if (walker_object[w].x > map_size)walker_object[w].x = 0;
            if (walker_object[w].x < 0)walker_object[w].x = map_size - 1;
            if (walker_object[w].y > map_size)walker_object[w].y = 0;
            if (walker_object[w].y < 0)walker_object[w].y = map_size - 1;

            *(map_array + (walker_object[w].x * map_size) + walker_object[w].y) = 0;
        }
    }
}

void applyPerlin()
{
    //Setup FastNoiseLite Library (this library contains perlin noise)
    FastNoiseLite perlinNoise;
    perlinNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    perlinNoise.SetFrequency(PERLIN_FREQ);
    perlinNoise.SetSeed(SEED);
    std::vector<float> noiseData(map_size * map_size);
    int noiseDataIndex{ 0 };

    for (int i = 0; i < map_size; i++)
    {
        for (int j = 0; j < map_size; j++)
        {
            noiseData[noiseDataIndex] = perlinNoise.GetNoise((float)i / PERLIN_ZOOM, (float)j / PERLIN_ZOOM);
            if (noiseData[noiseDataIndex] < PERLIN_LIMIT) {
                *(map_array + (i * map_size) + j) = 0;
            }
            noiseDataIndex++;
        }
    }
}

int cellularCountNeighbors(int* map, int x, int y)
{
    //Check all 8 neighbors around the cell if they are walls
    int _count{ 0 };
    for (int i = -1;i < 2;i++)
    {
        for (int j = -1;j < 2;j++)
        {
            int _neighbor_x = x + i;
            int _neighbor_y = y + j;

            if (i == 0 && j == 0) {

            }
            else if (_neighbor_x < 0 || _neighbor_y < 0 || _neighbor_x >= map_size || _neighbor_y >= map_size)
            {
                _count++;
            }
            else if (*(map + (_neighbor_x * map_size) + _neighbor_y))
            {
                _count++;
            }
        }
    }
    return _count;
}

int* cellularSimulationStep(int* _old_map_array)
{
    int* _new_map_array = new int[map_size * map_size];
    for (int i = 0;i < map_size; i++)
    {
        for (int j = 0;j < map_size;j++)
        {
            int neighbor_count = cellularCountNeighbors(_old_map_array, i, j);

            //Apply Rules
            if (*(_old_map_array + (i * map_size) + j))
            {
                if (neighbor_count < CELLULAR_DEATH_LIMIT)
                {
                    *(_new_map_array + (i * map_size) + j) = 0;
                }
                else
                {
                    *(_new_map_array + (i * map_size) + j) = 1;
                }
            }
            else
            {
                if (neighbor_count > CELLULAR_BIRTH_LIMIT)
                {
                    *(_new_map_array + (i * map_size) + j) = 1;
                }
                else
                {
                    *(_new_map_array + (i * map_size) + j) = 0;
                }
            }
        }
    }
    return _new_map_array;
}

void applyCellular()
{
    //Make Cells Air or Wall depending on the CELLULAR_CHANCE
    for (int i = 1;i < map_size - 1;i++)
    {
        for (int j = 1;j < map_size - 1;j++)
        {
            *(map_array + (i * map_size) + j) = (rand() % 100) > CELLULAR_CHANCE ? 0 : 1;
        }
    }

    //Simulate Algorithm Steps
    for (int i = 0;i < CELLULAR_ITERATION;i++)
    {
        map_array = cellularSimulationStep(map_array);
    }
}

void applyGenTechnique()
{
    switch (genTechnique)
    {
    case walker:
        std::cout << "Random Walker" << endl;

        applyWalker(WALKER_COUNT, WALKER_MIN_LIFE, WALKER_MAX_LIFE);
        break;

    case perlin:
        std::cout << "Perlin Noise" << endl;

        applyPerlin();
        break;

    case cellular:
        std::cout << "Cellular Automata" << endl;

        applyCellular();
        break;

    default:
        std::cout << "Wrong Generation Type, Exiting..." << endl;
        exit(0);
        break;
    }
}

int main()
{
    //Setup Graphic Window
    sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE, WINDOW_SIZE), "SFML works!");

    start:
    srand(SEED);                                    //Randomize Seed
    map_size = getMapSize();                        //Get map size input
    grid_cell_size = int(WINDOW_SIZE / map_size);   //Calculate cell size depending on the window size

    //Setup Pixel Buffer, Texture and Sprite
    sf::Uint8* pixels = new sf::Uint8[map_size*map_size*4];
    sf::Texture texture;
    texture.create(map_size,map_size);
    sf::Sprite sprite(texture);

    resetMapArray();                                            

    genTechnique = static_cast<gen_type>(getGenTechnique());    //Get generation type input

    applyGenTechnique();                                        //Apply the selected generation method on the map array

    //displayMapConsole();

    //Window Update
    while (window.isOpen())
    {
        window.clear();

        //Draw pixels on texture
        for (int x = 0; x < map_size; x++)
        {
            for (int y = 0; y < map_size; y++)
            {
                pixels[4 * (x * map_size + y)] = 0;         //R
                pixels[4 * (x * map_size + y) + 1] = 255;   //G
                pixels[4 * (x * map_size + y) + 2] = 0;     //B
                pixels[4 * (x * map_size + y) + 3] = *(map_array + (x * map_size + y)) ? 255 : 0;   //A
            }
        }

        //Update sprite and draw it on window
        texture.update(pixels);
        sprite.setScale(NOISE_WINDOW_TEXTURE_SCALE, NOISE_WINDOW_TEXTURE_SCALE);
        sprite.setPosition(((WINDOW_SIZE - map_size) / 2) - (map_size * (NOISE_WINDOW_TEXTURE_SCALE - 1)) / 2,((WINDOW_SIZE - map_size) / 2) - (map_size*(NOISE_WINDOW_TEXTURE_SCALE - 1)) / 2);
        window.draw(sprite);

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            //Restart Generation
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Enter)
                {
                    window.clear();
                    window.display();
                    system("cls");
                    goto start;
                }
            }
        }
        window.display();
    }

    system("pause");
    system("cls");
    goto start;
    return 0;
}