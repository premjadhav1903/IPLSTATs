#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <cctype>
#include <cstdlib>

#if defined(_WIN32)
    #include <direct.h>
    #define MAKE_DIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #define MAKE_DIR(path) mkdir(path, 0755)
#endif

using namespace std;

// Creates the folder if it doesn't exist yet. Ignores the error if it already
// exists (that's the common case on every run after the first).
void ensureDirectoryExists(const string& path)
{
    MAKE_DIR(path.c_str());
}

// ============================================================
// STRUCTURES
// ============================================================

// One row of data/IPL.csv -> one ball bowled.
struct Delivery
{
    string match_id;
    string date;
    string match_type;
    string event_name;

    int innings = 0;

    string batting_team;
    string bowling_team;

    int over = 0;
    int ball = 0;
    double ball_no = 0;

    string batter;

    int bat_pos = 0;
    int runs_batter = 0;
    int balls_faced = 0;

    string bowler;

    int valid_ball = 0;

    int runs_extras = 0;
    int runs_total = 0;
    int runs_bowler = 0;
    int runs_not_boundary = 0;

    string extra_type;

    string non_striker;
    int non_striker_pos = 0;

    string wicket_kind;
    string player_out;
    string fielders;

    int runs_target = 0;

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

    int day = 0;
    int month = 0;
    int year = 0;
    int season = 0;

    string gender;
    string team_type;

    string superover_winner;

    string result_type;
    string method;

    int balls_per_over = 6;
    string overs;

    int event_match_no = 0;
    string stage;
    int match_number = 0;

    int team_runs = 0;
    int team_balls = 0;
    int team_wicket = 0;

    string new_batter;
    string power_surge_start;

    int batter_runs = 0;
    int batter_balls = 0;

    int bowler_wicket = 0;

    string batting_partners;
    string striker_out;
    string next_batter;
};


// One row of data/IPL_Innings_Scores.csv or data/IPL_Lowest_Scores.csv
// -> one team's total for one innings of one match.
struct InningsScore
{
    string match_id;
    int innings = 0;

    string batting_team;
    string bowling_team;

    int season = 0;
    string date;

    string venue;
    string city;

    string winner;
    string result_type;

    string toss_winner;
    string toss_decision;

    string player_of_match;
    string stage;
    int match_number = 0;

    int total_runs = 0;
    int batter_runs = 0;   // runs scored off the bat (extras excluded)
    int balls = 0;
    int wickets = 0;

    int fours = 0;
    int sixes = 0;

    bool completed_match = true;
};


struct PlayerStats
{
    int runs = 0;
    int balls = 0;
    int fours = 0;
    int sixes = 0;

    int wickets = 0;
    int runsConceded = 0;
    int bowlingBalls = 0;

    set<string> matches;
};


struct TeamStats
{
    int matches = 0;
    int wins = 0;
    int losses = 0;

    long long totalRuns = 0;
    int highestScore = 0;

    set<string> matchIDs;
};


// ============================================================
// GLOBAL DATA
// ============================================================

vector<Delivery> deliveries;

vector<InningsScore> inningsScores;   // data/IPL_Innings_Scores.csv  (full dataset)
vector<InningsScore> lowestScoreData; // data/IPL_Lowest_Scores.csv   (completed innings only -
                                       // curated so freak abandoned/rain-hit innings don't
                                       // pollute "lowest score" style records)

// match_id -> season, built from deliveries (data/IPL.csv), which always carries a season
// for every match. Used to patch up rows in the innings-score files where the season
// column itself is blank.
unordered_map<string, int> matchSeasonMap;

// match_id -> winning team, built once from deliveries and reused everywhere instead of
// re-scanning all ~295k delivery rows for every lookup.
unordered_map<string, string> matchWinnerMap;


// ============================================================
// SMALL UTILITIES
// ============================================================

