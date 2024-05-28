#ifndef DYNAMIC__FILTERING_H
#define DYNAMIC__FILTERING_H
/*

expects the part "dynamicfilters" of the igate_config.json like this
{
"mode" = "single",    //must be "single" or "group"
"testing" = "false",  //generates circle-objects
"throttle" = {
    "packets" = "20",
    "minutes" = "15"
    },
"single" = {
    "radius" = "10" # kilometers
    }
"group" = {
    "radius" = "10",
    "inrange" = "10",
    "lowerlimit" = "3"
    }
}

*/
struct SETUP {
    String mode;
    bool testing;
    int throttle_packets;
    int throttle_minutes;
    int single_radius;
    int group_radius;
    int group_inrange;
    int group_lowerlimit;
}; 
struct latlon {
    double lat;
    double lon;
};

class dynamicfilter
{
public:
    /* initialzation comes with the following arguments:
    - initialfilter: APRS-IS filter configuration which will lead to the last of the filters,
    - config: the stringifyed parts of "dynamicfilters" from igate_config.json,
    - backgroundmode: true/false if the processing will be done in the background*/
    dynamicfilter(String initialfilter, String config, bool backgroundmode = true);

    /* if in background or not, this function will do all the calculations*/
    void run();

    /* returns true if transmission of the packet is in the throtteling limits */
    bool isThrottleFree();

    

    /* returns the filtercommand in the form of "filter f/.... r/.... initialfilter"*/
    String getFilterCommand();

    /* changes from/to backgroundmode */
    void set_backgroundmode(bool mode);
    
    /*returns array of circle positions with name,lat,lon,range */
    mh_group *getCircles(void);

    /*returns true if the filters were updated*/
    bool hasnewfilter(void);


    /* returns true if the call+ssid is in the mh_list*/
    bool isinmhlistcall(String call);

    /* returns true if the call+ssid is in the mh_list*/
    bool isinmhlistpacket(String packet);

    /* adds or updates an entry in the mh_list*/
    void addToList(String packet);

    

    void delFromListPacket(String packet);

    /* updates the mhlist if oldest is timeouted */
    void mhlisttimeout();
    

private:
    String inifilter;
    String dynfilter;
    StaticJsonDocument<512> config;
    bool inputupdated;
    bool filterupdated;
    typedef struct
    {
        String call = "";
        double lat;
        double lon;
        int group = 0;
        String raw = "";
    } mh_entry;
    double group_inrange;
    double single_radius;

    mh_entry mhlist[20];

    mh_group mhgroup[9];
    void process_single();
    void process_groups();
    double getDistance(double lat1, double lon1, double lat2, double lon2);
    double dtor(double fdegrees);
    double rtod(double fradians);
    mh_group getmiddle(double lat1, double lon1, double lat2, double lon2);
    unsigned long throttle[];
    mh_entry get_mhentry_from_APRS(String packet);

    /* delete a callsing(incl. SSID) from all lists*/
    void delFromListCall(String call);
    latlon getCenter();
    
};

#endif
