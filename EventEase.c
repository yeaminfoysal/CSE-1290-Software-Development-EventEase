/* eventease.c
   EventEase - Event Management System (single-file C implementation)
   Features:
   - Add/View/Edit/Delete/Search/Sort events
   - Admin & Guest roles (hardcoded admin password)
   - Save/load events to/from a binary file "events.dat"
   - Upcoming events using time.h
   - Event summary (counts per day/month/year)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define DATA_FILE "events.dat"
#define ADMIN_PASSWORD "admin123"
#define MAX_STR 256

typedef struct {
    unsigned int id;           // unique event id
    char title[MAX_STR];
    char date[11];   // "YYYY-MM-DD"
    char time[6];    // "HH:MM"
    char location[MAX_STR];
    char description[512];
} Event;

typedef struct {
    Event *arr;
    size_t size;
    size_t capacity;
    unsigned int next_id;
} EventList;

/* ---------- Utility functions ---------- */

void strip_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    if (n == 0) return;
    if (s[n-1] == '\n') s[n-1] = '\0';
}

/* safe getline-like for stdin using fgets */
void read_line(const char *prompt, char *out, size_t out_size) {
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    if (fgets(out, (int)out_size, stdin) != NULL) {
        strip_newline(out);
    } else {
        /* EOF or error - set empty */
        out[0] = '\0';
    }
}

/* Case-insensitive substring search */
int contains_case_insensitive(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    size_t H = strlen(hay), N = strlen(needle);
    if (N == 0) return 1;
    for (size_t i = 0; i + N <= H; ++i) {
        size_t j;
        for (j = 0; j < N; ++j) {
            if (tolower((unsigned char)hay[i+j]) != tolower((unsigned char)needle[j])) break;
        }
        if (j == N) return 1;
    }
    return 0;
}

/* parse "YYYY-MM-DD" and "HH:MM" into struct tm; returns 0 on success */
int parse_datetime(const char *date, const char *time_s, struct tm *out_tm) {
    if (!date || !time_s || !out_tm) return -1;
    int y, m, d, hh, mm;
    if (sscanf(date, "%4d-%2d-%2d", &y, &m, &d) != 3) return -1;
    if (sscanf(time_s, "%2d:%2d", &hh, &mm) != 2) return -1;
    memset(out_tm, 0, sizeof(struct tm));
    out_tm->tm_year = y - 1900;
    out_tm->tm_mon  = m - 1;
    out_tm->tm_mday = d;
    out_tm->tm_hour = hh;
    out_tm->tm_min  = mm;
    out_tm->tm_sec  = 0;
    out_tm->tm_isdst = -1;
    return 0;
}

/* compare two events by datetime (for sorting) */
int compare_event_datetime(const void *a, const void *b) {
    const Event *ea = a;
    const Event *eb = b;
    struct tm ta, tb;
    if (parse_datetime(ea->date, ea->time, &ta) == 0 && parse_datetime(eb->date, eb->time, &tb) == 0) {
        time_t ta_t = mktime(&ta);
        time_t tb_t = mktime(&tb);
        if (ta_t < tb_t) return -1;
        if (ta_t > tb_t) return 1;
        return 0;
    }
    /* fallback alphabetical by date string */
    int r = strcmp(ea->date, eb->date);
    if (r != 0) return r;
    return strcmp(ea->time, eb->time);
}

/* compare by title */
int compare_event_title(const void *a, const void *b) {
    const Event *ea = a; const Event *eb = b;
    return strcasecmp(ea->title, eb->title);
}

/* compare by location */
int compare_event_location(const void *a, const void *b) {
    const Event *ea = a; const Event *eb = b;
    return strcasecmp(ea->location, eb->location);
}

/* ---------- EventList management ---------- */

void init_event_list(EventList *lst) {
    lst->arr = NULL;
    lst->size = 0;
    lst->capacity = 0;
    lst->next_id = 1;
}

