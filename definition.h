#ifndef STUDYSMART_DEFINITION_H
#define STUDYSMART_DEFINITION_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define SS_MAX_TASKS 64
#define SS_MAX_SCENARIOS 16
#define SS_MAX_NAME_LEN 128
#define SS_MAX_TYPE_LEN 32
#define SS_MAX_SCENARIO_LEN 96
#define SS_MAX_TIME_CAPACITY 1000

typedef enum StrategyType {
  STRATEGY_SORTING = 0,
  STRATEGY_GREEDY,
  STRATEGY_DP,
  STRATEGY_AI_RECOMMENDATION,
  STRATEGY_BACKTRACKING
} StrategyType;

typedef struct Task {
  int id;
  char name[SS_MAX_NAME_LEN];
  int studyTime;
  int importance;
  int deadline;
  int difficulty;
  char taskType[SS_MAX_TYPE_LEN];
} Task;

typedef struct Scenario {
  char name[SS_MAX_SCENARIO_LEN];
  Task tasks[SS_MAX_TASKS];
  int taskCount;
  int availableTime;
} Scenario;

typedef struct ScenarioFeatures {
  int numTasks;
  int totalRequiredTime;
  int availableTime;
  int timePressureRatio;
  int averageImportance;
  int deadlineTightness;
  int importanceVariation;
} ScenarioFeatures;

typedef struct AlgorithmResult {
  StrategyType strategy;
  StrategyType recommendedStrategy;
  Task orderedTasks[SS_MAX_TASKS];
  int orderedCount;
  Task selectedTasks[SS_MAX_TASKS];
  int selectedCount;
  int totalStudyTime;
  int totalImportance;
  double elapsedMs;
  ScenarioFeatures features;
} AlgorithmResult;

static const char *ss_strategy_name(StrategyType strategy) {
  switch (strategy) {
  case STRATEGY_SORTING:
    return "Sorting-based ranking";
  case STRATEGY_GREEDY:
    return "Greedy strategy";
  case STRATEGY_DP:
    return "Dynamic Programming";
  case STRATEGY_AI_RECOMMENDATION:
    return "AI/ML recommendation";
  case STRATEGY_BACKTRACKING:
    return "Backtracking";
  default:
    return "Unknown";
  }
}

static double ss_task_ratio(const Task *task) {
  if (task == NULL || task->studyTime <= 0) {
    return 0.0;
  }
  return (double)task->importance / (double)task->studyTime;
}

static int ss_task_priority_cmp(const Task *left, const Task *right) {
  double leftRatio = ss_task_ratio(left);
  double rightRatio = ss_task_ratio(right);

  if (leftRatio > rightRatio) {
    return -1;
  }
  if (leftRatio < rightRatio) {
    return 1;
  }
  if (left->importance != right->importance) {
    return (left->importance > right->importance) ? -1 : 1;
  }
  if (left->deadline != right->deadline) {
    return (left->deadline < right->deadline) ? -1 : 1;
  }
  if (left->studyTime != right->studyTime) {
    return (left->studyTime < right->studyTime) ? -1 : 1;
  }
  if (left->id != right->id) {
    return (left->id < right->id) ? -1 : 1;
  }
  return 0;
}

static void ss_result_init(AlgorithmResult *result, StrategyType strategy) {
  if (result == NULL) {
    return;
  }
  memset(result, 0, sizeof(*result));
  result->strategy = strategy;
  result->recommendedStrategy = strategy;
}

static void ss_result_add_selected(AlgorithmResult *result, const Task *task) {
  if (result == NULL || task == NULL || result->selectedCount >= SS_MAX_TASKS) {
    return;
  }
  result->selectedTasks[result->selectedCount++] = *task;
  result->totalStudyTime += task->studyTime;
  result->totalImportance += task->importance;
}

#endif
