#pragma once
#include <string>
#include <vector>
#include <map>
using namespace std;
struct Room {
    string id;        
    string building;  
    int    floor;     
    string type;      
};
struct TimetableEntry {
    string course;    
    string day;       
    string timeSlot;  
    string subject;
    string faculty;
    string room;      
    string building;
    int    floor;
};
extern map<string, Room>rooms;
extern vector<TimetableEntry>  timetable;
void buildRooms();
void loadTimetable(const string& filename);
vector<string> findFreeRooms(const string& day, const string& timeSlot);
