#ifndef DYNAMIC__FILTERING_H
#define DYNAMIC__FILTERING_H
#include <ArduinoJson.h>
#include "dynamic_filtering.h"
#include <digi_utils.h>

typedef struct
{
    String name = "";
    double lat = 0;
    double lon = 0;
    int range = 0;
} mh_group;
typedef struct
{
    String call = "";
    double lat = 0;
    double lon = 0;
    int group = 0;
    String raw = "";
    unsigned long timestamp = 0;
} mh_entry;


class dynamicfilter
{
public:
    dynamicfilter::dynamicfilter(String initialfilter, String config, bool backgroundmode = true)
    {
        this->inifilter = initialfilter;
        DeserializationError error = deserializeJson(this->config, config);
        this->throttle[this->config["throttle"]["packets"]];
        this->backgroundmode = backgroundmode;
    }

    bool dynamicfilter::needsrun()
    {
        // if this class runs on the second core, we need to know if changes are made and processing is needed
        return this->inputupdated;
    }

    void dynamicfilter::run()
    {
        String oldfilter = this->dynfilter;
        this->mhlisttimeout();
        if (this->config['mode'] == "singel")
        {
            this->process_single();
        }

        if (this->config['mode'] == "group" && sizeof(this->mhlist) < this->config['group']['lowerlimit'])
        {
            this->process_single();
        }
        else
        {
            this->process_groups();
        }

        this->inputupdated = false;
        if (oldfilter = this->dynfilter)
        {
            this->filterupdated = false;
        }
        else
        {
            this->filterupdated = true;
        }
    }

    bool dynamicfilter::isThrottleFree(void)
    {
        // returns true if we are in the throttle-limits
        unsigned long now = millis();
        if (((now - this->throttle[0]) / 60000) > this->config["throttle"]["minutes"])
        {
            for (int i = 1; i < sizeof(this->throttle); i++)
            {
                this->throttle[i - 1] = this->throttle[i];
            }
            this->throttle[sizeof(this->throttle) - 1] = char(0);
        }
        if (sizeof(this->throttle) < sizeof(this->config["throttle"]["packets"]))
        {
            this->throttle[sizeof(this->throttle)] = now;
            return true;
        }

        return false;
    }
    void dynamicfilter::addToList(String packet)
    {
        // add a positionpacket to the list of mheard stations
        mh_entry thisentry = this->get_mhentry_from_APRS(packet);
        thisentry.timestamp = millis();
        bool isold = false;
        for (int i; i < sizeof(this->mhlist); i++)
        {
            if (this->mhlist[i].call = thisentry.call)
            {
                this->mhlist[i] = thisentry;

                isold = true;
            }
        }
        if (!isold)
        {
            this->mhlist[sizeof(this->mhlist)] = thisentry;
        }
        this->inputupdated = true;
        if (!this->backgroundmode)
        {
            this->run();
        }
    }

    void dynamicfilter::delFromListPacket(String packet)
    {
        // remove a station from mheard list
        mh_entry thisentry = this->get_mhentry_from_APRS(packet);
        this->delFromListCall(thisentry.call);
        
    }

    String dynamicfilter::getFilterCommand()
    {
        String retval = "filter ";
        retval += this->dynfilter + " ";
        retval += this->inifilter;
        this->filterupdated = false;
        return retval;
    }
    void dynamicfilter::set_backgroundmode(bool mode)
    {
        this->backgroundmode = mode;
    }

    mh_group *dynamicfilter::getCircles()
    {

        return this->mhgroup;
    }

    bool dynamicfilter::hasnewfilter(void)
    {
        return this->filterupdated;
    }

    bool dynamicfilter::isinmhlistcall(String call)
    {
        for (int i = 0; i < sizeof(this->mhlist); i++)
        {
            if (this->mhlist[i].call == call)
            {
                return true;
            }
        }
        return false;
    }

    bool dynamicfilter::isinmhlistpacket(String packet)
    {
        mh_entry thisentry = this->get_mhentry_from_APRS(packet);
        return this->isinmhlistcall(thisentry.call);
    }
    


    void dynamicfilter::mhlisttimeout()
    {
        unsigned long now = millis();
        if((now - this->mhlist[0].timestamp)/60000 > 15) {
            this->delFromListCall(this->mhlist[0].call);
        }
        this->inputupdated = true;
        

    }
private:
    String inifilter = "";
    String dynfilter = "";
    StaticJsonDocument<512> config;
    bool inputupdated = false;
    bool filterupdated = false;
    bool backgroundmode = true;

