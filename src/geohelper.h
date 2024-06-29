#ifndef GEOHELPER_H
#define GEOHELPER_H
#include <string>
#include <vector>

struct GeoRect
{
    float minLat, maxLat, minLon, maxLon;
    std::string continent;
    std::string server;
};

namespace geohelper
{

    std::string getContinentServer(float lat, float lon)
    {
        std::vector<GeoRect> continents = {
            {-56.0, 13.0, -81.0, -34.0, "South America","soam.aprs2.net"},
            {24.0, 71.0, -168.0, -52.0, "North America","noam.aprs2.net"},
            {-35.0, 37.0, -17.0, 51.0, "Africa","euro.aprs2.net"},
            {34.0, 72.0, -10.0, 45.0, "Europe","euro.aprs2.net"},
            {-10.0, 55.0, 110.0, 155.0, "Australia","aunz.aprs2.net"},
            {1.0, 25.0, 101.0, 123.0, "Southeast Asia","aunz.aprs2.net"},
            {6.0, 35.0, 60.0, 102.0, "Middle East","euro.aprs2.net"},
            {-50.0, -10.0, 110.0, 155.0, "Oceania","aunz.aprs2.net"},
            {-90.0, -60.0, -180.0, 180.0, "Antarctica",""},
            {26.0, 70.0, 32.0, 180.0, "Asia","asia.aprs2.net"}};

        for (const auto &rect : continents)
        {
            if (lat >= rect.minLat && lat <= rect.maxLat && lon >= rect.minLon && lon <= rect.maxLon)
            {
                return rect.server;
            }
        }
        return "rotate.aprs.net";
    }

}

#endif