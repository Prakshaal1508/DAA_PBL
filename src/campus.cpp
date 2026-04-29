#include "campus.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
map<string, Room>      rooms;
vector<TimetableEntry> timetable;
void customSortRooms(vector<string>& list) {
    int n = list.size();
    for (int i = 0; i < n - 1; i++) {
        int minIndex = i;
        
        for (int j = i + 1; j < n; j++) {
            if (list[j] < list[minIndex]) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            string temp = list[i];
            list[i] = list[minIndex];
            list[minIndex] = temp;
        }
    }
}
void buildRooms() {
    // ── BTech Building ────────────────────────────────────────────
    rooms["BT-LAB1"] = {"BT-LAB1", "BTech", 0, "Lab"};
    rooms["BT-LAB2"] = {"BT-LAB2", "BTech", 0, "Lab"};
    rooms["BT-LAB3"] = {"BT-LAB3", "BTech", 0, "Lab"};
    rooms["BT-GF1"]  = {"BT-GF1",  "BTech", 0, "Office"};
    rooms["BT-101"]  = {"BT-101",  "BTech", 1, "Classroom"};
    rooms["BT-102"]  = {"BT-102",  "BTech", 1, "Classroom"};
    rooms["BT-103"]  = {"BT-103",  "BTech", 1, "Classroom"};
    rooms["BT-201"]  = {"BT-201",  "BTech", 2, "Classroom"};
    rooms["BT-202"]  = {"BT-202",  "BTech", 2, "Classroom"};
    rooms["BT-301"]  = {"BT-301",  "BTech", 3, "Classroom"};
    rooms["BT-HOD"]  = {"BT-HOD",  "BTech", 3, "Office"};
    // ── CSIT Building ─────────────────────────────────────────────
    rooms["CS-LAB1"] = {"CS-LAB1", "CSIT", 0, "Lab"};
    rooms["CS-LAB2"] = {"CS-LAB2", "CSIT", 0, "Lab"};
    rooms["CS-LAB3"] = {"CS-LAB3", "CSIT", 0, "Lab"};
    rooms["CS-101"]  = {"CS-101",  "CSIT", 1, "Classroom"};
    rooms["CS-102"]  = {"CS-102",  "CSIT", 1, "Classroom"};
    rooms["CS-103"]  = {"CS-103",  "CSIT", 1, "Classroom"};
    rooms["CS-201"]  = {"CS-201",  "CSIT", 2, "Classroom"};
    rooms["CS-202"]  = {"CS-202",  "CSIT", 2, "Classroom"};
    rooms["CS-301"]  = {"CS-301",  "CSIT", 3, "Classroom"};
    rooms["CS-HOD"]  = {"CS-HOD",  "CSIT", 3, "Office"};
    // ── Aryabhatta Building ───────────────────────────────────────
    rooms["AR-101"]  = {"AR-101",  "Aryabhatta", 1, "Classroom"};
    rooms["AR-102"]  = {"AR-102",  "Aryabhatta", 1, "Classroom"};
    rooms["AR-201"]  = {"AR-201",  "Aryabhatta", 2, "Classroom"};
    rooms["AR-202"]  = {"AR-202",  "Aryabhatta", 2, "Classroom"};
    rooms["AR-LAB1"] = {"AR-LAB1", "Aryabhatta", 0, "Lab"};
    // ── ParamLab Building ─────────────────────────────────────────
    rooms["PL-101"]  = {"PL-101",  "ParamLab", 1, "Classroom"};
    rooms["PL-102"]  = {"PL-102",  "ParamLab", 1, "Classroom"};
    rooms["PL-LAB1"] = {"PL-LAB1", "ParamLab", 0, "Lab"};
    rooms["PL-LAB2"] = {"PL-LAB2", "ParamLab", 0, "Lab"};
    // ── Mechanical Building ───────────────────────────────────────
    rooms["ME-101"]  = {"ME-101",  "Mechanical", 1, "Classroom"};
    rooms["ME-LAB1"] = {"ME-LAB1", "Mechanical", 0, "Lab"};
    // ── Civil Building ────────────────────────────────────────────
    rooms["CV-101"]  = {"CV-101",  "Civil", 1, "Classroom"};
    rooms["CV-LAB1"] = {"CV-LAB1", "Civil", 0, "Lab"};
    // ── Library ───────────────────────────────────────────────────
    rooms["LIB-GF"]  = {"LIB-GF",  "Library", 0, "Library"};
    rooms["LIB-1F"]  = {"LIB-1F",  "Library", 1, "Library"};
}

//   course|day|HH:MM-HH:MM|subject|faculty|RoomID|Building|Floor
void loadTimetable(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Warning: cannot open " << filename << "\n";
        return;
    }
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        TimetableEntry e;
        string floorStr;

        getline(ss, e.course,   '|');
        getline(ss, e.day,      '|');
        getline(ss, e.timeSlot, '|');
        getline(ss, e.subject,  '|');
        getline(ss, e.faculty,  '|');
        getline(ss, e.room,     '|');
        getline(ss, e.building, '|');
        getline(ss, floorStr,   '|');

        e.floor = stoi(floorStr);
        timetable.push_back(e);
    }
}
vector<string> findFreeRooms(const string& day, const string& timeSlot) {

    set<string> occupied;
    for (auto& e : timetable) {
        if (e.day == day && e.timeSlot == timeSlot)
            occupied.insert(e.room);
    }

    vector<string> freeRooms;
    for (auto& [id, room] : rooms) {
        if (room.type == "Classroom" && !occupied.count(id))
            freeRooms.push_back(id);
    }

    customSortRooms(freeRooms);
    return freeRooms;
}
