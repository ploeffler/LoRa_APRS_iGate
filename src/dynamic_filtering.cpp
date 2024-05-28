#ifndef DYNAMIC__FILTERING_H
#define DYNAMIC__FILTERING_H
#include <ArduinoJson.h>
#include <dynamic_filtering.h>
#include <digi_utils.h>
#include <vector>
#include <stdlib.h>

struct SETUP
{
    String mode = "";
    bool testing;
    int throttle_packets;
    int throttle_minutes;
    int single_radius;
    int group_radius;
    int group_inrange;
    int group_lowerlimit;
};
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

struct latlon
{
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
        StaticJsonDocument<512> tempconfig;
        DeserializationError error = deserializeJson(tempconfig, config);

        this->backgroundmode = backgroundmode;
        myconfig.group_inrange = tempconfig["group"]["inrange"];
        myconfig.group_lowerlimit = tempconfig["group"]["lowerlimit"];
        myconfig.group_radius = tempconfig["group"]["radius"];
        myconfig.mode = tempconfig["mode"];
        myconfig.testing = bool(tempconfig["testing"]);
        myconfig.single_radius = tempconfig["single"]["radius"];
        myconfig.throttle_minutes = tempconfig["throttle"]["minutes"];
        myconfig.throttle_packets = tempconfig["throttle"]["packets"];
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
        if (this->myconfig.mode == "singel")
        {
            this->process_single();
        }

        if (this->myconfig.mode == "group" && sizeof(this->mhlist) < this->myconfig.group_lowerlimit)
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
        if (((nowentry.timestamp - this->throttle.at(0).timestamp) / 60000) > this->myconfig.throttle_minutes)
        {
            // remove oldest from list
            this->throttle.erase(this->throttle.begin());
        }

        // true if we are lower packets limit
        if (sizeof(this->throttle) < this->myconfig.throttle_packets)
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
        if ((now - this->mhlist.at(0).timestamp) / 60000 > 15)
        {
            this->delFromListCall(this->mhlist.at(0).call);
        }
        this->inputupdated = true;
    }

private:
    String inifilter = "";
    String dynfilter = "";

    bool inputupdated = false;
    bool filterupdated = false;
    bool backgroundmode = true;
    double group_inrange = 0;
    double single_radius = 0;
    SETUP myconfig;
    MHGROUP mhgroup;
    MHLIST mhlist;
    THLIST throttle;

    /*#####################################*/

    void dynamicfilter::process_single()
    {
        this->dynfilter = "";
        for (int i = 0; i < this->mhlist.size(); i++)
        {
            mhlist[i].group = 0;
        }

        for (int i = 0; i < sizeof(mhlist) || i < 8; i++)
        {
            this->dynfilter = this->dynfilter + "f/" + this->mhlist.at(i).call + "/" + this->myconfig.single_radius + " ";
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
                    // chech distance between mh_entry and group
                    if (this->getDistance(this->mhlist.at(i).lat, this->mhlist.at(i).lon, this->mhgroup.at(g).lat, this->mhgroup.at(g).lon) < this->myconfig.group_inrange)
                    {
                        // add entry to existing group
                        this->mhlist.at(i).group = g;
                        latlon newcenter = this->getCenter(this->mhlist.at(i).lat, this->mhlist.at(i).lon, this->mhgroup.at(g).lat, this->mhgroup.at(g).lon);
                        this->mhgroup.at(g).lat = newcenter.lat;
                        this->mhgroup.at(g).lon = newcenter.lon;
                    }
                }
            }

            // iterate through all entries
            if (i + 1 < this->mhlist.size())
            {
                // iterate through all entries+1=y
                for (int y = i + 1; y < this->mhlist.size(); y++)
                {
                    // check if y has already a group
                    if (!this->mhlist.at(y).group > 0)
                    {
                        if (this->getDistance(this->mhlist.at(i).lat, this->mhlist.at(i).lon, this->mhlist.at(y).lat, this->mhlist.at(y).lon) < this->myconfig.group_inrange)
                        {
                            // found a new group member
                            if (!this->mhlist.at(i).group == 0)
                            {
                                this->mhlist.at(y).group = this->mhlist.at(i).group;
                                latlon newcenter = this->getCenter(this->mhlist.at(i).lat, this->mhlist.at(i).lon, this->mhgroup.at(this->mhlist.at(y).group).lat, this->mhgroup.at(this->mhlist.at(y).group).lon);
                                this->mhgroup.at(y).lat = newcenter.lat;
                                this->mhgroup.at(y).lon = newcenter.lon;
                            }
                            else
                            {
                                // found a new group
                                this->mhlist.at(y).group = this->mhgroup.size() + 1;
                                this->mhlist.at(i).group = this->mhgroup.size() + 1;
                                latlon center = this->getCenter(this->mhlist.at(y).lat, this->mhlist.at(y).lon, this->mhlist.at(i).lat, this->mhlist.at(i).lon);
                                mh_group newgroup;
                                newgroup.lat = center.lat;
                                newgroup.lon = center.lon;
                                newgroup.name = "Cluster " + this->mhgroup.size() + 1;
                                mhgroup.push_back(newgroup);
                            }
                        }
                    }
                }
            }
            // build the grousfilter
            for (int i = 0; i < this->mhgroup.size(); i++)
            {
                this->dynfilter = this->dynfilter + "r/" + this->mhgroup.at(i).lat + "/" + this->mhgroup.at(i).lon + "/" + this->myconfig.group_radius + " ";
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