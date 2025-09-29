#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_EVENTS 100
#define MAX_TITLE_LEN 100
#define MAX_LOCATION_LEN 100
#define MAX_DESCRIPTION_LEN 200
#define FILENAME "events.txt"
#define ADMIN_PASSWORD "admin123"

typedef struct
{
    int id;
    char title[MAX_TITLE_LEN];
    char date[11];
    char time[6];
    char description[MAX_DESCRIPTION_LEN];
    char location[MAX_LOCATION_LEN];
} Event;

Event events[MAX_EVENTS];
int eventCount = 0;
int isAdmin = 0;

void login();
void displayMenu();
void addEvent();
void viewEvents();
void editEvent();
void deleteEvent();
void saveEvents();
void loadEvents();
void searchEvents();
void eventSummary();
int validateDate(const char *date);
int validateTime(const char *time);
// void sortEvents();
// int compareDates(const char *date1, const char *date2);
void toLowerCase(char *str);
int isLeapYear(int year);
int isValidDate(int day, int month, int year);
void clearInputBuffer();

int main()
{
    loadEvents();
    login();

    int choice;
    do
    {
        displayMenu();
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input! Please enter a number.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice)
        {
        case 1:
            if (isAdmin)
                addEvent();
            else
                printf("Access denied! Admin only feature.\n");
            break;
        case 2:
            viewEvents();
            break;
        case 3:
            if (isAdmin)
                editEvent();
            else
                printf("Access denied! Admin only feature.\n");
            break;
        case 4:
            if (isAdmin)
                deleteEvent();
            else
                printf("Access denied! Admin only feature.\n");
            break;
        case 5:
            searchEvents();
            break;
        // case 6:
        //     if (isAdmin)
        //         sortEvents();
        //     else
        //         printf("Access denied! Admin only feature.\n");
        //     break;
        case 6:
            eventSummary();
            break;
        case 7:
            login();
            break;
        case 8:
            saveEvents();
            printf("Exiting program. Goodbye!\n");
            break;
        default:
            printf("Invalid choice! Please try again.\n");
        }
        printf("\nPress Enter to continue...");
        clearInputBuffer();
    } while (choice != 9);

    return 0;
}

void clearInputBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

