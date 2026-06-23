#ifndef STUDYSMART_DP_H
#define STUDYSMART_DP_H

#include "definition.h"

static void ss_dp_run(const Scenario *scenario, AlgorithmResult *result) {
  ss_result_init(result, STRATEGY_DP);
  if (scenario == NULL || result == NULL) {
    return;
  }

  int n = scenario->taskCount;
  int capacity = scenario->availableTime;
  if (capacity < 0) {
    capacity = 0;
  }
  if (capacity > SS_MAX_TIME_CAPACITY) {
    capacity = SS_MAX_TIME_CAPACITY;
  }

  static int dp[SS_MAX_TASKS + 1][SS_MAX_TIME_CAPACITY + 1];
  static unsigned char take[SS_MAX_TASKS + 1][SS_MAX_TIME_CAPACITY + 1];

  for (int i = 0; i <= n; i++) {
    for (int w = 0; w <= capacity; w++) {
      dp[i][w] = 0;
      take[i][w] = 0;
    }
  }

  /*
   * 0/1 knapsack: capacity is available study time, value is importance.
   * Each task can be selected at most once.
   */
  for (int i = 1; i <= n; i++) {
    const Task *task = &scenario->tasks[i - 1];
    for (int w = 0; w <= capacity; w++) {
      int excludeValue = dp[i - 1][w];
      int includeValue = -1;

      if (task->studyTime <= w) {
        includeValue = task->importance + dp[i - 1][w - task->studyTime];
      }

      if (includeValue > excludeValue) {
        dp[i][w] = includeValue;
        take[i][w] = 1;
      } else {
        dp[i][w] = excludeValue;
      }
    }
  }

  Task reversed[SS_MAX_TASKS];
  int reversedCount = 0;
  int w = capacity;

  for (int i = n; i > 0; i--) {
    if (take[i][w]) {
      const Task *task = &scenario->tasks[i - 1];
      reversed[reversedCount++] = *task;
      w -= task->studyTime;
    }
  }

  for (int i = reversedCount - 1; i >= 0; i--) {
    result->orderedTasks[result->orderedCount++] = reversed[i];
    ss_result_add_selected(result, &reversed[i]);
  }
}

#endif