vector<string> splitCSVLine(const string& line)
{
    vector<string> result;
    string current;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.length(); i++)
    {
        char c = line[i];

        if (c == '"')
        {
            if (insideQuotes && i + 1 < line.length() && line[i + 1] == '"')
            {
                current += '"';
                i++;
            }
            else
            {
                insideQuotes = !insideQuotes;
            }
        }
        else if (c == ',' && !insideQuotes)
        {
            result.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    result.push_back(current);
    return result;
}

// Strips a trailing '\r' (Windows line ending) so the last column of a row never
// silently carries a stray character into string comparisons.
string stripLineEnding(string line)
{
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.pop_back();
    return line;
}

string trim(const string& value)
{
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

// Safe int parse. Handles empty strings, "Unknown"-style placeholders, and
// float-looking values like "2013.0" (stoi simply reads the leading digits).
int toInt(const string& rawValue)
{
    string value = trim(rawValue);

    if (value.empty())
        return 0;

    try
    {
        return stoi(value);
    }
    catch (...)
    {
        return 0;
    }
}

string lowerString(string value)
{
    transform(value.begin(), value.end(), value.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return value;
}

// Case-insensitive substring search, used for free-text lookups (venue, city, search box)
// so the user doesn't need to match the dataset's exact capitalization.
bool containsIgnoreCase(const string& haystack, const string& needle)
{
    if (needle.empty())
        return true;

    string h = lowerString(haystack);
    string n = lowerString(needle);
    return h.find(n) != string::npos;
}

// Tries each candidate path in turn and opens the first one that exists. This makes the
// program work whether the CSVs live in a "data/" subfolder or right next to the binary.
bool openFirstAvailable(ifstream& file, const vector<string>& candidates, string& usedPath)
{
    for (const auto& path : candidates)
    {
        file.open(path);

        if (file.is_open())
        {
            usedPath = path;
            return true;
        }

        file.clear();
    }

    return false;
}


// ============================================================
// LOAD MAIN BALL-BY-BALL DATASET (data/IPL.csv)
// ============================================================

// Parses "season" values in either the plain "2016" form or the crossover
// "2007/08" form (older seasons that spanned two calendar years). We keep the
// ending year as the season number, e.g. "2007/08" -> 2008.
int parseSeasonField(const string& rawValue)
{
    string seasonValue = trim(rawValue);

    if (seasonValue.empty())
        return 0;

    size_t slash = seasonValue.find('/');

    if (slash != string::npos)
    {
        string startYear = seasonValue.substr(0, slash);
        string endYear = seasonValue.substr(slash + 1);

        if (endYear.length() == 2)
            endYear = startYear.substr(0, 2) + endYear;

        return toInt(endYear);
    }

    return toInt(seasonValue);
}

bool loadIPLData()
{
    ifstream file;
    string usedPath;

    if (!openFirstAvailable(file, {"data/IPL.csv", "IPL.csv"}, usedPath))
    {
        cout << "\nERROR: Could not open IPL.csv (looked in data/IPL.csv and ./IPL.csv)\n";
        return false;
    }

    string line;
    getline(file, line); // header

    while (getline(file, line))
    {
        line = stripLineEnding(line);

        if (line.empty())
            continue;

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
        d.ball_no = toInt(row[9]); // kept as int-precision via toInt; fine for sorting/display

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
        d.season = parseSeasonField(row[42]);

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
    cout << "Source            : " << usedPath << "\n";
    cout << "Delivery records  : " << deliveries.size() << "\n";
    cout << "============================================\n";

    return true;
}


// Build match_id -> season and match_id -> winner lookups once, from the
// ball-by-ball dataset (which always has both fields populated).
void buildMatchLookups()
{
    for (const auto& d : deliveries)
    {
        if (d.match_id.empty())
            continue;

        if (d.season != 0 && matchSeasonMap.find(d.match_id) == matchSeasonMap.end())
            matchSeasonMap[d.match_id] = d.season;

        if (!d.match_won_by.empty() && matchWinnerMap.find(d.match_id) == matchWinnerMap.end())
            matchWinnerMap[d.match_id] = d.match_won_by;
    }
}


// ============================================================
// LOAD INNINGS-LEVEL SCORE DATASETS
// (data/IPL_Innings_Scores.csv and data/IPL_Lowest_Scores.csv share the exact
//  same column layout, so one loader handles both.)
// ============================================================

int loadScoreCSV(const vector<string>& candidatePaths, vector<InningsScore>& target)
{
    ifstream file;
    string usedPath;

    if (!openFirstAvailable(file, candidatePaths, usedPath))
        return -1;

    string line;
    getline(file, line); // header

    while (getline(file, line))
    {
        line = stripLineEnding(line);

        if (line.empty())
            continue;

        vector<string> row = splitCSVLine(line);

        // Real column layout (verified against the actual CSV header):
        //  0 match_id        1 innings          2 batting_team     3 bowling_team
        //  4 season          5 date             6 venue            7 city
        //  8 winner          9 result_type     10 toss_winner     11 toss_decision
        // 12 player_of_match 13 stage          14 match_number    15 total_runs
        // 16 batter_runs     17 total_balls    18 wickets         19 fours
        // 20 sixes           21 is_completed_match
        if (row.size() < 22)
            continue;

        InningsScore s;

        s.match_id = row[0];
        s.innings = toInt(row[1]);

        s.batting_team = row[2];
        s.bowling_team = row[3];

        s.season = parseSeasonField(row[4]);

        s.date = row[5];

        s.venue = row[6];
        s.city = row[7];

        s.winner = row[8];
        s.result_type = row[9];

        s.toss_winner = row[10];
        s.toss_decision = row[11];

        s.player_of_match = row[12];
        s.stage = row[13];
        s.match_number = toInt(row[14]);

        s.total_runs = toInt(row[15]);
        s.batter_runs = toInt(row[16]);
        s.balls = toInt(row[17]);
        s.wickets = toInt(row[18]);

        s.fours = toInt(row[19]);
        s.sixes = toInt(row[20]);

        string completed = lowerString(trim(row[21]));
        s.completed_match = !(completed == "false" || completed == "0" || completed == "no");

        // Patch missing/blank season values using the authoritative season derived from
        // the ball-by-ball dataset. Fixes "Season not found" for seasons whose innings-score
        // rows shipped with an empty season column.
        auto seasonLookup = matchSeasonMap.find(s.match_id);
        if (seasonLookup != matchSeasonMap.end())
            s.season = seasonLookup->second;

        target.push_back(s);
    }

    file.close();
    return static_cast<int>(target.size());
}

void loadInningsScores()
{
    int count = loadScoreCSV({"data/IPL_Innings_Scores.csv", "IPL_Innings_Scores.csv"}, inningsScores);

    if (count < 0)
    {
        cout << "\nWARNING: Could not open IPL_Innings_Scores.csv\n";
        cout << "Season/venue/team score analytics will be limited.\n";
        return;
    }

    cout << "Innings score records : " << count << "\n";
}

void loadLowestScores()
{
    int count = loadScoreCSV({"data/IPL_Lowest_Scores.csv", "IPL_Lowest_Scores.csv"}, lowestScoreData);

    if (count < 0)
    {
        cout << "\nWARNING: Could not open IPL_Lowest_Scores.csv\n";
        cout << "Lowest-score records will fall back to IPL_Innings_Scores.csv.\n";
        return;
    }

    cout << "Lowest-score records   : " << count << "\n";
}


// Finds the lowest completed-innings total matching a filter, preferring the curated
// IPL_Lowest_Scores.csv dataset (which already excludes abandoned/rain-shortened innings)
// and only falling back to IPL_Innings_Scores.csv if nothing there matches the filter.
// Returns -1 if nothing at all matches.
template <typename Predicate>
int findLowestScore(Predicate matches)
{
    int lowest = numeric_limits<int>::max();
    bool found = false;

    for (const auto& s : lowestScoreData)
    {
        if (matches(s))
        {
            lowest = min(lowest, s.total_runs);
            found = true;
        }
    }

    if (found)
        return lowest;

    for (const auto& s : inningsScores)
    {
        if (!s.completed_match)
            continue;

        if (matches(s))
        {
            lowest = min(lowest, s.total_runs);
            found = true;
        }
    }

    return found ? lowest : -1;
}


// ============================================================
// SEASON STATISTICS
// ============================================================

void seasonStatistics()
{
    int season;

    cout << "\nEnter IPL season (e.g. 2016): ";

    if (!(cin >> season))
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nInvalid season. Please enter a numeric year.\n";
        return;
    }

    vector<InningsScore> seasonData;

    for (const auto& s : inningsScores)
    {
        if (s.season == season && s.innings >= 1 && s.innings <= 2 && s.completed_match)
            seasonData.push_back(s);
    }

    if (seasonData.empty())
    {
        cout << "\nSeason not found. Available seasons can be seen under "
                "'Dataset Statistics'.\n";
        return;
    }

    set<string> matches;
    set<string> teams;

    long long totalRuns = 0;
    long long totalFours = 0;
    long long totalSixes = 0;

    int highestScore = 0;

    for (const auto& s : seasonData)
    {
        matches.insert(s.match_id);
        teams.insert(s.batting_team);

        totalRuns += s.total_runs;
        totalFours += s.fours;
        totalSixes += s.sixes;

        highestScore = max(highestScore, s.total_runs);
    }

    int lowestScore = findLowestScore([season](const InningsScore& s)
    {
        return s.season == season && s.innings >= 1 && s.innings <= 2;
    });

    if (lowestScore < 0)
    {
        // Should not happen since seasonData is non-empty, but stay safe.
        lowestScore = 0;
        for (const auto& s : seasonData)
            lowestScore = (lowestScore == 0) ? s.total_runs : min(lowestScore, s.total_runs);
    }

    double averageScore = static_cast<double>(totalRuns) / seasonData.size();

    cout << "\n============================================\n";
    cout << "           SEASON STATISTICS\n";
    cout << "============================================\n";
    cout << "Season              : " << season << "\n";
    cout << "Matches             : " << matches.size() << "\n";
    cout << "Teams               : " << teams.size() << "\n";
    cout << "Total Innings Runs  : " << totalRuns << "\n";
    cout << "Total Fours         : " << totalFours << "\n";
    cout << "Total Sixes         : " << totalSixes << "\n";
    cout << "Highest Team Score  : " << highestScore << "\n";
    cout << "Lowest Team Score   : " << lowestScore << "\n";
    cout << fixed << setprecision(2);
    cout << "Average Score       : " << averageScore << "\n";
    cout << "============================================\n";
}


// ============================================================
// PLAYER STATS
// ============================================================

PlayerStats getPlayerStats(const string& player)
{
    PlayerStats stats;

    for (const auto& d : deliveries)
    {
        if (d.batter == player)
        {
            stats.runs += d.runs_batter;

            if (d.valid_ball == 1 && lowerString(d.extra_type) != "wides")
                stats.balls++;

            if (d.runs_batter == 4)
                stats.fours++;

            if (d.runs_batter == 6)
                stats.sixes++;

            stats.matches.insert(d.match_id);
        }

        if (d.bowler == player)
        {
            stats.runsConceded += d.runs_bowler;
            stats.bowlingBalls += d.valid_ball;

            if (d.bowler_wicket == 1)
                stats.wickets++;

            stats.matches.insert(d.match_id);
        }
    }

    return stats;
}

void printPlayerStatsBlock(const string& label, const PlayerStats& stats,
                            double strikeRate, double economy)
{
    cout << "Player              : " << label << "\n";
    cout << "Matches Played      : " << stats.matches.size() << "\n";
    cout << "\n--- BATTING ---\n";
    cout << "Runs                : " << stats.runs << "\n";
    cout << "Balls Faced         : " << stats.balls << "\n";
    cout << "Fours               : " << stats.fours << "\n";
    cout << "Sixes               : " << stats.sixes << "\n";
    cout << fixed << setprecision(2);
    cout << "Strike Rate         : " << strikeRate << "\n";
    cout << "\n--- BOWLING ---\n";
    cout << "Wickets             : " << stats.wickets << "\n";
    cout << "Runs Conceded       : " << stats.runsConceded << "\n";
    cout << "Bowling Balls       : " << stats.bowlingBalls << "\n";
    cout << "Economy Rate        : " << economy << "\n";
}

void playerAnalytics()
{
    string player;

    cout << "\nEnter player name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, player);
    player = trim(player);

    PlayerStats stats = getPlayerStats(player);

    if (stats.matches.empty())
    {
        cout << "\nPlayer not found. Player names must match the dataset exactly "
                "(try 'Match Search' to look one up).\n";
        return;
    }

    double strikeRate = (stats.balls > 0)
        ? static_cast<double>(stats.runs) / stats.balls * 100 : 0;

    double economy = (stats.bowlingBalls > 0)
        ? static_cast<double>(stats.runsConceded) * 6 / stats.bowlingBalls : 0;

    cout << "\n============================================\n";
    cout << "             PLAYER ANALYTICS\n";
    cout << "============================================\n";
    printPlayerStatsBlock(player, stats, strikeRate, economy);
    cout << "============================================\n";
}


// ============================================================
// TEAM STATS
// ============================================================

TeamStats getTeamStats(const string& team)
{
    TeamStats stats;
    map<string, int> matchScores;

    for (const auto& d : deliveries)
    {
        if (d.batting_team != team && d.bowling_team != team)
            continue;

        stats.matchIDs.insert(d.match_id);

        if (d.batting_team == team)
        {
            stats.totalRuns += d.runs_total;
            matchScores[d.match_id] += d.runs_total;
        }
    }

    stats.matches = static_cast<int>(stats.matchIDs.size());

    for (const auto& score : matchScores)
        stats.highestScore = max(stats.highestScore, score.second);

    for (const auto& matchId : stats.matchIDs)
    {
        auto winner = matchWinnerMap.find(matchId);

        if (winner == matchWinnerMap.end())
            continue;

        if (winner->second == team)
            stats.wins++;
        else
            stats.losses++;
    }

    return stats;
}

void teamAnalytics()
{
    string team;

    cout << "\nEnter team name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, team);
    team = trim(team);

    TeamStats stats = getTeamStats(team);

    if (stats.matches == 0)
    {
        cout << "\nTeam not found. Team names must match the dataset exactly "
                "(try 'Match Search' to look one up).\n";
        return;
    }

    double winPercentage = static_cast<double>(stats.wins) / stats.matches * 100;

    cout << "\n============================================\n";
    cout << "              TEAM ANALYTICS\n";
    cout << "============================================\n";
    cout << "Team                : " << team << "\n";
    cout << "Matches Played      : " << stats.matches << "\n";
    cout << "Wins                : " << stats.wins << "\n";
    cout << "Losses              : " << stats.losses << "\n";
    cout << fixed << setprecision(2);
    cout << "Win Percentage      : " << winPercentage << "%\n";
    cout << "Total Runs          : " << stats.totalRuns << "\n";
    cout << "Highest Team Score  : " << stats.highestScore << "\n";
    cout << "============================================\n";
}


// ============================================================
// MATCH SEARCH
// ============================================================

void matchSearch()
{
    string search;

    cout << "\nEnter team/player/venue/city to search: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, search);
    search = trim(search);

    if (search.empty())
    {
        cout << "\nPlease enter something to search for.\n";
        return;
    }

    set<string> foundMatches;

    for (const auto& d : deliveries)
    {
        if (containsIgnoreCase(d.batting_team, search) ||
            containsIgnoreCase(d.bowling_team, search) ||
            containsIgnoreCase(d.batter, search) ||
            containsIgnoreCase(d.bowler, search) ||
            containsIgnoreCase(d.venue, search) ||
            containsIgnoreCase(d.city, search))
        {
            foundMatches.insert(d.match_id);
        }
    }

    if (foundMatches.empty())
    {
        cout << "\nNo matching records found.\n";
        return;
    }

    cout << "\n============================================\n";
    cout << "              MATCH SEARCH\n";
    cout << "============================================\n";
    cout << "Search              : " << search << "\n";
    cout << "Matches Found       : " << foundMatches.size() << "\n";
    cout << "\nMatch IDs (first 30):\n";

    int count = 0;
    for (const auto& id : foundMatches)
    {
        cout << "- " << id << "\n";
        count++;
        if (count >= 30)
            break;
    }

    if (foundMatches.size() > 30)
        cout << "... and " << (foundMatches.size() - 30) << " more.\n";

    cout << "============================================\n";
}


// ============================================================
// RANKING HELPERS (Top batsmen / bowlers / sixes / fours)
// ============================================================

void printRanking(const string& title, const map<string, int>& tally, const string& unit,
                   int topN = 10)
{
    vector<pair<string, int>> ranking(tally.begin(), tally.end());

    sort(ranking.begin(), ranking.end(), [](const auto& a, const auto& b)
    {
        if (a.second != b.second)
            return a.second > b.second;
        return a.first < b.first;
    });

    cout << "\n============================================\n";
    cout << title << "\n";
    cout << "============================================\n";

    int position = 1;
    for (const auto& entry : ranking)
    {
        cout << position << ". " << entry.first << " - " << entry.second << " " << unit << "\n";
        position++;
        if (position > topN)
            break;
    }

    if (ranking.empty())
        cout << "(no data)\n";

    cout << "============================================\n";
}

void topBatsmen()
{
    map<string, int> playerRuns;

    for (const auto& d : deliveries)
        if (!d.batter.empty())
            playerRuns[d.batter] += d.runs_batter;

    printRanking("             TOP 10 BATSMEN", playerRuns, "runs");
}

void topBowlers()
{
    map<string, int> playerWickets;

    for (const auto& d : deliveries)
        if (!d.bowler.empty() && d.bowler_wicket == 1)
            playerWickets[d.bowler]++;

    printRanking("             TOP 10 BOWLERS", playerWickets, "wickets");
}

void mostSixes()
{
    map<string, int> playerSixes;

    for (const auto& d : deliveries)
        if (!d.batter.empty() && d.runs_batter == 6)
            playerSixes[d.batter]++;

    printRanking("               MOST SIXES", playerSixes, "sixes");
}

void mostFours()
{
    map<string, int> playerFours;

    for (const auto& d : deliveries)
        if (!d.batter.empty() && d.runs_batter == 4)
            playerFours[d.batter]++;

    printRanking("               MOST FOURS", playerFours, "fours");
}


// ============================================================
// PLAYER COMPARISON
// ============================================================

void playerComparison()
{
    string player1, player2;

    cout << "\nEnter first player: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, player1);
    player1 = trim(player1);

    cout << "Enter second player: ";
    getline(cin, player2);
    player2 = trim(player2);

    PlayerStats p1 = getPlayerStats(player1);
    PlayerStats p2 = getPlayerStats(player2);

    if (p1.matches.empty())
    {
        cout << "\nFirst player not found.\n";
        return;
    }

    if (p2.matches.empty())
    {
        cout << "\nSecond player not found.\n";
        return;
    }

    double sr1 = (p1.balls > 0) ? static_cast<double>(p1.runs) / p1.balls * 100 : 0;
    double sr2 = (p2.balls > 0) ? static_cast<double>(p2.runs) / p2.balls * 100 : 0;

    double eco1 = (p1.bowlingBalls > 0) ? static_cast<double>(p1.runsConceded) * 6 / p1.bowlingBalls : 0;
    double eco2 = (p2.bowlingBalls > 0) ? static_cast<double>(p2.runsConceded) * 6 / p2.bowlingBalls : 0;

    cout << "\n============================================================\n";
    cout << "                 PLAYER COMPARISON\n";
    cout << "============================================================\n";

    cout << left << setw(25) << "Statistic" << setw(20) << player1 << setw(20) << player2 << "\n";
    cout << "------------------------------------------------------------\n";

    cout << setw(25) << "Matches" << setw(20) << p1.matches.size() << setw(20) << p2.matches.size() << "\n";
    cout << setw(25) << "Runs" << setw(20) << p1.runs << setw(20) << p2.runs << "\n";
    cout << setw(25) << "Balls Faced" << setw(20) << p1.balls << setw(20) << p2.balls << "\n";
    cout << setw(25) << "Fours" << setw(20) << p1.fours << setw(20) << p2.fours << "\n";
    cout << setw(25) << "Sixes" << setw(20) << p1.sixes << setw(20) << p2.sixes << "\n";

    cout << fixed << setprecision(2);
    cout << setw(25) << "Strike Rate" << setw(20) << sr1 << setw(20) << sr2 << "\n";
    cout << setw(25) << "Wickets" << setw(20) << p1.wickets << setw(20) << p2.wickets << "\n";
    cout << setw(25) << "Runs Conceded" << setw(20) << p1.runsConceded << setw(20) << p2.runsConceded << "\n";
    cout << setw(25) << "Economy" << setw(20) << eco1 << setw(20) << eco2 << "\n";

    cout << "============================================================\n";
}


// ============================================================
// TEAM VS TEAM
// ============================================================

void teamVsTeam()
{
    string team1, team2;

    cout << "\nEnter first team: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, team1);
    team1 = trim(team1);

    cout << "Enter second team: ";
    getline(cin, team2);
    team2 = trim(team2);

    TeamStats t1 = getTeamStats(team1);
    TeamStats t2 = getTeamStats(team2);

    if (t1.matches == 0)
    {
        cout << "\nFirst team not found.\n";
        return;
    }

    if (t2.matches == 0)
    {
        cout << "\nSecond team not found.\n";
        return;
    }

    set<string> commonMatches;
    for (const auto& id : t1.matchIDs)
        if (t2.matchIDs.count(id))
            commonMatches.insert(id);

    int team1Wins = 0;
    int team2Wins = 0;

    for (const auto& id : commonMatches)
    {
        auto winner = matchWinnerMap.find(id);
        if (winner == matchWinnerMap.end())
            continue;

        if (winner->second == team1)
            team1Wins++;
        else if (winner->second == team2)
            team2Wins++;
    }

    cout << "\n============================================================\n";
    cout << "                    TEAM VS TEAM\n";
    cout << "============================================================\n";
    cout << "Team 1              : " << team1 << "\n";
    cout << "Team 2              : " << team2 << "\n";
    cout << "Head-to-Head Matches: " << commonMatches.size() << "\n";
    cout << team1 << " Wins           : " << team1Wins << "\n";
    cout << team2 << " Wins           : " << team2Wins << "\n";

    cout << "\n--- OVERALL TEAM STATS ---\n";
    cout << team1 << " Matches         : " << t1.matches << "\n";
    cout << team1 << " Total Runs      : " << t1.totalRuns << "\n";
    cout << team1 << " Highest Score   : " << t1.highestScore << "\n";
    cout << "\n";
    cout << team2 << " Matches         : " << t2.matches << "\n";
    cout << team2 << " Total Runs      : " << t2.totalRuns << "\n";
    cout << team2 << " Highest Score   : " << t2.highestScore << "\n";

    cout << "============================================================\n";
}


// ============================================================
// VENUE ANALYTICS
// ============================================================

void venueAnalytics()
{
    string venue;

    cout << "\nEnter venue name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, venue);
    venue = trim(venue);

    if (venue.empty())
    {
        cout << "\nPlease enter a venue name.\n";
        return;
    }

    set<string> matches;
    set<string> teams;

    int highestScore = 0;
    long long totalRuns = 0;
    int inningsCount = 0;

    for (const auto& s : inningsScores)
    {
        if (!containsIgnoreCase(s.venue, venue))
            continue;

        if (!s.completed_match)
            continue;

        matches.insert(s.match_id);
        teams.insert(s.batting_team);

        highestScore = max(highestScore, s.total_runs);
        totalRuns += s.total_runs;
        inningsCount++;
    }

    if (matches.empty())
    {
        cout << "\nVenue not found.\n";
        return;
    }

    int lowestScore = findLowestScore([&venue](const InningsScore& s)
    {
        return containsIgnoreCase(s.venue, venue);
    });

    if (lowestScore < 0)
        lowestScore = 0;

    double averageScore = (inningsCount > 0) ? static_cast<double>(totalRuns) / inningsCount : 0;

    cout << "\n============================================\n";
    cout << "             VENUE ANALYTICS\n";
    cout << "============================================\n";
    cout << "Venue               : " << venue << "\n";
    cout << "Matches             : " << matches.size() << "\n";
    cout << "Teams Played        : " << teams.size() << "\n";
    cout << "Highest Score       : " << highestScore << "\n";
    cout << "Lowest Score        : " << lowestScore << "\n";
    cout << fixed << setprecision(2);
    cout << "Average Score       : " << averageScore << "\n";
    cout << "============================================\n";
}


// ============================================================
// TOSS ANALYSIS
// ============================================================

void tossAnalysis()
{
    map<string, int> tossWins;
    map<string, int> tossDecisions;
    map<string, int> tossAndMatchWin;

    set<string> matches;
    unordered_map<string, string> matchTossWinner;

    for (const auto& d : deliveries)
    {
        if (d.match_id.empty())
            continue;

        matches.insert(d.match_id);

        if (!d.toss_winner.empty())
            matchTossWinner[d.match_id] = d.toss_winner;

        if (!d.toss_decision.empty())
            tossDecisions[d.toss_decision]++;
    }

    for (const auto& item : matchTossWinner)
    {
        tossWins[item.second]++;

        auto winner = matchWinnerMap.find(item.first);

        if (winner != matchWinnerMap.end() && winner->second == item.second)
            tossAndMatchWin[item.second]++;
    }

    cout << "\n============================================\n";
    cout << "               TOSS ANALYSIS\n";
    cout << "============================================\n";
    cout << "Total Matches      : " << matches.size() << "\n";

    cout << "\n--- TOSS DECISIONS ---\n";
    for (const auto& decision : tossDecisions)
        cout << decision.first << " : " << decision.second << "\n";

    vector<pair<string, int>> ranking(tossWins.begin(), tossWins.end());
    sort(ranking.begin(), ranking.end(), [](const auto& a, const auto& b)
    {
        return a.second > b.second;
    });

    cout << "\n--- MOST TOSS WINS ---\n";
    int position = 1;
    for (const auto& team : ranking)
    {
        cout << position << ". " << team.first << " - " << team.second << "\n";
        position++;
        if (position > 10)
            break;
    }

    cout << "\n--- TOSS WINNER ALSO WON MATCH ---\n";
    for (const auto& item : tossAndMatchWin)
        cout << item.first << " : " << item.second << " matches\n";

    cout << "============================================\n";
}


// ============================================================
// MATCH SUMMARY
// ============================================================

void matchSummary()
{
    string matchID;

    cout << "\nEnter Match ID: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, matchID);
    matchID = trim(matchID);

    set<string> teams;

    string date, venue, city, winner, playerOfMatch, tossWinner, tossDecision, resultType;
    map<string, int> teamScores;

    bool found = false;

    for (const auto& s : inningsScores)
    {
        if (s.match_id != matchID)
            continue;

        found = true;

        teams.insert(s.batting_team);
        teamScores[s.batting_team] = s.total_runs;

        if (date.empty()) date = s.date;
        if (venue.empty()) venue = s.venue;
        if (city.empty()) city = s.city;

        if (!s.winner.empty()) winner = s.winner;
        if (!s.player_of_match.empty()) playerOfMatch = s.player_of_match;
        if (!s.toss_winner.empty()) tossWinner = s.toss_winner;
        if (!s.toss_decision.empty()) tossDecision = s.toss_decision;
        if (!s.result_type.empty()) resultType = s.result_type;
    }

    if (!found)
    {
        cout << "\nMatch not found.\n";
        return;
    }

    cout << "\n============================================\n";
    cout << "               MATCH SUMMARY\n";
    cout << "============================================\n";
    cout << "Match ID            : " << matchID << "\n";
    cout << "Date                : " << date << "\n";
    cout << "Venue               : " << venue << "\n";
    cout << "City                : " << city << "\n";

    cout << "\n--- TEAMS ---\n";
    for (const auto& team : teams)
        cout << "- " << team << "\n";

    cout << "\n--- SCORES ---\n";
    for (const auto& score : teamScores)
        cout << score.first << " : " << score.second << "\n";

    cout << "\nWinner              : " << winner << "\n";
    cout << "Player of Match     : " << playerOfMatch << "\n";
    cout << "Toss Winner         : " << tossWinner << "\n";
    cout << "Toss Decision       : " << tossDecision << "\n";
    cout << "Result Type         : " << resultType << "\n";
    cout << "============================================\n";
}


// ============================================================
// SEASON COMPARISON
// ============================================================

void seasonComparison()
{
    int season1, season2;

    cout << "\nEnter first season: ";
    if (!(cin >> season1))
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nInvalid season.\n";
        return;
    }

    cout << "Enter second season: ";
    if (!(cin >> season2))
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\nInvalid season.\n";
        return;
    }

    vector<InningsScore> s1, s2;

    for (const auto& s : inningsScores)
    {
        if (!s.completed_match)
            continue;

        if (s.season == season1) s1.push_back(s);
        if (s.season == season2) s2.push_back(s);
    }

    if (s1.empty())
    {
        cout << "\nFirst season not found.\n";
        return;
    }

    if (s2.empty())
    {
        cout << "\nSecond season not found.\n";
        return;
    }

    auto summarize = [](const vector<InningsScore>& data,
                         set<string>& matchesOut, set<string>& teamsOut,
                         long long& runsOut, int& foursOut, int& sixesOut, int& highOut)
    {
        for (const auto& s : data)
        {
            matchesOut.insert(s.match_id);
            teamsOut.insert(s.batting_team);
            runsOut += s.total_runs;
            foursOut += s.fours;
            sixesOut += s.sixes;
            highOut = max(highOut, s.total_runs);
        }
    };

    set<string> matches1, matches2, teams1, teams2;
    long long runs1 = 0, runs2 = 0;
    int fours1 = 0, fours2 = 0, sixes1 = 0, sixes2 = 0, high1 = 0, high2 = 0;

    summarize(s1, matches1, teams1, runs1, fours1, sixes1, high1);
    summarize(s2, matches2, teams2, runs2, fours2, sixes2, high2);

    int low1 = findLowestScore([season1](const InningsScore& s) { return s.season == season1; });
    int low2 = findLowestScore([season2](const InningsScore& s) { return s.season == season2; });
    if (low1 < 0) low1 = 0;
    if (low2 < 0) low2 = 0;

    double avg1 = static_cast<double>(runs1) / s1.size();
    double avg2 = static_cast<double>(runs2) / s2.size();

    cout << "\n============================================================\n";
    cout << "                  SEASON COMPARISON\n";
    cout << "============================================================\n";

    cout << left << setw(25) << "Statistic" << setw(20) << season1 << setw(20) << season2 << "\n";
    cout << "------------------------------------------------------------\n";

    cout << setw(25) << "Matches" << setw(20) << matches1.size() << setw(20) << matches2.size() << "\n";
    cout << setw(25) << "Teams" << setw(20) << teams1.size() << setw(20) << teams2.size() << "\n";
    cout << setw(25) << "Total Runs" << setw(20) << runs1 << setw(20) << runs2 << "\n";
    cout << setw(25) << "Fours" << setw(20) << fours1 << setw(20) << fours2 << "\n";
    cout << setw(25) << "Sixes" << setw(20) << sixes1 << setw(20) << sixes2 << "\n";
    cout << setw(25) << "Highest Score" << setw(20) << high1 << setw(20) << high2 << "\n";
    cout << setw(25) << "Lowest Score" << setw(20) << low1 << setw(20) << low2 << "\n";

    cout << fixed << setprecision(2);
    cout << setw(25) << "Average Score" << setw(20) << avg1 << setw(20) << avg2 << "\n";

    cout << "============================================================\n";
}


// ============================================================
// DATASET STATISTICS
// ============================================================

void datasetStatistics()
{
    set<string> matches, players, teams;
    set<int> seasons;
    set<string> venues;

    long long totalRuns = 0, totalFours = 0, totalSixes = 0, totalWickets = 0;

    for (const auto& d : deliveries)
    {
        if (!d.match_id.empty()) matches.insert(d.match_id);
        if (!d.batter.empty()) players.insert(d.batter);
        if (!d.batting_team.empty()) teams.insert(d.batting_team);
        if (d.season != 0) seasons.insert(d.season);
        if (!d.venue.empty()) venues.insert(d.venue);

        totalRuns += d.runs_total;

        if (d.runs_batter == 4) totalFours++;
        if (d.runs_batter == 6) totalSixes++;
        if (d.bowler_wicket == 1) totalWickets++;
    }

    int overallLowest = findLowestScore([](const InningsScore&) { return true; });
    int overallHighest = 0;
    for (const auto& s : inningsScores)
        if (s.completed_match)
            overallHighest = max(overallHighest, s.total_runs);

    cout << "\n============================================\n";
    cout << "             DATASET STATISTICS\n";
    cout << "============================================\n";
    cout << "Delivery Records    : " << deliveries.size() << "\n";
    cout << "Innings Records     : " << inningsScores.size() << "\n";
    cout << "Matches             : " << matches.size() << "\n";
    cout << "Players             : " << players.size() << "\n";
    cout << "Teams               : " << teams.size() << "\n";
    cout << "Seasons             : " << seasons.size() << "\n";
    cout << "Venues              : " << venues.size() << "\n";
    cout << "Total Runs          : " << totalRuns << "\n";
    cout << "Total Fours         : " << totalFours << "\n";
    cout << "Total Sixes         : " << totalSixes << "\n";
    cout << "Total Wickets       : " << totalWickets << "\n";

    if (overallHighest > 0)
        cout << "Highest Team Score  : " << overallHighest << " (all-time)\n";

    if (overallLowest >= 0)
        cout << "Lowest Team Score   : " << overallLowest << " (all-time, completed innings)\n";

    if (!seasons.empty())
    {
        cout << "First Season        : " << *seasons.begin() << "\n";
        cout << "Latest Season       : " << *seasons.rbegin() << "\n";
    }

    cout << "============================================\n";
}


// ============================================================
// VISUAL REPORTS (HTML / Chart.js — real charts, not terminal art)
//
// Each report writes a small self-contained .html file under reports/ that
// renders an interactive chart via Chart.js (loaded from a CDN, so the
// machine viewing it needs internet access once, when the file is opened).
// ============================================================

string jsonEscape(const string& value)
{
    string out;
    out.reserve(value.size());

    for (char c : value)
    {
        if (c == '"' || c == '\\')
            out += '\\';

        out += c;
    }

    return out;
}

// Opens a file in the user's default browser, best-effort, cross-platform.
void openInBrowser(const string& filepath)
{
    string cmd;

#if defined(_WIN32)
    cmd = "start \"\" \"" + filepath + "\"";
#elif defined(__APPLE__)
    cmd = "open \"" + filepath + "\"";
#else
    cmd = "xdg-open \"" + filepath + "\" >/dev/null 2>&1 &";
#endif

    int result = system(cmd.c_str());
    (void)result; // best-effort; nothing we can usefully do if this fails
}

void offerToOpen(const string& filepath)
{
    cout << "Saved chart to: " << filepath << "\n";
    cout << "Open it now in your browser? (y/n): ";

    string answer;
    cin >> answer;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (!answer.empty() && tolower(answer[0]) == 'y')
        openInBrowser(filepath);
}

string chartHtmlHead(const string& pageTitle)
{
    ostringstream html;

    html << "<!DOCTYPE html>\n<html><head><meta charset='utf-8'>\n";
    html << "<title>" << jsonEscape(pageTitle) << "</title>\n";
    html << "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>\n";
    html << "<style>\n"
            "  body { font-family: 'Segoe UI', Arial, sans-serif; background:#0f172a; "
            "color:#e2e8f0; margin:0; padding:32px; }\n"
            "  h1 { text-align:center; font-weight:600; margin-bottom:28px; }\n"
            "  .chart-wrap { max-width:920px; margin:0 auto; background:#1e293b; "
            "padding:28px; border-radius:14px; box-shadow:0 4px 24px rgba(0,0,0,0.35); }\n"
            "</style>\n</head><body>\n";
    html << "<h1>" << jsonEscape(pageTitle) << "</h1>\n";
    html << "<div class='chart-wrap'><canvas id='chart'></canvas></div>\n";

    return html.str();
}

const char* PALETTE =
    "['#38bdf8','#818cf8','#f472b6','#fbbf24','#34d399','#f87171','#a78bfa','#22d3ee','#facc15','#4ade80']";

// Single-series chart: bar, line, pie, or doughnut.
bool writeSingleSeriesChart(const string& filename, const string& pageTitle,
                             const string& chartType, const vector<string>& labels,
                             const vector<double>& values, const string& seriesLabel)
{
    ensureDirectoryExists("reports");
    string filepath = "reports/" + filename;

    ofstream out(filepath);
    if (!out.is_open())
    {
        cout << "\nERROR: could not write " << filepath << "\n";
        return false;
    }

    bool isCircular = (chartType == "pie" || chartType == "doughnut");

    out << chartHtmlHead(pageTitle);
    out << "<script>\nconst labels = [";
    for (size_t i = 0; i < labels.size(); i++)
    {
        out << "\"" << jsonEscape(labels[i]) << "\"";
        if (i + 1 < labels.size()) out << ",";
    }
    out << "];\nconst data = [";
    for (size_t i = 0; i < values.size(); i++)
    {
        out << values[i];
        if (i + 1 < values.size()) out << ",";
    }
    out << "];\n";

    out << "new Chart(document.getElementById('chart'), {\n"
        << "  type: '" << chartType << "',\n"
        << "  data: { labels: labels, datasets: [{ label: '" << jsonEscape(seriesLabel)
        << "', data: data, backgroundColor: " << PALETTE
        << ", borderColor: '#38bdf8', borderWidth: 1 }] },\n"
        << "  options: { responsive: true, plugins: { legend: { display: "
        << (isCircular ? "true" : "false") << " } },\n"
        << "    scales: " << (isCircular ? "{}" : "{ y: { beginAtZero: true } }") << " }\n"
        << "});\n</script>\n</body></html>\n";

    out.close();
    return true;
}

// Two-series grouped bar chart (e.g. team A vs team B across several metrics).
bool writeGroupedBarChart(const string& filename, const string& pageTitle,
                           const vector<string>& labels,
                           const string& series1Label, const vector<double>& series1,
                           const string& series2Label, const vector<double>& series2)
{
    ensureDirectoryExists("reports");
    string filepath = "reports/" + filename;

    ofstream out(filepath);
    if (!out.is_open())
    {
        cout << "\nERROR: could not write " << filepath << "\n";
        return false;
    }

    out << chartHtmlHead(pageTitle);
    out << "<script>\nconst labels = [";
    for (size_t i = 0; i < labels.size(); i++)
    {
        out << "\"" << jsonEscape(labels[i]) << "\"";
        if (i + 1 < labels.size()) out << ",";
    }
    out << "];\n";

    auto writeArray = [&out](const vector<double>& values)
    {
        out << "[";
        for (size_t i = 0; i < values.size(); i++)
        {
            out << values[i];
            if (i + 1 < values.size()) out << ",";
        }
        out << "]";
    };

    out << "new Chart(document.getElementById('chart'), {\n"
        << "  type: 'bar',\n"
        << "  data: { labels: labels, datasets: [\n"
        << "    { label: '" << jsonEscape(series1Label) << "', data: ";
    writeArray(series1);
    out << ", backgroundColor: '#38bdf8' },\n"
        << "    { label: '" << jsonEscape(series2Label) << "', data: ";
    writeArray(series2);
    out << ", backgroundColor: '#f472b6' }\n"
        << "  ] },\n"
        << "  options: { responsive: true, scales: { y: { beginAtZero: true } } }\n"
        << "});\n</script>\n</body></html>\n";

    out.close();
    return true;
}

void topNBarChartReport(const map<string, int>& tally, const string& pageTitle,
                         const string& seriesLabel, const string& filename, int topN = 10)
{
    vector<pair<string, int>> ranking(tally.begin(), tally.end());

    sort(ranking.begin(), ranking.end(), [](const auto& a, const auto& b)
    {
        if (a.second != b.second)
            return a.second > b.second;
        return a.first < b.first;
    });

    if (static_cast<int>(ranking.size()) > topN)
        ranking.resize(topN);

    if (ranking.empty())
    {
        cout << "\nNo data available for this chart.\n";
        return;
    }

    vector<string> labels;
    vector<double> values;
    for (const auto& entry : ranking)
    {
        labels.push_back(entry.first);
        values.push_back(entry.second);
    }

    if (writeSingleSeriesChart(filename, pageTitle, "bar", labels, values, seriesLabel))
        offerToOpen("reports/" + filename);
}

void seasonTrendChartReport(bool average)
{
    map<int, long long> runsBySeason;
    map<int, int> inningsBySeason;

    for (const auto& s : inningsScores)
    {
        if (!s.completed_match || s.season == 0)
            continue;

        runsBySeason[s.season] += s.total_runs;
        inningsBySeason[s.season]++;
    }

    if (runsBySeason.empty())
    {
        cout << "\nNo season data available for this chart.\n";
        return;
    }

    vector<string> labels;
    vector<double> values;

    for (const auto& entry : runsBySeason)
    {
        labels.push_back(to_string(entry.first));

        if (average)
            values.push_back(static_cast<double>(entry.second) / inningsBySeason[entry.first]);
        else
            values.push_back(static_cast<double>(entry.second));
    }

    string title = average ? "Average Score by Season" : "Total Runs by Season";
    string filename = average ? "season_average_score.html" : "season_total_runs.html";
    string seriesLabel = average ? "Average Score" : "Total Runs";

    if (writeSingleSeriesChart(filename, title, "line", labels, values, seriesLabel))
        offerToOpen("reports/" + filename);
}

void tossDecisionChartReport()
{
    unordered_map<string, string> matchTossDecision;

    for (const auto& d : deliveries)
    {
        if (!d.toss_decision.empty() &&
            matchTossDecision.find(d.match_id) == matchTossDecision.end())
        {
            matchTossDecision[d.match_id] = d.toss_decision;
        }
    }

    map<string, int> counts;
    for (const auto& entry : matchTossDecision)
        counts[entry.second]++;

    if (counts.empty())
    {
        cout << "\nNo toss data available for this chart.\n";
        return;
    }

    vector<string> labels;
    vector<double> values;
    for (const auto& entry : counts)
    {
        labels.push_back(entry.first);
        values.push_back(entry.second);
    }

    if (writeSingleSeriesChart("toss_decision_split.html", "Toss Decision Split", "pie",
                                labels, values, "Matches"))
        offerToOpen("reports/toss_decision_split.html");
}

void teamComparisonChartReport()
{
    string team1, team2;

    cout << "\nEnter first team: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, team1);
    team1 = trim(team1);

    cout << "Enter second team: ";
    getline(cin, team2);
    team2 = trim(team2);

    TeamStats t1 = getTeamStats(team1);
    TeamStats t2 = getTeamStats(team2);

    if (t1.matches == 0)
    {
        cout << "\nFirst team not found.\n";
        return;
    }

    if (t2.matches == 0)
    {
        cout << "\nSecond team not found.\n";
        return;
    }

    vector<string> labels = {"Matches", "Wins", "Losses"};
    vector<double> series1 = {
        static_cast<double>(t1.matches), static_cast<double>(t1.wins), static_cast<double>(t1.losses)};
    vector<double> series2 = {
        static_cast<double>(t2.matches), static_cast<double>(t2.wins), static_cast<double>(t2.losses)};

    if (writeGroupedBarChart("team_comparison.html", team1 + " vs " + team2,
                              labels, team1, series1, team2, series2))
        offerToOpen("reports/team_comparison.html");
}

void visualReportsMenu()
{
    while (true)
    {
        cout << "\n\n";
        cout << "============================================\n";
        cout << "             VISUAL REPORTS (HTML)\n";
        cout << "============================================\n";
        cout << "1. Top 10 Batsmen (bar)\n";
        cout << "2. Top 10 Bowlers (bar)\n";
        cout << "3. Most Sixes (bar)\n";
        cout << "4. Most Fours (bar)\n";
        cout << "5. Total Runs by Season (line)\n";
        cout << "6. Average Score by Season (line)\n";
        cout << "7. Toss Decision Split (pie)\n";
        cout << "8. Team vs Team Comparison (grouped bar)\n";
        cout << "9. Back to Main Menu\n";
        cout << "============================================\n";
        cout << "Enter your choice: ";

        int choice;
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "\nInvalid input. Please enter a number from 1-9.\n";
            continue;
        }

        if (choice == 1)
        {
            map<string, int> playerRuns;
            for (const auto& d : deliveries)
                if (!d.batter.empty())
                    playerRuns[d.batter] += d.runs_batter;

            topNBarChartReport(playerRuns, "Top 10 Batsmen", "Runs", "top_batsmen.html");
        }
        else if (choice == 2)
        {
            map<string, int> playerWickets;
            for (const auto& d : deliveries)
                if (!d.bowler.empty() && d.bowler_wicket == 1)
                    playerWickets[d.bowler]++;

            topNBarChartReport(playerWickets, "Top 10 Bowlers", "Wickets", "top_bowlers.html");
        }
        else if (choice == 3)
        {
            map<string, int> playerSixes;
            for (const auto& d : deliveries)
                if (!d.batter.empty() && d.runs_batter == 6)
                    playerSixes[d.batter]++;

            topNBarChartReport(playerSixes, "Most Sixes", "Sixes", "most_sixes.html");
        }
        else if (choice == 4)
        {
            map<string, int> playerFours;
            for (const auto& d : deliveries)
                if (!d.batter.empty() && d.runs_batter == 4)
                    playerFours[d.batter]++;

            topNBarChartReport(playerFours, "Most Fours", "Fours", "most_fours.html");
        }
        else if (choice == 5)
        {
            seasonTrendChartReport(false);
        }
        else if (choice == 6)
        {
            seasonTrendChartReport(true);
        }
        else if (choice == 7)
        {
            tossDecisionChartReport();
        }
        else if (choice == 8)
        {
            teamComparisonChartReport();
        }
        else if (choice == 9)
        {
            return;
        }
        else
        {
            cout << "\nInvalid choice. Try again.\n";
        }
    }
}


