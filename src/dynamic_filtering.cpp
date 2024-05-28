#ifndef DYNAMIC__FILTERING_H
#define DYNAMIC__FILTERING_H
#include <ArduinoJson.h>
#include <dynamic_filtering.h>
#include <digi_utils.h>
#include <vector>
#include <stdlib.h>

struct mh_group
{
    String name = "";
    double lat = 0;
    double lon = 0;
    int range = 0;
};
struct mh_entry
{
    String call = "";
    double lat = 0;
    double lon = 0;
    int group = 0;
    String raw = "";
    unsigned long timestamp = 0;
};


struct latlon {
    double lat;
    double lon;
};

struct th_entry
{
    unsigned long timestamp = 0;
};

typedef std::vector<mh_group> MHGROUP;
typedef std::vector<mh_entry> MHLIST;
typedef std::vector<th_entry> THLIST;

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
        unsigned long tsnow = millis();
        th_entry nowentry;
        nowentry.timestamp = tsnow;
        // remove oldest from list if outside minutes
        if (((nowentry.timestamp - this->throttle.at(0).timestamp) / 60000) > this->config["throttle"]["minutes"])
        {
            // remove oldest from list
            this->throttle.erase(this->throttle.begin());
        }

        // true if we are lower packets limit
        if (sizeof(this->throttle) < sizeof(this->config["throttle"]["packets"]))
        {
            // add nowtimestamp at the end of throttle
            this->throttle.push_back(nowentry);
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

            if (this->mhlist.at(i).call == thisentry.call)
            {
                this->mhlist.at(i) = thisentry;

                isold = true;
            }
        }
        if (!isold)
        {
            this->mhlist.push_back(thisentry);
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

    MHGROUP dynamicfilter::getCircles()
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
            if (this->mhlist.at(i).call == call)
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
        if ((now - this->mhlist[0].timestamp) / 60000 > 15)
        {
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
    double group_inrange = 0;
    double single_radius = 0;

    MHGROUP mhgroup;
    MHLIST mhlist;
    THLIST throttle;

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
            this->dynfilter = this->dynfilter + "f/" + this->mhlist.at(i).call + "/" + this->single_radius + " ";
        }

        this->dynfilter += this->inifilter;
        this->filterupdated = true;
    }
    void dynamicfilter::process_groups()
    {
        // delet all groupinfo in mhlist
        for (int i = 0; i < sizeof(this->mhlist); i++)
        {
            this->mhlist.at(i).group = 0;
        }
        // remove all groups
        this->mhgroup.clear();

        // iterate through all entrys
        for (int i = 0; i < sizeof(this->mhlist); i++)
        {
            // iterate through all groups if there are any already
            if (mhgroup.size() > 0 && this->mhlist.at(i).group == 0)
            {
                for (int g = 0; g < mhgroup.size(); g++)
                {
                    //chech distance between mh_entry and group
                    if (this->getDistance(this->mhlist.at(i).lat, this->mhlist.at(i).lon, this->mhgroup.at(g).lat, this->mhgroup.at(g).lon) < this->group_inrange)
                    {
                        // add entry to existing group
                        this->mhlist.at(i).group = g;
                    }
                }
            }

            //iterate through all entries
            if (i + 1 < this->mhlist.size())
            {
                for (int y = i + 1; y < this->mhlist.size(); y++)
                {
                    //check if y has already a group
                    if( ! this->mhlist.at(y).group > 0) {
                    if (this->getDistance(this->mhlist.at(i).lat, this->mhlist.at(i).lon, this->mhlist.at(y).lat, this->mhlist.at(y).lon) < this->group_inrange)
                    {
                        // found a new group member
                        if(!this->mhlist.at(i).group == 0) {
                          this->mhlist.at(y).group = this->mhlist.at(i).group   ;  

                        } else {
                        //found a new group
                        this->mhlist.at(y).group = this->mhgroup.size()+1;
                        this->mhlist.at(i).group = this->mhgroup.size()+1;
                        latlon center = this->getCenter(this->mhlist.at(y).lat,this->mhlist.at(y).lon,this->mhlist.at(i).lat, this->mhlist.at(i).lon );
                        mh_group newgroup;
                        newgroup.lat = center.lat;
                        newgroup.lon = center.lon;
                        newgroup.name = "Cluster "+this->mhgroup.size()+1;
                        mhgroup.push_back(newgroup);
                        }
                    }
                    }
                }
            }
            /*
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
            */
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
                this->mhlist[sizeof(this->mhlist) - 1] = mh_entry();
            }
        }
        this->inputupdated = true;
        if (!this->backgroundmode)
        {
            this->run();
        }
    }
    latlon dynamicfilter::getCenter(double lat1, double lon1, double lat2, double lon2)
    {
        latlon center;
        center.lat = (lat1 + lat2) / 2;
        center.lon = (lon1 + lon2) / 2;
        return center;
    }
};

#endif