void free_event_list(EventList *lst) {
    free(lst->arr);
    lst->arr = NULL;
    lst->size = lst->capacity = 0;
}

void ensure_capacity(EventList *lst, size_t min_cap) {
    if (lst->capacity >= min_cap) return;
    size_t newcap = lst->capacity ? lst->capacity * 2 : 8;
    while (newcap < min_cap) newcap *= 2;
    Event *tmp = realloc(lst->arr, newcap * sizeof(Event));
    if (!tmp) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    lst->arr = tmp;
    lst->capacity = newcap;
}

Event *find_event_by_id(EventList *lst, unsigned int id) {
    for (size_t i = 0; i < lst->size; ++i) {
        if (lst->arr[i].id == id) return &lst->arr[i];
    }
    return NULL;
}

/* ---------- File I/O ---------- */

int save_events(EventList *lst) {
    FILE *f = fopen(DATA_FILE, "wb");
    if (!f) {
        perror("fopen for write");
        return -1;
    }
    /* Write header: next_id and count */
    if (fwrite(&lst->next_id, sizeof(lst->next_id), 1, f) != 1) { fclose(f); return -1; }
    size_t count = lst->size;
    if (fwrite(&count, sizeof(count), 1, f) != 1) { fclose(f); return -1; }
    if (count > 0) {
        if (fwrite(lst->arr, sizeof(Event), count, f) != count) { fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}

int load_events(EventList *lst) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) {
        /* no file yet - that's ok */
        return 0;
    }
    unsigned int next_id = 1;
    size_t count = 0;
    if (fread(&next_id, sizeof(next_id), 1, f) != 1) { fclose(f); return -1; }
    if (fread(&count, sizeof(count), 1, f) != 1) { fclose(f); return -1; }
    if (count > 0) {
        ensure_capacity(lst, count);
        if (fread(lst->arr, sizeof(Event), count, f) != count) { fclose(f); return -1; }
        lst->size = count;
    }
    lst->next_id = next_id;
    fclose(f);
    return 0;
}

/* ---------- UI helpers ---------- */

void print_event_header() {
    printf("--------------------------------------------------------------------------------\n");
    printf("| ID  |     Date    | Time  | %-20s | %-12s |\n", "Title", "Location");
    printf("--------------------------------------------------------------------------------\n");
}

void print_event_row(const Event *e) {
    char title_short[21] = {0}, loc_short[13] = {0};
    strncpy(title_short, e->title, 20);
    strncpy(loc_short, e->location, 12);
    printf("| %3u | %10s | %5s | %-20s | %-12s |\n", e->id, e->date, e->time, title_short, loc_short);
}

/* ---------- Core features ---------- */

void add_event(EventList *lst) {
    Event tmp;
    char buf[MAX_STR];
    printf("\n-- Add New Event (Admin) --\n");

    read_line("Title: ", tmp.title, sizeof(tmp.title));
    if (strlen(tmp.title) == 0) {
        puts("Title cannot be empty. Cancelled.");
        return;
    }

    while (1) {
        read_line("Date (YYYY-MM-DD): ", tmp.date, sizeof(tmp.date));
        struct tm t;
        if (parse_datetime(tmp.date, "00:00", &t) == 0) break;
        puts("Invalid date format. Try again.");
    }

    while (1) {
        read_line("Time (HH:MM): ", tmp.time, sizeof(tmp.time));
        /* simple validation */
        int hh, mm;
        if (sscanf(tmp.time, "%2d:%2d", &hh, &mm) == 2 && hh >= 0 && hh < 24 && mm >= 0 && mm < 60) break;
        puts("Invalid time format. Try again.");
    }

    read_line("Location: ", tmp.location, sizeof(tmp.location));
    read_line("Description: ", tmp.description, sizeof(tmp.description));

    /* assign id */
    tmp.id = lst->next_id++;
    ensure_capacity(lst, lst->size + 1);
    lst->arr[lst->size++] = tmp;

    if (save_events(lst) == 0) {
        printf("Event added and saved with ID %u.\n", tmp.id);
    } else {
        puts("Event added but failed to save to disk.");
    }
}