// ============================================================
// MENU
// ============================================================

void menu()
{
    int choice;

    while (true)
    {
        cout << "\n\n";
        cout << "============================================\n";
        cout << "             IPL STATS ANALYTICS\n";
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
        cout << "16. Visual Reports (HTML Charts)\n";
        cout << "17. Exit\n";
        cout << "============================================\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "\nInvalid input. Please enter a number from 1-17.\n";
            continue;
        }

        switch (choice)
        {
            case 1:  seasonStatistics(); break;
            case 2:  playerAnalytics(); break;
            case 3:  teamAnalytics(); break;
            case 4:  matchSearch(); break;
            case 5:  topBatsmen(); break;
            case 6:  topBowlers(); break;
            case 7:  mostSixes(); break;
            case 8:  mostFours(); break;
            case 9:  playerComparison(); break;
            case 10: teamVsTeam(); break;
            case 11: venueAnalytics(); break;
            case 12: tossAnalysis(); break;
            case 13: matchSummary(); break;
            case 14: seasonComparison(); break;
            case 15: datasetStatistics(); break;
            case 16: visualReportsMenu(); break;
            case 17:
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

int main()
{
    cout << "\n";
    cout << "============================================\n";
    cout << "             IPL STATS ANALYTICS\n";
    cout << "============================================\n";
    cout << "\nLoading IPL datasets...\n";

    if (!loadIPLData())
    {
        cout << "\nERROR: No main IPL data loaded.\n";
        cout << "Check:\n";
        cout << "data/IPL.csv (or IPL.csv next to the executable)\n";
        return 1;
    }

    buildMatchLookups();

    loadInningsScores();
    loadLowestScores();

    if (inningsScores.empty())
    {
        cout << "\nWARNING: IPL_Innings_Scores.csv was not loaded.\n";
        cout << "Check:\n";
        cout << "data/IPL_Innings_Scores.csv\n";
    }

    if (lowestScoreData.empty())
    {
        cout << "\nNOTE: IPL_Lowest_Scores.csv was not loaded; lowest-score "
                "records will use IPL_Innings_Scores.csv instead.\n";
    }

    cout << "\nStarting analytics system...\n";

    menu();

    return 0;
}
