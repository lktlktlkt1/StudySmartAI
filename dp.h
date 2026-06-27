#ifndef STUDYSMART_DP_H
#define STUDYSMART_DP_H

#include "definition.h"

static void ss_dp_run(const Scenario *scenario, AlgorithmResult *result) {
  ss_result_init(result, STRATEGY_DP);
  if (scenario == NULL || result == NULL) {
    return;
  }

  /* Core Logic: Treat study time as the capacity of a backpack, and the importance of a task as the value of an item */
  int n = scenario->taskCount;
  int capacity = scenario->availableTime;
  
  
  if (capacity < 0) {
    capacity = 0;
  }
  if (capacity > SS_MAX_TIME_CAPACITY) {
    capacity = SS_MAX_TIME_CAPACITY;
  }

  /* dp[i][w] represents the maximum total importance that can be achieved in the first i tasks with a total duration of no more than w.
   * take[i][w] is used to indicate whether the current state has selected the i-th task, to facilitate subsequent backtracking path recovery
   */
  static int dp[SS_MAX_TASKS + 1][SS_MAX_TIME_CAPACITY + 1];
  static unsigned char take[SS_MAX_TASKS + 1][SS_MAX_TIME_CAPACITY + 1];

  for (int i = 0; i <= n; i++) {
    for (int w = 0; w <= capacity; w++) {
      dp[i][w] = 0;
      take[i][w] = 0;
    }
  }

  /* For each task, there are two possible decisions: include or exclude.
   */
  for (int i = 1; i <= n; i++) {
    const Task *task = &scenario->tasks[i - 1];
    for (int w = 0; w <= capacity; w++) {
      /* Option 1: Do not select the current task; 
      *the value is equal to the optimal solution for the first i-1 tasks at the same time. 
      */
      int excludeValue = dp[i - 1][w];
      int includeValue = -1;

      /* Option 2: Try selecting the current task;
      *provided that the remaining time, w, is sufficient to cover the time required for that task
      */
      if (task->studyTime <= w) {
        includeValue = task->importance + dp[i - 1][w - task->studyTime];
      }

      /* Take the greater of the two values and update the path record */
      if (includeValue > excludeValue) {
        dp[i][w] = includeValue;
        take[i][w] = 1;
      } else {
        dp[i][w] = excludeValue;
      }
    }
  }

  /* Trace back from the final state to determine which tasks were selected */
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

  /* Since the backtracking is stored from the end to the beginning, 
  *it needs to be reversed to restore the original task order. 
  */
  for (int i = reversedCount - 1; i >= 0; i--) {
    result->orderedTasks[result->orderedCount++] = reversed[i];
    ss_result_add_selected(result, &reversed[i]);
  }
}

#endif