void view_all_events(EventList *lst) {
    if (lst->size == 0) {
        puts("\nNo events available.\n");
        return;
    }
    printf("\n-- All Events (%zu) --\n", lst->size);
    print_event_header();
    for (size_t i = 0; i < lst->size; ++i) {
        print_event_row(&lst->arr[i]);
    }
    printf("--------------------------------------------------------------------------------\n\n");
}

void show_event_details(const Event *e) {
    if (!e) return;
    printf("\n--- Event Details (ID %u) ---\n", e->id);
    printf("Title      : %s\n", e->title);
    printf("Date       : %s\n", e->date);
    printf("Time       : %s\n", e->time);
    printf("Location   : %s\n", e->location);
    printf("Description: %s\n", e->description);
    printf("-------------------------------\n");
}

void edit_event(EventList *lst) {
    char buf[64];
    read_line("Enter Event ID to edit: ", buf, sizeof(buf));
    unsigned int id = (unsigned int)atoi(buf);
    Event *e = find_event_by_id(lst, id);
    if (!e) { printf("No event with ID %u\n", id); return; }
    show_event_details(e);
    printf("Press ENTER to keep current value.\n");

    char tmp[MAX_STR];
    read_line("New Title: ", tmp, sizeof(tmp));
    if (strlen(tmp) > 0) strncpy(e->title, tmp, sizeof(e->title));

    read_line("New Date (YYYY-MM-DD): ", tmp, sizeof(tmp));
    if (strlen(tmp) > 0) {
        struct tm t; if (parse_datetime(tmp, "00:00", &t) == 0) strncpy(e->date, tmp, sizeof(e->date));
        else puts("Invalid date format; keeping old.");
    }

    read_line("New Time (HH:MM): ", tmp, sizeof(tmp));
    if (strlen(tmp) > 0) {
        int hh, mm;
        if (sscanf(tmp, "%2d:%2d", &hh, &mm) == 2 && hh>=0 && hh<24 && mm>=0 && mm<60) strncpy(e->time, tmp, sizeof(e->time));
        else puts("Invalid time format; keeping old.");
    }

    read_line("New Location: ", tmp, sizeof(tmp));
    if (strlen(tmp) > 0) strncpy(e->location, tmp, sizeof(e->location));

    read_line("New Description: ", tmp, sizeof(tmp));
    if (strlen(tmp) > 0) strncpy(e->description, tmp, sizeof(e->description));

    if (save_events(lst) == 0) puts("Event updated and saved.");
    else puts("Event updated but failed to save.");
}

void delete_event(EventList *lst) {
    char buf[64];
    read_line("Enter Event ID to delete: ", buf, sizeof(buf));
    unsigned int id = (unsigned int)atoi(buf);
    size_t idx = (size_t)-1;
    for (size_t i = 0; i < lst->size; ++i) if (lst->arr[i].id == id) { idx = i; break; }
    if (idx == (size_t)-1) { printf("No event with ID %u\n", id); return; }
    show_event_details(&lst->arr[idx]);
    read_line("Type YES to confirm deletion: ", buf, sizeof(buf));
    if (strcmp(buf, "YES") == 0) {
        /* shift */
        for (size_t i = idx + 1; i < lst->size; ++i) lst->arr[i-1] = lst->arr[i];
        lst->size--;
        if (save_events(lst) == 0) puts("Event deleted and saved.");
        else puts("Event deleted but failed to save.");
    } else {
        puts("Deletion cancelled.");
    }
}

