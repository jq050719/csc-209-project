#ifndef COMMON_H
#define COMMON_H

#define NUM_WORKERS 3
#define MAX_TEAM_NAME 40

typedef struct {
    char name[MAX_TEAM_NAME];
    double attack;
    double midfield;
    double defense;
} Team;


// task sent from parent to worker
typedef struct {
    int sim_id;
    double home_chance_prob;
    double home_goal_prob;
    double away_chance_prob;
    double away_goal_prob;
    char *home_name;
    char *away_name;
} SimulationTask;


// result sent from worker to parent
typedef struct {
    int sim_id;
    int home_goals;
    int away_goals;
    int win;
    int draw;
    int loss;
} SimulationResult;

void run_worker(int read_fd, int write_fd, int worker_id);

#endif // COMMON_H
