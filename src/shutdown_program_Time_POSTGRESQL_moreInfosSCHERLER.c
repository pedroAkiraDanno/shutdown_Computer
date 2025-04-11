



#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // For sleep function
#include <libpq-fe.h> // PostgreSQL C library

#ifdef _WIN32
#include <windows.h>
#include <lmcons.h> // For UNLEN and GetUserName
#else
#include <pwd.h> // For getpwuid
#include <limits.h> // For HOST_NAME_MAX
#endif

// Function to insert shutdown event into the database
void log_shutdown_event(PGconn *conn, const char *username, const char *computername) {
    // Get the current timestamp
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Prepare the SQL query
    char query[256];
    snprintf(query, sizeof(query), 
             "INSERT INTO shutdown_events (event_time, username, computername) VALUES ('%s', '%s', '%s')", 
             timestamp, username, computername);

    // Execute the query
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Error inserting record: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    PQclear(res);
    printf("Shutdown event logged successfully in PostgreSQL.\n");
}

// Function to shut down the computer
void shutdown_computer() {
    #ifdef _WIN32
        // Windows shutdown command
        system("shutdown /s /t 0");
    #else
        // Linux shutdown command
        system("shutdown now");
    #endif
}

// Function to get shutdown time from user in HH:MM format
void get_shutdown_time(int *target_hour, int *target_minute) {
    char input[6];
    int valid = 0;
    
    while (!valid) {
        printf("Enter shutdown time in HH:MM format (e.g., 03:00, 20:30): ");
        scanf("%5s", input);
        
        if (sscanf(input, "%d:%d", target_hour, target_minute) == 2) {
            if (*target_hour >= 0 && *target_hour < 24 && 
                *target_minute >= 0 && *target_minute < 60) {
                valid = 1;
            } else {
                printf("Invalid time! Hours must be 0-23 and minutes must be 0-59.\n");
            }
        } else {
            printf("Invalid format! Please use HH:MM format.\n");
        }
    }
}

// Function to calculate seconds until target time
int calculate_seconds_until_target(int target_hour, int target_minute) {
    time_t now;
    struct tm *tm_now, tm_target;
    time(&now);
    tm_now = localtime(&now);
    
    // Set target time to user input
    tm_target = *tm_now;
    tm_target.tm_hour = target_hour;
    tm_target.tm_min = target_minute;
    tm_target.tm_sec = 0;
    
    // If it's already past target time, schedule for next day
    if (mktime(tm_now) >= mktime(&tm_target)) {
        tm_target.tm_mday++;
        mktime(&tm_target); // Normalize the time structure
    }
    
    return (int)difftime(mktime(&tm_target), mktime(tm_now));
}

// Function to display messages with a delay
void display_messages_with_delay(int seconds) {
    while (seconds > 0) {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        int secs = seconds % 60;
        
        printf("Shutting down in %02d:%02d:%02d...\n", hours, minutes, secs);
        
        // Update every minute if more than 1 hour remains, otherwise every 10 seconds
        if (seconds > 3600) {
            sleep(60); // Delay for 1 minute
            seconds -= 60;
        } else {
            sleep(10); // Delay for 10 seconds
            seconds -= 10;
        }
    }
}

// Function to detect the operating system
void detect_os() {
    #ifdef _WIN32
        printf("Operating System: Windows\n");
    #else
        printf("Operating System: Linux\n");
    #endif
}

// Function to get the current username
char* get_username() {
    static char username[256];
    #ifdef _WIN32
        DWORD username_len = UNLEN + 1;
        if (GetUserName(username, &username_len)) {
            return username;
        }
    #else
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            return pw->pw_name;
        }
    #endif
    return "unknown";
}

// Function to get the computer name
char* get_computername() {
    static char hostname[256];
    #ifdef _WIN32
        DWORD size = sizeof(hostname);
        if (GetComputerName(hostname, &size)) {
            return hostname;
        }
    #else
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            return hostname;
        }
    #endif
    return "unknown";
}

int main() {
    // Detect and display the operating system
    detect_os();

    // Database connection parameters
    const char *conninfo = "dbname=shutdown_logsinfos user=postgres password=p0w2i8 port=5432";

    // Connect to the database
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    printf("Connected to the database successfully.\n");

    // Get shutdown time from user
    int target_hour, target_minute;
    get_shutdown_time(&target_hour, &target_minute);
    
    // Calculate seconds until target time
    int seconds_until_shutdown = calculate_seconds_until_target(target_hour, target_minute);
    
    // Get current time and target time for display
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    struct tm tm_target = *tm_now;
    tm_target.tm_hour = target_hour;
    tm_target.tm_min = target_minute;
    tm_target.tm_sec = 0;
    mktime(&tm_target); // Normalize
    
    char current_time[64], target_time[64];
    strftime(current_time, sizeof(current_time), "%Y-%m-%d %H:%M:%S", tm_now);
    strftime(target_time, sizeof(target_time), "%Y-%m-%d %H:%M:%S", &tm_target);
    
    printf("Current time: %s\n", current_time);
    printf("Scheduled shutdown at: %s\n", target_time);

    // Get the username and computer name
    char *username = get_username();
    char *computername = get_computername();
    printf("Username: %s\n", username);
    printf("Computer Name: %s\n", computername);

    // Log the shutdown event
    log_shutdown_event(conn, username, computername);

    // Display messages with countdown
    display_messages_with_delay(seconds_until_shutdown);

    // Close the database connection
    PQfinish(conn);

    // Shut down the computer
    shutdown_computer();

    return 0;
}





































/*

POSTGRESQL

        CREATE DATABASE shutdown_logsinfos;

        \c shutdown_logsinfos;

        CREATE TABLE shutdown_events (
            id SERIAL PRIMARY KEY,
            event_time TIMESTAMP NOT NULL,
            username TEXT NOT NULL,
            computername TEXT NOT NULL
        );


        psql -d database -U user
        psql -d postgres -U postgres




        psql -d shutdown_logsinfos -U postgres
        SELECT * FROM shutdown_events;








WINDOWS

        gcc shutdown_program_Time_POSTGRESQL_moreInfosSCHERLER.c -o shutdown_program_Time_POSTGRESQL_moreInfosSCHERLER.exe -I "C:\Program Files\PostgreSQL\17\include" -L "C:\Program Files\PostgreSQL\17\lib" -lpq

        gcc shutdown_program_Time_POSTGRESQL_moreInfosSCHERLER.c -o shutdown_program_Time_POSTGRESQL_moreInfosSCHERLER.exe -I "C:\Program Files\PostgreSQL\16\include" -L "C:\Program Files\PostgreSQL\16\lib" -lpq




LINUX:
        gcc -o shutdown_logger shutdown_logger.c -lpq ./shutdown_logger        









*/






