#pragma once
#include <string>
#include <vector>
#include <map>
using namespace std;
struct User {
    string username;
    string password;
    string role;   
};
struct PendingUser {
    string username;
    string password;
    string role;
    string fullName;
    string email;
};
extern const string USERS_FILE;
extern const string PENDING_FILE;
map<string,User> loadUsers();
void saveUser(const User& u);
vector<PendingUser> loadPending();
void savePendingList(const vector<PendingUser>& list);
void addPending(const PendingUser& p);
