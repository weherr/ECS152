#ifndef EVENT_H_
#define EVENT_H_

// A GEL a Linked List
class Event{
    protected:
/*        Event* NextEvent;
        Event* PreviousEvent;*/

        double EventTime;

        bool isArrival;

    public:
        Event(double Time, bool isArrival);

    /*    void SetNextEvent(Event* next){
            NextEvent = next;
        }
        void SetPreviousEvent(Event* previous){
            PreviousEvent = previous;
        }

        Event* GetNextEvent(){
            return NextEvent;
        }

        Event* GetPreviousEvent(){
            return PreviousEvent;
        }*/

        double GetEventTime() const{
            return EventTime;
        }

        bool IsArrival(){
            return isArrival;
        }

};

Event::Event(double Time, bool Arrival) {
  EventTime = Time;
  isArrival = Arrival;
}

#endif
