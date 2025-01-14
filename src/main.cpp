#include <iostream>
#include <vector>
#include <cmath>
#include <unistd.h>

#include "SDL.h"
#include "SDLauxiliary.h"

#include <glm/glm.hpp>

#include "tile.h"
#include "rng_utility.h"
#include "l_system.h"
#include "road_network.h"

using namespace std;
using glm::ivec2;

// ***** GLOBAL VARIABLE *****
Tile map[MAP_SIZE][MAP_SIZE];
SDL_Surface* screen;

//Road parameter
const int min_seg_length = 25;
const int max_seg_length = 40;
const int max_road_size = 4;

//Algorithm parameters (in pixel)
const int DEFAULT_SUB_SQUARE_SIZE = 50;
const double DEFAULT_MAX_DISTANCE = 30;

// ***** FUNCTION DECLARATION *****
void display_map(SDL_Surface* screen, Tile map[MAP_SIZE][MAP_SIZE]);
void generate_partial_network(vector<ivec2> positions, Road_Network &network, int road_size, const double max_distance);
void main_algorithm(int sub_square_size, Road_Network &network, const double max_distance);

int main(int argc, char** argv)
{
    string density_map_path = "../maps/density/dmap.png";
    string height_map_path  = "../maps/height/hmap.png";

    int sub_square_size = DEFAULT_SUB_SQUARE_SIZE;
    double max_distance = DEFAULT_MAX_DISTANCE;

    cout << "Program " << argv[0] << endl;
    if(argc <= 1){
        cout << "Using default map paths " << endl;
    }

    int opt;
    uintmax_t num;
    while ( (opt = getopt(argc, argv, "d:h:s:m:")) != -1 ) {
        switch ( opt ) {
            case 'd':
                density_map_path = optarg;
                break;
            case 'h':
                height_map_path = optarg;
                break;
            case 's':
                num = strtoumax(optarg, NULL, 10);
                if (num == UINTMAX_MAX && errno == ERANGE){
                    cerr << "Error, -s cannot parse the specified number.." << endl;
                    exit(0);
                }
                sub_square_size = (int) num;
                break;
            case 'm':
                sscanf(optarg, "%lf", &max_distance);
                break;
            case '?':
                cerr << "Only use -d <density map path> or -h <heigh map path>. Option " << char(optopt) << " is not recognised." << endl;
                exit(0);
                break;
        }
    }

    cout << "density map: " << density_map_path << endl;
    cout << "height map:  " << height_map_path << endl;
    cout << "sub_square_size:  " << sub_square_size << endl;
    cout << "max_distance :  " << max_distance << endl;

    //screen = InitializeSDL( MAP_SIZE, MAP_SIZE );

    readPng(map, density_map_path.c_str(), height_map_path.c_str());

    //Starting position correspond to an entrance point of the city.
    ivec2 start_position(0, 350);

    Node n;
    n.p = start_position;
    n.road_size = 3;

    Road_Network network(n,(min_seg_length+max_seg_length)/4);

    main_algorithm(sub_square_size, network, max_distance);

    network.mark_road_network(map);

    /*while( NoQuitMessageSDL() ){
        display_map(screen,map);
    }*/

    writeToPng(map, "output.png");

    return 0;
}

/**
 * Display the map on the screen using SDL
 * @brief display_map
 * @param screen
 * @param map
 */
void display_map(SDL_Surface* screen, Tile map[MAP_SIZE][MAP_SIZE]){
    if( SDL_MUSTLOCK(screen) )
        SDL_LockSurface(screen);

    for (int x = 0; x < MAP_SIZE; ++x) {
        for (int y = 0; y < MAP_SIZE; ++y) {
            Tile t = map[x][y];

            if(t.is_road){
                PutPixelSDL( screen, x, y, glm::vec3(0,0,0));
            }else if(!t.is_accessible){
                PutPixelSDL( screen, x, y, glm::vec3(0.8,0.8,0.85));
            }else{
                PutPixelSDL( screen, x, y, glm::vec3(t.height,t.density,0.5));
            }
        }
    }

    if( SDL_MUSTLOCK(screen) )
        SDL_UnlockSurface(screen);
    SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

/**
 * @brief generate_partial_network
 * @param positions
 * @param network
 * @param road_size
 */
void generate_partial_network(vector<ivec2> positions, Road_Network &network, int road_size, const double max_distance){

    for (unsigned int i = 0; i < positions.size(); ++i) {
        ivec2 end = positions[i];
        ivec2 start = network.find_closest_node(end, road_size,map);

        while(distance(start, end) > max_distance){
            int lenght = distance(start, end);
            double angle = find_angle(start, end);

            int nb_iter =  ceil(log((double)lenght / (max_seg_length + min_seg_length)));

            if(nb_iter > 5){
                nb_iter = 5;
            }else if(nb_iter <= 0){
                nb_iter = 1;
            }

            nb_iter = ceil((double)nb_iter*1.5);

            cout << "start : (" << start.x <<", " << start.y << ")" << endl;
            cout << "end : (" << end.x <<", " << end.y << ")" << endl;
            cout << "L_system -- length: " << lenght << ", angle: " << angle << ", nb_iteration: " << nb_iter << endl;

            L_system sys2(start,angle,min_seg_length,max_seg_length,nb_iter,road_size);
            sys2.generate_network(map, network);

            start = network.find_closest_node(end, road_size,map);
        }
    }
}

/**
 * @brief main_algorithm
 * @param sub_square_size
 * @param network
 */
void main_algorithm(int sub_square_size, Road_Network &network, const double max_distance){
    vector<pair<ivec2, double> > s_map = get_subdivided_map(map, sub_square_size);
    double max_pop = s_map[0].second;

    //Generate a road network of size 3 for area where the population is greater than 60% of max_pop
    unsigned int i = 0;

    for(int size = 3; size > 0; --size){
        double percent = 0.6;
        vector<ivec2> positions;
        while(i < s_map.size() && s_map[i].second > percent*max_pop){
            positions.push_back(s_map[i].first);
            i++;
        }

        generate_partial_network(positions, network, size, max_distance);
        positions.clear();
        percent -= 0.2;
    }

}
