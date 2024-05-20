#ifndef DYNAMIC__FILTERING_H
#define DYNAMIC__FILTERING_H
#include <ArduinoJson.h>
#include "dynamic_filtering.h"
#include <digi_utils.h>

class dynamicfilter
{
    public:
    dynamicfilter::dynamicfilter(String initialfilter,String config,bool backgroundmode = true) 
    {
    this->inifilter = initialfilter;
    DeserializationError error = deserializeJson(this->config, config);
    this->throttle[this->config["throttle"]["packets"]];

    }
    
    bool dynamicfilter::needsrun(){
        // if this class runs on the second core, we need to know if changes are made and processing is needed
        return this->inputupdated;
    }
    
    void dynamicfilter::run() {
        // in the loop of the second core we process
        if(this->config['mode']=="singel" && sizeof(this->mhlist) < this->config['group']['lowerlimit']) {
            this->process_single();
        }
        if(this->config['mode']=="group") {
            this->process_groups();
        }
    }
    
    bool dynamicfilter::isThrotleFree()
    {
        // returns true if we are in the throtle-limits
        return false;
    }
    void dynamicfilter::addToList(String packet)
    {
        // add a positionpacket to the list of mheard stations
        
        
        this->inputupdated=true;
    }

    void dynamicfilter::delFromList(String packet)
    {
        // remove a station from mheard list
        this->inputupdated=true;
    }
    
    
    String dynamicfilter::getFilterCommand(){
       String retval = "filter ";
       retval += this->dynfilter + " ";
       retval += this->inifilter;
       this->filterupdated = false;
       return retval; 
    }
    private:
    String inifilter = "";
    String dynfilter = "";
    StaticJsonDocument<512> config ;
    bool inputupdated = false;
    bool filterupdated = false;
    bool backgroundmode = true;
    
    typedef struct  {
        String call = "";
        double lat = 0;
        double lon = 0;
        int group = 0;
        String raw="";
    } mh_entry;
    typedef struct {
        double lat = 0;
        double lon = 0;
    } mh_group ;
    mh_group mhgroup[9];
    mh_entry mhlist[20];
    unsigned long throttle[];

    void dynamicfilter::process_single() {
        this->dynfilter="";
        for(int i =0; i < sizeof(mhlist) ; i++){
                mhlist[i].group = 0;
            }
        
        
            for(int i =0; i < sizeof(mhlist) || i < 8; i++){
            this->dynfilter = this->dynfilter + "f/" + this->mhlist[i].call + "/" + this->config['singel']['radius']+ " "; 
            }
        
        
        this->dynfilter += this->inifilter;
        this->filterupdated = true;
    }
    void dynamicfilter::process_groups()
    {
        
            for(int i =0; i < sizeof(this->mhlist) ; i++){
                this->mhlist[i].group = 0;
            }
            
            
                for(int i =0; i < sizeof(this->mhlist) ; i++){
                   if(sizeof(mh_group)==0 && i == 0) {
                    this->mhgroup[0].lat=this->mhlist[0].lat;
                    this->mhgroup[0].lon=this->mhlist[0].lon;
                    this->mhlist[0].group=1;
                   } else {
                    for(int y =0;y<sizeof(this->mhgroup);y++) {
                     if(this->getDistance(this->mhgroup[y].lat,this->mhgroup[y].lon,this->mhlist[i].lat,this->mhlist[i].lon)<  this->config['group']['inrange'].toDouble()) {

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
        a = pow(sin(dlat/2),2) + cos(dtor(lat1)) * cos(dtor(lat2)) * pow(sin(dlon/2),2);
        c = 2 * atan2(sqrt(a), sqrt(1-a));

        dist = 6378140 * c;  //radius of the earth (6378140 meters) in feet 20925656.2
        return( (long) dist + 0.5);
        
    }
    double dtor(double fdegrees)
        {
        return(fdegrees * PI / 180);
        }
    double rtod(double fradians)
        {
        return(fradians * 180.0 / PI);
        }
    
    mh_group dynamicfilter::getmiddle(double lat1, double lon1, double lat2, double lon2)
        {
            mh_group middle ;
            middle.lat = (lat1+lat2)/2;
            middle.lon = (lon1+lon2)/2;
            return middle;
        }
};

#endif