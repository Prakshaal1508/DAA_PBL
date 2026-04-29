

#include "httplib.h"
#include "graph.h"
#include "dfs.h"
#include "auth.h"
#include "campus.h"

#include <sstream>
#include <set>
#include <climits>

using namespace std;
using namespace httplib;

// JSON helper 
// Wraps a string in JSON quotes with proper escaping
static string J(const string& s) {
    string r;
    for (char c : s) {
        if      (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else                r += c;
    }
    return "\"" + r + "\"";
}

// Register all API routes then start the server
int main() {

    //Load data into memory 
    buildCampusGraph();
    buildRooms();
    loadTimetable("data/timetable.txt");
    cout << "Data loaded.\n";

    Server svr;


    // GET /api/graph
    // Returns all campus nodes + edges so the frontend draws the map

    svr.Get("/api/graph", [](const Request&, Response& res) {

        // Build nodes array
        string json = "{\"nodes\":[";
        bool first = true;
        for (auto& [id, _] : campusGraph) {
            auto pos = nodePos.count(id) ? nodePos[id] : make_pair(50,50);
            if (!first) json += ",";
            json += "{\"id\":"  + J(id)
                  + ",\"x\":"  + to_string(pos.first)
                  + ",\"y\":"  + to_string(pos.second) + "}";
            first = false;
        }

        // Build edges array (skip duplicates)
        json += "],\"edges\":[";
        set<string> seen;
        first = true;
        for (auto& [node, edges] : campusGraph) {
            for (auto& e : edges) {
                string key = node < e.to ? node+"-"+e.to : e.to+"-"+node;
                if (seen.count(key)) continue;
                seen.insert(key);
                if (!first) json += ",";
                json += "{\"from\":"   + J(node)
                      + ",\"to\":"     + J(e.to)
                      + ",\"weight\":" + to_string(e.weight) + "}";
                first = false;
            }
        }
        json += "]}";
        res.set_content(json, "application/json");
    });



    // GET /api/shortest?from=X&to=Y
    // Dijkstra: shortest path between two buildings

    svr.Get("/api/shortest", [](const Request& req, Response& res) {
        string src = req.get_param_value("from");
        string dst = req.get_param_value("to");

        if (src.empty() || dst.empty() ||
            !campusGraph.count(src) || !campusGraph.count(dst)) {
            res.set_content("{\"error\":\"Invalid nodes\"}", "application/json");
            return;
        }

        map<string,string> prev;
        auto dist = dijkstra(src, prev);

        if (dist[dst] == INT_MAX) {
            res.set_content("{\"error\":\"No path found\"}", "application/json");
            return;
        }

        auto path = getPath(prev, src, dst);
        string json = "{\"distance\":" + to_string(dist[dst]) + ",\"path\":[";
        for (size_t i = 0; i < path.size(); ++i) {
            if (i) json += ",";
            json += J(path[i]);
        }
        json += "]}";
        res.set_content(json, "application/json");
    });
    // GET /api/dfs?from=X
    // DFS traversal starting from a campus building
   
    svr.Get("/api/dfs", [](const Request& req, Response& res) {
        string src = req.get_param_value("from");

        if (src.empty() || !campusGraph.count(src)) {
            res.set_content("{\"error\":\"Invalid node\"}", "application/json");
            return;
        }

        auto order = dfsExplore(src);
        string json = "{\"order\":[";
        for (size_t i = 0; i < order.size(); ++i) {
            if (i) json += ",";
            json += J(order[i]);
        }
        json += "]}";
        res.set_content(json, "application/json");
    });



    // GET /api/room?id=BT-101&from=MainGate
    // Room details + navigation path from a building

    svr.Get("/api/room", [](const Request& req, Response& res) {
        string roomId = req.get_param_value("id");
        string from   = req.get_param_value("from");

        if (!rooms.count(roomId)) {
            res.set_content("{\"error\":\"Room not found\"}", "application/json");
            return;
        }

        Room& r = rooms[roomId];
        string floorLabel = (r.floor == 0) ? "Ground Floor"
                                           : "Floor " + to_string(r.floor);

        string json = "{\"room\":"     + J(r.id)
                    + ",\"building\":" + J(r.building)
                    + ",\"floor\":"    + J(floorLabel)
                    + ",\"type\":"     + J(r.type);

        // If 'from' building given, compute navigation path
        if (!from.empty() && campusGraph.count(from) && campusGraph.count(r.building)) {
            map<string,string> prev;
            auto dist = dijkstra(from, prev);
            if (dist[r.building] != INT_MAX) {
                auto path = getPath(prev, from, r.building);
                json += ",\"navPath\":[";
                for (size_t i = 0; i < path.size(); ++i) {
                    if (i) json += ",";
                    json += J(path[i]);
                }
                json += "],\"navDist\":" + to_string(dist[r.building]);
            }
        }
        json += "}";
        res.set_content(json, "application/json");
    });


    // GET /api/rooms
    // List all campus rooms (used to populate the rooms table)
   
    svr.Get("/api/rooms", [](const Request&, Response& res) {
        string json = "[";
        bool first = true;
        for (auto& [id, r] : rooms) {
            if (!first) json += ",";
            string floorLabel = (r.floor == 0) ? "Ground Floor"
                                               : "Floor " + to_string(r.floor);
            json += "{\"id\":"       + J(r.id)
                  + ",\"building\":" + J(r.building)
                  + ",\"floor\":"    + J(floorLabel)
                  + ",\"type\":"     + J(r.type) + "}";
            first = false;
        }
        json += "]";
        res.set_content(json, "application/json");
    });


   
    // GET /api/timetable?course=BTECH
    // Return timetable entries for a course

    svr.Get("/api/timetable", [](const Request& req, Response& res) {
        string course = req.get_param_value("course");
        string json = "[";
        bool first = true;
        for (auto& e : timetable) {
            if (!course.empty() && e.course != course) continue;
            if (!first) json += ",";
            json += "{\"course\":"   + J(e.course)
                  + ",\"day\":"      + J(e.day)
                  + ",\"time\":"     + J(e.timeSlot)
                  + ",\"subject\":"  + J(e.subject)
                  + ",\"faculty\":"  + J(e.faculty)
                  + ",\"room\":"     + J(e.room)
                  + ",\"building\":" + J(e.building)
                  + ",\"floor\":"    + to_string(e.floor) + "}";
            first = false;
        }
        json += "]";
        res.set_content(json, "application/json");
    });


    
    // POST /api/login
    // Body: username=X&password=Y

    svr.Post("/api/login", [](const Request& req, Response& res) {
        string user = req.get_param_value("username");
        string pass = req.get_param_value("password");

        auto users = loadUsers();
        if (users.count(user) && users[user].password == pass) {
            res.set_content(
                "{\"ok\":true,\"role\":"     + J(users[user].role) +
                ",\"username\":"             + J(user) + "}",
                "application/json");
        } else {
            res.set_content("{\"ok\":false,\"error\":\"Invalid credentials\"}",
                            "application/json");
        }
    });



    // POST /api/register
    // Body: username password role fullName email
    // New users go into pending.txt until admin approves

    svr.Post("/api/register", [](const Request& req, Response& res) {
        string user  = req.get_param_value("username");
        string pass  = req.get_param_value("password");
        string role  = req.get_param_value("role");
        string name  = req.get_param_value("fullName");
        string email = req.get_param_value("email");

        if (user.empty() || pass.empty() || role.empty()) {
            res.set_content("{\"ok\":false,\"error\":\"Missing fields\"}",
                            "application/json");
            return;
        }

        auto users = loadUsers();
        if (users.count(user)) {
            res.set_content("{\"ok\":false,\"error\":\"Username already exists\"}",
                            "application/json");
            return;
        }

        auto pending = loadPending();
        for (auto& p : pending) {
            if (p.username == user) {
                res.set_content("{\"ok\":false,\"error\":\"Registration already pending\"}",
                                "application/json");
                return;
            }
        }

        addPending({user, pass, role, name, email});
        res.set_content("{\"ok\":true,\"message\":\"Registration submitted. Awaiting admin approval.\"}",
                        "application/json");
    });



    // GET /api/pending
    // Admin: list all pending registrations

    svr.Get("/api/pending", [](const Request&, Response& res) {
        auto pending = loadPending();
        string json = "[";
        bool first = true;
        for (auto& p : pending) {
            if (!first) json += ",";
            json += "{\"username\":" + J(p.username)
                  + ",\"role\":"     + J(p.role)
                  + ",\"fullName\":" + J(p.fullName)
                  + ",\"email\":"    + J(p.email) + "}";
            first = false;
        }
        json += "]";
        res.set_content(json, "application/json");
    });


    // POST /api/approve
    // Body: username=X&action=approve|reject
    svr.Post("/api/approve", [](const Request& req, Response& res) {
        string user   = req.get_param_value("username");
        string action = req.get_param_value("action");

        auto pending = loadPending();
        vector<PendingUser> remaining;
        bool found = false;

        for (auto& p : pending) {
            if (p.username == user) {
                found = true;
                if (action == "approve")
                    saveUser({p.username, p.password, p.role});
                // reject = just don't keep it
            } else {
                remaining.push_back(p);
            }
        }

        savePendingList(remaining);

        if (!found)
            res.set_content("{\"ok\":false,\"error\":\"User not in pending\"}",
                            "application/json");
        else
            res.set_content("{\"ok\":true}", "application/json");
    });


    // GET /api/freerooms?day=Monday&time=09:00-10:00
    // Faculty: classrooms not scheduled at this slot
    svr.Get("/api/freerooms", [](const Request& req, Response& res) {
        string day  = req.get_param_value("day");
        string time = req.get_param_value("time");

        if (day.empty() || time.empty()) {
            res.set_content("{\"error\":\"Provide day and time\"}",
                            "application/json");
            return;
        }

        auto freeList = findFreeRooms(day, time);
        string json = "[";
        for (size_t i = 0; i < freeList.size(); ++i) {
            if (i) json += ",";
            Room& r = rooms[freeList[i]];
            string fl = (r.floor == 0) ? "Ground Floor"
                                       : "Floor " + to_string(r.floor);
            json += "{\"room\":"     + J(r.id)
                  + ",\"building\":" + J(r.building)
                  + ",\"floor\":"    + J(fl) + "}";
        }
        json += "]";
        res.set_content(json, "application/json");
    });


    // GET /api/users
    // Admin: list all approved users (hides admin)
    svr.Get("/api/users", [](const Request&, Response& res) {
        auto users = loadUsers();
        string json = "[";
        bool first = true;
        for (auto& [uname, u] : users) {
            if (u.role == "admin") continue;
            if (!first) json += ",";
            json += "{\"username\":" + J(u.username)
                  + ",\"role\":"     + J(u.role) + "}";
            first = false;
        }
        json += "]";
        res.set_content(json, "application/json");
    });

    ///api/allpaths?from=&to=

    svr.Get("/api/allpaths", [&](const Request& req, Response& res) {
    string src = req.get_param_value("from");
    string dst = req.get_param_value("to");

    if (src.empty() || dst.empty()) {
        res.set_content("{\"error\":\"Missing params\"}", "application/json");
        return;
    }

    auto paths = getAllPaths(src, dst);
    
    // Convert vector<vector<string>> to JSON
    string json = "{\"paths\":[";
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i) json += ",";
        json += "[";
        for (size_t j = 0; j < paths[i].size(); ++j) {
            if (j) json += ",";
            json += J(paths[i][j]);
        }
        json += "]";
    }
    json += "]}";
    res.set_content(json, "application/json");
});
    
    //Start server on port 8080 
    svr.listen("localhost", 8080);
    return 0;
}