    mh_group mhgroup[9];
    mh_entry mhlist[20];
    unsigned long throttle[0];

    /*#####################################*/

    void dynamicfilter::process_single()
    {
        this->dynfilter = "";
        for (int i = 0; i < sizeof(mhlist); i++)
        {
            mhlist[i].group = 0;
        }

        for (int i = 0; i < sizeof(mhlist) || i < 8; i++)
        {
            this->dynfilter = this->dynfilter + "f/" + this->mhlist[i].call + "/" + this->config['singel']['radius'] + " ";
        }

        this->dynfilter += this->inifilter;
        this->filterupdated = true;
    }
    void dynamicfilter::process_groups()
    {

        for (int i = 0; i < sizeof(this->mhlist); i++)
        {
            this->mhlist[i].group = 0;
        }

        for (int i = 0; i < sizeof(this->mhlist); i++)
        {
            if (sizeof(mh_group) == 0 && i == 0)
            {
                this->mhgroup[0].lat = this->mhlist[0].lat;
                this->mhgroup[0].lon = this->mhlist[0].lon;
                this->mhlist[0].group = 1;
            }
            else
            {
                for (int y = 0; y < sizeof(this->mhgroup); y++)
                {
                    if (this->getDistance(this->mhgroup[y].lat, this->mhgroup[y].lon, this->mhlist[i].lat, this->mhlist[i].lon) < this->config['group']['inrange'].toDouble())
                    {
                    }
                }
            }
        }

        this->dynfilter += this->inifilter;
        this->filterupdated = true;
    }
    double dynamicfilter::getDistance(double lat1, double lon1, double lat2, double lon2)
    {
        double dlon, dlat, a, c;
        double dist = 0.0;
        dlon = dtor(lon2 - lon1);
        dlat = dtor(lat2 - lat1);
        a = pow(sin(dlat / 2), 2) + cos(dtor(lat1)) * cos(dtor(lat2)) * pow(sin(dlon / 2), 2);
        c = 2 * atan2(sqrt(a), sqrt(1 - a));

        dist = 6378140 * c; // radius of the earth (6378140 meters) in feet 20925656.2
        return ((long)dist + 0.5);
    }
    double dtor(double fdegrees)
    {
        return (fdegrees * PI / 180);
    }
    double rtod(double fradians)
    {
        return (fradians * 180.0 / PI);
    }

    mh_group dynamicfilter::getmiddle(double lat1, double lon1, double lat2, double lon2)
    {
        mh_group middle;
        middle.lat = (lat1 + lat2) / 2;
        middle.lon = (lon1 + lon2) / 2;
        return middle;
    }
    mh_entry dynamicfilter::get_mhentry_from_APRS(String packet)
    {
        mh_entry thisentry;
        String GPSPacket = packet.substring(packet.indexOf(":!") + 3);
        String encodedLatitude = GPSPacket.substring(0, 4);
        String encodedLongtitude = GPSPacket.substring(4, 8);
        thisentry.call = packet.substring(packet.indexOf(">"));
        int Y1 = int(encodedLatitude[0]);
        int Y2 = int(encodedLatitude[1]);
        int Y3 = int(encodedLatitude[2]);
        int Y4 = int(encodedLatitude[3]);
        thisentry.lat = 90.0 - ((((Y1 - 33) * pow(91, 3)) + ((Y2 - 33) * pow(91, 2)) + ((Y3 - 33) * 91) + Y4 - 33) / 380926.0);

        int X1 = int(encodedLongtitude[0]);
        int X2 = int(encodedLongtitude[1]);
        int X3 = int(encodedLongtitude[2]);
        int X4 = int(encodedLongtitude[3]);
        thisentry.lon = -180.0 + ((((X1 - 33) * pow(91, 3)) + ((X2 - 33) * pow(91, 2)) + ((X3 - 33) * 91) + X4 - 33) / 190463.0);
        thisentry.raw = packet;
        thisentry.timestamp = millis();
        return thisentry;
    }

    void dynamicfilter::delFromListCall(String call)
    {
        for (int i; i < sizeof(this->mhlist); i++)
        {
            if (this->mhlist[i].call = call)
            {
                for (int j = i + 1; j < sizeof(this->mhlist); j++)
                {
                    this->mhlist[j - 1] = this->mhlist[j];
                }
                this->mhlist[sizeof(this->mhlist)-1] = mh_entry();
            }
        }
        this->inputupdated = true;
        if (!this->backgroundmode)
        {
            this->run();
        }
    }
};

#endif