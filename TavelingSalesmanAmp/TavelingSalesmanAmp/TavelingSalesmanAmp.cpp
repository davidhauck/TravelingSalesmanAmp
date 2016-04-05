// Started by: David Hauck
//12/1/13

#include "stdafx.h"
#include <amp.h>
#include <amp_math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <omp.h>

using namespace std;
using namespace concurrency;

const int numCities = 11;
float _lats[numCities];
float _longs[numCities];

long _permutations = 1;


float PathFromRoutePermutation(int permutations, index<1> permutation, int cities, int paths[1][numCities], int pathIndex) restrict(amp)
{
    for (int i = 0; i < cities; i++)
    {
        paths[pathIndex][i] = i;
    }

    for (int remaining = cities, divisor = permutations; remaining > 0; remaining--) // Credit: SpaceRat. http://www.daniweb.com/software-development/cpp/code/274075/all-permutations-non-recursive
    {
        divisor /= remaining;
        int index = (permutation[0] / divisor) % remaining;

        int swap = paths[pathIndex][index];
        paths[pathIndex][index] = paths[pathIndex][remaining - 1];
        paths[pathIndex][remaining - 1] = swap;
    }

    return 0;
}

float square(float value) restrict(amp)
{
	return value * value;
}

float FindPathDistance(int permutations, index<1> permutation, int cities, array_view<float, 1> latitudes, array_view<float, 1> longitudes, int paths[1][numCities], int pathIndex) restrict(amp)
{
	PathFromRoutePermutation(permutations, permutation, cities, paths, pathIndex);

    float distance = 0;
    int city = paths[pathIndex][0];
    float previousLatitude = latitudes[city];
    float previousLongitude = longitudes[city];

    for (int i = 1; i < cities; i++)
    {
        city = paths[pathIndex][i];
        float latitude = latitudes[city];
        float longitude = longitudes[city];
		distance += (float)fast_math::sqrt(square(latitude - previousLatitude) + square(longitude - previousLongitude));
        previousLatitude = latitude;
        previousLongitude = longitude;
    }

    return distance;
}

int _tmain(int argc, _TCHAR* argv[])
{
	string line;
	ifstream myfile ("C:\\Users\\david\\Documents\\Projects\\TavelingSalesmanAmp\\cities.txt");
	if (myfile.is_open())
	{
		for ( int i = 0; getline (myfile,line) && i < numCities; i++ )
		{
			string buf;
			stringstream ss(line);
			vector<string> tokens;

			while (ss >> buf)
				tokens.push_back(buf);

			_lats[i] = atof(tokens[1].c_str());
			_longs[i] = atof(tokens[2].c_str());
		}
		myfile.close();

		for (int i = 2; i <= numCities; i++)
        {
            _permutations *= i;
        }
	}
	else
	{
		std::cout << "file doesnt exist";
		return -1;
	}
	
	unsigned int start = clock();
	float * distances = (float*)malloc(sizeof(float) * _permutations);
	
	std::cout << "Permutations: " << _permutations << endl;

	int np[] = {_permutations};
	array_view<int, 1> num_permutations(1, np);
	array_view<float, 1> lats(numCities, _lats);
	array_view<float, 1> longs(numCities, _longs);
	try
	{
		array_view<float, 1> gpudistances(_permutations, distances);
	
		parallel_for_each(gpudistances.extent, [=](index<1> idx) restrict(amp)
			{
				int path[1][numCities];
				gpudistances[idx] = FindPathDistance(num_permutations[0], idx, numCities, lats, longs, path, 0);
			});
		float bestDistance = INT_MAX;
    int bestPermutation = -1;

	gpudistances.synchronize(); //copies the gpu array back into the cpu array

	//find the best distance from all of the distances.
	for (int i = 0; i < _permutations; i++)
	{
		if (distances[i] < bestDistance)
		{
			bestDistance = distances[i];
			bestPermutation = i;
		}
	}
	unsigned int stop = clock();


    std::cout << "Best Distance: " << bestDistance << endl;
	std::cout << "Best Permutation: " << bestPermutation << endl;
	std::cout << "Time taken: " << stop - start << endl;
	string key;
	std::cin >> key;
	}
	catch(runtime_exception& ex)
	{
		std::cout<< ex.what() << std::endl;
	string key;
	std::cin >> key;
	}
	delete [] distances;
	
	return 0;
}