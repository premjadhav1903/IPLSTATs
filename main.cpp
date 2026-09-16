#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <iomanip>
#include <algorithm>
#include <limits>

using namespace std;

// ============================================================
// DELIVERY STRUCTURE
// ============================================================

struct Delivery {
    string match_id;
    string date;
    string match_type;
    string event_name;

    int innings;

    string batting_team;
    string bowling_team;

    int over;
    int ball;
    int ball_no;

    string batter;
    int bat_pos;
    int runs_batter;
    int balls_faced;

    string bowler;

    int valid_ball;
    int runs_extras;
    int runs_total;
    int runs_bowler;
    int runs_not_boundary;

    string extra_type;

    string non_striker;
    int non_striker_pos;

    string wicket_kind;
    string player_out;
    string fielders;

    int runs_target;

    string review_batter;
    string team_reviewed;
    string review_decision;

    string umpire;
    string umpires_call;

    string player_of_match;

    string match_won_by;
    string win_outcome;

    string toss_winner;
    string toss_decision;

    string venue;
    string city;

    int day;
    int month;
    int year;
    int season;

    string gender;
    string team_type;

    string superover_winner;
    string result_type;
    string method;

    int balls_per_over;

    string overs;

    int event_match_no;
    string stage;
    int match_number;

    int team_runs;
    int team_balls;
    int team_wicket;

    string new_batter;
    string power_surge_start;

    int batter_runs;
    int batter_balls;

    int bowler_wicket;

    string batting_partners;
    string striker_out;
    string next_batter;
};


// ============================================================
// GLOBAL DATA
// ============================================================

vector<Delivery> deliveries;


// ============================================================
// CSV PARSER
// ============================================================

vector<string> splitCSVLine(const string& line) {

    vector<string> result;

    string current;

    bool insideQuotes = false;

    for (size_t i = 0; i < line.length(); i++) {

        char c = line[i];

        if (c == '"') {

            if (insideQuotes &&
                i + 1 < line.length() &&
                line[i + 1] == '"') {

                current += '"';
                i++;

            } else {

                insideQuotes = !insideQuotes;
            }

        }

        else if (c == ',' && !insideQuotes) {

            result.push_back(current);
            current.clear();

        }

        else {

            current += c;
        }
    }

    result.push_back(current);

    return result;
}


// ============================================================
// SAFE INTEGER CONVERSION
// ============================================================

int toInt(const string& value) {

    if (value.empty())
        return 0;

    try {

        return stoi(value);

    } catch (...) {

        return 0;
    }
}


// ============================================================
// LOAD IPL DATA
// ============================================================

void loadIPLData() {

    ifstream file("data/IPL.csv");

    if (!file.is_open()) {

        cout << "\nERROR: Could not open data/IPL.csv\n";

        cout << "Make sure IPL.csv is inside the data folder.\n";

        return;
    }

    string line;

    // Skip header
    getline(file, line);

    while (getline(file, line)) {

        vector<string> row = splitCSVLine(line);

        if (row.size() < 64)
            continue;

        Delivery d;

        d.match_id = row[0];
        d.date = row[1];
        d.match_type = row[2];
        d.event_name = row[3];

        d.innings = toInt(row[4]);

        d.batting_team = row[5];
        d.bowling_team = row[6];

        d.over = toInt(row[7]);
        d.ball = toInt(row[8]);
        d.ball_no = toInt(row[9]);

        d.batter = row[10];

        d.bat_pos = toInt(row[11]);

        d.runs_batter = toInt(row[12]);
        d.balls_faced = toInt(row[13]);

        d.bowler = row[14];

        d.valid_ball = toInt(row[15]);

        d.runs_extras = toInt(row[16]);
        d.runs_total = toInt(row[17]);
        d.runs_bowler = toInt(row[18]);
        d.runs_not_boundary = toInt(row[19]);

        d.extra_type = row[20];

        d.non_striker = row[21];

        d.non_striker_pos = toInt(row[22]);

        d.wicket_kind = row[23];
        d.player_out = row[24];
        d.fielders = row[25];

        d.runs_target = toInt(row[26]);

        d.review_batter = row[27];
        d.team_reviewed = row[28];
        d.review_decision = row[29];

        d.umpire = row[30];
        d.umpires_call = row[31];

        d.player_of_match = row[32];

        d.match_won_by = row[33];
        d.win_outcome = row[34];

        d.toss_winner = row[35];
        d.toss_decision = row[36];

        d.venue = row[37];
        d.city = row[38];

        d.day = toInt(row[39]);
        d.month = toInt(row[40]);
        d.year = toInt(row[41]);
        d.season = toInt(row[42]);

        d.gender = row[43];
        d.team_type = row[44];

        d.superover_winner = row[45];

        d.result_type = row[46];
        d.method = row[47];

        d.balls_per_over = toInt(row[48]);

        d.overs = row[49];

        d.event_match_no = toInt(row[50]);

        d.stage = row[51];

        d.match_number = toInt(row[52]);

        d.team_runs = toInt(row[53]);
        d.team_balls = toInt(row[54]);
        d.team_wicket = toInt(row[55]);

        d.new_batter = row[56];
        d.power_surge_start = row[57];

        d.batter_runs = toInt(row[58]);
        d.batter_balls = toInt(row[59]);

        d.bowler_wicket = toInt(row[60]);

        d.batting_partners = row[61];

        d.striker_out = row[62];
        d.next_batter = row[63];

        deliveries.push_back(d);
    }

    file.close();

    cout << "\n============================================\n";
    cout << "       IPL DATA LOADED SUCCESSFULLY\n";
    cout << "============================================\n";

    cout << "Delivery records : "
         << deliveries.size() << "\n";

    cout << "============================================\n";
}


