#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "common.h"
#define MAX_TEAMS 300
#define MAX_SIMULATIONS 1000


void convert_str(char *dest, const char *src) {
    int length = strlen(src);
    // remove whitespace
    while (length > 0 && isspace((unsigned char)src[length - 1])) {
        length--;
    }
    // find start of non-whitespace characters
    int start = 0;
    while (src[start] && isspace((unsigned char)src[start])) {
        start++;
    }

    // set characters to lowercase and null terminate
    int i = 0;
    while (start < length) {
        dest[i++] = tolower(src[start++]);
    }
    dest[i] = '\0';
}

int load_teams(const char *filename, Team *teams) {
    FILE *file_ptr = fopen(filename, "r");
    if (file_ptr == NULL) {
        perror("fopen");
        return -1;
    }

    char line[100];
    int count = 0;
    int line_num = 0;
    char dummy;     // variable to track excess items in csv line

    while (fgets(line, sizeof(line), file_ptr) && count < MAX_TEAMS) {
        line_num++;

        if (sscanf(line, "%[^,],%lf,%lf,%lf,%c", teams[count].name, &teams[count].attack, &teams[count].midfield, &teams[count].defense, &dummy) != 4) {
            // malformed csv data
            fprintf(stderr, "Invalid items in line %d of CSV. Skipping this line...\n", line_num);
        }
        else {
            count++;
        }
    }
    fclose(file_ptr);
    return count;
}


