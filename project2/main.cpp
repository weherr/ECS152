#include <iostream>
//#include <queue>
#include <set>
#include <stdlib.h> // for rand
#include <math.h>   // for log
#include <limits>
#include <time.h>
#include <vector>
#include <iterator>
#include "event.h"
#include "host.h"
//#include "packet.h"   // i dont thing we actually need a queue

#define NUMHOSTS 10

using namespace std;
//queue<Packet> packetQueue;
int maxQueue, queueLen;
struct comp {
        bool operator() (const Event& lhs, const Event& rhs) const{ 
            if(lhs.getEventTime() == rhs.getEventTime()){
                cout << "Error: CONFLICTS" << endl;
            }
            return lhs.getEventTime() < rhs.getEventTime();
        }
};
set<Event, comp> gel;
double sRate, aRate, channelAvailAt;
double curTime;
int droppedPkts;
int areaQLen = 0;
double DIFS, SIFS;
double timeOutPeriod;
//double hostsBOUpdatedOn=0;
double tValue;

int numberHosts;
vector<Host> hosts;

//Stats vars
double totalSent =0;


double negExpDist(double rate){
        double u = drand48();
        double exp = ((-1.0/rate)*log(1.0-u));
        return exp;
}

double transmissionTime(double rate){
    double r = negExpDist(rate) * 1544;
    while(r > 1544){
        r = negExpDist(rate) * 1544;
    }
    totalSent += r;
    double out  = (r*8.0)/11e6;
    // cout << "transtime: " << out << endl;
    return out;
}

double backoff(double t){
    return drand48() * t;
}


Host smallestBackoff(){ // returns host with min valid non zero backoff time
    Host minH = Host(-1);   // valid = not waiting for ACK and queue not empty
    minH.setBackoffTime(99999999);
    for(int i=0; i<hosts.size(); i++){
        if(hosts[i].getBackoffTime() >= 0 
        && hosts[i].getBackoffTime() < minH.getBackoffTime()
        && hosts[i].getIsWaitingForAck() == false 
        && hosts[i].getqueueLength() > 0){
            minH = hosts[i];
        }
    }
    return minH;
}

void pushStartAndFinishEvents(int host){
    if(channelAvailAt > curTime){
        cout << "Error: should not schedule start/finish bc channel is BUSY. \t skipping..." << endl;
        return;
    }
    if(hosts.at(host).canSendNow() == false && host != smallestBackoff().getHostNumber()){
        cout << "Error: cannot send now" << endl;
        return;
    }
    double transTime = transmissionTime(sRate);
    gel.insert(Event(curTime + DIFS + hosts.at(host).getBackoffTime(), 's', host));
    gel.insert(Event(curTime + transTime + hosts.at(host).getBackoffTime() + DIFS, 'f', host));
    //update channelAvailAt
    channelAvailAt = curTime + transTime + hosts.at(host).getBackoffTime() + DIFS;
}

void pushStartAndFinishEventsForAck(int host){
    if(channelAvailAt > curTime){
        cout << "Error: should not schedule start/finish bc channel is BUSY. \t skipping..." << endl;
        return;
    }
    double transTime = (64*8)/(11e6);
    totalSent += 64;
    gel.insert(Event(curTime + SIFS + hosts.at(host).getBackoffTime(), 's', host));
    gel.insert(Event(curTime + transTime + hosts.at(host).getBackoffTime() + SIFS, 'f', host));
    //update channelAvailAt
    channelAvailAt = curTime + transTime + hosts.at(host).getBackoffTime() + SIFS;
}


void init(){
        curTime = 0;
        aRate = 0.5;
        sRate = 1;
        channelAvailAt = 0;
        DIFS = 0.0001;
        SIFS = 0.00005;
        timeOutPeriod = 0.010; // change this around
        tValue = 1.0;

        for(int i =0; i < NUMHOSTS; i++){
            Host newHost(i);
            hosts.push_back(newHost);
            gel.insert(Event(curTime + negExpDist(aRate), 'a', i));
        }

        numberHosts = hosts.size();
        cout << "numberHosts: " << numberHosts<< endl;
}