/* Search helpers */
void search_by_date(EventList *lst) {
    char date[16];
    read_line("Enter date (YYYY-MM-DD): ", date, sizeof(date));
    if (strlen(date) == 0) { puts("No date entered."); return; }
    int found = 0;
    print_event_header();
    for (size_t i = 0; i < lst->size; ++i) {
        if (strcmp(lst->arr[i].date, date) == 0) {
            print_event_row(&lst->arr[i]);
            found = 1;
        }
    }
    if (!found) puts("No events on that date.");
    else printf("--------------------------------------------------------------------------------\n");
}

void search_by_name(EventList *lst) {
    char q[128];
    read_line("Enter name/title to search: ", q, sizeof(q));
    if (strlen(q)==0) return;
    int found = 0;
    print_event_header();
    for (size_t i = 0; i < lst->size; ++i) {
        if (contains_case_insensitive(lst->arr[i].title, q)) { print_event_row(&lst->arr[i]); found = 1; }
    }
    if (!found) puts("No matching events found.");
    else printf("--------------------------------------------------------------------------------\n");
}

void search_by_location(EventList *lst) {
    char q[128];
    read_line("Enter location to search: ", q, sizeof(q));
    if (strlen(q)==0) return;
    int found = 0;
    print_event_header();
    for (size_t i = 0; i < lst->size; ++i) {
        if (contains_case_insensitive(lst->arr[i].location, q)) { print_event_row(&lst->arr[i]); found = 1; }
    }
    if (!found) puts("No matching events found.");
    else printf("--------------------------------------------------------------------------------\n");
}

/* Sorting options */
void sort_events(EventList *lst) {
    if (lst->size == 0) { puts("No events to sort."); return; }
    puts("Sort by:\n1) Date/Time\n2) Alphabetical (Name)\n3) Location\nChoose (1-3): ");
    char opt[8]; read_line("> ", opt, sizeof(opt));
    int o = atoi(opt);
    if (o == 1) qsort(lst->arr, lst->size, sizeof(Event), compare_event_datetime);
    else if (o == 2) qsort(lst->arr, lst->size, sizeof(Event), compare_event_title);
    else if (o == 3) qsort(lst->arr, lst->size, sizeof(Event), compare_event_location);
    else { puts("Invalid choice."); return; }
    if (save_events(lst) == 0) puts("Sorted and saved.");
    else puts("Sorted but save failed.");
}

/* Event summary */
void event_summary(EventList *lst) {
    printf("\n-- Event Summary --\nTotal events: %zu\n", lst->size);
    if (lst->size == 0) return;

    /* Count by date (simple approach using temporary arrays) */
    /* We'll create a dynamic list of strings for unique dates and counts */
    char **dates = NULL;
    size_t *counts = NULL;
    size_t unique = 0;

    for (size_t i = 0; i < lst->size; ++i) {
        char *d = lst->arr[i].date;
        size_t j;
        for (j = 0; j < unique; ++j) if (strcmp(dates[j], d) == 0) break;
        if (j < unique) counts[j]++;
        else {
            dates = realloc(dates, (unique+1)*sizeof(char*));
            counts = realloc(counts, (unique+1)*sizeof(size_t));
            dates[unique] = strdup(d);
            counts[unique] = 1;
            unique++;
        }
    }
    printf("\nEvents by date:\n");
    for (size_t i = 0; i < unique; ++i) {
        printf("  %s : %zu\n", dates[i], counts[i]);
        free(dates[i]);
    }
    free(dates); free(counts);
}

/* Upcoming events: events with datetime > now */
void upcoming_events(EventList *lst) {
    time_t now = time(NULL);
    int found = 0;
    print_event_header();
    for (size_t i = 0; i < lst->size; ++i) {
        struct tm tm_e;
        if (parse_datetime(lst->arr[i].date, lst->arr[i].time, &tm_e) == 0) {
            time_t t_event = mktime(&tm_e);
            if (t_event > now) { print_event_row(&lst->arr[i]); found = 1; }
        }
    }
    if (!found) puts("No upcoming events.");
    else printf("--------------------------------------------------------------------------------\n");
}

