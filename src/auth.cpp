#include "auth.h"
#include <fstream>
#include <sstream>
const string USERS_FILE   = "data/users.txt";
const string PENDING_FILE = "data/pending.txt";
map<string,User> loadUsers() {
    map<string,User> users;
    ifstream file(USERS_FILE);
    string line;

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        User u;
        getline(ss, u.username, '|');
        getline(ss, u.password, '|');
        getline(ss, u.role,     '|');

        users[u.username] = u;
    }
    return users;
}
void saveUser(const User& u) {
    ofstream file(USERS_FILE, ios::app);
    file << u.username << '|'
         << u.password << '|'
         << u.role     << '\n';
}
vector<PendingUser> loadPending() {
    vector<PendingUser> list;
    ifstream file(PENDING_FILE);
    string line;

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        PendingUser p;
        getline(ss, p.username, '|');
        getline(ss, p.password, '|');
        getline(ss, p.role,     '|');
        getline(ss, p.fullName, '|');
        getline(ss, p.email,    '|');

        list.push_back(p);
    }
    return list;
}
void savePendingList(const vector<PendingUser>& list) {
    ofstream file(PENDING_FILE); 
    for (auto& p : list)
        file << p.username << '|'
             << p.password << '|'
             << p.role     << '|'
             << p.fullName << '|'
             << p.email    << '\n';
}
void addPending(const PendingUser& p) {
    ofstream file(PENDING_FILE, ios::app);
    file << p.username << '|'
         << p.password << '|'
         << p.role     << '|'
         << p.fullName << '|'
         << p.email    << '\n';
}
