#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "common.h"

// get random probability between 0 and 100
double get_random_double() {
    return ((double)rand() / RAND_MAX) * 100.0;
}

void run_worker(int read_fd, int write_fd, int worker_id) {
    srand(time(NULL)); // seed random number generator with unique value

    SimulationTask task;

    ssize_t bytes_read;
    
    while ((bytes_read = read(read_fd, &task, sizeof(SimulationTask))) > 0) {
        if (bytes_read != sizeof(SimulationTask)) {
            fprintf(stderr, "Worker %d: Error reading task. Skipping this simulation.\n", worker_id);
            continue;
        }

         // simulate match based on probabilities
        int home_goals = 0;
        int away_goals = 0;
        int stoppage = 0;
        for (int i = 0; i < 90; i++) {
            double fatigue_modifier = 1.0 - (i / 360.0);
            if (get_random_double() <= task.home_chance_prob * fatigue_modifier) {
                if (get_random_double() <= task.home_goal_prob * fatigue_modifier) {
                    home_goals++;
                    stoppage++;
                }
            }
            else if (get_random_double() <= task.away_chance_prob * fatigue_modifier) {
                if (get_random_double() <= task.away_goal_prob * fatigue_modifier) {
                    away_goals++;
                    stoppage++;
                }
            }
        }
        // add stoppage time goals
        for (int i = 0; i < stoppage; i++) {
            double fatigue_modifier = 1.0 - ((90.0 + i) / 360.0);
            if (get_random_double() <= task.home_chance_prob * fatigue_modifier) {
                if (get_random_double() <= task.home_goal_prob * fatigue_modifier) {
                    home_goals++;
                }
            }
            else if (get_random_double() <= task.away_chance_prob * fatigue_modifier) {
                if (get_random_double() <= task.away_goal_prob * fatigue_modifier) {
                    away_goals++;
                }
            }
        }

        SimulationResult result;
        result.home_goals = home_goals;
        result.away_goals = away_goals;
        result.sim_id = task.sim_id;
        if (home_goals > away_goals) {
            result.win = 1;
            result.draw = 0;
            result.loss = 0;
        }
        else if (home_goals < away_goals) {
            result.win = 0;
            result.draw = 0;
            result.loss = 1;
        }
        else {
            result.win = 0;
            result.draw = 1;
            result.loss = 0;
        }
        if (write(write_fd, &result, sizeof(SimulationResult)) == -1) {
            perror("write");
            exit(1);
        }

        printf("WORKER %d completed sim %d: %s %d - %d %s\n", worker_id, task.sim_id, task.home_name, home_goals, away_goals, task.away_name);
    }

    close(read_fd);
    close(write_fd);
    exit(0);
}