/* show details by ID (for both users) */
void view_event_by_id(EventList *lst) {
    char buf[64];
    read_line("Enter event ID to view details: ", buf, sizeof(buf));
    unsigned int id = (unsigned int)atoi(buf);
    Event *e = find_event_by_id(lst, id);
    if (!e) { puts("Event not found."); return; }
    show_event_details(e);
}

/* ---------- Menus and main loop ---------- */

void admin_menu(EventList *lst) {
    for (;;) {
        puts("\n--- Admin Menu ---");
        puts("1) Add Event");
        puts("2) View All Events");
        puts("3) View Event Details (by ID)");
        puts("4) Edit Event");
        puts("5) Delete Event");
        puts("6) Search Events");
        puts("7) Sort Events");
        puts("8) Upcoming Events");
        puts("9) Event Summary");
        puts("0) Logout");
        char opt[8]; read_line("Choose: ", opt, sizeof(opt));
        int c = atoi(opt);
        switch (c) {
            case 1: add_event(lst); break;
            case 2: view_all_events(lst); break;
            case 3: view_event_by_id(lst); break;
            case 4: edit_event(lst); break;
            case 5: delete_event(lst); break;
            case 6:
                puts("Search by:\n1) Date\n2) Name\n3) Location");
                read_line("> ", opt, sizeof(opt));
                if (atoi(opt) == 1) search_by_date(lst);
                else if (atoi(opt) == 2) search_by_name(lst);
                else if (atoi(opt) == 3) search_by_location(lst);
                else puts("Invalid.");
                break;
            case 7: sort_events(lst); break;
            case 8: upcoming_events(lst); break;
            case 9: event_summary(lst); break;
            case 0: return;
            default: puts("Invalid choice.");
        }
    }
}

void guest_menu(EventList *lst) {
    for (;;) {
        puts("\n--- Guest Menu ---");
        puts("1) View All Events");
        puts("2) View Event Details (by ID)");
        puts("3) Search Events");
        puts("4) Upcoming Events");
        puts("0) Logout");
        char opt[8]; read_line("Choose: ", opt, sizeof(opt));
        int c = atoi(opt);
        switch (c) {
            case 1: view_all_events(lst); break;
            case 2: view_event_by_id(lst); break;
            case 3:
                puts("Search by:\n1) Date\n2) Name\n3) Location");
                read_line("> ", opt, sizeof(opt));
                if (atoi(opt) == 1) search_by_date(lst);
                else if (atoi(opt) == 2) search_by_name(lst);
                else if (atoi(opt) == 3) search_by_location(lst);
                else puts("Invalid.");
                break;
            case 4: upcoming_events(lst); break;
            case 0: return;
            default: puts("Invalid choice.");
        }
    }
}

int main(void) {
    EventList lst;
    init_event_list(&lst);
    if (load_events(&lst) != 0) {
        puts("Warning: failed to load events file.");
    } else {
        printf("Loaded %zu events. Next event id: %u\n", lst.size, lst.next_id);
    }

    puts("Welcome to EventEase - Event Management System");
    for (;;) {
        puts("\nMain Menu:");
        puts("1) Admin Login");
        puts("2) Continue as Guest");
        puts("0) Exit");
        char opt[32]; read_line("Choose: ", opt, sizeof(opt));
        int c = atoi(opt);
        if (c == 1) {
            char pw[128];
            read_line("Enter admin password: ", pw, sizeof(pw));
            if (strcmp(pw, ADMIN_PASSWORD) == 0) {
                puts("Admin access granted.");
                admin_menu(&lst);
            } else {
                puts("Incorrect password.");
            }
        } else if (c == 2) {
            guest_menu(&lst);
        } else if (c == 0) {
            puts("Exiting. Goodbye!");
            break;
        } else {
            puts("Invalid choice.");
        }
    }

    free_event_list(&lst);
    return 0;
}