void login()
{
    char password[50];
    printf("=== EventEase Login ===\n");
    printf("Enter admin password (or press Enter for guest access): ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0; // Remove newline

    if (strcmp(password, ADMIN_PASSWORD) == 0)
    {
        isAdmin = 1;
        printf("Admin access granted!\n");
    }
    else
    {
        isAdmin = 0;
        printf("Guest access granted.\n");
    }
}

void displayMenu()
{
    printf("\n=== EventEase - Event Management System ===\n");
    printf("1. Add New Event (Admin Only)\n");
    printf("2. View All Events\n");
    printf("3. Edit an Event (Admin Only)\n");
    printf("4. Delete an Event (Admin Only)\n");
    printf("5. Search Events\n");
    printf("6. Event Summary\n");
    printf("7. Switch User\n");
    printf("8. Exit\n");
}

void addEvent()
{
    if (eventCount >= MAX_EVENTS)
    {
        printf("Event storage full! Cannot add more events.\n");
        return;
    }

    Event newEvent;
    newEvent.id = eventCount > 0 ? events[eventCount - 1].id + 1 : 1;

    printf("Enter event title: ");
    fgets(newEvent.title, sizeof(newEvent.title), stdin);
    newEvent.title[strcspn(newEvent.title, "\n")] = 0;

    printf("Enter event description: ");
    fgets(newEvent.description, sizeof(newEvent.description), stdin);
    newEvent.description[strcspn(newEvent.description, "\n")] = 0;

    printf("Enter event location: ");
    fgets(newEvent.location, sizeof(newEvent.location), stdin);
    newEvent.location[strcspn(newEvent.location, "\n")] = 0;

    do
    {
        printf("Enter event date (YYYY-MM-DD): ");
        fgets(newEvent.date, sizeof(newEvent.date), stdin);
        newEvent.date[strcspn(newEvent.date, "\n")] = 0;
    } while (!validateDate(newEvent.date));

    do
    {
        printf("Enter event time (HH:MM): ");
        fgets(newEvent.time, sizeof(newEvent.time), stdin);
        newEvent.time[strcspn(newEvent.time, "\n")] = 0;
    } while (!validateTime(newEvent.time));

    events[eventCount++] = newEvent;
    saveEvents();
    printf("Event added successfully with ID: %d\n", newEvent.id);
}

void viewEvents()
{
    if (eventCount == 0)
    {
        printf("No events to display.\n");
        return;
    }

    printf("\n=== All Events ===\n");
    printf("ID    Title                  Date         Time   Location   Description\n");
    printf("-----------------------------------------------------------------------\n");
    for (int i = 0; i < eventCount; i++)
    {
        printf("%-5d %-22s %-12s %-6s %-10s %s\n",
               events[i].id,
               events[i].title,
               events[i].date,
               events[i].time,
               events[i].location,
               events[i].description);
    }

    // REMOVE THIS PART - it's causing the input buffer issue
    /*
    printf("\nWould you like to see detailed descriptions? (y/n): ");
    char choice;
    scanf("%c", &choice);
    clearInputBuffer();

    if(choice == 'y' || choice == 'Y') {
        printf("\n=== Event Details ===\n");
        for(int i = 0; i < eventCount; i++) {
            printf("\nID: %d\nTitle: %s\nDate: %s\nTime: %s\nLocation: %s\nDescription: %s\n",
                   events[i].id, events[i].title, events[i].date,
                   events[i].time, events[i].location, events[i].description);
            printf("----------------------------------------\n");
        }
    }
    */
}

void editEvent()
{
    if (eventCount == 0)
    {
        printf("No events to edit.\n");
        return;
    }

    int id;
    printf("Enter event ID to edit: ");
    if (scanf("%d", &id) != 1)
    {
        printf("Invalid input! Please enter a number.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer(); // Consume newline

    int found = -1;
    for (int i = 0; i < eventCount; i++)
    {
        if (events[i].id == id)
        {
            found = i;
            break;
        }
    }

    if (found == -1)
    {
        printf("Event with ID %d not found.\n", id);
        return;
    }

    printf("Editing Event ID: %d\n", id);
    printf("Leave field blank to keep current value.\n");

    char input[100];

    printf("Current title: %s\n", events[found].title);
    printf("Enter new title: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0)
    {
        strcpy(events[found].title, input);
    }

    printf("Current date: %s\n", events[found].date);
    do
    {
        printf("Enter new date (YYYY-MM-DD): ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0)
        {
            if (validateDate(input))
            {
                strcpy(events[found].date, input);
                break;
            }
        }
        else
        {
            break;
        }
    } while (1);

    printf("Current time: %s\n", events[found].time);
    do
    {
        printf("Enter new time (HH:MM): ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0)
        {
            if (validateTime(input))
            {
                strcpy(events[found].time, input);
                break;
            }
        }
        else
        {
            break;
        }
    } while (1);

    printf("Current location: %s\n", events[found].location);
    printf("Enter new location: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0)
    {
        strcpy(events[found].location, input);
    }

    printf("Current description: %s\n", events[found].description);
    printf("Enter new description: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0)
    {
        strcpy(events[found].description, input);
    }

    saveEvents();
    printf("Event updated successfully.\n");
}

void deleteEvent()
{
    if (eventCount == 0)
    {
        printf("No events to delete.\n");
        return;
    }

    int id;
    printf("Enter event ID to delete: ");
    if (scanf("%d", &id) != 1)
    {
        printf("Invalid input! Please enter a number.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer(); // Consume newline

    int found = -1;
    for (int i = 0; i < eventCount; i++)
    {
        if (events[i].id == id)
        {
            found = i;
            break;
        }
    }

    if (found == -1)
    {
        printf("Event with ID %d not found.\n", id);
        return;
    }

    // Confirm deletion
    char confirm;
    printf("Are you sure you want to delete event '%s'? (y/n): ", events[found].title);
    if (scanf("%c", &confirm) != 1)
    {
        printf("Invalid input!\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer(); // Consume newline

    if (confirm == 'y' || confirm == 'Y')
    {
        // Shift all events after the found index to the left
        for (int i = found; i < eventCount - 1; i++)
        {
            events[i] = events[i + 1];
        }
        eventCount--;
        saveEvents();
        printf("Event deleted successfully.\n");
    }
    else
    {
        printf("Deletion cancelled.\n");
    }
}

void searchEvents()
{
    if (eventCount == 0)
    {
        printf("No events to search.\n");
        return;
    }

    int choice;
    printf("Search by:\n");
    printf("1. Date\n");
    printf("2. Title\n");
    printf("3. Location\n");
    printf("Enter your choice: ");
    if (scanf("%d", &choice) != 1)
    {
        printf("Invalid input! Please enter a number.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer(); // Consume newline

    char searchTerm[100];
    int found = 0;

    switch (choice)
    {
    case 1: // Search by date
        printf("Enter date to search (YYYY-MM-DD): ");
        fgets(searchTerm, sizeof(searchTerm), stdin);
        searchTerm[strcspn(searchTerm, "\n")] = 0;

        if (!validateDate(searchTerm))
        {
            printf("Invalid date format.\n");
            return;
        }

        printf("\n=== Events on %s ===\n", searchTerm);
        printf("ID    Title                Time   Location\n");
        printf("------------------------------------------\n");
        for (int i = 0; i < eventCount; i++)
        {
            if (strcmp(events[i].date, searchTerm) == 0)
            {
                printf("%-5d %-20s %-6s %s\n",
                       events[i].id, events[i].title,
                       events[i].time, events[i].location);
                found = 1;
            }
        }
        break;

    case 2: // Search by title
        printf("Enter title to search: ");
        fgets(searchTerm, sizeof(searchTerm), stdin);
        searchTerm[strcspn(searchTerm, "\n")] = 0;
        toLowerCase(searchTerm);

        printf("\n=== Events with '%s' in title ===\n", searchTerm);
        printf("ID    Title                Date       Time   Location\n");
        printf("----------------------------------------------------\n");
        for (int i = 0; i < eventCount; i++)
        {
            char tempTitle[MAX_TITLE_LEN];
            strcpy(tempTitle, events[i].title);
            toLowerCase(tempTitle);

            if (strstr(tempTitle, searchTerm) != NULL)
            {
                printf("%-5d %-20s %-10s %-6s %s\n",
                       events[i].id, events[i].title, events[i].date,
                       events[i].time, events[i].location);
                found = 1;
            }
        }
        break;

    case 3: // Search by location
        printf("Enter location to search: ");
        fgets(searchTerm, sizeof(searchTerm), stdin);
        searchTerm[strcspn(searchTerm, "\n")] = 0;
        toLowerCase(searchTerm);

        printf("\n=== Events in '%s' ===\n", searchTerm);
        printf("ID    Title                Date       Time   Location\n");
        printf("----------------------------------------------------\n");
        for (int i = 0; i < eventCount; i++)
        {
            char tempLocation[MAX_LOCATION_LEN];
            strcpy(tempLocation, events[i].location);
            toLowerCase(tempLocation);

            if (strstr(tempLocation, searchTerm) != NULL)
            {
                printf("%-5d %-20s %-10s %-6s %s\n",
                       events[i].id, events[i].title, events[i].date,
                       events[i].time, events[i].location);
                found = 1;
            }
        }
        break;

    default:
        printf("Invalid choice.\n");
        return;
    }

    if (!found)
    {
        printf("No events found matching your search.\n");
    }
}

void saveEvents()
{
    FILE *file = fopen(FILENAME, "w");
    if (file == NULL)
    {
        printf("Error opening file for writing.\n");
        return;
    }

    for (int i = 0; i < eventCount; i++)
    {
        fprintf(file, "%d|%s|%s|%s|%s|%s\n",
                events[i].id, events[i].title, events[i].date,
                events[i].time, events[i].location, events[i].description);
    }

    fclose(file);
}

void loadEvents()
{
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL)
    {
        printf("No existing events file found. Starting fresh.\n");
        return;
    }

    eventCount = 0;
    char line[500];

    while (fgets(line, sizeof(line), file) && eventCount < MAX_EVENTS)
    {
        line[strcspn(line, "\n")] = 0; // Remove newline

        char *token = strtok(line, "|");
        if (token == NULL)
            continue;

        events[eventCount].id = atoi(token);

        token = strtok(NULL, "|");
        if (token)
            strcpy(events[eventCount].title, token);

        token = strtok(NULL, "|");
        if (token)
            strcpy(events[eventCount].date, token);

        token = strtok(NULL, "|");
        if (token)
            strcpy(events[eventCount].time, token);

        token = strtok(NULL, "|");
        if (token)
            strcpy(events[eventCount].location, token);

        token = strtok(NULL, "|");
        if (token)
            strcpy(events[eventCount].description, token);

        eventCount++;
    }

    fclose(file);
    printf("Loaded %d events from file.\n", eventCount);
}

void eventSummary()
{
    printf("\n=== Event Summary ===\n");
    printf("Total number of events: %d\n", eventCount);

    if (eventCount == 0)
        return;

    // Count events by year, month, and day
    int yearCount[100] = {0}; // Assuming events within 100 years
    int monthCount[13] = {0}; // Index 1-12 for months
    int dayCount[32] = {0};   // Index 1-31 for days

    int currentYear, currentMonth, currentDay;
    for (int i = 0; i < eventCount; i++)
    {
        sscanf(events[i].date, "%d-%d-%d", &currentYear, &currentMonth, &currentDay);

        // Adjust year index (assuming events between 2000-2099)
        int yearIndex = currentYear - 2000;
        if (yearIndex >= 0 && yearIndex < 100)
        {
            yearCount[yearIndex]++;
        }

        if (currentMonth >= 1 && currentMonth <= 12)
        {
            monthCount[currentMonth]++;
        }

        if (currentDay >= 1 && currentDay <= 31)
        {
            dayCount[currentDay]++;
        }
    }

    // Display events by year
    printf("\nEvents by year:\n");
    for (int i = 0; i < 100; i++)
    {
        if (yearCount[i] > 0)
        {
            printf("  %d: %d events\n", 2000 + i, yearCount[i]);
        }
    }

    // Display events by month
    printf("\nEvents by month:\n");
    char *months[] = {"", "January", "February", "March", "April", "May", "June",
                      "July", "August", "September", "October", "November", "December"};
    for (int i = 1; i <= 12; i++)
    {
        if (monthCount[i] > 0)
        {
            printf("  %s: %d events\n", months[i], monthCount[i]);
        }
    }

    // Display events by day
    printf("\nEvents by day:\n");
    for (int i = 1; i <= 31; i++)
    {
        if (dayCount[i] > 0)
        {
            printf("  %d: %d events\n", i, dayCount[i]);
        }
    }
}

int validateDate(const char *date)
{
    if (strlen(date) != 10)
        return 0;
    if (date[4] != '-' || date[7] != '-')
        return 0;

    int year, month, day;
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3)
        return 0;

    return isValidDate(day, month, year);
}

int isValidDate(int day, int month, int year)
{
    if (year < 2000 || year > 2100)
        return 0;
    if (month < 1 || month > 12)
        return 0;
    if (day < 1 || day > 31)
        return 0;

    // Check for months with 30 days
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30)
        return 0;

    // Check for February
    if (month == 2)
    {
        if (isLeapYear(year))
        {
            if (day > 29)
                return 0;
        }
        else
        {
            if (day > 28)
                return 0;
        }
    }

    return 1;
}

int isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int validateTime(const char *time)
{
    if (strlen(time) != 5)
        return 0;
    if (time[2] != ':')
        return 0;

    int hour, minute;
    if (sscanf(time, "%d:%d", &hour, &minute) != 2)
        return 0;

    if (hour < 0 || hour > 23)
        return 0;
    if (minute < 0 || minute > 59)
        return 0;

    return 1;
}

void toLowerCase(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        str[i] = tolower(str[i]);
    }
}

/*

int compareDates(const char *date1, const char *date2)
{
    int y1, m1, d1, y2, m2, d2;
    sscanf(date1, "%d-%d-%d", &y1, &m1, &d1);
    sscanf(date2, "%d-%d-%d", &y2, &m2, &d2);

    if (y1 != y2)
        return y1 - y2;
    if (m1 != m2)
        return m1 - m2;
    return d1 - d2;
}

void sortEvents()
{
    if (eventCount == 0)
    {
        printf("No events to sort.\n");
        return;
    }

    int choice;
    printf("Sort by:\n");
    printf("1. Date/Time\n");
    printf("2. Title (Alphabetical)\n");
    printf("3. Location\n");
    printf("Enter your choice: ");
    if (scanf("%d", &choice) != 1)
    {
        printf("Invalid input! Please enter a number.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer(); // Consume newline

    // Bubble sort implementation
    for (int i = 0; i < eventCount - 1; i++)
    {
        for (int j = 0; j < eventCount - i - 1; j++)
        {
            int swap = 0;

            switch (choice)
            {
            case 1: // Sort by date/time
                if (compareDates(events[j].date, events[j + 1].date) > 0)
                {
                    swap = 1;
                }
                else if (compareDates(events[j].date, events[j + 1].date) == 0)
                {
                    // If dates are equal, compare times
                    if (strcmp(events[j].time, events[j + 1].time) > 0)
                    {
                        swap = 1;
                    }
                }
                break;

            case 2: // Sort by title
                if (strcasecmp(events[j].title, events[j + 1].title) > 0)
                {
                    swap = 1;
                }
                break;

            case 3: // Sort by location
                if (strcasecmp(events[j].location, events[j + 1].location) > 0)
                {
                    swap = 1;
                }
                break;

            default:
                printf("Invalid choice.\n");
                return;
            }

            if (swap)
            {
                Event temp = events[j];
                events[j] = events[j + 1];
                events[j + 1] = temp;
            }
        }
    }

    saveEvents();
    printf("Events sorted successfully.\n");
    viewEvents();
}

*/