int main() {
    Team teams[MAX_TEAMS];
    int num_teams = load_teams("football_teams.csv", teams);
    if (num_teams == -1) {
        printf("Failed to load teams from file.\n");
        exit(1);
    }

    int task_pipes[NUM_WORKERS][2]; // pipes for parent to worker communication
    int result_pipes[NUM_WORKERS][2]; // pipes for worker to parent communication
    pid_t pids[NUM_WORKERS];

    // create pipes and fork workers
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (pipe(task_pipes[i]) == -1 || pipe(result_pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(1);
        }
        // child
        else if (pids[i] == 0) {
            // close unused pipe ends
            close(task_pipes[i][1]);
            close(result_pipes[i][0]);
            run_worker(task_pipes[i][0], result_pipes[i][1], i);
        }
        else {
            // close unused pipe ends
            close(task_pipes[i][0]);
            close(result_pipes[i][1]);
        }
    }

    char home_input[40] = "";
    char away_input[40] = "";
    char home_name[40] = "";
    char away_name[40] = "";
    int num_simulations;
    double home_advantage;
    Team *home = NULL;
    Team *away = NULL;

    printf("Enter home team name: ");
    scanf(" %39[^\n]", home_input);     // read max of 39 characters
    convert_str(home_name, home_input);

    printf("Enter away team name: ");
    scanf(" %39[^\n]", away_input);
    convert_str(away_name, away_input);

    for (int i = 0; i < num_teams; i++) {
        char clean_csv_name[40];
        convert_str(clean_csv_name, teams[i].name);
        if (strcmp(clean_csv_name, home_name) == 0) {
            home = &teams[i];
        }
        if (strcmp(clean_csv_name, away_name) == 0) {
            away = &teams[i];
        }
    }

    if (home == NULL) {
        printf("Home team '%s' not found.\n", home_name);
        // clean up workers
        for (int i = 0; i < NUM_WORKERS; i++) {
            close(task_pipes[i][1]);
            close(result_pipes[i][0]);
        }
        return 1;
    }
    if (away == NULL) {
        printf("Away team '%s' not found.\n", away_name);
        // clean up workers
        for (int i = 0; i < NUM_WORKERS; i++) {
            close(task_pipes[i][1]);
            close(result_pipes[i][0]);
        }
        return 1;
    }

    printf("Enter number of simulations (1-1000): ");
    if (scanf("%d", &num_simulations) != 1) {
        printf("Invalid input. Please enter a valid integer.\n");
        // clean up workers
        for (int i = 0; i < NUM_WORKERS; i++) {
            close(task_pipes[i][1]);
            close(result_pipes[i][0]);
        }
        return 1;
    }
    if (num_simulations < 1) {
        printf("You must run at least 1 simulation.\n");
        // clean up workers
        for (int i = 0; i < NUM_WORKERS; i++) {
            close(task_pipes[i][1]);
            close(result_pipes[i][0]);
        }
        return 1;
    }
    else if (num_simulations > MAX_SIMULATIONS) {
        printf("Number of simulations cannot exceed %d.\n", MAX_SIMULATIONS);
        // clean up workers
        for (int i = 0; i < NUM_WORKERS; i++) {
            close(task_pipes[i][1]);
            close(result_pipes[i][0]);
        }
        return 1;
    }
    
    printf("A larger number indicates a greater home advantage.\n");
    printf("Enter home team advantage: ");
    if (scanf("%lf", &home_advantage) != 1) {
        printf("Invalid input. Please enter a valid number.\n");
        // clean up workers
        for (int i = 0; i < NUM_WORKERS; i++) {
            close(task_pipes[i][1]);
            close(result_pipes[i][0]);
        }
        return 1;
    }


    double home_overall = (home->attack + home->midfield + home->defense) / 3.0;
    double away_overall = (away->attack + away->midfield + away->defense) / 3.0;
    double home_chance_prob = (home->midfield / (home->midfield + away->midfield) * (home->midfield / 2.5));
    home_chance_prob = home_chance_prob + home_advantage + ((home_overall - away_overall) / 7.0);
    double away_chance_prob = (away->midfield / (home->midfield + away->midfield) * (away->midfield / 2.5));
    away_chance_prob = away_chance_prob - home_advantage + ((away_overall - home_overall) / 7.0);
    if (home_chance_prob < 2) {
        home_chance_prob = 2;
    }
    else if (home_chance_prob > 55) {
        home_chance_prob = 55;
    }

    if (away_chance_prob < 2) {
        away_chance_prob = 2;
    }
    else if (away_chance_prob > 55) {
        away_chance_prob = 55;
    }

    double home_goal_prob = (home->attack / (home->attack + away->defense)) * (home->attack / (2 * away->defense)) * 100.0;
    home_goal_prob = home_goal_prob + home_advantage - 12.0;
    double away_goal_prob = (away->attack / (away->attack + home->defense)) * (away->attack / (2 * home->defense)) * 100.0;
    away_goal_prob = away_goal_prob - home_advantage - 12.0;
    if (home_goal_prob < 1) {
        home_goal_prob = 1;
    }
    else if (home_goal_prob > 79) {
        home_goal_prob = 79;
    }

    if (away_goal_prob < 1) {
        away_goal_prob = 1;
    }
    else if (away_goal_prob > 79) {
        away_goal_prob = 79;
    }

    if (home_chance_prob < 10 && away_chance_prob < 10) {
        home_chance_prob = 10;
        away_chance_prob = 10;
    }

    if (home_goal_prob < 10 && away_goal_prob < 10) {
        home_goal_prob = 10;
        away_goal_prob = 10;
    }

    // send tasks to workers
    for (int i = 0; i < num_simulations; i++) {
        SimulationTask task;
        task.sim_id = i;
        task.home_chance_prob = home_chance_prob;
        task.home_goal_prob = home_goal_prob;
        task.away_chance_prob = away_chance_prob;
        task.away_goal_prob = away_goal_prob;
        task.home_name = home->name;
        task.away_name = away->name;

        int worker_id = i % NUM_WORKERS;
        if (write(task_pipes[worker_id][1], &task, sizeof(SimulationTask)) == -1) {
            perror("write");
            exit(1);
        }
    }

     // close write end to signal no more tasks
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (close(task_pipes[i][1]) == -1) {
            perror("close");
            exit(1);
        }
    }

    int total_home_goals = 0;
    int total_away_goals = 0;
    int total_home_wins = 0;
    int total_draws = 0;
    int total_home_losses = 0;
    SimulationResult res;

    ssize_t bytes_read;

    // receive simulation results from workers
    for (int i = 0; i < NUM_WORKERS; i++) {
        while ((bytes_read = read(result_pipes[i][0], &res, sizeof(SimulationResult))) > 0) {
            if (bytes_read != sizeof(SimulationResult)) {
                fprintf(stderr, "Error reading result from worker %d. Skipping this result.\n", i);
                continue;
            }
            
            total_home_goals += res.home_goals;
            total_away_goals += res.away_goals;
            total_home_wins += res.win;
            total_draws += res.draw;
            total_home_losses += res.loss;
        }
        close(result_pipes[i][0]);
    }

    int status;
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (wait(&status) == -1) {
            perror("wait");
            exit(1);
        }

        if (WIFEXITED(status)) {
            printf("Worker %d exited with status %d\n", i, WEXITSTATUS(status));
        }
    }

    double average_home_goals = (double)total_home_goals / num_simulations;
    double average_away_goals = (double)total_away_goals / num_simulations;
    printf("Average scoreline after %d simulations:\n", num_simulations);
    printf("%s %.2f - %.2f %s\n", home->name, average_home_goals, average_away_goals, away->name);
    printf("%s won %d times, %s won %d times, and they drew %d times.\n", home->name, total_home_wins, away->name, total_home_losses, total_draws);

    return 0;
}