// ============================================================
// 1. SEASON STATISTICS
// ============================================================

void seasonStatistics() {

    int season;

    cout << "\nEnter IPL season: ";
    cin >> season;

    set<string> matchIDs;
    set<string> teams;

    map<pair<string, int>, int> inningsScores;

    int totalRuns = 0;
    int totalSixes = 0;
    int totalFours = 0;

    for (const auto& d : deliveries) {

        if (d.season != season)
            continue;

        matchIDs.insert(d.match_id);

        teams.insert(d.batting_team);

        inningsScores[
            {d.match_id, d.innings}
        ] += d.runs_total;

        totalRuns += d.runs_batter;

        if (d.runs_batter == 6)
            totalSixes++;

        if (d.runs_batter == 4)
            totalFours++;
    }

    if (matchIDs.empty()) {

        cout << "\nSeason not found.\n";

        return;
    }

    int highestScore = 0;
    int lowestScore = 999999;

    int totalInningsRuns = 0;

    for (const auto& innings : inningsScores) {

        int score = innings.second;

        totalInningsRuns += score;

        if (score > highestScore)
            highestScore = score;

        if (score < lowestScore)
            lowestScore = score;
    }

    double averageScore = 0;

    if (!inningsScores.empty()) {

        averageScore =
            static_cast<double>(totalInningsRuns)
            / inningsScores.size();
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "           SEASON STATISTICS\n";
    cout << "============================================\n";

    cout << "Season              : "
         << season << "\n";

    cout << "Matches             : "
         << matchIDs.size() << "\n";

    cout << "Teams               : "
         << teams.size() << "\n";

    cout << "Total Batter Runs   : "
         << totalRuns << "\n";

    cout << "Total Fours         : "
         << totalFours << "\n";

    cout << "Total Sixes         : "
         << totalSixes << "\n";

    cout << "Highest Team Score  : "
         << highestScore << "\n";

    cout << "Lowest Team Score   : "
         << lowestScore << "\n";

    cout << fixed << setprecision(2);

    cout << "Average Score       : "
         << averageScore << "\n";

    cout << "============================================\n";
}


// ============================================================
// 2. PLAYER ANALYTICS
// ============================================================

void playerAnalytics() {

    string player;

    cout << "\nEnter player name: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(cin, player);

    int runs = 0;
    int balls = 0;

    int fours = 0;
    int sixes = 0;

    int wickets = 0;

    int runsConceded = 0;
    int bowlingBalls = 0;

    set<string> matchesPlayed;

    for (const auto& d : deliveries) {

        if (d.batter == player) {

            runs += d.runs_batter;

            /*
             * Count valid balls faced.
             * Wides do not count as balls faced.
             */
            if (d.valid_ball == 1 &&
                d.extra_type != "wides") {

                balls++;
            }

            if (d.runs_batter == 4)
                fours++;

            if (d.runs_batter == 6)
                sixes++;

            matchesPlayed.insert(d.match_id);
        }

        if (d.bowler == player) {

            runsConceded += d.runs_bowler;

            bowlingBalls += d.valid_ball;

            if (d.bowler_wicket == 1)
                wickets++;

            matchesPlayed.insert(d.match_id);
        }
    }

    if (matchesPlayed.empty()) {

        cout << "\nPlayer not found.\n";

        return;
    }

    double strikeRate = 0;

    if (balls > 0) {

        strikeRate =
            (static_cast<double>(runs) / balls) * 100;
    }

    double economy = 0;

    if (bowlingBalls > 0) {

        economy =
            (static_cast<double>(runsConceded) * 6)
            / bowlingBalls;
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "             PLAYER ANALYTICS\n";
    cout << "============================================\n";

    cout << "Player              : "
         << player << "\n";

    cout << "Matches Played      : "
         << matchesPlayed.size() << "\n";

    cout << "\n--- BATTING ---\n";

    cout << "Runs                : "
         << runs << "\n";

    cout << "Balls Faced         : "
         << balls << "\n";

    cout << "Fours               : "
         << fours << "\n";

    cout << "Sixes               : "
         << sixes << "\n";

    cout << fixed << setprecision(2);

    cout << "Strike Rate         : "
         << strikeRate << "\n";

    cout << "\n--- BOWLING ---\n";

    cout << "Wickets             : "
         << wickets << "\n";

    cout << "Runs Conceded       : "
         << runsConceded << "\n";

    cout << "Bowling Balls       : "
         << bowlingBalls << "\n";

    cout << "Economy Rate        : "
         << economy << "\n";

    cout << "============================================\n";
}


// ============================================================
// 3. TEAM ANALYTICS
// ============================================================

void teamAnalytics() {

    string team;

    cout << "\nEnter team name: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(cin, team);

    set<string> matchesPlayed;

    map<string, string> matchWinners;

    int totalRuns = 0;

    map<string, int> teamMatchScores;

    for (const auto& d : deliveries) {

        if (d.batting_team != team &&
            d.bowling_team != team) {

            continue;
        }

        matchesPlayed.insert(d.match_id);

        if (d.batting_team == team) {

            totalRuns += d.runs_total;

            teamMatchScores[d.match_id]
                += d.runs_total;
        }

        if (!d.match_won_by.empty()) {

            matchWinners[d.match_id]
                = d.match_won_by;
        }
    }

    if (matchesPlayed.empty()) {

        cout << "\nTeam not found.\n";

        return;
    }

    int highestScore = 0;

    for (const auto& score : teamMatchScores) {

        if (score.second > highestScore)
            highestScore = score.second;
    }

    int wins = 0;
    int losses = 0;

    for (const auto& match : matchWinners) {

        if (match.second == team)
            wins++;

        else
            losses++;
    }

    double winPercentage = 0;

    if (!matchesPlayed.empty()) {

        winPercentage =
            (static_cast<double>(wins)
             / matchesPlayed.size()) * 100;
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "              TEAM ANALYTICS\n";
    cout << "============================================\n";

    cout << "Team                : "
         << team << "\n";

    cout << "Matches Played      : "
         << matchesPlayed.size() << "\n";

    cout << "Wins                : "
         << wins << "\n";

    cout << "Losses              : "
         << losses << "\n";

    cout << fixed << setprecision(2);

    cout << "Win Percentage      : "
         << winPercentage << "%\n";

    cout << "Total Runs          : "
         << totalRuns << "\n";

    cout << "Highest Team Score  : "
         << highestScore << "\n";

    cout << "============================================\n";
}


// ============================================================
// 4. MATCH SEARCH
// ============================================================

void matchSearch() {

    string search;

    cout << "\nEnter team/player/venue/city to search: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(cin, search);

    set<string> foundMatches;

    for (const auto& d : deliveries) {

        bool found = false;

        if (d.batting_team.find(search)
            != string::npos)

            found = true;

        if (d.bowling_team.find(search)
            != string::npos)

            found = true;

        if (d.batter.find(search)
            != string::npos)

            found = true;

        if (d.bowler.find(search)
            != string::npos)

            found = true;

        if (d.venue.find(search)
            != string::npos)

            found = true;

        if (d.city.find(search)
            != string::npos)

            found = true;

        if (found)
            foundMatches.insert(d.match_id);
    }

    if (foundMatches.empty()) {

        cout << "\nNo matching records found.\n";

        return;
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "              MATCH SEARCH\n";
    cout << "============================================\n";

    cout << "Search              : "
         << search << "\n";

    cout << "Matches Found       : "
         << foundMatches.size() << "\n";

    cout << "\nMatch IDs:\n";

    int count = 0;

    for (const auto& id : foundMatches) {

        cout << "- " << id << "\n";

        count++;

        if (count >= 30) {

            cout << "\nShowing first 30 matches.\n";

            break;
        }
    }

    cout << "============================================\n";
}


// ============================================================
// 5. TOP 10 BATSMEN
// ============================================================

void topBatsmen() {

    map<string, int> playerRuns;

    for (const auto& d : deliveries) {

        if (!d.batter.empty()) {

            playerRuns[d.batter]
                += d.runs_batter;
        }
    }

    vector<pair<string, int>> ranking(
        playerRuns.begin(),
        playerRuns.end()
    );

    sort(ranking.begin(), ranking.end(),

        [](const auto& a, const auto& b) {

            if (a.second != b.second)
                return a.second > b.second;

            return a.first < b.first;
        }
    );

    cout << "\n";

    cout << "============================================\n";
    cout << "             TOP 10 BATSMEN\n";
    cout << "============================================\n";

    int position = 1;

    for (const auto& player : ranking) {

        cout << position
             << ". "
             << player.first
             << " - "
             << player.second
             << " runs\n";

        position++;

        if (position > 10)
            break;
    }

    cout << "============================================\n";
}


// ============================================================
// 6. TOP 10 BOWLERS
// ============================================================

void topBowlers() {

    map<string, int> playerWickets;

    for (const auto& d : deliveries) {

        if (!d.bowler.empty() &&
            d.bowler_wicket == 1) {

            playerWickets[d.bowler]++;
        }
    }

    vector<pair<string, int>> ranking(
        playerWickets.begin(),
        playerWickets.end()
    );

    sort(ranking.begin(), ranking.end(),

        [](const auto& a, const auto& b) {

            if (a.second != b.second)
                return a.second > b.second;

            return a.first < b.first;
        }
    );

    cout << "\n";

    cout << "============================================\n";
    cout << "             TOP 10 BOWLERS\n";
    cout << "============================================\n";

    int position = 1;

    for (const auto& player : ranking) {

        cout << position
             << ". "
             << player.first
             << " - "
             << player.second
             << " wickets\n";

        position++;

        if (position > 10)
            break;
    }

    cout << "============================================\n";
}


// ============================================================
// 7. MOST SIXES
// ============================================================

void mostSixes() {

    map<string, int> playerSixes;

    for (const auto& d : deliveries) {

        if (!d.batter.empty() &&
            d.runs_batter == 6) {

            playerSixes[d.batter]++;
        }
    }

    vector<pair<string, int>> ranking(
        playerSixes.begin(),
        playerSixes.end()
    );

    sort(ranking.begin(), ranking.end(),

        [](const auto& a, const auto& b) {

            if (a.second != b.second)
                return a.second > b.second;

            return a.first < b.first;
        }
    );

    cout << "\n";

    cout << "============================================\n";
    cout << "               MOST SIXES\n";
    cout << "============================================\n";

    int position = 1;

    for (const auto& player : ranking) {

        cout << position
             << ". "
             << player.first
             << " - "
             << player.second
             << " sixes\n";

        position++;

        if (position > 10)
            break;
    }

    cout << "============================================\n";
}


// ============================================================
// 8. MOST FOURS
// ============================================================

void mostFours() {

    map<string, int> playerFours;

    for (const auto& d : deliveries) {

        if (!d.batter.empty() &&
            d.runs_batter == 4) {

            playerFours[d.batter]++;
        }
    }

    vector<pair<string, int>> ranking(
        playerFours.begin(),
        playerFours.end()
    );

    sort(ranking.begin(), ranking.end(),

        [](const auto& a, const auto& b) {

            if (a.second != b.second)
                return a.second > b.second;

            return a.first < b.first;
        }
    );

    cout << "\n";

    cout << "============================================\n";
    cout << "               MOST FOURS\n";
    cout << "============================================\n";

    int position = 1;

    for (const auto& player : ranking) {

        cout << position
             << ". "
             << player.first
             << " - "
             << player.second
             << " fours\n";

        position++;

        if (position > 10)
            break;
    }

    cout << "============================================\n";
}


// ============================================================
// PLAYER STATS HELPER
// ============================================================

struct PlayerStats {

    int runs = 0;
    int balls = 0;

    int fours = 0;
    int sixes = 0;

    int wickets = 0;

    int runsConceded = 0;
    int bowlingBalls = 0;

    set<string> matches;
};


PlayerStats getPlayerStats(const string& player) {

    PlayerStats stats;

    for (const auto& d : deliveries) {

        if (d.batter == player) {

            stats.runs += d.runs_batter;

            if (d.valid_ball == 1 &&
                d.extra_type != "wides") {

                stats.balls++;
            }

            if (d.runs_batter == 4)
                stats.fours++;

            if (d.runs_batter == 6)
                stats.sixes++;

            stats.matches.insert(d.match_id);
        }

        if (d.bowler == player) {

            stats.runsConceded += d.runs_bowler;

            stats.bowlingBalls += d.valid_ball;

            if (d.bowler_wicket == 1)
                stats.wickets++;

            stats.matches.insert(d.match_id);
        }
    }

    return stats;
}


// ============================================================
// 9. PLAYER COMPARISON
// ============================================================

void playerComparison() {

    string player1;
    string player2;

    cout << "\nEnter first player: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(cin, player1);

    cout << "Enter second player: ";

    getline(cin, player2);

    PlayerStats p1 = getPlayerStats(player1);
    PlayerStats p2 = getPlayerStats(player2);

    if (p1.matches.empty()) {

        cout << "\nFirst player not found.\n";

        return;
    }

    if (p2.matches.empty()) {

        cout << "\nSecond player not found.\n";

        return;
    }

    double sr1 = 0;
    double sr2 = 0;

    double eco1 = 0;
    double eco2 = 0;

    if (p1.balls > 0)
        sr1 =
            static_cast<double>(p1.runs)
            / p1.balls * 100;

    if (p2.balls > 0)
        sr2 =
            static_cast<double>(p2.runs)
            / p2.balls * 100;

    if (p1.bowlingBalls > 0)
        eco1 =
            static_cast<double>(p1.runsConceded)
            * 6 / p1.bowlingBalls;

    if (p2.bowlingBalls > 0)
        eco2 =
            static_cast<double>(p2.runsConceded)
            * 6 / p2.bowlingBalls;

    cout << "\n";

    cout << "============================================================\n";
    cout << "                 PLAYER COMPARISON\n";
    cout << "============================================================\n";

    cout << left
         << setw(25) << "Statistic"
         << setw(20) << player1
         << setw(20) << player2
         << "\n";

    cout << "------------------------------------------------------------\n";

    cout << setw(25) << "Matches"
         << setw(20) << p1.matches.size()
         << setw(20) << p2.matches.size()
         << "\n";

    cout << setw(25) << "Runs"
         << setw(20) << p1.runs
         << setw(20) << p2.runs
         << "\n";

    cout << setw(25) << "Balls Faced"
         << setw(20) << p1.balls
         << setw(20) << p2.balls
         << "\n";

    cout << setw(25) << "Fours"
         << setw(20) << p1.fours
         << setw(20) << p2.fours
         << "\n";

    cout << setw(25) << "Sixes"
         << setw(20) << p1.sixes
         << setw(20) << p2.sixes
         << "\n";

    cout << fixed << setprecision(2);

    cout << setw(25) << "Strike Rate"
         << setw(20) << sr1
         << setw(20) << sr2
         << "\n";

    cout << setw(25) << "Wickets"
         << setw(20) << p1.wickets
         << setw(20) << p2.wickets
         << "\n";

    cout << setw(25) << "Runs Conceded"
         << setw(20) << p1.runsConceded
         << setw(20) << p2.runsConceded
         << "\n";

    cout << setw(25) << "Economy"
         << setw(20) << eco1
         << setw(20) << eco2
         << "\n";

    cout << "============================================================\n";
}


// ============================================================
// TEAM STATS HELPER
// ============================================================

struct TeamStats {

    int matches = 0;
    int wins = 0;
    int losses = 0;

    int totalRuns = 0;

    int highestScore = 0;

    set<string> matchIDs;
};


TeamStats getTeamStats(const string& team) {

    TeamStats stats;

    map<string, string> winners;

    map<string, int> matchScores;

    for (const auto& d : deliveries) {

        if (d.batting_team != team &&
            d.bowling_team != team)

            continue;

        stats.matchIDs.insert(d.match_id);

        if (d.batting_team == team) {

            stats.totalRuns += d.runs_total;

            matchScores[d.match_id]
                += d.runs_total;
        }

        if (!d.match_won_by.empty()) {

            winners[d.match_id]
                = d.match_won_by;
        }
    }

    stats.matches = stats.matchIDs.size();

    for (const auto& score : matchScores) {

        if (score.second > stats.highestScore)
            stats.highestScore = score.second;
    }

    for (const auto& winner : winners) {

        if (winner.second == team)
            stats.wins++;

        else
            stats.losses++;
    }

    return stats;
}


// ============================================================
// 10. TEAM VS TEAM
// ============================================================

void teamVsTeam() {

    string team1;
    string team2;

    cout << "\nEnter first team: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(cin, team1);

    cout << "Enter second team: ";

    getline(cin, team2);

    TeamStats t1 = getTeamStats(team1);
    TeamStats t2 = getTeamStats(team2);

    if (t1.matches == 0) {

        cout << "\nFirst team not found.\n";

        return;
    }

    if (t2.matches == 0) {

        cout << "\nSecond team not found.\n";

        return;
    }

    int headToHead = 0;

    int team1Wins = 0;
    int team2Wins = 0;

    set<string> commonMatches;

    for (const auto& id : t1.matchIDs) {

        if (t2.matchIDs.count(id))
            commonMatches.insert(id);
    }

    for (const auto& id : commonMatches) {

        for (const auto& d : deliveries) {

            if (d.match_id == id &&
                !d.match_won_by.empty()) {

                if (d.match_won_by == team1)
                    team1Wins++;

                else if (d.match_won_by == team2)
                    team2Wins++;

                break;
            }
        }

        headToHead++;
    }

    cout << "\n";

    cout << "============================================================\n";
    cout << "                    TEAM VS TEAM\n";
    cout << "============================================================\n";

    cout << "Team 1              : "
         << team1 << "\n";

    cout << "Team 2              : "
         << team2 << "\n";

    cout << "Head-to-Head Matches: "
         << headToHead << "\n";

    cout << "\n";

    cout << team1
         << " Wins           : "
         << team1Wins << "\n";

    cout << team2
         << " Wins           : "
         << team2Wins << "\n";

    cout << "\n--- OVERALL TEAM STATS ---\n";

    cout << team1
         << " Matches         : "
         << t1.matches << "\n";

    cout << team1
         << " Total Runs      : "
         << t1.totalRuns << "\n";

    cout << team1
         << " Highest Score   : "
         << t1.highestScore << "\n";

    cout << "\n";

    cout << team2
         << " Matches         : "
         << t2.matches << "\n";

    cout << team2
         << " Total Runs      : "
         << t2.totalRuns << "\n";

    cout << team2
         << " Highest Score   : "
         << t2.highestScore << "\n";

    cout << "============================================================\n";
}


// ============================================================
// 11. VENUE ANALYTICS
// ============================================================

void venueAnalytics() {

    string venue;

    cout << "\nEnter venue name: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(cin, venue);

    set<string> matches;

    set<string> teams;

    map<pair<string, int>, int> inningsScores;

    for (const auto& d : deliveries) {

        if (d.venue.find(venue) == string::npos)
            continue;

        matches.insert(d.match_id);

        teams.insert(d.batting_team);

        inningsScores[
            {d.match_id, d.innings}
        ] += d.runs_total;
    }

    if (matches.empty()) {

        cout << "\nVenue not found.\n";

        return;
    }

    int highestScore = 0;
    int lowestScore = 999999;

    int totalRuns = 0;

    for (const auto& entry : inningsScores) {

        int score = entry.second;

        totalRuns += score;

        if (score > highestScore)
            highestScore = score;

        if (score < lowestScore)
            lowestScore = score;
    }

    double averageScore = 0;

    if (!inningsScores.empty()) {

        averageScore =
            static_cast<double>(totalRuns)
            / inningsScores.size();
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "             VENUE ANALYTICS\n";
    cout << "============================================\n";

    cout << "Venue               : "
         << venue << "\n";

    cout << "Matches             : "
         << matches.size() << "\n";

    cout << "Teams Played        : "
         << teams.size() << "\n";

    cout << "Highest Score       : "
         << highestScore << "\n";

    cout << "Lowest Score        : "
         << lowestScore << "\n";

    cout << fixed << setprecision(2);

    cout << "Average Score       : "
         << averageScore << "\n";

    cout << "============================================\n";
}


// ============================================================
// 12. TOSS ANALYSIS
// ============================================================

void tossAnalysis() {

    map<string, int> tossWins;

    map<string, int> tossDecisions;

    map<string, int> tossAndMatchWin;

    set<string> matches;

    for (const auto& d : deliveries) {

        if (d.match_id.empty())
            continue;

        matches.insert(d.match_id);

        if (!d.toss_winner.empty()) {

            tossWins[d.toss_winner]++;
        }

        if (!d.toss_decision.empty()) {

            tossDecisions[d.toss_decision]++;
        }

        if (!d.toss_winner.empty() &&
            d.match_won_by == d.toss_winner) {

            tossAndMatchWin[d.toss_winner]++;
        }
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "               TOSS ANALYSIS\n";
    cout << "============================================\n";

    cout << "Total Matches      : "
         << matches.size() << "\n";

    cout << "\n--- TOSS DECISIONS ---\n";

    for (const auto& decision : tossDecisions) {

        cout << decision.first
             << " : "
             << decision.second
             << "\n";
    }

    vector<pair<string, int>> ranking(
        tossWins.begin(),
        tossWins.end()
    );

    sort(ranking.begin(), ranking.end(),

        [](const auto& a, const auto& b) {

            return a.second > b.second;
        }
    );

    cout << "\n--- MOST TOSS WINS ---\n";

    int position = 1;

    for (const auto& team : ranking) {

        cout << position
             << ". "
             << team.first
             << " - "
             << team.second
             << "\n";

        position++;

        if (position > 10)
            break;
    }

    cout << "\n--- TOSS WINNER ALSO WON MATCH ---\n";

    for (const auto& team : tossAndMatchWin) {

        cout << team.first
             << " : "
             << team.second
             << " matches\n";
    }

    cout << "============================================\n";
}


// ============================================================
// 13. MATCH SUMMARY
// ============================================================

void matchSummary() {

    string matchID;

    cout << "\nEnter Match ID: ";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(cin, matchID);

    set<string> teams;

    string date;
    string venue;
    string city;

    string winner;
    string playerOfMatch;

    string tossWinner;
    string tossDecision;

    string resultType;

    map<string, int> teamScores;

    bool found = false;

    for (const auto& d : deliveries) {

        if (d.match_id != matchID)
            continue;

        found = true;

        teams.insert(d.batting_team);

        teamScores[d.batting_team]
            += d.runs_total;

        if (date.empty())
            date = d.date;

        if (venue.empty())
            venue = d.venue;

        if (city.empty())
            city = d.city;

        if (!d.match_won_by.empty())
            winner = d.match_won_by;

        if (!d.player_of_match.empty())
            playerOfMatch = d.player_of_match;

        if (!d.toss_winner.empty())
            tossWinner = d.toss_winner;

        if (!d.toss_decision.empty())
            tossDecision = d.toss_decision;

        if (!d.result_type.empty())
            resultType = d.result_type;
    }

    if (!found) {

        cout << "\nMatch not found.\n";

        return;
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "               MATCH SUMMARY\n";
    cout << "============================================\n";

    cout << "Match ID            : "
         << matchID << "\n";

    cout << "Date                : "
         << date << "\n";

    cout << "Venue               : "
         << venue << "\n";

    cout << "City                : "
         << city << "\n";

    cout << "\n--- TEAMS ---\n";

    for (const auto& team : teams) {

        cout << "- "
             << team << "\n";
    }

    cout << "\n--- SCORES ---\n";

    for (const auto& score : teamScores) {

        cout << score.first
             << " : "
             << score.second
             << "\n";
    }

    cout << "\nWinner              : "
         << winner << "\n";

    cout << "Player of Match     : "
         << playerOfMatch << "\n";

    cout << "Toss Winner         : "
         << tossWinner << "\n";

    cout << "Toss Decision       : "
         << tossDecision << "\n";

    cout << "Result Type         : "
         << resultType << "\n";

    cout << "============================================\n";
}


// ============================================================
// 14. SEASON COMPARISON
// ============================================================

struct SeasonStats {

    int matches = 0;
    int teams = 0;

    int batterRuns = 0;

    int fours = 0;
    int sixes = 0;

    int highestScore = 0;

    double averageScore = 0;
};


SeasonStats getSeasonStats(int season) {

    SeasonStats stats;

    set<string> matches;

    set<string> teams;

    map<pair<string, int>, int> inningsScores;

    for (const auto& d : deliveries) {

        if (d.season != season)
            continue;

        matches.insert(d.match_id);

        teams.insert(d.batting_team);

        stats.batterRuns += d.runs_batter;

        if (d.runs_batter == 4)
            stats.fours++;

        if (d.runs_batter == 6)
            stats.sixes++;

        inningsScores[
            {d.match_id, d.innings}
        ] += d.runs_total;
    }

    stats.matches = matches.size();

    stats.teams = teams.size();

    int totalScore = 0;

    for (const auto& score : inningsScores) {

        totalScore += score.second;

        if (score.second > stats.highestScore)
            stats.highestScore = score.second;
    }

    if (!inningsScores.empty()) {

        stats.averageScore =
            static_cast<double>(totalScore)
            / inningsScores.size();
    }

    return stats;
}


void seasonComparison() {

    int season1;
    int season2;

    cout << "\nEnter first season: ";
    cin >> season1;

    cout << "Enter second season: ";
    cin >> season2;

    SeasonStats s1 = getSeasonStats(season1);
    SeasonStats s2 = getSeasonStats(season2);

    if (s1.matches == 0) {

        cout << "\nFirst season not found.\n";

        return;
    }

    if (s2.matches == 0) {

        cout << "\nSecond season not found.\n";

        return;
    }

    cout << "\n";

    cout << "============================================================\n";
    cout << "                  SEASON COMPARISON\n";
    cout << "============================================================\n";

    cout << left
         << setw(25) << "Statistic"
         << setw(20) << season1
         << setw(20) << season2
         << "\n";

    cout << "------------------------------------------------------------\n";

    cout << setw(25) << "Matches"
         << setw(20) << s1.matches
         << setw(20) << s2.matches
         << "\n";

    cout << setw(25) << "Teams"
         << setw(20) << s1.teams
         << setw(20) << s2.teams
         << "\n";

    cout << setw(25) << "Batter Runs"
         << setw(20) << s1.batterRuns
         << setw(20) << s2.batterRuns
         << "\n";

    cout << setw(25) << "Fours"
         << setw(20) << s1.fours
         << setw(20) << s2.fours
         << "\n";

    cout << setw(25) << "Sixes"
         << setw(20) << s1.sixes
         << setw(20) << s2.sixes
         << "\n";

    cout << setw(25) << "Highest Score"
         << setw(20) << s1.highestScore
         << setw(20) << s2.highestScore
         << "\n";

    cout << fixed << setprecision(2);

    cout << setw(25) << "Average Score"
         << setw(20) << s1.averageScore
         << setw(20) << s2.averageScore
         << "\n";

    cout << "============================================================\n";
}


// ============================================================
// 15. DATASET STATISTICS
// ============================================================

void datasetStatistics() {

    set<string> matches;

    set<string> players;

    set<string> teams;

    set<int> seasons;

    set<string> venues;

    long long totalRuns = 0;

    long long totalFours = 0;

    long long totalSixes = 0;

    long long totalWickets = 0;

    for (const auto& d : deliveries) {

        if (!d.match_id.empty())
            matches.insert(d.match_id);

        if (!d.batter.empty())
            players.insert(d.batter);

        if (!d.batting_team.empty())
            teams.insert(d.batting_team);

        if (d.season != 0)
            seasons.insert(d.season);

        if (!d.venue.empty())
            venues.insert(d.venue);

        totalRuns += d.runs_total;

        if (d.runs_batter == 4)
            totalFours++;

        if (d.runs_batter == 6)
            totalSixes++;

        if (d.bowler_wicket == 1)
            totalWickets++;
    }

    cout << "\n";

    cout << "============================================\n";
    cout << "             DATASET STATISTICS\n";
    cout << "============================================\n";

    cout << "Delivery Records    : "
         << deliveries.size() << "\n";

    cout << "Matches             : "
         << matches.size() << "\n";

    cout << "Players             : "
         << players.size() << "\n";

    cout << "Teams               : "
         << teams.size() << "\n";

    cout << "Seasons             : "
         << seasons.size() << "\n";

    cout << "Venues              : "
         << venues.size() << "\n";

    cout << "Total Runs          : "
         << totalRuns << "\n";

    cout << "Total Fours         : "
         << totalFours << "\n";

    cout << "Total Sixes         : "
         << totalSixes << "\n";

    cout << "Total Wickets       : "
         << totalWickets << "\n";

    if (!seasons.empty()) {

        cout << "First Season        : "
             << *seasons.begin() << "\n";

        cout << "Latest Season       : "
             << *seasons.rbegin() << "\n";
    }

    cout << "============================================\n";
}


// ============================================================
// MENU
// ============================================================

void menu() {

    int choice;

    while (true) {

        cout << "\n\n";

        cout << "============================================\n";
        cout << "            IPL STATS ANALYTICS\n";
        cout << "============================================\n";

        cout << "1.  Season Statistics\n";
        cout << "2.  Player Analytics\n";
        cout << "3.  Team Analytics\n";
        cout << "4.  Match Search\n";

        cout << "\n";

        cout << "5.  Top 10 Batsmen\n";
        cout << "6.  Top 10 Bowlers\n";
        cout << "7.  Most Sixes\n";
        cout << "8.  Most Fours\n";

        cout << "\n";

        cout << "9.  Player Comparison\n";
        cout << "10. Team vs Team\n";

        cout << "\n";

        cout << "11. Venue Analytics\n";
        cout << "12. Toss Analysis\n";
        cout << "13. Match Summary\n";
        cout << "14. Season Comparison\n";

        cout << "\n";

        cout << "15. Dataset Statistics\n";
        cout << "16. Exit\n";

        cout << "============================================\n";

        cout << "Enter your choice: ";

        cin >> choice;

        switch (choice) {

            case 1:
                seasonStatistics();
                break;

            case 2:
                playerAnalytics();
                break;

            case 3:
                teamAnalytics();
                break;

            case 4:
                matchSearch();
                break;

            case 5:
                topBatsmen();
                break;

            case 6:
                topBowlers();
                break;

            case 7:
                mostSixes();
                break;

            case 8:
                mostFours();
                break;

            case 9:
                playerComparison();
                break;

            case 10:
                teamVsTeam();
                break;

            case 11:
                venueAnalytics();
                break;

            case 12:
                tossAnalysis();
                break;

            case 13:
                matchSummary();
                break;

            case 14:
                seasonComparison();
                break;

            case 15:
                datasetStatistics();
                break;

            case 16:

                cout << "\nExiting IPL Stats Analytics...\n";

                return;

            default:

                cout << "\nInvalid choice. Try again.\n";
        }
    }
}


// ============================================================
// MAIN
// ============================================================

int main() {

    cout << "\n";

    cout << "============================================\n";
    cout << "            IPL STATS ANALYTICS\n";
    cout << "============================================\n";

    cout << "\nLoading IPL dataset...\n";

    loadIPLData();

    if (deliveries.empty()) {

        cout << "\nNo data loaded.\n";

        cout << "Please check the location of IPL.csv.\n";

        return 1;
    }

    cout << "\nStarting analytics system...\n";

    menu();

    return 0;
}