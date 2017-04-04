#include <iostream>
#include <queue>


#ifndef HOST_H_
#define HOST_H_

using namespace std;


struct packet {
  char type;
  int destination;
} ;


class Host {
    protected:
    double backoffTime = 0;
    //int tValue = 1;
    //int queueLength = 0;
    int hostNumber;
    bool isWaitingForAck = false;
    //double timeOut;
    //queue<char> q;
    //queue<int> destForAck;
    queue<packet> packetQueue;

    int retransmitTimes = 0;

    /* STUFF IN QUEUE
       p = pkt
       a = ACK
     */
     
     //Stats stuff
     double queueDelay =0, lastPop=0;

    public:

    Host(int num){
            hostNumber = num;
    }

    double getBackoffTime(){
            return backoffTime;
    }

    /*	int getTValue(){
            return tValue;
        }*/

    int getqueueLength(){
            return packetQueue.size();
    }

    void push(packet p, double curTime){
        //Stats
        queueDelay += (curTime - (double)lastPop) * packetQueue.size();
        lastPop = curTime;
        
        packetQueue.push(p);
    }


    packet front(){
            return packetQueue.front();
    }

    packet pop(double curTime){
        if(packetQueue.size() == 0) {
            packet p;
            p.type = '!';
            p.destination = 99;
            return p; // for invalid
        }
        retransmitTimes = 0;    //reset retransmitTimes
        packet p = packetQueue.front();
        packetQueue.pop();
        backoffTime = 0;    //reset backoff time
        
        //Stats
        queueDelay += (curTime - (double)lastPop) * packetQueue.size();
        lastPop = curTime;
        
        return p;
    }

    int getHostNumber(){
            return hostNumber;
    }

    void setBackoffTime(double t){
            backoffTime = t;
    }
    void redBackoffTime(double t){      //won't get below 0
            if(t < 0) {
                cout << "Error: cannot reduce by neg val."<<endl; 
                return; 
            }
            backoffTime-= t;
            if(backoffTime < 0) {
                    cout << "Error: backoffTime < 0" << endl;
                    backoffTime = 0;
            }
    }

    void setIsWaitingForAck(bool t){
            isWaitingForAck = t;
    }

    bool getIsWaitingForAck(){
            return isWaitingForAck;
    }


    int getRetransmitTimes(){
            return retransmitTimes;
    }

    void incRetransmitTimes(){
            retransmitTimes++;
    }

    bool canSendNow(){
        return (backoffTime == 0.0 && isWaitingForAck == false && packetQueue.size() > 0);
    }
    
    //Stats
    double getQueueDelay(){
        return queueDelay;
    }


};



#endif
