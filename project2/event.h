#ifndef EVENT_H_
#define EVENT_H_

// A GEL a Linked List
class Event{
    protected:

        double EventTime;
        char eventType;
        int host;
        /*
        a = pkt arrival
        s = start transmission
        f = finish transmission
        t = timeout
        A = ACK arrival
        */
    public:
        Event(double Time, char type, int host);

        double getEventTime() const{
            return EventTime;
        }

        char getType(){
            return eventType;
        }
        
        int getHost(){
            return host;
        }

};

Event::Event(double Time, char type, int host) {
  EventTime = Time;
  eventType = type;
  this->host = host;
}

#endif