int main(){
        init();
        int i = 0;
        // double serverBusyTime = 0.0;
        for (set<Event, comp>::iterator iter= gel.begin(); iter!=gel.end() && i < 100000; ++iter, i++) {

                Event f = *(iter);
                
                // double dt = f.getEventTime() - curTime;
                // areaQLen += dt * queueLen;
                curTime = f.getEventTime(); //update time

                for(int x=0; x<numberHosts; x++){
                    cout << hosts[x].getqueueLength() << " ";
                }
                cout << "\tevent: '" << f.getType() << "' i: " << i <<" host: " << f.getHost();
                cout << endl;
                //this is actually possible
                /*for(int x=0; x<numberHosts; x++){
                    if(hosts.at(x).getIsWaitingForAck() == true){
                            cout<< "Error: wait for ACK and have NZ backoff -- host: "<< x << endl;
                    }
                }*/
                
                // Arrival Event
                if(f.getType() == 'a') {
                    // create next Arrival
                    gel.insert(Event(curTime + negExpDist(aRate), 'a', f.getHost()));

                    // Decided who pkt is going to be sent to
                    int sendTo = (int)(drand48() * numberHosts);
                    while(sendTo == f.getHost()){
                        sendTo = (int)(drand48() * numberHosts);
                    }

                    //push pkt onto the queue
                    packet pkt;
                    pkt.type = 'p';
                    pkt.destination = sendTo;
                    hosts.at(f.getHost()).push(pkt, curTime);

                    if(curTime >= channelAvailAt) { // channel idle

                        //There is ACK in front queue and can send
                        if(hosts.at(f.getHost()).canSendNow() && hosts.at(f.getHost()).front().type == 'a' && hosts.at(f.getHost()).getIsWaitingForAck() == false ){
                            // Schedule ACK start/finish
                            pushStartAndFinishEventsForAck(f.getHost()); //maybe can never happen
                        }
                        // Pkt in front of queue and can send
                        else if(hosts.at(f.getHost()).canSendNow() && hosts.at(f.getHost()).front().type == 'p' && hosts.at(f.getHost()).getIsWaitingForAck() == false){
                            // Schedule ptk start/finish
                            pushStartAndFinishEvents(f.getHost());

                        }
                    }
                    else{ // channel still busy (same as 'A' case)

                        // Set a new backoff time if dont already have one
                        if(hosts.at(f.getHost()).getBackoffTime() == 0 && hosts.at(f.getHost()).getIsWaitingForAck() == false){ //This second condition fixes freak of nature
                            double x = backoff(tValue);
                            hosts.at(f.getHost()).setBackoffTime(x);
                        }
                    }

                }

                // ACK arrival
                else if(f.getType() == 'A'){

                    if(curTime >= channelAvailAt) { // channel idle

                        //There is ACK in front queue and can send
                        if(hosts.at(f.getHost()).canSendNow() && hosts.at(f.getHost()).front().type == 'a' && hosts.at(f.getHost()).getIsWaitingForAck() == false){
                            // Schedule ACK start/finish
                            pushStartAndFinishEventsForAck(f.getHost());
                        }
                        // Pkt in front of queue and can send
                        else if(hosts.at(f.getHost()).canSendNow() && hosts.at(f.getHost()).front().type == 'p' && hosts.at(f.getHost()).getIsWaitingForAck() == false){
                            // This should never happen, but logic is correct
                            cout << "Error: freak of nature, says billy" << endl;

                        }
                    }
                    else{   // channel busy (same as 'a' case)

                        // Set a new backoff time if dont already have one
                        if(hosts.at(f.getHost()).getBackoffTime() == 0 && hosts.at(f.getHost()).getIsWaitingForAck() == false){ //This second condition fixes freak of nature
                            double x = backoff(tValue);
                            hosts.at(f.getHost()).setBackoffTime(x);
                        }

                    }
                }

                // Finish transmission event
                else if(f.getType() == 'f'){
                    char sentPacketType = hosts.at(f.getHost()).front().type; // dont pop until we recieved ACK
                    int sentPacketDestination = hosts.at(f.getHost()).front().destination;


                    // sent regular pkt
                    if(sentPacketType == 'p'){
                        // create timeout event
                        gel.insert(Event(curTime + timeOutPeriod, 't', f.getHost()));
                        hosts.at(f.getHost()).setIsWaitingForAck(true);

                        packet ACK;
                        ACK.destination = f.getHost();
                        ACK.type = 'a';
                        hosts.at(sentPacketDestination).push(ACK, curTime);

                        // create an arrival event for ACK
                        auto x = gel.insert(Event(curTime + 0.000001111111101, 'A', sentPacketDestination));

                    }
                    // sent ack
                    else if(sentPacketType == 'a'){
                        if(hosts.at(f.getHost()).getIsWaitingForAck() == true){
                            cout << "Error: Waiting for ACK is true...ERROR this should never happen" << endl;
                        }
//                        hosts.at(f.getHost()).setIsWaitingForAck(false); //host does not wait for ACK for ACK


                        packet sentACK = hosts.at(f.getHost()).pop(curTime);
                        int destHost = sentACK.destination;

                        if(destHost != 99){ // correct destination got ACK
                            // tries then we have already popped that packet in the 't' event
                            // therefore we should not pop it here
                            if( hosts.at(destHost).getIsWaitingForAck() == true && hosts.at(destHost).front().destination == f.getHost()){
                                hosts.at(destHost).setIsWaitingForAck(false); // dest got the ACK

                                //pop off packet of host recieving the correct ACK
                                hosts.at(destHost).pop(curTime);
                            }
                        }
                        else{
                            cout << "Error: dest host invalid " << endl;
                        }

                    }



                }
                else if(f.getType() == 's'){    // started a transmission
                    char sentPacketType = hosts.at(f.getHost()).front().type; // dont pop until we recieved ACK

                    // sent regular pkt
                    if(sentPacketType == 'p'){
                        hosts.at(f.getHost()).setIsWaitingForAck(true);
                    }
                    

                }
                
                // Timeout event
                else if(f.getType() == 't'){
                   if(hosts.at(f.getHost()).getRetransmitTimes() >= 2 ){ // 2 not 3 bc we only increment after 1 fail
                       cout << "give up trying to send pkt" << endl;
                       hosts.at(f.getHost()).setIsWaitingForAck(false);
                       hosts.at(f.getHost()).pop(curTime); //give up on that pkt
                       continue;
                   }
                   
                    //havnt given up yet
                    if(hosts.at(f.getHost()).getIsWaitingForAck() == true ){ // still waiting for ACK
                        
                        // resend pkt (not popped off untill ACK recieved)
                        hosts.at(f.getHost()).setIsWaitingForAck(false); // Stop waiting for ACK -- until resend
                        if(curTime >= channelAvailAt && hosts.at(f.getHost()).canSendNow()){ // channel idle
                            pushStartAndFinishEvents(f.getHost());
                            //increment retransmission tries
                            hosts.at(f.getHost()).incRetransmitTimes();
                        }

                        // channel is busy need to set back off time
                        else{

                            // Set a new backoff time
                            if(hosts.at(f.getHost()).getBackoffTime() == 0){
                                hosts.at(f.getHost()).setBackoffTime(backoff(tValue));
                            }
                            else{   //backoff time should already be zero
                                cout << "Error: Another freak of nature!" << endl;
                            }
                        }
                    }
                }
                
                Event nextEvent = *(next(iter,1));
                // Handle backoff reducing
                if(curTime >= channelAvailAt){ // channel idle
                    // need to see whos backoff is smallest
                    Host minH = smallestBackoff();
                    int minHNum = minH.getHostNumber();
                    
                    // Sanity checks
                    if(minHNum!=-1 && (minH.getIsWaitingForAck() || minH.getBackoffTime() < 0.0 || minH.getqueueLength() ==0)){
                        cout <<"Error: smallestBackoff is wrong! \n" <<
                            "\thost: "<<minHNum<<
                            "\twaiting: " <<minH.getIsWaitingForAck()<<
                            "\tBO: " <<minH.getBackoffTime()<<
                            "\tqLen: " <<minH.getqueueLength()<< endl;
                        
                    }
                    
                    // Schedule next start and finish for min backoff host
                    // if min is valid and backoff will expire
                    if(minHNum != -1 && minH.getBackoffTime() + curTime < nextEvent.getEventTime() ){
                        if(minH.front().type == 'p'){    //schedule for pkt
                            pushStartAndFinishEvents(minHNum);
                        }
                        else if(minH.front().type == 'a'){   //schedle for ACK
                            pushStartAndFinishEventsForAck(minHNum);
                        }

                        // subtract the min backoff time from everyone
                        for(auto&& host : hosts){
                            if(host.getBackoffTime() != 0 && host.getIsWaitingForAck() == false){
                                host.redBackoffTime(minH.getBackoffTime());
                            }
                        }
                    }
                    else if(minHNum != -1 && minH.getBackoffTime() + curTime > nextEvent.getEventTime()){   //min backoff won't expire
                        // subtract the min backoff time from everyone
                        for(auto&& host : hosts){
                            if(host.getBackoffTime() != 0 && host.getIsWaitingForAck() == false){
                                host.redBackoffTime(nextEvent.getEventTime() - curTime);
                            }
                        }
                    }
                }
        }// end main for loop
        
        //Print Stats
        cout << "totalSent: " << totalSent << endl;
        cout << "curTime: " << curTime << endl;
        double thruput = totalSent/(double)curTime;
        cout << "ThuPut: " << thruput <<" bytes/sec" <<endl;
        
        double totalQueueDelay =0;
        for(auto host : hosts){
            totalQueueDelay += host.getQueueDelay();
        }
        cout << "Total Queue Delay: " << totalQueueDelay << endl;
        cout << "Avg Network Delay: " << totalQueueDelay/(double)thruput << endl;
        

        return 0;
}
