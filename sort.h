#ifndef STUDYSMART_SORT_H
#define STUDYSMART_SORT_H

#include "definition.h"

static void ss_sort_merge(Task tasks[], Task temp[], int left, int mid,
                          int right) {
  int i = left;
  int j = mid + 1;
  int k = left;

  while (i <= mid && j <= right) {
    if (ss_task_priority_cmp(&tasks[i], &tasks[j]) <= 0) {
      temp[k++] = tasks[i++];
    } else {
      temp[k++] = tasks[j++];
    }
  }

  while (i <= mid) {
    temp[k++] = tasks[i++];
  }
  while (j <= right) {
    temp[k++] = tasks[j++];
  }
  for (i = left; i <= right; i++) {
    tasks[i] = temp[i];
  }
}

static void ss_sort_merge_sort_range(Task tasks[], Task temp[], int left,
                                     int right) {
  if (left >= right) {
    return;
  }

  int mid = left + (right - left) / 2;
  ss_sort_merge_sort_range(tasks, temp, left, mid);
  ss_sort_merge_sort_range(tasks, temp, mid + 1, right);
  ss_sort_merge(tasks, temp, left, mid, right);
}

static void ss_sort_run(const Scenario *scenario, AlgorithmResult *result) {
  ss_result_init(result, STRATEGY_SORTING);
  if (scenario == NULL || result == NULL) {
    return;
  }

  result->orderedCount = scenario->taskCount;
  for (int i = 0; i < scenario->taskCount; i++) {
    result->orderedTasks[i] = scenario->tasks[i];
  }

  if (result->orderedCount > 1) {
    Task temp[SS_MAX_TASKS];
    ss_sort_merge_sort_range(result->orderedTasks, temp, 0,
                             result->orderedCount - 1);
  }

  /*
   * The sorting module primarily ranks tasks. For the comparison table, it
   * uses the ranked prefix until the next task would exceed the time budget.
   */
  for (int i = 0; i < result->orderedCount; i++) {
    Task *task = &result->orderedTasks[i];
    if (result->totalStudyTime + task->studyTime > scenario->availableTime) {
      break;
    }
    ss_result_add_selected(result, task);
  }
}

#endif
