





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

// Function to get the shutdown delay in minutes from the user
int get_shutdown_delay() {
    int minutes;
    printf("Enter the shutdown delay in minutes: ");
    scanf("%d", &minutes);
    return minutes;
}

// Function to display messages with a delay
void display_messages_with_delay(int minutes) {
    int seconds = minutes * 60;
    while (seconds > 0) {
        printf("Shutting down in %d seconds...\n", seconds);
        sleep(10); // Delay for 10 seconds
        seconds -= 10;
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

    // Get the shutdown delay from the user
    int shutdown_delay_minutes = get_shutdown_delay();

    // Get the username and computer name
    char *username = get_username();
    char *computername = get_computername();
    printf("Username: %s\n", username);
    printf("Computer Name: %s\n", computername);

    // Log the shutdown event
    log_shutdown_event(conn, username, computername);

    // Display messages with a delay
    display_messages_with_delay(shutdown_delay_minutes);

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

        gcc shutdown_program_Time_POSTGRESQL_moreInfos.c -o shutdown_program_Time_POSTGRESQL_moreInfos.exe -I "C:\Program Files\PostgreSQL\17\include" -L "C:\Program Files\PostgreSQL\17\lib" -lpq

        gcc shutdown_program_Time_POSTGRESQL_moreInfos.c -o shutdown_program_Time_POSTGRESQL_moreInfos.exe -I "C:\Program Files\PostgreSQL\16\include" -L "C:\Program Files\PostgreSQL\16\lib" -lpq




LINUX:
        gcc -o shutdown_logger shutdown_logger.c -lpq ./shutdown_logger        









*/






