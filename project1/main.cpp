#include <iostream>
//#include <queue>
#include <set>
#include <stdlib.h> // for rand
#include <math.h>   // for log
#include <limits>
#include <time.h>
#include "event.h"
//#include "packet.h"   // i dont thing we actually need a queue

using namespace std;
//queue<Packet> packetQueue;
int maxQueue, queueLen;
struct comp {
        bool operator() (const Event& lhs, const Event& rhs) const
        { return lhs.GetEventTime() < rhs.GetEventTime(); }
};
set<Event, comp> gel;
double sRate, aRate, serverAvailAt;
double curTime;
int droppedPkts;
int areaQLen = 0;
int numArrivals = 0;
int numberHosts;

double negExpDist(double rate){
        double u = drand48();
        double exp = ((-1.0/rate)*log(1.0-u));
        return exp;
}

double parettoDist(double rate){    //paretto with Xm = 1
    double p = exp(negExpDist(rate)); 
    //cout << "paretto: " << p << endl;
    return p;
}

void init(){
        curTime = 0;
        aRate = 0.1;
        sRate = 1.0;
        serverAvailAt = 0;
        maxQueue = 9999999;
        queueLen = 0;

        numberHosts = 10; // N


        // Make an array of N queue Lengths

        gel.insert(Event(curTime + parettoDist(aRate), true));
}

int main(){
        init();
        int i = 0;
        double serverBusyTime = 0.0;
        for (set<Event, comp>::iterator iter= gel.begin(); iter!=gel.end() && i < 100000; ++iter, i++) {
                Event f = *(iter);
                double dt = f.GetEventTime() - curTime;
                areaQLen += dt * queueLen;
                curTime = f.GetEventTime(); //update time
                if(f.IsArrival()) {
                    numArrivals++;
                    // create A and insert into gel
                    gel.insert(Event(curTime + parettoDist(aRate), true));

                    if(curTime > serverAvailAt) { // server idle
                            //create departure and insert into gel
                    		double serviceTime = negExpDist(sRate);
                            serverBusyTime += serviceTime;
                    		serverAvailAt = curTime + serviceTime;
                            gel.insert(Event(curTime + serviceTime, false));
                    }
                    else{ // server still busy
                            if(queueLen < maxQueue){ //buffer not full
                                queueLen++;
                            }
                            else{ // buffer full
                                //drop packet
                                droppedPkts++;
                            }
                    }
                }
                else{ //is departure
                        if(queueLen > 0){ //buffer not empty
                            //pop packet from buffer
                            queueLen--;
                            //schedule next departure
                            double serviceTime = negExpDist(sRate);

                            // cout << "service Time: " << serviceTime << endl;
                            serverBusyTime += serviceTime;

                            serverAvailAt = curTime + serviceTime;


                            gel.insert(Event(curTime + serviceTime, false));
                        }
                }
        }


/*        for (set<Event, comp>::iterator iter= gel.begin(); iter!=gel.end(); ++iter) {
        	Event f = *(iter);
            curTime = f.GetEventTime(); //update time
            cout << curTime << " " << f.IsArrival() << endl;
        }*/

        // cout << curTime << "AND " << serverBusyTime << endl;
        cout << "sRate = " << sRate << endl;
        cout << "aRate = " << aRate << endl;

         cout << "utilization:" << serverBusyTime/curTime << endl;
         cout << "pkts dropped: " << droppedPkts << endl;
         cout << "mean q len: " << (double)areaQLen/curTime << endl;
          cout << endl;
        return 0;
}
