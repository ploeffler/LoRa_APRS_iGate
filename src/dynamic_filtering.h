#ifndef DYNAMIC__FILTERING_H
#define DYNAMIC__FILTERING_H
/*

expects config-json
{
"mode" = "single",    //must be "single" or "group"
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

class dynamicfilter
{
    public:
    dynamicfilter(String initialfilter,String config,bool backgroundmode = true); 
    void run();
    
    bool isThrotleFree();
    
    void delFromList(String call);
    
    String getFilterCommand();

    private:
    
    String inifilter ;
    String dynfilter ;
    StaticJsonDocument<512> config ;
    bool inputupdated;
    bool filterupdated;
    typedef struct  {
        String call = "";
        double lat;
        double lon;
        int group = 0;
        String raw="";
    } mh_entry;
    mh_entry mhlist[20];
    typedef struct {
        double lat = 0;
        double lon = 0;
    } mh_group ;
    mh_group mhgroup[9];
    void process_single();
    void process_groups();
    double getDistance(double lat1,double lon1,double lat2, double lon2);
    double dtor(double fdegrees);
    double rtod(double fradians);
    mh_group getmiddle(double lat1,double lon1,double lat2, double lon2);
    unsigned long throttle[];

};

#endif