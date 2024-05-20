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
    double lat;
    double lon;
    int group = 0;
    String raw = "";
} mh_entry;
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

    /* delete a callsing(incl. SSID) from all lists*/
    void delFromList(String call);

    /* returns the filtercommand in the form of "filter f/.... r/.... initialfilter"*/
    String getFilterCommand();

    /* changes from/to backgroundmode */
    void set_backgroundmode(bool mode);

    mh_group *getCircles(void);

    bool hasnewfilter(void);

    bool isinmhlist(String call);

    void addToList(String packet);

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
};

#endif